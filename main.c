#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "tui.h"

typedef enum {
    MODE_NORMAL,
    MODE_SEARCH
} AppState;

// Helper to draw text on the screen
void draw_text(int x, int y, const char* text, Color fg, Color bg) {
    int width = tui_getWindowWidth();
    for (size_t i = 0; text[i] != '\0'; ++i) {
        if (x + i < (size_t)width) {
            Cell c;
            memset(c.ch, 0, 4);
            c.ch[0] = text[i];
            c.fg = fg;
            c.bg = bg;
            tui_addch(x + i, y, c);
        }
    }
}

void perform_search(const unsigned char* buffer, long size, const char* query, size_t** results, int* count) {
    // Clear previous results
    if (*results) {
        free(*results);
        *results = NULL;
    }
    *count = 0;
    if (!query || query[0] == '\0') {
        return;
    }

    int query_len = strlen(query);
    size_t capacity = 10;
    *results = malloc(capacity * sizeof(size_t));

    for (long i = 0; i <= size - query_len; ++i) {
        if (memcmp(buffer + i, query, query_len) == 0) {
            if (*count >= capacity) {
                capacity *= 2;
                *results = realloc(*results, capacity * sizeof(size_t));
            }
            (*results)[*count] = i;
            (*count)++;
        }
    }
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    char *filename = argv[1];
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char *buffer = malloc(fsize);
    if(!buffer) {
        fprintf(stderr, "Failed to allocate memory for file\n");
        fclose(file);
        return 1;
    }
    fread(buffer, 1, fsize, file);
    fclose(file);

    TUIContext tui_ctx;
    tui_init(&tui_ctx);

    AppState app_mode = MODE_NORMAL;
    char search_query[256] = {0};
    int query_len = 0;

    size_t* search_results = NULL;
    int num_results = 0;
    int current_result_idx = -1;

    bool running = true;
    size_t offset = 0;
    
    bool dynamic_width = true;
    const int default_bytes_per_line = 16;
    int bytes_per_line = default_bytes_per_line;

    while (running) {
        tui_update();

        int width = tui_getWindowWidth();
        int height = tui_getWindowHeight();
        
        if (dynamic_width) {
            int data_width = width - 10 - 2;
            if (data_width < 0) data_width = 0;
            bytes_per_line = data_width / 4;
            if (bytes_per_line < 4) bytes_per_line = 4;
        } else {
            bytes_per_line = default_bytes_per_line;
        }

        Cell default_cell;
        default_cell.ch[0] = ' ';
        default_cell.ch[1] = 0;
        default_cell.fg = (Color){200, 200, 200};
        default_cell.bg = (Color){20, 20, 20};
        tui_fill(default_cell);
        
        bool highlight_active = current_result_idx != -1;

        for (int y = 0; y < height - 1; ++y) {
            size_t current_offset = offset + (y * bytes_per_line);
            if (current_offset >= (size_t)fsize) break;

            // 1. Offset column
            char offset_str[9];
            sprintf(offset_str, "%08zx", current_offset);
            draw_text(0, y, offset_str, (Color){100, 100, 255}, default_cell.bg);
            
            // 2. Hex bytes columns
            for (int i = 0; i < bytes_per_line; ++i) {
                size_t byte_pos = current_offset + i;
                if (byte_pos < (size_t)fsize) {
                    bool is_highlighted = false;
                    if (highlight_active) {
                        size_t highlight_start = search_results[current_result_idx];
                        int highlight_len = strlen(search_query);
                        if (byte_pos >= highlight_start && byte_pos < highlight_start + highlight_len) {
                            is_highlighted = true;
                        }
                    }
                    unsigned char byte = buffer[byte_pos];
                    char hex_str[3];
                    sprintf(hex_str, "%02x", byte);
                    draw_text(10 + i * 3, y, hex_str, is_highlighted ? default_cell.bg : default_cell.fg, is_highlighted ? default_cell.fg : default_cell.bg);
                }
            }

            // 3. ASCII representation column
            int ascii_start_col = 10 + bytes_per_line * 3 + 2;
            for (int i = 0; i < bytes_per_line; ++i) {
                size_t byte_pos = current_offset + i;
                if (byte_pos < (size_t)fsize) {
                    bool is_highlighted = false;
                    if (highlight_active) {
                        size_t highlight_start = search_results[current_result_idx];
                        int highlight_len = strlen(search_query);
                        if (byte_pos >= highlight_start && byte_pos < highlight_start + highlight_len) {
                            is_highlighted = true;
                        }
                    }
                    unsigned char byte = buffer[byte_pos];
                    char display_char = (isprint(byte)) ? byte : '.';
                    char s[2] = {display_char, '\0'};
                    draw_text(ascii_start_col + i, y, s, is_highlighted ? (Color){20,20,20} : (Color){200, 200, 100}, is_highlighted ? (Color){200, 200, 100} : default_cell.bg);
                }
            }
        }
        
        // Status Bar / Search Bar
        Cell status_bg_cell;
        status_bg_cell.ch[0] = ' '; status_bg_cell.ch[1] = 0;
        status_bg_cell.fg = (Color){0,0,0}; status_bg_cell.bg = (Color){200,200,200};
        for(int i=0; i<width; ++i) tui_addch(i, height - 1, status_bg_cell);

        if (app_mode == MODE_NORMAL) {
            char status[256];
            snprintf(status, sizeof(status), "File: %s | Press '/' to search", filename);
             if (num_results > 0) {
                char result_status[100];
                snprintf(result_status, 100, " | Found %d results for \"%s\" (%d/%d)", num_results, search_query, current_result_idx + 1, num_results);
                strncat(status, result_status, sizeof(status) - strlen(status) -1);
            }
            if (strlen(status) > (size_t)width) status[width] = '\0';
            draw_text(0, height - 1, status, (Color){0, 0, 0}, (Color){200, 200, 200});
        } else { // MODE_SEARCH
            char search_label[256];
            snprintf(search_label, sizeof(search_label), "/%s", search_query);
            draw_text(0, height-1, search_label, (Color){0,0,0}, (Color){200,200,200});
            tui_cgoto(query_len + 2, height);
            tui_showCursor();
        }

        tui_print();
        if (app_mode == MODE_SEARCH) tui_hideCursor();


        int c = getchar();

        if (app_mode == MODE_NORMAL) {
            switch (c) {
                case 'q': running = false; break;
                case 'w': dynamic_width = !dynamic_width; break;
                case '/': 
                    app_mode = MODE_SEARCH;
                    query_len = 0;
                    search_query[0] = '\0';
                    break;
                case 'n':
                    if (num_results > 0) {
                        current_result_idx = (current_result_idx + 1) % num_results;
                        offset = search_results[current_result_idx] / bytes_per_line * bytes_per_line;
                    }
                    break;
                case 'p':
                     if (num_results > 0) {
                        current_result_idx = (current_result_idx - 1 + num_results) % num_results;
                        offset = search_results[current_result_idx] / bytes_per_line * bytes_per_line;
                    }
                    break;
                case 0xd0: if (getchar() == 0xb9) { running = false; } break;
                case 27: // Escape sequence
                    getchar(); // Skip '['
                    switch(getchar()) {
                        case 'A': // Up Arrow
                            if (offset >= (size_t)bytes_per_line) offset -= bytes_per_line;
                            else offset = 0;
                            break;
                        case 'B': // Down Arrow
                            if (offset + (height - 1) * bytes_per_line < (size_t)fsize) {
                                offset += bytes_per_line;
                            }
                            break;
                    }
                    break;
            }
        } else { // MODE_SEARCH
            switch (c) {
                case '\n': // Enter
                    perform_search(buffer, fsize, search_query, &search_results, &num_results);
                    if (num_results > 0) {
                        current_result_idx = 0;
                        offset = search_results[0] / bytes_per_line * bytes_per_line;
                    } else {
                        current_result_idx = -1;
                    }
                    app_mode = MODE_NORMAL;
                    break;
                case 27: // Escape
                    app_mode = MODE_NORMAL;
                    break;
                case 127: // Backspace
                    if (query_len > 0) {
                        query_len--;
                        search_query[query_len] = '\0';
                    }
                    break;
                default:
                    if (isprint(c) && query_len < sizeof(search_query) - 1) {
                        search_query[query_len++] = c;
                        search_query[query_len] = '\0';
                    }
                    break;
            }
        }
    }

    if (search_results) {
        free(search_results);
    }
    tui_deinit();
    free(buffer);

    return 0;
}

