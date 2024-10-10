#include <ppu-lv2.h>
#include <sys/spu.h>
#include <lv2/spu.h>

#include <lv2/process.h>
#include <sys/systime.h>
#include <sys/thread.h>
#include <sys/file.h>
#include <sysmodule/sysmodule.h>

#include <sysutil/sysutil.h>

#include <io/pad.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <tiny3d.h>
#include <libfont.h>

#include "font.h"

#include <math.h>
#include <time.h>

#include "sincos.h"

//#define FROM_FILE

#ifndef FROM_FILE
#include "spu_soundmodule.bin.h" // load SPU Module
#else
void * spu_soundmodule_bin = NULL;
#endif

#include "spu_soundlib.h"
#include "audioplayer.h"

#include "m2003_mp3_bin.h"
#include "effect_mp3_bin.h"
#include "sound_ogg_bin.h"

float text_x = 0.0f; // text X position for DrawFormatString()
float text_y = 0.0f; // text Y position for DrawFormatString()

// SPU
u32 spu = 0;
sysSpuImage spu_image;

#define SPU_SIZE(x) (((x)+127) & ~127)

padInfo padinfo;
padData paddata;

u32 inited;

#define INITED_CALLBACK     1
#define INITED_SPU          2
#define INITED_SOUNDLIB     4
#define INITED_AUDIOPLAYER  8

// Definitions
#define GRID_SIZE 3
#define CELL_SIZE 100
#define GRID_X 324 // Centered X position
#define GRID_Y 206 // Centered Y position

// Tic-Tac-Toe Board
char board[GRID_SIZE][GRID_SIZE];

// Player turn: 1 for X, 2 for O
int player_turn = 1, turns, winner;

// Initialize the game board
void initialize_board() {
    int i, j; // Declare loop variables here
    for (i = 0; i < GRID_SIZE; i++) {
        for (j = 0; j < GRID_SIZE; j++) {
            board[i][j] = ' '; // Empty cell
        }
    }
    turns=1;
    player_turn=1;
    winner=0;
}

void drawW(int x, int y) {
    // Set color for the W (black)
    u32 black_color = 0xeeeeeeee; // Black color

    tiny3d_SetPolygon(TINY3D_LINES);
    tiny3d_VertexColor(black_color);

    // Define the points for the W shape
    int width = 50; // Width of the W
    int height = 50; // Height of the W

    // Draw the lines to form the W
    tiny3d_VertexPos(x, y, 65535.0f);
    tiny3d_VertexPos(x + width / 4, y + height, 65535.0f);

    tiny3d_VertexPos(x + width / 4, y + height, 65535.0f);
    tiny3d_VertexPos(x + width / 2, y, 65535.0f);

    tiny3d_VertexPos(x + width / 2, y, 65535.0f);
    tiny3d_VertexPos(x + (3 * width / 4), y + height, 65535.0f);

    tiny3d_VertexPos(x + (3 * width / 4), y + height, 65535.0f);
    tiny3d_VertexPos(x + width, y, 65535.0f);
    tiny3d_End();
}

// Function to draw N
void drawN(int x, int y) {
    // Set color for the N (black)
    u32 black_color = 0xeeeeeeee; // Black color

    tiny3d_SetPolygon(TINY3D_LINES);
    tiny3d_VertexColor(black_color);

    // Define the points for the N shape
    int height = 50; // Height of the N
    int width = 50;  // Width of the N

    // Draw the lines to form the N
    tiny3d_VertexPos(x, y, 65535.0f);            // Left vertical line
    tiny3d_VertexPos(x, y + height, 65535.0f);  // Left vertical line
    tiny3d_VertexPos(x, y, 65535.0f);            // Diagonal line
    tiny3d_VertexPos(x + width, y + height, 65535.0f);   // Diagonal line
    tiny3d_VertexPos(x + width, y, 65535.0f);   // Right vertical line
    tiny3d_VertexPos(x + width, y + height, 65535.0f); // Right vertical line
    tiny3d_End();
}

// Function to draw T
void drawT(int x, int y) {
    // Set color for the T (black)
    u32 black_color = 0xeeeeeeee; // Black color

    tiny3d_SetPolygon(TINY3D_LINES);
    tiny3d_VertexColor(black_color);

    // Define the points for the T shape
    int width = 20;  // Width of the T
    int height = 30; // Height of the T

    // Draw the horizontal line (top of the T)
    tiny3d_VertexPos(x, y, 65535.0f);               // Top horizontal line
    tiny3d_VertexPos(x + width, y, 65535.0f);      // Top horizontal line

    // Draw the vertical line (stem of the T)
    tiny3d_VertexPos(x + width / 2, y, 65535.0f); // Vertical line
    tiny3d_VertexPos(x + width / 2, y + height, 65535.0f); // Vertical line
    tiny3d_End();
}

// Function to draw I
void drawI(int x, int y) {
    // Set color for the I (black)
    u32 black_color = 0xeeeeeeee; // Black color

    tiny3d_SetPolygon(TINY3D_LINES);
    tiny3d_VertexColor(black_color);

    int width = 20; // Width of the I
    int height = 30; // Height of the I

    // Draw the T for the top part of I
    tiny3d_VertexPos(x, y, 65535.0f);               // Top horizontal line
    tiny3d_VertexPos(x + width, y, 65535.0f);      // Top horizontal line

    // Draw the vertical line (stem of the I)
    tiny3d_VertexPos(x + width / 2, y, 65535.0f); // Vertical line
    tiny3d_VertexPos(x + width / 2, y + height, 65535.0f); // Vertical line

    // Draw the bottom line
    tiny3d_VertexPos(x, y + height, 65535.0f);           // Bottom line
    tiny3d_VertexPos(x + width, y + height, 65535.0f);   // Bottom line
    tiny3d_End();
}

// Function to draw E
void drawE(int x, int y) {
    // Set color for the E (black)
    u32 black_color = 0xeeeeeeee; // Black color

    tiny3d_SetPolygon(TINY3D_LINES);
    tiny3d_VertexColor(black_color);

    // Define the dimensions for the letter E
    int width = 20;  // Width of the E
    int height = 30; // Height of the E

    // Draw the vertical line (stem of the E)
    tiny3d_VertexPos(x, y, 65535.0f);           // Vertical line
    tiny3d_VertexPos(x, y + height, 65535.0f); // Vertical line

    // Draw the three horizontal lines (top, middle, bottom of the E)
    tiny3d_VertexPos(x, y, 65535.0f);           // Top horizontal line
    tiny3d_VertexPos(x + width, y, 65535.0f);   // Top horizontal line
    tiny3d_VertexPos(x, y + height / 2, 65535.0f); // Middle horizontal line
    tiny3d_VertexPos(x + width, y + height / 2, 65535.0f); // Middle horizontal line
    tiny3d_VertexPos(x, y + height, 65535.0f); // Bottom horizontal line
    tiny3d_VertexPos(x + width, y + height, 65535.0f); // Bottom horizontal line
    tiny3d_End();
}

void drawX(int x, int y) {
    u32 blue_color = 0xff0000ff; // Blue color for X
    tiny3d_SetPolygon(TINY3D_LINES);
    tiny3d_VertexColor(blue_color);

    // Draw the X shape
    tiny3d_VertexPos(x, y, 65535.0f); // Top left
    tiny3d_VertexPos(x + 50, y + 50, 65535.0f); // Bottom right

    tiny3d_VertexPos(x, y + 50, 65535.0f); // Bottom left
    tiny3d_VertexPos(x + 50, y, 65535.0f); // Top right
    tiny3d_End();
}

// Function to draw an O at the given coordinates
void drawO(int x, int y) {
    u32 red_color = 0xffff0000; // Red color for O
    tiny3d_SetPolygon(TINY3D_LINES);
    tiny3d_VertexColor(red_color);

    // Calculate center position for O
    float centerX = x + CELL_SIZE / 2;
    float centerY = y + CELL_SIZE / 2;
    int k;

    // Draw the circle (O)
    for (k = 0; k < 36; k++) {
        float angle = (k / 36.0) * 2.0 * M_PI;
        float posX = centerX + (CELL_SIZE / 4) * cos(angle);
        float posY = centerY + (CELL_SIZE / 4) * sin(angle);
        tiny3d_VertexPos(posX, posY, 65535.0f);
    }
    tiny3d_End();
}

// Draw the Tic-Tac-Toe grid and the symbols
void draw_grid() {
    u32 red_color = 0xff0000ff; // Green color
    u32 white_color = 0xffffffff;
    u32 blue_color = 0x0000FFFF;
    tiny3d_SetPolygon(TINY3D_LINES);
    tiny3d_VertexColor(white_color);

    int i; // Declare loop variable here

    // Draw vertical lines
    for (i = 1; i < GRID_SIZE; i++) {
        tiny3d_VertexPos(GRID_X + i * CELL_SIZE, GRID_Y, 65535.0f);
        tiny3d_VertexPos(GRID_X + i * CELL_SIZE, GRID_Y + GRID_SIZE * CELL_SIZE, 65535.0f);
    }

    // Draw horizontal lines
    for (i = 1; i < GRID_SIZE; i++) {
        tiny3d_VertexPos(GRID_X, GRID_Y + i * CELL_SIZE, 65535.0f);
        tiny3d_VertexPos(GRID_X + GRID_SIZE * CELL_SIZE, GRID_Y + i * CELL_SIZE, 65535.0f);
    }

    // Draw symbols (X and O)
    for (i = 0; i < GRID_SIZE; i++) {
        int j; // Declare loop variable here
        for (j = 0; j < GRID_SIZE; j++) {
            if (board[i][j] == 'X') {
                // Draw X
                tiny3d_VertexColor(blue_color);
                tiny3d_VertexPos(GRID_X + j * CELL_SIZE, GRID_Y + i * CELL_SIZE, 65535.0f);
                tiny3d_VertexPos(GRID_X + (j + 1) * CELL_SIZE, GRID_Y + (i + 1) * CELL_SIZE, 65535.0f);
                tiny3d_VertexPos(GRID_X + j * CELL_SIZE, GRID_Y + (i + 1) * CELL_SIZE, 65535.0f);
                tiny3d_VertexPos(GRID_X + (j + 1) * CELL_SIZE, GRID_Y + i * CELL_SIZE, 65535.0f);
            } else if (board[i][j] == 'O') {
                // Draw O
                float centerX = GRID_X + j * CELL_SIZE + CELL_SIZE / 2;
                float centerY = GRID_Y + i * CELL_SIZE + CELL_SIZE / 2;
                int k; // Declare loop variable here
                tiny3d_VertexColor(red_color);
                for (k = 0; k < 36; k++) {
                    float angle = (k / 36.0) * 2.0 * M_PI;
                    float x = centerX + (CELL_SIZE / 4) * cos(angle);
                    float y = centerY + (CELL_SIZE / 4) * sin(angle);
                    tiny3d_VertexPos(x, y, 65535.0f);
                }
            }
        }
    }
    tiny3d_End();
}

// Draw the cursor
void draw_cursor(int cellX, int cellY) {
    u32 green_color = 0xff00ff00; // Green color

    // Draw an outlined square for the cursor
    tiny3d_SetPolygon(TINY3D_LINES);
    tiny3d_VertexColor(green_color);

    float cursorX = GRID_X + cellX * CELL_SIZE;
    float cursorY = GRID_Y + cellY * CELL_SIZE;

    // Define the four corners of the square
    tiny3d_VertexPos(cursorX, cursorY, 65535.0f); // Bottom left
    tiny3d_VertexPos(cursorX + CELL_SIZE, cursorY, 65535.0f); // Bottom right

    tiny3d_VertexPos(cursorX + CELL_SIZE, cursorY, 65535.0f); // Bottom right
    tiny3d_VertexPos(cursorX + CELL_SIZE, cursorY + CELL_SIZE, 65535.0f); // Top right

    tiny3d_VertexPos(cursorX + CELL_SIZE, cursorY + CELL_SIZE, 65535.0f); // Top right
    tiny3d_VertexPos(cursorX, cursorY + CELL_SIZE, 65535.0f); // Top left

    tiny3d_VertexPos(cursorX, cursorY + CELL_SIZE, 65535.0f); // Top left
    tiny3d_VertexPos(cursorX, cursorY, 65535.0f); // Bottom left

    tiny3d_End();
}

// Check if the game is over
int check_winner() {
    int i; // Declare loop variable here
    for (i = 0; i < GRID_SIZE; i++) {
        // Check rows and columns
        if ((board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ') ||
            (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' ')) {
            return board[i][i] == 'X' ? 1 : 2; // Return player number
        }
    }

    // Check diagonals
    if ((board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ') ||
        (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' ')) {
        return board[1][1] == 'X' ? 1 : 2;
    }

    return 0; // No winner
}

void demo() {
    initialize_board();
    padData paddata;

    int cellX = 0, cellY = 0; // Declare these variables before the loop
    int n, movedBefore=0, moved; // Declare index variable for pad info loop
    char buf[10];
    while (1) {
        tiny3d_Clear(0xff000000, TINY3D_CLEAR_ALL);
        draw_grid();
        draw_cursor(cellX, cellY); // Draw the cursor
        ioPadGetInfo(&padinfo);
        for (n = 0; n < MAX_PADS; n++) {
            if (padinfo.status[n]) {
                moved=0;
                ioPadGetData(n, &paddata);
                if (paddata.BTN_LEFT) {
                    if(!movedBefore)
                        cellX = cellX == 0 ? 2 : cellX - 1; // Move cursor left
                    moved=1;
                }
                else if (paddata.BTN_RIGHT) {
                    if(!movedBefore)
                        cellX = cellX == 2 ? 0 : cellX + 1; // Move cursor right
                    moved=1;
                }
                if (paddata.BTN_UP) {
                    if(!movedBefore)
                        cellY = cellY == 0 ? 2 : cellY - 1; // Move cursor up
                    moved=1;
                }
                else if (paddata.BTN_DOWN) {
                    if(!movedBefore)
                        cellY = cellY == 2 ? 0 : cellY + 1; // Move cursor down
                    moved=1;
                }
                movedBefore=moved;
                if (paddata.BTN_CROSS) {
                    // Place X or O if the cell is empty
                    if (!winner && board[cellY][cellX] == ' ') {
                        board[cellY][cellX] = (player_turn == 1) ? 'X' : 'O';
                        winner = check_winner();
                        if (winner)
                            sprintf(buf, "%c wins!", player_turn==1?'X':'O');
                        else if(turns++==9)
                            winner='T';
                        player_turn = (player_turn == 1) ? 2 : 1; // Switch player
                    }
                }
                else if (paddata.BTN_START && winner) {
                    initialize_board();
                }
            }
        }
        if(winner) {
            if(winner=='T') {
                drawT(0, 0);
                drawI(30, 0);
                drawE(60, 0);
            }
            else {
                if(winner==1)
                    drawX(25, 25);
                else
                    drawO(0, 0);
                drawW(120, 25);
                drawO(150, 0);
                drawN(230, 25);
            }
        }
        tiny3d_Flip();
    }
}

int main(int argc, const char* argv[]) {
    tiny3d_Init(1024 * 1024);
    ioPadInit(7);

    // Initialize the Tic-Tac-Toe game
    demo();

    return 0;
}
