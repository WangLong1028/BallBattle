#include <stdio.h>
#include <Windows.h>
#include <graphics.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include <math.h>

#define WINDOW_WIDTH 1024	// ���ڿ��
#define WINDOW_HEIGHT 600	// ���ڸ߶�
#define WINDOW_REFRESH_DURATION 10 // ����ˢ��Ƶ��
#define MAP_WIDTH 4096	// ��ͼ���
#define MAP_HEIGHT 2400	// ��ͼ�߶�
#define GRID_SIZE 10 // �������Ӵ�С

#define DIRECTION_LEFT -1 // �˶�����
#define DIRECTION_RIGHT 1
#define DIRECTION_TOP -1
#define DIRECTION_BOTTOM 1

#define BALL_DEFAULT_RADIUS 30 // ��ĳ�ʼ��С
#define BALL_DEFAULT_SPEED 6 // ��ĳ�ʼ�ٶ�
#define BALL_MIN_SPEED 0.5f // �����С�ٶ�
#define BALL_EAT_RATE 0.2f // ʳ��뾶ת����
#define BALL_ADBANDON_RATE 0.2 // ���������Ĳ���

#define ANIMATION_DURATION 100 // ����ʱ��

#define FOOD_COUNT 1200	//��Ϸ��ʳ������
#define FOOD_MAX_RADIUS 2 // ʳ������С

#define ENEMY_COUNT 20 // ��������

// map����
typedef struct Map {
	int x; // ��ͼ��x����
	int y; // ��ͼ��y����
} Map;

// �˶�����
typedef struct Direction {
	int x;
	int y;
} Direction;

// �򶯻�����
typedef struct BallAnimator {
	int x_v;
	int y_v;
	float r_v;
	int time;
}BallAnimator;

// ���Ŀ�ĵ�
typedef struct BallTarget {
	int x;
	int y;
}BallTarget;

// ������
typedef struct Ball {
	int x; // x����
	int y; // y����
	float r; // �뾶
	float speed; // �ٶ�
	DWORD color; // ����ɫ
	char name[20]; // ����
	bool is_alive; // �Ƿ����
	Direction dir; // �˶�����
	BallTarget target; // �˶�Ŀ��
	BallAnimator anim; // ����������
} Ball;

// ʳ������
typedef struct Food {
	int x;
	int y;
	float r;
	DWORD color; // ʳ����ɫ
} Food;



Map map; // ��ͼ
Ball player;// ���
Food feed[FOOD_COUNT]; // ʳ��
Ball enemies[ENEMY_COUNT]; // ����

int size_score; // �������

// ����ƶ�����
void move_ball_to_top(Ball * ball);
void move_ball_to_bottom(Ball * ball);
void move_ball_to_left(Ball * ball);
void move_ball_to_right(Ball * ball);

// ���˵��ƶ�����
void move_enemy_to_top(Ball * ball);
void move_enemy_to_bottom(Ball * ball);
void move_enemy_to_left(Ball * ball);
void move_enemy_to_right(Ball * ball);

void reset_animator(Ball * ball); // ���趯��
bool is_animating(Ball * ball); // �ж����Ƿ��ж�������
float generate_anim_attr(float attr); // ���ɶ�������

// ��ļ��ٺ���
void accelerate_ball(Ball * ball);

Food generate_food(); // ʳ�����ɺ���
Ball generate_enemy(); // �������ɺ���

void update_eat_food(Ball * ball); // ����ʳ�����
void eat_ball(Ball * L, Ball * S);
void update_player_enemy_rel(Ball * player, Ball * enemy); // �ж���Һ͵����Ƿ���ײ
void update_two_ball_rel(Ball * b1, Ball * b2); // ����������Ƿ���Լ�λ�ù�ϵ

void enemy_move(Ball * enemy); // ��������Ӧ����ô����

void draw_background() {
	// ����Ļ��Χ�ڻ��ư�ɫ����
	setfillcolor(WHITE);
	solidrectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	// ����Ļ��Χ���ڻ��Ƹ���
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
	// ֻ������Ļ��Χ�ڵ�ʳ��
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
	// ֻ��������Ļ��Χ�ڵĵ���
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

	// �����������
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
	sprintf_s(text, "��ǰ����: %d", size_score);
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 24;
	settextcolor(RGB(0, 0, 0));
	_tcscpy_s(f.lfFaceName, _T("����"));
	f.lfQuality = ANTIALIASED_QUALITY;
	settextstyle(&f);
	setbkmode(TRANSPARENT);
	int text_x = WINDOW_WIDTH - 20 - textwidth(text);
	int text_y = 20;
	outtextxy(text_x, text_y, text);
}

void draw_game() {
	// ���Ʊ���
	draw_background();
	// ����ʳ��
	draw_feed();
	// ���Ƶ���
	draw_enemies();
	// �������
	draw_player();
	// �����������
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
	/*		�����Ǳ��ز���		*/
	// ������Լ�����в����
	// ���㹫ʽ: �Է��뾶/�Լ��뾶 + ��Ļ�뾶 ^ 2 / �Է����� ^ 2
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
		// �������
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

		// ���Ƶ�����
		if (enemy->x - enemy->speed <= enemy->r || enemy->x + enemy->speed >= MAP_WIDTH - enemy->r) {
			// ˮƽ����
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
			// ��ֱ����
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


	/*		�����Ǽ�������		*/
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

		// ��ʳ��
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

		// ���¶�������
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
	// ���������������ٸ���
	if (!player.is_alive) {
		return;
	}
	// ����Ƿ�Ե���ʳ��
	// �Ե�����������ʳ��
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

	// �ж��Ƿ�Ե��˵���
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

	// �����������
	size_score = player.r;
}

void update_game() {
	// �������״̬
	update_player();
	// ���µ���״̬
	update_enemies();
}

void move_ball_to_lt(float speed, int * map_dir, int * ball_dir, int ball_r, int map_border_size, int win_border_size) {
	if (*map_dir == map_border_size && *ball_dir != win_border_size / 2) {
		if (*ball_dir - speed >= win_border_size / 2) {
			*ball_dir -= speed;
			return;
		}
		// ��ʱ���ƶ�������
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
		// ��ʱ���ƶ�������
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
	// ���к����������ʳ��
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
	// ��������״̬
	// Ĭ�ϵ�ͼ�ƶ�
	// ��ͼ�ƶ����߽�������ƶ�

#if 0	// ����ͬʱ�����������¼�
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
#else // ����ͬʱ�����������������ƽ��
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
	// �˴��ҵĻ�����֪��ʲôԭ��, & 0x8000 ֮����Ϊ 32768
	// ����������������15λ
	if (GetAsyncKeyState('O') & 0x8000 >> 15 || GetAsyncKeyState('o') & 0x8000 >> 15 || GetAsyncKeyState('0') & 0x8000 >> 15) {
		accelerate_ball(&player);
	}
	player.dir.x = dir_x;
	player.dir.y = dir_y;
#endif
}

void init() {
	// �������������
	srand((unsigned int)time(NULL));

	// Ĭ����ҳ���������
	player.x = WINDOW_WIDTH / 2;
	player.y = WINDOW_HEIGHT / 2;
	player.r = BALL_DEFAULT_RADIUS;
	player.color = RED;
	player.speed = BALL_DEFAULT_SPEED;
	player.is_alive = true;
	sprintf_s(player.name, "���");

	size_score = BALL_DEFAULT_SPEED;

	// Ĭ�ϵ�ͼλ�����Ͻ�
	map.x = 0;
	map.y = 0;

	// ��ʼ��ʳ��
	for (int i = 0; i < FOOD_COUNT; i++) {
		feed[i] = generate_food();
	}

	// ��ʼ������
	for (int i = 0; i < ENEMY_COUNT; i++) {
		enemies[i] = generate_enemy();
	}
}

int main() {
	// ��ʼ����Ϸ���ڣ�������̨���������
	initgraph(WINDOW_WIDTH, WINDOW_HEIGHT, EW_SHOWCONSOLE);
	// ��ʼ���������
	init();
	BeginBatchDraw();
	while (1) {
		// ����״̬
		update_game();
		// ������Ϸ
		draw_game();

		// ��ⰴ���¼�
		if (_kbhit() && player.is_alive) {
			detect_key_event();
		}

		FlushBatchDraw();

		// ÿ10msˢ��һ��
		Sleep(WINDOW_REFRESH_DURATION);
	}
	// �رմ���
	closegraph();
	return 0;
}