import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

Row {
    id: root
    spacing: Theme.sp2
    Accessible.name: "操作按钮组"
    Accessible.description: "生成内容后的操作按钮，包括审校、复制、插入、替换、新章、重生"
    Accessible.role: Accessible.PageTabList

    property bool enabledAll: true

    signal copyClicked()
    signal insertClicked()
    signal replaceClicked()
    signal newChapterClicked()
    signal regenerateClicked()
    signal consistencyCheckRequested()

    Button {
        text: "审校"
        font.pixelSize: 12
        enabled: root.enabledAll
        onClicked: root.consistencyCheckRequested()
        background: Rectangle {
            color: Theme.info
            radius: Theme.radiusSm
            opacity: enabled ? 1 : 0.5
        }
        contentItem: Text {
            text: parent.text
            color: "#ffffff"
            font: parent.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    Button {
        text: "复制"
        font.pixelSize: 12
        enabled: root.enabledAll
        onClicked: root.copyClicked()
        background: Rectangle {
            color: Theme.surface
            border.color: Theme.line
            radius: Theme.radiusSm
        }
        contentItem: Text {
            text: parent.text
            color: Theme.body
            font: parent.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    Button {
        text: "插入"
        font.pixelSize: 12
        enabled: root.enabledAll
        onClicked: root.insertClicked()
        background: Rectangle {
            color: Theme.aiSource
            radius: Theme.radiusSm
            opacity: enabled ? 1 : 0.5
        }
        contentItem: Text {
            text: parent.text
            color: "#ffffff"
            font: parent.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    Button {
        text: "替换本章"
        font.pixelSize: 12
        enabled: root.enabledAll
        onClicked: root.replaceClicked()
        background: Rectangle {
            color: Theme.warn
            radius: Theme.radiusSm
            opacity: enabled ? 1 : 0.5
        }
        contentItem: Text {
            text: parent.text
            color: "#ffffff"
            font: parent.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    Button {
        text: "新章"
        font.pixelSize: 12
        enabled: root.enabledAll
        onClicked: root.newChapterClicked()
        background: Rectangle {
            color: Theme.info
            radius: Theme.radiusSm
            opacity: enabled ? 1 : 0.5
        }
        contentItem: Text {
            text: parent.text
            color: "#ffffff"
            font: parent.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    Button {
        text: "重生"
        font.pixelSize: 12
        enabled: root.enabledAll
        onClicked: root.regenerateClicked()
        background: Rectangle {
            color: Theme.danger
            radius: Theme.radiusSm
            opacity: enabled ? 1 : 0.5
        }
        contentItem: Text {
            text: parent.text
            color: "#ffffff"
            font: parent.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
