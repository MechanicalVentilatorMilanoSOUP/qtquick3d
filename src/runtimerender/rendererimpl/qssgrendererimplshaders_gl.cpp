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

#include <QtQuick3DRuntimeRender/private/qssgrendererimpl_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdynamicobjectsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegeneratorv2_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgvertexpipelineimpl_p.h>

// this file contains the getXxxxShader implementations suitable for the legacy direct OpenGL path

QT_BEGIN_NAMESPACE

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getCubeShadowDepthShader(TessellationModeValues inTessMode)
{
    if (!m_contextInterface->renderContext()->supportsTessellation() || inTessMode == TessellationModeValues::NoTessellation) {
        return getCubeDepthNoTessShader();
    } else if (inTessMode == TessellationModeValues::Linear) {
        return getCubeDepthTessLinearShader();
    } else if (inTessMode == TessellationModeValues::Phong) {
        return getCubeDepthTessPhongShader();
    } else if (inTessMode == TessellationModeValues::NPatch) {
        return getCubeDepthTessNPatchShader();
    }

    return getCubeDepthNoTessShader();
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getCubeDepthNoTessShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_cubemapDepthShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "cubemap face depth shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());

        if (!depthShaderProgram) {
            // GetProgramGenerator()->BeginProgram(
            // TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex |
            // ShaderGeneratorStages::Fragment | ShaderGeneratorStages::Geometry) );
            getProgramGenerator()->beginProgram();
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            // IShaderStageGenerator& geometryShader( *GetProgramGenerator()->GetStage(
            // ShaderGeneratorStages::Geometry ) );

            QSSGShaderProgramGeneratorInterface::outputCubeFaceDepthVertex(vertexShader);
            // IShaderProgramGenerator::OutputCubeFaceDepthGeometry( geometryShader );
            QSSGShaderProgramGeneratorInterface::outputCubeFaceDepthFragment(fragmentShader);
        } else if (theCache->isShaderCachePersistenceEnabled()) {
            // we load from shader cache set default shader stages
            getProgramGenerator()->beginProgram();
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getCubeDepthTessLinearShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_cubemapDepthTessLinearShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "cubemap face depth linear tess shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());

        if (!depthShaderProgram) {
            // GetProgramGenerator()->BeginProgram(
            // TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex |
            // ShaderGeneratorStages::Fragment | ShaderGeneratorStages::Geometry) );
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            // IShaderStageGenerator& geometryShader( *GetProgramGenerator()->GetStage(
            // ShaderGeneratorStages::Geometry ) );
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));

            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("}");

            // IShaderProgramGenerator::OutputCubeFaceDepthGeometry( geometryShader );
            QSSGShaderProgramGeneratorInterface::outputCubeFaceDepthFragment(fragmentShader);

            tessCtrlShader.addInclude("tessellationLinear.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationLinear.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.addUniform("modelMatrix", "mat4");
            tessEvalShader.addOutgoing("world_pos", "vec4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            tessEvalShader.append("\tworld_pos = modelMatrix * pos;");
            tessEvalShader.append("\tworld_pos /= world_pos.w;");
            tessEvalShader.append("\tgl_Position = modelViewProjection * pos;");
            tessEvalShader.append("}");
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getCubeDepthTessPhongShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_cubemapDepthTessPhongShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "cubemap face depth phong tess shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());

        if (!depthShaderProgram) {
            // GetProgramGenerator()->BeginProgram(
            // TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex |
            // ShaderGeneratorStages::Fragment | ShaderGeneratorStages::Geometry) );
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            // IShaderStageGenerator& geometryShader( *GetProgramGenerator()->GetStage(
            // ShaderGeneratorStages::Geometry ) );
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));

            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addIncoming("attr_norm", "vec3");
            vertexShader.addOutgoing("outNormal", "vec3");
            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("\toutNormal = attr_norm;");
            vertexShader.append("}");

            // IShaderProgramGenerator::OutputCubeFaceDepthGeometry( geometryShader );
            QSSGShaderProgramGeneratorInterface::outputCubeFaceDepthFragment(fragmentShader);

            tessCtrlShader.addInclude("tessellationPhong.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("\tctNorm[0] = outNormal[0];");
            tessCtrlShader.append("\tctNorm[1] = outNormal[1];");
            tessCtrlShader.append("\tctNorm[2] = outNormal[2];");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationPhong.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.addUniform("modelMatrix", "mat4");
            tessEvalShader.addOutgoing("world_pos", "vec4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            tessEvalShader.append("\tworld_pos = modelMatrix * pos;");
            tessEvalShader.append("\tworld_pos /= world_pos.w;");
            tessEvalShader.append("\tgl_Position = modelViewProjection * pos;");
            tessEvalShader.append("}");
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getCubeDepthTessNPatchShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_cubemapDepthTessNPatchShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "cubemap face depth npatch tess shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());

        if (!depthShaderProgram) {
            // GetProgramGenerator()->BeginProgram(
            // TShaderGeneratorStageFlags(ShaderGeneratorStages::Vertex |
            // ShaderGeneratorStages::Fragment | ShaderGeneratorStages::Geometry) );
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            // IShaderStageGenerator& geometryShader( *GetProgramGenerator()->GetStage(
            // ShaderGeneratorStages::Geometry ) );
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));

            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addIncoming("attr_norm", "vec3");
            vertexShader.addOutgoing("outNormal", "vec3");
            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("\toutNormal = attr_norm;");
            vertexShader.append("}");

            // IShaderProgramGenerator::OutputCubeFaceDepthGeometry( geometryShader );
            QSSGShaderProgramGeneratorInterface::outputCubeFaceDepthFragment(fragmentShader);

            tessCtrlShader.addOutgoing("outNormalTC", "vec3");
            tessCtrlShader.addInclude("tessellationNPatch.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("\tctNorm[0] = outNormal[0];");
            tessCtrlShader.append("\tctNorm[1] = outNormal[1];");
            tessCtrlShader.append("\tctNorm[2] = outNormal[2];");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("\toutNormalTC[gl_InvocationID] = outNormal[gl_InvocationID];\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationNPatch.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.addUniform("modelMatrix", "mat4");
            tessEvalShader.addOutgoing("world_pos", "vec4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tctNorm[0] = outNormalTC[0];");
            tessEvalShader.append("\tctNorm[1] = outNormalTC[1];");
            tessEvalShader.append("\tctNorm[2] = outNormalTC[2];");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            tessEvalShader.append("\tworld_pos = modelMatrix * pos;");
            tessEvalShader.append("\tworld_pos /= world_pos.w;");
            tessEvalShader.append("\tgl_Position = modelViewProjection * pos;");
            tessEvalShader.append("}");
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getOrthographicDepthShader(TessellationModeValues inTessMode)
{
    if (!m_contextInterface->renderContext()->supportsTessellation() || inTessMode == TessellationModeValues::NoTessellation) {
        return getOrthographicDepthNoTessShader();
    } else if (inTessMode == TessellationModeValues::Linear) {
        return getOrthographicDepthTessLinearShader();
    } else if (inTessMode == TessellationModeValues::Phong) {
        return getOrthographicDepthTessPhongShader();
    } else if (inTessMode == TessellationModeValues::NPatch) {
        return getOrthographicDepthTessNPatchShader();
    }

    return getOrthographicDepthNoTessShader();
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getOrthographicDepthNoTessShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_orthographicDepthShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "orthographic depth shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram();
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addUniform("modelViewProjection", "mat4");
            vertexShader.addOutgoing("outDepth", "vec3");
            vertexShader.append("void main() {");
            vertexShader.append("   gl_Position = modelViewProjection * vec4( attr_pos, 1.0 );");
            vertexShader.append("   outDepth.x = gl_Position.z / gl_Position.w;");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("\tfloat depth = (outDepth.x + 1.0) * 0.5;");
            fragmentShader.append("\tfragOutput = vec4(depth);");
            fragmentShader.append("}");
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getOrthographicDepthTessLinearShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_orthographicDepthTessLinearShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "orthographic depth tess linear shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));

            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addUniform("modelViewProjection", "mat4");

            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("\tfloat depth = (outDepth.x + 1.0) * 0.5;");
            fragmentShader.append("\tfragOutput = vec4(depth);");
            fragmentShader.append("}");

            tessCtrlShader.addInclude("tessellationLinear.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationLinear.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.addOutgoing("outDepth", "vec3");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            tessEvalShader.append("\tgl_Position = modelViewProjection * pos;");
            tessEvalShader.append("\toutDepth.x = gl_Position.z / gl_Position.w;");
            tessEvalShader.append("}");
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getOrthographicDepthTessPhongShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_orthographicDepthTessPhongShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "orthographic depth tess phong shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));

            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addIncoming("attr_norm", "vec3");
            vertexShader.addOutgoing("outNormal", "vec3");
            vertexShader.addUniform("modelViewProjection", "mat4");

            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("\toutNormal = attr_norm;");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("\tfloat depth = (outDepth.x + 1.0) * 0.5;");
            fragmentShader.append("\tfragOutput = vec4(depth);");
            fragmentShader.append("}");

            tessCtrlShader.addInclude("tessellationPhong.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("\tctNorm[0] = outNormal[0];");
            tessCtrlShader.append("\tctNorm[1] = outNormal[1];");
            tessCtrlShader.append("\tctNorm[2] = outNormal[2];");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationPhong.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.addOutgoing("outDepth", "vec3");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            tessEvalShader.append("\tgl_Position = modelViewProjection * pos;");
            tessEvalShader.append("\toutDepth.x = gl_Position.z / gl_Position.w;");
            tessEvalShader.append("}");
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

QSSGRef<QSSGRenderableDepthPrepassShader> QSSGRendererImpl::getOrthographicDepthTessNPatchShader()
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthShader = m_orthographicDepthTessNPatchShader;

    if (theDepthShader.isNull()) {
        QByteArray name = "orthographic depth tess npatch shader";

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));

            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addIncoming("attr_norm", "vec3");
            vertexShader.addOutgoing("outNormal", "vec3");
            vertexShader.addUniform("modelViewProjection", "mat4");
            fragmentShader.addUniform("modelViewProjection", "mat4");
            fragmentShader.addUniform("cameraProperties", "vec2");
            fragmentShader.addUniform("cameraPosition", "vec3");
            fragmentShader.addUniform("cameraDirection", "vec3");
            fragmentShader.addInclude("depthpass.glsllib");

            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("\toutNormal = attr_norm;");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            // fragmentShader.Append("\tfragOutput = vec4(0.0, 0.0, 0.0, 0.0);");
            fragmentShader.append("\tfloat depth = (outDepth.x - cameraProperties.x) / "
                                  "(cameraProperties.y - cameraProperties.x);");
            fragmentShader.append("\tfragOutput = vec4(depth);");
            fragmentShader.append("}");

            tessCtrlShader.addInclude("tessellationNPatch.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.addOutgoing("outNormalTC", "vec3");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("\tctNorm[0] = outNormal[0];");
            tessCtrlShader.append("\tctNorm[1] = outNormal[1];");
            tessCtrlShader.append("\tctNorm[2] = outNormal[2];");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationNPatch.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.addUniform("modelMatrix", "mat4");
            tessEvalShader.addOutgoing("outDepth", "vec3");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            tessEvalShader.append("\tgl_Position = modelViewProjection * pos;");
            tessEvalShader.append("\toutDepth.x = gl_Position.z / gl_Position.w;");
            tessEvalShader.append("}");
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());

        if (depthShaderProgram) {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }

    return theDepthShader;
}

// ---------------------------------

const QSSGRef<QSSGRenderableDepthPrepassShader> &QSSGRendererImpl::getDepthPrepassShader(bool inDisplaced)
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthPrePassShader = (!inDisplaced)
            ? m_depthPrepassShader
            : m_depthPrepassShaderDisplaced;

    if (theDepthPrePassShader.isNull()) {
        // check if we do displacement mapping
        QByteArray name = "depth prepass shader";
        if (inDisplaced)
            name.append(" displacement");

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram();
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addUniform("modelViewProjection", "mat4");

            vertexShader.append("void main() {");

            if (inDisplaced) {
                contextInterface()->defaultMaterialShaderGenerator()->addDisplacementMappingForDepthPass(vertexShader);
            } else {
                vertexShader.append("\tgl_Position = modelViewProjection * vec4(attr_pos, 1.0);");
            }
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("\tfragOutput = vec4(0.0, 0.0, 0.0, 0.0);");
            fragmentShader.append("}");
        } else if (theCache->isShaderCachePersistenceEnabled()) {
            // we load from shader cache set default shader stages
            getProgramGenerator()->beginProgram();
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());

        if (depthShaderProgram) {
            theDepthPrePassShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthPrePassShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }
    return theDepthPrePassShader;
}

const QSSGRef<QSSGRenderableDepthPrepassShader> &QSSGRendererImpl::getDepthTessPrepassShader(TessellationModeValues inTessMode, bool inDisplaced)
{
    if (!m_contextInterface->renderContext()->supportsTessellation() || inTessMode == TessellationModeValues::NoTessellation) {
        return getDepthPrepassShader(inDisplaced);
    } else if (inTessMode == TessellationModeValues::Linear) {
        return getDepthTessLinearPrepassShader(inDisplaced);
    } else if (inTessMode == TessellationModeValues::Phong) {
        return getDepthTessPhongPrepassShader();
    } else if (inTessMode == TessellationModeValues::NPatch) {
        return getDepthTessNPatchPrepassShader();
    }

    return getDepthPrepassShader(inDisplaced);
}

const QSSGRef<QSSGRenderableDepthPrepassShader> &QSSGRendererImpl::getDepthTessLinearPrepassShader(bool inDisplaced)
{
    QSSGRef<QSSGRenderableDepthPrepassShader> &theDepthPrePassShader = (!inDisplaced)
            ? m_depthTessLinearPrepassShader
            : m_depthTessLinearPrepassShaderDisplaced;

    if (theDepthPrePassShader.isNull()) {
        // check if we do displacement mapping
        QByteArray name = "depth tess linear prepass shader";
        if (inDisplaced)
            name.append(" displacement");

        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            vertexShader.addIncoming("attr_pos", "vec3");
            if (inDisplaced) {
                vertexShader.addIncoming("attr_uv0", "vec2");
                vertexShader.addIncoming("attr_norm", "vec3");

                vertexShader.addUniform("displacementMap_rot", "vec4");
                vertexShader.addUniform("displacementMap_offset", "vec3");

                vertexShader.addOutgoing("outNormal", "vec3");
                vertexShader.addOutgoing("outUV", "vec2");
            }
            vertexShader.addOutgoing("outWorldPos", "vec3");
            vertexShader.addUniform("modelViewProjection", "mat4");
            vertexShader.addUniform("modelMatrix", "mat4");
            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            if (inDisplaced) {
                vertexShader.append("\toutNormal = attr_norm;");
                vertexShader.append("\tvec3 uTransform = vec3( displacementMap_rot.x, "
                                    "displacementMap_rot.y, displacementMap_offset.x );");
                vertexShader.append("\tvec3 vTransform = vec3( displacementMap_rot.z, "
                                    "displacementMap_rot.w, displacementMap_offset.y );");
                vertexShader.addInclude("defaultMaterialLighting.glsllib"); // getTransformedUVCoords is in the
                // lighting code addition.
                vertexShader << "\tvec2 uv_coords = attr_uv0;"
                             << "\n";
                vertexShader << "\toutUV = getTransformedUVCoords( vec3( uv_coords, 1.0), "
                                "uTransform, vTransform );\n";
            }
            vertexShader.append("\toutWorldPos = (modelMatrix * vec4(attr_pos, 1.0)).xyz;");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("\tfragOutput = vec4(0.0, 0.0, 0.0, 0.0);");
            fragmentShader.append("}");

            tessCtrlShader.addInclude("tessellationLinear.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.addOutgoing("outUVTC", "vec2");
            tessCtrlShader.addOutgoing("outNormalTC", "vec3");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("\tctWorldPos[0] = outWorldPos[0];");
            tessCtrlShader.append("\tctWorldPos[1] = outWorldPos[1];");
            tessCtrlShader.append("\tctWorldPos[2] = outWorldPos[2];");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");

            if (inDisplaced) {
                tessCtrlShader.append("\toutUVTC[gl_InvocationID] = outUV[gl_InvocationID];");
                tessCtrlShader.append("\toutNormalTC[gl_InvocationID] = outNormal[gl_InvocationID];");
            }

            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationLinear.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            if (inDisplaced) {
                tessEvalShader.addUniform("displacementSampler", "sampler2D");
                tessEvalShader.addUniform("displaceAmount", "float");
                tessEvalShader.addInclude("defaultMaterialFileDisplacementTexture.glsllib");
            }
            tessEvalShader.addOutgoing("outUV", "vec2");
            tessEvalShader.addOutgoing("outNormal", "vec3");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");

            if (inDisplaced) {
                tessEvalShader << "\toutUV = gl_TessCoord.x * outUVTC[0] + gl_TessCoord.y * "
                                  "outUVTC[1] + gl_TessCoord.z * outUVTC[2];"
                               << "\n";
                tessEvalShader << "\toutNormal = gl_TessCoord.x * outNormalTC[0] + gl_TessCoord.y * "
                                  "outNormalTC[1] + gl_TessCoord.z * outNormalTC[2];"
                               << "\n";
                tessEvalShader << "\tvec3 displacedPos = defaultMaterialFileDisplacementTexture( "
                                  "displacementSampler , displaceAmount, outUV , outNormal, pos.xyz );"
                               << "\n";
                tessEvalShader.append("\tgl_Position = modelViewProjection * vec4(displacedPos, 1.0);");
            } else
                tessEvalShader.append("\tgl_Position = modelViewProjection * pos;");

            tessEvalShader.append("}");
        } else if (theCache->isShaderCachePersistenceEnabled()) {
            // we load from shader cache set default shader stages
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
        }

        QSSGShaderCacheProgramFlags theFlags(ShaderCacheProgramFlagValues::TessellationEnabled);

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, theFlags, ShaderFeatureSetList());

        if (depthShaderProgram) {
            theDepthPrePassShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            theDepthPrePassShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }
    return theDepthPrePassShader;
}

const QSSGRef<QSSGRenderableDepthPrepassShader> &QSSGRendererImpl::getDepthTessPhongPrepassShader()
{
    if (m_depthTessPhongPrepassShader.isNull()) {
        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QByteArray name = "depth tess phong prepass shader";
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addIncoming("attr_norm", "vec3");
            vertexShader.addOutgoing("outNormal", "vec3");
            vertexShader.addOutgoing("outWorldPos", "vec3");
            vertexShader.addUniform("modelViewProjection", "mat4");
            vertexShader.addUniform("modelMatrix", "mat4");
            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("\toutWorldPos = (modelMatrix * vec4(attr_pos, 1.0)).xyz;");
            vertexShader.append("\toutNormal = attr_norm;");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("\tfragOutput = vec4(0.0, 0.0, 0.0, 0.0);");
            fragmentShader.append("}");

            tessCtrlShader.addInclude("tessellationPhong.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("\tctWorldPos[0] = outWorldPos[0];");
            tessCtrlShader.append("\tctWorldPos[1] = outWorldPos[1];");
            tessCtrlShader.append("\tctWorldPos[2] = outWorldPos[2];");
            tessCtrlShader.append("\tctNorm[0] = outNormal[0];");
            tessCtrlShader.append("\tctNorm[1] = outNormal[1];");
            tessCtrlShader.append("\tctNorm[2] = outNormal[2];");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationPhong.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            tessEvalShader.append("\tgl_Position = modelViewProjection * pos;\n");
            tessEvalShader.append("}");
        } else if (theCache->isShaderCachePersistenceEnabled()) {
            // we load from shader cache set default shader stages
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
        }

        QSSGShaderCacheProgramFlags theFlags(ShaderCacheProgramFlagValues::TessellationEnabled);

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, theFlags, ShaderFeatureSetList());

        if (depthShaderProgram) {
            m_depthTessPhongPrepassShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            m_depthTessPhongPrepassShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }
    return m_depthTessPhongPrepassShader;
}

const QSSGRef<QSSGRenderableDepthPrepassShader> &QSSGRendererImpl::getDepthTessNPatchPrepassShader()
{
    if (m_depthTessNPatchPrepassShader.isNull()) {
        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QByteArray name = "depth tess npatch prepass shader";
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
            QSSGShaderStageGeneratorInterface &vertexShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &tessCtrlShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessControl));
            QSSGShaderStageGeneratorInterface &tessEvalShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::TessEval));
            QSSGShaderStageGeneratorInterface &fragmentShader(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            vertexShader.addIncoming("attr_pos", "vec3");
            vertexShader.addIncoming("attr_norm", "vec3");
            vertexShader.addOutgoing("outNormal", "vec3");
            vertexShader.addOutgoing("outWorldPos", "vec3");
            vertexShader.addUniform("modelViewProjection", "mat4");
            vertexShader.addUniform("modelMatrix", "mat4");
            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = vec4(attr_pos, 1.0);");
            vertexShader.append("\toutWorldPos = (modelMatrix * vec4(attr_pos, 1.0)).xyz;");
            vertexShader.append("\toutNormal = attr_norm;");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("\tfragOutput = vec4(0.0, 0.0, 0.0, 0.0);");
            fragmentShader.append("}");

            tessCtrlShader.addOutgoing("outNormalTC", "vec3");
            tessCtrlShader.addInclude("tessellationNPatch.glsllib");
            tessCtrlShader.addUniform("tessLevelInner", "float");
            tessCtrlShader.addUniform("tessLevelOuter", "float");
            tessCtrlShader.append("void main() {\n");
            tessCtrlShader.append("\tctWorldPos[0] = outWorldPos[0];");
            tessCtrlShader.append("\tctWorldPos[1] = outWorldPos[1];");
            tessCtrlShader.append("\tctWorldPos[2] = outWorldPos[2];");
            tessCtrlShader.append("\tctNorm[0] = outNormal[0];");
            tessCtrlShader.append("\tctNorm[1] = outNormal[1];");
            tessCtrlShader.append("\tctNorm[2] = outNormal[2];");
            tessCtrlShader.append("\tctTangent[0] = outNormal[0];"); // we don't care for the tangent
            tessCtrlShader.append("\tctTangent[1] = outNormal[1];");
            tessCtrlShader.append("\tctTangent[2] = outNormal[2];");
            tessCtrlShader.append("\tgl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;");
            tessCtrlShader.append("\ttessShader( tessLevelOuter, tessLevelInner);\n");
            tessCtrlShader.append("\toutNormalTC[gl_InvocationID] = outNormal[gl_InvocationID];\n");
            tessCtrlShader.append("}");

            tessEvalShader.addInclude("tessellationNPatch.glsllib");
            tessEvalShader.addUniform("modelViewProjection", "mat4");
            tessEvalShader.append("void main() {");
            tessEvalShader.append("\tctNorm[0] = outNormalTC[0];");
            tessEvalShader.append("\tctNorm[1] = outNormalTC[1];");
            tessEvalShader.append("\tctNorm[2] = outNormalTC[2];");
            tessEvalShader.append("\tctTangent[0] = outNormalTC[0];"); // we don't care for the tangent
            tessEvalShader.append("\tctTangent[1] = outNormalTC[1];");
            tessEvalShader.append("\tctTangent[2] = outNormalTC[2];");
            tessEvalShader.append("\tvec4 pos = tessShader( );\n");
            tessEvalShader.append("\tgl_Position = modelViewProjection * pos;\n");
            tessEvalShader.append("}");
        } else if (theCache->isShaderCachePersistenceEnabled()) {
            // we load from shader cache set default shader stages
            getProgramGenerator()->beginProgram(
                    QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::TessControl
                                               | QSSGShaderGeneratorStage::TessEval | QSSGShaderGeneratorStage::Fragment));
        }

        QSSGShaderCacheProgramFlags theFlags(ShaderCacheProgramFlagValues::TessellationEnabled);

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, theFlags, ShaderFeatureSetList());

        if (depthShaderProgram) {
            m_depthTessNPatchPrepassShader = QSSGRef<QSSGRenderableDepthPrepassShader>(
                    new QSSGRenderableDepthPrepassShader(depthShaderProgram, context()));
        } else {
            m_depthTessNPatchPrepassShader = QSSGRef<QSSGRenderableDepthPrepassShader>();
        }
    }
    return m_depthTessNPatchPrepassShader;
}

QSSGRef<QSSGSkyBoxShader> QSSGRendererImpl::getSkyBoxShader()
{
    if (!m_skyBoxShader) {
        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QByteArray name = "fullscreen skybox shader";
        QSSGRef<QSSGRenderShaderProgram> skyBoxShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!skyBoxShaderProgram) {
            QSSGRef<QSSGShaderProgramGeneratorInterface> theGenerator(getProgramGenerator());
            theGenerator->beginProgram();
            QSSGShaderStageGeneratorInterface &vertexGenerator(*theGenerator->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &fragmentGenerator(*theGenerator->getStage(QSSGShaderGeneratorStage::Fragment));

            vertexGenerator.addIncoming("attr_pos", "vec3");

            vertexGenerator.addOutgoing("eye_direction", "vec3");

            vertexGenerator.addUniform("viewMatrix", "mat4");
            vertexGenerator.addUniform("projection", "mat4");

            vertexGenerator.append("void main() {");
            vertexGenerator.append("\tgl_Position = vec4(attr_pos, 1.0);");
            vertexGenerator.append("\tmat4 inverseProjection = inverse(projection);");
            vertexGenerator.append("\tvec3 unprojected = (inverseProjection * gl_Position).xyz;");
            vertexGenerator.append("\teye_direction = normalize(mat3(viewMatrix) * unprojected);");
            vertexGenerator.append("}");

            fragmentGenerator.addInclude("customMaterial.glsllib"); // Needed for PI, PI_TWO

            fragmentGenerator.addUniform("skybox_image", "sampler2D");
            fragmentGenerator.addUniform("output_color", "vec3");

            fragmentGenerator.append("void main() {");

            // Ideally, we would just reuse getProbeSampleUV like this, but that leads to issues
            // with incorrect texture gradients because we're drawing on a quad and not a sphere.
            // See explanation below.
            // fragmentGenerator.addInclude("sampleProbe.glsllib");
            // fragmentGenerator.append("\tgl_FragColor = texture2D(skybox_image, getProbeSampleUV(eye, vec4(1.0, 0.0, 0.0, 1.0), vec2(0,0)));");

            // nlerp direction vector, not entirely correct, but simple/efficient
            fragmentGenerator.append("\tvec3 eye = normalize(eye_direction);");

            // Equirectangular textures project longitude and latitude to the xy plane
            fragmentGenerator.append("\tfloat longitude = atan(eye.x, eye.z) / PI_TWO + 0.5;");
            fragmentGenerator.append("\tfloat latitude = asin(eye.y) / PI + 0.5;");
            fragmentGenerator.append("\tvec2 uv = vec2(longitude, latitude);");

            // Because of the non-standard projection, the texture lookup for normal texture
            // filtering is messed up.
            // TODO: Alternatively, we could check if it's possible to disable some of the texture
            // filtering just for the skybox part.
            fragmentGenerator.append("\tvec4 color = textureLod(skybox_image, uv, 0.0);");
            fragmentGenerator.append("\tvec3 tonemappedColor = color.rgb / (color.rgb + vec3(1.0));");
            fragmentGenerator.append("\tvec3 gammaCorrectedColor = pow( tonemappedColor, vec3( 1.0 / 2.2 ));");
            fragmentGenerator.append("\tgl_FragColor = vec4(gammaCorrectedColor, 1.0);");
            fragmentGenerator.append("}");

            // No flags enabled
            skyBoxShaderProgram = theGenerator->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
        }

        if (skyBoxShaderProgram) {
            m_skyBoxShader = QSSGRef<QSSGSkyBoxShader>(
                        new QSSGSkyBoxShader(skyBoxShaderProgram, context()));
        } else {
            m_skyBoxShader = QSSGRef<QSSGSkyBoxShader>();
        }
    }
    return m_skyBoxShader;
}

QSSGRef<QSSGDefaultAoPassShader> QSSGRendererImpl::getDefaultAoPassShader(const ShaderFeatureSetList &inFeatureSet)
{
    if (m_defaultAoPassShader.isNull()) {
        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QByteArray name = "fullscreen AO pass shader";
        QSSGRef<QSSGRenderShaderProgram> aoPassShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!aoPassShaderProgram) {
            getProgramGenerator()->beginProgram();
            QSSGShaderStageGeneratorInterface &theVertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &theFragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            theVertexGenerator.addIncoming("attr_pos", "vec3");
            theVertexGenerator.addIncoming("attr_uv", "vec2");
            theVertexGenerator.addOutgoing("uv_coords", "vec2");
            theVertexGenerator.append("void main() {");
            theVertexGenerator.append("\tgl_Position = vec4(attr_pos.xy, 0.5, 1.0 );");
            theVertexGenerator.append("\tuv_coords = attr_uv;");
            theVertexGenerator.append("}");

            // fragmentGenerator.AddInclude( "SSAOCustomMaterial.glsllib" );
            theFragmentGenerator.addInclude("viewProperties.glsllib");
            theFragmentGenerator.addInclude("screenSpaceAO.glsllib");
            if (m_context->renderContextType() == QSSGRenderContextType::GLES2) {
                theFragmentGenerator << "\tuniform vec4 aoProperties;"
                                     << "\n"
                                     << "\tuniform vec4 aoProperties2;"
                                     << "\n"
                                     << "\tuniform vec4 shadowProperties;"
                                     << "\n"
                                     << "\tuniform vec4 aoScreenConst;"
                                     << "\n"
                                     << "\tuniform vec4 uvToEyeConst;"
                                     << "\n";
            } else {
                theFragmentGenerator << "layout (std140) uniform aoShadow { "
                                     << "\n"
                                     << "\tvec4 aoProperties;"
                                     << "\n"
                                     << "\tvec4 aoProperties2;"
                                     << "\n"
                                     << "\tvec4 shadowProperties;"
                                     << "\n"
                                     << "\tvec4 aoScreenConst;"
                                     << "\n"
                                     << "\tvec4 uvToEyeConst;"
                                     << "\n"
                                     << "};"
                                     << "\n";
            }
            theFragmentGenerator.addUniform("cameraDirection", "vec3");
            theFragmentGenerator.addUniform("depthTexture", "sampler2D");
            theFragmentGenerator.append("void main() {");
            theFragmentGenerator << "\tfloat aoFactor;"
                                 << "\n";
            theFragmentGenerator << "\tvec3 screenNorm;"
                                 << "\n";

            // We're taking multiple depth samples and getting the derivatives at each of them
            // to get a more
            // accurate view space normal vector.  When we do only one, we tend to get bizarre
            // values at the edges
            // surrounding objects, and this also ends up giving us weird AO values.
            // If we had a proper screen-space normal map, that would also do the trick.
            if (m_context->renderContextType() == QSSGRenderContextType::GLES2) {
                theFragmentGenerator.addUniform("depthTextureSize", "vec2");
                theFragmentGenerator.append("\tivec2 iCoords = ivec2( gl_FragCoord.xy );");
                theFragmentGenerator.append("\tfloat depth = getDepthValue( "
                                            "texture2D(depthTexture, vec2(iCoords)"
                                            " / depthTextureSize), cameraProperties );");
                theFragmentGenerator.append("\tdepth = depthValueToLinearDistance( depth, cameraProperties );");
                theFragmentGenerator.append("\tdepth = (depth - cameraProperties.x) / "
                                            "(cameraProperties.y - cameraProperties.x);");
                theFragmentGenerator.append("\tfloat depth2 = getDepthValue( "
                                            "texture2D(depthTexture, vec2(iCoords+ivec2(1))"
                                            " / depthTextureSize), cameraProperties );");
                theFragmentGenerator.append("\tdepth2 = depthValueToLinearDistance( depth, cameraProperties );");
                theFragmentGenerator.append("\tfloat depth3 = getDepthValue( "
                                            "texture2D(depthTexture, vec2(iCoords-ivec2(1))"
                                            " / depthTextureSize), cameraProperties );");
            } else {
                theFragmentGenerator.append("\tivec2 iCoords = ivec2( gl_FragCoord.xy );");
                theFragmentGenerator.append("\tfloat depth = getDepthValue( "
                                            "texelFetch(depthTexture, iCoords, 0), "
                                            "cameraProperties );");
                theFragmentGenerator.append("\tdepth = depthValueToLinearDistance( depth, cameraProperties );");
                theFragmentGenerator.append("\tdepth = (depth - cameraProperties.x) / "
                                            "(cameraProperties.y - cameraProperties.x);");
                theFragmentGenerator.append("\tfloat depth2 = getDepthValue( "
                                            "texelFetch(depthTexture, iCoords+ivec2(1), 0), "
                                            "cameraProperties );");
                theFragmentGenerator.append("\tdepth2 = depthValueToLinearDistance( depth, cameraProperties );");
                theFragmentGenerator.append("\tfloat depth3 = getDepthValue( "
                                            "texelFetch(depthTexture, iCoords-ivec2(1), 0), "
                                            "cameraProperties );");
            }
            theFragmentGenerator.append("\tdepth3 = depthValueToLinearDistance( depth, cameraProperties );");
            theFragmentGenerator.append("\tvec3 tanU = vec3(10, 0, dFdx(depth));");
            theFragmentGenerator.append("\tvec3 tanV = vec3(0, 10, dFdy(depth));");
            theFragmentGenerator.append("\tscreenNorm = normalize(cross(tanU, tanV));");
            theFragmentGenerator.append("\ttanU = vec3(10, 0, dFdx(depth2));");
            theFragmentGenerator.append("\ttanV = vec3(0, 10, dFdy(depth2));");
            theFragmentGenerator.append("\tscreenNorm += normalize(cross(tanU, tanV));");
            theFragmentGenerator.append("\ttanU = vec3(10, 0, dFdx(depth3));");
            theFragmentGenerator.append("\ttanV = vec3(0, 10, dFdy(depth3));");
            theFragmentGenerator.append("\tscreenNorm += normalize(cross(tanU, tanV));");
            theFragmentGenerator.append("\tscreenNorm = -normalize(screenNorm);");

            theFragmentGenerator.append("\taoFactor = \
                                        SSambientOcclusion( depthTexture, screenNorm, aoProperties, aoProperties2, \
                                                            cameraProperties, aoScreenConst, uvToEyeConst );");

            theFragmentGenerator.append("\tgl_FragColor = vec4(aoFactor, aoFactor, aoFactor, 1.0);");

            theFragmentGenerator.append("}");
        }

        aoPassShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), inFeatureSet);

        if (aoPassShaderProgram) {
            m_defaultAoPassShader = QSSGRef<QSSGDefaultAoPassShader>(
                    new QSSGDefaultAoPassShader(aoPassShaderProgram, context()));
        } else {
            m_defaultAoPassShader = QSSGRef<QSSGDefaultAoPassShader>();
        }
    }
    return m_defaultAoPassShader;
}

QSSGRef<QSSGDefaultAoPassShader> QSSGRendererImpl::getFakeDepthShader(ShaderFeatureSetList inFeatureSet)
{
    if (m_fakeDepthShader.isNull()) {
        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QByteArray name = "depth display shader";
        QSSGRef<QSSGRenderShaderProgram> depthShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!depthShaderProgram) {
            getProgramGenerator()->beginProgram();
            QSSGShaderStageGeneratorInterface &theVertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &theFragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            theVertexGenerator.addIncoming("attr_pos", "vec3");
            theVertexGenerator.addIncoming("attr_uv", "vec2");
            theVertexGenerator.addOutgoing("uv_coords", "vec2");
            theVertexGenerator.append("void main() {");
            theVertexGenerator.append("\tgl_Position = vec4(attr_pos.xy, 0.5, 1.0 );");
            theVertexGenerator.append("\tuv_coords = attr_uv;");
            theVertexGenerator.append("}");

            theFragmentGenerator.addUniform("depthTexture", "sampler2D");
            theFragmentGenerator.append("void main() {");
            theFragmentGenerator.append("\tivec2 iCoords = ivec2( gl_FragCoord.xy );");
            theFragmentGenerator.append("\tfloat depSample = texelFetch(depthTexture, iCoords, 0).x;");
            theFragmentGenerator.append("\tgl_FragColor = vec4( depSample, depSample, depSample, 1.0 );");
            theFragmentGenerator.append("\treturn;");
            theFragmentGenerator.append("}");
        }

        depthShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), inFeatureSet);

        if (depthShaderProgram) {
            m_fakeDepthShader = QSSGRef<QSSGDefaultAoPassShader>(new QSSGDefaultAoPassShader(depthShaderProgram, context()));
        } else {
            m_fakeDepthShader = QSSGRef<QSSGDefaultAoPassShader>();
        }
    }
    return m_fakeDepthShader;
}

QSSGRef<QSSGDefaultAoPassShader> QSSGRendererImpl::getFakeCubeDepthShader(ShaderFeatureSetList inFeatureSet)
{
    if (!m_fakeCubemapDepthShader) {
        QSSGRef<QSSGShaderCache> theCache = m_contextInterface->shaderCache();
        QByteArray name = "cube depth display shader";
        QSSGRef<QSSGRenderShaderProgram> cubeShaderProgram = theCache->getProgram(name, ShaderFeatureSetList());
        if (!cubeShaderProgram) {
            getProgramGenerator()->beginProgram();
            QSSGShaderStageGeneratorInterface &theVertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
            QSSGShaderStageGeneratorInterface &theFragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
            theVertexGenerator.addIncoming("attr_pos", "vec3");
            theVertexGenerator.addIncoming("attr_uv", "vec2");
            theVertexGenerator.addOutgoing("sample_dir", "vec3");
            theVertexGenerator.append("void main() {");
            theVertexGenerator.append("\tgl_Position = vec4(attr_pos.xy, 0.5, 1.0 );");
            theVertexGenerator.append("\tsample_dir = vec3(4.0 * (attr_uv.x - 0.5), -1.0, 4.0 * (attr_uv.y - 0.5));");
            theVertexGenerator.append("}");
            theFragmentGenerator.addUniform("depthCube", "samplerCube");
            theFragmentGenerator.append("void main() {");
            theFragmentGenerator.append("\tfloat smpDepth = texture( depthCube, sample_dir ).x;");
            theFragmentGenerator.append("\tgl_FragColor = vec4(smpDepth, smpDepth, smpDepth, 1.0);");
            theFragmentGenerator.append("}");
        }

        cubeShaderProgram = getProgramGenerator()->compileGeneratedShader(name, QSSGShaderCacheProgramFlags(), inFeatureSet);

        if (cubeShaderProgram) {
            m_fakeCubemapDepthShader = QSSGRef<QSSGDefaultAoPassShader>(
                    new QSSGDefaultAoPassShader(cubeShaderProgram, context()));
        } else {
            m_fakeCubemapDepthShader = QSSGRef<QSSGDefaultAoPassShader>();
        }
    }
    return m_fakeCubemapDepthShader;
}

QSSGRef<QSSGLayerSceneShader> QSSGRendererImpl::getSceneLayerShader()
{
    if (m_sceneLayerShader)
        return m_sceneLayerShader;

    getProgramGenerator()->beginProgram();

    QSSGShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
    QSSGShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));

    vertexGenerator.addIncoming("attr_pos", "vec3");
    vertexGenerator.addIncoming("attr_uv", "vec2");
    // xy of text dimensions are scaling factors, zw are offset factors.
    vertexGenerator.addUniform("layer_dimensions", "vec2");
    vertexGenerator.addUniform("modelViewProjection", "mat4");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");
    vertexGenerator << "\tvec3 layerPos = vec3(attr_pos.x * layer_dimensions.x / 2.0"
                    << ", attr_pos.y * layer_dimensions.y / 2.0"
                    << ", attr_pos.z);"
                    << "\n";

    vertexGenerator.append("\tgl_Position = modelViewProjection * vec4(layerPos, 1.0);");
    vertexGenerator.append("\tuv_coords = attr_uv;");
    vertexGenerator.append("}");

    fragmentGenerator.addUniform("layer_image", "sampler2D");
    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tvec2 theCoords = uv_coords;\n");
    fragmentGenerator.append("\tvec4 theLayerTexture = texture2D( layer_image, theCoords );\n");
    fragmentGenerator.append("\tif( theLayerTexture.a == 0.0 ) discard;\n");
    fragmentGenerator.append("\tfragOutput = theLayerTexture;\n");
    fragmentGenerator.append("}");
    QSSGRef<QSSGRenderShaderProgram> theShader = getProgramGenerator()->compileGeneratedShader("layer shader",
                                                                                                   QSSGShaderCacheProgramFlags(),
                                                                                                   ShaderFeatureSetList());
    QSSGRef<QSSGLayerSceneShader> retval;
    if (theShader)
        retval = QSSGRef<QSSGLayerSceneShader>(new QSSGLayerSceneShader(theShader));
    m_sceneLayerShader = retval;
    return m_sceneLayerShader;
}

QSSGRef<QSSGLayerProgAABlendShader> QSSGRendererImpl::getLayerProgAABlendShader()
{
    if (m_layerProgAAShader)
        return m_layerProgAAShader;

    getProgramGenerator()->beginProgram();

    QSSGShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
    QSSGShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
    vertexGenerator.addIncoming("attr_pos", "vec3");
    vertexGenerator.addIncoming("attr_uv", "vec2");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");
    vertexGenerator.append("\tgl_Position = vec4(attr_pos, 1.0 );");
    vertexGenerator.append("\tuv_coords = attr_uv;");
    vertexGenerator.append("}");
    fragmentGenerator.addUniform("accumulator", "sampler2D");
    fragmentGenerator.addUniform("last_frame", "sampler2D");
    fragmentGenerator.addUniform("blend_factors", "vec2");
    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tvec4 accum = texture2D( accumulator, uv_coords );");
    fragmentGenerator.append("\tvec4 lastFrame = texture2D( last_frame, uv_coords );");
    fragmentGenerator.append("\tgl_FragColor = accum*blend_factors.y + lastFrame*blend_factors.x;");
    fragmentGenerator.append("}");
    QSSGRef<QSSGRenderShaderProgram>
            theShader = getProgramGenerator()->compileGeneratedShader("layer progressiveAA blend shader",
                                                                      QSSGShaderCacheProgramFlags(),
                                                                      ShaderFeatureSetList());
    QSSGRef<QSSGLayerProgAABlendShader> retval;
    if (theShader)
        retval = QSSGRef<QSSGLayerProgAABlendShader>(new QSSGLayerProgAABlendShader(theShader));
    m_layerProgAAShader = retval;
    return m_layerProgAAShader;
}

QSSGRef<QSSGLayerLastFrameBlendShader> QSSGRendererImpl::getLayerLastFrameBlendShader()
{
    if (m_layerLastFrameBlendShader)
        return m_layerLastFrameBlendShader;

    getProgramGenerator()->beginProgram();

    QSSGShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
    QSSGShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
    vertexGenerator.addIncoming("attr_pos", "vec3");
    vertexGenerator.addIncoming("attr_uv", "vec2");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");
    vertexGenerator.append("\tgl_Position = vec4(attr_pos, 1.0);");
    vertexGenerator.append("\tuv_coords = attr_uv;");
    vertexGenerator.append("}");
    fragmentGenerator.addUniform("last_frame", "sampler2D");
    fragmentGenerator.addUniform("blend_factor", "float");
    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tvec4 lastFrame = texture2D(last_frame, uv_coords);");
    fragmentGenerator.append("\tgl_FragColor = vec4(lastFrame.rgb, blend_factor);");
    fragmentGenerator.append("}");
    QSSGRef<QSSGRenderShaderProgram>
            theShader = getProgramGenerator()->compileGeneratedShader("layer last frame blend shader",
                                                                      QSSGShaderCacheProgramFlags(),
                                                                      ShaderFeatureSetList());
    QSSGRef<QSSGLayerLastFrameBlendShader> retval;
    if (theShader)
        retval = QSSGRef<QSSGLayerLastFrameBlendShader>(new QSSGLayerLastFrameBlendShader(theShader));
    m_layerLastFrameBlendShader = retval;
    return m_layerLastFrameBlendShader;
}

QSSGRef<QSSGShadowmapPreblurShader> QSSGRendererImpl::getCubeShadowBlurXShader()
{
    if (m_cubeShadowBlurXShader)
        return m_cubeShadowBlurXShader;

    getProgramGenerator()->beginProgram();

    QSSGShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
    QSSGShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
    vertexGenerator.addIncoming("attr_pos", "vec3");
    // vertexGenerator.AddIncoming("attr_uv", "vec2");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");
    vertexGenerator.append("\tgl_Position = vec4(attr_pos, 1.0 );");
    vertexGenerator.append("\tuv_coords.xy = attr_pos.xy;");
    vertexGenerator.append("}");

    // This with the ShadowBlurYShader design for a 2-pass 5x5 (sigma=1.0)
    // Weights computed using -- http://dev.theomader.com/gaussian-kernel-calculator/
    fragmentGenerator.addUniform("cameraProperties", "vec2");
    fragmentGenerator.addUniform("depthCube", "samplerCube");
    // fragmentGenerator.AddUniform("depthSrc", "sampler2D");
    fragmentGenerator.append("layout(location = 0) out vec4 frag0;");
    fragmentGenerator.append("layout(location = 1) out vec4 frag1;");
    fragmentGenerator.append("layout(location = 2) out vec4 frag2;");
    fragmentGenerator.append("layout(location = 3) out vec4 frag3;");
    fragmentGenerator.append("layout(location = 4) out vec4 frag4;");
    fragmentGenerator.append("layout(location = 5) out vec4 frag5;");
    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tfloat ofsScale = cameraProperties.x / 2500.0;");
    fragmentGenerator.append("\tvec3 dir0 = vec3(1.0, -uv_coords.y, -uv_coords.x);");
    fragmentGenerator.append("\tvec3 dir1 = vec3(-1.0, -uv_coords.y, uv_coords.x);");
    fragmentGenerator.append("\tvec3 dir2 = vec3(uv_coords.x, 1.0, uv_coords.y);");
    fragmentGenerator.append("\tvec3 dir3 = vec3(uv_coords.x, -1.0, -uv_coords.y);");
    fragmentGenerator.append("\tvec3 dir4 = vec3(uv_coords.x, -uv_coords.y, 1.0);");
    fragmentGenerator.append("\tvec3 dir5 = vec3(-uv_coords.x, -uv_coords.y, -1.0);");
    fragmentGenerator.append("\tfloat depth0;");
    fragmentGenerator.append("\tfloat depth1;");
    fragmentGenerator.append("\tfloat depth2;");
    fragmentGenerator.append("\tfloat outDepth;");
    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir0).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir0 + vec3(0.0, 0.0, -ofsScale)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir0 + vec3(0.0, 0.0, ofsScale)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir0 + vec3(0.0, 0.0, -2.0*ofsScale)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir0 + vec3(0.0, 0.0, 2.0*ofsScale)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag0 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir1).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir1 + vec3(0.0, 0.0, -ofsScale)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir1 + vec3(0.0, 0.0, ofsScale)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir1 + vec3(0.0, 0.0, -2.0*ofsScale)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir1 + vec3(0.0, 0.0, 2.0*ofsScale)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag1 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir2).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir2 + vec3(-ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir2 + vec3(ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir2 + vec3(-2.0*ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir2 + vec3(2.0*ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag2 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir3).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir3 + vec3(-ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir3 + vec3(ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir3 + vec3(-2.0*ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir3 + vec3(2.0*ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag3 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir4).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir4 + vec3(-ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir4 + vec3(ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir4 + vec3(-2.0*ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir4 + vec3(2.0*ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag4 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir5).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir5 + vec3(-ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir5 + vec3(ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir5 + vec3(-2.0*ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir5 + vec3(2.0*ofsScale, 0.0, 0.0)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag5 = vec4(outDepth);");

    fragmentGenerator.append("}");

    QSSGRef<QSSGRenderShaderProgram> theShader = getProgramGenerator()
                                                             ->compileGeneratedShader("cubemap shadow blur X shader",
                                                                                      QSSGShaderCacheProgramFlags(),
                                                                                      ShaderFeatureSetList());
    QSSGRef<QSSGShadowmapPreblurShader> retval;
    if (theShader)
        retval = QSSGRef<QSSGShadowmapPreblurShader>(new QSSGShadowmapPreblurShader(theShader));
    m_cubeShadowBlurXShader = retval;
    return m_cubeShadowBlurXShader;
}

QSSGRef<QSSGShadowmapPreblurShader> QSSGRendererImpl::getCubeShadowBlurYShader()
{
    if (m_cubeShadowBlurYShader)
        return m_cubeShadowBlurYShader;

    getProgramGenerator()->beginProgram();

    QSSGShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
    QSSGShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
    vertexGenerator.addIncoming("attr_pos", "vec3");
    // vertexGenerator.AddIncoming("attr_uv", "vec2");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");
    vertexGenerator.append("\tgl_Position = vec4(attr_pos, 1.0 );");
    vertexGenerator.append("\tuv_coords.xy = attr_pos.xy;");
    vertexGenerator.append("}");

    // This with the ShadowBlurXShader design for a 2-pass 5x5 (sigma=1.0)
    // Weights computed using -- http://dev.theomader.com/gaussian-kernel-calculator/
    fragmentGenerator.addUniform("cameraProperties", "vec2");
    fragmentGenerator.addUniform("depthCube", "samplerCube");
    // fragmentGenerator.AddUniform("depthSrc", "sampler2D");
    fragmentGenerator.append("layout(location = 0) out vec4 frag0;");
    fragmentGenerator.append("layout(location = 1) out vec4 frag1;");
    fragmentGenerator.append("layout(location = 2) out vec4 frag2;");
    fragmentGenerator.append("layout(location = 3) out vec4 frag3;");
    fragmentGenerator.append("layout(location = 4) out vec4 frag4;");
    fragmentGenerator.append("layout(location = 5) out vec4 frag5;");
    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tfloat ofsScale = cameraProperties.x / 2500.0;");
    fragmentGenerator.append("\tvec3 dir0 = vec3(1.0, -uv_coords.y, -uv_coords.x);");
    fragmentGenerator.append("\tvec3 dir1 = vec3(-1.0, -uv_coords.y, uv_coords.x);");
    fragmentGenerator.append("\tvec3 dir2 = vec3(uv_coords.x, 1.0, uv_coords.y);");
    fragmentGenerator.append("\tvec3 dir3 = vec3(uv_coords.x, -1.0, -uv_coords.y);");
    fragmentGenerator.append("\tvec3 dir4 = vec3(uv_coords.x, -uv_coords.y, 1.0);");
    fragmentGenerator.append("\tvec3 dir5 = vec3(-uv_coords.x, -uv_coords.y, -1.0);");
    fragmentGenerator.append("\tfloat depth0;");
    fragmentGenerator.append("\tfloat depth1;");
    fragmentGenerator.append("\tfloat depth2;");
    fragmentGenerator.append("\tfloat outDepth;");
    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir0).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir0 + vec3(0.0, -ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir0 + vec3(0.0, ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir0 + vec3(0.0, -2.0*ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir0 + vec3(0.0, 2.0*ofsScale, 0.0)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag0 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir1).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir1 + vec3(0.0, -ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir1 + vec3(0.0, ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir1 + vec3(0.0, -2.0*ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir1 + vec3(0.0, 2.0*ofsScale, 0.0)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag1 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir2).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir2 + vec3(0.0, 0.0, -ofsScale)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir2 + vec3(0.0, 0.0, ofsScale)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir2 + vec3(0.0, 0.0, -2.0*ofsScale)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir2 + vec3(0.0, 0.0, 2.0*ofsScale)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag2 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir3).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir3 + vec3(0.0, 0.0, -ofsScale)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir3 + vec3(0.0, 0.0, ofsScale)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir3 + vec3(0.0, 0.0, -2.0*ofsScale)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir3 + vec3(0.0, 0.0, 2.0*ofsScale)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag3 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir4).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir4 + vec3(0.0, -ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir4 + vec3(0.0, ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir4 + vec3(0.0, -2.0*ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir4 + vec3(0.0, 2.0*ofsScale, 0.0)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag4 = vec4(outDepth);");

    fragmentGenerator.append("\tdepth0 = texture(depthCube, dir5).x;");
    fragmentGenerator.append("\tdepth1 = texture(depthCube, dir5 + vec3(0.0, -ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthCube, dir5 + vec3(0.0, ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 = texture(depthCube, dir5 + vec3(0.0, -2.0*ofsScale, 0.0)).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthCube, dir5 + vec3(0.0, 2.0*ofsScale, 0.0)).x;");
    fragmentGenerator.append("\toutDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfrag5 = vec4(outDepth);");

    fragmentGenerator.append("}");

    QSSGRef<QSSGRenderShaderProgram> theShader = getProgramGenerator()
                                                             ->compileGeneratedShader("cubemap shadow blur Y shader",
                                                                                      QSSGShaderCacheProgramFlags(),
                                                                                      ShaderFeatureSetList());
    QSSGRef<QSSGShadowmapPreblurShader> retval;
    if (theShader)
        retval = QSSGRef<QSSGShadowmapPreblurShader>(new QSSGShadowmapPreblurShader(theShader));
    m_cubeShadowBlurYShader = retval;
    return m_cubeShadowBlurYShader;
}

QSSGRef<QSSGShadowmapPreblurShader> QSSGRendererImpl::getOrthoShadowBlurXShader()
{
    if (m_orthoShadowBlurXShader)
        return m_orthoShadowBlurXShader;

    getProgramGenerator()->beginProgram();

    QSSGShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
    QSSGShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
    vertexGenerator.addIncoming("attr_pos", "vec3");
    vertexGenerator.addIncoming("attr_uv", "vec2");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");
    vertexGenerator.append("\tgl_Position = vec4(attr_pos, 1.0 );");
    vertexGenerator.append("\tuv_coords.xy = attr_uv.xy;");
    vertexGenerator.append("}");

    fragmentGenerator.addUniform("cameraProperties", "vec2");
    fragmentGenerator.addUniform("depthSrc", "sampler2D");
    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tvec2 ofsScale = vec2( cameraProperties.x / 7680.0, 0.0 );");
    fragmentGenerator.append("\tfloat depth0 = texture(depthSrc, uv_coords).x;");
    fragmentGenerator.append("\tfloat depth1 = texture(depthSrc, uv_coords + ofsScale).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthSrc, uv_coords - ofsScale).x;");
    fragmentGenerator.append("\tfloat depth2 = texture(depthSrc, uv_coords + 2.0 * ofsScale).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthSrc, uv_coords - 2.0 * ofsScale).x;");
    fragmentGenerator.append("\tfloat outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfragOutput = vec4(outDepth);");
    fragmentGenerator.append("}");

    QSSGRef<QSSGRenderShaderProgram> theShader = getProgramGenerator()
                                                             ->compileGeneratedShader("shadow map blur X shader",
                                                                                      QSSGShaderCacheProgramFlags(),
                                                                                      ShaderFeatureSetList());
    QSSGRef<QSSGShadowmapPreblurShader> retval;
    if (theShader)
        retval = QSSGRef<QSSGShadowmapPreblurShader>(new QSSGShadowmapPreblurShader(theShader));
    m_orthoShadowBlurXShader = retval;
    return m_orthoShadowBlurXShader;
}

QSSGRef<QSSGShadowmapPreblurShader> QSSGRendererImpl::getOrthoShadowBlurYShader()
{
    if (m_orthoShadowBlurYShader)
        return m_orthoShadowBlurYShader;

    getProgramGenerator()->beginProgram();

    QSSGShaderStageGeneratorInterface &vertexGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex));
    QSSGShaderStageGeneratorInterface &fragmentGenerator(*getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment));
    vertexGenerator.addIncoming("attr_pos", "vec3");
    vertexGenerator.addIncoming("attr_uv", "vec2");
    vertexGenerator.addOutgoing("uv_coords", "vec2");
    vertexGenerator.append("void main() {");
    vertexGenerator.append("\tgl_Position = vec4(attr_pos, 1.0 );");
    vertexGenerator.append("\tuv_coords.xy = attr_uv.xy;");
    vertexGenerator.append("}");

    fragmentGenerator.addUniform("cameraProperties", "vec2");
    fragmentGenerator.addUniform("depthSrc", "sampler2D");
    fragmentGenerator.append("void main() {");
    fragmentGenerator.append("\tvec2 ofsScale = vec2( 0.0, cameraProperties.x / 7680.0 );");
    fragmentGenerator.append("\tfloat depth0 = texture(depthSrc, uv_coords).x;");
    fragmentGenerator.append("\tfloat depth1 = texture(depthSrc, uv_coords + ofsScale).x;");
    fragmentGenerator.append("\tdepth1 += texture(depthSrc, uv_coords - ofsScale).x;");
    fragmentGenerator.append("\tfloat depth2 = texture(depthSrc, uv_coords + 2.0 * ofsScale).x;");
    fragmentGenerator.append("\tdepth2 += texture(depthSrc, uv_coords - 2.0 * ofsScale).x;");
    fragmentGenerator.append("\tfloat outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;");
    fragmentGenerator.append("\tfragOutput = vec4(outDepth);");
    fragmentGenerator.append("}");

    QSSGRef<QSSGRenderShaderProgram> theShader = getProgramGenerator()
                                                             ->compileGeneratedShader("shadow map blur Y shader",
                                                                                      QSSGShaderCacheProgramFlags(),
                                                                                      ShaderFeatureSetList());
    QSSGRef<QSSGShadowmapPreblurShader> retval;
    if (theShader)
        retval = QSSGRef<QSSGShadowmapPreblurShader>(new QSSGShadowmapPreblurShader(theShader));
    m_orthoShadowBlurYShader = retval;
    return m_orthoShadowBlurYShader;
}

QT_END_NAMESPACE