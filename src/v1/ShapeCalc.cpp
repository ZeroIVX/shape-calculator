#define _CRT_SECURE_NO_WARNINGS
#include <graphics.h>              // 引用图形库头文件
#include <conio.h>
#include <stdio.h>
#include <windows.h>				//用到了定时函数sleep()
#include <math.h>
#include <string.h>
#define PI 3.1416					//定义π的数值
int button[8][4] = {{ 20,200,110,260 }, { 150,200,240,260 },{ 280,200,370,260 },{ 20,280,110,340 }, { 150,280,240,340 },{ 280,280,370,340 },{ 20,360,180,440 },{ 210,360,370,440 } };//按钮的二维数组
int display[] = { 20,20,370,180 };//显示区域矩形数组
float a, b, c, h, r;//a边，b边，c边，高度h，半径r
char Text[100]{};//显示文本数组
int button_judge(int x, int y)
{
	if (x > button[0][0] && x<button[0][2] && y>button[0][1] && y < button[0][3])return 1;
	if (x > button[1][0] && x<button[1][2] && y>button[1][1] && y < button[1][3])return 2;
	if (x > button[2][0] && x<button[2][2] && y>button[2][1] && y < button[2][3])return 3;
	if (x > button[3][0] && x<button[3][2] && y>button[3][1] && y < button[3][3])return 4;
	if (x > button[4][0] && x<button[4][2] && y>button[4][1] && y < button[4][3])return 5;
	if (x > button[5][0] && x<button[5][2] && y>button[5][1] && y < button[5][3])return 6;
	if (x > button[6][0] && x<button[6][2] && y>button[6][1] && y < button[6][3])return 7;
	if (x > button[7][0] && x<button[7][2] && y>button[7][1] && y < button[7][3])return 8;
	return 0;
}//按钮判断函数
float Rect(float a,float b) {
	float s = a * b;
	return(s);
}//矩形计算函数

float Tri(float a, float b,float c) {
	float p = (a + b + c) / 2;
	float s = sqrt(p * (p - a) * (p - b) * (p - c));
	if (s < 0)
	{
		sprintf_s(Text, "Error");
		s = -1;
	}
	return(s);
}//三角形计算函数，使用海伦公式计算

float Cir(float r) {
	float s = PI * r * r;
	return(s);
}//圆形计算函数，π取3.1416

float Para(float a, float h) {
	float s = a * h;
	return(s);
}//平行四边形计算函数

float Rhom(float a,float b){
	float s = a * b;
	return(s);
}//菱形计算函数

float Trap(float a,float b,float h) {
	float s = (a + b) * h / 2;
	return(s);
}//梯形计算函数

//每个图形计算函数均互相独立，方便后续更改或增减
int main()
{
	int i, event = 0;
	char s[30];//输入字符串变量
	short win_width, win_height;//定义窗口的宽度和高度
	win_width = 390; win_height = 460;
	float result = 0;//定义一个数用于存储计算结果
	initgraph(win_width, win_height,NOMINIMIZE | NOCLOSE | SHOWCONSOLE);//初始化窗口（黑屏）
	HWND hnd = GetHWnd();//获取窗口
	SetWindowText(hnd, "图形计算器");//命名窗口
	for (i = 0; i < 256; i += 5)//由黑到白的开屏动画
	{
		setbkcolor(RGB(i, i, i));//设置背景色，原来默认黑色
		cleardevice();//清屏（取决于背景色）
		Sleep(10);//延时
	}
	RECT B1 = { button[0][0],button[0][1],button[0][2],button[0][3] };
	RECT B2 = { button[1][0],button[1][1],button[1][2],button[1][3] };
	RECT B3 = { button[2][0],button[2][1],button[2][2],button[2][3] };
	RECT B4 = { button[3][0],button[3][1],button[3][2],button[3][3] };
	RECT B5 = { button[4][0],button[4][1],button[4][2],button[4][3] };
	RECT B6 = { button[5][0],button[5][1],button[5][2],button[5][3] };
	RECT B7 = { button[6][0],button[6][1],button[6][2],button[6][3] };
	RECT B8 = { button[7][0],button[7][1],button[7][2],button[7][3] };
	RECT DISPLAY = { display[0],display[1],display[2],display[3] };
	LOGFONT f;//字体样式指针
	gettextstyle(&f);					//获取字体样式
	_tcscpy(f.lfFaceName, _T("宋体"));	//设置字体为宋体
	f.lfQuality = ANTIALIASED_QUALITY;    // 设置输出效果为抗锯齿  
	settextstyle(&f);                     // 设置字体样式
	settextcolor(BLACK);				//BLACK在graphic.h头文件里面被定义为黑色的颜色常量
	drawtext("矩形", &B1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);//在矩形区域内输入文字，水平居中，垂直居中，单行显示
	drawtext("三角形", &B2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	drawtext("圆形", &B3, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	drawtext("平行四边形", &B4, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	drawtext("菱形", &B5, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	drawtext("梯形", &B6, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	drawtext("计算", &B7, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	drawtext("退出", &B8, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	setlinecolor(BLACK);
	rectangle(button[0][0], button[0][1], button[0][2], button[0][3]);
	rectangle(button[1][0], button[1][1], button[1][2], button[1][3]);
	rectangle(button[2][0], button[2][1], button[2][2], button[2][3]);
	rectangle(button[3][0], button[3][1], button[3][2], button[3][3]);
	rectangle(button[4][0], button[4][1], button[4][2], button[4][3]);
	rectangle(button[5][0], button[5][1], button[5][2], button[5][3]);
	rectangle(button[6][0], button[6][1], button[6][2], button[6][3]);
	rectangle(button[7][0], button[7][1], button[7][2], button[7][3]);
	rectangle(display[0], display[1], display[2], display[3]);
	MOUSEMSG m;//鼠标指针

	while (true)
	{
		m = GetMouseMsg();//获取一条鼠标消息

		switch (m.uMsg)
		{
		case WM_MOUSEMOVE:
			setrop2(R2_XORPEN);
			setlinecolor(LIGHTCYAN);//线条颜色为亮青色
			setlinestyle(PS_SOLID, 3);//设置画线样式为实现，10磅
			setfillcolor(WHITE);//填充颜色为白色
			if (button_judge(m.x, m.y) != 0)
			{
				if (event != button_judge(m.x, m.y))
				{
					event = button_judge(m.x, m.y);//记录这一次触发的按钮
					fillrectangle(button[event - 1][0], button[event - 1][1], button[event - 1][2], button[event - 1][3]);//有框填充矩形（X1,Y1,X2,Y2）
				}
			}
			else
			{
				if (event != 0)//上次触发的按钮未被修正为原来的颜色
				{
					fillrectangle(button[event - 1][0], button[event - 1][1], button[event - 1][2], button[event - 1][3]);//两次同或为原来颜色
					event = 0;
				}
			}
			break;
		case WM_LBUTTONDOWN:
			setrop2(R2_NOTXORPEN);//二元光栅——NOT(屏幕颜色 XOR 当前颜色)
			for (i = 0; i <= 10; i++)
			{
				setlinecolor(RGB(25 * i, 25 * i, 25 * i));//设置圆颜色
				circle(m.x, m.y, 2 * i);
				Sleep(20);//停顿30ms
				circle(m.x, m.y, 2 * i);//抹去刚刚画的圆
			}
			//按照按钮判断左键单击后的操作
			switch (button_judge(m.x, m.y))
			{
				//复原按钮原型
			case 1:
				InputBox(s, 30, "请输入矩形的长a");
				sscanf(s, "%f", &a);//将输入字符串依次扫描到全局变量里面
				InputBox(s, 30, "请输入矩形的宽b");
				sscanf(s, "%f", &b);//将输入字符串依次扫描到全局变量里面
				result = Rect(a, b);
				FlushMouseMsgBuffer();//单击事件后清空鼠标消息
				break;
			case 2:
				InputBox(s, 30, "请输入三角形的边a");
				sscanf(s, "%f", &a);//将输入字符串依次扫描到全局变量里面
				InputBox(s, 30, "请输入三角形的边b");
				sscanf(s, "%f", &b);
				InputBox(s, 30, "请输入三角形的边c");
				sscanf(s, "%f", &c);
				result = Tri(a, b, c);
				FlushMouseMsgBuffer();
				break;
			case 3:
				InputBox(s, 30, "请输入圆的半径r");
				sscanf(s, "%f", &r);
				result = Cir(r);
				FlushMouseMsgBuffer();
				break;
			case 4:
				InputBox(s, 30, "请输入平行四边形的底边a");
				sscanf(s, "%f", &a);
				InputBox(s, 30, "请输入平行四边形的高h");
				sscanf(s, "%f", &h);
				result = Para(a,h);
				FlushMouseMsgBuffer();
				break;
			case 5:
				InputBox(s, 30, "请输入菱形的对角线a");
				sscanf(s, "%f", &a);
				InputBox(s, 30, "请输入菱形的对角线b");
				sscanf(s, "%f", &b);
				result = Rhom(a,b);
				FlushMouseMsgBuffer();
				break;
			case 6:
				InputBox(s, 30, "请输入梯形的上底a");
				sscanf(s, "%f", &a);
				InputBox(s, 30, "请输入梯形的下底b");
				sscanf(s, "%f", &b);
				InputBox(s, 30, "请输入梯形的高");
				sscanf(s, "%f", &h);
				result = Trap(a,b,h);
				FlushMouseMsgBuffer();
				break;
			case 7:
				gettextstyle(&f);					//获取字体样式
				_tcscpy(f.lfFaceName, _T("宋体"));	//设置字体为宋体
				f.lfHeight = 48;
				f.lfQuality = ANTIALIASED_QUALITY;    // 设置输出效果为抗锯齿  
				settextstyle(&f);    // 设置字体样式        
				//setbkmode(TRANSPARENT);// 设置背景样式
				sprintf_s(Text, "%.4f", result);
				if (result < 0)//判断结果值为正
				{
					sprintf_s(Text, "Error");//若结果值为负，则数值有误，将显示的文字更改为Error报错
				}
				//int text_width = 350 / 2 - textwidth(Text) / 2;//获取结果字符长度并提供输出位置
				//int	text_height = 160 / 2 - textheight(Text) / 2;        (居中操作，报错故放弃)
				outtextxy(35,80,Text);//输出文字
				FlushMouseMsgBuffer();//单击事件后清空鼠标消息
				break;
			case 8:
				closegraph();//关闭绘图环境
				exit(0);//正常退出
			default:
				FlushMouseMsgBuffer();//单击事件后清空鼠标消息
				//printf("\r\n(%d,%d)",m.x,m.y);//打印鼠标坐标，方便调试时确定区域
				break;
			}
			break;
		}
	}
	return 0;
}