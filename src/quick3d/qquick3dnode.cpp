/****************************************************************************
**
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

#include "qquick3dnode_p.h"
#include "qquick3dnode_p_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>
#include <QtQuick3DUtils/private/qssgrendereulerangles_p.h>
#include <QtQuick3D/private/qquick3dobject_p_p.h>

#include <QtMath>

QT_BEGIN_NAMESPACE

QQuick3DNodePrivate::QQuick3DNodePrivate()
{

}

QQuick3DNodePrivate::~QQuick3DNodePrivate()
{

}

void QQuick3DNodePrivate::init()
{

}

/*!
    \qmltype Node
    \inherits Object3D
    \instantiates QQuick3DNode
    \inqmlmodule QtQuick3D
    \brief The base component for an object that exists in a 3D Scene.


*/

QQuick3DNode::QQuick3DNode(QQuick3DNode *parent)
    : QQuick3DObject(*(new QQuick3DNodePrivate), parent)
{
    Q_D(QQuick3DNode);
    d->init();
}

QQuick3DNode::QQuick3DNode(QQuick3DNodePrivate &dd, QQuick3DNode *parent)
    : QQuick3DObject(dd, parent)
{
    Q_D(QQuick3DNode);
    d->init();
}

QQuick3DNode::~QQuick3DNode() {}

/*!
    \qmlproperty float QtQuick3D::Node::x

    This property contains the x value of the position translation in
    local coordinate space.

    \sa QtQuick3D::Node::position
*/
float QQuick3DNode::x() const
{
    Q_D(const QQuick3DNode);
    return d->m_position.x();
}

/*!
    \qmlproperty float QtQuick3D::Node::y

    This property contains the y value of the position translation in
    local coordinate space.

    \sa QtQuick3D::Node::position
*/
float QQuick3DNode::y() const
{
    Q_D(const QQuick3DNode);
    return d->m_position.y();
}

/*!
    \qmlproperty float QtQuick3D::Node::z

    This property contains the z value of the position translation in
    local coordinate space.

    \sa QtQuick3D::Node::position
*/
float QQuick3DNode::z() const
{
    Q_D(const QQuick3DNode);
    return d->m_position.z();
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::rotation

    This property contains the rotation values for the x, y, and z axis.
    These values are stored as euler angles.
*/
QVector3D QQuick3DNode::rotation() const
{
    Q_D(const QQuick3DNode);
    return d->m_rotation;
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::position

    This property contains the position translation in local coordinate space.

    \sa QtQuick3D::Node::x, QtQuick3D::Node::y, QtQuick3D::Node::z
*/
QVector3D QQuick3DNode::position() const
{
    Q_D(const QQuick3DNode);
    return d->m_position;
}


/*!
    \qmlproperty vector3d QtQuick3D::Node::scale

    This property contains the scale values for the x, y, and z axis.
*/
QVector3D QQuick3DNode::scale() const
{
    Q_D(const QQuick3DNode);
    return d->m_scale;
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::pivot

    This property contains the pivot values for the x, y, and z axis.  These
    values are used as the pivot points when applying rotations to the node.

*/
QVector3D QQuick3DNode::pivot() const
{
    Q_D(const QQuick3DNode);
    return d->m_pivot;
}

/*!
    \qmlproperty float QtQuick3D::Node::opacity

    This property contains the local opacity value of the Node.  Since Node
    objects are not necessarialy visible, this value might not have any affect,
    but this value is inherited by all children of the Node, which might be visible.

*/
float QQuick3DNode::localOpacity() const
{
    Q_D(const QQuick3DNode);
    return d->m_opacity;
}

/*!
    \qmlproperty int QtQuick3D::Node::boneId

    This property contains the skeletonID used for skeletal animations

    \note This property currently has no effect, since skeletal animations are
    not implimented.

*/
qint32 QQuick3DNode::skeletonId() const
{
    Q_D(const QQuick3DNode);
    return d->m_boneid;
}

/*!
    \qmlproperty enumeration QtQuick3D::Node::rotationOrder

    This property defines in what order the Node::rotation properties components
    are applied in.

*/
QQuick3DNode::RotationOrder QQuick3DNode::rotationOrder() const
{
    Q_D(const QQuick3DNode);
    return d->m_rotationorder;
}

/*!
    \qmlproperty enumeration QtQuick3D::Node::orientation

    This property defines whether the Node is using a RightHanded or Lefthanded
    coordinate system.


*/
QQuick3DNode::Orientation QQuick3DNode::orientation() const
{
    Q_D(const QQuick3DNode);
    return d->m_orientation;
}

/*!
    \qmlproperty bool QtQuick3D::Node::visible

    When this property is true, the Node (and its children) can be visible.

*/
bool QQuick3DNode::visible() const
{
    Q_D(const QQuick3DNode);
    return d->m_visible;
}

QQuick3DNode *QQuick3DNode::parentNode() const
{
    // The parent of a QQuick3DNode should never be anything else than a (subclass
    // of) QQuick3DNode (but the children/leaf nodes can be something else).
    return static_cast<QQuick3DNode *>(parentItem());
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::forward

    This property returns a normalized vector of the nodes forward direction
    in scene space.

    \sa up(), right(), mapDirectionToScene()
*/
QVector3D QQuick3DNode::forward() const
{
    return mapDirectionToScene(QVector3D(0, 0, 1)).normalized();
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::up

    This property returns a normalized vector of the nodes up direction
    in scene space.

    \sa forward(), right(), mapDirectionToScene()
*/
QVector3D QQuick3DNode::up() const
{
    return mapDirectionToScene(QVector3D(0, 1, 0)).normalized();
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::right

    This property returns a normalized vector of the nodes right direction
    in scene space.

    \sa forward(), up(), mapDirectionToScene()
*/
QVector3D QQuick3DNode::right() const
{
    return mapDirectionToScene(QVector3D(1, 0, 0)).normalized();
}
/*!
    \qmlproperty vector3d QtQuick3D::Node::scenePosition

    This property returns the position of the node in scene space.

    \note This is sometimes also reffered to as the global position. But
    then in the meaning "global in the 3D world", and not "global to the
    screen or desktop" (which is usually the interpretation in other Qt APIs).
    \note the position will be reported in the same orientation as the node.

    \sa mapPositionToScene()
*/
QVector3D QQuick3DNode::positionInScene() const
{
    return mat44::getPosition(globalTransform());
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::sceneRotation

    This property returns the rotation of the node in scene space.
*/
QVector3D QQuick3DNode::rotationInScene() const
{
    Q_D(const QQuick3DNode);
    return mat44::getRotation(globalTransform(), d->m_rotationorder);
}

/*!
    \qmlproperty vector3d QtQuick3D::Node::sceneScale

    This property returns the scale of the node in scene space.
*/
QVector3D QQuick3DNode::scaleInScene() const
{
    return mat44::getScale(globalTransform());
}

/*!
    \qmlproperty matrix4x4 QtQuick3D::Node::globalTransform

    This property returns the global transform matrix for this node.
    \note the return value will be \l LeftHanded or \l RightHanded
    depending on \l orientation().

    \sa globalTransformRightHanded()
*/
QMatrix4x4 QQuick3DNode::globalTransform() const
{
    Q_D(const QQuick3DNode);
    return d->m_orientation == LeftHanded ? globalTransformLeftHanded() : globalTransformRightHanded();
}

/*!
    This function returns the global transform matrix for this node
    as a left-handed coordinate system, regardless of \l orientation().

    \sa globalTransform() globalTransformRightHanded()
*/
QMatrix4x4 QQuick3DNode::globalTransformLeftHanded() const
{
    QMatrix4x4 transform = globalTransformRightHanded();
    mat44::flip(transform);
    return transform;
}

/*!
    This function returns the global transform matrix for this node
    as a right-handed coordinate system, regardless of \l orientation().

    \sa globalTransform() globalTransformLeftHanded()
*/
QMatrix4x4 QQuick3DNode::globalTransformRightHanded() const
{
    Q_D(const QQuick3DNode);
    if (d->m_globalTransformDirty)
        const_cast<QQuick3DNodePrivate *>(d)->calculateGlobalVariables();
    return d->m_globalTransformRightHanded;
}

void QQuick3DNodePrivate::calculateGlobalVariables()
{
    Q_Q(QQuick3DNode);
    m_globalTransformDirty = false;
    QMatrix4x4 localTransformRightHanded = calculateLocalTransformRightHanded();
    QQuick3DNode *parent = q->parentNode();
    if (!parent) {
        m_globalTransformRightHanded = localTransformRightHanded;
        return;
    }
    QQuick3DNodePrivate *privateParent = QQuick3DNodePrivate::get(parent);

    if (privateParent->m_globalTransformDirty)
        privateParent->calculateGlobalVariables();
    m_globalTransformRightHanded = privateParent->m_globalTransformRightHanded * localTransformRightHanded;
}

QMatrix4x4 QQuick3DNodePrivate::calculateLocalTransformRightHanded()
{
    // Create a right-handed rotation transform from the radians.
    const float radX = qDegreesToRadians(m_rotation.x());
    const float radY = qDegreesToRadians(m_rotation.y());
    const float radZ = qDegreesToRadians(m_rotation.z());
    const QVector3D radians(radX, radY, radZ);
    const QMatrix4x4 rotationTransform = QSSGEulerAngleConverter::createRotationMatrix(radians, quint32(m_rotationorder));

    const QVector3D pivot = -m_pivot * m_scale;
    QMatrix4x4 localTransform;

    localTransform(0, 0) = m_scale[0];
    localTransform(1, 1) = m_scale[1];
    localTransform(2, 2) = m_scale[2];

    localTransform(0, 3) = pivot[0];
    localTransform(1, 3) = pivot[1];
    localTransform(2, 3) = pivot[2];

    localTransform = rotationTransform * localTransform;

    localTransform(0, 3) += m_position[0];
    localTransform(1, 3) += m_position[1];
    localTransform(2, 3) += m_position[2];

    if (Q_LIKELY(m_orientation == QQuick3DNode::LeftHanded))
        mat44::flip(localTransform);

    return localTransform;
}

QQuick3DObject::Type QQuick3DNode::type() const
{
    return QQuick3DObject::Node;
}

void QQuick3DNodePrivate::emitChangesToGlobalTransform()
{
    Q_Q(QQuick3DNode);
    const QVector3D prevPosition = mat44::getPosition(m_globalTransformRightHanded);
    const QVector3D prevRotation = mat44::getRotation(m_globalTransformRightHanded, m_rotationorder);
    const QVector3D prevScale = mat44::getScale(m_globalTransformRightHanded);

    calculateGlobalVariables();

    const QVector3D newPosition = mat44::getPosition(m_globalTransformRightHanded);
    const QVector3D newRotation = mat44::getRotation(m_globalTransformRightHanded, m_rotationorder);
    const QVector3D newScale = mat44::getScale(m_globalTransformRightHanded);

    const bool positionChanged = prevPosition != newPosition;
    const bool rotationChanged = prevRotation != newRotation;
    const bool scaleChanged = prevScale != newScale;

    if (!positionChanged && !rotationChanged && !scaleChanged)
        return;

    emit q->globalTransformChanged();

    if (positionChanged)
        emit q->positionInSceneChanged();
    if (rotationChanged)
        emit q->rotationInSceneChanged();
    if (scaleChanged)
        emit q->scaleInSceneChanged();
}

bool QQuick3DNodePrivate::isGlobalTransformRelatedSignal(const QMetaMethod &signal) const
{
    // Return true if its likely that we need to emit
    // the given signal when our global transform changes.
    static const QMetaMethod globalTransformSignal = QMetaMethod::fromSignal(&QQuick3DNode::globalTransformChanged);
    static const QMetaMethod globalPositionSignal = QMetaMethod::fromSignal(&QQuick3DNode::positionInSceneChanged);
    static const QMetaMethod globalRotationSignal = QMetaMethod::fromSignal(&QQuick3DNode::rotationInSceneChanged);
    static const QMetaMethod globalScaleSignal = QMetaMethod::fromSignal(&QQuick3DNode::scaleInSceneChanged);

    return (signal == globalTransformSignal
            || signal == globalPositionSignal
            || signal == globalRotationSignal
            || signal == globalScaleSignal);
}

void QQuick3DNode::connectNotify(const QMetaMethod &signal)
{
    Q_D(QQuick3DNode);
    // Since we want to avoid calculating the global transform in the frontend
    // unnecessary, we keep track of the number of connections/QML bindings
    // that needs it. If there are no connections, we can skip calculating it
    // whenever our geometry changes (unless someone asks for it explicitly).
    if (d->isGlobalTransformRelatedSignal(signal))
        d->m_globalTransformConnectionCount++;
}

void QQuick3DNode::disconnectNotify(const QMetaMethod &signal)
{
    Q_D(QQuick3DNode);
    if (d->isGlobalTransformRelatedSignal(signal))
        d->m_globalTransformConnectionCount--;
}

void QQuick3DNode::componentComplete()
{
    Q_D(QQuick3DNode);
    QQuick3DObject::componentComplete();
    if (d->m_globalTransformConnectionCount > 0)
        d->emitChangesToGlobalTransform();
}

void QQuick3DNodePrivate::markGlobalTransformDirty()
{
    Q_Q(QQuick3DNode);
    // Note: we recursively set m_globalTransformDirty to true whenever our geometry
    // changes. But we only set it back to false if someone actually queries our global
    // transform (because only then do we need to calculate it). This means that if no
    // one ever does that, m_globalTransformDirty will remain true, perhaps through out
    // the life time of the node. This is in contrast with the backend, which need to
    // update dirty transform nodes for every scene graph sync (and clear the backend
    // dirty transform flags - QQuick3DObjectPrivate::dirtyAttributes).
    // This means that for most nodes, calling markGlobalTransformDirty() should be
    // cheap, since we normally expect to return early in the following test.
    if (m_globalTransformDirty)
        return;

    m_globalTransformDirty = true;

    if (m_globalTransformConnectionCount > 0)
        emitChangesToGlobalTransform();

    auto children = QQuick3DObjectPrivate::get(q)->childItems;
    for (auto child : children) {
        if (auto node = qobject_cast<QQuick3DNode *>(child)) {
            QQuick3DNodePrivate::get(node)->markGlobalTransformDirty();
        }
    }
}

void QQuick3DNode::setX(float x)
{
    Q_D(QQuick3DNode);
    if (qFuzzyCompare(d->m_position.x(), x))
        return;

    d->m_position.setX(x);
    d->markGlobalTransformDirty();
    emit positionChanged(d->m_position);
    emit xChanged(x);
    update();
}

void QQuick3DNode::setY(float y)
{
    Q_D(QQuick3DNode);
    if (qFuzzyCompare(d->m_position.y(), y))
        return;

    d->m_position.setY(y);
    d->markGlobalTransformDirty();
    emit positionChanged(d->m_position);
    emit yChanged(y);
    update();
}

void QQuick3DNode::setZ(float z)
{
    Q_D(QQuick3DNode);
    if (qFuzzyCompare(d->m_position.z(), z))
        return;

    d->m_position.setZ(z);
    d->markGlobalTransformDirty();
    emit positionChanged(d->m_position);
    emit zChanged(z);
    update();
}

void QQuick3DNode::setRotation(QVector3D rotation)
{
    Q_D(QQuick3DNode);
    if (d->m_rotation == rotation)
        return;

    d->m_rotation = rotation;
    d->markGlobalTransformDirty();
    emit rotationChanged(d->m_rotation);
    update();
}

void QQuick3DNode::setPosition(QVector3D position)
{
    Q_D(QQuick3DNode);
    if (d->m_position == position)
        return;

    const bool xUnchanged = qFuzzyCompare(position.x(), d->m_position.x());
    const bool yUnchanged = qFuzzyCompare(position.y(), d->m_position.y());
    const bool zUnchanged = qFuzzyCompare(position.z(), d->m_position.z());

    d->m_position = position;
    d->markGlobalTransformDirty();
    emit positionChanged(d->m_position);

    if (!xUnchanged)
        emit xChanged(d->m_position.x());
    if (!yUnchanged)
        emit yChanged(d->m_position.y());
    if (!zUnchanged)
        emit zChanged(d->m_position.z());

    update();
}

void QQuick3DNode::setScale(QVector3D scale)
{
    Q_D(QQuick3DNode);
    if (d->m_scale == scale)
        return;

    d->m_scale = scale;
    d->markGlobalTransformDirty();
    emit scaleChanged(d->m_scale);
    update();
}

void QQuick3DNode::setPivot(QVector3D pivot)
{
    Q_D(QQuick3DNode);
    if (d->m_pivot == pivot)
        return;

    d->m_pivot = pivot;
    d->markGlobalTransformDirty();
    emit pivotChanged(d->m_pivot);
    update();
}

void QQuick3DNode::setLocalOpacity(float opacity)
{
    Q_D(QQuick3DNode);
    if (qFuzzyCompare(d->m_opacity, opacity))
        return;

    d->m_opacity = opacity;
    emit localOpacityChanged(d->m_opacity);
    update();
}

void QQuick3DNode::setSkeletonId(qint32 boneid)
{
    Q_D(QQuick3DNode);
    if (d->m_boneid == boneid)
        return;

    d->m_boneid = boneid;
    emit skeletonIdChanged(d->m_boneid);
    update();
}

void QQuick3DNode::setRotationOrder(QQuick3DNode::RotationOrder rotationorder)
{
    Q_D(QQuick3DNode);
    if (d->m_rotationorder == rotationorder)
        return;

    d->m_rotationorder = rotationorder;
    d->markGlobalTransformDirty();
    emit rotationOrderChanged(d->m_rotationorder);
    update();
}

void QQuick3DNode::setOrientation(QQuick3DNode::Orientation orientation)
{
    Q_D(QQuick3DNode);
    if (d->m_orientation == orientation)
        return;

    d->m_orientation = orientation;
    d->markGlobalTransformDirty();
    emit orientationChanged(d->m_orientation);
    update();
}

void QQuick3DNode::setVisible(bool visible)
{
    Q_D(QQuick3DNode);
    if (d->m_visible == visible)
        return;

    d->m_visible = visible;
    emit visibleChanged(d->m_visible);
    update();
}

QSSGRenderGraphObject *QQuick3DNode::updateSpatialNode(QSSGRenderGraphObject *node)
{
    Q_D(QQuick3DNode);
    if (!node)
        node = new QSSGRenderNode();

    auto spacialNode = static_cast<QSSGRenderNode *>(node);
    bool transformIsDirty = false;
    if (spacialNode->position != d->m_position) {
        transformIsDirty = true;
        spacialNode->position = d->m_position;
    }
    if (spacialNode->rotation != d->m_rotation) {
        transformIsDirty = true;
        spacialNode->rotation = QVector3D(qDegreesToRadians(d->m_rotation.x()),
                                          qDegreesToRadians(d->m_rotation.y()),
                                          qDegreesToRadians(d->m_rotation.z()));
    }
    if (spacialNode->scale != d->m_scale) {
        transformIsDirty = true;
        spacialNode->scale = d->m_scale;
    }
    if (spacialNode->pivot != d->m_pivot) {
        transformIsDirty = true;
        spacialNode->pivot = d->m_pivot;
    }

    if (spacialNode->rotationOrder != quint32(d->m_rotationorder)) {
        transformIsDirty = true;
        spacialNode->rotationOrder = quint32(d->m_rotationorder);
    }

    spacialNode->localOpacity = d->m_opacity;
    spacialNode->skeletonId = d->m_boneid;

    if (d->m_orientation == LeftHanded)
        spacialNode->flags.setFlag(QSSGRenderNode::Flag::LeftHanded, true);
    else
        spacialNode->flags.setFlag(QSSGRenderNode::Flag::LeftHanded, false);

    spacialNode->flags.setFlag(QSSGRenderNode::Flag::Active, d->m_visible);

    if (transformIsDirty) {
        spacialNode->markDirty(QSSGRenderNode::TransformDirtyFlag::TransformIsDirty);
        spacialNode->calculateGlobalVariables();
        // Still needs to be marked dirty if it will show up correctly in the backend
        // Note: no longer sure if this is still needed after we now do our own
        // calculation of the global matrix from the front-end.
        spacialNode->flags.setFlag(QSSGRenderNode::Flag::Dirty, true);
    } else {
        spacialNode->markDirty(QSSGRenderNode::TransformDirtyFlag::TransformNotDirty);
    }

    return spacialNode;
}

/*!
    Transforms \a localPosition from local space to scene space.

    \note "Scene space" is sometimes also reffered to as the "global space". But
    then in the meaning "global in the 3D world", and not "global to the
    screen or desktop" (which is usually the interpretation in other Qt APIs).

    \sa mapPositionFromScene(), mapPositionToNode(), mapPositionFromNode()
*/
QVector3D QQuick3DNode::mapPositionToScene(const QVector3D localPosition) const
{
    return mat44::transform(globalTransform(), localPosition);
}

/*!
    Transforms \a scenePosition from scene space to local space.

    \sa mapPositionToScene(), mapPositionToNode(), mapPositionFromNode()
*/
QVector3D QQuick3DNode::mapPositionFromScene(const QVector3D scenePosition) const
{
    return mat44::transform(globalTransform().inverted(), scenePosition);
}

/*!
    Transforms \a localPosition from the local space of this node to
    the local space of \a node.

    \sa mapPositionToScene(), mapPositionFromScene(), mapPositionFromNode()
*/
QVector3D QQuick3DNode::mapPositionToNode(const QQuick3DNode *node, const QVector3D localPosition) const
{
    return node->mapPositionFromScene(mapPositionToScene(localPosition));
}

/*!
    Transforms \a localPosition from the local space of \a node to
    the local space of this node.

    \sa mapPositionToScene(), mapPositionFromScene(), mapPositionFromNode()
*/
QVector3D QQuick3DNode::mapPositionFromNode(const QQuick3DNode *node, const QVector3D localPosition) const
{
    return mapPositionFromScene(node->mapPositionToScene(localPosition));
}

/*!
    Transforms \a localDirection from local space to scene space.
    The return value is not affected by the (inherited) scale or
    position of the node.

    \note the return value will have the same length as \a localDirection
    (i.e not normalized).

    \sa mapDirectionFromScene(), mapDirectionToNode(), mapDirectionFromNode()
*/
QVector3D QQuick3DNode::mapDirectionToScene(const QVector3D localDirection) const
{
    QMatrix3x3 theDirMatrix = mat44::getUpper3x3(globalTransform());
    theDirMatrix = mat33::getInverse(theDirMatrix).transposed();
    return mat33::transform(theDirMatrix, localDirection);
}

/*!
    Transforms \a sceneDirection from scene space to local space.
    The return value is not affected by the (inherited) scale or
    position of the node.

    \note the return value will have the same length as \a sceneDirection
    (i.e not normalized).

    \sa mapDirectionToScene(), mapDirectionToNode(), mapDirectionFromNode()
*/
QVector3D QQuick3DNode::mapDirectionFromScene(const QVector3D sceneDirection) const
{
    QMatrix3x3 theDirMatrix = mat44::getUpper3x3(globalTransform());
    theDirMatrix = theDirMatrix.transposed();
    return mat33::transform(theDirMatrix, sceneDirection);
}

/*!
    Transforms \a localDirection from this nodes local space to the
    local space of \a node.
    The return value is not affected by the (inherited) scale or
    position of the node.

    \note the return value will have the same length as \a localDirection
    (i.e not normalized).

    \sa mapDirectionFromNode(), mapDirectionFromScene(), mapDirectionToScene()
*/
QVector3D QQuick3DNode::mapDirectionToNode(const QQuick3DNode *node, const QVector3D localDirection) const
{
    return node->mapDirectionFromScene(mapDirectionToScene(localDirection));
}

/*!
    Transforms \a localDirection from the local space of \a node to the
    local space of this node.
    The return value is not affected by the (inherited) scale or
    position of the node.

    \note the return value will have the same length as \a localDirection
    (i.e not normalized).

    \sa mapDirectionToNode(), mapDirectionFromScene(), mapDirectionToScene()
*/
QVector3D QQuick3DNode::mapDirectionFromNode(const QQuick3DNode *node, const QVector3D localDirection) const
{
    return mapDirectionFromScene(node->mapDirectionToScene(localDirection));
}

QT_END_NAMESPACE
