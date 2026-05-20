#include "tui.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

void tui_enableRawMode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
void tui_disableRawMode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag |= (ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
void tui_hideCursor() {
    printf("\033[?25l");
    fflush(stdout);
}

void tui_showCursor() {
    printf("\033[?25h");
    fflush(stdout);
}

static TUIContext* ctx = NULL;
void tui_selectContext(TUIContext* context)
{
    ctx = context;
}


int tui_getFrameNumber() {return ctx->frameCounter;}
void tui_calculateDiff()
{
    Color lastBg = (Color){1, 2, 3};
    Color lastFg = (Color){3, 2, 1};
    int lastIndex = 0;
    ctx->diffLength = 0;
    for(int i = 0; i < ctx->windowSize; i++)
    {
        if (memcmp(&ctx->inputBuffer[i], &ctx->screenBuffer[i], sizeof(Cell)) == 0) continue;
        if (lastIndex != i)
        {
            char move[32];
            int moveLen = sprintf(move, "\033[%d;%dH", i / ctx->windowWidth + 1, i % ctx->windowWidth + 1);
            ctx->diff = (char *)realloc(ctx->diff, ctx->diffLength + moveLen);
            memcpy(&ctx->diff[ctx->diffLength], move, moveLen);
            ctx->diffLength += moveLen;
            lastIndex = i;
        }
        if ( (memcmp(&ctx->inputBuffer[i].bg, &ctx->screenBuffer[i].bg, sizeof(Color)) != 0) || memcmp(&ctx->inputBuffer[i].bg, &lastBg, sizeof(Color)) != 0)
        {
            char bgString[20];
            int len = sprintf(bgString, "\033[48;2;%d;%d;%dm", ctx->inputBuffer[i].bg.r, ctx->inputBuffer[i].bg.g, ctx->inputBuffer[i].bg.b);
            lastBg = ctx->inputBuffer[i].bg;
            ctx->diff = (char *)realloc(ctx->diff, ctx->diffLength + len);
            memcpy(&ctx->diff[ctx->diffLength], bgString, len);
            ctx->diffLength += len;
        }
        if ( (memcmp(&ctx->inputBuffer[i].fg, &ctx->screenBuffer[i].fg, sizeof(Color)) != 0) || memcmp(&ctx->inputBuffer[i].fg, &lastFg, sizeof(Color)) != 0)
        {
            char fgString[20];
            int len = sprintf(fgString, "\033[38;2;%d;%d;%dm", ctx->inputBuffer[i].fg.r, ctx->inputBuffer[i].fg.g, ctx->inputBuffer[i].fg.b);
            lastFg = ctx->inputBuffer[i].fg;
            ctx->diff = (char *)realloc(ctx->diff, ctx->diffLength + len);
            memcpy(&ctx->diff[ctx->diffLength], fgString, len);
            ctx->diffLength += len;
        }
        if (memcmp(ctx->inputBuffer[i].ch, ctx->screenBuffer[i].ch, 4))
        {
            int len = tui_getUTF8Len((unsigned char)ctx->inputBuffer[i].ch[0]);
            ctx->diff = (char *)realloc(ctx->diff, ctx->diffLength + len);
            memcpy(&ctx->diff[ctx->diffLength], &ctx->inputBuffer[i].ch , len);
            ctx->diffLength += len;
        }
    }
}
void tui_updateSize()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    ctx->windowHeight = w.ws_row;
    ctx->windowWidth = w.ws_col;
    ctx->windowSize = ctx->windowHeight * ctx->windowWidth;
}
int tui_getUTF8Len(unsigned char firstByte) 
    {
        if (firstByte < 128) return 1;
        if (firstByte < 224) return 2;
        if (firstByte < 240) return 3;
        return 4;
    }
char tui_update() // returns true if screen was resized
{
    char wasResized = 0; // false
    int oldSize = ctx->windowSize;
    tui_updateSize();
    if (ctx->windowSize != oldSize)
    {
        ctx->inputBuffer = (Cell *)realloc(ctx->inputBuffer, ctx->windowSize * sizeof(Cell));
        ctx->screenBuffer = (Cell *)realloc(ctx->screenBuffer, ctx->windowSize * sizeof(Cell));
        wasResized = 1; // true
    }
    return wasResized;
}

int tui_getWindowHeight() {return ctx->windowHeight;}
int tui_getWindowWidth() {return ctx->windowWidth;}
int tui_getWindowSize() {return ctx->windowSize;}
void tui_print()
{
    if (!ctx->frameCounter)
    {
        printf("\033[2J\033[H");
    }
    ctx->frameCounter++;
    tui_calculateDiff();
    fwrite(ctx->diff, sizeof(char), ctx->diffLength, stdout);
    fflush(stdout);
    memcpy(ctx->screenBuffer, ctx->inputBuffer, ctx->windowSize * sizeof(Cell));
}
void tui_fill(Cell symbol)
{
    for(int i = 0; i < ctx->windowSize; i++)
    {
        ctx->inputBuffer[i] = symbol;
    }
}
void tui_init(TUIContext* context)
{
    tui_selectContext(context);
    ctx->frameCounter = 0;
    tui_updateSize();
    ctx->inputBuffer = NULL;
    ctx->inputBuffer = (Cell *)malloc(ctx->windowSize * sizeof(Cell));
    ctx->screenBuffer = NULL;
    ctx->screenBuffer = (Cell *)calloc(ctx->windowSize, sizeof(Cell));
    tui_fill((Cell){" ", BLUE, BLUE});
    ctx->diff = (char *)malloc(ctx->windowSize / 2 * sizeof(Cell));
    tui_enableRawMode();
    tui_hideCursor();
}
void tui_deinit()
{
    free(ctx->inputBuffer);
        ctx->inputBuffer = NULL;
    free(ctx->screenBuffer);
        ctx->screenBuffer = NULL;
    tui_disableRawMode();
    tui_showCursor();
}

void tui_addchi(int idx, Cell ch)
{
    if (idx >= tui_getWindowSize()) return;
    ctx->inputBuffer[idx] = ch;
}
char tui_addch(int x, int y, Cell ch)
{
    int idx = y * tui_getWindowWidth() + x;
    if (idx >= tui_getWindowSize()) return 1;
    ctx->inputBuffer[idx] = ch;
    return 0;
}
void tui_cgotoi(int idx)
{
    int y = idx / tui_getWindowWidth(); 
    int x = idx % tui_getWindowWidth();
    printf("\033[%d;%dH", y+1, x+1);
}
void tui_cgoto(int x, int y)
{
    printf("\033[%d;%dH", y+1, x+1);
}
void tui_drawSquare(int posX, int posY, int sizeX, int sizeY, Cell symbol)
{
    for (int y = 0; y < sizeY; y++)
    {
        tui_addch(posX, posY + y, symbol);
        tui_addch(posX + sizeX - 1, posY + y, symbol);
    }
    for (int x = 0; x < sizeX; x++)
    {
        tui_addch(posX + x, posY, symbol);
        tui_addch(posX + x, posY + sizeY - 1, symbol);
    }
}
