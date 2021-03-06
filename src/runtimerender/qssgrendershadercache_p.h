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

#ifndef QSSG_RENDER_SHADER_CACHE_H
#define QSSG_RENDER_SHADER_CACHE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <QtQuick3DUtils/private/qssgdataref_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderinputstreamfactory_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrhicontext_p.h>

#include <QtCore/QString>

#include <QtCore/QSharedPointer>
#include <QtCore/QVector>

QT_BEGIN_NAMESPACE

class QSSGRhiShaderStages;
class QSSGInputStreamFactory;
class QSSGPerfTimer;

enum class ShaderCacheProgramFlagValues : quint32
{
    TessellationEnabled = 1 << 0, // tessellation enabled
    GeometryShaderEnabled = 1 << 1, // geometry shader enabled
};

Q_DECLARE_FLAGS(QSSGShaderCacheProgramFlags, ShaderCacheProgramFlagValues)

namespace QSSGShaderDefines
{
enum Define : quint8
{
    LightProbe = 0,
    LightProbe2,
    IblFov,
    Ssm,
    Ssao,
    Ssdo,
    CgLighting,
    Rhi,
    Count /* New defines are added before this one! */
};

const char *asString(QSSGShaderDefines::Define def);
}

// There are a number of macros used to turn on or off various features.  This allows those
// features
// to be propagated into the shader cache's caching mechanism.  They will be translated into
//#define name value where value is 1 or zero depending on if the feature is enabled or not.
struct QSSGShaderPreprocessorFeature
{
    QByteArray name;
    size_t key = 0;
    mutable bool enabled = false;
    QSSGShaderPreprocessorFeature() = default;
    QSSGShaderPreprocessorFeature(const QByteArray &inName, bool val) : name(inName), enabled(val)
    {
        static const size_t qhashSeed = 0xfee383a1;
        Q_ASSERT(inName != nullptr);
        key = qHash(inName, qhashSeed);
    }
    inline bool operator<(const QSSGShaderPreprocessorFeature &other) const Q_DECL_NOTHROW { return name < other.name; }
    inline bool operator==(const QSSGShaderPreprocessorFeature &other) const Q_DECL_NOTHROW { return name == other.name && enabled == other.enabled; }
};

using ShaderFeatureSetList = QVarLengthArray<QSSGShaderPreprocessorFeature, QSSGShaderDefines::Count>;

inline const ShaderFeatureSetList shaderCacheNoFeatures()
{
    return ShaderFeatureSetList();
}

// Hash is dependent on the order of the keys; so make sure their order is consistent!!
size_t hashShaderFeatureSet(const ShaderFeatureSetList &inFeatureSet);

struct QSSGShaderCacheKey
{
    QByteArray m_key;
    ShaderFeatureSetList m_features;
    size_t m_hashCode = 0;

    explicit QSSGShaderCacheKey(const QByteArray &key = QByteArray()) : m_key(key), m_hashCode(0) {}

    QSSGShaderCacheKey(const QSSGShaderCacheKey &other) = default;
    QSSGShaderCacheKey &operator=(const QSSGShaderCacheKey &other) = default;

    void generateHashCode()
    {
        m_hashCode = qHash(m_key);
        m_hashCode = m_hashCode ^ hashShaderFeatureSet(m_features);
    }

    bool operator==(const QSSGShaderCacheKey &inOther) const
    {
        return m_key == inOther.m_key && m_features == inOther.m_features;
    }
};


class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGShaderCache
{
    enum class ShaderType
    {
        Vertex, Fragment, Geometry, Compute
    };

public:
    QAtomicInt ref;
private:
    typedef QHash<QSSGShaderCacheKey, QSSGRef<QSSGRhiShaderStages>> TRhiShaderMap;
    QSSGRef<QSSGRhiContext> m_rhiContext;
    TRhiShaderMap m_rhiShaders;
    QString m_cacheFilePath;
    QByteArray m_vertexCode;
    QByteArray m_geometryCode;
    QByteArray m_fragmentCode;
    QByteArray m_insertStr;
    QString m_flagString;
    QString m_contextTypeString;
    QSSGShaderCacheKey m_tempKey;

    QSSGRef<QSSGInputStreamFactory> m_inputStreamFactory;

    void addRhiShaderPreprocessor(QByteArray &str,
                                      const QByteArray &inKey,
                                      ShaderType shaderType,
                                      const ShaderFeatureSetList &inFeatures);

    void addShaderPreprocessor(QByteArray &str,
                               const QByteArray &inKey,
                               ShaderType shaderType,
                               const ShaderFeatureSetList &inFeatures);

public:
    QSSGShaderCache(const QSSGRef<QSSGRhiContext> &ctx,
                const QSSGRef<QSSGInputStreamFactory> &inInputStreamFactory,
                QSSGPerfTimer *inPerfTimer);
    ~QSSGShaderCache();

    QSSGRef<QSSGRhiShaderStages> getRhiShaderStages(const QByteArray &inKey,
                                                    const ShaderFeatureSetList &inFeatures);

    QSSGRef<QSSGRhiShaderStages> compileForRhi(const QByteArray &inKey,
                                               const QByteArray &inVert,
                                               const QByteArray &inFrag,
                                               QSSGShaderCacheProgramFlags inFlags,
                                               const ShaderFeatureSetList &inFeatures);

    QSSGRef<QSSGRhiShaderStages> loadBuiltinForRhi(const QByteArray &inKey);

    static QSSGRef<QSSGShaderCache> createShaderCache(const QSSGRef<QSSGRhiContext> &inContext,
                                                      const QSSGRef<QSSGInputStreamFactory> &inInputStreamFactory,
                                                      QSSGPerfTimer *inPerfTimer);
};

QT_END_NAMESPACE

#endif
