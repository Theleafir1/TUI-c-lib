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

struct Color{
    uint8_t r, g, b;
    bool operator!=(const Color& other) const {
        return memcmp(this, &other, sizeof(Color)) != 0;
    }
};
struct Cell{
    char ch[4];
    Color fg, bg;
    bool operator==(const Cell& other) const {
        return memcmp(this, &other, sizeof(Cell)) == 0;
    }
};

namespace TUIUtils
{
    void enableRawMode();
    void disableRawMode();
    //     char c;
    //     while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
    //     }
}
class TUI
{
private:
    Cell *rawBuffer;
    Cell *newBuffer;
    Cell * saveBuffer[8];
    int windowSize;
    int windowHeight;
    int windowWidth;
    long long frameCounter;
    char *diff;
    int diffLength;
    void updateSize();
    int getUTF8Len(unsigned char first_byte);
public:
    int getFrameNumber();
    TUI();
    ~TUI();
    int getWindowHeight(); 
    int getWindowWidth();
    int getWindowSize();
    bool update();
    void print();
    void fill(Cell symbol);

    void save(int idx);
    void load(int idx, bool lose);

    void addchi(int idx, Cell ch);
    void addch(int x, int y, Cell ch);

    void cgotoi(int idx);
    void cgoto(int x, int y);

    void calculateDiff();
};
#endif