import QtQuick 2.15
import QtQuick.Controls 2.15
import ShanHe 1.0

// 进度环（Canvas 绘制，零额外依赖），统一走设计系统配色
Item {
    id: root
    property real progress: 0
    property int size: 70
    property color color: Theme.primaryHi
    property int thickness: 7
    width: size; height: size

    Canvas {
        id: cv
        anchors.fill: parent
        antialiasing: true
        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            const cx = width / 2, cy = height / 2
            const r = (Math.min(width, height) - root.thickness) / 2
            ctx.lineWidth = root.thickness
            ctx.strokeStyle = Theme.line
            ctx.beginPath(); ctx.arc(cx, cy, r, 0, Math.PI * 2); ctx.stroke()
            const start = -Math.PI / 2
            const end = start + (root.progress / 100) * Math.PI * 2
            ctx.lineWidth = root.thickness
            ctx.lineCap = "round"
            ctx.strokeStyle = root.color
            ctx.beginPath(); ctx.arc(cx, cy, r, start, end); ctx.stroke()
        }
    }
    Text {
        anchors.centerIn: parent
        text: Math.round(root.progress) + "%"
        color: Theme.ink
        font.family: Theme.fontFamily
        font.pixelSize: root.size * 0.26
        font.bold: true
    }
    onProgressChanged: cv.requestPaint()
    Component.onCompleted: cv.requestPaint()
}
