#include "tui.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
void TUIUtils::enableRawMode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
void TUIUtils::disableRawMode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag |= (ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
void TUIUtils::hideCursor() {
    printf("\033[?25l");
    fflush(stdout);
}

void TUIUtils::showCursor() {
    printf("\033[?25h");
    fflush(stdout);
}

int TUI::getFrameNumber() {return frameCounter;}
void TUI::calculateDiff()
{
    /*
        TODO:
        Done: Implement checking for should we even add Move ESC-code
        
    */
    Color lastBg = (Color){1, 2, 3};
    Color lastFg = (Color){3, 2, 1};
    int lastIndex = 0;
    diffLength = 0;
    for(int i = 0; i < windowSize; i++)
    {
        if (rawBuffer[i] == newBuffer[i]) continue;
        if (lastIndex != i)
        {
            char move[32];
            int moveLen = sprintf(move, "\033[%d;%dH", i / windowWidth + 1, i % windowWidth + 1);
            diff = (char *)realloc(diff, diffLength + moveLen);
            memcpy(&diff[diffLength], move, moveLen);
            diffLength += moveLen;
            lastIndex = i;
        }
        if ( (rawBuffer[i].bg != newBuffer[i].bg) || rawBuffer[i].bg != lastBg)
        {
            char bgString[20];
            int len = sprintf(bgString, "\033[48;2;%d;%d;%dm", rawBuffer[i].bg.r, rawBuffer[i].bg.g, rawBuffer[i].bg.b);
            lastBg = rawBuffer[i].bg;
            diff = (char *)realloc(diff, diffLength + len);
            memcpy(&diff[diffLength], bgString, len);
            diffLength += len;
        }
        if ( (rawBuffer[i].fg != newBuffer[i].fg) || rawBuffer[i].fg != lastFg)
        {
            char fgString[20];
            int len = sprintf(fgString, "\033[38;2;%d;%d;%dm", rawBuffer[i].fg.r, rawBuffer[i].fg.g, rawBuffer[i].fg.b);
            lastFg = rawBuffer[i].fg;
            diff = (char *)realloc(diff, diffLength + len);
            memcpy(&diff[diffLength], fgString, len);
            diffLength += len;
        }
        if (memcmp(rawBuffer[i].ch, newBuffer[i].ch, 4))
        {
            int len = getUTF8Len( (unsigned char)rawBuffer[i].ch[0]);
            diff = (char *)realloc(diff, diffLength + len);
            memcpy(&diff[diffLength], &rawBuffer[i].ch , len);
            diffLength += len;
        }
    }
}
void TUI::updateSize()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    windowHeight = w.ws_row;
    windowWidth = w.ws_col;
    windowSize = windowHeight * windowWidth;
}
int TUI::getUTF8Len(unsigned char firstByte) 
    {
        if (firstByte < 128) return 1;
        if (firstByte < 224) return 2;
        if (firstByte < 240) return 3;
        return 4;
    }
bool TUI::update() // returns true if screen was resized
{
    bool wasResized = false;
    int oldSize = windowSize;
    updateSize();
    if (windowSize != oldSize)
    {
        rawBuffer = (Cell *)realloc(rawBuffer, windowSize * sizeof(Cell));
        newBuffer = (Cell *)realloc(newBuffer, windowSize * sizeof(Cell));
        wasResized = true;
    }
    return wasResized;
}

int TUI::getWindowHeight() {return windowHeight;}
int TUI::getWindowWidth() {return windowWidth;}
int TUI::getWindowSize() {return windowSize;}
void TUI::print()
{
    if (!frameCounter)
    {
        printf("\033[2J\033[H");
    }
    frameCounter++;
    calculateDiff();
    fwrite(diff, sizeof(char), diffLength, stdout);
    fflush(stdout);
    memcpy(newBuffer, rawBuffer, windowSize * sizeof(Cell));
}
void TUI::fill(Cell symbol)
{
    for(int i = 0; i < windowSize; i++)
    {
        rawBuffer[i] = symbol;
    }
}
TUI::TUI() : frameCounter(0)
{
    updateSize();
    rawBuffer = nullptr;
    rawBuffer = (Cell *)malloc(windowSize * sizeof(Cell));
    newBuffer = nullptr;
    newBuffer = (Cell *)calloc(windowSize, sizeof(Cell));
    fill((Cell){" ", BLUE, BLUE});
    for(int i = 0; i < 8; i++) saveBuffer[i] = nullptr;
    diff = (char *)malloc(1);
    TUIUtils::enableRawMode();
    TUIUtils::hideCursor();
}
TUI::~TUI()
{
    if (rawBuffer){
        free(rawBuffer);
            rawBuffer = nullptr;
    }
    if (newBuffer){
        free(newBuffer);
            newBuffer = nullptr;
    }
    TUIUtils::disableRawMode();
    TUIUtils::showCursor();
}
void TUI::save(int idx)
{
    if(idx<0 || idx>=8) return;

    saveBuffer[idx] = (Cell *)malloc(windowSize * sizeof(Cell));
    if(saveBuffer[idx])
        memcpy(saveBuffer[idx], rawBuffer, windowSize);
}
void TUI::load(int idx, bool lose = 1)
{
    if(idx<0 || idx>=8) return;
        memcpy(rawBuffer, saveBuffer[idx], windowSize);

    if(saveBuffer[idx])
        memcpy(saveBuffer[idx], rawBuffer, windowSize);
    if (lose)
    {
        free(saveBuffer[idx]);
        saveBuffer[idx] = nullptr;
    }
}

void TUI::addchi(int idx, Cell ch)
{
    if (idx >= getWindowSize()) return;
    rawBuffer[idx] = ch;
}
void TUI::addch(int x, int y, Cell ch)
{
    int idx = y * getWindowWidth() + x;
    if (idx >= getWindowSize()) return;
    rawBuffer[idx] = ch;
}
void TUI::cgotoi(int idx)
{
    int y = idx / getWindowWidth(); 
    int x = idx % getWindowWidth();
    printf("\033[%d;%dH", y+1, x+1);
}
void TUI::cgoto(int x, int y)
{
    printf("\033[%d;%dH", y+1, x+1);
}

