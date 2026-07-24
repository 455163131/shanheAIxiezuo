import QtQuick 2.15
import ShanHe 1.0

// 单色描边图标（Lucide 风格），用内联 SVG(data URI) 渲染，替代 emoji。
// 用法：Icon { name: "settings"; color: Theme.ink; size: 18 }
Item {
    id: root
    property string name: ""
    property color color: Theme.sub
    property int size: 18
    property real weight: 2          // stroke-width
    width: size; height: size

    readonly property var paths: ({
        "settings": "M12.22 2h-.44a2 2 0 0 0-2 2v.18a2 2 0 0 1-1 1.73l-.43.25a2 2 0 0 1-2 0l-.15-.08a2 2 0 0 0-2.73.73l-.22.38a2 2 0 0 0 .73 2.73l.15.1a2 2 0 0 1 1 1.72v.51a2 2 0 0 1-1 1.74l-.15.09a2 2 0 0 0-.73 2.73l.22.38a2 2 0 0 0 2.73.73l.15-.08a2 2 0 0 1 2 0l.43.25a2 2 0 0 1 1 1.73V20a2 2 0 0 0 2 2h.44a2 2 0 0 0 2-2v-.18a2 2 0 0 1 1-1.73l.43-.25a2 2 0 0 1 2 0l.15.08a2 2 0 0 0 2.73-.73l.22-.38a2 2 0 0 0-.73-2.73l-.15-.08a2 2 0 0 1-1-1.74v-.5a2 2 0 0 1 1-1.74l.15-.09a2 2 0 0 0 .73-2.73l-.22-.38a2 2 0 0 0-2.73-.73l-.15.08a2 2 0 0 1-2 0l-.43-.25a2 2 0 0 1-1-1.73V4a2 2 0 0 0-2-2z M15 12a3 3 0 1 1-6 0 3 3 0 0 1 6 0z",
        "plus": "M5 12h14 M12 5v14",
        "book": "M4 19.5A2.5 2.5 0 0 1 6.5 17H20 M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z",
        "book-open": "M12 7v14 M3 18a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h6v18a2 2 0 0 1-2-2z M21 18a2 2 0 0 0 2-2V4a2 2 0 0 0-2-2h-6v18a2 2 0 0 0 2-2z",
        "pen": "M12 20h9 M16.5 3.5a2.12 2.12 0 0 1 3 3L7 19l-4 1 1-4Z",
        "split": "M12 3v18 M8 4h4 M8 20h4 M5 8a2 2 0 1 0 0-4 2 2 0 0 0 0 4z M19 8a2 2 0 1 0 0-4 2 2 0 0 0 0 4z M5 20a2 2 0 1 0 0-4 2 2 0 0 0 0 4z M19 20a2 2 0 1 0 0-4 2 2 0 0 0 0 4z",
        "chevron-left": "M15 18l-6-6 6-6",
        "chevron-right": "M9 18l6-6-6-6",
        "chevron-down": "M6 9l6 6 6-6",
        "check": "M20 6 9 17l-5-5",
        "close": "M18 6 6 18 M6 6l12 12",
        "refresh": "M21 12a9 9 0 1 1-2.64-6.36 M21 3v6h-6",
        "alert": "M12 9v4 M12 17h.01 M10.29 3.86 1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z",
        "user": "M19 21v-2a4 4 0 0 0-4-4H9a4 4 0 0 0-4 4v2 M16 7a4 4 0 1 1-8 0 4 4 0 0 1 8 0z",
        "sparkles": "M12 3l1.9 5.8L19.7 10.6 13.9 12.5 12 18.3 10.1 12.5 4.3 10.6 10.1 8.8z M19 14.5l.85 2.6L22.5 17.95l-2.65.85L19 21.4l-.85-2.6L15.5 17.95l2.65-.85z",
        "arrow-right": "M5 12h14 M13 5l7 7-7 7",
        "plug": "M12 22v-5 M9 8V2 M15 8V2 M7 8h10v3a5 5 0 0 1-10 0z",
        "layers": "M12 2 2 7l10 5 10-5-10-5z M2 17l10 5 10-5 M2 12l10 5 10-5",
        "wand": "M15 4V2 M15 16v-2 M8 9h2 M20 9h2 M17.8 11.8 19 13 M17.8 6.2 19 5 M3 21l9-9",
        "target": "M12 12m-10 0a10 10 0 1 0 20 0a10 10 0 1 0-20 0 M12 12m-6 0a6 6 0 1 0 12 0a6 6 0 1 0-12 0 M12 12m-2 0a2 2 0 1 0 4 0a2 2 0 1 0-4 0",
        "flame": "M12 2s4 4 4 9a4 4 0 0 1-8 0c0-1 .5-2 1-3-2 1-4 3-4 6a7 7 0 0 0 14 0c0-5-7-9-7-12z",
        "dot": "M12 12m-3 0a3 3 0 1 0 6 0a3 3 0 1 0-6 0",
        "send": "M22 2 11 13 M22 2 15 22l-4-9-9-4Z",
        "stop": "M18 6 6 18 M6 6l12 12",
        "type": "M4 7V4h16v3 M9 20h6 M12 4v16",
        "focus": "M8 3H5a2 2 0 0 0-2 2v3 M21 8V5a2 2 0 0 0-2-2h-3 M3 16v3a2 2 0 0 0 2 2h3 M16 21h3a2 2 0 0 0 2-2v-3",
        "minimize": "M8 3v3a2 2 0 0 1-2 2H3 M21 8h-3a2 2 0 0 1-2-2V3 M3 16h3a2 2 0 0 1 2 2v3 M16 21v-3a2 2 0 0 1 2-2h3"
    })

    Image {
        anchors.fill: parent
        sourceSize.width: root.size
        sourceSize.height: root.size
        cache: false
        source: "data:image/svg+xml;utf8," + encodeURIComponent(
            "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' " +
            "stroke='" + root.color + "' stroke-width='" + root.weight + "' " +
            "stroke-linecap='round' stroke-linejoin='round'>" +
            "<path d='" + (paths[root.name] || "") + "'/></svg>")
    }
}
