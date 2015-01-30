/*
 * =====================================================================================
 *
 *       	Title:  tvtennis
 *
 *   class number:  3EP1-33
 *   	   Author:  Tsubasa Takayama
 *   Organization:  Kanazawa Institute of Technology
 *
 *        Version:  1.0
 *        Created:  2014年11月26日 13時02分15秒
 *       Revision:  Copyright(c) 2013 Tsubasa Takayama, Kanazawa Institue of Technology
 *       															All rights reserved.
 *       Compiler:  gcc
 *
 *   	 FileName:  user.c
 *
 * =====================================================================================
 */

#include "user.h"
#include <stdlib.h>
#include <avr/eeprom.h>

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
int ball_wait = 0; // ボールが動くまでの時間待ち
int player_wait = 0; // プレイヤーが下がるまでの時間待ち
int judge_wait = 0; // 得点判定をするまでの時間待ち
int score_wait = 0; // スコアを表示し続ける時間待ち
int score_flag = 0; // スコアを表示するかどうか
int left_score[MAX_SCORE + 1][8] = { // 左プレイヤーの得点表示用ビット
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

int right_score[MAX_SCORE + 1][8] = { // 右プレイヤーの得点表示用ビット
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
	left_player.pos1 = 1; // 左プレイヤーの板の位置（上側）の初期化
	left_player.pos2 = 0; // 左プレイヤーの板の位置（下側）の初期化
	left_player.score = 0; // 左プレイヤーのスコアの初期化

	right_player.pos1 = 1; // 右プレイヤーの板の位置（上側）の初期化
	right_player.pos2 = 0; // 右プレイヤーの板の位置（下側）の初期化
	right_player.score = 0; // 右プレイヤーのスコアの初期化

	score_flag = 0; // スコアを表示するかのフラグ初期化

	init_rand(); // eepromによる擬似乱数の初期化

	ball.pos.m = rand() & 0x07; // ボールの縦の開始位置を乱数で初期化
	ball.pos.n = 0x04; // ボールの横の開始位置を初期化（固定値）
	do {
		ball.dir = rand() % 7; // ボールの移動方向を乱数で初期化
	} while(ball.dir == 0 || ball.dir == 4 || ball.dir == 2 || ball.dir == 6); // ボールの方向が左右上下だった場合は乱数再取得
}

void postions_init(void) { // スコア以外を初期化
	left_player.pos1 = 1; // 左プレイヤーの板の位置（上側）の初期化
	left_player.pos2 = 0; // 左プレイヤーの板の位置（下側）の初期化

	right_player.pos1 = 1; // 右プレイヤーの板の位置（上側）の初期化
	right_player.pos2 = 0; // 右プレイヤーの板の位置（下側）の初期化

	init_rand(); // eepromによる擬似乱数の初期化

	ball.pos.m = rand() & 0x07; // ボールの縦の開始位置を乱数で初期化
	ball.pos.n = 0x04; // ボールの横の開始位置を初期化（固定値）
	do {
		ball.dir = rand() % 7; // ボールの移動方向を乱数で初期化
	} while(ball.dir == 0 || ball.dir == 4 || ball.dir == 2 || ball.dir == 6); // ボールの方向が左右上下だった場合は乱数再取得
}

/* ユーザー処理(100ms毎に呼ばれる) */
void user_main(void) {
	if(!score_flag) { // スコア表示フラグが立っていない場合
		MovePlayer(); // プレイヤーの移動
		MoveBall(); // ボールの移動
	}

	UpdateLED(); // LEDの表示
	
	if(!score_flag) { // スコア表示フラグが立っていない場合
		Judge(); // 得点の判定
	}
}

// ボールの移動
static void MoveBall() {
	// 300ms毎にボールが移動
	ball_wait++;
	if(ball_wait >= BALL_MOVE_TIME) {
		switch(ball.dir) { // ボールの移動方向に応じた処理
			case 0: // 上に移動時（今回は使わない）
				// ボールの移動
				if(ball.pos.m < 7) {
					ball.pos.m++;
				}

				// 壁との衝突
				if(ball.pos.m >= 7) {
					ball.dir = 4;
				}
				break;
			case 1: // 右上に移動時
				// ボールの移動
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

				// 角にボールがハマってしまった時の処理
				if(ball.pos.m == 7 && ball.pos.n == 1) {
					if(ball.dir == 7) {
						ball.dir = 5;
					}
				}

				break;
			case 2: // 右に移動時（今回は使わない）
				// ボールの移動
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
			case 3: // 右下に移動時
				// ボールの移動
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

				// 角にボールがハマってしまった時の処理
				if(ball.pos.m == 0 && ball.pos.n == 1) {
					if(ball.dir == 5) {
						ball.dir = 7;
					}
				}

				break;
			case 4: // 下に移動時（今回は使わない）
				// ボールの移動
				if(ball.pos.m > 0) {
					ball.pos.m--;
				}

				if(ball.pos.m <= 0) {
					ball.dir = 0;
				}

				break;
			case 5: // 左下に移動時
				// ボールの移動
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

				// 角にボールがハマってしまった時の処理
				if(ball.pos.m == 0 && ball.pos.n == 6) {
					if(ball.dir == 3) {
						ball.dir = 1;
					}
				}

				break;
			case 6: // 左に移動時（今回は使わない）
				// ボールの移動
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
			case 7: // 左上に移動時
				// ボールの移動
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

				// 角にボールがハマってしまった時の処理
				if(ball.pos.m == 7 && ball.pos.n == 6) {
					if(ball.dir == 1) {
						ball.dir = 3;
					}
				}

				break;
			default:
				break;
		}
		ball_wait = 0; // ボール移動待ち状態に戻す
	}
}

// プレイヤーの移動
static void MovePlayer() {
	if(sw == 1 || sw == 3) { // 左のボタンが押されている時
		if(left_player.pos1 < 7) {
			left_player.pos1++; // 左プレイヤーを上昇
			left_player.pos2++;
		}
	}
	if(sw == 2 || sw == 3) { // 右のボタンが押されている時
		if(right_player.pos1 < 7) {
			right_player.pos1++; // 右プレイヤーを上昇
			right_player.pos2++;
		}
	}

	// 200ms毎にプレイヤーが自動で下がる
	player_wait++;
	if(player_wait >= PLAYER_DOWN_TIME) {
		if(sw == 0 || sw == 2) { // 左のボタンが押されていない時
			if(left_player.pos2 > 0) {
				left_player.pos1--; // 左プレイヤーを下降
				left_player.pos2--;
			}
			player_wait = 0;
		}
		if(sw == 0 || sw == 1) { // 右のボタンが押されていない時
			if(right_player.pos2 > 0) {
				right_player.pos1--; // 右プレイヤーを下降
				right_player.pos2--;
			}
			player_wait = 0;
		}
	}
}

// 得点が入ったかどうかの判定
static void Judge() {
	// 300ms毎（ボールと同じタイミング）で得点判定
	if(judge_wait >= BALL_MOVE_TIME) {
		if(ball.pos.n == 0) {
			_sound(BEEP_HIGH); // 高音を鳴らす
			left_player.score++; // 左プレイヤーのスコアを加算
			score_flag = 1; // スコアを表示するフラグを立てる
		}
		else if(ball.pos.n == 7) {
			_sound(BEEP_HIGH); // 高音を鳴らす
			right_player.score++; // 右プレイヤーのスコアを加算
			score_flag = 1; // スコアを表示するフラグを立てる
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

	if(score_flag) { // スコア表示フラグが立っている時
		for(i = 0; i < LED_SZ; i++) { 
			led[LED_SZ - 1 - i] |= left_score[left_player.score][i]; // 左プレイヤーの得点を表示
			led[LED_SZ - 1 - i] |= right_score[right_player.score][i]; // 右プレイヤーの得点を表示
		}

		// 1000msまでスコアを表示
		score_wait++;
		if(score_wait >= 10) {
			postions_init(); // プレイヤーとボールの位置を初期化
			
			// どちらかのプレイヤーが最大スコアに達した時
			if(left_player.score >= MAX_SCORE || right_player.score >= MAX_SCORE) {
				while(1); // 無限ループでマイコンリセット
			}
			score_flag = 0; // スコア表示フラグを0にする
			score_wait = 0;
		}
	}
	else {
		// プレイヤー表示
		led[left_player.pos1] |= (uchar)0x80; // 左プレイヤーの板の位置（上側）にled表示
		led[left_player.pos2] |= (uchar)0x80; // 左プレイヤーの板の位置（下側）にled表示
		led[right_player.pos1] |= (uchar)0x01; // 右プレイヤーの板の位置（上側）にled表示
		led[right_player.pos2] |= (uchar)0x01; // 右プレイヤーの板の位置（下側）にled表示

		// ボール表示
		led[ball.pos.m] |= (uchar)(0x01 << ball.pos.n);
	}
}

// eepromでの乱数初期化
static void init_rand() {
	srand(eeprom_read_word((uint16_t*)EEPADDR));
	eeprom_write_word((uint16_t*)EEPADDR, rand());
}
