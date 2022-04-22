#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>

//
//    ////////// ////// ////////// /////////  //////// ////////
//       //     //         //     //     //     //    //
//      //     //////     //     /////////     //     //////
//     //     //         //     //    \\      //          //  
//    //     //////     //     //      \\  ///////  ///////
//
// RED 
// LEFT           RIGHT
// |         |    |   [2]   |
// |   [1][3]|    |   [1][3]|
// |[2][0]   |    |      [0]| 
//               
//
// GREEN
// LEFT           RIGHT
// |         |    |      [1]|
// |[2][1]   |    |   [2][3]|
// |   [0][3]|    |   [0]   |
//                   
// CYAN
// LEFT           RIGHT
// |      [3]   | |            |
// |      [2]   | |            |
// |      [1]   | |[2][1][0][3]|
// |      [0]   | |            |
//
// BLUE
// LEFT           UP
// |   [1]   |    |         |
// |   [2]   |    |[2][1][3]|
// |   [0][3]|    |[0]      |
// 
// RIGHT          DOWN
// |[2][3]   |    |      [3]|
// |   [1]   |    |[2][1][0]|      
// |   [0]   |    |         |
//
// YELLOW
// |            |
// |   [2][1]   |         
// |   [0][3]   |   
// |            |
//      
// PURPLE
// LEFT         UP
// |   [1]   |  |         |
// |   [2][3]|  |[2][1][3]|
// |   [0]   |  |   [0]   |
// 
// RIGHT        DOWN
// |   [3]   |  |   [1]   |
// |[2][1]   |  |[2][0][3]|
// |   [0]   |  |         |
//      
// ORANGE
// LEFT          UP
// |   [3]   |   |[2]      |
// |   [1]   |   |[0][1][3]|
// |[2][0]   |   |         |
// 
// RIGHT         DOWN
// [   [2][3]|   |         |
// [   [1]   |   |[2][1][3]|
// [   [0]   |   |      [0]|
//
#define BLOCK "[ ]"
#define paint(y, x) mvprintw(y, x, BLOCK);
#define whiteout(y, x) mvprintw(y, x, "   ");
#define blocktomatrix(x) (x*3)+7
#define matrixtoblock(x) (x-7)/3
#define MATRIX "|                             |"
#define MATRIX_BOTTOM "|_____________________________|"
#define MATRIX_LENGTH 26
#define ESC_KEY 27

typedef enum {LEFT, RIGHT, UP, DOWN} Orientation; 
typedef enum {RED, GREEN, CYAN, BLUE, YELLOW, PURPLE, ORANGE, RANDOM} Color;
typedef enum {NEXT, HOLD} Display;

typedef struct block_coords {
    int x;
    int y;
} block_coords;

typedef struct tetrimo {
    block_coords current_xy[4];
    block_coords next_xy[4];
    Color color;
    Orientation current_orientation;
    void (*toggle)(void);
} tetrimo;

void initMatrix();
void draw(tetrimo t);
void elim(tetrimo t);
void drawRed(Display d);
void eraseRed(Display d);
void drawGreen(Display d);
void eraseGreen(Display d);
void drawCyan(Display d);
void eraseCyan(Display d);
void drawBlue(Display d);
void eraseBlue(Display d);
void drawYellow(Display d);
void eraseYellow(Display d);
void drawPurple(Display d);
void erasePurple(Display d);
void drawOrange(Display d);
void eraseOrange(Display d);
void shift(Orientation o);
void updateScoreLevel();
void update(Orientation o);
void toggleRed();
void toggleGreen();
void toggleCyan();
void toggleBlue();
void toggleYellow();
void togglePurple();
void toggleOrange();
void eraseNext(Color c);
void drawNext(Color c);
void newBlock(Color c);
void checkLine();
void save();
void load();
void drawHeld(Color c);
void eraseHeld(Color c);
void drawTitle(bool isSave);
void drawOptions();
void drawControls();
void drawGameOver();
bool checkDown();
void updateMatrix();

tetrimo currentTetrimo;

bool matrix[25][9];

int delay = 1000;
int speedcnt = 0;
int level = 1;

int max_y = 0;
int max_x = 0;


int pause_flg;
int toggle_flg;
int block_flg;

bool gameOver;
int score;

int is2Player;

Color next_tetrimo;
Color held_tetrimo;
bool heldExists;
bool heldLast;

int main(int argc, char *argv[]) {
    time_t t;
    srand((unsigned) time(&t));

    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    timeout(delay);
    getmaxyx(stdscr, max_y, max_x);

    START:
    int numOptions = 4;
    if(!fopen("save.txt", "r")){
        numOptions = 3;
        drawTitle(FALSE);
    } else {
        drawTitle(TRUE);
    }
    

    int option = 0;
    int arrow_pos = 13;
    bool title_flg;
    while(!title_flg) {
        switch(wgetch(stdscr)) {
            case KEY_UP:
                if(option != 0) {
                    option--;
                    mvprintw(arrow_pos, 23, "  ");
                    arrow_pos -= 2;
                    mvprintw(arrow_pos, 23, "->");
                }
                break;
            case KEY_DOWN:
                if(option != numOptions) {
                    option++;
                    mvprintw(arrow_pos, 23, "  ");
                    arrow_pos += 2;
                    mvprintw(arrow_pos, 23, "->");
                }
                break;
            case '\n':
                title_flg = TRUE;
            default:
                break;
        }
    }

    if(numOptions == 3) {
        switch (option)
        {
        case 1:
            option = 2;
            break;
        case 2:
            option = 3;
            break;
        case 3:
            option = 4;
            break;
        default:
            break;
        }
    }

    initMatrix();

    refresh();

    next_tetrimo = rand()%7;
    newBlock(RANDOM);

    int idle_cnt = 0;
    int cnt = 0;
    int o;
    switch(option) {
        case 0:
            break;
        case 1:
            load();
            break;
        case 2:
            is2Player = 30;
            break;
        case 3:
            clear();
            drawOptions();
            bool options_flg;
            int new_level;
            while(!options_flg) {
                switch (wgetch(stdscr))
                {
                case KEY_LEFT:
                    if(new_level != 1) {
                        new_level--;
                        mvprintw(13, 26, "Level: %d", new_level);
                    }
                    break;
                case KEY_RIGHT:
                    if(new_level != 20) {
                        new_level++;
                        mvprintw(13, 26, "Level: %d", new_level);
                    }
                    break;
                case ESC_KEY:
                    options_flg = 1;
                    break;
                default:
                    break;
                }
            }
            while(level != new_level) {
                updateScoreLevel();
            }
            score = 0;
            speedcnt = 0;
            options_flg = 0;
            title_flg = 0;
            option = 0;
            clear();
            goto START;
            break;
        case 4:
            clear();
            drawControls();
            while(wgetch(stdscr)!= ESC_KEY) {}
            title_flg = 0;
            option = 0;
            clear();
            goto START;
            break;
        default:
            break;
    }
    while(1) {
        if(pause_flg) {
            mvprintw(13,18, "PAUSED");
            while(wgetch(stdscr) != 'p'){}
            mvprintw(13,18, "      ");
            pause_flg = 0;
        }
        Orientation movement = DOWN;
        int key = wgetch(stdscr);
        switch (key)
        {
        case 'p':
            pause_flg = ~pause_flg;
            break;
        case '\t':
            if(cnt++ == 2) {
                cnt = 0;
                idle_cnt++;
                update(movement);
            }
            currentTetrimo.toggle();
            toggle_flg = 1;
            break;
        case KEY_RIGHT:
            if(cnt++ == 2) {
                cnt = 0;
                idle_cnt++;
                update(movement);
            }
            // next_x = current_x + 3;
            movement = RIGHT;
            break;
        case KEY_LEFT:
            if(cnt++ == 2) {
                cnt = 0;
                idle_cnt++;
                update(movement);
            }
            movement = LEFT;
            // next_x = current_x - 3;
            break;
        case 's':
            save();
            break;
        case 'z':
            save();
            endwin();
            exit(EXIT_SUCCESS);
            // f
            break;
        case ESC_KEY:
            option = 0;
            delay = 1000;
            speedcnt = 0;
            pause_flg = 0;
            toggle_flg = 0;
            block_flg = 0;
            score = 0;
            heldExists = FALSE;
            heldLast = FALSE;
            title_flg = FALSE;
            clear();
            goto START;
            break;
        case ' ':
            if(heldLast) {
                break;
            }
            heldLast = TRUE;
            elim(currentTetrimo);
            if(heldExists) {
                eraseNext(next_tetrimo);
                next_tetrimo = held_tetrimo;
            }
            eraseHeld(held_tetrimo);
            held_tetrimo = currentTetrimo.color;
            drawHeld(held_tetrimo); 
            heldExists = 1;
            newBlock(RANDOM);
            break;
        default:
            idle_cnt++;
            break;
        }
        if(!toggle_flg){
            update(movement);
        }
        if(gameOver) {
            drawGameOver();
            gameOver = FALSE;
            while(wgetch(stdscr) != ESC_KEY) {}
            title_flg = 0;
            option = 0;
            clear();
            goto START;
        }
        toggle_flg = 0;
        // mvprintw(0, 0, "%d, %d", current_x, current_y);
        // mvprintw(2, 0, "%d, %d", next_x, next_y);
        // mvprintw(4, 0, "%d", current_orientation);
        // mvprintw(6, 0, "%d", next_orientation);
        checkLine();
        refresh();
    }
    endwin();
}

void drawTitle(bool isSave) {
    mvprintw(5,5, "////////// ////// ////////// /////////  //////// ////////");
    mvprintw(6,5, "   //     //         //     //     //     //    //");
    mvprintw(7,5, "  //     //////     //     /////////     //     //////");
    mvprintw(8,5, " //     //         //     //    \\\\      //          //");
    mvprintw(9,5, "//     //////     //     //      \\\\  ///////  ///////");
    mvprintw(13, 23, "->");
    mvprintw(13, 26, "NEW GAME");
    if(isSave) {
        mvprintw(15, 26, "CONTINUE");
        mvprintw(17, 26, "2-PLAYER");
        mvprintw(19, 26, "OPTIONS");
        mvprintw(21, 26, "CONTROLS");
    } else {
        mvprintw(15, 23, "LOCAL 2 PLAYER");
        mvprintw(17, 26, "OPTIONS");
        mvprintw(19, 26, "CONTROLS");
    }
    
    
}

void drawControls() {
    mvprintw(5,5, "Tab..............Rotate block");
    mvprintw(6,5, "Down.............Drop block faster");
    mvprintw(7,5, "Left/Right.......Move left and Right");
    mvprintw(8,5, "Space............Hold block");
    mvprintw(9,5, "S................Save game");
    mvprintw(11,5, "P................Pause game");
    mvprintw(12,5, "Z................Save and quit game");
    mvprintw(12,5, "ESC..............Go back to title screen");
}

void drawOptions() {
    mvprintw(13, 23, "->");
    mvprintw(13, 26, "Level: %d", level);
    mvprintw(15, 22, "(ESC to go back)");
}

void newBlock(Color c) {
    tetrimo new_tetrimo;
    new_tetrimo.color = next_tetrimo;
    new_tetrimo.current_orientation = LEFT;

    switch(new_tetrimo.color) {
        case(RED):
            new_tetrimo.current_xy[0].x = 19;
            new_tetrimo.current_xy[0].y = 1;
            new_tetrimo.current_xy[1].x = 19;
            new_tetrimo.current_xy[1].y = 0;
            new_tetrimo.current_xy[2].x = 16;
            new_tetrimo.current_xy[2].y = 1;
            new_tetrimo.current_xy[3].x = 22;
            new_tetrimo.current_xy[3].y = 0;
            new_tetrimo.toggle = &toggleRed;
            break;
        case(GREEN):
            new_tetrimo.current_xy[0].x = 19;
            new_tetrimo.current_xy[0].y = 1;
            new_tetrimo.current_xy[1].x = 19;
            new_tetrimo.current_xy[1].y = 0;
            new_tetrimo.current_xy[2].x = 16;
            new_tetrimo.current_xy[2].y = 0;
            new_tetrimo.current_xy[3].x = 22;
            new_tetrimo.current_xy[3].y = 1;
            new_tetrimo.toggle = &toggleGreen;
            break;
        case(CYAN):
            new_tetrimo.current_xy[0].x = 22;
            new_tetrimo.current_xy[0].y = 3;
            new_tetrimo.current_xy[1].x = 22;
            new_tetrimo.current_xy[1].y = 2;
            new_tetrimo.current_xy[2].x = 22;
            new_tetrimo.current_xy[2].y = 1;
            new_tetrimo.current_xy[3].x = 22;
            new_tetrimo.current_xy[3].y = 0;
            new_tetrimo.toggle = &toggleCyan;
            break;
        case(BLUE):
            new_tetrimo.current_xy[0].x = 19;
            new_tetrimo.current_xy[0].y = 2;
            new_tetrimo.current_xy[1].x = 19;
            new_tetrimo.current_xy[1].y = 0;
            new_tetrimo.current_xy[2].x = 19;
            new_tetrimo.current_xy[2].y = 1;
            new_tetrimo.current_xy[3].x = 22;
            new_tetrimo.current_xy[3].y = 2;
            new_tetrimo.toggle = &toggleBlue;
            break;
        case(YELLOW):
            new_tetrimo.current_xy[0].x = 19;
            new_tetrimo.current_xy[0].y = 1;
            new_tetrimo.current_xy[1].x = 22;
            new_tetrimo.current_xy[1].y = 0;
            new_tetrimo.current_xy[2].x = 19;
            new_tetrimo.current_xy[2].y = 0;
            new_tetrimo.current_xy[3].x = 22;
            new_tetrimo.current_xy[3].y = 1;
            new_tetrimo.toggle = &toggleYellow;
            break;
        case(PURPLE):
            new_tetrimo.current_xy[0].x = 19;
            new_tetrimo.current_xy[0].y = 2;
            new_tetrimo.current_xy[1].x = 19;
            new_tetrimo.current_xy[1].y = 0;
            new_tetrimo.current_xy[2].x = 19;
            new_tetrimo.current_xy[2].y = 1;
            new_tetrimo.current_xy[3].x = 22;
            new_tetrimo.current_xy[3].y = 1;
            new_tetrimo.toggle = &togglePurple;
            break;
        case(ORANGE):
            new_tetrimo.current_xy[0].x = 19;
            new_tetrimo.current_xy[0].y = 2;
            new_tetrimo.current_xy[1].x = 19;
            new_tetrimo.current_xy[1].y = 1;
            new_tetrimo.current_xy[2].x = 16;
            new_tetrimo.current_xy[2].y = 2;
            new_tetrimo.current_xy[3].x = 19;
            new_tetrimo.current_xy[3].y = 0;
            new_tetrimo.toggle = &toggleOrange;
            break;
        default:
            break;
    }
    currentTetrimo = new_tetrimo;
    if(c==RANDOM) {
        next_tetrimo = rand() % 7;
    } else {
        next_tetrimo = c;
    }
    for(int i = 0; i < 4; i++) {
        currentTetrimo.next_xy[i].x = currentTetrimo.current_xy[i].x;
        currentTetrimo.next_xy[i].y = currentTetrimo.current_xy[i].y;
    }
    draw(currentTetrimo);
    eraseNext(new_tetrimo.color);
    drawNext(next_tetrimo);
}

void eraseNext(Color c) {
    for(int i = 2; i < 6; i++) {
        mvprintw(i, 39, "             ");
    }
}

void drawNext(Color c) {
    switch(c) {
        case(RED):
            drawRed(NEXT);
            break;
        case(GREEN):
            drawGreen(NEXT);
            break;
        case(CYAN):
            drawCyan(NEXT);
            break;
        case(BLUE):
            drawBlue(NEXT);
            break;
        case(YELLOW):
            drawYellow(NEXT);
            break;
        case(PURPLE):
            drawPurple(NEXT);
            break;
        case(ORANGE):
            drawOrange(NEXT);
            break;
        default:
            break;
    }
}

void eraseHeld(Color c) {
    switch(c) {
        case(RED):
            eraseRed(HOLD);
            break;
        case(GREEN):
            eraseGreen(HOLD);
            break;
        case(CYAN):
            eraseCyan(HOLD);
            break;
        case(BLUE):
            eraseBlue(HOLD);
            break;
        case(YELLOW):
            eraseYellow(HOLD);
            break;
        case(PURPLE):
            erasePurple(HOLD);
            break;
        case(ORANGE):
            eraseOrange(HOLD);
            break;
        default:
            break;
    }
}

void drawHeld(Color c) {
    switch(c) {
        case(RED):
            drawRed(HOLD);
            break;
        case(GREEN):
            drawGreen(HOLD);
            break;
        case(CYAN):
            drawCyan(HOLD);
            break;
        case(BLUE):
            drawBlue(HOLD);
            break;
        case(YELLOW):
            drawYellow(HOLD);
            break;
        case(PURPLE):
            drawPurple(HOLD);
            break;
        case(ORANGE):
            drawOrange(HOLD);
            break;
        default:
            break;
    }
}

void initMatrix() {
    clear();
    int i;
    for(i = 0; i < MATRIX_LENGTH - 1; i++) {
        mvprintw(i, 5, MATRIX);
    }
    mvprintw(i, 5, MATRIX_BOTTOM);

    for(int i = 0; i < 25; i++) {
        for(int j = 0; j < 9; j++) {
            matrix[i][j] = FALSE;
        }
    }
    mvprintw(26,16,"Score: %d", score);
    mvprintw(27,16,"Level: %d", level);
    mvprintw(0,38, "Next:");
    mvprintw(1,39, "_____________");
    for(int i = 2; i < 7; i++) {
        mvprintw(i,38,"|");
        mvprintw(i,52,"|");
    }
    mvprintw(6,39, "_____________");
    mvprintw(11,39, "_____________");
    for(int i = 12; i < 17; i++) {
        mvprintw(i,38,"|");
        mvprintw(i,52,"|");
    }
    mvprintw(16,39, "_____________");
    mvprintw(10,38, "Hold:");
}

void save() {
    FILE *savefp;
    savefp = fopen("save.txt", "w+");
    for(int i = 0; i < 25; i++) {
        for(int j = 0; j < 9; j++) {
            if(matrix[i][j] == TRUE){
                fputc('1', savefp);
            } else {
                fputc('0', savefp);
            }
        }
        fputc('\n', savefp);
    }
    fputc(currentTetrimo.color+48, savefp);
    fputc(' ', savefp);
    char stats[15];
    sprintf(stats, "%d %d %d %d", delay, speedcnt, level, score);
    fputs(stats, savefp);
    fputc('\n', savefp);
    char tets[10];
    sprintf(tets, "%d %d %d %d", next_tetrimo, heldExists, heldLast, held_tetrimo);
    fputs(tets, savefp);
    // Color next_tetrimo;
    // Color held_tetrimo;
    // bool heldExists;
    // bool heldLast;
    fputc('\n', savefp);
    fclose(savefp);
}

void load() {
    FILE *loadfp;
    if(!(loadfp = fopen("save.txt", "r+"))) {
        FILE *err;
        fopen("err.txt", "w+");
        fputs("Unable to open file save.txt", err);
        return;
    }
    char m[9];
    for(int i = 0; i < 25; i++) {
        // fgets(buff, 10, loadfp);
        fscanf(loadfp, "%s", m);
        for(int j = 0; j < 10; j++) {
            if(m[j]=='1') {
                matrix[i][j]=TRUE;
                paint(i, blocktomatrix(j));
            } else if(m[j]=='0'){
                matrix[i][j]==FALSE;
                whiteout(i, blocktomatrix(j));
            }
        }
    }
    char c[2];
    fscanf(loadfp, "%s", c);
    //mvprintw(30,0,"/%s/",c);
    // next_tetrimo = fgetc(loadfp);
    // mvprintw(1,0,"%c", next_tetrimo);
    eraseNext(next_tetrimo);
    next_tetrimo = atoi(c);
    char d[5];
    fscanf(loadfp, "%s", d);
    delay = atoi(d);
    char s[5];
    fscanf(loadfp, "%s", s);
    speedcnt = atoi(s);
    char l[2];
    fscanf(loadfp, "%s", l);
    level = atoi(l);
    mvprintw(27,21, "     ");
    mvprintw(27,16,"Level: %d", level);
    char sc[2];
    fscanf(loadfp, "%s", sc);
    score = atoi(sc);
    char n[2];
    fscanf(loadfp, "%s", n);
    newBlock(atoi(n));
    char he[2];
    fscanf(loadfp, "%s", he);
    heldExists = atoi(he);
    if(heldExists) {
        char hl[2];
        fscanf(loadfp, "%s", hl);
        heldLast = atoi(hl);
        char h[2];
        fscanf(loadfp, "%s", h);
        eraseHeld(held_tetrimo);
        held_tetrimo = atoi(h);
        drawHeld(held_tetrimo);
    }
    fclose(loadfp);
}

bool checkRight() {
    if(currentTetrimo.current_xy[3].x >= 29) {
        return true;
    }
    for(int i = 0; i < 25; i++) {
        for(int j = 0; j < 9; j++) {
            if(matrix[i][j] == TRUE) {
                for(int k = 0; k < 4; k++) {
                    if(currentTetrimo.next_xy[k].y == i 
                    && currentTetrimo.next_xy[k].x == blocktomatrix(j)){
                        block_flg = 1;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool checkLeft() {
    if(currentTetrimo.current_xy[2].x <= 8) {
        return true;
    }
    for(int i = 0; i < 25; i++) {
        for(int j = 0; j < 9; j++) {
            if(matrix[i][j] == TRUE) {
                for(int k = 0; k < 4; k++) {
                    if(currentTetrimo.next_xy[k].y == i 
                    && currentTetrimo.next_xy[k].x == blocktomatrix(j)){
                        block_flg = 1;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool checkDown() {
    if(currentTetrimo.current_xy[0].y >= MATRIX_LENGTH-2) {
        block_flg = 1;
        return true;
    }
    for(int i = 0; i < 25; i++) {
        for(int j = 0; j < 9; j++) {
            if(matrix[i][j] == TRUE) {
                for(int k = 0; k < 4; k++) {
                    if(currentTetrimo.next_xy[k].y == i 
                    && currentTetrimo.next_xy[k].x == blocktomatrix(j)){
                        block_flg = 1;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}


void toggleRed() {
    if(currentTetrimo.current_orientation == LEFT) {
        currentTetrimo.next_xy[0].x = currentTetrimo.current_xy[0].x+3;
        currentTetrimo.next_xy[0].y = currentTetrimo.current_xy[0].y;
        currentTetrimo.next_xy[1].x = currentTetrimo.current_xy[1].x;
        currentTetrimo.next_xy[1].y = currentTetrimo.current_xy[1].y;
        currentTetrimo.next_xy[2].x = currentTetrimo.current_xy[2].x+3;
        currentTetrimo.next_xy[2].y = currentTetrimo.current_xy[2].y-2;
        currentTetrimo.next_xy[3].x = currentTetrimo.current_xy[3].x;
        currentTetrimo.next_xy[3].y = currentTetrimo.current_xy[3].y;
        currentTetrimo.current_orientation = RIGHT;  
    } else {
        if(checkLeft()) {
            return;
        }
        currentTetrimo.next_xy[0].x = currentTetrimo.current_xy[0].x-3;
        currentTetrimo.next_xy[0].y = currentTetrimo.current_xy[0].y;
        currentTetrimo.next_xy[1].x = currentTetrimo.current_xy[1].x;
        currentTetrimo.next_xy[1].y = currentTetrimo.current_xy[1].y;
        currentTetrimo.next_xy[2].x = currentTetrimo.current_xy[2].x-3;
        currentTetrimo.next_xy[2].y = currentTetrimo.current_xy[2].y+2;
        currentTetrimo.next_xy[3].x = currentTetrimo.current_xy[3].x;
        currentTetrimo.next_xy[3].y = currentTetrimo.current_xy[3].y;
        currentTetrimo.current_orientation = LEFT;
    }
    elim(currentTetrimo);
    draw(currentTetrimo);
    for(int i = 0; i < 4; i++) {
        currentTetrimo.current_xy[i].x = currentTetrimo.next_xy[i].x;
        currentTetrimo.current_xy[i].y = currentTetrimo.next_xy[i].y;
    }
}

void toggleGreen() {
    if(currentTetrimo.current_orientation == LEFT) {
        currentTetrimo.next_xy[0].x = currentTetrimo.current_xy[0].x;
        currentTetrimo.next_xy[0].y = currentTetrimo.current_xy[0].y;
        currentTetrimo.next_xy[1].x = currentTetrimo.current_xy[1].x+3;
        currentTetrimo.next_xy[1].y = currentTetrimo.current_xy[1].y-1;
        currentTetrimo.next_xy[2].x = currentTetrimo.current_xy[2].x+3;
        currentTetrimo.next_xy[2].y = currentTetrimo.current_xy[2].y;
        currentTetrimo.next_xy[3].x = currentTetrimo.current_xy[3].x;
        currentTetrimo.next_xy[3].y = currentTetrimo.current_xy[3].y-1;
        currentTetrimo.current_orientation = RIGHT;  
    } else {
        if(checkLeft()) {
            return;
        }
        currentTetrimo.next_xy[0].x = currentTetrimo.current_xy[0].x;
        currentTetrimo.next_xy[0].y = currentTetrimo.current_xy[0].y;
        currentTetrimo.next_xy[1].x = currentTetrimo.current_xy[1].x-3;
        currentTetrimo.next_xy[1].y = currentTetrimo.current_xy[1].y+1;
        currentTetrimo.next_xy[2].x = currentTetrimo.current_xy[2].x-3;
        currentTetrimo.next_xy[2].y = currentTetrimo.current_xy[2].y;
        currentTetrimo.next_xy[3].x = currentTetrimo.current_xy[3].x;
        currentTetrimo.next_xy[3].y = currentTetrimo.current_xy[3].y+1;
        currentTetrimo.current_orientation = LEFT;
    }
    elim(currentTetrimo);
    draw(currentTetrimo);
    for(int i = 0; i < 4; i++) {
        currentTetrimo.current_xy[i].x = currentTetrimo.next_xy[i].x;
        currentTetrimo.current_xy[i].y = currentTetrimo.next_xy[i].y;
    }
}

void toggleCyan() {
    if(currentTetrimo.current_orientation == LEFT) {
        if(currentTetrimo.current_xy[2].x <= 11 || 
           checkRight()) {
            return;
        }
        currentTetrimo.next_xy[0].x = currentTetrimo.current_xy[0].x;
        currentTetrimo.next_xy[0].y = currentTetrimo.current_xy[0].y-1;
        currentTetrimo.next_xy[1].x = currentTetrimo.current_xy[1].x-3;
        currentTetrimo.next_xy[1].y = currentTetrimo.current_xy[1].y;
        currentTetrimo.next_xy[2].x = currentTetrimo.current_xy[2].x-6;
        currentTetrimo.next_xy[2].y = currentTetrimo.current_xy[2].y+1;
        currentTetrimo.next_xy[3].x = currentTetrimo.current_xy[3].x+3;
        currentTetrimo.next_xy[3].y = currentTetrimo.current_xy[3].y+2;
        currentTetrimo.current_orientation = RIGHT;  
    } else {
        currentTetrimo.next_xy[0].x = currentTetrimo.current_xy[0].x;
        currentTetrimo.next_xy[0].y = currentTetrimo.current_xy[0].y+1;
        currentTetrimo.next_xy[1].x = currentTetrimo.current_xy[1].x+3;
        currentTetrimo.next_xy[1].y = currentTetrimo.current_xy[1].y;
        currentTetrimo.next_xy[2].x = currentTetrimo.current_xy[2].x+6;
        currentTetrimo.next_xy[2].y = currentTetrimo.current_xy[2].y-1;
        currentTetrimo.next_xy[3].x = currentTetrimo.current_xy[3].x-3;
        currentTetrimo.next_xy[3].y = currentTetrimo.current_xy[3].y-2;
        currentTetrimo.current_orientation = LEFT;
    }
    elim(currentTetrimo);
    draw(currentTetrimo);
    for(int i = 0; i < 4; i++) {
        currentTetrimo.current_xy[i].x = currentTetrimo.next_xy[i].x;
        currentTetrimo.current_xy[i].y = currentTetrimo.next_xy[i].y;
    }
}

void toggleBlue() {
    if(currentTetrimo.current_orientation == LEFT) {
        if(checkLeft()) {
            return;
        }
        currentTetrimo.next_xy[0].x = currentTetrimo.current_xy[0].x-3;
        currentTetrimo.next_xy[0].y = currentTetrimo.current_xy[0].y;
        currentTetrimo.next_xy[1].x = currentTetrimo.current_xy[1].x;
        currentTetrimo.next_xy[1].y = currentTetrimo.current_xy[1].y+1;
        currentTetrimo.next_xy[2].x = currentTetrimo.current_xy[2].x-3;
        currentTetrimo.next_xy[2].y = currentTetrimo.current_xy[2].y;
        currentTetrimo.next_xy[3].x = currentTetrimo.current_xy[3].x;
        currentTetrimo.next_xy[3].y = currentTetrimo.current_xy[3].y-1;
        currentTetrimo.current_orientation = UP;  
    } else if(currentTetrimo.current_orientation == UP) {
        currentTetrimo.next_xy[0].x = currentTetrimo.current_xy[0].x+3;
        currentTetrimo.next_xy[0].y = currentTetrimo.current_xy[0].y;
        currentTetrimo.next_xy[1].x = currentTetrimo.current_xy[1].x;
        currentTetrimo.next_xy[1].y = currentTetrimo.current_xy[1].y;
        currentTetrimo.next_xy[2].x = currentTetrimo.current_xy[2].x;
        currentTetrimo.next_xy[2].y = currentTetrimo.current_xy[2].y-1;
        currentTetrimo.next_xy[3].x = currentTetrimo.current_xy[3].x-3;
        currentTetrimo.next_xy[3].y = currentTetrimo.current_xy[3].y-1;
        currentTetrimo.current_orientation = RIGHT;
    } else if(currentTetrimo.current_orientation == RIGHT) {
        if(checkRight()) {
            return;
        }
        currentTetrimo.next_xy[0].x = currentTetrimo.current_xy[0].x+3;
        currentTetrimo.next_xy[0].y = currentTetrimo.current_xy[0].y-1;
        currentTetrimo.next_xy[1].x = currentTetrimo.current_xy[1].x;
        currentTetrimo.next_xy[1].y = currentTetrimo.current_xy[1].y;
        currentTetrimo.next_xy[2].x = currentTetrimo.current_xy[2].x;
        currentTetrimo.next_xy[2].y = currentTetrimo.current_xy[2].y+1;
        currentTetrimo.next_xy[3].x = currentTetrimo.current_xy[3].x+3;
        currentTetrimo.next_xy[3].y = currentTetrimo.current_xy[3].y;
        currentTetrimo.current_orientation = DOWN;
    } else if(currentTetrimo.current_orientation == DOWN) {
        currentTetrimo.next_xy[0].x = currentTetrimo.current_xy[0].x-3;
        currentTetrimo.next_xy[0].y = currentTetrimo.current_xy[0].y+1;
        currentTetrimo.next_xy[1].x = currentTetrimo.current_xy[1].x;
        currentTetrimo.next_xy[1].y = currentTetrimo.current_xy[1].y-1;
        currentTetrimo.next_xy[2].x = currentTetrimo.current_xy[2].x+3;
        currentTetrimo.next_xy[2].y = currentTetrimo.current_xy[2].y;
        currentTetrimo.next_xy[3].x = currentTetrimo.current_xy[3].x;
        currentTetrimo.next_xy[3].y = currentTetrimo.current_xy[3].y+2;
        currentTetrimo.current_orientation = LEFT;
    }
    elim(currentTetrimo);
    draw(currentTetrimo);
    for(int i = 0; i < 4; i++) {
        currentTetrimo.current_xy[i].x = currentTetrimo.next_xy[i].x;
        currentTetrimo.current_xy[i].y = currentTetrimo.next_xy[i].y;
    }
}

void toggleYellow() {
    elim(currentTetrimo);
    draw(currentTetrimo);
    for(int i = 0; i < 4; i++) {
        currentTetrimo.current_xy[i].x = currentTetrimo.next_xy[i].x;
        currentTetrimo.current_xy[i].y = currentTetrimo.next_xy[i].y;
    }
}

void togglePurple() {
    if(currentTetrimo.current_orientation == LEFT) {
        if(checkLeft()) {
            return;
        }
        currentTetrimo.next_xy[0].x = currentTetrimo.current_xy[0].x;
        currentTetrimo.next_xy[0].y = currentTetrimo.current_xy[0].y;
        currentTetrimo.next_xy[1].x = currentTetrimo.current_xy[1].x;
        currentTetrimo.next_xy[1].y = currentTetrimo.current_xy[1].y+1;
        currentTetrimo.next_xy[2].x = currentTetrimo.current_xy[2].x-3;
        currentTetrimo.next_xy[2].y = currentTetrimo.current_xy[2].y;
        currentTetrimo.next_xy[3].x = currentTetrimo.current_xy[3].x;
        currentTetrimo.next_xy[3].y = currentTetrimo.current_xy[3].y;
        currentTetrimo.current_orientation = UP;  
    } else if(currentTetrimo.current_orientation == UP) {
        currentTetrimo.next_xy[0].x = currentTetrimo.current_xy[0].x;
        currentTetrimo.next_xy[0].y = currentTetrimo.current_xy[0].y;
        currentTetrimo.next_xy[1].x = currentTetrimo.current_xy[1].x;
        currentTetrimo.next_xy[1].y = currentTetrimo.current_xy[1].y;
        currentTetrimo.next_xy[2].x = currentTetrimo.current_xy[2].x;
        currentTetrimo.next_xy[2].y = currentTetrimo.current_xy[2].y;
        currentTetrimo.next_xy[3].x = currentTetrimo.current_xy[3].x-3;
        currentTetrimo.next_xy[3].y = currentTetrimo.current_xy[3].y-1;
        currentTetrimo.current_orientation = RIGHT;
    } else if(currentTetrimo.current_orientation == RIGHT) {
        if(checkRight()) {
            return;
        }
        currentTetrimo.next_xy[0].x = currentTetrimo.current_xy[0].x;
        currentTetrimo.next_xy[0].y = currentTetrimo.current_xy[0].y-1;
        currentTetrimo.next_xy[1].x = currentTetrimo.current_xy[1].x;
        currentTetrimo.next_xy[1].y = currentTetrimo.current_xy[1].y-1;
        currentTetrimo.next_xy[2].x = currentTetrimo.current_xy[2].x;
        currentTetrimo.next_xy[2].y = currentTetrimo.current_xy[2].y;
        currentTetrimo.next_xy[3].x = currentTetrimo.current_xy[3].x+3;
        currentTetrimo.next_xy[3].y = currentTetrimo.current_xy[3].y+1;
        currentTetrimo.current_orientation = DOWN;
    } else if(currentTetrimo.current_orientation == DOWN) {
        currentTetrimo.next_xy[0].x = currentTetrimo.current_xy[0].x;
        currentTetrimo.next_xy[0].y = currentTetrimo.current_xy[0].y+1;
        currentTetrimo.next_xy[1].x = currentTetrimo.current_xy[1].x;
        currentTetrimo.next_xy[1].y = currentTetrimo.current_xy[1].y;
        currentTetrimo.next_xy[2].x = currentTetrimo.current_xy[2].x+3;
        currentTetrimo.next_xy[2].y = currentTetrimo.current_xy[2].y;
        currentTetrimo.next_xy[3].x = currentTetrimo.current_xy[3].x;
        currentTetrimo.next_xy[3].y = currentTetrimo.current_xy[3].y;
        currentTetrimo.current_orientation = LEFT;
    }
    elim(currentTetrimo);
    draw(currentTetrimo);
    for(int i = 0; i < 4; i++) {
        currentTetrimo.current_xy[i].x = currentTetrimo.next_xy[i].x;
        currentTetrimo.current_xy[i].y = currentTetrimo.next_xy[i].y;
    }
}

void toggleOrange() {
    if(currentTetrimo.current_orientation == LEFT) {
        if(checkRight()) {
            return;
        }
        currentTetrimo.next_xy[0].x = currentTetrimo.current_xy[0].x-3;
        currentTetrimo.next_xy[0].y = currentTetrimo.current_xy[0].y-1;
        currentTetrimo.next_xy[1].x = currentTetrimo.current_xy[1].x;
        currentTetrimo.next_xy[1].y = currentTetrimo.current_xy[1].y;
        currentTetrimo.next_xy[2].x = currentTetrimo.current_xy[2].x;
        currentTetrimo.next_xy[2].y = currentTetrimo.current_xy[2].y-2;
        currentTetrimo.next_xy[3].x = currentTetrimo.current_xy[3].x+3;
        currentTetrimo.next_xy[3].y = currentTetrimo.current_xy[3].y+1;
        currentTetrimo.current_orientation = UP;  
    } else if(currentTetrimo.current_orientation == UP) {
        currentTetrimo.next_xy[0].x = currentTetrimo.current_xy[0].x+3;
        currentTetrimo.next_xy[0].y = currentTetrimo.current_xy[0].y+1;
        currentTetrimo.next_xy[1].x = currentTetrimo.current_xy[1].x;
        currentTetrimo.next_xy[1].y = currentTetrimo.current_xy[1].y;
        currentTetrimo.next_xy[2].x = currentTetrimo.current_xy[2].x+3;
        currentTetrimo.next_xy[2].y = currentTetrimo.current_xy[2].y;
        currentTetrimo.next_xy[3].x = currentTetrimo.current_xy[3].x;
        currentTetrimo.next_xy[3].y = currentTetrimo.current_xy[3].y-1;
        currentTetrimo.current_orientation = RIGHT;
    } else if(currentTetrimo.current_orientation == RIGHT) {
        if(checkLeft()) {
            return;
        }
        currentTetrimo.next_xy[0].x = currentTetrimo.current_xy[0].x+3;
        currentTetrimo.next_xy[0].y = currentTetrimo.current_xy[0].y;
        currentTetrimo.next_xy[1].x = currentTetrimo.current_xy[1].x;
        currentTetrimo.next_xy[1].y = currentTetrimo.current_xy[1].y;
        currentTetrimo.next_xy[2].x = currentTetrimo.current_xy[2].x-3;
        currentTetrimo.next_xy[2].y = currentTetrimo.current_xy[2].y+1;
        currentTetrimo.next_xy[3].x = currentTetrimo.current_xy[3].x;
        currentTetrimo.next_xy[3].y = currentTetrimo.current_xy[3].y+1;
        currentTetrimo.current_orientation = DOWN;
    } else if(currentTetrimo.current_orientation == DOWN) {
        currentTetrimo.next_xy[0].x = currentTetrimo.current_xy[0].x-3;
        currentTetrimo.next_xy[0].y = currentTetrimo.current_xy[0].y;
        currentTetrimo.next_xy[1].x = currentTetrimo.current_xy[1].x;
        currentTetrimo.next_xy[1].y = currentTetrimo.current_xy[1].y;
        currentTetrimo.next_xy[2].x = currentTetrimo.current_xy[2].x;
        currentTetrimo.next_xy[2].y = currentTetrimo.current_xy[2].y+1;
        currentTetrimo.next_xy[3].x = currentTetrimo.current_xy[3].x-3;
        currentTetrimo.next_xy[3].y = currentTetrimo.current_xy[3].y-1;
        currentTetrimo.current_orientation = LEFT;
    }
    elim(currentTetrimo);
    draw(currentTetrimo);
    for(int i = 0; i < 4; i++) {
        currentTetrimo.current_xy[i].x = currentTetrimo.next_xy[i].x;
        currentTetrimo.current_xy[i].y = currentTetrimo.next_xy[i].y;
    }
}

void hitMatrix(int x, int y, bool hit) {
    matrix[y][matrixtoblock(x)] = hit;
}

void updateMatrix() {
    for(int i = 0; i < 4; i++) {
        // mvprintw(4+i,0,"%d, %d", currentTetrimo.current_xy[i].x, currentTetrimo.current_xy[i].y);
        hitMatrix(currentTetrimo.current_xy[i].x, currentTetrimo.current_xy[i].y, TRUE);
    }
}

void eraseLine(int y) {
    for(int i = 0; i < 9; i++) {
        whiteout(y, (i*3)+7);
        matrix[y][i] = FALSE;
    }
}

void replaceLines(int first, int last) {
    int final = 26;
    for(int i = 0; i < 25; i++) {
        for(int j = 0; j < 9; j++) {
            if(matrix[i][j] == TRUE) {
                final = i;
                break;
            }
        }
        if(final != 26){
            break;
        }
    }
    for(int i = first, k = 0; i >= final; i--, k++) {
        eraseLine(i);
        for(int j = 0; j < 9; j++) {
            if(matrix[last-1-k][j] == TRUE) {
                matrix[i][j] = TRUE;
                paint(i, blocktomatrix(j));
            }
        }
    }
}

void updateScoreLevel() {
    score += 100;
    speedcnt += 100;
    if(speedcnt >= 1000) {
        if(delay <= 100) {
            delay -= 10;
        } else {
            delay -= 100;
        }
        timeout(delay);
        speedcnt = 0;
        level++;
    }
}

void checkLine() {
    int first = 0;
    int last = 0;
    for(int i = 24; i >= 0; i--) {
        for(int j = 0; j < 9; j++) {
            if(matrix[i][j] == FALSE) {
                break;
            }
            if(j == 8) {
                updateScoreLevel();
                mvprintw(27,21, "     ");
                mvprintw(27,16,"Level: %d", level);
                last = i;
                if(first == 0) {
                    first = i;
                }
            }
        } 
    }
    if(first != 0 && last != 0) {
        replaceLines(first, last);
    }
    mvprintw(26,21, "     ");
    mvprintw(26,16,"Score: %d", score);
}

void shift(Orientation o) {
    switch (o)
    {
    case RIGHT:
        for(int i = 0; i < 4; i++) {
            currentTetrimo.next_xy[i].x = currentTetrimo.current_xy[i].x+3;
            currentTetrimo.next_xy[i].y = currentTetrimo.current_xy[i].y;
        }
        if(checkRight()) {
            for(int i = 0; i < 4; i++) {
                currentTetrimo.next_xy[i].x = currentTetrimo.current_xy[i].x;
                currentTetrimo.next_xy[i].y = currentTetrimo.current_xy[i].y;
            }
        }
        break;
    case LEFT:
        for(int i = 0; i < 4; i++) {
                currentTetrimo.next_xy[i].x = currentTetrimo.current_xy[i].x-3;
                currentTetrimo.next_xy[i].y = currentTetrimo.current_xy[i].y;
        }
        if(checkLeft()) {
            for(int i = 0; i < 4; i++) {
                currentTetrimo.next_xy[i].x = currentTetrimo.current_xy[i].x;
                currentTetrimo.next_xy[i].y = currentTetrimo.current_xy[i].y;
            }
        }
        break;
    case DOWN:
        for(int i = 0; i < 4; i++) {
                currentTetrimo.next_xy[i].x = currentTetrimo.current_xy[i].x;
                currentTetrimo.next_xy[i].y = currentTetrimo.current_xy[i].y+1;
        }
        if(checkDown()) {
            block_flg = 1;
            heldLast = FALSE;
            for(int i = 0; i < 4; i++) {
                currentTetrimo.next_xy[i].x = currentTetrimo.current_xy[i].x;
                currentTetrimo.next_xy[i].y = currentTetrimo.current_xy[i].y;
                if(currentTetrimo.current_xy[i].y==0) {
                    gameOver = TRUE;
                }
            }
            updateMatrix();
        }
        break;
    default:
        break;
    }
}

void drawGameOver() {
    clear();
    mvprintw(15,15,"GAME OVER");
}

void update(Orientation o) {
    shift(o);
    if(block_flg){
        block_flg = 0;
        updateMatrix();
        newBlock(RANDOM);
    } else {
        elim(currentTetrimo);
        draw(currentTetrimo);
        for(int i = 0; i < 4; i++) {
            currentTetrimo.current_xy[i].x = currentTetrimo.next_xy[i].x;
            currentTetrimo.current_xy[i].y = currentTetrimo.next_xy[i].y;
        }
    }
}

void elim(tetrimo t) {
    for(int i = 0; i < 4; i++) {
        whiteout(t.current_xy[i].y, t.current_xy[i].x);
    }
}

void draw(tetrimo t) {
    for(int i = 0; i < 4; i++) {
        paint(t.next_xy[i].y, t.next_xy[i].x);
    }
}

void eraseRed(Display d) {
    if(d == NEXT) {
        whiteout(4, 41);
        whiteout(4, 44);
        whiteout(3, 44);
        whiteout(3, 47);
    } else {
        whiteout(14, 41);
        whiteout(14, 44);
        whiteout(13, 44);
        whiteout(13, 47);
    }
}

void drawRed(Display d) {
    if(d == NEXT) {
        paint(4, 41);
        paint(4, 44);
        paint(3, 44);
        paint(3, 47);
    } else {
        paint(14, 41);
        paint(14, 44);
        paint(13, 44);
        paint(13, 47);
    }
}

void eraseGreen(Display d) {
    if(d == NEXT) {
        whiteout(3, 41);
        whiteout(3, 44);
        whiteout(4, 44);
        whiteout(4, 47);
    } else {
        whiteout(13, 41);
        whiteout(13, 44);
        whiteout(14, 44);
        whiteout(14, 47);
    }
}

void drawGreen(Display d) {
    if(d == NEXT) {
        paint(3, 41);
        paint(3, 44);
        paint(4, 44);
        paint(4, 47);
    } else {
        paint(13, 41);
        paint(13, 44);
        paint(14, 44);
        paint(14, 47);
    }
}

void drawCyan(Display d) {
    if(d == NEXT) {
        paint(2, 44);
        paint(3, 44);
        paint(4, 44);
        paint(5, 44);
    } else {
        paint(12, 44);
        paint(13, 44);
        paint(14, 44);
        paint(15, 44);
    }
}

void eraseCyan(Display d) {
    if(d == NEXT) {
        whiteout(2, 44);
        whiteout(3, 44);
        whiteout(4, 44);
        whiteout(5, 44);
    } else {
        whiteout(12, 44);
        whiteout(13, 44);
        whiteout(14, 44);
        whiteout(15, 44);
    }
}

void drawBlue(Display d) {
    if(d == NEXT) {
        paint(2, 44);
        paint(3, 44);
        paint(4, 44);
        paint(4, 47);
    } else {
        paint(12, 44);
        paint(13, 44);
        paint(14, 44);
        paint(14, 47);
    }
}

void eraseBlue(Display d) {
    if(d == NEXT) {
        whiteout(2, 44);
        whiteout(3, 44);
        whiteout(4, 44);
        whiteout(4, 47);
    } else {
        whiteout(12, 44);
        whiteout(13, 44);
        whiteout(14, 44);
        whiteout(14, 47);
    }
}

void drawYellow(Display d) {
    if(d == NEXT) {
        paint(2, 41);
        paint(2, 44);
        paint(3, 41);
        paint(3, 44);
    } else {
        paint(12, 41);
        paint(12, 44);
        paint(13, 41);
        paint(13, 44);
    }
}

void eraseYellow(Display d) {
    if(d == NEXT) {
        whiteout(2, 41);
        whiteout(2, 44);
        whiteout(3, 41);
        whiteout(3, 44);
    } else {
        whiteout(12, 41);
        whiteout(12, 44);
        whiteout(13, 41);
        whiteout(13, 44);
    }
}

void drawPurple(Display d) {
    if(d == NEXT) {
        paint(2, 41);
        paint(3, 41);
        paint(3, 44);
        paint(4, 41);
    } else {
        paint(12, 41);
        paint(13, 41);
        paint(13, 44);
        paint(14, 41);
    }
}

void erasePurple(Display d) {
    if(d == NEXT) {
        whiteout(2, 41);
        whiteout(3, 41);
        whiteout(3, 44);
        whiteout(4, 41);
    } else {
        whiteout(12, 41);
        whiteout(13, 41);
        whiteout(13, 44);
        whiteout(14, 41);
    }
}

void drawOrange(Display d) {
    if(d == NEXT) {
        paint(4, 41);
        paint(4, 44);
        paint(3, 41);
        paint(2, 41);
    } else {
        paint(14, 41);
        paint(14, 44);
        paint(13, 41);
        paint(12, 41);
    }
}

void eraseOrange(Display d) {
    if(d == NEXT) {
        whiteout(4, 41);
        whiteout(4, 44);
        whiteout(3, 41);
        whiteout(2, 41);
    } else {
        whiteout(14, 41);
        whiteout(14, 44);
        whiteout(13, 41);
        whiteout(12, 41);
    }
}