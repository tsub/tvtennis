#define LED_SZ 8

typedef unsigned char uchar;

/* ブザー用 */
enum {
	BEEP_LOW,
	BEEP_HIGH
};

/* システム定義 */
extern void _sound(uchar tone);
extern volatile uchar sw;
extern volatile uchar led[LED_SZ];

/* ユーザー定義 */
extern void user_init(void);
extern void user_main(void);
