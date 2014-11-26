/*
 * =====================================================================================
 *
 *       Filename:  user.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2014年11月26日 13時02分15秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  TSUBASA TAKAYAMA
 *   Organization:  Kanazawa Institute of Technology
 *
 * =====================================================================================
 */

#include "user.h"
#include <stdlib.h>
#include <avr/eeprom.h>
#include <util/delay.h>

/* ローカル関数 */
static void MovePlayer(void);
static void MoveBall(void);
static void Judge(void);
static void UpdateLED(void);
static void init_rand(void);
static void postions_init(void);

#define MAX_SCORE 5 // スコアの最大点
#define BALL_MOVE_TIME 3 // ボールが動く速さ
#define PLAYER_DOWN_TIME 2 // プレイヤーが下がる早さ
#define EEPADDR 0x000

/* グローバル変数 */
volatile uchar sw = 0; /* 押しボタン */
volatile uchar led[LED_SZ]; /* マトリクスLED */
int ball_wait = 0;
int player_wait = 0;
int judge_wait = 0;
int score_wait = 0;
int score_flag = 0;
int left_score[MAX_SCORE + 1][8] = {
	{
		0b00000000,
		0b11100000,
		0b10100000,
		0b10100000,
		0b10100000,
		0b11100000,
		0b00000000,
		0b00000000,
	},
	{
		0b00000000,
		0b11000000,
		0b01000000,
		0b01000000,
		0b01000000,
		0b11100000,
		0b00000000,
		0b00000000,
	},
	{
		0b00000000,
		0b11100000,
		0b00100000,
		0b01000000,
		0b10000000,
		0b11100000,
		0b00000000,
		0b00000000,
	},
	{
		0b00000000,
		0b11100000,
		0b00100000,
		0b11100000,
		0b00100000,
		0b11100000,
		0b00000000,
		0b00000000,
	},
	{
		0b00000000,
		0b10100000,
		0b10100000,
		0b11100000,
		0b00100000,
		0b00100000,
		0b00000000,
		0b00000000,
	},
	{
		0b00000000,
		0b11100000,
		0b10000000,
		0b11100000,
		0b00100000,
		0b11100000,
		0b00000000,
		0b00000000,
	},
};

int right_score[MAX_SCORE + 1][8] = {
	{
		0b00000000,
		0b00000111,
		0b00000101,
		0b00000101,
		0b00000101,
		0b00000111,
		0b00000000,
		0b00000000,
	},
	{
		0b00000000,
		0b00000110,
		0b00000010,
		0b00000010,
		0b00000010,
		0b00000111,
		0b00000000,
		0b00000000,
	},
	{
		0b00000000,
		0b00000111,
		0b00000001,
		0b00000010,
		0b00000100,
		0b00000111,
		0b00000000,
		0b00000000,
	},
	{
		0b00000000,
		0b00000111,
		0b00000001,
		0b00000111,
		0b00000001,
		0b00000111,
		0b00000000,
		0b00000000,
	},
	{
		0b00000000,
		0b00000101,
		0b00000101,
		0b00000111,
		0b00000001,
		0b00000001,
		0b00000000,
		0b00000000,
	},
	{
		0b00000000,
		0b00000111,
		0b00000100,
		0b00000111,
		0b00000001,
		0b00000111,
		0b00000000,
		0b00000000,
	},
};

typedef struct Pos {
	uchar m; // 行
	uchar n; // 列
} Pos;

/* ローカル変数 */

// 左プレイヤーの情報
static struct {
	uchar pos1; // 左プレイヤーの板の位置（上側）
	uchar pos2; // 左プレイヤーの板の位置（下側）
	int score; // 左プレイヤーのスコア
} left_player;

// 右プレイヤーの情報
static struct {
	uchar pos1; // 右プレイヤーの板の位置（上側）
	uchar pos2; // 右プレイヤーの板の位置（下側）
	int score; // 右プレイヤーのスコア
} right_player;

// ボールの情報
static struct {
	struct Pos pos; // ボールの位置
	int dir; // ボールの方向
} ball;

/* ユーザー処理の初期化 */
void user_init(void) {
	left_player.pos1 = 1;
	left_player.pos2 = 0;
	left_player.score = 0;

	right_player.pos1 = 1;
	right_player.pos2 = 0;
	right_player.score = 0;

	score_flag = 0;

	init_rand();

	ball.pos.m = rand() & 0x07;
	ball.pos.n = 0x04;
	do {
		ball.dir = rand() % 7;
	} while(ball.dir == 0 || ball.dir == 4 || ball.dir == 2 || ball.dir == 6);
}

void postions_init(void) {
	left_player.pos1 = 1;
	left_player.pos2 = 0;

	right_player.pos1 = 1;
	right_player.pos2 = 0;

	init_rand();

	ball.pos.m = rand() & 0x07;
	ball.pos.n = 0x04;
	do {
		ball.dir = rand() % 7;
	} while(ball.dir == 0 || ball.dir == 4 || ball.dir == 2 || ball.dir == 6);
}

/* ユーザー処理(100ms毎に呼ばれる) */
void user_main(void) {
	if(!score_flag) {
		MovePlayer();
		MoveBall();
	}

	UpdateLED();
	
	if(!score_flag) {
		Judge();
	}
}

// ボールの移動
static void MoveBall() {
	if(ball_wait >= BALL_MOVE_TIME) {
		switch(ball.dir) {
			case 0:
				if(ball.pos.m < 7) {
					ball.pos.m++;
				}

				if(ball.pos.m >= 7) {
					ball.dir = 4;
				}
				break;
			case 1:
				if(ball.pos.m < 7) {
					ball.pos.m++;
				}
				if(ball.pos.n > 0) {
					ball.pos.n--;
				}

				// 壁との衝突
				if(ball.pos.m >= 7) {
					ball.dir = 3;
				}
				else if(ball.pos.n <= 0) {
					ball.dir = 7;
				}

				// プレイヤーとの衝突
				if(ball.pos.m == right_player.pos1 || ball.pos.m == right_player.pos2) {
					if(ball.pos.n == 1) {
						ball.dir = 7;
						_sound(BEEP_LOW);
					}
				}

				if(ball.pos.m == 7 && ball.pos.n == 1) {
					if(ball.dir == 7) {
						ball.dir = 5;
					}
				}

				break;
			case 2:
				if(ball.pos.n > 0) {
					ball.pos.n--;
				}

				// 壁との衝突
				if(ball.pos.n <= 0) {
					ball.dir = 6;
				}

				// プレイヤーとの衝突
				if(ball.pos.m == right_player.pos1 || ball.pos.m == right_player.pos2) {
					if(ball.pos.n == 1) {
						ball.dir = 6;
						_sound(BEEP_LOW);
					}
				}

				break;
			case 3:
				if(ball.pos.m > 0) {
					ball.pos.m--;
				}
				if(ball.pos.n > 0) {
					ball.pos.n--;
				}

				// 壁との衝突
				if(ball.pos.m <= 0) {
					ball.dir = 1;
				}
				else if(ball.pos.n <= 0) {
					ball.dir = 5;
				}

				// プレイヤーとの衝突
				if(ball.pos.m == right_player.pos1 || ball.pos.m == right_player.pos2) {
					if(ball.pos.n == 1) {
						ball.dir = 5;
						_sound(BEEP_LOW);
					}
				}

				if(ball.pos.m == 0 && ball.pos.n == 1) {
					if(ball.dir == 5) {
						ball.dir = 7;
					}
				}

				break;
			case 4:
				if(ball.pos.m > 0) {
					ball.pos.m--;
				}

				if(ball.pos.m <= 0) {
					ball.dir = 0;
				}

				break;
			case 5:
				if(ball.pos.m > 0) {
					ball.pos.m--;
				}
				if(ball.pos.n < 7) {
					ball.pos.n++;
				}

				// 壁との衝突
				if(ball.pos.m <= 0) {
					ball.dir = 7;
				}
				else if(ball.pos.n >= 7) {
					ball.dir = 3;
				}

				// プレイヤーとの衝突
				if(ball.pos.m == left_player.pos1 || ball.pos.m == left_player.pos2) {
					if(ball.pos.n == 6) {
						ball.dir = 3;
						_sound(BEEP_LOW);
					}
				}

				if(ball.pos.m == 0 && ball.pos.n == 6) {
					if(ball.dir == 3) {
						ball.dir = 1;
					}
				}

				break;
			case 6:
				if(ball.pos.n < 7) {
					ball.pos.n++;
				}

				// 壁との衝突
				if(ball.pos.n >= 7) {
					ball.dir = 2;
				}

				// プレイヤーとの衝突
				if(ball.pos.m == left_player.pos1 || ball.pos.m == left_player.pos2) {
					if(ball.pos.n == 6) {
						ball.dir = 2;
						_sound(BEEP_LOW);
					}
				}

				break;
			case 7:
				if(ball.pos.m < 7) {
					ball.pos.m++;
				}
				if(ball.pos.n < 7) {
					ball.pos.n++;
				}

				// 壁との衝突
				if(ball.pos.m >= 7) {
					ball.dir = 5;
				}
				else if(ball.pos.n >= 7) {
					ball.dir = 1;
				}

				// プレイヤーとの衝突
				if(ball.pos.m == left_player.pos1 || ball.pos.m == left_player.pos2) {
					if(ball.pos.n == 6) {
						ball.dir = 1;
						_sound(BEEP_LOW);
					}
				}

				if(ball.pos.m == 7 && ball.pos.n == 6) {
					if(ball.dir == 1) {
						ball.dir = 3;
					}
				}

				break;
			default:
				break;
		}
		ball_wait = 0;
	}
	else {
		ball_wait++;
	}
}

// プレイヤーの移動
static void MovePlayer() {
	if(sw == 1 || sw == 3) {
		if(left_player.pos1 < 7) {
			left_player.pos1++;
			left_player.pos2++;
		}
	}
	if(sw == 2 || sw == 3) {
		if(right_player.pos1 < 7) {
			right_player.pos1++;
			right_player.pos2++;
		}
	}

	// 300ms毎にプレイヤーが自動で下がる
	if(player_wait >= PLAYER_DOWN_TIME) {
		if(sw == 0 || sw == 2) {
			if(left_player.pos2 > 0) {
				left_player.pos1--;
				left_player.pos2--;
			}
			player_wait = 0;
		}
		if(sw == 0 || sw == 1) {
			if(right_player.pos2 > 0) {
				right_player.pos1--;
				right_player.pos2--;
			}
			player_wait = 0;
		}
	}
	else {
		player_wait++;
	}
}

// 得点が入ったかどうかの判定
static void Judge() {
	if(judge_wait >= BALL_MOVE_TIME) {
		if(ball.pos.n == 0) {
			_sound(BEEP_HIGH);
			left_player.score++;
			score_flag = 1;
		}
		else if(ball.pos.n == 7) {
			_sound(BEEP_HIGH);
			right_player.score++;
			score_flag = 1;
		}
		judge_wait = 0;
	}
	else {
		judge_wait++;
	}
}

/* LED表示の更新 */
static void UpdateLED(void) {
	uchar i;

	// 画面の初期化
	for(i = 0; i < LED_SZ; i++) {
		led[i] = 0x00;
	}

	if(score_flag) {
		for(i = 0; i < LED_SZ; i++) {
			led[LED_SZ - 1 - i] |= left_score[left_player.score][i];
			led[LED_SZ - 1 - i] |= right_score[right_player.score][i];
		}

		score_wait++;
		if(score_wait >= 10) {
			postions_init();
			if(left_player.score >= MAX_SCORE || right_player.score >= MAX_SCORE) {
				while(1);
			}
			score_flag = 0;
			score_wait = 0;
		}
	}
	else {
		// プレイヤー表示
		led[left_player.pos1] |= (uchar)0x80;
		led[left_player.pos2] |= (uchar)0x80;
		led[right_player.pos1] |= (uchar)0x01;
		led[right_player.pos2] |= (uchar)0x01;

		// ボール表示
		led[ball.pos.m] |= (uchar)(0x01 << ball.pos.n);
	}
}

// eepromでの乱数初期化
static void init_rand() {
	srand(eeprom_read_word((uint16_t*)EEPADDR));
	eeprom_write_word((uint16_t*)EEPADDR, rand());
}
