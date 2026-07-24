pragma Singleton
import QtQuick 2.15

// 山河AI写作 · 全局设计系统（Design System）
// 暗色「水墨 + 暖金」基调，克制、专业、有写作工具质感。
// 旧属性名全部保留（兼容既有界面），新增 surface / 语义色 / 排版 / 间距 / 圆角 / 动效 token。
QtObject {
    // ---------- 兼容旧属性（请勿删除，既有的 Workbench/Studio 等仍引用） ----------
    property color bg:        "#0d0f14"
    property color bg2:       "#11141b"
    property color panel:     "#161a22"
    property color panel2:    "#1c2230"
    property color line:      "#2a3140"
    property color gold:      "#c9a85f"
    property color goldBr:    "#e7c987"
    property color ink:       "#ece8df"
    property color sub:       "#8b94a7"
    property color male:      "#6ea8fe"
    property color female:    "#ff8fb1"
    property color ok:        "#4fb98e"
    property color danger:    "#e5705e"
    property real r:          14
    property real rSm:        9

    // ---------- 背景层级（surface 体系，替代零散 panel 命名） ----------
    property color surface:      "#161a22"  // 卡片 / 面板
    property color surface2:     "#1c2230"  // 凸起 / 悬浮 / 输入框
    property color surfaceHover:  "#232b3a"  // hover 态
    property color surfacePressed:"#10131a" // 按下态
    property color lineSoft:     "#1f2531"  // 更弱分隔线
    property color overlay:      "#05070b"  // 模态遮罩底色

    // ---------- 文字层级 ----------
    property color body:        "#cdd3df"   // 常规正文
    property color faint:       "#5b6478"   // 占位 / 最弱

    // ---------- 品牌（暖金）与语义色 ----------
    property color primary:      "#d9b36a"  // 主行动色（雅致金，非刺眼金箔）
    property color primaryHi:    "#e7c987"  // 高亮金
    property color primarySoft:  "#d9b36a"
    property real  primaryA:     0.14        // 主色柔光透明度（用于底色块）
    property color success:      "#4fb98e"
    property color warn:         "#e0a347"
    property color info:         "#6ea8fe"
    property color dangerHi:     "#f08a7c"

    // 性别 / 频道流（保留 male/female，新增柔光底）
    property color maleSoft:     "#6ea8fe"
    property color femaleSoft:   "#ff8fb1"

    // ---------- AI 生成来源标识（借鉴 NovelAI：区分 AI / 用户文本） ----------
    property color aiSource:     "#7fd1c4"  // 青绿，标注 AI 生成内容
    property real  aiSourceA:    0.14       // AI 标识柔光透明度
    property color aiSourceLine: "#3a5e58"  // AI 标识左侧细线

    // ---------- 成就 / 鼓励提醒（借鉴 Novlr 写作目标达成提醒） ----------
    property color nudgeFg:      "#7fd1c4"
    property color nudgeBg:      "#16221f"

    // ---------- 排版（Typography） ----------
    property string fontFamily: "'PingFang SC','Microsoft YaHei UI','Source Han Sans SC','Noto Sans CJK SC',system-ui,sans-serif"
    property string fontMono:   "'JetBrains Mono','Fira Code',ui-monospace,SFMono-Regular,Menlo,Consolas,monospace"
    // 字号阶梯（px）：xs 11 / sm 12 / base 13 / md 14 / lg 16 / xl 18 / 2xl 20 / 3xl 24 / display 28
    property int tXs: 11
    property int tSm: 12
    property int tBase: 13
    property int tMd: 14
    property int tLg: 16
    property int tXl: 18
    property int t2xl: 20
    property int t3xl: 24
    property int tDisplay: 28
    // 字重
    property int wNormal: 400
    property int wMedium: 500
    property int wSemi: 600
    property int wBold: 700
    // 行高倍率
    property real lhTight: 1.25
    property real lhNormal: 1.55
    property real lhRelaxed: 1.75

    // ---------- 间距（8pt 基线网格） ----------
    property int sp1: 4
    property int sp2: 8
    property int sp3: 12
    property int sp4: 16
    property int sp5: 20
    property int sp6: 24
    property int sp8: 32
    property int sp10: 40
    property int sp12: 48

    // ---------- 圆角 ----------
    property int radiusSm: 8
    property int radiusMd: 12
    property int radiusLg: 16
    property int radiusXl: 22
    property int radiusPill: 999

    // ---------- 阴影 token（组件内用半透明圆角矩形模拟，Qt 原生 layer 无 shadow 属性） ----------
    property real shSmR: 6;   property real shSmY: 1;  property real shSmO: 0.30
    property real shMdR: 18;  property real shMdY: 6;  property real shMdO: 0.38
    property real shLgR: 40;  property real shLgY: 18; property real shLgO: 0.46

    // ---------- 动效时长（ms） ----------
    property int durXs: 90
    property int durFast: 140
    property int durNormal: 220
    property int durSlow: 340
    property int durXl: 480
}
