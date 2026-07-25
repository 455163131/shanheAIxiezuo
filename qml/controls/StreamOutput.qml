import QtQuick 2.15
import QtQuick.Controls 2.15
import ShanHe 1.0

ScrollView {
    id: root
    implicitWidth: 300
    implicitHeight: 200

    property string text: ""
    property bool generating: false
    property string thinkingPhase: ""

    signal textChanged()
    signal generatingChanged()

    clip: true

    Flickable {
        id: flick
        contentWidth: width
        contentHeight: contentColumn.height
        anchors.fill: parent

        Column {
            id: contentColumn
            width: parent.width
            spacing: Theme.sp3

            Text {
                id: outputText
                width: parent.width
                text: root.text
                color: Theme.body
                font.family: Theme.fontFamily
                font.pixelSize: Theme.editorFontSize
                lineHeight: Theme.editorLineHeight
                wrapMode: Text.WordWrap
            }

            Row {
                visible: root.generating
                spacing: Theme.sp2
                opacity: 0.6

                Rectangle {
                    id: cursor
                    width: 2
                    height: outputText.font.pixelSize
                    color: Theme.primary
                    NumberAnimation on opacity {
                        running: root.generating
                        from: 1; to: 0
                        duration: 500
                        loops: Animation.Infinite
                    }
                }
                Text {
                    text: root.thinkingPhase
                    color: Theme.sub
                    font.pixelSize: 12
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        property bool followBottom: true

        onContentYChanged: {
            if (flick.moving && !flick.atYEnd) {
                followBottom = false
            }
        }

        Binding on followBottom {
            value: flick.atYEnd
            when: !flick.moving
        }

        function ensureVisible() {
            if (followBottom) flick.contentY = flick.contentHeight - flick.height
        }
    }

    onTextChanged: flick.ensureVisible()
}
