import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import ShanHe 1.0

// 资源库屏（P0 占位，P3 填充流派/人格/提示词管理）
// 当前仅展示 EmptyState 风格占位，让 SideNav 4 入口都能切换到内容
Item {
    id: root

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: Theme.bg }
            GradientStop { position: 1; color: Theme.bg2 }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: Theme.sp4

        // 图标占位（用色块代替，P3 接入真实 Icon）
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 72; height: 72; radius: Theme.radiusLg
            color: Theme.surface2
            border.color: Theme.line; border.width: 1
            Label {
                anchors.centerIn: parent
                text: "山"
                color: Theme.primary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.tDisplay
                font.bold: true
            }
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "资源库"
            color: Theme.ink
            font.family: Theme.fontFamily
            font.pixelSize: Theme.t2xl
            font.bold: true
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "流派范式 · 人格模板 · 提示词片段"
            color: Theme.sub
            font.family: Theme.fontFamily
            font.pixelSize: Theme.tSm
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "（P3 阶段填充，当前为占位）"
            color: Theme.faint
            font.family: Theme.fontFamily
            font.pixelSize: Theme.tXs
        }
    }
}
