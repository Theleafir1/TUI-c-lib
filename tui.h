#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#ifndef _TUI_H
#define _TUI_H

#define RED     (Color){255, 0, 0}
#define GREEN   (Color){0, 255, 0}
#define BLUE    (Color){0, 0, 255,}
#define BLACK   (Color){0, 0, 0,}

typedef struct
{
    uint8_t r, g, b;
} Color;

typedef struct
{
    char ch[4];
    Color fg, bg;
} Cell;

typedef struct
{
    Cell *inputBuffer;
    Cell *screenBuffer;
    int windowSize;
    int windowHeight;
    int windowWidth;
    unsigned int frameCounter;
    char *diff;
    int diffLength;
} TUIContext;

void tui_selectContext(TUIContext* context);

void tui_init(TUIContext* context);

void tui_enableRawMode();
void tui_disableRawMode();
void tui_hideCursor();
void tui_showCursor();

void tui_updateSize();
int  tui_getUTF8Len(unsigned char first_byte);

int  tui_getFrameNumber();

int  tui_getWindowHeight(); 
int  tui_getWindowWidth();
int  tui_getWindowSize();
char tui_update(); // bool
void tui_print();
void tui_fill(Cell symbol);

void tui_addchi(int idx, Cell ch);
char tui_addch(int x, int y, Cell ch);

void tui_cgotoi(int idx);
void tui_cgoto(int x, int y);

void tui_calculateDiff();

void tui_drawSquare(int posX, int posY, int sizeX, int sizeY, Cell symbol);

void tui_deinit();



#endif
