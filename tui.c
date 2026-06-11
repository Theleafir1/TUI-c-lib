#include "tui.h"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>

static struct termios orig_termios;
static char tui_needForceRedraw = 0;

void tui_enableRawMode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
void tui_disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static void tui_handleSIGWINCH(int sig)
{
    (void)sig;
    tui_needForceRedraw = 1;
    tui_update();
    tui_print();
}

void tui_hideCursor()
{
    printf("\033[?25l");
    fflush(stdout);
}

void tui_showCursor()
{
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
        if (!tui_needForceRedraw && memcmp(&ctx->inputBuffer[i], &ctx->screenBuffer[i], sizeof(Cell)) == 0) continue;
        if (tui_needForceRedraw || lastIndex != i)
        {
            char move[32];
            int moveLen = sprintf(move, "\033[%d;%dH", i / ctx->windowWidth + 1, i % ctx->windowWidth + 1);
            memcpy(&ctx->diff[ctx->diffLength], move, moveLen);
            ctx->diffLength += moveLen;
            lastIndex = i;
        }
        if ( (tui_needForceRedraw || memcmp(&ctx->inputBuffer[i].bg, &ctx->screenBuffer[i].bg, sizeof(Color)) != 0) || memcmp(&ctx->inputBuffer[i].bg, &lastBg, sizeof(Color)) != 0)
        {
            char bgString[20];
            int len = sprintf(bgString, "\033[48;2;%d;%d;%dm", ctx->inputBuffer[i].bg.r, ctx->inputBuffer[i].bg.g, ctx->inputBuffer[i].bg.b);
            lastBg = ctx->inputBuffer[i].bg;
            memcpy(&ctx->diff[ctx->diffLength], bgString, len);
            ctx->diffLength += len;
        }
        if ( (tui_needForceRedraw || memcmp(&ctx->inputBuffer[i].fg, &ctx->screenBuffer[i].fg, sizeof(Color)) != 0) || memcmp(&ctx->inputBuffer[i].fg, &lastFg, sizeof(Color)) != 0)
        {
            char fgString[20];
            int len = sprintf(fgString, "\033[38;2;%d;%d;%dm", ctx->inputBuffer[i].fg.r, ctx->inputBuffer[i].fg.g, ctx->inputBuffer[i].fg.b);
            lastFg = ctx->inputBuffer[i].fg;
            memcpy(&ctx->diff[ctx->diffLength], fgString, len);
            ctx->diffLength += len;
        }
        if (tui_needForceRedraw || memcmp(ctx->inputBuffer[i].ch, ctx->screenBuffer[i].ch, 4))
        {
            int len = tui_getUTF8Len((unsigned char)ctx->inputBuffer[i].ch[0]);
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
void tui_update()
{
    int oldSize = ctx->windowSize;
    tui_updateSize();
    if (ctx->windowSize != oldSize)
    {
        ctx->inputBuffer = (Cell *)realloc(ctx->inputBuffer, ctx->windowSize * sizeof(Cell));
        ctx->screenBuffer = (Cell *)realloc(ctx->screenBuffer, ctx->windowSize * sizeof(Cell));

        memset(ctx->screenBuffer, 0, ctx->windowSize * sizeof(Cell));
        ctx->diffSize = ctx->windowSize * 64;  // 64 is even worse than the worst case, just for sure
        ctx->diff = (char *)realloc(ctx->diff, ctx->diffSize);
    }
}

int tui_getWindowHeight() {return ctx->windowHeight;}
int tui_getWindowWidth() {return ctx->windowWidth;}
int tui_getWindowSize() {return ctx->windowSize;}
void tui_print()
{
    if (!ctx->frameCounter || tui_needForceRedraw)
    {
        printf("\033[2J\033[H");
        fflush(stdout);
        tui_needForceRedraw = 0;
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
    ctx->diffSize = ctx->windowSize * 50;
    ctx->diff = (char *)malloc(ctx->diffSize);
    tui_enableRawMode();
    tui_hideCursor();
    signal(SIGWINCH, tui_handleSIGWINCH);
}
void tui_deinit()
{
    free(ctx->inputBuffer);
        ctx->inputBuffer = NULL;
    free(ctx->screenBuffer);
        ctx->screenBuffer = NULL;
    free(ctx->diff);
        ctx->diff = NULL;
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


void tui_inputBox(int maxLen, const char* prompt, char* out)
{
    
}