//////////////////////////////////////
//
//	JumpAndJump
//	跳一跳小游戏
//	
//	huidong <mailkey@yeah.net>
//
//	创建时间：2020.11.27
//	最后修改：2020.12.26
//

#include <easyx.h>
#include <time.h>
#include <math.h>
#include <conio.h>
#include <stdlib.h>

#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1:0)
#define RandNum(min, max)	(rand() % (max - min + 1) + min)

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

// 方块数
const int m_nBlocksNum = 512;

// 背景色
COLORREF m_colorBk = RGB(0, 162, 232);

// 方块间距极值
int m_nMinInterval = 50;
int m_nMaxInterval = 400;

// 方块宽度极值
int m_nMinBlockSize = 20;
int m_nMaxBlockSize = 100;

// 方块顶部和底部
int m_nBlockTop = WINDOW_HEIGHT - 100;
int m_nBlockBottom = WINDOW_HEIGHT;

// 玩家大小极值
int m_nMaxPlayerSize = 50;
int m_nMinPlayerSize = 20;

// 玩家结构体
struct Player
{
	// 方块坐标
	RECT rctBlocks[m_nBlocksNum] = { 0 };

	// 方块颜色
	COLORREF colorBlocks[m_nBlocksNum] = { 0 };

	// 背景画布（方块画在上面）
	IMAGE* imgBk = NULL;

	// 图像偏移
	int nImgOffsetX;

	// 得分
	int nScore;

	// 玩家坐标（方块左下角坐标）
	float fPlayerX, fPlayerY;

	// 当前玩家身高
	float fPlayerSize;

	// 松开空格后跳跃的距离
	float fJumpDistance;

	// 当前玩家站在第几个方块上
	int nStandBlockNum;

	// 按下空格标志
	bool bEnter;

	// 起跳标志
	bool bJump;
};


// 跳跃曲线计算函数
// x1, y1 二次函数和x轴的左侧交界点
// top_x, top_y 二次函数的顶点
// 返回关于x的二次函数中坐标x对应的坐标y
double jump_func(double x1, double y1, double top_x, double top_y, double x)
{
	/*
		函数解析式：y = a(x - top_x)^2 + top_y
		a = (y1 - top_y) / (x1 - top_x)^2

		所以 y = (y1 - top_y) / (x1 - top_x)^2 * (x - top_x)^2 + top_y
	*/

	return (y1 - top_y) / pow(x1 - top_x, 2) * pow(x - top_x, 2) + top_y;
}

// 精确延时函数(可以精确到 1ms，精度 ±1ms)
// by yangw80<yw80@qq.com>, 2011-5-4
void HpSleep(int ms)
{
	static clock_t oldclock = clock();		// 静态变量，记录上一次 tick

	oldclock += ms * CLOCKS_PER_SEC / 1000;	// 更新 tick

	if (clock() > oldclock)					// 如果已经超时，无需延时
		oldclock = clock();
	else
		while (clock() < oldclock)			// 延时
			Sleep(1);						// 释放 CPU 控制权，降低 CPU 占用率
}

void init_graph()
{
	initgraph(WINDOW_WIDTH, WINDOW_HEIGHT);
	setbkcolor(m_colorBk);
	setbkmode(TRANSPARENT);
	cleardevice();
	BeginBatchDraw();
	srand((UINT)time(NULL));
}

void end_graph()
{
	EndBatchDraw();
	closegraph();
}

// 得到云朵图像
// img			传出图像指针
// bkColor		云朵背景色
// cloudColor	云朵颜色
// width		整个云朵的宽，由于宽高必须成比例，所以只需要设置宽即可
void DrawCloud(IMAGE* img, COLORREF bkColor = BLACK, COLORREF cloudColor = WHITE, int width = 180)
{
	IMAGE* old = GetWorkingImage();
	SetWorkingImage(img);

	// 比例
	double ratio = width / 180.0;
	Resize(img, width, (int)(100 * ratio));
	setbkcolor(bkColor);
	setfillcolor(cloudColor);
	setlinecolor(cloudColor);
	cleardevice();

	fillcircle((int)(95 * ratio), (int)(40 * ratio), (int)(40 * ratio));
	fillcircle((int)(85 * ratio), (int)(70 * ratio), (int)(27 * ratio));
	fillcircle((int)(40 * ratio), (int)(60 * ratio), (int)(40 * ratio));
	fillcircle((int)(125 * ratio), (int)(75 * ratio), (int)(25 * ratio));
	fillcircle((int)(145 * ratio), (int)(40 * ratio), (int)(30 * ratio));

	SetWorkingImage(old);
}

// 初始化游戏
void init_game(Player* player)
{
	player->nStandBlockNum = 0;
	player->fPlayerSize = (float)m_nMaxPlayerSize;
	player->nScore = 0;
	player->nStandBlockNum = 0;
	player->nImgOffsetX = 0;
	player->fJumpDistance = 0;
	player->bEnter = false;
	player->bJump = false;

	// 生成方块
	for (int i = 0, last = 0; i < m_nBlocksNum; i++)
	{
		player->rctBlocks[i].top = m_nBlockTop;
		player->rctBlocks[i].bottom = m_nBlockBottom;
		player->rctBlocks[i].left = i == 0 ? player->nImgOffsetX : last + RandNum(m_nMinInterval, m_nMaxInterval);
		player->rctBlocks[i].right = player->rctBlocks[i].left + RandNum(m_nMinBlockSize, m_nMaxBlockSize);
		player->colorBlocks[i] = rand();
		last = player->rctBlocks[i].right;
	}

	if (!player->imgBk)
		player->imgBk = new IMAGE;
	player->imgBk->Resize(player->rctBlocks[m_nBlocksNum - 1].right, WINDOW_HEIGHT);
	SetWorkingImage(player->imgBk);

	// 绘制方块
	setbkcolor(m_colorBk);
	cleardevice();
	for (int i = 0; i < m_nBlocksNum; i++)
	{
		setlinecolor(player->colorBlocks[i]);
		setfillcolor(player->colorBlocks[i]);
		fillrectangle(
			player->rctBlocks[i].left,
			player->rctBlocks[i].top,
			player->rctBlocks[i].right,
			player->rctBlocks[i].bottom
		);
	}

	// 绘制云朵
	IMAGE imgCloud;
	int nMinCloudInterval = 600;
	int nMaxCloudInterval = 1300;
	for (int i = 0, nLastCloudX = 0; i < m_nMaxInterval * m_nMaxBlockSize / nMaxCloudInterval; i++)
	{
		nLastCloudX += RandNum(nMinCloudInterval, nMaxCloudInterval);
		DrawCloud(&imgCloud, m_colorBk, WHITE, RandNum(90, 200));
		putimage(nLastCloudX, RandNum(50, 150), &imgCloud);
	}

	SetWorkingImage();

	player->fPlayerX = (float) player->rctBlocks[0].left;
	player->fPlayerY = (float) m_nBlockTop;
}

void draw(Player* player)
{
	cleardevice();

	// 背景画布
	putimage(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, player->imgBk, player->nImgOffsetX, 0);

	// 画小人
	setlinecolor(BLACK);
	setfillcolor(BLUE);
	fillrectangle(
		(int)player->fPlayerX - player->nImgOffsetX,			// 由于需要画布偏移，小人的位置要和画布位置对应，所以需要减去画布偏移量
		(int)player->fPlayerY - (int)player->fPlayerSize,	// 坐标是方块左下角坐标，所以需要计算出方块左上角坐标
		((int)player->fPlayerX - player->nImgOffsetX) + m_nMaxPlayerSize,	// 右侧x坐标是左侧x坐标加上玩家宽度
		(int)player->fPlayerY							// 坐标是方块左下角坐标，所以底部y坐标直接用
	);

	// 画分数
	wchar_t strSocre[12] = { 0 };
	_itow_s(player->nScore, strSocre, 12, 10);

	settextcolor(WHITE);
	settextstyle(32, 0, L"system");
	outtextxy(30, 30, strSocre);

	FlushBatchDraw();
}

void lose_menu(Player* player)
{
	RECT rctTip = { 0, WINDOW_HEIGHT / 2 - 60, WINDOW_WIDTH, WINDOW_HEIGHT / 2 + 60 };

	setfillcolor(BLUE);
	fillrectangle(rctTip.left, rctTip.top, rctTip.right, rctTip.bottom);

	// 画分数
	wchar_t strSocre[12] = { 0 };
	wsprintf(strSocre, L"得分：%d", player->nScore);

	settextstyle(48, 0, L"system");
	drawtext(strSocre, &rctTip, DT_VCENTER | DT_CENTER | DT_SINGLELINE);

	rctTip.top = rctTip.bottom - 20;
	settextstyle(12, 0, L"system");
	drawtext(L"按空格重新开始", &rctTip, DT_VCENTER | DT_CENTER | DT_SINGLELINE);

	while (!KEY_DOWN(' '))
	{
		FlushBatchDraw();
		Sleep(10);
	}
}

void user_input(Player* player)
{
	if (KEY_DOWN(VK_SPACE))
	{
		player->bEnter = true;

		// 如果小人没有下蹲到最低身高，就继续下蹲
		if (player->fPlayerSize > m_nMinPlayerSize)
		{
			// 小人跳起准备时，身高减值
			const float fJumpPrepareHeight = (float)0.03;

			// 跳跃距离增值，通过计算确保小人的弹跳力可以跳到下一个方块
			const float fJumpLength = (float)(m_nMaxInterval * 1.8 / (m_nMaxPlayerSize - m_nMinPlayerSize) * fJumpPrepareHeight);

			player->fPlayerSize -= fJumpPrepareHeight;
			player->fJumpDistance += fJumpLength;
		}
	}

	// 松开空格，就跳起
	else if (player->bEnter)
	{
		player->bEnter = false;
		player->bJump = true;
	}
}

void game_run(Player* player)
{
	draw(player);

	// 跳起
	if (player->bJump)
	{
		player->bJump = false;

		// 由于xy坐标会变化，现在存起来
		float x = player->fPlayerX;
		float y = player->fPlayerY;

		// 是否已经跳出
		bool bJumped = false;

		// 是否锁定x坐标
		bool bLockX = false;

		// 锁定的x坐标值
		float fLockX = 0;

		// y坐标误差判定长度，即y坐标在多大的范围内仍然进行判定
		int nOffsetY = 10;

		// 玩家碰到物体外壁时，玩家和物体的x坐标差最大值
		int nOffsetHitX = 3;

		// 跳跃
		for (; ; player->fPlayerX++)
		{
			// 跳跃的同时恢复身高
			if (player->fPlayerSize < m_nMaxPlayerSize)
			{
				player->fPlayerSize += (float)0.2;
			}

			/* 若x坐标在锁定状态下 */
			if (bLockX)
			{
				player->fPlayerX = fLockX;
				player->fPlayerY += (float)0.1;	/* y保持下降 */
			}
			else
			{
				// 跳跃曲线计算
				player->fPlayerY = 
					(float)jump_func(
						x,									// 二次函数中和x轴交界的左侧点x坐标
						y,									// 二次函数中和x轴交界的左侧点y坐标
						x + player->fJumpDistance / 2,		// 二次函数中顶点x坐标（和x轴交界的两个点之间的线段的一半）
						y - player->fJumpDistance / 3,		// 二次函数中顶点y坐标（取x轴交界的两个点之间的线段的1/3作为跳跃高度）
						player->fPlayerX)					// 返回x坐标对应的y坐标
					+ (y - player->fPlayerY) / 4;			// 重力加速度
			}

			// y变高了，标记下已经跳出
			if (player->fPlayerY < y)
				bJumped = true;

			// 绘制
			draw(player);
			HpSleep(1);

			// 已经跳出且下落到原来的y点
			if (player->fPlayerY >= y && bJumped)
			{
				// 判断是否跳跃到了点上
				for (int i = player->nStandBlockNum; i < m_nBlocksNum; i++)
				{
					// 判断人物方块的左下角点或右下角点是否在物体上
					if (((player->fPlayerX > player->rctBlocks[i].left &&
						player->fPlayerX < player->rctBlocks[i].right) ||							// 身体左边判定
						(player->fPlayerX + m_nMaxPlayerSize > player->rctBlocks[i].left &&
							player->fPlayerX + m_nMaxPlayerSize < player->rctBlocks[i].right)) &&	// 身体右边判定
						player->fPlayerY - y <= nOffsetY)	// y坐标下降到一定高度
					{
						// 站稳且不在原方块上，加分
						if (i != player->nStandBlockNum)
						{
							player->nScore++;
							player->nStandBlockNum = i;
						}

						// 无论有无跳出原方块，落在方块上了就跳出循环
						// 否则会出现站在方块上却掉落下去的bug
						goto out;
					}

					// 若没有下落到物体上，且y坐标已经超出判定范围时，若x坐标满足和物体相撞，那么玩家顺着物体滑到地上。
					else if (!bLockX && player->fPlayerY - y > nOffsetY &&
						(player->fPlayerX + m_nMaxPlayerSize) - player->rctBlocks[i].left < nOffsetHitX &&
						(player->fPlayerX + m_nMaxPlayerSize) - player->rctBlocks[i].left > -nOffsetHitX)
					{
						bLockX = true;
						fLockX = (float)(player->rctBlocks[i].left - m_nMaxPlayerSize);
						player->fPlayerX = fLockX;
						draw(player);
						Sleep(150);
					}
				}
			}

			// 已经跳到屏幕外
			if (player->fPlayerY >= WINDOW_HEIGHT)
			{
				lose_menu(player);
				init_game(player);
				return;
			}
		}
	out:
		// y坐标复原
		player->fPlayerY = (float) y;

		// 跳跃长度清零
		player->fJumpDistance = 0;

		Sleep(100);

		// 画面移动
		for (; player->nImgOffsetX <= player->fPlayerX - 20; player->nImgOffsetX++)
		{
			draw(player);
		}
	}

}

void startmenu(Player* player)
{
	draw(player);

	settextstyle(72, 0, L"宋体");
	settextcolor(BLUE);
	for (int x = 220, y = 120; x > 215, y > 115; x--, y--)
	{
		outtextxy(x, y, L"跳一跳");
		FlushBatchDraw();
		Sleep(150);
	}

	Sleep(100);
	settextstyle(16, 0, L"system");
	outtextxy(230, 220, L"made by huidong 2020.12.26");
	FlushBatchDraw();

	Sleep(800);
	settextcolor(WHITE);
	outtextxy(140, 400, L" <<< Press and hold the space to start your first jump >>> ");

	while (true)
	{
		FlushBatchDraw();
		Sleep(10);

		if (_kbhit())
		{
			if (_getch() == ' ')
			{
				break;
			}
		}
	}
}

int main()
{
	Player player;

	init_graph();
	init_game(&player);
	startmenu(&player);

	while (true)
	{
		user_input(&player);
		game_run(&player);
	}

	end_graph();
	return 0;
}
