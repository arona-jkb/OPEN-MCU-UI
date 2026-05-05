"""Generate Chinese API reference PDF for the STM32 GUI framework."""
import os
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import mm
from reportlab.lib.colors import HexColor
from reportlab.platypus import (SimpleDocTemplate, Paragraph, Spacer,
                                 Table, TableStyle, PageBreak)
from reportlab.lib.enums import TA_LEFT, TA_CENTER
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont

# Register Chinese font
FONT = 'SimSun'
FONT_BOLD = 'SimSun'
pdfmetrics.registerFont(TTFont(FONT, 'C:/Windows/Fonts/simsun.ttc', subfontIndex=0))

OUTPUT = os.path.join(os.path.dirname(__file__), "GUI_Framework_API_Reference_zh.pdf")

doc = SimpleDocTemplate(OUTPUT, pagesize=A4,
                        leftMargin=20*mm, rightMargin=20*mm,
                        topMargin=15*mm, bottomMargin=15*mm)

styles = getSampleStyleSheet()

# Override built-in styles with Chinese font
for key in list(styles.byName.keys()):
    s = styles[key]
    s.fontName = FONT
    if 'Bold' in key or 'Title' in key or 'Heading' in key:
        pass  # keep bold-ish
    if key == 'Title':
        s.fontSize = 20
        s.leading = 26
    if 'Heading1' in key:
        s.fontSize = 15
        s.leading = 20
        s.spaceBefore = 16
        s.spaceAfter = 6
    if 'Heading2' in key:
        s.fontSize = 12
        s.leading = 16
        s.spaceBefore = 12
        s.spaceAfter = 4
    if 'Heading3' in key:
        s.fontSize = 10.5
        s.leading = 14
        s.spaceBefore = 10
        s.spaceAfter = 2
    if key == 'Normal':
        s.fontSize = 9
        s.leading = 13

# Custom styles
styles.add(ParagraphStyle(name='CodeBlock', fontName='Courier', fontSize=7.5,
                          leading=10, leftIndent=8, spaceBefore=4, spaceAfter=4,
                          backColor=HexColor('#F4F4F4'),
                          borderColor=HexColor('#CCCCCC'),
                          borderWidth=0.5, borderPadding=4))
styles.add(ParagraphStyle(name='CodeSm', fontName='Courier', fontSize=6.8,
                          leading=9, leftIndent=8, spaceBefore=2, spaceAfter=2,
                          backColor=HexColor('#F4F4F4'),
                          borderColor=HexColor('#CCCCCC'),
                          borderWidth=0.5, borderPadding=3))
styles.add(ParagraphStyle(name='H3zh', fontName=FONT, fontSize=10.5,
                          leading=14, spaceBefore=10, spaceAfter=2))
styles.add(ParagraphStyle(name='Note', fontName=FONT, fontSize=8.5,
                          leading=12, leftIndent=10, spaceBefore=4, spaceAfter=4,
                          textColor=HexColor('#666666')))
styles.add(ParagraphStyle(name='BulletZH', fontName=FONT, fontSize=9,
                          leading=13, leftIndent=12, bulletIndent=0))

story = []

def h1(t):   story.extend([Spacer(1, 4*mm), Paragraph(t, styles['Heading1'])])
def h2(t):   story.extend([Spacer(1, 3*mm), Paragraph(t, styles['Heading2'])])
def h3(t):   story.append(Paragraph(t, styles['H3zh']))
def body(t): story.append(Paragraph(t, styles['Normal']))
def bul(t):  story.append(Paragraph('• ' + t, styles['BulletZH']))
def code(t): story.append(Paragraph(t.replace(' ', '&nbsp;').replace('\n', '<br/>'), styles['CodeBlock']))
def codesm(t): story.append(Paragraph(t.replace(' ', '&nbsp;').replace('\n', '<br/>'), styles['CodeSm']))
def note(t): story.append(Paragraph(t, styles['Note']))
def gap():   story.append(Spacer(1, 2*mm))

def api_table(rows):
    data = [['函数', '说明']]
    for sig, desc in rows:
        data.append([Paragraph(sig, styles['CodeSm']), Paragraph(desc, styles['Normal'])])
    t = Table(data, colWidths=[doc.width*0.52, doc.width*0.48])
    t.setStyle(TableStyle([
        ('BACKGROUND', (0,0), (-1,0), HexColor('#333333')),
        ('TEXTCOLOR', (0,0), (-1,0), HexColor('#FFFFFF')),
        ('FONTSIZE', (0,0), (-1,-1), 8),
        ('GRID', (0,0), (-1,-1), 0.5, HexColor('#CCCCCC')),
        ('VALIGN', (0,0), (-1,-1), 'TOP'),
        ('TOPPADDING', (0,0), (-1,-1), 3),
        ('BOTTOMPADDING', (0,0), (-1,-1), 3),
        ('LEFTPADDING', (0,0), (-1,-1), 4),
    ]))
    story.append(t)
    gap()

# ============================================================
# 封面
# ============================================================
story.append(Spacer(1, 30*mm))
story.append(Paragraph("STM32 GUI Framework", styles['Title']))
story.append(Spacer(1, 4*mm))
story.append(Paragraph("API 参考手册", styles['Heading2']))
story.append(Spacer(1, 8*mm))
story.append(Paragraph("目标平台：STM32F103xB + SSD1306 128x64 OLED<br/>"
                        "底层驱动：u8g2 &amp; STM32CubeMX HAL<br/>"
                        "版本 1.0 &mdash; 2026-05",
                        styles['Normal']))
story.append(Spacer(1, 15*mm))
story.append(Paragraph(
    "<b>模块总览：</b><br/>"
    "• <b>menu</b> &mdash; 多级菜单系统，支持页面切换动画<br/>"
    "• <b>popup</b> &mdash; 弹窗对话框（数值调节 / 布尔开关 / Toast 通知）<br/>"
    "• <b>ux_move</b> &mdash; 基于坐标插值的动画引擎<br/>"
    "• <b>Key</b> &mdash; 四按键输入，软件去抖动",
    styles['Normal']))
story.append(PageBreak())

# ============================================================
# 1. 项目概览
# ============================================================
h1("1. 项目概览")
body("本框架为 STM32 平台上的单色 OLED 显示器提供完整的 GUI 工具集，构建于 u8g2 图形库 "
     "与 STM32CubeMX HAL 之上。包含多级菜单、弹窗对话框、平滑动画以及整洁的开发者 API。")

h2("1.1 硬件需求")
bul("MCU: STM32F103xB (Cortex-M3, 20 KB RAM, 64 KB Flash)")
bul("显示屏: SSD1306 128x64 OLED，I2C 接口，地址 0x78")
bul("按键: 4 个，上拉输入 (PC13 / PA0 / PA1 / PA2)")

h2("1.2 文件结构")
codesm("Core/Inc/menu.h       菜单系统头文件\n"
       "Core/Src/menu.c       菜单系统实现\n"
       "Core/Inc/popup.h      弹窗系统头文件（数值 / 布尔 / Toast）\n"
       "Core/Src/popup.c      弹窗系统实现\n"
       "Core/Inc/ux_move.h    动画引擎头文件\n"
       "Core/Src/ux_move.c    动画引擎实现\n"
       "Core/Inc/Key.h        按键输入头文件\n"
       "Core/Src/Key.c        按键去抖动与读取\n"
       "Core/Inc/stm32_u8g2.h u8g2 硬件适配层\n"
       "Core/Src/stm32_u8g2.c I2C 回调与延迟函数")

h2("1.3 构建命令")
body("开发者编写的 .c 文件请添加到根目录 <b>CMakeLists.txt</b> "
     "（<i>非</i> cmake/stm32cubemx/CMakeLists.txt，后者由 CubeMX 自动生成）：")
code("target_sources(${CMAKE_PROJECT_NAME} PRIVATE\n"
     "    Core/Src/stm32_u8g2.c\n"
     "    Core/Src/ux_move.c\n"
     "    Core/Src/Key.c\n"
     "    Core/Src/menu.c\n"
     "    Core/Src/popup.c\n"
     ")")
body("配置与构建：")
code("cmake --preset Debug\n"
     "cmake --build build/Debug")

story.append(PageBreak())

# ============================================================
# 2. 菜单系统
# ============================================================
h1("2. 菜单系统 (menu.h)")

h2("2.1 数据结构")
h3("menu_item_t（菜单项）")
body("表示菜单中的一行。")
code("typedef struct menu_item {\n"
     "    const char              *name;       // 显示文字\n"
     "    void                   (*action)(void);  // 按确认键时回调\n"
     "    const struct menu_page *submenu;    // 子菜单页，无则为 NULL\n"
     "} menu_item_t;")
note("若 action != NULL，用户按下确认键(Key 3)时触发回调。"
     "若 submenu != NULL，按确认键时跳转至该子页面。"
     "action 优先级高于 submenu。")

h3("menu_page_t（菜单页）")
body("表示菜单层级中的一页。")
code("typedef struct menu_page {\n"
     "    const char           *title;    // 标题栏显示文字\n"
     "    const menu_item_t    *items;    // 菜单项数组\n"
     "    uint8_t               count;    // 菜单项数量\n"
     "    const struct menu_page *parent; // 父页面 (根页面为 NULL)\n"
     "} menu_page_t;")

h3("menu_state_t（菜单运行时状态）")
body("菜单系统的全局状态，在 main.c 中分配一个实例即可。")
code("typedef struct {\n"
     "    const menu_page_t *current;\n"
     "    uint8_t            selected;\n"
     "    anim_ctrl_t        scroll_anim;   // 文字滚动\n"
     "    anim_ctrl_t        bar_anim;      // 选择条\n"
     "    int16_t             bar_target_y, bar_target_w;\n"
     "    menu_trans_e        trans;         // 页面切换状态\n"
     "    anim_ctrl_t         trans_anim;    // 切换进度 0->100\n"
     "} menu_state_t;")

h2("2.2 API 参考")
api_table([
    ("void menu_init(menu_state_t *s,<br/>&nbsp;&nbsp;const menu_page_t *root)",
     "以给定的根页面初始化菜单状态。启动时调用一次。"),
    ("void menu_update(menu_state_t *s)",
     "每帧在主循环中调用，紧随 anim_manager_update() 之后。处理页面切换的收尾工作。"),
    ("void menu_render(u8g2_t *u8g2,<br/>&nbsp;&nbsp;menu_state_t *s)",
     "将当前菜单页（或切换动画）渲染至 u8g2 帧缓冲区。需要包裹在 u8g2_FirstPage / NextPage 之间。"),
    ("bool menu_key_up(menu_state_t *s)",
     "向上移动选择。有动作时返回 true。"),
    ("bool menu_key_down(menu_state_t *s)",
     "向下移动选择。有动作时返回 true。"),
    ("bool menu_key_enter(menu_state_t *s)",
     "执行 action 回调或进入子菜单。"),
    ("bool menu_key_back(menu_state_t *s)",
     "返回父页面。在根页面时无操作。"),
])

h2("2.3 页面切换动画")
body("用户进入子菜单或按返回键时，旧页面分裂离开（标题栏向上飞出，文字向下沉出），"
     "新页面从反方向汇聚进入。时长由 TRANS_MS 定义（默认 800 ms）。"
     "切换过程中选择条跟随新页面首行文字运动。")
body("切换期间菜单按键被屏蔽（menu_key_* 返回 false）。"
     "切换动画由 menu_update(.) 驱动，每帧必须调用。")

h2("2.4 集成示例")
code("// 1. 定义页面（静态数据）\n"
     "static const menu_item_t settings_items[] = {\n"
     "    {\"Brightness\", bright_cb, NULL},\n"
     "    {\"Power Save\", power_cb,  NULL},\n"
     "};\n"
     "static menu_page_t settings_page = {\n"
     "    .title=\"Settings\", .items=settings_items,\n"
     "    .count=2, .parent=NULL  // 运行时设置\n"
     "};\n"
     "static menu_item_t root_items[] = {\n"
     "    {\"Settings\", NULL, &settings_page},\n"
     "};\n"
     "static menu_page_t root_page = {\n"
     "    .title=\"Main Menu\", .items=root_items,\n"
     "    .count=1, .parent=NULL\n"
     "};\n"
     "\n"
     "// 2. 连接父页面指针（运行时）\n"
     "settings_page.parent = &root_page;\n"
     "\n"
     "// 3. 初始化\n"
     "static menu_state_t menu;\n"
     "menu_init(&menu, &root_page);\n"
     "\n"
     "// 4. 主循环\n"
     "while (1) {\n"
     "    anim_manager_update();\n"
     "    menu_update(&menu);\n"
     "    int8_t k = Key();\n"
     "    if (k==1) menu_key_up(&menu);\n"
     "    else if (k==2) menu_key_down(&menu);\n"
     "    else if (k==3) menu_key_enter(&menu);\n"
     "    else if (k==4) menu_key_back(&menu);\n"
     "    u8g2_FirstPage(&u8g2); do { menu_render(&u8g2, &menu);\n"
     "    } while (u8g2_NextPage(&u8g2));\n"
     "}")

story.append(PageBreak())

# ============================================================
# 3. 弹窗系统
# ============================================================
h1("3. 弹窗系统 (popup.h)")

body("三种弹窗类型共享同一个状态机（IDLE -> OPENING -> ACTIVE -> CLOSING -> IDLE），"
     "以及统一的滑入/滑出动画（由 ux_move 引擎驱动）。")

h2("3.1 数值调节弹窗 (popup_num_*)")
body("允许用户通过上下键调整整数数值。显示标题、分隔线以及居中大号数字。"
     "确认键(Key 3)或返回键(Key 4)关闭弹窗。")

h3("popup_num_cfg_t")
code("typedef struct {\n"
     "    const char *title;   // 弹窗标题\n"
     "    int16_t    *value;   // 指向被调节变量的指针\n"
     "    int16_t     min;     // 允许的最小值\n"
     "    int16_t     max;     // 允许的最大值\n"
     "    int16_t     step;    // 每次按键的步进量\n"
     "} popup_num_cfg_t;")

api_table([
    ("void popup_num_init(popup_num_t *p)", "初始化数值弹窗实例。"),
    ("void popup_num_open(popup_num_t *p,<br/>&nbsp;&nbsp;const char *title, int16_t *value,<br/>&nbsp;&nbsp;int16_t min, int16_t max, int16_t step)",
     "配置并打开弹窗，伴随滑入动画。"),
    ("bool popup_num_active(const popup_num_t *p)", "非 IDLE 状态时返回 true。"),
    ("void popup_num_update(popup_num_t *p,<br/>&nbsp;&nbsp;int8_t key)", "处理按键输入。空闲时为空操作。"),
    ("void popup_num_render(const popup_num_t *p,<br/>&nbsp;&nbsp;u8g2_t *u8g2)", "在菜单上方渲染。"),
])

h2("3.2 布尔开关弹窗 (popup_bool_*)")
body("一个开关切换器。上下键均翻转布尔值。可自定义 ON/OFF 标签。")

h3("popup_bool_cfg_t")
code("typedef struct {\n"
     "    const char *title;\n"
     "    bool       *value;\n"
     "    const char *text_on;     // true 时显示的文字\n"
     "    const char *text_off;    // false 时显示的文字\n"
     "} popup_bool_cfg_t;")

api_table([
    ("void popup_bool_init(popup_bool_t *p)", "初始化布尔弹窗实例。"),
    ("void popup_bool_open(popup_bool_t *p,<br/>&nbsp;&nbsp;const char *title, bool *value,<br/>&nbsp;&nbsp;const char *on, const char *off)",
     "配置并打开。"),
    ("bool popup_bool_active(const popup_bool_t *p)", "非 IDLE 状态时返回 true。"),
    ("void popup_bool_update(popup_bool_t *p,<br/>&nbsp;&nbsp;int8_t key)", "Key 1/2 = 翻转；Key 3/4 = 关闭。"),
    ("void popup_bool_render(const popup_bool_t *p,<br/>&nbsp;&nbsp;u8g2_t *u8g2)", "在菜单上方渲染。"),
])

h2("3.3 Toast 通知弹窗 (popup_toast_*)")
body("紧凑型自动消失通知。无需用户交互——滑入后停留 1 秒，自动滑出。"
     "使用较小字号 (6x10) 及紧凑内边距。")
body("<b>区别于其他弹窗：</b>popup_toast_update(.) <i>没有 key 参数</i>——关闭计时由 HAL_GetTick() 内部驱动。")

api_table([
    ("void popup_toast_init(popup_toast_t *p)", "初始化 Toast 实例。"),
    ("void popup_toast_show(popup_toast_t *p,<br/>&nbsp;&nbsp;const char *text)", "以指定的文本触发 Toast。"),
    ("bool popup_toast_active(const popup_toast_t *p)", "非 IDLE 状态时返回 true。"),
    ("void popup_toast_update(popup_toast_t *p)", "自动关闭计时。无 key 参数。"),
    ("void popup_toast_render(const popup_toast_t *p,<br/>&nbsp;&nbsp;u8g2_t *u8g2)", "渲染紧凑通知框。"),
])

h2("3.4 主循环集成（与菜单联动）")
code("while (1) {\n"
     "    anim_manager_update();\n"
     "    menu_update(&menu);\n"
     "    int8_t k = Key();\n"
     "\n"
     "    // 弹窗激活时消费按键（空闲时为空操作）\n"
     "    popup_num_update(&num_popup, k);\n"
     "    popup_bool_update(&bool_popup, k);\n"
     "    popup_toast_update(&toast);  // 不需要 key\n"
     "\n"
     "    // 任意弹窗打开时屏蔽菜单按键\n"
     "    if (!popup_num_active(&num_popup)\n"
     "        && !popup_bool_active(&bool_popup)\n"
     "        && !popup_toast_active(&toast)) {\n"
     "        if (k==1) menu_key_up(&menu);\n"
     "        else if (k==2) menu_key_down(&menu);\n"
     "        else if (k==3) menu_key_enter(&menu);\n"
     "        else if (k==4) menu_key_back(&menu);\n"
     "    }\n"
     "\n"
     "    u8g2_FirstPage(&u8g2); do {\n"
     "        menu_render(&u8g2, &menu);\n"
     "        popup_num_render(&num_popup, &u8g2);\n"
     "        popup_bool_render(&bool_popup, &u8g2);\n"
     "        popup_toast_render(&toast, &u8g2);\n"
     "    } while (u8g2_NextPage(&u8g2));\n"
     "}")

h2("3.5 Action 回调示例")
code("static int16_t     bright_val = 50;\n"
     "static popup_num_t bright_popup;\n"
     "\n"
     "static void bright_action(void) {\n"
     "    popup_num_open(&bright_popup, \"Brightness\",\n"
     "                   &bright_val, 0, 100, 5);\n"
     "}\n"
     "\n"
     "// 在 menu_item 中使用：\n"
     "{\"Brightness\", bright_action, NULL}")

story.append(PageBreak())

# ============================================================
# 4. 动画引擎
# ============================================================
h1("4. 动画引擎 (ux_move.h)")

body("轻量级、纯整数运算的动画引擎，用于坐标插值。最多支持 MAX_ANIM_NUM（默认 10）个并发动画，"
     "提供缓动函数、步骤序列、暂停/恢复以及反向播放功能。")

h2("4.1 anim_ctrl_t（动画控制块）")
code("typedef struct {\n"
     "    anim_state_t state;       // IDLE | PLAYING | PAUSED | FINISHED | BACKING\n"
     "    uint32_t  start_time;     // 起始 HAL_GetTick()\n"
     "    uint32_t  duration;       // 总时长 (ms)\n"
     "    int16_t   start_x, start_y, end_x, end_y;\n"
     "    int16_t   cur_x, cur_y;   // 当前插值坐标\n"
     "    void    (*easing)(...);    // 缓动函数指针\n"
     "    void    (*on_finish)(void *);  // 完成回调\n"
     "    // 序列支持：\n"
     "    const anim_step_t *steps;\n"
     "    uint8_t  step_count, current_step;\n"
     "    bool     loop;\n"
     "} anim_ctrl_t;")

h2("4.2 缓动函数")
body("所有缓动函数共享同一个签名：")
code("void easing(int32_t t,     // 已流逝时间 (ms)\n"
     "           int32_t b,     // 起始值\n"
     "           int32_t c,     // 总变化量 (end - start)\n"
     "           int32_t d,     // 总时长 (ms)\n"
     "           int32_t *out); // 输出：当前值")
body("<b>quad_ease_out</b> &mdash; 二次缓出（减速），默认缓动函数。<br/>"
     "<b>linear_ease</b> &mdash; 匀速线性插值。")

h2("4.3 API 参考")
api_table([
    ("void anim_init(anim_ctrl_t *a)", "重置所有字段；默认缓动为 quad_ease_out。"),
    ("void anim_set_position(anim_ctrl_t *a,<br/>&nbsp;&nbsp;int16_t x, int16_t y)",
     "将当前 / 起始 / 结束均设为 (x,y)，状态置为 IDLE。"),
    ("void anim_start(anim_ctrl_t *a, int16_t sx,<br/>&nbsp;&nbsp;int16_t sy, int16_t ex, int16_t ey,<br/>&nbsp;&nbsp;uint32_t ms, void *easing)",
     "从 (sx,sy) 动画至 (ex,ey)，时长 ms 毫秒。自动注册到全局管理器。"),
    ("void anim_start_step(anim_ctrl_t *a)", "从第 0 步开始播放步骤序列。"),
    ("void anim_pause(anim_ctrl_t *a)", "暂停；保存已流逝时间供恢复使用。"),
    ("void anim_resume(anim_ctrl_t *a)", "从暂停位置恢复。"),
    ("void anim_stop(anim_ctrl_t *a)", "强制进入 IDLE，从管理器注销。"),
    ("void anim_back(anim_ctrl_t *a)", "反向朝起始位置播放。"),
    ("void anim_manager_update(void)", "<b>必须在主循环中每帧调用。</b>"),
    ("bool anim_manager_is_idle(void)", "无已注册动画时返回 true。"),
    ("void anim_get_position(anim_ctrl_t *a,<br/>&nbsp;&nbsp;int16_t *x, int16_t *y)",
     "读取 cur_x / cur_y（内联函数）。"),
    ("bool anim_is_playing(anim_ctrl_t *a)", "state == ANIM_PLAYING 时返回 true（内联函数）。"),
])

h2("4.4 常见用法")
body("<b>仅对 Y 轴做动画（菜单滚动常用）：</b>")
code("anim_start(&a, 0, a.cur_y, 0, target_y, 350, quad_ease_out);")
body("<b>X 作为宽度、Y 作为位置（选择条）：</b>")
code("anim_start(&bar, old_w, old_y, new_w, new_y, 200, quad_ease_out);\n"
     "int16_t w = bar.cur_x;   // 当前宽度\n"
     "int16_t y = bar.cur_y;   // 当前 Y 坐标")

story.append(PageBreak())

# ============================================================
# 5. 按键输入
# ============================================================
h1("5. 按键输入 (Key.h)")

body("四个低电平有效的按钮，内置上拉。通过\"松手瞬间检测\"实现软件去抖动，"
     "无需定时器滤波。")

h2("5.1 引脚映射")
code("Key 1   PA0    (key1)     菜单上移\n"
     "Key 2   PA1    (key2)     菜单下移\n"
     "Key 3   PA2    (key3)     确认 / 进入\n"
     "Key 4   PC13   (user_key) 返回")

h2("5.2 API")
code("unsigned char Key(void);    // 读取并清除已锁存的按键 (1..4, 0=无按键)\n"
     "void Key_Tick(void);        // 约每 20 ms 调用一次 (在 SysTick ISR 中)")

h2("5.3 ISR 接线 (stm32f1xx_it.c)")
code("void SysTick_Handler(void) {\n"
     "    static uint8_t counter = 0;\n"
     "    if (++counter == 20) { Key_Tick(); counter = 0; }\n"
     "    HAL_IncTick();\n"
     "}")
body("SysTick 以 1 kHz 频率触发；计数器将其分频至 50 Hz (20 ms) 供按键扫描。"
     "HAL_IncTick() <b>必须</b>在 Key_Tick() 之后调用。")

story.append(PageBreak())

# ============================================================
# 6. 显示集成
# ============================================================
h1("6. 显示集成 (stm32_u8g2.h)")

body("u8g2 库配置为 SSD1306 128x64 通过硬件 I2C（地址 0x78）驱动。"
     "使用全帧缓冲模式 (_f 变体)，占用 1 KB RAM。")

h2("6.1 初始化")
code("// main.c 中：\n"
     "u8g2_t u8g2;\n"
     "HAL_TIM_Base_Start(&htim1);   // I2C 延迟所必需\n"
     "u8g2Init(&u8g2);              // 内部调用 u8g2_Setup_ssd1306_..._f\n"
     "\n"
     "// 每帧的渲染模式：\n"
     "u8g2_FirstPage(&u8g2);\n"
     "do {\n"
     "    // 在此处执行绘制调用\n"
     "} while (u8g2_NextPage(&u8g2));")

h2("6.2 SetFontMode")
body("<b>在使用 XOR 模式（颜色 2）之前，务必调用 u8g2_SetFontMode(u8g2, 1)。</b> "
     "透明模式 (1) 确保字形的背景像素不会被绘制。")

h2("6.3 本框架使用的字体")
bul("菜单文字：u8g2_font_helvB10_tr (Helvetica Bold 10 px，比例宽，透明底色)")
bul("标题栏：u8g2_font_6x10_tr (6x10 像素，等宽)")
bul("Toast：u8g2_font_6x10_tr")

h2("6.4 颜色模式")
code("SetDrawColor(u8g2, 0)   黑色  (清除像素)\n"
     "SetDrawColor(u8g2, 1)   白色  (设置像素)\n"
     "SetDrawColor(u8g2, 2)   XOR   (反转帧缓冲像素)")

body("选择条使用 XOR 模式：圆角矩形的选择框以颜色 2 绘制在已画好的菜单文字之上。"
     "框与白色文字重叠处，XOR 产生黑色（反色）；与空白处重叠，XOR 产生白色（框体本身）。")

story.append(PageBreak())

# ============================================================
# 7. 完整模板
# ============================================================
h1("7. 主循环完整模板")
code("int main(void) {\n"
     "    HAL_Init();\n"
     "    SystemClock_Config();\n"
     "    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);\n"
     "    MX_GPIO_Init(); MX_I2C1_Init(); MX_TIM1_Init();\n"
     "    HAL_TIM_Base_Start(&htim1);\n"
     "\n"
     "    u8g2_t u8g2;  u8g2Init(&u8g2);\n"
     "\n"
     "    // 连接父页面指针\n"
     "    settings_page.parent = &root_page;\n"
     "    // ...\n"
     "\n"
     "    // 初始化各子系统\n"
     "    static menu_state_t   menu;\n"
     "    static popup_num_t    pop_num;\n"
     "    static popup_bool_t   pop_bool;\n"
     "    static popup_toast_t  toast;\n"
     "    menu_init(&menu, &root_page);\n"
     "    popup_num_init(&pop_num);\n"
     "    popup_bool_init(&pop_bool);\n"
     "    popup_toast_init(&toast);\n"
     "\n"
     "    while (1) {\n"
     "        anim_manager_update();\n"
     "        menu_update(&menu);\n"
     "        int8_t k = Key();\n"
     "\n"
     "        popup_num_update(&pop_num, k);\n"
     "        popup_bool_update(&pop_bool, k);\n"
     "        popup_toast_update(&toast);\n"
     "\n"
     "        if (!popup_num_active(&pop_num)\n"
     "            && !popup_bool_active(&pop_bool)\n"
     "            && !popup_toast_active(&toast)) {\n"
     "            if (k==1) menu_key_up(&menu);\n"
     "            else if (k==2) menu_key_down(&menu);\n"
     "            else if (k==3) menu_key_enter(&menu);\n"
     "            else if (k==4) menu_key_back(&menu);\n"
     "        }\n"
     "\n"
     "        u8g2_FirstPage(&u8g2); do {\n"
     "            menu_render(&u8g2, &menu);\n"
     "            popup_num_render(&pop_num, &u8g2);\n"
     "            popup_bool_render(&pop_bool, &u8g2);\n"
     "            popup_toast_render(&toast, &u8g2);\n"
     "        } while (u8g2_NextPage(&u8g2));\n"
     "    }\n"
     "}")

story.append(Spacer(1, 5*mm))

# ============================================================
# 8. 扩展指南
# ============================================================
h1("8. 扩展指南")
body("<b>新增菜单页：</b>定义 menu_item_t[] 和 menu_page_t，在运行时连接 parent 指针，"
     "在父页面的 items 数组中新增一项并将 submenu 指向 &new_page。")
body("<b>新增 Action 回调：</b>编写 void fn(void) 函数，设置为某个 menu_item_t 的 action。"
     "在回调内使用 popup_num_open / popup_bool_open / popup_toast_show 弹出对话框。")
body("<b>新增弹窗类型：</b>参照 popup.h / popup.c 中的模式。定义 init / open / active / "
     "update / render 函数。在 main.c 中添加实例，并在主循环中调用其 _update 和 _render。")

story.append(Spacer(1, 10*mm))
story.append(Paragraph("— 文档结束 —", styles['Normal']))

# Build
doc.build(story)
print(f"PDF 已生成：{OUTPUT}")
