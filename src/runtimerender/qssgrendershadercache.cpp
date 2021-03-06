/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qssgrendershadercache_p.h"

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderinputstreamfactory_p.h>
#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>

#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtCore/qfile.h>

#include <QtGui/qsurfaceformat.h>

#include <QtGui/qopenglcontext.h>

#include <QtShaderTools/private/qshaderbaker_p.h>

QT_BEGIN_NAMESPACE

static const char *defineTable[QSSGShaderDefines::Count] {
    "QSSG_ENABLE_LIGHT_PROBE",
    "QSSG_ENABLE_LIGHT_PROBE_2",
    "QSSG_ENABLE_IBL_FOV",
    "QSSG_ENABLE_SSM",
    "QSSG_ENABLE_SSAO",
    "QSSG_ENABLE_SSDO",
    "QSSG_ENABLE_CG_LIGHTING",
    "QSSG_ENABLE_RHI"
};

const char *QSSGShaderDefines::asString(QSSGShaderDefines::Define def) { return defineTable[def]; }

size_t qHash(const QSSGShaderCacheKey &key)
{
    return key.m_hashCode;
}

size_t hashShaderFeatureSet(const ShaderFeatureSetList &inFeatureSet)
{
    size_t retval(0);
    for (int idx = 0, end = inFeatureSet.size(); idx < end; ++idx) {
        // From previous implementation, it seems we need to ignore the order of the features.
        // But we need to bind the feature flag together with its name, so that the flags will
        // influence
        // the final hash not only by the true-value count.
        retval ^= (inFeatureSet.at(idx).key ^ size_t(inFeatureSet.at(idx).enabled));
    }
    return retval;
}

QSSGShaderCache::~QSSGShaderCache() {}

QSSGRef<QSSGShaderCache> QSSGShaderCache::createShaderCache(const QSSGRef<QSSGRhiContext> &inContext,
                                                                  const QSSGRef<QSSGInputStreamFactory> &inInputStreamFactory,
                                                                  QSSGPerfTimer *inPerfTimer)
{
    return QSSGRef<QSSGShaderCache>(new QSSGShaderCache(inContext, inInputStreamFactory, inPerfTimer));
}

QSSGShaderCache::QSSGShaderCache(const QSSGRef<QSSGRhiContext> &ctx, const QSSGRef<QSSGInputStreamFactory> &inInputStreamFactory, QSSGPerfTimer *)
    : m_rhiContext(ctx)
    , m_inputStreamFactory(inInputStreamFactory)
{
}

QSSGRef<QSSGRhiShaderStages> QSSGShaderCache::getRhiShaderStages(const QByteArray &inKey, const ShaderFeatureSetList &inFeatures)
{
    m_tempKey.m_key = inKey;
    m_tempKey.m_features = inFeatures;
    m_tempKey.generateHashCode();
    const auto theIter = m_rhiShaders.constFind(m_tempKey);
    if (theIter != m_rhiShaders.cend())
        return theIter.value();
    return nullptr;
}


void QSSGShaderCache::addRhiShaderPreprocessor(QByteArray &str,
                                               const QByteArray &inKey,
                                               ShaderType shaderType,
                                               const ShaderFeatureSetList &inFeatures)
{
    m_insertStr.clear();

    m_insertStr += "#version 440\n";

    if (!inKey.isNull()) {
        m_insertStr += "//Shader name -";
        m_insertStr += inKey;
        m_insertStr += "\n";
    }

    m_insertStr += "#define texture2D texture\n";

    str.insert(0, m_insertStr);
    QString::size_type insertPos = int(m_insertStr.size());

    if (inFeatures.size()) {
        m_insertStr.clear();
        for (int idx = 0, end = inFeatures.size(); idx < end; ++idx) {
            QSSGShaderPreprocessorFeature feature(inFeatures[idx]);
            m_insertStr.append("#define ");
            m_insertStr.append(inFeatures[idx].name);
            m_insertStr.append(" ");
            m_insertStr.append(feature.enabled ? "1" : "0");
            m_insertStr.append("\n");
        }
        str.insert(insertPos, m_insertStr);
        insertPos += int(m_insertStr.size());
    }

    m_insertStr.clear();
    if (shaderType == ShaderType::Fragment) {
        m_insertStr += "#ifndef NO_FRAG_OUTPUT\n";
        m_insertStr += "layout(location = 0) out vec4 fragOutput;\n";
        m_insertStr += "#endif\n";
    }
    str.insert(insertPos, m_insertStr);
}

void QSSGShaderCache::addShaderPreprocessor(QByteArray &str, const QByteArray &inKey, ShaderType shaderType, const ShaderFeatureSetList &inFeatures)
{
    addRhiShaderPreprocessor(str, inKey, shaderType, inFeatures);
}

static void initBaker(QShaderBaker *baker, QRhi::Implementation target)
{
    QVector<QShaderBaker::GeneratedShader> outputs;
    switch (target) {
    case QRhi::D3D11:
        outputs.append({ QShader::HlslShader, QShaderVersion(50) }); // Shader Model 5.0
        break;
    case QRhi::Metal:
        outputs.append({ QShader::MslShader, QShaderVersion(20) }); // Metal 2.0 (required for array of textures (custom materials))
        break;
    case QRhi::OpenGLES2:
    {
        const QSurfaceFormat format = QSurfaceFormat::defaultFormat();
        if (format.profile() == QSurfaceFormat::CoreProfile) {
            outputs.append({ QShader::GlslShader, QShaderVersion(330) }); // OpenGL 3.3+
        } else {
            if (format.renderableType() == QSurfaceFormat::OpenGLES || QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGLES) {
                if (format.majorVersion() >= 3)
                    outputs.append({ QShader::GlslShader, QShaderVersion(300, QShaderVersion::GlslEs) }); // GLES 3.0+
                else
                    outputs.append({ QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs) }); // GLES 2.0
            } else {
                outputs.append({ QShader::GlslShader, QShaderVersion(120) }); // OpenGL 2.1
            }
        }
    }
        break;
    default: // Vulkan, Null
        outputs.append({ QShader::SpirvShader, QShaderVersion(100) });
        break;
    }

    baker->setGeneratedShaders(outputs);
    baker->setGeneratedShaderVariants({ QShader::StandardShader });
}

QSSGRef<QSSGRhiShaderStages> QSSGShaderCache::compileForRhi(const QByteArray &inKey, const QByteArray &inVert, const QByteArray &inFrag,
                                                            QSSGShaderCacheProgramFlags inFlags, const ShaderFeatureSetList &inFeatures)
{
    Q_UNUSED(inFlags);

    const QSSGRef<QSSGRhiShaderStages> &rhiShaders = getRhiShaderStages(inKey, inFeatures);
    if (rhiShaders)
        return rhiShaders;

    QSSGShaderCacheKey tempKey(inKey);
    tempKey.m_features = inFeatures;
    tempKey.generateHashCode();

    m_vertexCode = inVert;
    m_fragmentCode = inFrag;

    if (!m_vertexCode.isEmpty())
        addShaderPreprocessor(m_vertexCode, inKey, ShaderType::Vertex, inFeatures);

    if (!m_fragmentCode.isEmpty())
        addShaderPreprocessor(m_fragmentCode, inKey, ShaderType::Fragment, inFeatures);

    // lo and behold the final shader strings are ready

    QSSGRef<QSSGRhiShaderStages> shaders;
    bool valid = true;

    QShaderBaker baker;
    initBaker(&baker, m_rhiContext->rhi()->backend());

   static const bool shaderDebug = qEnvironmentVariableIntValue("QT_RHI_SHADER_DEBUG");

    if (shaderDebug) {
        qDebug("VERTEX SHADER:\n*****\n");
        QList<QByteArray> lines = m_vertexCode.split('\n');
        for (int i = 0; i < lines.size(); i++)
            qDebug("%3d  %s", i + 1, lines.at(i).constData());
        qDebug("\n*****\n");
    }
    baker.setSourceString(m_vertexCode, QShader::VertexStage);
    QShader vertexShader = baker.bake();
    if (!vertexShader.isValid()) {
        const QString err = baker.errorMessage();
        qWarning("Failed to compile vertex shader: %s", qPrintable(err));
        valid = false;
        if (shaderDebug) {
            QFile f(QLatin1String("failedvert.txt"));
            f.open(QIODevice::WriteOnly | QIODevice::Text);
            f.write(m_vertexCode);
            f.close();
        }
    }

    if (shaderDebug) {
        qDebug("FRAGMENT SHADER:\n*****\n");
        QList<QByteArray> lines = m_fragmentCode.split('\n');
        for (int i = 0; i < lines.size(); i++)
            qDebug("%3d  %s", i + 1, lines.at(i).constData());
        qDebug("\n*****\n");
    }
    baker.setSourceString(m_fragmentCode, QShader::FragmentStage);
    QShader fragmentShader = baker.bake();
    if (!fragmentShader.isValid()) {
        const QString err = baker.errorMessage();
        qWarning("Failed to compile fragment shader: %s", qPrintable(err));
        valid = false;
        if (shaderDebug) {
            QFile f(QLatin1String("failedfrag.txt"));
            f.open(QIODevice::WriteOnly | QIODevice::Text);
            f.write(m_fragmentCode);
            f.close();
        }
    }

    if (valid) {
        shaders = new QSSGRhiShaderStages(*m_rhiContext.data());
        shaders->addStage(QRhiShaderStage(QRhiShaderStage::Vertex, vertexShader));
        shaders->addStage(QRhiShaderStage(QRhiShaderStage::Fragment, fragmentShader));
        if (shaderDebug)
            qDebug("Compilation for vertex and fragment stages succeeded");
    }

    const auto inserted = m_rhiShaders.insert(tempKey, shaders);
    return inserted.value();
}

QSSGRef<QSSGRhiShaderStages> QSSGShaderCache::loadBuiltinForRhi(const QByteArray &inKey)
{
    const QSSGRef<QSSGRhiShaderStages> &rhiShaders = getRhiShaderStages(inKey, ShaderFeatureSetList());
    if (rhiShaders)
        return rhiShaders;

    qDebug("Loading builtin rhi shader: %s", inKey.constData());

    // Note that we are required to return a non-null (but empty) shader set even if loading fails.
    QSSGRef<QSSGRhiShaderStages> shaders(new QSSGRhiShaderStages(*m_rhiContext.data()));

    // inShaderName is a prefix of a .qsb file, so "abc" means we should
    // look for abc.vert.qsb and abc.frag.qsb.

    const QString prefix = QLatin1String(":/res/rhishaders/") + QString::fromUtf8(inKey);
    const QString vertexFileName = prefix + QLatin1String(".vert.qsb");
    const QString fragmentFileName = prefix + QLatin1String(".frag.qsb");

    QShader vertexShader;
    QShader fragmentShader;

    QFile f;
    f.setFileName(vertexFileName);
    if (f.open(QIODevice::ReadOnly)) {
        const QByteArray vsData = f.readAll();
        vertexShader = QShader::fromSerialized(vsData);
        f.close();
    } else {
        qWarning("Failed to open %s", qPrintable(f.fileName()));
    }
    f.setFileName(fragmentFileName);
    if (f.open(QIODevice::ReadOnly)) {
        const QByteArray fsData = f.readAll();
        fragmentShader = QShader::fromSerialized(fsData);
        f.close();
    } else {
        qWarning("Failed to open %s", qPrintable(f.fileName()));
    }

    if (vertexShader.isValid() && fragmentShader.isValid()) {
        shaders->addStage(QRhiShaderStage(QRhiShaderStage::Vertex, vertexShader));
        shaders->addStage(QRhiShaderStage(QRhiShaderStage::Fragment, fragmentShader));
        qDebug("Loading of vertex and fragment stages succeeded");
    }

    QSSGShaderCacheKey cacheKey(inKey);
    cacheKey.m_features = ShaderFeatureSetList();
    cacheKey.generateHashCode();

    const auto inserted = m_rhiShaders.insert(cacheKey, shaders);
    return inserted.value();
}

QT_END_NAMESPACE
