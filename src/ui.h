#ifndef XCRAFT_UI
#define XCRAFT_UI

#define BUTTON1_OFFSET 145
#define BUTTON2_OFFSET 148
#define BUTTON3_OFFSET 151
#define BUTTON4_OFFSET 154
#define BUTTON5_OFFSET 157
#define BUTTON6_OFFSET 193
#define BUTTON7_OFFSET 196

#include <stdint.h>

#include <bmpfont.h>

#define UI_SCALE 0.44f
#define UI_TITLE_SCALE 1.32f

struct button_t {
	float _x;
	float _y;
	float _scale;
	struct paragraph_t* backg;
	struct paragraph_t* backh;
	struct paragraph_t* forg;
	void (*highlighted_func) (void);
	void (*clicked_func) (void);
};

struct button_t* ui_create_button (int width, int height, float x, float y, float scale, uint8_t BACKGROUND, uint8_t BACKGROUND_HIGHLIGHTED, char* b_str, void (*h)(void), void (*c)(void));

struct button_t* ui_create_button_fit (float x, float y, float scale, uint8_t BACKGROUND, uint8_t BACKGROUND_HIGHLIGHTED, char* b_str, void (*h)(void), void (*c)(void));

void ui_destroy_button (struct button_t* b);

#endif
