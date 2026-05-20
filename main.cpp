#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <sstream>
#include "tui.h"

// Helper to convert a single byte to a 2-char hex string
std::string byte_to_hex(unsigned char b) {
    std::stringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    return ss.str();
}

void draw_text(TUI& tui, int x, int y, const std::string& text, Color fg, Color bg) {
    for (size_t i = 0; i < text.length(); ++i) {
        if (x + i < (size_t)tui.getWindowWidth()) {
            Cell c;
            memset(c.ch, 0, 4);
            c.ch[0] = text[i];
            c.fg = fg;
            c.bg = bg;
            tui.addch(x + i, y, c);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return 1;
    }

    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});

    TUI tui;
    bool running = true;
    size_t offset = 0;
    
    bool dynamic_width = true;
    const int default_bytes_per_line = 16;
    int bytes_per_line = default_bytes_per_line;

    while (running) {
        tui.update();

        int width = tui.getWindowWidth();
        int height = tui.getWindowHeight();
        
        if (dynamic_width) {
            // 10 for offset, 2 for space between hex/ascii
            int data_width = width - 10 - 2;
            if (data_width < 0) data_width = 0;
            // 3 for hex chars ("XX "), 1 for ascii
            bytes_per_line = data_width / 4;
            if (bytes_per_line < 4) bytes_per_line = 4;
        } else {
            bytes_per_line = default_bytes_per_line;
        }

        Cell default_cell;
        default_cell.ch[0] = ' ';
        default_cell.ch[1] = 0;
        default_cell.fg = {200, 200, 200};
        default_cell.bg = {20, 20, 20};
        tui.fill(default_cell);
        
        // Draw Hex View
        for (int y = 0; y < height - 1; ++y) {
            size_t current_offset = offset + (y * bytes_per_line);
            if (current_offset >= buffer.size()) break;

            // 1. Offset column
            std::stringstream ss_offset;
            ss_offset << std::hex << std::setw(8) << std::setfill('0') << current_offset;
            draw_text(tui, 0, y, ss_offset.str(), {100, 100, 255}, default_cell.bg);
            
            // 2. Hex bytes columns
            for (int i = 0; i < bytes_per_line; ++i) {
                if (current_offset + i < buffer.size()) {
                    unsigned char byte = buffer[current_offset + i];
                    draw_text(tui, 10 + i * 3, y, byte_to_hex(byte), default_cell.fg, default_cell.bg);
                }
            }

            // 3. ASCII representation column
            int ascii_start_col = 10 + bytes_per_line * 3 + 2;
            for (int i = 0; i < bytes_per_line; ++i) {
                if (current_offset + i < buffer.size()) {
                    unsigned char byte = buffer[current_offset + i];
                    char display_char = (isprint(byte)) ? byte : '.';
                    std::string s(1, display_char);
                    draw_text(tui, ascii_start_col + i, y, s, {200, 200, 100}, default_cell.bg);
                }
            }
        }

        // Status Bar
        Cell status_bg_cell;
        status_bg_cell.ch[0] = ' '; status_bg_cell.ch[1] = 0;
        status_bg_cell.fg = {0,0,0}; status_bg_cell.bg = {200,200,200};
        for(int i=0; i<width; ++i) tui.addch(i, height - 1, status_bg_cell);

        std::string status = "File: " + filename + " | q/й: Quit | w: Toggle dynamic width (" + (dynamic_width ? "On" : "Off") + ")";
        if (status.length() > (size_t)width) {
            status.resize(width);
        }
        draw_text(tui, 0, height - 1, status, {0, 0, 0}, {200, 200, 200});


        tui.print();

        // Handle Input
        int c = getchar();
        switch (c) {
            case 'q':
                running = false;
                break;
            case 'w':
                dynamic_width = !dynamic_width;
                break;
            case 0xd0: // First byte of a 2-byte UTF-8 char in Cyrillic
                if (getchar() == 0xb9) { // Second byte of 'й'
                    running = false;
                }
                break;
            case 27: // Escape sequence
                getchar(); // Skip '['
                switch(getchar()) {
                    case 'A': // Up Arrow
                        if (offset >= (size_t)bytes_per_line) offset -= bytes_per_line;
                        else offset = 0;
                        break;
                    case 'B': // Down Arrow
                        if (offset + (height - 1) * bytes_per_line < buffer.size()) {
                            offset += bytes_per_line;
                        }
                        break;
                }
                break;
        }
    }

    return 0;
}
