pragma Singleton
import QtQuick 2.15
import QtQuick.LocalStorage 2.0

QtObject {
    id: root

    property string skin: "novel"
    property bool dark: true

    function setSkin(s) { skin = s; saveSkin() }
    function toggleDark() { dark = !dark; saveDark() }

    readonly property var palettes: ({
        novel: {
            dark: {
                bg: "#0d0f14", bg2: "#11141b", panel: "#161a22", panel2: "#1c2230",
                surface: "#161a22", surface2: "#1c2230", surfaceHover: "#232b3a", surfacePressed: "#10131a",
                line: "#2a3140", lineSoft: "#1f2531", overlay: "#05070b",
                ink: "#ece8df", body: "#d1d7e3", sub: "#9aa3b6", faint: "#6b7488",
                primary: "#d9b36a", primaryHi: "#e7c987", primarySoft: "#d9b36a", primaryA: 0.14,
                gold: "#c9a85f", goldBr: "#e7c987",
                success: "#4fb98e", warn: "#e0a347", info: "#6ea8fe",
                danger: "#e5705e", dangerHi: "#f08a7c", ok: "#4fb98e",
                male: "#6ea8fe", female: "#ff8fb1", maleSoft: "#6ea8fe", femaleSoft: "#ff8fb1",
                aiSource: "#7fd1c4", aiSourceA: 0.14, aiSourceLine: "#3a5e58",
                nudgeFg: "#7fd1c4", nudgeBg: "#16221f",
                fontFamily: '"Noto Serif SC", "PingFang SC", "Source Han Serif SC", "Source Han Sans SC", "Microsoft YaHei UI", serif',
                fontMono: '"JetBrains Mono","Fira Code",ui-monospace,SFMono-Regular,Menlo,Consolas,monospace',
                editorFontSize: 17, editorLineHeight: 2.0,
                hasGlow: false, glow: "transparent"
            },
            light: {
                bg: "#faf7f2", bg2: "#f0ece4", panel: "#ffffff", panel2: "#f7f4ed",
                surface: "#ffffff", surface2: "#f7f4ed", surfaceHover: "#ede9df", surfacePressed: "#e8e4da",
                line: "#e0d9cd", lineSoft: "#ede9df", overlay: "#000000",
                ink: "#2a2520", body: "#443e36", sub: "#6a6358", faint: "#a69e90",
                primary: "#b08a3a", primaryHi: "#c9a35a", primarySoft: "#b08a3a", primaryA: 0.10,
                gold: "#b08a3a", goldBr: "#c9a35a",
                success: "#059669", warn: "#d97706", info: "#2563eb",
                danger: "#dc2626", dangerHi: "#ef4444", ok: "#059669",
                male: "#2563eb", female: "#db2777", maleSoft: "#2563eb", femaleSoft: "#db2777",
                aiSource: "#2d8f7e", aiSourceA: 0.10, aiSourceLine: "#a8d5cc",
                nudgeFg: "#2d8f7e", nudgeBg: "#f0f9f6",
                fontFamily: '"Noto Serif SC", "PingFang SC", "Source Han Serif SC", "Source Han Sans SC", "Microsoft YaHei UI", serif',
                fontMono: '"JetBrains Mono","Fira Code",ui-monospace,SFMono-Regular,Menlo,Consolas,monospace',
                editorFontSize: 17, editorLineHeight: 2.0,
                hasGlow: false, glow: "transparent"
            }
        },
        mass: {
            dark: {
                bg: "#0b0d12", bg2: "#10131a", panel: "#151a22", panel2: "#1b212c",
                surface: "#151a22", surface2: "#1b212c", surfaceHover: "#232a37", surfacePressed: "#0d1016",
                line: "#272e3b", lineSoft: "#1f2531", overlay: "#05070b",
                ink: "#e8ecf2", body: "#c1c8d4", sub: "#9aa3b3", faint: "#737d8d",
                primary: "#3b82f6", primaryHi: "#60a5fa", primarySoft: "#3b82f6", primaryA: 0.14,
                gold: "#3b82f6", goldBr: "#60a5fa",
                success: "#10b981", warn: "#f59e0b", info: "#3b82f6",
                danger: "#ef4444", dangerHi: "#f87171", ok: "#10b981",
                male: "#3b82f6", female: "#ec4899", maleSoft: "#3b82f6", femaleSoft: "#ec4899",
                aiSource: "#14b8a6", aiSourceA: 0.14, aiSourceLine: "#0f766e",
                nudgeFg: "#14b8a6", nudgeBg: "#0f2c29",
                fontFamily: '"Inter", "PingFang SC", "Microsoft YaHei", "Source Han Sans SC", "Noto Sans CJK SC", system-ui, sans-serif',
                fontMono: '"JetBrains Mono","Fira Code",ui-monospace,SFMono-Regular,Menlo,Consolas,monospace',
                editorFontSize: 15, editorLineHeight: 1.7,
                hasGlow: false, glow: "transparent"
            },
            light: {
                bg: "#f8fafc", bg2: "#f1f5f9", panel: "#ffffff", panel2: "#f8fafc",
                surface: "#ffffff", surface2: "#f8fafc", surfaceHover: "#eef2f7", surfacePressed: "#e2e8f0",
                line: "#e2e8f0", lineSoft: "#f1f5f9", overlay: "#000000",
                ink: "#0f172a", body: "#2c394a", sub: "#5b6b82", faint: "#94a3b8",
                primary: "#2563eb", primaryHi: "#3b82f6", primarySoft: "#2563eb", primaryA: 0.10,
                gold: "#2563eb", goldBr: "#3b82f6",
                success: "#059669", warn: "#d97706", info: "#2563eb",
                danger: "#dc2626", dangerHi: "#ef4444", ok: "#059669",
                male: "#2563eb", female: "#db2777", maleSoft: "#2563eb", femaleSoft: "#db2777",
                aiSource: "#0d9488", aiSourceA: 0.10, aiSourceLine: "#99f6e4",
                nudgeFg: "#0d9488", nudgeBg: "#f0fdfa",
                fontFamily: '"Inter", "PingFang SC", "Microsoft YaHei", "Source Han Sans SC", "Noto Sans CJK SC", system-ui, sans-serif',
                fontMono: '"JetBrains Mono","Fira Code",ui-monospace,SFMono-Regular,Menlo,Consolas,monospace',
                editorFontSize: 15, editorLineHeight: 1.7,
                hasGlow: false, glow: "transparent"
            }
        },
        neon: {
            dark: {
                bg: "#080810", bg2: "#0c0c18", panel: "#12121e", panel2: "#15152a",
                surface: "#15152a", surface2: "#1a1a35", surfaceHover: "#1e1e3a", surfacePressed: "#0a0a18",
                line: "#252545", lineSoft: "#1a1a35", overlay: "#020208",
                ink: "#e6e6ff", body: "#b0b0de", sub: "#8a8ab3", faint: "#666690",
                primary: "#22d3ee", primaryHi: "#67e8f9", primarySoft: "#22d3ee", primaryA: 0.18,
                gold: "#22d3ee", goldBr: "#67e8f9",
                success: "#34d399", warn: "#fbbf24", info: "#22d3ee",
                danger: "#f87171", dangerHi: "#fca5a5", ok: "#34d399",
                male: "#60a5fa", female: "#f472b6", maleSoft: "#60a5fa", femaleSoft: "#f472b6",
                aiSource: "#34d399", aiSourceA: 0.18, aiSourceLine: "#065f46",
                nudgeFg: "#34d399", nudgeBg: "#0a2a1e",
                fontFamily: '"Space Grotesk", "PingFang SC", "Microsoft YaHei", "Source Han Sans SC", "Noto Sans CJK SC", system-ui, sans-serif',
                fontMono: '"JetBrains Mono","Fira Code",ui-monospace,SFMono-Regular,Menlo,Consolas,monospace',
                editorFontSize: 15, editorLineHeight: 1.7,
                hasGlow: true, glow: "#22d3ee"
            },
            light: {
                bg: "#f5f5ff", bg2: "#ebebff", panel: "#ffffff", panel2: "#f8f8ff",
                surface: "#ffffff", surface2: "#f8f8ff", surfaceHover: "#eeefff", surfacePressed: "#e0e0ff",
                line: "#dddde5", lineSoft: "#ebebff", overlay: "#000000",
                ink: "#1a1a33", body: "#424270", sub: "#6a6a99", faint: "#a5a5d9",
                primary: "#0891b2", primaryHi: "#06b6d4", primarySoft: "#0891b2", primaryA: 0.10,
                gold: "#0891b2", goldBr: "#06b6d4",
                success: "#059669", warn: "#d97706", info: "#0891b2",
                danger: "#dc2626", dangerHi: "#ef4444", ok: "#059669",
                male: "#2563eb", female: "#db2777", maleSoft: "#2563eb", femaleSoft: "#db2777",
                aiSource: "#059669", aiSourceA: 0.10, aiSourceLine: "#a7f3d0",
                nudgeFg: "#059669", nudgeBg: "#ecfdf5",
                fontFamily: '"Space Grotesk", "PingFang SC", "Microsoft YaHei", "Source Han Sans SC", "Noto Sans CJK SC", system-ui, sans-serif',
                fontMono: '"JetBrains Mono","Fira Code",ui-monospace,SFMono-Regular,Menlo,Consolas,monospace',
                editorFontSize: 15, editorLineHeight: 1.7,
                hasGlow: false, glow: "transparent"
            }
        }
    })

    readonly property var p: palettes[skin] ? palettes[skin][dark ? "dark" : "light"] : palettes.novel.dark

    readonly property color bg: p.bg
    readonly property color bg2: p.bg2
    readonly property color panel: p.panel
    readonly property color panel2: p.panel2
    readonly property color surface: p.surface
    readonly property color surface2: p.surface2
    readonly property color surfaceHover: p.surfaceHover
    readonly property color surfacePressed: p.surfacePressed
    readonly property color line: p.line
    readonly property color lineSoft: p.lineSoft
    readonly property color overlay: p.overlay
    readonly property color ink: p.ink
    readonly property color body: p.body
    readonly property color sub: p.sub
    readonly property color faint: p.faint
    readonly property color primary: p.primary
    readonly property color primaryHi: p.primaryHi
    readonly property color primarySoft: p.primarySoft
    readonly property real primaryA: p.primaryA
    readonly property color gold: p.gold
    readonly property color goldBr: p.goldBr
    readonly property color success: p.success
    readonly property color warn: p.warn
    readonly property color info: p.info
    readonly property color danger: p.danger
    readonly property color dangerHi: p.dangerHi
    readonly property color ok: p.ok
    readonly property color male: p.male
    readonly property color female: p.female
    readonly property color maleSoft: p.maleSoft
    readonly property color femaleSoft: p.femaleSoft
    readonly property color aiSource: p.aiSource
    readonly property real aiSourceA: p.aiSourceA
    readonly property color aiSourceLine: p.aiSourceLine
    readonly property color nudgeFg: p.nudgeFg
    readonly property color nudgeBg: p.nudgeBg

    readonly property string fontFamily: p.fontFamily
    readonly property string fontMono: p.fontMono
    readonly property int editorFontSize: p.editorFontSize
    readonly property real editorLineHeight: p.editorLineHeight

    readonly property int tXs: 11
    readonly property int tSm: 12
    readonly property int tBase: 13
    readonly property int tMd: 14
    readonly property int tLg: 16
    readonly property int tXl: 18
    readonly property int t2xl: 20
    readonly property int t3xl: 24
    readonly property int tDisplay: 28
    readonly property int wNormal: 400
    readonly property int wMedium: 500
    readonly property int wSemi: 600
    readonly property int wBold: 700
    readonly property real lhTight: 1.25
    readonly property real lhNormal: 1.55
    readonly property real lhRelaxed: 1.75

    readonly property int sp1: 4
    readonly property int sp2: 8
    readonly property int sp3: 12
    readonly property int sp4: 16
    readonly property int sp5: 20
    readonly property int sp6: 24
    readonly property int sp8: 32
    readonly property int sp10: 40
    readonly property int sp12: 48

    readonly property real r: 14
    readonly property real rSm: 9
    readonly property int radiusSm: 8
    readonly property int radiusMd: 12
    readonly property int radiusLg: 16
    readonly property int radiusXl: 22
    readonly property int radiusPill: 999

    readonly property real shSmR: 6
    readonly property real shSmY: 1
    readonly property real shSmO: 0.30
    readonly property real shMdR: 18
    readonly property real shMdY: 6
    readonly property real shMdO: 0.38
    readonly property real shLgR: 40
    readonly property real shLgY: 18
    readonly property real shLgO: 0.46

    readonly property int durXs: 90
    readonly property int durSm: 160
    readonly property int durFast: 140
    readonly property int durMd: 220
    readonly property int durNormal: 220
    readonly property int durLg: 320
    readonly property int durSlow: 340
    readonly property int durXl: 480
    readonly property var easeOut: Easing.OutCubic
    readonly property var easeInOut: Easing.InOutCubic

    readonly property bool hasGlow: p.hasGlow || false
    readonly property color glow: p.glow || "transparent"

    function saveSkin() {
        try {
            var db = LocalStorage.openDatabaseSync("ShanHeTheme", "1.0", "Theme Settings", 1000000)
            db.transaction(function(tx) {
                tx.executeSql('CREATE TABLE IF NOT EXISTS settings(key TEXT PRIMARY KEY, value TEXT)')
                tx.executeSql('INSERT OR REPLACE INTO settings(key, value) VALUES(?, ?)', ["skin", skin])
            })
        } catch(e) {}
    }
    function saveDark() {
        try {
            var db = LocalStorage.openDatabaseSync("ShanHeTheme", "1.0", "Theme Settings", 1000000)
            db.transaction(function(tx) {
                tx.executeSql('CREATE TABLE IF NOT EXISTS settings(key TEXT PRIMARY KEY, value TEXT)')
                tx.executeSql('INSERT OR REPLACE INTO settings(key, value) VALUES(?, ?)', ["dark", dark ? "true" : "false"])
            })
        } catch(e) {}
    }
    function loadFromStorage() {
        try {
            var db = LocalStorage.openDatabaseSync("ShanHeTheme", "1.0", "Theme Settings", 1000000)
            db.transaction(function(tx) {
                tx.executeSql('CREATE TABLE IF NOT EXISTS settings(key TEXT PRIMARY KEY, value TEXT)')
                var res = tx.executeSql('SELECT value FROM settings WHERE key = ?', ["skin"])
                if (res.rows.length > 0) {
                    var savedSkin = res.rows.item(0).value
                    if (palettes[savedSkin]) skin = savedSkin
                }
                res = tx.executeSql('SELECT value FROM settings WHERE key = ?', ["dark"])
                if (res.rows.length > 0) {
                    dark = res.rows.item(0).value === "true"
                }
            })
        } catch(e) {}
    }
    Component.onCompleted: loadFromStorage()
}
