#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include "user.h"

static volatile unsigned char scan; /* LED操作カウンタ */
static volatile unsigned char clk; /* 間引き用クロック */

/* タイマ割り込み */
ISR(TIMER0_COMPA_vect) {
	unsigned char sc;

	/* 2ms毎にLED操作 */
	PORTB = 0;
	scan = (scan + 1) & 7;
	sc = ~(1 << scan);

	PORTC = 0x30 | (sc & 0x0f); /* sw1, 2をプルアップ */
	PORTD = sc & 0xf0;
	PORTB = led[scan];

	if(++clk >= 50) { /* 100ms毎に起動 */
		clk = 0;
		sw = (~PINC >> 4) & 3; /* スイッチ読み */
		TCCR2A = 0; /* ブザー停止 */
		user_main();
	}
}

int main(void) {
	/* LED, スイッチ, ブザーのポート設定 */
	DDRB = 0xff;
	DDRC = 0x0f;
	DDRD = 0xfa;

	/* タイマ割り込み設定 */
	OCR0A = 249; /* 2ms */
	TCCR0A = 2;
	TCCR0B = 3; /* 1/64 */
	TIMSK0 |= (1 << OCIE0A);

	/* ブザー用タイマ設定 */
	TCCR2B = 0x44; /* 1/64 */
	TCCR2A = 0;

	/* ゲーム初期化 */
	user_init();
	sei();
	for(;;) {
		wdt_reset();
	}

	return 0;
}

/* ブザー開始 */
void _sound(uchar tone) {
	OCR2A = tone == BEEP_LOW ? 169 : 42;
   TCCR2A = 0x12;	
}
