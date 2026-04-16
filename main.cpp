/*
    PREVIEW
    AI wrote this snake game so idk how it works
*/
#include "tui.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include <locale.h>
#include <string.h>

#define MAX_SNAKE 1000

struct Snake {
    int x[MAX_SNAKE];
    int y[MAX_SNAKE];
    int len;
    int dirX, dirY;
};

int kbhit() {
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
}

int getch() {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) return c;
    return -1;
}

// generating food 
void spawnFood(int *foodX, int *foodY, Snake &snake, int w, int h) {
    int freeCells = w * h - snake.len;
    if (freeCells <= 0) {
        *foodX = -1;
        *foodY = -1;
        return;
    }
    int r = rand() % freeCells;
    int idx = 0;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            bool occupied = false;
            for (int i = 0; i < snake.len; i++) {
                if (snake.x[i] == x && snake.y[i] == y) {
                    occupied = true;
                    break;
                }
            }
            if (!occupied) {
                if (idx == r) {
                    *foodX = x;
                    *foodY = y;
                    return;
                }
                idx++;
            }
        }
    }
    *foodX = -1;
}

int main() {
    setlocale(LC_ALL, "");
    srand(time(NULL));
    TUI screen;
    
    Color white = {255, 255, 255};
    
    int startX = screen.getWindowWidth() / 2;
    int startY = screen.getWindowHeight() / 2;
    Snake snake;
    snake.len = 3;
    for (int i = 0; i < snake.len; i++) {
        snake.x[i] = startX - i;
        snake.y[i] = startY;
    }
    snake.dirX = 1;
    snake.dirY = 0;
    
    int foodX, foodY;
    spawnFood(&foodX, &foodY, snake, screen.getWindowWidth(), screen.getWindowHeight());
    
    int score = 0;
    bool gameOver = false;
    int speed = 100000;
    
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    
    while (!gameOver) {
        bool resized = screen.update();
        if (resized) {
            // Перезапуск при ресайзе
            startX = screen.getWindowWidth() / 2;
            startY = screen.getWindowHeight() / 2;
            snake.len = 3;
            for (int i = 0; i < snake.len; i++) {
                snake.x[i] = startX - i;
                snake.y[i] = startY;
            }
            snake.dirX = 1;
            snake.dirY = 0;
            score = 0;
            speed = 100000;
            spawnFood(&foodX, &foodY, snake, screen.getWindowWidth(), screen.getWindowHeight());
            screen.fill((Cell){{' ',0,0,0}, BLACK, BLACK});
            screen.print();
            continue;
        }
        
        int ch = getch();
        if (ch != -1) {
            if (ch == 'w' || ch == 'W') {
                if (snake.dirY != 1) { snake.dirX = 0; snake.dirY = -1; }
            } else if (ch == 's' || ch == 'S') {
                if (snake.dirY != -1) { snake.dirX = 0; snake.dirY = 1; }
            } else if (ch == 'a' || ch == 'A') {
                if (snake.dirX != 1) { snake.dirX = -1; snake.dirY = 0; }
            } else if (ch == 'd' || ch == 'D') {
                if (snake.dirX != -1) { snake.dirX = 1; snake.dirY = 0; }
            } else if (ch == 'q') {
                gameOver = true;
            }
        }
        
        int newX = snake.x[0] + snake.dirX;
        int newY = snake.y[0] + snake.dirY;
        
        if (newX < 0 || newX >= screen.getWindowWidth() ||
            newY < 0 || newY >= screen.getWindowHeight()) {
            gameOver = true;
            break;
        }
        
        bool ate = (newX == foodX && newY == foodY);
        
        for (int i = snake.len; i > 0; i--) {
            snake.x[i] = snake.x[i-1];
            snake.y[i] = snake.y[i-1];
        }
        snake.x[0] = newX;
        snake.y[0] = newY;
        
        if (ate) {
            snake.len++;
            score++;
            if (speed > 40000) speed -= 5000;
            spawnFood(&foodX, &foodY, snake, screen.getWindowWidth(), screen.getWindowHeight());
            if (foodX == -1) {
                gameOver = true;
                break;
            }
        }
        
        for (int i = 1; i < snake.len; i++) {
            if (snake.x[0] == snake.x[i] && snake.y[0] == snake.y[i]) {
                gameOver = true;
                break;
            }
        }
        
        // Drawing
        screen.fill((Cell){{' ',0,0,0}, BLACK, BLACK});
        
        for (int i = 0; i < snake.len; i++) {
            Cell snakeCell = {{'O',0,0,0}, GREEN, BLACK};
            if (i == 0) snakeCell = {{'@',0,0,0}, GREEN, BLACK};
            screen.addch(snake.x[i], snake.y[i], snakeCell);
        }
        
        screen.addch(foodX, foodY, (Cell){{'*',0,0,0}, RED, BLACK});
        
        char scoreStr[32];
        sprintf(scoreStr, "Score: %d", score);
        for (int i = 0; scoreStr[i] != '\0'; i++) {
            screen.addch(i, 0, (Cell){{scoreStr[i],0,0,0}, white, BLACK});
        }
        
        screen.print();
        usleep(speed);
    }
    
    screen.fill((Cell){{' ',0,0,0}, BLACK, BLACK});
    char msg[32];
    sprintf(msg, "GAME OVER! Score: %d", score);
    int msgLen = strlen(msg);
    int startCol = (screen.getWindowWidth() - msgLen) / 2;
    int row = screen.getWindowHeight() / 2;
    for (int i = 0; i < msgLen; i++) {
        screen.addch(startCol + i, row, (Cell){{msg[i],0,0,0}, RED, BLACK});
    }
    screen.print();
    usleep(2000000);
    
    fcntl(STDIN_FILENO, F_SETFL, flags);
    return 0;
}