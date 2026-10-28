# 03 · 架构与实现

## 分层结构

v2 采用「数据驱动 + 三层分离」的单文件结构，不引入类继承体系：

```
┌─────────────────────────────────────────────┐
│  数据层  ShapeDef 表 + 15 个计算函数         │  ← 只有这部分随图形增加而增长
├─────────────────────────────────────────────┤
│  渲染层  draw_frame() → 背景/按钮/面板/画布   │
├─────────────────────────────────────────────┤
│  事件层  on_click / on_move / on_char / on_key│
└─────────────────────────────────────────────┘
          共享状态：g_selected / g_inputs / g_results / g_pts
```

新增一种图形时，只触碰数据层——渲染层读取表里的 `name`、`paramName`、`resultName` 自动生成界面，事件层按 `paramCount` 自动决定输入框数量与 `Tab` 循环边界。这使 15 种图形共用同一套 UI 与交互代码。

## 数据层：描述表

```cpp
struct ShapeDef
{
	const TCHAR* name;            // 图形名称（同时作为按钮文字）
	int          group;           // 0 = 平面，1 = 立体
	int          paramCount;      // 参数个数（决定输入框数量）
	const TCHAR* paramName[3];    // 参数标签
	int          resultCount;     // 结果个数
	const TCHAR* resultName[4];   // 结果标签
	bool       (*compute)(const float* in, float* out);   // 计算函数
};
```

表项示例：

```cpp
{ _T("矩形"), 0, 2,
  { _T("长 a"), _T("宽 b") },
  3,
  { _T("面积 S"), _T("周长 C"), _T("对角线 d") },
  calcRect },
```

计算函数统一签名 `bool (const float* in, float* out)`：返回 `false` 表示参数非法，`out` 按 `resultName` 顺序写入结果。函数内部只做纯计算与合法性判断，不接触任何界面状态——这让算法可以独立测试。

```cpp
static bool calcTriangle(const float* in, float* out)
{
	float a = in[0], b = in[1], c = in[2];
	if (a <= 0.0f || b <= 0.0f || c <= 0.0f) return false;
	if (a + b <= c || a + c <= b || b + c <= a) return false;   // 三边关系前置判断

	float p = (a + b + c) / 2.0f;
	float s = sqrtf(p * (p - a) * (p - b) * (p - c));

	out[0] = s;
	out[1] = a + b + c;
	out[2] = s / p;                 // 内切圆半径
	out[3] = a * b * c / (4.0f * s); // 外接圆半径
	return true;
}
```

## 渲染层

### 双缓冲与脏标记

```cpp
BeginBatchDraw();                       // 开启后台缓冲，避免逐条绘制闪烁
while (running)
{
    /* 处理消息，状态变化时置 dirty = true */
    if (update_animation()) dirty = true;   // 悬停过渡动画未收敛
    if (dirty) { draw_frame(); FlushBatchDraw(); dirty = false; }
    Sleep(12);
}
EndBatchDraw();
```

全量重绘（约 12 ms 一帧上限）而不是增量刷新：窗口尺寸固定、元素数量有限，全量重绘的代码复杂度远低于维护脏矩形列表。真正的开销控制靠 `dirty`——鼠标静止且动画收敛后不再重绘，空闲期 CPU 占用趋近于零。

### 绘制顺序

```
draw_background()   渐变背景（4 px 步长逐行填充）+ 角落装饰
draw_header()       标题与副标题
分组标题 + 15 个图形按钮
分隔线
4 个功能按钮
draw_panel()        右侧卡片：
                    ├─ 规则图形：名称 / 参数输入框 / 计算按钮 / 结果列表
                    ├─ 手绘模式：网格画布 / 结果 / 操作提示
                    └─ 未选中：引导提示 + 装饰轮廓
```

按钮与输入框统一用圆角矩形：先画偏移 2~3 px 的深色圆角作为投影，再画本体覆盖。EasyX 的 `roundrect` **只描边不填充**，需要填充必须用 `fillroundrect`——这是实现阶段踩过的坑（见 06 文档）。

### 悬停动画

每个按钮维护 `hover`（当前值）与 `target`（目标值，0 或 1），每帧向目标插值 35%：

```cpp
float d = b.target - b.hover;
if (fabsf(d) > 0.02f) { b.hover += d * 0.35f; busy = true; }
else b.hover = b.target;
```

`hover` 作为插值系数参与底色与文字色的混合 `mix(baseColor, hoverColor, hover)`，得到平滑的染色过渡而非硬切。`update_animation()` 返回 `busy`，动画未收敛时强制重绘，收敛后自动停止。

## 事件层

### 消息循环

```cpp
ExMessage msg;
while (peekmessage(&msg, EX_MOUSE | EX_KEY | EX_CHAR))   // 非阻塞，一次取完整个队列
{
    switch (msg.message)
    {
    case WM_MOUSEMOVE:    if (on_move(msg.x, msg.y)) dirty = true; break;
    case WM_LBUTTONDOWN:  running = on_click(msg.x, msg.y); dirty = true; break;
    case WM_RBUTTONDOWN:  on_rclick(msg.x, msg.y); dirty = true; break;
    case WM_KEYDOWN:      on_key(msg.vkcode); dirty = true; break;
    case WM_CHAR:         字符输入（过滤 13/9，避免与 WM_KEYDOWN 重复）break;
    }
}
```

键盘事件分两路：`WM_CHAR` 负责可见字符与退格，`WM_KEYDOWN` 负责 `Enter`/`Tab`/`Esc` 等功能键。`WM_CHAR` 中显式过滤 `\r` 与 `\t`，否则回车会被两个分支各处理一次。

### 命中测试

矩形包含判断，按优先级依次匹配：画布 → 输入框 → 计算按钮 → 功能按钮 → 图形按钮。手绘模式下手绘区优先于其它元素，避免画布与面板重叠区域的歧义。

### 状态互斥

规则图形与手绘模式是两个互斥界面，由 `g_drawMode` 与 `g_selected` 共同表达：

| 状态 | `g_drawMode` | `g_selected` | 右侧面板 |
|---|---|---|---|
| 未选择 | `false` | `-1` | 引导提示 |
| 规则图形 | `false` | `0..14` | 参数表单 |
| 手绘 | `true` | `-1` | 网格画布 |

`select_shape()` 与 `enter_draw_mode()` 各自负责把对方的状态复位，`draw_panel()` 只依据这两个变量分支，不维护额外的模式枚举。

## 几何算法

### 坐标映射

手绘画布的像素坐标与数学坐标分离：

```cpp
static float to_ux(int x) { return (x - kCanvas.left) / (float)kCell; }
static float to_uy(int y) { return (kCanvas.bottom - y) / (float)kCell; }  // y 轴翻转
```

原点落在画布左下角、向上为正，与数学坐标系一致，避免面积符号与用户直觉相反。落点吸附由 `snap_point()` 完成：除以格宽后四舍五入回格点，并夹取到 `[0, kCellCols] × [0, kCellRows]` 范围内。

### 鞋带公式

```cpp
static float polygon_area()
{
	if (g_ptCount < 3) return 0.0f;

	float sum = 0.0f;
	for (int i = 0; i < g_ptCount; ++i)
	{
		int j = (i + 1) % g_ptCount;           // 末点自动连回首点
		sum += to_ux(g_pts[i].x) * to_uy(g_pts[j].y)
			 - to_ux(g_pts[j].x) * to_uy(g_pts[i].y);
	}
	return fabsf(sum) / 2.0f;                   // 取绝对值，顺/逆时针均为正
}
```

`fabsf` 是关键：顶点按顺时针或逆时针输入会得到相反符号的有向面积，取绝对值后两种绕向都能得到正确结果。

## 设计取舍

| 决策 | 选择 | 理由 |
|---|---|---|
| 界面方案 | EasyX 而非原生 Win32 | 省去窗口类注册、消息分发、双缓冲等样板代码，把代码量集中在业务与界面上 |
| 界面框架 | 不用 MFC / Qt | 两者都会带来数百 KB 依赖与构建配置，与「单文件 + 一键脚本」的交付形态冲突 |
| 扩展方式 | 表 + 函数指针，不用继承 | 15 种图形没有共享状态，虚函数体系只会增加层级；函数指针表更直观且可静态初始化 |
| 容器 | 固定数组（顶点上限 64） | 无需堆分配与迭代器，边界明确；超出上限时静默忽略新点而不是崩溃 |
| 坐标存储 | 直接存客户区坐标 | 绘制时无需二次换算；数学坐标只在计算面积/周长时按需转换，避免两套坐标同步失效 |
| 结果呈现 | 多指标并列而非单个大数字 | 一次输入的价值在于同时看到互相校验的多个量（如三角形的内切/外接圆半径） |

## 扩展一种新图形

三步，均在数据层内完成：

1. 写计算函数，签名 `bool calcXxx(const float* in, float* out)`，参数非法返回 `false`
2. 在 `kShapes[]` 中追加一行，填写名称、分组、参数名、结果名、函数指针
3. 若图形总数超过 9，在 `layout()` 的网格循环里调整行数上界

界面布局、输入校验、结果渲染、`Tab` 焦点循环全部自动适配，无需改动。

---

<sub>Zero · 2026</sub>
