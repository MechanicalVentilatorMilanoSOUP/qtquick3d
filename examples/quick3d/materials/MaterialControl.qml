/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.3

Item {
    property real metalness: metalness.sliderValue
    property real roughness: roughness.sliderValue
    property real specular: specular.sliderValue
    property real specularTint: specularTint.sliderValue
    property real ior: ior.sliderValue

    Component {
        id: propertySlider
        RowLayout {
            Text {
                id: propText
                text: name
            }
            Slider {
                id: slider
                from: fromValue
                to: toValue
                value: sliderValue
                stepSize: 0.01
                onValueChanged: {
                    sliderValue = value
                }
            }
            Text {
                id: valueText
                text: slider.value
            }
        }
    }

    ColumnLayout {
        Loader {
            id: metalness
            property real sliderValue: 1.0
            property string name: "Metalness"
            property real fromValue: 0.0
            property real toValue: 1.0
            sourceComponent:  propertySlider
        }
        Loader {
            id: roughness
            property real sliderValue: 0.0
            property string name: "Roughness"
            property real fromValue: 0.0
            property real toValue: 1.0
            sourceComponent:  propertySlider
        }
        Loader {
            id: specular
            property real sliderValue: 0.0
            property string name: "Specular"
            property real fromValue: 0.0
            property real toValue: 1.0
            sourceComponent:  propertySlider
        }
        Loader {
            id: ior
            property real sliderValue: 1.8
            property string name: "IOR"
            property real fromValue: 1.0
            property real toValue: 1.8
            sourceComponent: propertySlider
        }
        Loader {
            id: specularTint
            property real sliderValue: 0.0
            property string name: "Specular Tint"
            property real fromValue: 0.0
            property real toValue: 1.0
            sourceComponent: propertySlider
        }
    }
}