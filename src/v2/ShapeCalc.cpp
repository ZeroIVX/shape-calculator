#define _CRT_SECURE_NO_WARNINGS

// 抬高目标平台版本：EasyX 头文件默认按 WINVER 0x0400 裁剪，会导致 CLEARTYPE_QUALITY 等未定义
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <graphics.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <windows.h>
#include <math.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

/* ================================================================
 *  图形计算器
 *  平面图形 / 立体图形 的面积、周长、体积等常用数值计算
 *
 *  - 表驱动：每个图形对应一条 ShapeDef 记录，新增图形只需
 *    写一个计算函数并在 kShapes 表里加一行
 *  - 自绘界面：渐变背景、圆角卡片、悬停过渡、参数输入面板
 *  - 全键盘输入：点击图形后直接敲数字，回车计算
 * ================================================================ */

/* ---------------------------- 常量 ---------------------------- */
static const int   kWinW = 920;
static const int   kWinH = 620;
static const float kPi   = 3.14159265f;

/* 配色（浅色主题） */
static const COLORREF kBgTop      = RGB(247, 249, 253);
static const COLORREF kBgBottom   = RGB(229, 236, 247);
static const COLORREF kCard       = RGB(255, 255, 255);
static const COLORREF kShadow     = RGB(214, 222, 236);
static const COLORREF kBorder     = RGB(226, 232, 240);
static const COLORREF kPrimary    = RGB(47, 107, 255);
static const COLORREF kPrimaryTint= RGB(232, 240, 255);
static const COLORREF kInk        = RGB(31, 41, 55);
static const COLORREF kInkSub     = RGB(88, 98, 114);
static const COLORREF kInkMuted   = RGB(138, 152, 172);
static const COLORREF kDanger     = RGB(220, 38, 38);
static const COLORREF kValue      = RGB(29, 78, 216);
static const COLORREF kInputBg    = RGB(248, 250, 253);

/* 布局 */
static const int kGridX   = 32;		// 左侧按钮区起点
static const int kGridW   = 532;		// 左侧按钮区宽度
static const int kBtnW    = 170;		// 单个按钮宽
static const int kBtnH    = 52;		// 单个按钮高
static const int kColGap  = 11;		// 列间距
static const int kRowGap  = 10;		// 行间距

static const int kPanelX  = 584;		// 右侧面板左边界
static const int kPanelW  = 304;		// 右侧面板宽
static const int kPanelY  = 96;		// 右侧面板上边界
static const int kPanelH  = 492;		// 右侧面板高

static const int kPadIn   = 20;		// 面板内边距
static const int kRowH    = 48;		// 参数行高
static const int kInputH  = 36;		// 输入框高

/* 手绘不规则多边形 */
static const int  kCell     = 20;		// 网格边长（1 格 = 1 单位）
static const int  kCanvasW  = 264;		// 画布宽
static const int  kCanvasH  = 320;		// 画布高
static const RECT kCanvas   = { 604, 146, 604 + kCanvasW, 146 + kCanvasH };
static const int  kCellCols = kCanvasW / kCell;	// 横向格数
static const int  kCellRows = kCanvasH / kCell;	// 纵向格数
static const int  kMaxPts   = 64;		// 顶点上限

/* -------------------------- 数据结构 -------------------------- */

// 一个图形的完整定义
struct ShapeDef
{
	const TCHAR* name;				// 图形名称（按钮文字）
	int          group;				// 0 = 平面图形，1 = 立体图形
	int          paramCount;		// 参数个数
	const TCHAR* paramName[3];		// 参数名称
	int          resultCount;		// 结果个数
	const TCHAR* resultName[4];		// 结果名称
	bool       (*compute)(const float* in, float* out);	// 计算函数
};

// 界面按钮
struct Button
{
	RECT         rect;
	const TCHAR* text;
	float        hover;				// 0~1，用于悬停过渡
	float        target;
};

// 参数输入框
struct InputBoxState
{
	RECT  rect;
	TCHAR text[24];
	bool  bad;						// 校验未通过时标红
};

/* ------------------------ 面积 / 体积计算 ---------------------- */

// 矩形：长 a、宽 b
static bool calcRect(const float* in, float* out)
{
	float a = in[0], b = in[1];
	if (a <= 0.0f || b <= 0.0f) return false;
	out[0] = a * b;
	out[1] = 2.0f * (a + b);
	out[2] = sqrtf(a * a + b * b);
	return true;
}

// 正方形：边长 a
static bool calcSquare(const float* in, float* out)
{
	float a = in[0];
	if (a <= 0.0f) return false;
	out[0] = a * a;
	out[1] = 4.0f * a;
	out[2] = a * sqrtf(2.0f);
	return true;
}

// 三角形：三边 a、b、c（海伦公式）
static bool calcTriangle(const float* in, float* out)
{
	float a = in[0], b = in[1], c = in[2];
	if (a <= 0.0f || b <= 0.0f || c <= 0.0f) return false;
	if (a + b <= c || a + c <= b || b + c <= a) return false;	// 不满足三边关系

	float p = (a + b + c) / 2.0f;
	float s = sqrtf(p * (p - a) * (p - b) * (p - c));

	out[0] = s;						// 面积
	out[1] = a + b + c;				// 周长
	out[2] = s / p;					// 内切圆半径
	out[3] = a * b * c / (4.0f * s);	// 外接圆半径
	return true;
}

// 圆：半径 r
static bool calcCircle(const float* in, float* out)
{
	float r = in[0];
	if (r <= 0.0f) return false;
	out[0] = kPi * r * r;
	out[1] = 2.0f * kPi * r;
	out[2] = 2.0f * r;
	return true;
}

// 平行四边形：底边 a、高 h、邻边 b
static bool calcPara(const float* in, float* out)
{
	float a = in[0], h = in[1], b = in[2];
	if (a <= 0.0f || h <= 0.0f || b <= 0.0f) return false;
	out[0] = a * h;
	out[1] = 2.0f * (a + b);
	return true;
}

// 菱形：两条对角线 d1、d2
static bool calcRhombus(const float* in, float* out)
{
	float d1 = in[0], d2 = in[1];
	if (d1 <= 0.0f || d2 <= 0.0f) return false;

	float side = sqrtf(d1 * d1 / 4.0f + d2 * d2 / 4.0f);	// 边长
	out[0] = d1 * d2 / 2.0f;
	out[1] = side;
	out[2] = 4.0f * side;
	return true;
}

// 梯形：上底 a、下底 b、高 h
static bool calcTrapezoid(const float* in, float* out)
{
	float a = in[0], b = in[1], h = in[2];
	if (a <= 0.0f || b <= 0.0f || h <= 0.0f) return false;
	out[0] = (a + b) * h / 2.0f;
	out[1] = (a + b) / 2.0f;		// 中位线
	return true;
}

// 扇形：半径 r、圆心角 deg（角度制）
static bool calcSector(const float* in, float* out)
{
	float r = in[0], deg = in[1];
	if (r <= 0.0f || deg <= 0.0f || deg > 360.0f) return false;

	float arc = 2.0f * kPi * r * deg / 360.0f;
	out[0] = kPi * r * r * deg / 360.0f;	// 面积
	out[1] = arc;							// 弧长
	out[2] = arc + 2.0f * r;				// 周长
	return true;
}

// 椭圆：长半轴 a、短半轴 b（周长用拉马努金近似）
static bool calcEllipse(const float* in, float* out)
{
	float a = in[0], b = in[1];
	if (a <= 0.0f || b <= 0.0f) return false;

	out[0] = kPi * a * b;
	out[1] = kPi * (3.0f * (a + b) - sqrtf((3.0f * a + b) * (a + 3.0f * b)));
	return true;
}

// 长方体：长 a、宽 b、高 h
static bool calcCuboid(const float* in, float* out)
{
	float a = in[0], b = in[1], h = in[2];
	if (a <= 0.0f || b <= 0.0f || h <= 0.0f) return false;
	out[0] = a * b * h;
	out[1] = 2.0f * (a * b + b * h + a * h);
	out[2] = sqrtf(a * a + b * b + h * h);
	return true;
}

// 正方体：棱长 a
static bool calcCube(const float* in, float* out)
{
	float a = in[0];
	if (a <= 0.0f) return false;
	out[0] = a * a * a;
	out[1] = 6.0f * a * a;
	out[2] = a * sqrtf(3.0f);
	return true;
}

// 圆柱：底半径 r、高 h
static bool calcCylinder(const float* in, float* out)
{
	float r = in[0], h = in[1];
	if (r <= 0.0f || h <= 0.0f) return false;
	out[0] = kPi * r * r * h;
	out[1] = 2.0f * kPi * r * h;
	out[2] = 2.0f * kPi * r * (r + h);
	return true;
}

// 圆锥：底半径 r、高 h
static bool calcCone(const float* in, float* out)
{
	float r = in[0], h = in[1];
	if (r <= 0.0f || h <= 0.0f) return false;

	float l = sqrtf(r * r + h * h);		// 母线
	out[0] = kPi * r * r * h / 3.0f;
	out[1] = kPi * r * l;
	out[2] = kPi * r * (r + l);
	out[3] = l;
	return true;
}

// 球：半径 r
static bool calcSphere(const float* in, float* out)
{
	float r = in[0];
	if (r <= 0.0f) return false;
	out[0] = 4.0f / 3.0f * kPi * r * r * r;
	out[1] = 4.0f * kPi * r * r;
	return true;
}

// 圆台：上底半径 r1、下底半径 r2、高 h
static bool calcFrustum(const float* in, float* out)
{
	float r1 = in[0], r2 = in[1], h = in[2];
	if (r1 <= 0.0f || r2 <= 0.0f || h <= 0.0f) return false;

	float l = sqrtf((r2 - r1) * (r2 - r1) + h * h);		// 母线
	out[0] = kPi * h * (r1 * r1 + r1 * r2 + r2 * r2) / 3.0f;
	out[1] = kPi * (r1 + r2) * l;
	out[2] = out[1] + kPi * r1 * r1 + kPi * r2 * r2;
	out[3] = l;
	return true;
}

/* --------------------------- 图形表 ---------------------------- */
static const ShapeDef kShapes[] =
{
	// 平面图形
	{ _T("矩形"),		0, 2, { _T("长 a"), _T("宽 b") },		3, { _T("面积 S"), _T("周长 C"), _T("对角线 d") },				calcRect },
	{ _T("正方形"),		0, 1, { _T("棱长 a") },					3, { _T("面积 S"), _T("周长 C"), _T("对角线 d") },				calcSquare },
	{ _T("三角形"),		0, 3, { _T("边 a"), _T("边 b"), _T("边 c") },	4, { _T("面积 S"), _T("周长 C"), _T("内切圆 r内"), _T("外接圆 R外") },	calcTriangle },
	{ _T("圆形"),		0, 1, { _T("半径 r") },					3, { _T("面积 S"), _T("周长 C"), _T("直径 d") },					calcCircle },
	{ _T("平行四边形"),	0, 3, { _T("底边 a"), _T("高 h"), _T("邻边 b") },	2, { _T("面积 S"), _T("周长 C") },						calcPara },
	{ _T("菱形"),		0, 2, { _T("对角线 d1"), _T("对角线 d2") },	3, { _T("面积 S"), _T("边长 a"), _T("周长 C") },					calcRhombus },
	{ _T("梯形"),		0, 3, { _T("上底 a"), _T("下底 b"), _T("高 h") },	2, { _T("面积 S"), _T("中位线 m") },						calcTrapezoid },
	{ _T("扇形"),		0, 2, { _T("半径 r"), _T("圆心角 θ") },		3, { _T("面积 S"), _T("弧长 L"), _T("周长 C") },					calcSector },
	{ _T("椭圆"),		0, 2, { _T("长半轴 a"), _T("短半轴 b") },	2, { _T("面积 S"), _T("周长 C 近似") },						calcEllipse },

	// 立体图形
	{ _T("长方体"),		1, 3, { _T("长 a"), _T("宽 b"), _T("高 h") },	3, { _T("体积 V"), _T("表面积 S总"), _T("对角线 d") },			calcCuboid },
	{ _T("正方体"),		1, 1, { _T("棱长 a") },					3, { _T("体积 V"), _T("表面积 S总"), _T("对角线 d") },			calcCube },
	{ _T("圆柱"),		1, 2, { _T("底半径 r"), _T("高 h") },		3, { _T("体积 V"), _T("侧面积 S侧"), _T("表面积 S总") },			calcCylinder },
	{ _T("圆锥"),		1, 2, { _T("底半径 r"), _T("高 h") },		4, { _T("体积 V"), _T("侧面积 S侧"), _T("表面积 S总"), _T("母线 l") },	calcCone },
	{ _T("球"),			1, 1, { _T("半径 r") },					2, { _T("体积 V"), _T("表面积 S总") },							calcSphere },
	{ _T("圆台"),		1, 3, { _T("上底 r1"), _T("下底 r2"), _T("高 h") },	4, { _T("体积 V"), _T("侧面积 S侧"), _T("表面积 S总"), _T("母线 l") },	calcFrustum },
};
static const int kShapeCount = sizeof(kShapes) / sizeof(kShapes[0]);

/* --------------------------- 运行状态 -------------------------- */
static Button        g_shapeBtns[kShapeCount];
static Button        g_calcBtn;
static Button        g_actionBtns[4];		// 底部：清空 / 自由多边形 / 复制结果 / 退出

static int           g_selected   = -1;		// 当前图形索引
static int           g_paramCount = 0;		// 当前参数个数
static InputBoxState g_inputs[3];			// 参数输入框
static int           g_focus      = -1;		// 聚焦的输入框

static float         g_results[4];			// 计算结果
static int           g_resultCount = 0;
static bool          g_hasResult   = false;
static TCHAR         g_message[128] = { 0 };	// 错误 / 提示信息

/* 手绘不规则多边形状态 */
static bool          g_drawMode = false;	// 是否处于手绘模式
static POINT         g_pts[kMaxPts];		// 顶点（客户区坐标，已吸附到网格）
static int           g_ptCount  = 0;
static bool          g_closed   = false;	// 图形是否已闭合
static POINT         g_hoverPt  = { 0, 0 };	// 鼠标吸附后的位置
static bool          g_hoverIn  = false;	// 鼠标是否在画布内

/* --------------------------- 小工具 ---------------------------- */

static COLORREF mix(COLORREF a, COLORREF b, float t)
{
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;

	int ar = GetRValue(a), ag = GetGValue(a), ab = GetBValue(a);
	int br = GetRValue(b), bg = GetGValue(b), bb = GetBValue(b);
	return RGB(ar + (int)((br - ar) * t), ag + (int)((bg - ag) * t), ab + (int)((bb - ab) * t));
}

// 设置字体：px 为字符高度，bold 为粗体
// 字体名必须用英文 "Microsoft YaHei"：中文名在 GDI 下匹配不到，会回退成宋体位图，小字会发虚
// 用 CLEARTYPE 质量，笔画边缘有亚像素过渡，比 ANTIALIASED 更锐利
static void set_font(int px, bool bold)
{
	LOGFONT f;
	memset(&f, 0, sizeof(f));
	_tcscpy_s(f.lfFaceName, _T("Microsoft YaHei"));
	f.lfHeight  = -px;
	f.lfWidth   = 0;
	f.lfWeight  = bold ? FW_BOLD : FW_NORMAL;
	f.lfCharSet = DEFAULT_CHARSET;
	f.lfQuality = CLEARTYPE_QUALITY;
	settextstyle(&f);
}

// 文字水平居中、垂直居中
static void text_center(int cx, int cy, const TCHAR* s, COLORREF color)
{
	int w = textwidth(s);
	int h = textheight(s);
	settextcolor(color);
	outtextxy(cx - w / 2, cy - h / 2, s);
}

// 文字右对齐（x 为右边界，cy 为垂直中心）
static void text_right(int x, int cy, const TCHAR* s, COLORREF color)
{
	int w = textwidth(s);
	int h = textheight(s);
	settextcolor(color);
	outtextxy(x - w, cy - h / 2, s);
}

// 文字左对齐
static void text_left(int x, int cy, const TCHAR* s, COLORREF color)
{
	int h = textheight(s);
	settextcolor(color);
	outtextxy(x, cy - h / 2, s);
}

// 数值格式化：常规值保留 4 位小数，极大或极小时用科学计数法
static void format_value(float v, TCHAR* buf, int size)
{
	float av = fabsf(v);
	if (av != 0.0f && (av >= 1.0e7f || av < 1.0e-4f))
		_stprintf_s(buf, size, _T("%.4e"), v);
	else
		_stprintf_s(buf, size, _T("%.4f"), v);
}

/* ---------------------- 手绘多边形几何计算 ---------------------- */

// 客户区横坐标 -> 数学坐标（画布左下角为原点）
static float to_ux(int x)
{
	return (x - kCanvas.left) / (float)kCell;
}

// 客户区纵坐标 -> 数学坐标（向上为正）
static float to_uy(int y)
{
	return (kCanvas.bottom - y) / (float)kCell;
}

// 把鼠标位置吸附到最近的网格交点
static POINT snap_point(int x, int y)
{
	int cx = (int)floorf((x - kCanvas.left) / (float)kCell + 0.5f);
	int cy = (int)floorf((kCanvas.bottom - y) / (float)kCell + 0.5f);
	if (cx < 0) cx = 0;
	if (cx > kCellCols) cx = kCellCols;
	if (cy < 0) cy = 0;
	if (cy > kCellRows) cy = kCellRows;

	POINT p = { kCanvas.left + cx * kCell, kCanvas.bottom - cy * kCell };
	return p;
}

// 鞋带公式：任意多边形面积（顶点顺序即绘制顺序，自动闭合）
static float polygon_area()
{
	if (g_ptCount < 3) return 0.0f;

	float sum = 0.0f;
	for (int i = 0; i < g_ptCount; ++i)
	{
		int j = (i + 1) % g_ptCount;
		sum += to_ux(g_pts[i].x) * to_uy(g_pts[j].y)
			 - to_ux(g_pts[j].x) * to_uy(g_pts[i].y);
	}
	return fabsf(sum) / 2.0f;
}

// 多边形周长：已闭合时含收尾边，未闭合时按折线累计
static float polygon_perimeter()
{
	if (g_ptCount < 2) return 0.0f;

	int seg = g_closed ? g_ptCount : g_ptCount - 1;
	float sum = 0.0f;
	for (int i = 0; i < seg; ++i)
	{
		int j = (i + 1) % g_ptCount;
		float dx = to_ux(g_pts[j].x) - to_ux(g_pts[i].x);
		float dy = to_uy(g_pts[j].y) - to_uy(g_pts[i].y);
		sum += sqrtf(dx * dx + dy * dy);
	}
	return sum;
}

/* --------------------------- 界面布局 -------------------------- */

static void layout()
{
	// 平面图形：3 列 3 行
	for (int i = 0; i < 9; ++i)
	{
		int col = i % 3, row = i / 3;
		int x = kGridX + col * (kBtnW + kColGap);
		int y = 118 + row * (kBtnH + kRowGap);
		RECT r = { x, y, x + kBtnW, y + kBtnH };
		g_shapeBtns[i].rect   = r;
		g_shapeBtns[i].text   = kShapes[i].name;
		g_shapeBtns[i].hover  = 0.0f;
		g_shapeBtns[i].target = 0.0f;
	}

	// 立体图形：3 列 2 行
	for (int i = 9; i < kShapeCount; ++i)
	{
		int col = (i - 9) % 3, row = (i - 9) / 3;
		int x = kGridX + col * (kBtnW + kColGap);
		int y = 332 + row * (kBtnH + kRowGap);
		RECT r = { x, y, x + kBtnW, y + kBtnH };
		g_shapeBtns[i].rect   = r;
		g_shapeBtns[i].text   = kShapes[i].name;
		g_shapeBtns[i].hover  = 0.0f;
		g_shapeBtns[i].target = 0.0f;
	}

	// 底部功能按钮：清空 / 自由多边形 / 复制结果 / 退出
	const int abw = (kGridW - 3 * kColGap) / 4;
	const TCHAR* actext[4] = { _T("清空"), _T("自由多边形"), _T("复制结果"), _T("退出") };
	for (int i = 0; i < 4; ++i)
	{
		int x = kGridX + i * (abw + kColGap);
		RECT r = { x, 470, x + abw, 518 };
		g_actionBtns[i].rect   = r;
		g_actionBtns[i].text   = actext[i];
		g_actionBtns[i].hover  = 0.0f;
		g_actionBtns[i].target = 0.0f;
	}

	// 面板内的计算按钮
	RECT rb = { kPanelX + kPadIn, 316, kPanelX + kPanelW - kPadIn, 356 };
	g_calcBtn.rect = rb;   g_calcBtn.text = _T("计  算");    g_calcBtn.hover = 0; g_calcBtn.target = 0;

	// 参数输入框位置
	for (int i = 0; i < 3; ++i)
	{
		int top = 168 + i * kRowH;
		RECT ri = { kPanelX + 116, top, kPanelX + kPanelW - kPadIn, top + kInputH };
		g_inputs[i].rect = ri;
		g_inputs[i].text[0] = 0;
		g_inputs[i].bad = false;
	}
}

/* --------------------------- 绘制部分 -------------------------- */

static void draw_background()
{
	for (int y = 0; y < kWinH; y += 4)
	{
		float t = (float)y / (float)kWinH;
		setfillcolor(mix(kBgTop, kBgBottom, t));
		solidrectangle(0, y, kWinW, y + 4);
	}

	// 右上角淡色装饰圆
	setfillcolor(RGB(232, 239, 251));
	solidcircle(806, 74, 86);
	setfillcolor(RGB(238, 244, 253));
	solidcircle(878, 138, 54);
}

static void draw_header()
{
	set_font(30, true);
	text_left(32, 30, _T("图形计算器"), kPrimary);

	set_font(14, false);
	text_left(32, 66, _T("平面图形 / 立体图形 · 面积 · 周长 · 体积 · 对角线"), kInkSub);

	// 标题左侧装饰竖条
	setfillcolor(kPrimary);
	solidroundrect(20, 14, 25, 46, 4, 4);
}

// 普通按钮（白底 + 悬停渐染 + 阴影）
static void draw_common_button(const Button& b, bool selected)
{
	const RECT& r = b.rect;

	setfillcolor(kShadow);
	solidroundrect(r.left + 1, r.top + 2, r.right + 1, r.bottom + 3, 12, 12);

	COLORREF fill = selected ? kPrimary : mix(kCard, kPrimaryTint, b.hover);
	COLORREF line = selected ? kPrimary : mix(kBorder, kPrimary, b.hover * 0.85f);
	COLORREF ink  = selected ? RGB(255, 255, 255) : mix(kInk, kPrimary, b.hover);

	setfillcolor(fill);
	setlinecolor(line);
	setlinestyle(PS_SOLID, selected ? 2 : 1);
	fillroundrect(r.left, r.top, r.right, r.bottom, 12, 12);

	set_font(16, selected);
	RECT tmp = r;
	settextcolor(ink);
	drawtext(b.text, &tmp, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// 主色按钮（用于“计算”）
static void draw_primary_button(const Button& b)
{
	const RECT& r = b.rect;

	setfillcolor(mix(kShadow, kPrimary, 0.30f));
	solidroundrect(r.left + 1, r.top + 2, r.right + 1, r.bottom + 3, 10, 10);

	setfillcolor(mix(kPrimary, RGB(23, 74, 200), b.hover));
	setlinecolor(mix(kPrimary, RGB(23, 74, 200), b.hover));
	setlinestyle(PS_SOLID, 1);
	fillroundrect(r.left, r.top, r.right, r.bottom, 10, 10);

	set_font(17, true);
	RECT tmp = r;
	settextcolor(RGB(255, 255, 255));
	drawtext(b.text, &tmp, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// 参数输入框
static void draw_input(int index)
{
	const InputBoxState& ib = g_inputs[index];
	const RECT& r = ib.rect;
	bool focused = (g_focus == index);

	COLORREF line = ib.bad ? kDanger : (focused ? kPrimary : kBorder);
	setfillcolor(ib.bad ? RGB(254, 242, 242) : (focused ? RGB(250, 252, 255) : kInputBg));
	setlinecolor(line);
	setlinestyle(PS_SOLID, focused || ib.bad ? 2 : 1);
	fillroundrect(r.left, r.top, r.right, r.bottom, 8, 8);

	set_font(16, false);
	if (ib.text[0] == 0)
	{
		text_left(r.left + 12, (r.top + r.bottom) / 2, _T("0"), kInkMuted);
	}
	else
	{
		text_left(r.left + 12, (r.top + r.bottom) / 2, ib.text, kInk);
	}
}

// 手绘模式的面板：网格画布 + 实时面积 / 周长
static void draw_canvas_panel()
{
	int left  = kPanelX + kPadIn;
	int right = kPanelX + kPanelW - kPadIn;

	set_font(22, true);
	text_left(left, 116, _T("自由多边形"), kInk);
	set_font(13, false);
	text_right(right, 116, _T("手绘 · 1 格 = 1 单位"), kInkSub);

	// 画布底
	setfillcolor(RGB(252, 253, 255));
	setlinecolor(RGB(224, 232, 244));
	setlinestyle(PS_SOLID, 1);
	fillroundrect(kCanvas.left, kCanvas.top, kCanvas.right, kCanvas.bottom, 8, 8);

	// 网格线
	setlinecolor(RGB(234, 240, 250));
	for (int x = kCanvas.left; x <= kCanvas.right; x += kCell)
		line(x, kCanvas.top, x, kCanvas.bottom);
	for (int y = kCanvas.top; y <= kCanvas.bottom; y += kCell)
		line(kCanvas.left, y, kCanvas.right, y);

	// 网格交点，提示可落点位置
	setfillcolor(RGB(212, 222, 238));
	for (int cx = 0; cx <= kCellCols; ++cx)
		for (int cy = 0; cy <= kCellRows; ++cy)
			solidcircle(kCanvas.left + cx * kCell, kCanvas.bottom - cy * kCell, 1);

	// 坐标轴（左边界与底边）
	setlinecolor(RGB(196, 209, 228));
	line(kCanvas.left, kCanvas.top, kCanvas.left, kCanvas.bottom);
	line(kCanvas.left, kCanvas.bottom, kCanvas.right, kCanvas.bottom);

	// 已画图形
	if (g_ptCount >= 3 && g_closed)
	{
		setfillcolor(RGB(214, 228, 252));
		setlinecolor(kPrimary);
		setlinestyle(PS_SOLID, 2);
		fillpolygon(g_pts, g_ptCount);
	}
	else if (g_ptCount >= 2)
	{
		setlinecolor(kPrimary);
		setlinestyle(PS_SOLID, 2);
		for (int i = 0; i < g_ptCount - 1; ++i)
			line(g_pts[i].x, g_pts[i].y, g_pts[i + 1].x, g_pts[i + 1].y);
	}

	// 最后一个顶点到光标的引导线
	if (!g_closed && g_ptCount > 0 && g_hoverIn)
	{
		setlinecolor(RGB(158, 187, 240));
		setlinestyle(PS_SOLID, 1);
		line(g_pts[g_ptCount - 1].x, g_pts[g_ptCount - 1].y, g_hoverPt.x, g_hoverPt.y);
	}

	// 顶点：起点橙色，其余主色
	for (int i = 0; i < g_ptCount; ++i)
	{
		setfillcolor(i == 0 ? RGB(249, 146, 62) : kPrimary);
		solidcircle(g_pts[i].x, g_pts[i].y, 4);
	}

	// 结果
	TCHAR buf[32];
	set_font(14, false);
	text_left(left, 496, _T("面积 S"), kInkSub);
	if (g_ptCount >= 3) format_value(polygon_area(), buf, 32);
	else _tcscpy_s(buf, _T("—"));
	set_font(20, true);
	text_right(right, 496, buf, kValue);

	set_font(14, false);
	text_left(left, 528, _T("周长 C"), kInkSub);
	if (g_ptCount >= 2) format_value(polygon_perimeter(), buf, 32);
	else _tcscpy_s(buf, _T("—"));
	set_font(20, true);
	text_right(right, 528, buf, kValue);

	// 操作提示
	const TCHAR* tip;
	if (g_ptCount == 0)       tip = _T("在网格上点击，依次落点");
	else if (g_ptCount < 3)   tip = _T("继续点击，至少需要 3 个顶点");
	else if (!g_closed)       tip = _T("点击橙色起点闭合图形");
	else                      tip = _T("右键撤销，点“清空”重画");

	set_font(12, false);
	text_center(kPanelX + kPanelW / 2, 562,
		g_message[0] != 0 ? g_message : tip,
		g_message[0] != 0 ? kPrimary : kInkMuted);
}

// 右侧面板
static void draw_panel()
{
	// 卡片阴影 + 卡片
	setfillcolor(kShadow);
	solidroundrect(kPanelX + 2, kPanelY + 4, kPanelX + kPanelW + 2, kPanelY + kPanelH + 4, 16, 16);
	setfillcolor(kCard);
	solidroundrect(kPanelX, kPanelY, kPanelX + kPanelW, kPanelY + kPanelH, 16, 16);

	// 手绘模式：右侧改为网格画布
	if (g_drawMode)
	{
		draw_canvas_panel();
		return;
	}

	if (g_selected < 0)
	{
		set_font(15, false);
		text_center(kPanelX + kPanelW / 2, kPanelY + kPanelH / 2 - 30, _T("请先选择左侧的图形"), kInkMuted);
		set_font(13, false);
		text_center(kPanelX + kPanelW / 2, kPanelY + kPanelH / 2 - 4, _T("点击后可直接键盘输入参数"), kInkMuted);

		// 装饰用的淡色几何轮廓
		setlinecolor(RGB(229, 236, 247));
		setlinestyle(PS_SOLID, 2);
		rectangle(662, 400, 702, 440);
		line(716, 440, 732, 400);
		line(732, 400, 748, 440);
		line(748, 440, 716, 440);
		circle(776, 420, 20);
		return;
	}

	const ShapeDef& s = kShapes[g_selected];
	int left = kPanelX + kPadIn;
	int right = kPanelX + kPanelW - kPadIn;

	// 图形名称
	set_font(22, true);
	text_left(left, 116, s.name, kInk);

	// 分组标签
	set_font(13, false);
	text_right(right, 116, s.group == 0 ? _T("平面图形") : _T("立体图形"), kInkSub);

	// 参数输入行
	set_font(15, false);
	for (int i = 0; i < g_paramCount; ++i)
	{
		int cy = (g_inputs[i].rect.top + g_inputs[i].rect.bottom) / 2;
		text_left(left, cy, s.paramName[i], kInkSub);
		draw_input(i);
	}

	// 计算按钮
	draw_primary_button(g_calcBtn);

	// 分隔线
	setlinecolor(kBorder);
	setlinestyle(PS_SOLID, 1);
	line(left, 372, right, 372);

	// 结果标题
	set_font(13, true);
	text_left(left, 386, _T("计算结果"), kInkSub);

	if (g_message[0] != 0 && !g_hasResult)
	{
		// 参数错误提示
		RECT tmp = { left, 408, right, 452 };
		set_font(14, false);
		settextcolor(kDanger);
		drawtext(g_message, &tmp, DT_LEFT | DT_TOP | DT_WORDBREAK);
		return;
	}

	if (!g_hasResult)
	{
		set_font(14, false);
		text_left(left, 420, _T("输入参数后按回车或点击“计算”"), kInkMuted);
		return;
	}

	// 结果列表
	for (int i = 0; i < g_resultCount; ++i)
	{
		int cy = 420 + i * 38;
		TCHAR buf[32];

		set_font(14, false);
		text_left(left, cy, s.resultName[i], kInkSub);

		format_value(g_results[i], buf, 32);
		set_font(20, true);
		text_right(right, cy, buf, kValue);
	}

	// 结果下方的辅助提示（如复制成功）
	if (g_message[0] != 0)
	{
		set_font(13, false);
		text_left(left, 556, g_message, kPrimary);
	}
}

static void draw_frame()
{
	setbkmode(TRANSPARENT);
	draw_background();
	draw_header();

	// 分组标题
	set_font(14, true);
	text_left(kGridX + 10, 100, _T("平面图形"), kInkSub);
	text_left(kGridX + 10, 314, _T("立体图形"), kInkSub);

	setfillcolor(kPrimary);
	solidroundrect(kGridX, 92, kGridX + 4, 108, 4, 4);
	solidroundrect(kGridX, 306, kGridX + 4, 322, 4, 4);

	// 图形按钮
	for (int i = 0; i < kShapeCount; ++i)
		draw_common_button(g_shapeBtns[i], i == g_selected);

	// 分隔线：区分图形按钮与操作按钮
	setlinecolor(RGB(206, 216, 232));
	setlinestyle(PS_SOLID, 1);
	line(kGridX, 458, kGridX + kGridW, 458);

	// 功能按钮（“自由多边形”在手绘模式下高亮）
	for (int i = 0; i < 4; ++i)
		draw_common_button(g_actionBtns[i], i == 1 && g_drawMode);

	draw_panel();
}

/* --------------------------- 状态操作 -------------------------- */

static void clear_result()
{
	g_hasResult = false;
	g_resultCount = 0;
	g_message[0] = 0;
	for (int i = 0; i < 3; ++i)
		g_inputs[i].bad = false;
}

// 进入手绘模式：右侧面板切换为网格画布
static void enter_draw_mode()
{
	g_drawMode   = true;
	g_selected   = -1;
	g_paramCount = 0;
	g_focus      = -1;
	clear_result();
}

static void select_shape(int index)
{
	g_drawMode = false;			// 选规则图形时退出手绘模式
	g_selected = index;
	g_paramCount = kShapes[index].paramCount;

	for (int i = 0; i < 3; ++i)
	{
		g_inputs[i].text[0] = 0;
		g_inputs[i].bad = false;
	}
	g_focus = 0;			// 自动聚焦到第一个参数，可直接输入
	clear_result();
}

static void clear_all()
{
	if (g_drawMode)				// 手绘模式：清空全部顶点
	{
		g_ptCount = 0;
		g_closed  = false;
		return;
	}

	for (int i = 0; i < 3; ++i)
	{
		g_inputs[i].text[0] = 0;
		g_inputs[i].bad = false;
	}
	g_focus = (g_selected >= 0) ? 0 : -1;
	clear_result();
}

// 执行计算
static void do_compute()
{
	clear_result();
	if (g_selected < 0)
		return;

	const ShapeDef& s = kShapes[g_selected];
	float in[3] = { 0.0f, 0.0f, 0.0f };

	for (int i = 0; i < s.paramCount; ++i)
	{
		if (g_inputs[i].text[0] == 0)
		{
			g_inputs[i].bad = true;
			_stprintf_s(g_message, 128, _T("请填写参数：%s"), s.paramName[i]);
			return;
		}
		in[i] = (float)atof(g_inputs[i].text);
	}

	float out[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	if (!s.compute(in, out))
	{
		for (int i = 0; i < s.paramCount; ++i)
			g_inputs[i].bad = true;
		_tcscpy_s(g_message, _T("参数无法构成该图形，请检查是否为正数且满足几何条件"));
		return;
	}

	for (int i = 0; i < s.resultCount; ++i)
		g_results[i] = out[i];
	g_resultCount = s.resultCount;
	g_hasResult = true;
}

// 把文本写入剪贴板，成功返回 true
static bool copy_text(const TCHAR* text)
{
	HWND hwnd = GetHWnd();
	if (!OpenClipboard(hwnd))
		return false;

	EmptyClipboard();
	bool ok = false;
	int bytes = (int)((_tcslen(text) + 1) * sizeof(TCHAR));
	HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, bytes);
	if (h)
	{
		void* dst = GlobalLock(h);
		if (dst)
		{
			memcpy(dst, text, bytes);
			GlobalUnlock(h);
			SetClipboardData(CF_TEXT, h);
			ok = true;
		}
	}
	CloseClipboard();
	return ok;
}

// 复制结果到剪贴板
static void do_copy()
{
	// 手绘模式：复制多边形结果
	if (g_drawMode)
	{
		if (g_ptCount < 3)
		{
			_tcscpy_s(g_message, _T("至少需要 3 个顶点"));
			return;
		}

		TCHAR n1[32], n2[32], pb[256];
		format_value(polygon_area(), n1, 32);
		format_value(polygon_perimeter(), n2, 32);
		_stprintf_s(pb, 256, _T("不规则多边形  %d 个顶点\r\n面积 S = %s\r\n周长 C = %s\r\n"),
			g_ptCount, n1, n2);
		if (copy_text(pb))
			_tcscpy_s(g_message, _T("结果已复制到剪贴板"));
		return;
	}

	if (!g_hasResult || g_selected < 0)
	{
		_tcscpy_s(g_message, _T("暂无可复制的计算结果"));
		return;
	}

	const ShapeDef& s = kShapes[g_selected];
	TCHAR buf[512] = { 0 };
	TCHAR num[32];

	_stprintf_s(buf, 512, _T("%s"), s.name);
	for (int i = 0; i < g_paramCount; ++i)
	{
		format_value((float)atof(g_inputs[i].text), num, 32);
		_tcscat_s(buf, _T("  "));
		_tcscat_s(buf, s.paramName[i]);
		_tcscat_s(buf, _T("="));
		_tcscat_s(buf, num);
	}
	_tcscat_s(buf, _T("\r\n"));
	for (int i = 0; i < g_resultCount; ++i)
	{
		format_value(g_results[i], num, 32);
		_tcscat_s(buf, s.resultName[i]);
		_tcscat_s(buf, _T(" = "));
		_tcscat_s(buf, num);
		_tcscat_s(buf, _T("\r\n"));
	}

	if (copy_text(buf))
		_tcscpy_s(g_message, _T("结果已复制到剪贴板"));
	else
		_tcscpy_s(g_message, _T("剪贴板打开失败"));
}

/* --------------------------- 事件处理 -------------------------- */

static bool in_rect(const RECT& r, int x, int y)
{
	return x >= r.left && x <= r.right && y >= r.top && y <= r.bottom;
}

// 鼠标移动：更新按钮悬停状态
static bool on_move(int x, int y)
{
	bool changed = false;

	for (int i = 0; i < kShapeCount; ++i)
	{
		float t = in_rect(g_shapeBtns[i].rect, x, y) ? 1.0f : 0.0f;
		if (g_shapeBtns[i].target != t) { g_shapeBtns[i].target = t; changed = true; }
	}
	for (int i = 0; i < 4; ++i)
	{
		float t = in_rect(g_actionBtns[i].rect, x, y) ? 1.0f : 0.0f;
		if (g_actionBtns[i].target != t) { g_actionBtns[i].target = t; changed = true; }
	}

	float tc = in_rect(g_calcBtn.rect, x, y) ? 1.0f : 0.0f;
	if (g_calcBtn.target != tc) { g_calcBtn.target = tc; changed = true; }

	// 手绘模式：跟踪画布内的鼠标位置，用于绘制引导线
	if (g_drawMode)
	{
		bool  inside = in_rect(kCanvas, x, y);
		POINT p      = inside ? snap_point(x, y) : g_hoverPt;
		if (inside != g_hoverIn || (inside && (p.x != g_hoverPt.x || p.y != g_hoverPt.y)))
		{
			g_hoverPt = p;
			g_hoverIn = inside;
			changed   = true;
		}
	}

	return changed;
}

// 悬停动画推进，返回是否仍在动画中
static bool update_animation()
{
	bool busy = false;
	for (int i = 0; i < kShapeCount; ++i)
	{
		Button& b = g_shapeBtns[i];
		float d = b.target - b.hover;
		if (fabsf(d) > 0.02f) { b.hover += d * 0.35f; busy = true; }
		else b.hover = b.target;
	}

	for (int i = 0; i < 4; ++i)
	{
		Button& b = g_actionBtns[i];
		float d = b.target - b.hover;
		if (fabsf(d) > 0.02f) { b.hover += d * 0.35f; busy = true; }
		else b.hover = b.target;
	}

	{
		Button& b = g_calcBtn;
		float d = b.target - b.hover;
		if (fabsf(d) > 0.02f) { b.hover += d * 0.35f; busy = true; }
		else b.hover = b.target;
	}

	return busy;
}

// 鼠标点击，返回 false 表示退出程序
static bool on_click(int x, int y)
{
	// 手绘模式：在画布内落点或闭合
	if (g_drawMode && in_rect(kCanvas, x, y))
	{
		if (!g_closed)
		{
			POINT p = snap_point(x, y);

			// 点在橙色起点附近 -> 闭合图形
			if (g_ptCount >= 3)
			{
				int dx = p.x - g_pts[0].x;
				int dy = p.y - g_pts[0].y;
				if (dx * dx + dy * dy <= (kCell / 2) * (kCell / 2))
				{
					g_closed = true;
					return true;
				}
			}

			// 与上一个顶点重合则忽略
			if (g_ptCount > 0 && p.x == g_pts[g_ptCount - 1].x && p.y == g_pts[g_ptCount - 1].y)
				return true;

			if (g_ptCount < kMaxPts)
			{
				g_pts[g_ptCount] = p;
				++g_ptCount;
			}
		}
		return true;
	}

	// 输入框
	for (int i = 0; i < g_paramCount; ++i)
	{
		if (in_rect(g_inputs[i].rect, x, y)) { g_focus = i; return true; }
	}

	// 计算按钮
	if (in_rect(g_calcBtn.rect, x, y)) { do_compute(); return true; }

	// 功能按钮
	if (in_rect(g_actionBtns[0].rect, x, y)) { clear_all();       return true; }
	if (in_rect(g_actionBtns[1].rect, x, y)) { enter_draw_mode(); return true; }
	if (in_rect(g_actionBtns[2].rect, x, y)) { do_copy();         return true; }
	if (in_rect(g_actionBtns[3].rect, x, y)) return false;

	// 图形按钮
	for (int i = 0; i < kShapeCount; ++i)
	{
		if (in_rect(g_shapeBtns[i].rect, x, y)) { select_shape(i); return true; }
	}

	g_focus = -1;
	return true;
}

// 右键撤销：先解除闭合，再逐个删除顶点
static void on_rclick(int x, int y)
{
	if (!g_drawMode || !in_rect(kCanvas, x, y))
		return;

	if (g_closed)
		g_closed = false;
	else if (g_ptCount > 0)
		--g_ptCount;
}

// 字符输入
static void on_char(TCHAR ch)
{
	if (g_focus < 0 || g_focus >= g_paramCount)
		return;

	InputBoxState& ib = g_inputs[g_focus];

	if (ch == 8)								// 退格
	{
		size_t n = _tcslen(ib.text);
		if (n > 0) ib.text[n - 1] = 0;
		ib.bad = false;
		return;
	}

	if (ch >= _T('0') && ch <= _T('9'))
	{
		size_t n = _tcslen(ib.text);
		if (n < 15)
		{
			ib.text[n] = ch;
			ib.text[n + 1] = 0;
			ib.bad = false;
		}
		return;
	}

	if (ch == _T('.'))
	{
		if (_tcschr(ib.text, _T('.')) == NULL)
		{
			size_t n = _tcslen(ib.text);
			if (n == 0) { ib.text[0] = _T('0'); ib.text[1] = _T('.'); ib.text[2] = 0; }
			else if (n < 15) { ib.text[n] = _T('.'); ib.text[n + 1] = 0; }
			ib.bad = false;
		}
		return;
	}
}

// 功能键：回车计算，Tab 切换参数
static void on_key(int vk)
{
	if (vk == VK_RETURN)
	{
		do_compute();
	}
	else if (vk == VK_TAB)
	{
		if (g_paramCount > 0)
			g_focus = (g_focus + 1) % g_paramCount;
	}
	else if (vk == VK_ESCAPE)
	{
		clear_all();
	}
}

/* ----------------------------- 主函数 --------------------------- */
int main()
{
	initgraph(kWinW, kWinH, NOMINIMIZE | NOCLOSE);

	HWND hwnd = GetHWnd();
	SetWindowText(hwnd, _T("图形计算器"));

	// 强制浅色标题栏，与界面配色统一
	BOOL bLightTitle = FALSE;
	DwmSetWindowAttribute(hwnd, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &bLightTitle, sizeof(bLightTitle));

	layout();
	BeginBatchDraw();

	bool running = true;
	bool dirty   = true;

	while (running)
	{
		ExMessage msg;
		while (peekmessage(&msg, EX_MOUSE | EX_KEY | EX_CHAR))
		{
			switch (msg.message)
			{
			case WM_MOUSEMOVE:
				if (on_move(msg.x, msg.y)) dirty = true;
				break;

			case WM_LBUTTONDOWN:
				running = on_click(msg.x, msg.y);
				dirty = true;
				break;

			case WM_RBUTTONDOWN:
				on_rclick(msg.x, msg.y);
				dirty = true;
				break;

			case WM_KEYDOWN:
				on_key(msg.vkcode);
				dirty = true;
				break;

			case WM_CHAR:
				if (msg.ch != 13 && msg.ch != 9)	// 回车 / Tab 由 WM_KEYDOWN 处理
				{
					on_char((TCHAR)msg.ch);
					dirty = true;
				}
				break;

			default:
				break;
			}
		}

		if (update_animation())
			dirty = true;

		if (dirty)
		{
			draw_frame();
			FlushBatchDraw();
			dirty = false;
		}

		Sleep(12);
	}

	EndBatchDraw();
	closegraph();
	return 0;
}
