/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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


#include <QtQuick3DRuntimeRender/private/qssgrenderitem2d_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderprefiltertexture_p.h>
#include <QtQuick/QSGTexture>

QT_BEGIN_NAMESPACE

QSSGRenderItem2D::QSSGRenderItem2D()
    : QSSGRenderNode(QSSGRenderGraphObject::Type::Item2D)
{
    m_flags.setFlag(Flag::Dirty);
}

QSSGRenderItem2D::~QSSGRenderItem2D() = default;

bool QSSGRenderItem2D::clearDirty(const QSSGRef<QSSGBufferManager> &inBufferManager)
{
    bool wasDirty = m_flags.testFlag(Flag::Dirty);
    m_flags.setFlag(Flag::Dirty, false);
    QSSGRenderImageTextureData newImage;

    if (wasDirty) {
        if (m_qsgTexture)
            newImage = inBufferManager->loadRenderImage(m_qsgTexture);

        if (newImage.m_texture != m_textureData.m_texture)
            m_textureData = newImage;
    }
    return wasDirty;
}

QT_END_NAMESPACE