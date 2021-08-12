#include <stdio.h>
#include <Windows.h>
#include <graphics.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include <math.h>

#define WINDOW_WIDTH 1024	// 窗口宽度
#define WINDOW_HEIGHT 600	// 窗口高度
#define WINDOW_REFRESH_DURATION 10 // 窗口刷新频率
#define MAP_WIDTH 4096	// 地图宽度
#define MAP_HEIGHT 2400	// 地图高度
#define GRID_SIZE 10 // 背景格子大小

#define DIRECTION_LEFT -1 // 运动方向
#define DIRECTION_RIGHT 1
#define DIRECTION_TOP -1
#define DIRECTION_BOTTOM 1

#define BALL_DEFAULT_RADIUS 30 // 球的初始大小
#define BALL_DEFAULT_SPEED 6 // 球的初始速度
#define BALL_MIN_SPEED 0.5f // 球的最小速度
#define BALL_EAT_RATE 0.2f // 食物半径转化率
#define BALL_ADBANDON_RATE 0.2 // 加速抛弃的部分

#define ANIMATION_DURATION 100 // 动画时长

#define FOOD_COUNT 1200	//游戏中食物数量
#define FOOD_MAX_RADIUS 2 // 食物最大大小

#define ENEMY_COUNT 20 // 敌人数量

// map属性
typedef struct Map {
	int x; // 地图的x坐标
	int y; // 地图的y坐标
} Map;

// 运动方向
typedef struct Direction {
	int x;
	int y;
} Direction;

// 球动画属性
typedef struct BallAnimator {
	int x_v;
	int y_v;
	float r_v;
	int time;
}BallAnimator;

// 球的目的地
typedef struct BallTarget {
	int x;
	int y;
}BallTarget;

// 球属性
typedef struct Ball {
	int x; // x坐标
	int y; // y坐标
	float r; // 半径
	float speed; // 速度
	DWORD color; // 球颜色
	char name[20]; // 球名
	bool is_alive; // 是否活着
	Direction dir; // 运动方向
	BallTarget target; // 运动目标
	BallAnimator anim; // 动画的属性
} Ball;

// 食物属性
typedef struct Food {
	int x;
	int y;
	float r;
	DWORD color; // 食物颜色
} Food;



Map map; // 地图
Ball player;// 玩家
Food feed[FOOD_COUNT]; // 食物
Ball enemies[ENEMY_COUNT]; // 敌人

int size_score; // 体积分数

// 球的移动函数
void move_ball_to_top(Ball * ball);
void move_ball_to_bottom(Ball * ball);
void move_ball_to_left(Ball * ball);
void move_ball_to_right(Ball * ball);

// 敌人的移动函数
void move_enemy_to_top(Ball * ball);
void move_enemy_to_bottom(Ball * ball);
void move_enemy_to_left(Ball * ball);
void move_enemy_to_right(Ball * ball);

void reset_animator(Ball * ball); // 重设动画
bool is_animating(Ball * ball); // 判断球是否有动画进行
float generate_anim_attr(float attr); // 生成动画属性

// 球的加速函数
void accelerate_ball(Ball * ball);

Food generate_food(); // 食物生成函数
Ball generate_enemy(); // 敌人生成函数

void update_eat_food(Ball * ball); // 更新食物情况
void eat_ball(Ball * L, Ball * S);
void update_player_enemy_rel(Ball * player, Ball * enemy); // 判断玩家和敌人是否相撞
void update_two_ball_rel(Ball * b1, Ball * b2); // 检测两个球是否相吃及位置关系

void enemy_move(Ball * enemy); // 决定敌人应该怎么操作

void draw_background() {
	// 在屏幕范围内绘制白色背景
	setfillcolor(WHITE);
	solidrectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	// 在屏幕范围内内绘制格子
	setlinecolor(RGB(230, 231, 239));
	for (int x = 0; x <= WINDOW_WIDTH; x += GRID_SIZE) {
		line(x, 0, x, WINDOW_HEIGHT);
	}
	for (int y = 0; y <= WINDOW_HEIGHT; y += GRID_SIZE) {
		setfillcolor(RGB(230, 231, 239));
		line(0, y, WINDOW_WIDTH, y);
	}
}

void draw_feed() {
	// 只绘制屏幕范围内的食物
	for (int i = 0; i < FOOD_COUNT; i++) {
		Food cur = feed[i];
		if (cur.x < map.x - cur.r * 2 || cur.x > map.x + WINDOW_WIDTH + cur.r * 2) {
			continue;
		}
		if (cur.y < map.y - cur.r * 2 || cur.y > map.y + WINDOW_HEIGHT + cur.r * 2) {
			continue;
		}
		setfillcolor(cur.color);
		solidcircle(cur.x - map.x, cur.y - map.y, cur.r);
	}
}

void draw_enemies() {
	// 只绘制在屏幕范围内的敌人
	for (int i = 0; i < ENEMY_COUNT; i++) {
		Ball * cur = &enemies[i];
		if (!cur->is_alive) {
			continue;
		}
		if (cur->x < map.x - cur->r || cur->x > map.x + WINDOW_WIDTH + cur->r) {
			continue;
		}
		if (cur->y < map.y - cur->r || cur->y > map.y + WINDOW_HEIGHT + cur->r) {
			continue;
		}
		if (is_animating(&player)) {
			player.x += (int)player.anim.x_v;
			player.y += (int)player.anim.y_v;
			player.r += player.anim.r_v;
			player.anim.time += 1;
			if (player.anim.time == ANIMATION_DURATION / WINDOW_REFRESH_DURATION) {
				reset_animator(&player);
			}
		}
		setfillcolor(cur->color);
		solidcircle(cur->x - map.x, cur->y - map.y, cur->r);
	}
}

void draw_player() {
	if (is_animating(&player)) {
		player.x += (int)player.anim.x_v;
		player.y += (int)player.anim.y_v;
		player.r += player.anim.r_v;
		player.anim.time += 1;
		if (player.anim.time == ANIMATION_DURATION / WINDOW_REFRESH_DURATION) {
			reset_animator(&player);
		}
	}
	setfillcolor(player.color);
	solidcircle(player.x, player.y, player.r);

	// 绘制玩家名字
	LOGFONT logfont;
	gettextstyle(&logfont);
	logfont.lfHeight = player.r * 0.4f;
	settextcolor(WHITE);
	settextstyle(&logfont);
	int text_x = player.x - textwidth(player.name) / 2;
	int text_y = player.y - textheight(player.name) / 2;
	outtextxy(text_x, text_y, player.name);
}

void draw_score() {
	char text[20] = "";
	sprintf_s(text, "当前分数: %d", size_score);
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 24;
	settextcolor(RGB(0, 0, 0));
	_tcscpy_s(f.lfFaceName, _T("宋体"));
	f.lfQuality = ANTIALIASED_QUALITY;
	settextstyle(&f);
	setbkmode(TRANSPARENT);
	int text_x = WINDOW_WIDTH - 20 - textwidth(text);
	int text_y = 20;
	outtextxy(text_x, text_y, text);
}

void draw_game() {
	// 绘制背景
	draw_background();
	// 绘制食物
	draw_feed();
	// 绘制敌人
	draw_enemies();
	// 绘制玩家
	draw_player();
	// 绘制体积分数
	draw_score();
}

bool detect_circles_crash(int x1, int y1, int r1, int x2, int y2, int r2) {
	double distance = (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
	double r = (r1 + r2) * (r1 + r2);
	return distance <= r;
}

Food generate_food() {
	Food temp;
	temp.color = RGB(rand() % 256, rand() % 256, rand() % 256);
	temp.x = rand() % MAP_WIDTH;
	temp.y = rand() % MAP_HEIGHT;
	temp.r = rand() % FOOD_MAX_RADIUS + 2;
	return temp;
}

Ball generate_enemy() {
	Ball temp;
	temp.color = RGB(rand() % 256, rand() % 256, rand() % 256);
	temp.x = rand() % MAP_WIDTH;
	temp.y = rand() % MAP_HEIGHT;
	temp.r = BALL_DEFAULT_RADIUS;
	temp.speed = BALL_DEFAULT_SPEED;
	temp.is_alive = true;
	temp.anim.time = 0;
	temp.anim.r_v = 0;
	temp.anim.x_v = 0;
	temp.anim.y_v = 0;
	temp.dir.x = 0;
	temp.dir.y = 0;
	temp.target.x = 0;
	temp.target.y = 0;
	return temp;
}

void update_eat_food(Ball * ball) {
	for (int i = 0; i < FOOD_COUNT; i++) {
		Food cur = feed[i];
		if (detect_circles_crash(ball->x, ball->y, ball->r, cur.x, cur.y, cur.r)) {
			float new_radius = ball->r + cur.r * BALL_EAT_RATE;
			ball->speed = ball->speed * ball->r / new_radius;
			if (ball->speed < BALL_MIN_SPEED) ball->speed = BALL_MIN_SPEED;
			ball->r = new_radius;
			feed[i] = generate_food();
			continue;
		}
	}
}

void eat_ball(Ball * L, Ball * S)
{
	float new_radius = L->r + S->r * BALL_EAT_RATE;
	L->speed = L->speed * L->r / new_radius;
	if (L->speed < BALL_MIN_SPEED) L->speed = BALL_MIN_SPEED;
	//L->r = new_radius;
	L->anim.r_v = generate_anim_attr(new_radius - L->r);
	S->is_alive = false;
}

void update_player_enemy_rel(Ball * player, Ball * enemy)
{
	if (!detect_circles_crash(player->x, player->y, player->r, enemy->x - map.x, enemy->y - map.y, enemy->r)) {
		return;
	}
	if (player->r > enemy->r) {
		eat_ball(player, enemy);
	}
	if (player->r < enemy->r) {
		eat_ball(enemy, player);
	}
}

void update_two_ball_rel(Ball * b1, Ball * b2)
{
	if (!detect_circles_crash(b1->x, b1->y, b1->r, b2->x, b2->y, b2->r)) {
		return;
	}
	if (b1->r > b2->r) {
		eat_ball(b1, b2);
	}
	if (b1->r < b2->r) {
		eat_ball(b2, b1);
	}
}

void enemy_move(Ball * enemy)
{
	/*		以下是保守策略		*/
	// 计算对自己有威胁的球
	// 计算公式: 对方半径/自己半径 + 屏幕半径 ^ 2 / 对方距离 ^ 2
	Ball * cur_enemy = NULL;
	for (int i = 0; i < ENEMY_COUNT; i++) {
		if (!enemies[i].is_alive)
			continue;
		if (enemy == &enemies[i])
			continue;
		if (abs(enemies[i].x - enemy->x) > WINDOW_WIDTH || abs(enemies[i].y - enemy->y) > WINDOW_HEIGHT)
			continue;
		if (enemies[i].r <= enemy->r)
			continue;
		if (enemies[i].r > enemy->r) {
			if (detect_circles_crash(enemies[i].x, enemies[i].y, enemies[i].r * 2, enemy->x, enemy->y, enemy->r)) {
				cur_enemy = &enemies[i];
			}
		}
	}

	if (player.r > enemy->r) {
		if (detect_circles_crash(player.x, player.y, player.r * 2, enemy->x - map.x, enemy->y - map.y, enemy->r)) {
			Ball temp;
			temp.x = map.x + player.x;
			temp.y = map.y + player.y;
			cur_enemy = &temp;
		}
	}

	if (cur_enemy != NULL) {
		// 做出规避
		enemy->dir.x = 0;
		enemy->dir.y = 0;

		if (enemy->x < cur_enemy->x && enemy->x - enemy->speed > enemy->r) {
			move_enemy_to_left(enemy);
		}

		if (enemy->x > cur_enemy->x && enemy->x + enemy->speed < MAP_WIDTH - enemy->r) {
			move_enemy_to_right(enemy);
		}

		if (enemy->y < cur_enemy->y && enemy->y - enemy->speed > enemy->r) {
			move_enemy_to_top(enemy);
		}

		if (enemy->y > cur_enemy->y && enemy->y + enemy->speed < MAP_HEIGHT - enemy->r) {
			move_enemy_to_bottom(enemy);
		}

		// 被逼到角落
		if (enemy->x - enemy->speed <= enemy->r || enemy->x + enemy->speed >= MAP_WIDTH - enemy->r) {
			// 水平方向
			if (enemy->y < cur_enemy->r * 2) {
				enemy->dir.y = 1;
				accelerate_ball(enemy);
			}
			else if (enemy->y > MAP_HEIGHT - cur_enemy->r * 2) {
				enemy->dir.y = -1;
				accelerate_ball(enemy);
			}
		}
		if (enemy->y - enemy->speed <= enemy->r || enemy->y + enemy->speed >= MAP_HEIGHT - enemy->r) {
			// 竖直方向
			if (enemy->x < cur_enemy->r * 2) {
				enemy->dir.x = 1;
				accelerate_ball(enemy);
			}
			else if (enemy->x > MAP_WIDTH - cur_enemy->r * 2) {
				enemy->dir.x = -1;
				accelerate_ball(enemy);
			}
		}
		return;
	}


	/*		以下是激进策略		*/
	if (enemy->target.x == 0 || enemy->target.y == 0) {
		enemy->target.x = rand() % ((int)(MAP_WIDTH / enemy->speed));
		enemy->target.y = rand() % ((int)(MAP_HEIGHT / enemy->speed));
	}

	if (enemy->target.x > 0) {
		if (enemy->target.x * enemy->speed > enemy->x)
			move_enemy_to_right(enemy);
		else
			move_enemy_to_left(enemy);
		enemy->target.x--;
	}

	if (enemy->target.y > 0) {
		if (enemy->target.y * enemy->speed > enemy->y)
			move_enemy_to_bottom(enemy);
		else
			move_enemy_to_top(enemy);
		enemy->target.y--;
	}
}

void update_enemies() {
	for (int i = 0; i < ENEMY_COUNT; i++) {
		Ball * cur = &enemies[i];
		if (!cur->is_alive) {
			continue;
		}
		/*int x = rand() % 3;
		int y = rand() % 3;
		if (x == 0) {
			move_enemy_to_left(cur);
		}
		else if (x == 1) {
			move_enemy_to_right(cur);
		}
		if (y == 0) {
			move_enemy_to_top(cur);
		}
		else if (y == 1) {
			move_enemy_to_bottom(cur);
		}*/
		enemy_move(cur);

		// 吃食物
		update_eat_food(cur);

		for (int j = 0; j < ENEMY_COUNT; j++) {
			if (i == j) {
				continue;
			}
			if (!enemies[j].is_alive) {
				continue;
			}
			update_two_ball_rel(&enemies[i], &enemies[j]);
		}

		// 更新动画属性
		if (is_animating(cur)) {
			cur->x += (int)cur->anim.x_v;
			cur->y += (int)cur->anim.y_v;
			cur->r += cur->anim.r_v;
			cur->anim.time += 1;
			if (cur->anim.time == ANIMATION_DURATION / WINDOW_REFRESH_DURATION) {
				reset_animator(cur);
			}
		}
	}
}

void update_player() {
	// 如果玩家死亡，则不再更新
	if (!player.is_alive) {
		return;
	}
	// 检测是否吃到了食物
	// 吃到了则再生成食物
	for (int i = 0; i < FOOD_COUNT; i++) {
		Food cur = feed[i];
		if (cur.x < map.x || cur.x > map.x + WINDOW_WIDTH) {
			continue;
		}
		if (cur.y < map.y || cur.y > map.y + WINDOW_HEIGHT) {
			continue;
		}
		if (detect_circles_crash(player.x, player.y, player.r, cur.x - map.x, cur.y - map.y, cur.r)) {
			float new_radius = player.r + cur.r * BALL_EAT_RATE;
			player.speed = player.speed * player.r / new_radius;
			if (player.speed < BALL_MIN_SPEED) player.speed = BALL_MIN_SPEED;
			player.r = new_radius;
			feed[i] = generate_food();
			continue;
		}
	}

	// 判断是否吃到了敌人
	for (int i = 0; i < ENEMY_COUNT; i++) {
		Ball * cur = &enemies[i];
		if (!cur->is_alive) {
			continue;
		}
		if (cur->x < map.x || cur->x > map.x + WINDOW_WIDTH) {
			continue;
		}
		if (cur->y < map.y || cur->y > map.y + WINDOW_HEIGHT) {
			continue;
		}
		update_player_enemy_rel(&player, &enemies[i]);
	}

	// 更新体积分数
	size_score = player.r;
}

void update_game() {
	// 更新玩家状态
	update_player();
	// 更新敌人状态
	update_enemies();
}

void move_ball_to_lt(float speed, int * map_dir, int * ball_dir, int ball_r, int map_border_size, int win_border_size) {
	if (*map_dir == map_border_size && *ball_dir != win_border_size / 2) {
		if (*ball_dir - speed >= win_border_size / 2) {
			*ball_dir -= speed;
			return;
		}
		// 此时已移动至中央
		speed -= *ball_dir - win_border_size / 2;
		*ball_dir = win_border_size / 2;
	}
	if (*map_dir - speed <= 0) {
		int player_move_distance = (int)(speed - *map_dir);
		*map_dir = 0;
		*ball_dir = *ball_dir - player_move_distance;
		if (*ball_dir - ball_r <= 0) {
			*ball_dir = ball_r;
		}
	}
	else
	{
		*map_dir = (int)(*map_dir - speed);
	}
}

void move_ball_to_rb(float speed, int * map_dir, int map_extreme_pos, int * ball_dir, int ball_extreme_pos, int ball_r, int map_border_size, int win_border_size) {
	if (*map_dir == map_border_size && *ball_dir != win_border_size / 2) {
		if (*ball_dir + speed <= win_border_size / 2) {
			*ball_dir += speed;
			return;
		}
		// 此时已移动至中央
		speed -= *ball_dir - win_border_size / 2;
		*ball_dir = win_border_size / 2;
	}
	if (*map_dir + speed >= map_extreme_pos) {
		int player_move_distance = (int)(speed - (map_extreme_pos - *map_dir));
		*map_dir = map_extreme_pos;
		*ball_dir = *ball_dir + player_move_distance;
		if (*ball_dir + ball_r >= ball_extreme_pos) {
			*ball_dir = ball_extreme_pos - ball_r;
		}
	}
	else
	{
		*map_dir = (int)(*map_dir + speed);
	}
}

void move_ball_to_top(Ball * ball) {
	move_ball_to_lt(ball->speed, &map.y, &ball->y, ball->r, MAP_HEIGHT - WINDOW_HEIGHT, WINDOW_HEIGHT);
}

void move_ball_to_bottom(Ball * ball) {
	move_ball_to_rb(ball->speed, &map.y, MAP_HEIGHT - WINDOW_HEIGHT, &ball->y, WINDOW_HEIGHT, ball->r, 0, WINDOW_HEIGHT);
}

void move_ball_to_left(Ball * ball) {
	move_ball_to_lt(ball->speed, &map.x, &ball->x, ball->r, MAP_WIDTH - WINDOW_WIDTH, WINDOW_WIDTH);
}

void move_ball_to_right(Ball * ball) {
	move_ball_to_rb(ball->speed, &map.x, MAP_WIDTH - WINDOW_WIDTH, &ball->x, WINDOW_WIDTH, ball->r, 0, WINDOW_WIDTH);
}

void move_enemy_to_top(Ball * ball) {
	int result = ball->y - ball->speed;
	ball->y = result > 0 ? result : 0;
}

void move_enemy_to_bottom(Ball * ball) {
	int result = ball->y + ball->speed;
	ball->y = result < MAP_HEIGHT ? result : MAP_HEIGHT;
}

void move_enemy_to_left(Ball * ball) {
	int result = ball->x - ball->speed;
	ball->x = result > 0 ? result : 0;
}

void move_enemy_to_right(Ball * ball) {
	int result = ball->x + ball->speed;
	ball->x = result < MAP_WIDTH ? result : MAP_WIDTH;
}

void reset_animator(Ball * ball)
{
	ball->anim.x_v = 0;
	ball->anim.y_v = 0;
	ball->anim.r_v = 0;
	ball->anim.time = 0;
}

bool is_animating(Ball * ball)
{
	return ball->anim.r_v != 0 || ball->anim.x_v != 0 || ball->anim.r_v != 0;
}

float generate_anim_attr(float attr)
{
	return attr * WINDOW_REFRESH_DURATION / ANIMATION_DURATION;
}

void accelerate_ball(Ball * ball) {
	// 将残骸部分替代成食物
	float new_child_r = ball->r * BALL_ADBANDON_RATE / BALL_EAT_RATE / 2;
	float new_r = ball->r * (1 - BALL_ADBANDON_RATE);

	int origin_x = ball->x;
	int origin_y = ball->y;

	int index = rand() % FOOD_COUNT;
	feed[index].r = new_child_r;
	feed[index].color = ball->color;

	ball->speed = ball->speed * ball->r / new_r;
	//ball->r = new_r;
	ball->anim.r_v = (new_r - ball->r) * WINDOW_REFRESH_DURATION / ANIMATION_DURATION;
	if (ball->dir.x == 0 && ball->dir.y == 0) {
		//ball->x += new_child_r;
		ball->anim.x_v = new_r * WINDOW_REFRESH_DURATION / ANIMATION_DURATION;
		feed[index].x = map.x + origin_x - ball->r * 2;
		feed[index].y = map.y + origin_y;
		printf("food x: %d y:%d r: %.2f\n", feed[index].x, feed[index].y, feed[index].r);
		return;
	}
	//ball->x += ball->dir.x * new_r;
	//ball->y += ball->dir.y * new_r;
	ball->anim.x_v = ball->dir.x * new_r * WINDOW_REFRESH_DURATION / ANIMATION_DURATION;
	ball->anim.y_v = ball->dir.y * new_r * WINDOW_REFRESH_DURATION / ANIMATION_DURATION;
	feed[index].x = map.x + origin_x - ball->r * 2 * ball->dir.x;
	feed[index].y = map.y + origin_y - ball->r * 2 * ball->dir.y;
}

void detect_key_event() {
	// 监听按键状态
	// 默认地图移动
	// 地图移动至边界则玩家移动

#if 0	// 不能同时处理多个按键事件
	char key = _getch();
	switch (key)
	{
	case 'w':
	case 'W':
	case 72:
		move_ball_to_top(&player);
		break;
	case 's':
	case 'S':
	case 80:
		move_ball_to_bottom(&player);
		break;
	case 'a':
	case 'A':
	case 75:
		move_ball_to_left(&player);
		break;
	case 'd':
	case 'D':
	case 77:
		move_ball_to_right(&player);
		break;
	}
#else // 可以同时处理多个按键，输入较平滑
	int dir_x = 0;
	int dir_y = 0;
	if (GetAsyncKeyState(VK_UP) & 0x8000 || GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState('w') & 0x8000) {
		move_ball_to_top(&player);
		dir_y = DIRECTION_TOP;
	}
	if (GetAsyncKeyState(VK_DOWN) & 0x8000 || GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState('s') & 0x8000) {
		move_ball_to_bottom(&player);
		dir_y = DIRECTION_BOTTOM;
	}
	if (GetAsyncKeyState(VK_LEFT) & 0x8000 || GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState('a') & 0x8000) {
		move_ball_to_left(&player);
		dir_x = DIRECTION_LEFT;
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000 || GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState('d') & 0x8000) {
		move_ball_to_right(&player);
		dir_x = DIRECTION_RIGHT;
	}
	// 此处我的机器不知道什么原因, & 0x8000 之后结果为 32768
	// 所以我在这里左移15位
	if (GetAsyncKeyState('O') & 0x8000 >> 15 || GetAsyncKeyState('o') & 0x8000 >> 15 || GetAsyncKeyState('0') & 0x8000 >> 15) {
		accelerate_ball(&player);
	}
	player.dir.x = dir_x;
	player.dir.y = dir_y;
#endif
}

void init() {
	// 生成随机数种子
	srand((unsigned int)time(NULL));

	// 默认玩家出生在中央
	player.x = WINDOW_WIDTH / 2;
	player.y = WINDOW_HEIGHT / 2;
	player.r = BALL_DEFAULT_RADIUS;
	player.color = RED;
	player.speed = BALL_DEFAULT_SPEED;
	player.is_alive = true;
	sprintf_s(player.name, "玩家");

	size_score = BALL_DEFAULT_SPEED;

	// 默认地图位于左上角
	map.x = 0;
	map.y = 0;

	// 初始化食物
	for (int i = 0; i < FOOD_COUNT; i++) {
		feed[i] = generate_food();
	}

	// 初始化敌人
	for (int i = 0; i < ENEMY_COUNT; i++) {
		enemies[i] = generate_enemy();
	}
}

int main() {
	// 初始化游戏窗口，带控制台，方便调试
	initgraph(WINDOW_WIDTH, WINDOW_HEIGHT, EW_SHOWCONSOLE);
	// 初始化相关属性
	init();
	BeginBatchDraw();
	while (1) {
		// 更新状态
		update_game();
		// 绘制游戏
		draw_game();

		// 检测按键事件
		if (_kbhit() && player.is_alive) {
			detect_key_event();
		}

		FlushBatchDraw();

		// 每10ms刷新一次
		Sleep(WINDOW_REFRESH_DURATION);
	}
	// 关闭窗口
	closegraph();
	return 0;
}