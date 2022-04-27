#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
//#include <pthread.h>

#include "tcp_client.h"
#include "tcp_server.h"

// to compile for windows: gcc -I/mingw64/include/ncurses -o tetris.exe tetris.c tcp_client.c -lncurses -lws2_32 -L/mingw64/bin -static
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
#define log(x) fputs(x, err);
#define blocktomatrix(x) (x*3)+7
#define blocktomatrix2(x) (x*3)+62
#define matrixtoblock(x) (x-7)/3
#define MATRIX "|                             |"
#define MATRIX_BOTTOM "|_____________________________|"
#define MATRIX_LENGTH 26
#define MATRIX_WIDTH 9
#define ESC_KEY 27
#define INITIAL_DELAY 1000
#define ARROW_X 23

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
    void (*toggle)(struct tetrimo*, bool[MATRIX_LENGTH-1][MATRIX_WIDTH]);
} tetrimo;

void initMatrix(bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]);
void drawBoard(int score, int level, int offset);
void draw(tetrimo t);
void elim(tetrimo t);
void drawRed(Display d, int offset);
void eraseRed(Display d, int offset);
void drawGreen(Display d, int offset);
void eraseGreen(Display d, int offset);
void drawCyan(Display d, int offset);
void eraseCyan(Display d, int offset);
void drawBlue(Display d, int offset);
void eraseBlue(Display d, int offset);
void drawYellow(Display d, int offset);
void eraseYellow(Display d, int offset);
void drawPurple(Display d, int offset);
void erasePurple(Display d, int offset);
void drawOrange(Display d, int offset);
void eraseOrange(Display d, int offset);
bool shift(Orientation o, tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]);
void updateScoreLevel(int *score, int *level, int *speedcnt, int *delay);
bool update(Orientation o, tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH], bool *gameOver);
void toggleRed(struct tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]);
void toggleGreen(struct tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]);
void toggleCyan(struct tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]);
void toggleBlue(struct tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]);
void toggleYellow(struct tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]);
void togglePurple(struct tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]);
void toggleOrange(struct tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]);
void eraseNext(Color c, int offset);
void drawNext(Color c, int offset);
tetrimo newBlock(Color c, Color *next_tetrimo, int offset);
bool checkLine();
void save(bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH], Color current_color, int delay, int speedcnt, int level, int score, Color next_tetrimo, bool heldExists, bool heldLast, Color held_tetrimo);
void load(bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH], Color *current_color, int *delay, int *speedcnt, int *level, int *score, Color *next_tetrimo, bool *heldExists, bool *heldLast, Color *held_tetrimo);
void drawHeld(Color c, int offset);
void eraseHeld(Color c, int offset);
void drawTitle(bool isSave);
void drawOptions(int level);
void drawControls();
void drawGameOver();
bool checkDown(tetrimo t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]);
void updateMatrix(tetrimo t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]);
void play(bool is2Player, bool isLoad, bool isClient);
int hostOrClient();
void drawSecondPlayer(char *second);

int max_y = 0;
int max_x = 0;

//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

bool gameOver;


    FILE *err;

int main(int argc, char *argv[]) {
    err = fopen("err.txt", "w+");
    time_t t;
    srand((unsigned) time(&t));

    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    timeout(INITIAL_DELAY);
    int level = 1;
    int score = 0;
    int speedcnt = 0;
    int delay = 1000;
    while(1) {
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
                        mvprintw(arrow_pos, ARROW_X, "  ");
                        arrow_pos -= 2;
                        mvprintw(arrow_pos, ARROW_X, "->");
                    }
                    break;
                case KEY_DOWN:
                    if(option != numOptions) {
                        option++;
                        mvprintw(arrow_pos, ARROW_X, "  ");
                        arrow_pos += 2;
                        mvprintw(arrow_pos, ARROW_X, "->");
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


        refresh();

        
        int player;
        switch(option) {
            case 0:
                clear();
                play(false, false, false);
                break;
            case 1:
                clear();
                play(false, true, false);
                break;
            case 2:
                clear();
                //play(true, false);
                int client = hostOrClient();
                if(client) {
                                        // char q[3];
                    // sprintf(q, "%d", c);
                    // log(q);
                    // if(c != -1 && c != 1) {
                    //     log("Connected!");
                        
                    //     fclose(err);
                    // } else {
                    //     log("Connect failed");
                    //     fclose(err);
                    // }
                    // char message[10] = "Test";
                    // tcp_client_send_request(&sock, message);
                    // tcp_client_receive_response(&sock);
                    // tcp_client_close(sock);
                    play(true, false, true);
                } else {
                    play(true, false, false);
                }
                break;
            case 3:
                clear();
                drawOptions(level);
                bool options_flg;
                int new_level = 1;
                while(!options_flg) {
                    switch (wgetch(stdscr))
                    {
                    case KEY_LEFT:
                        if(new_level != 1) {
                            new_level--;
                            mvprintw(13, 26, "Level: %2d", new_level);
                        }
                        break;
                    case KEY_RIGHT:
                        if(new_level != 20) {
                            new_level++;
                            mvprintw(13, 26, "Level: %2d", new_level);
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
                    updateScoreLevel(&score, &level, &speedcnt, &delay);
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
        
        
        title_flg = 0;
        option = 0;
    }
    endwin();
}

void play(bool is2Player, bool isLoad, bool isClient) {
    // char s[2];
    // sprintf(s, "%d", *p);
    // log(s);
    bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH];
    int score = 0;
    Color held_tetrimo;
    int delay = 1000;
    int speedcnt = 0;
    int level = 1;

    bool pause_flg = false;
    bool toggle_flg = false;
    bool block_flg = false;

    bool gameOver = false;
    
    bool heldExists = false;
    bool heldLast = false;
    

    initMatrix(matrix);
    
    Color next_tetrimo = rand()%7;
    Color c = RANDOM;
    
    int cnt = 0;
    
    tetrimo t = newBlock(c, &next_tetrimo, 0);

    // bool matrix2[MATRIX_LENGTH-1][MATRIX_WIDTH];
    // int score2;
    // Color held_tetrimo2;
    // int delay2;
    // int speedcnt2;
    // int level2;

    // bool toggle_flg2;
    // bool block_flg2;

    // bool gameOver;
    
    // bool heldExists2;
    // bool heldLast2;
    // Color next_tetrimo2;
    // Color c2;
    int offset;
        
    // int cnt2;
    
    if(!is2Player && !isLoad) {
        drawBoard(score, level, 0);
    } else if(isLoad) {
        drawBoard(score, level, 0);
        load(matrix, &c, &delay, &speedcnt, &level, &score, &next_tetrimo, &heldExists, &heldLast, &held_tetrimo);
        timeout(delay);
    }else {
        offset = 55;
        drawBoard(score, level, 0);
        drawBoard(score, level, offset);
    }
    SOCKET csock;
    SOCKET lsock;
    Config con;
    if(is2Player) {
        if(isClient) {
            con.host = "10.0.0.2";
            con.port = "8085";
        } else {
            tcp_server_create(&lsock);
        }
    }
  
    int idle_cnt = 0;
    
    
    while(1) {
        if(pause_flg) {
            mvprintw(13,18, "PAUSED");
            while(wgetch(stdscr) != 'p'){}
            mvprintw(13,18, "      ");
            pause_flg = false;
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
                //idle_cnt++;
                update(movement, &t, matrix, &gameOver);
            }
            t.toggle(&t, matrix);
            toggle_flg = true;
            break;
        case KEY_RIGHT:
            if(cnt++ == 2) {
                cnt = 0;
                //idle_cnt++;
                update(movement, &t, matrix, &gameOver);
            }
            // next_x = current_x + 3;
            movement = RIGHT;
            break;
        case KEY_LEFT:
            if(cnt++ == 2) {
                cnt = 0;
                //idle_cnt++;
                update(movement, &t, matrix, &gameOver);
            }
            movement = LEFT;
            // next_x = current_x - 3;
            break;
        case 's':
            save(matrix, t.color, delay, speedcnt, level, score, next_tetrimo, heldExists, heldLast, held_tetrimo);
            break;
        case 'z':
            save(matrix, t.color, delay, speedcnt, level, score, next_tetrimo, heldExists, heldLast, held_tetrimo);
            endwin();
            exit(EXIT_SUCCESS);
            // f
            break;
        case ' ':
            if(heldLast) {
                break;
            }
            heldLast = TRUE;
            elim(t);
            if(heldExists) {
                eraseNext(next_tetrimo, 0);
                next_tetrimo = held_tetrimo;
            }
            eraseHeld(held_tetrimo, 0);
            held_tetrimo = t.color;
            drawHeld(held_tetrimo, 0); 
            heldExists = true;
            t = newBlock(RANDOM, &next_tetrimo, 0);
            break;
        case ESC_KEY:
            delay = 1000;
            speedcnt = 0;
            pause_flg = false;
            toggle_flg = false;
            block_flg = false;
            score = 0;
            level = 1;
            heldExists = FALSE;
            heldLast = FALSE;
            clear();
            fclose(err);
            if(is2Player) {
                if(isClient) {
                    tcp_client_close(csock);
                } else {
                    tcp_server_close(csock, lsock);
                }
            }
            return;
            break;
        default:
            cnt = 0;
            break;
        }
        idle_cnt++;
        if(!toggle_flg){
            if(!update(movement, &t, matrix, &gameOver)) {
                updateMatrix(t, matrix);
                t = newBlock(RANDOM, &next_tetrimo, 0);
                heldLast = false;
            }
        }
        if(gameOver) {
            // FILE *err;
            // err = fopen("err.log", "w+");
            // fputs("gamover", err);
            drawGameOver();
            gameOver = FALSE;
            while(wgetch(stdscr) != ESC_KEY) {}
            clear();
            return;
        }
        toggle_flg = false;
        // mvprintw(0, 0, "%d, %d", current_x, current_y);
        // mvprintw(2, 0, "%d, %d", next_x, next_y);
        // mvprintw(4, 0, "%d", current_orientation);
        // mvprintw(6, 0, "%d", next_orientation);
        if(checkLine(matrix)) {
            updateScoreLevel(&score, &level, &speedcnt, &delay);
        }
        if(is2Player) {
            // char send[25*11];
            // char receive[25*11];
            char send[34];
            char receive[34];
            // for(int i = 0; i < 25; i++) {
            //     for(int j = 0; j < 9; j++) {
            //         if(matrix[i][j] == TRUE){
            //             send[(i*9)+j] = '1';
            //         } else {
            //             send[(i*9)+j] = '0';
            //         }
            //     }
            // }
            sprintf(send, "%02d%02d%02d%02d%02d%02d%02d%02d\0", 
                    t.current_xy[0].y, t.current_xy[0].x,
                    t.current_xy[1].y, t.current_xy[1].x,
                    t.current_xy[2].y, t.current_xy[2].x,
                    t.current_xy[3].y, t.current_xy[3].x);

            char c[5];
            sprintf(c, "%d", idle_cnt);
            if(isClient) {  
                FILE *s;
                s = fopen("client_err.txt", "w+");
                if(tcp_client_connect(con, &csock)) {
                    fputs("connect error", s);
                }
                if(tcp_client_send_request(&csock, send)) {
                    fputs("send request error", s);
                }
                // if(tcp_client_receive_response(&sock, receive)) {
                //     fputs("receive response rror", s);
                // }
                tcp_client_close(csock);
                fputs(c, s);
                fclose(s);
            } else {
                FILE *q;
                q = fopen("server_err.txt", "w+");

                if(tcp_server_accept_connection(&lsock, &csock)) {
                    fputs("accept connection errror", q); 
                }
                if(tcp_server_receive_request(&csock, receive)) {
                    fputs("receive request errror", q);
                }
                drawSecondPlayer(receive);
                // if(tcp_server_send_response(&sock, send)) {
                //     fputs("send response eror", q);
                // }
                //tcp_server_close(sock);
                fputs(c, q);
                fclose(q);
        }
    }
        refresh();
    }
    fclose(err);
}

void drawSecondPlayer(char *second) {
    // for(int i = 0; i < 25; i++) {
    //     for(int j = 0; j < 9; j++) {
    //         if(second[(i*9)+j] == '1'){
    //             paint(i, blocktomatrix2(j));
    //         } else if(second[(i*9)+j] == '0') {
    //             whiteout(i, blocktomatrix2(j));
    //         }
    //     }
    // }
    int y;
    int x;
    char d[5];
    FILE *f;
    f = fopen("adfsd.txt", "w+");
    static tetrimo last;
    static bool hasLast;
    if(hasLast) {
        for(int i = 0; i < 4; i++) {
            whiteout(last.current_xy[i].y, last.current_xy[i].x+55);
        }
    }
    for(int i = 0; i < 16; i+=4) {
        if(second[i]=='0') {
            sprintf(d, "%c", second[i+1]);
        } else {
            sprintf(d, "%c%c", second[i], second[i+1]);
        }
       
        y = atoi(d);
        char q[5];
        sprintf(q, "|%d|", y);
        fputs(q, f);
        if(second[i+2]=='0') {
            sprintf(d, "%c", second[i+3]);
        } else {
            sprintf(d, "%c%c", second[i+2], second[i+3]);
        }
        x = atoi(d);
        sprintf(q, "|%d|", x);
        fputs(q, f);
        fputs(d, f);
        paint(y, x+55);
        last.current_xy[i/4].y = y;
        last.current_xy[i/4].x = x;
        if((last.current_xy[1/4].y - y) > 3) {
            hasLast = false;
        } else {
            hasLast = true;
        }
    }
    fclose(f);
}

void drawTitle(bool isSave) {
    mvprintw(5,5, "////////// ////// ////////// /////////  //////// ////////");
    mvprintw(6,5, "   //     //         //     //     //     //    //");
    mvprintw(7,5, "  //     //////     //     /////////     //     //////");
    mvprintw(8,5, " //     //         //     //    \\\\      //          //");
    mvprintw(9,5, "//     //////     //     //      \\\\  ///////  ///////");
    mvprintw(13, ARROW_X, "->");
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
    mvprintw(10,5, "P................Pause game");
    mvprintw(11,5, "Z................Save and quit game");
    mvprintw(12,5, "ESC..............Go back to title screen");
}

void drawOptions(int level) {
    mvprintw(13, ARROW_X, "->");
    mvprintw(13, 26, "Level: %d", level);
    mvprintw(15, 22, "(ESC to go back)");
}

int hostOrClient() {
    mvprintw(13, ARROW_X, "->");
    mvprintw(13, 26, "HOST");
    mvprintw(15, 26, "JOIN");

    int option = 0;
    bool select_flg = false;
    int arrow_pos = 13;
    while(!select_flg) {
        switch(wgetch(stdscr)) {
            case KEY_UP:
                if(option != 0) {
                    option--;
                    mvprintw(arrow_pos, ARROW_X, "  ");
                    arrow_pos -= 2;
                    mvprintw(arrow_pos, ARROW_X, "->");
                }
                break;
            case KEY_DOWN:
                if(option != 1) {
                    option++;
                    mvprintw(arrow_pos, ARROW_X, "  ");
                    arrow_pos += 2;
                    mvprintw(arrow_pos, ARROW_X, "->");
                }
                break;
            case '\n':
                select_flg = TRUE;
            default:
                break;
        }
    }
    return option;
}

tetrimo newBlock(Color c, Color *next_tetrimo, int offset) {
    tetrimo new_tetrimo;
    new_tetrimo.color = *next_tetrimo;
    new_tetrimo.current_orientation = LEFT;

    // FILE *err;
    // err = fopen("err.txt", "w+");
    // char c[4] = "New"
    // fputs(c, err);
    switch(new_tetrimo.color) {
        case(RED):
            new_tetrimo.current_xy[0].x = 19+offset+offset;
            new_tetrimo.current_xy[0].y = 1;
            new_tetrimo.current_xy[1].x = 19+offset;
            new_tetrimo.current_xy[1].y = 0;
            new_tetrimo.current_xy[2].x = 16+offset;
            new_tetrimo.current_xy[2].y = 1;
            new_tetrimo.current_xy[3].x = 22+offset;
            new_tetrimo.current_xy[3].y = 0;
            new_tetrimo.toggle = &toggleRed;
            break;
        case(GREEN):
            new_tetrimo.current_xy[0].x = 19+offset;
            new_tetrimo.current_xy[0].y = 1;
            new_tetrimo.current_xy[1].x = 19+offset;
            new_tetrimo.current_xy[1].y = 0;
            new_tetrimo.current_xy[2].x = 16+offset;
            new_tetrimo.current_xy[2].y = 0;
            new_tetrimo.current_xy[3].x = 22+offset;
            new_tetrimo.current_xy[3].y = 1;
            new_tetrimo.toggle = &toggleGreen;
            break;
        case(CYAN):
            new_tetrimo.current_xy[0].x = 22+offset;
            new_tetrimo.current_xy[0].y = 3;
            new_tetrimo.current_xy[1].x = 22+offset;
            new_tetrimo.current_xy[1].y = 2;
            new_tetrimo.current_xy[2].x = 22+offset;
            new_tetrimo.current_xy[2].y = 1;
            new_tetrimo.current_xy[3].x = 22+offset;
            new_tetrimo.current_xy[3].y = 0;
            new_tetrimo.toggle = &toggleCyan;
            break;
        case(BLUE):
            new_tetrimo.current_xy[0].x = 19+offset;
            new_tetrimo.current_xy[0].y = 2;
            new_tetrimo.current_xy[1].x = 19+offset;
            new_tetrimo.current_xy[1].y = 0;
            new_tetrimo.current_xy[2].x = 19+offset;
            new_tetrimo.current_xy[2].y = 1;
            new_tetrimo.current_xy[3].x = 22+offset;
            new_tetrimo.current_xy[3].y = 2;
            new_tetrimo.toggle = &toggleBlue;
            break;
        case(YELLOW):
            new_tetrimo.current_xy[0].x = 19+offset;
            new_tetrimo.current_xy[0].y = 1;
            new_tetrimo.current_xy[1].x = 22+offset;
            new_tetrimo.current_xy[1].y = 0;
            new_tetrimo.current_xy[2].x = 19+offset;
            new_tetrimo.current_xy[2].y = 0;
            new_tetrimo.current_xy[3].x = 22+offset;
            new_tetrimo.current_xy[3].y = 1;
            new_tetrimo.toggle = &toggleYellow;
            break;
        case(PURPLE):
            new_tetrimo.current_xy[0].x = 19+offset;
            new_tetrimo.current_xy[0].y = 2;
            new_tetrimo.current_xy[1].x = 19+offset;
            new_tetrimo.current_xy[1].y = 0;
            new_tetrimo.current_xy[2].x = 19+offset;
            new_tetrimo.current_xy[2].y = 1;
            new_tetrimo.current_xy[3].x = 22+offset;
            new_tetrimo.current_xy[3].y = 1;
            new_tetrimo.toggle = &togglePurple;
            break;
        case(ORANGE):
            new_tetrimo.current_xy[0].x = 19+offset;
            new_tetrimo.current_xy[0].y = 2;
            new_tetrimo.current_xy[1].x = 19+offset;
            new_tetrimo.current_xy[1].y = 1;
            new_tetrimo.current_xy[2].x = 16+offset;
            new_tetrimo.current_xy[2].y = 2;
            new_tetrimo.current_xy[3].x = 19+offset;
            new_tetrimo.current_xy[3].y = 0;
            new_tetrimo.toggle = &toggleOrange;
            break;
        default:
            break;
    }
    //currentTetrimo = new_tetrimo;
    if(c==RANDOM) {
        *next_tetrimo = rand() % 7;
    } else {
        *next_tetrimo = c;
    }
    for(int i = 0; i < 4; i++) {
        new_tetrimo.next_xy[i].x = new_tetrimo.current_xy[i].x;
        new_tetrimo.next_xy[i].y = new_tetrimo.current_xy[i].y;
    }
    draw(new_tetrimo);
    eraseNext(new_tetrimo.color, offset);
    drawNext(*next_tetrimo, offset);
    return new_tetrimo;
}

void eraseNext(Color c, int offset) {
    for(int i = 2; i < 6; i++) {
        mvprintw(i, 39+offset, "             ");
    }
}

void drawNext(Color c, int offset) {
    switch(c) {
        case(RED):
            drawRed(NEXT, offset);
            break;
        case(GREEN):
            drawGreen(NEXT, offset);
            break;
        case(CYAN):
            drawCyan(NEXT, offset);
            break;
        case(BLUE):
            drawBlue(NEXT, offset);
            break;
        case(YELLOW):
            drawYellow(NEXT, offset);
            break;
        case(PURPLE):
            drawPurple(NEXT, offset);
            break;
        case(ORANGE):
            drawOrange(NEXT, offset);
            break;
        default:
            break;
    }
}

void eraseHeld(Color c, int offset) {
    switch(c) {
        case(RED):
            eraseRed(HOLD, offset);
            break;
        case(GREEN):
            eraseGreen(HOLD, offset);
            break;
        case(CYAN):
            eraseCyan(HOLD, offset);
            break;
        case(BLUE):
            eraseBlue(HOLD, offset);
            break;
        case(YELLOW):
            eraseYellow(HOLD, offset);
            break;
        case(PURPLE):
            erasePurple(HOLD, offset);
            break;
        case(ORANGE):
            eraseOrange(HOLD, offset);
            break;
        default:
            break;
    }
}

void drawHeld(Color c, int offset) {
    switch(c) {
        case(RED):
            drawRed(HOLD, offset);
            break;
        case(GREEN):
            drawGreen(HOLD, offset);
            break;
        case(CYAN):
            drawCyan(HOLD, offset);
            break;
        case(BLUE):
            drawBlue(HOLD, offset);
            break;
        case(YELLOW):
            drawYellow(HOLD, offset);
            break;
        case(PURPLE):
            drawPurple(HOLD, offset);
            break;
        case(ORANGE):
            drawOrange(HOLD, offset);
            break;
        default:
            break;
    }
}

void drawBoard(int score, int level, int offset) {
    //clear();
    int i;
    for(i = 0; i < MATRIX_LENGTH - 1; i++) {
        mvprintw(i, 5+offset, MATRIX);
    }
    mvprintw(i, 5+offset, MATRIX_BOTTOM);
    mvprintw(26,16+offset,"Score: %d", score);
    mvprintw(27,16+offset,"Level: %d", level);
    mvprintw(0,38+offset, "Next:");
    mvprintw(1,39+offset, "_____________");
    for(int i = 2; i < 7; i++) {
        mvprintw(i,38+offset,"|");
        mvprintw(i,52+offset,"|");
    }
    mvprintw(6,39+offset, "_____________");
    mvprintw(11,39+offset, "_____________");
    for(int i = 12; i < 17; i++) {
        mvprintw(i,38+offset,"|");
        mvprintw(i,52+offset,"|");
    }
    mvprintw(16,39+offset, "_____________");
    mvprintw(10,38+offset, "Hold:");
}

void initMatrix(bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    for(int i = 0; i < 25; i++) {
        for(int j = 0; j < 9; j++) {
            matrix[i][j] = FALSE;
        }
    }
}

void save(bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH], Color current_color, int delay, int speedcnt, int level, int score, Color next_tetrimo, bool heldExists, bool heldLast, Color held_tetrimo) {
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
    fputc(current_color+48, savefp);
    fputc(' ', savefp);
    char stats[20];
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

void load(bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH], Color *current_color, int *delay, int *speedcnt, int *level, int *score, Color *next_tetrimo, bool *heldExists, bool *heldLast, Color *held_tetrimo) {
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
    // eraseNext(*next_tetrimo);
    (*next_tetrimo) = atoi(c);
    char d[5];
    fscanf(loadfp, "%s", d);
    (*delay) = atoi(d);
    char s[5];
    fscanf(loadfp, "%s", s);
    (*speedcnt) = atoi(s);
    char l[2];
    fscanf(loadfp, "%s", l);
    (*level) = atoi(l);
    // mvprintw(27,21, "     ");
    // mvprintw(27,16,"Level: %d", (*level));
    char sc[2];
    fscanf(loadfp, "%s", sc);
    (*score) = atoi(sc);
    // mvprintw(26,21, "     ");
    // mvprintw(26,16,"Score: %d", (*score));
    char n[2];
    fscanf(loadfp, "%s", n);
    (*current_color) = atoi(n);
    char he[2];
    fscanf(loadfp, "%s", he);
    (*heldExists) = atoi(he);
    if(*heldExists) {
        char hl[2];
        fscanf(loadfp, "%s", hl);
        (*heldLast) = atoi(hl);
        char h[2];
        fscanf(loadfp, "%s", h);
        eraseHeld(*held_tetrimo, 0);
        (*held_tetrimo) = atoi(h);
        drawHeld(*held_tetrimo, 0);
    }
    fclose(loadfp);
}

bool checkRight(tetrimo t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    if(t.current_xy[3].x >= 29) {
        return true;
    }
    for(int i = 0; i < 25; i++) {
        for(int j = 0; j < 9; j++) {
            if(matrix[i][j] == TRUE) {
                for(int k = 0; k < 4; k++) {
                    if(t.next_xy[k].y == i 
                    && t.next_xy[k].x == blocktomatrix(j)){
                        //block_flg = 1;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool checkLeft(tetrimo t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    if(t.current_xy[2].x <= 8) {
        return true;
    }
    for(int i = 0; i < 25; i++) {
        for(int j = 0; j < 9; j++) {
            if(matrix[i][j] == TRUE) {
                for(int k = 0; k < 4; k++) {
                    if(t.next_xy[k].y == i 
                    && t.next_xy[k].x == blocktomatrix(j)){
                        //block_flg = 1;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool checkDown(tetrimo t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    if(t.current_xy[0].y >= MATRIX_LENGTH-2) {
        //block_flg = 1;
        return true;
    }
    for(int i = 0; i < 25; i++) {
        for(int j = 0; j < 9; j++) {
            if(matrix[i][j] == TRUE) {
                for(int k = 0; k < 4; k++) {
                    if(t.next_xy[k].y == i 
                    && t.next_xy[k].x == blocktomatrix(j)){
                        //block_flg = 1;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}


void toggleRed(tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    if((*t).current_orientation == LEFT) {
        (*t).next_xy[0].x = (*t).current_xy[0].x+3;
        (*t).next_xy[0].y = (*t).current_xy[0].y;
        (*t).next_xy[1].x = (*t).current_xy[1].x;
        (*t).next_xy[1].y = (*t).current_xy[1].y;
        (*t).next_xy[2].x = (*t).current_xy[2].x+3;
        (*t).next_xy[2].y = (*t).current_xy[2].y-2;
        (*t).next_xy[3].x = (*t).current_xy[3].x;
        (*t).next_xy[3].y = (*t).current_xy[3].y;
        (*t).current_orientation = RIGHT;  
    } else {
        if(checkLeft(*t, matrix)) {
            return;
        }
        (*t).next_xy[0].x = (*t).current_xy[0].x-3;
        (*t).next_xy[0].y = (*t).current_xy[0].y;
        (*t).next_xy[1].x = (*t).current_xy[1].x;
        (*t).next_xy[1].y = (*t).current_xy[1].y;
        (*t).next_xy[2].x = (*t).current_xy[2].x-3;
        (*t).next_xy[2].y = (*t).current_xy[2].y+2;
        (*t).next_xy[3].x = (*t).current_xy[3].x;
        (*t).next_xy[3].y = (*t).current_xy[3].y;
        (*t).current_orientation = LEFT;
    }
    elim(*t);
    draw(*t);
    for(int i = 0; i < 4; i++) {
        (*t).current_xy[i].x = (*t).next_xy[i].x;
        (*t).current_xy[i].y = (*t).next_xy[i].y;
    }
}

void toggleGreen(struct tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    if((*t).current_orientation == LEFT) {
        (*t).next_xy[0].x = (*t).current_xy[0].x;
        (*t).next_xy[0].y = (*t).current_xy[0].y;
        (*t).next_xy[1].x = (*t).current_xy[1].x+3;
        (*t).next_xy[1].y = (*t).current_xy[1].y-1;
        (*t).next_xy[2].x = (*t).current_xy[2].x+3;
        (*t).next_xy[2].y = (*t).current_xy[2].y;
        (*t).next_xy[3].x = (*t).current_xy[3].x;
        (*t).next_xy[3].y = (*t).current_xy[3].y-1;
        (*t).current_orientation = RIGHT;  
    } else {
        if(checkLeft(*t, matrix)) {
            return;
        }
        (*t).next_xy[0].x = (*t).current_xy[0].x;
        (*t).next_xy[0].y = (*t).current_xy[0].y;
        (*t).next_xy[1].x = (*t).current_xy[1].x-3;
        (*t).next_xy[1].y = (*t).current_xy[1].y+1;
        (*t).next_xy[2].x = (*t).current_xy[2].x-3;
        (*t).next_xy[2].y = (*t).current_xy[2].y;
        (*t).next_xy[3].x = (*t).current_xy[3].x;
        (*t).next_xy[3].y = (*t).current_xy[3].y+1;
        (*t).current_orientation = LEFT;
    }
    elim(*t);
    draw(*t);
    for(int i = 0; i < 4; i++) {
        (*t).current_xy[i].x = (*t).next_xy[i].x;
        (*t).current_xy[i].y = (*t).next_xy[i].y;
    }
}

void toggleCyan(struct tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    if((*t).current_orientation == LEFT) {
        if((*t).current_xy[2].x <= 11 || 
           checkRight(*t, matrix)) {
            return;
        }
        (*t).next_xy[0].x = (*t).current_xy[0].x;
        (*t).next_xy[0].y = (*t).current_xy[0].y-1;
        (*t).next_xy[1].x = (*t).current_xy[1].x-3;
        (*t).next_xy[1].y = (*t).current_xy[1].y;
        (*t).next_xy[2].x = (*t).current_xy[2].x-6;
        (*t).next_xy[2].y = (*t).current_xy[2].y+1;
        (*t).next_xy[3].x = (*t).current_xy[3].x+3;
        (*t).next_xy[3].y = (*t).current_xy[3].y+2;
        (*t).current_orientation = RIGHT;  
    } else {
        (*t).next_xy[0].x = (*t).current_xy[0].x;
        (*t).next_xy[0].y = (*t).current_xy[0].y+1;
        (*t).next_xy[1].x = (*t).current_xy[1].x+3;
        (*t).next_xy[1].y = (*t).current_xy[1].y;
        (*t).next_xy[2].x = (*t).current_xy[2].x+6;
        (*t).next_xy[2].y = (*t).current_xy[2].y-1;
        (*t).next_xy[3].x = (*t).current_xy[3].x-3;
        (*t).next_xy[3].y = (*t).current_xy[3].y-2;
        (*t).current_orientation = LEFT;
    }
    elim(*t);
    draw(*t);
    for(int i = 0; i < 4; i++) {
        (*t).current_xy[i].x = (*t).next_xy[i].x;
        (*t).current_xy[i].y = (*t).next_xy[i].y;
    }
}

void toggleBlue(struct tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    if((*t).current_orientation == LEFT) {
        if(checkLeft(*t, matrix)) {
            return;
        }
        (*t).next_xy[0].x = (*t).current_xy[0].x-3;
        (*t).next_xy[0].y = (*t).current_xy[0].y;
        (*t).next_xy[1].x = (*t).current_xy[1].x;
        (*t).next_xy[1].y = (*t).current_xy[1].y+1;
        (*t).next_xy[2].x = (*t).current_xy[2].x-3;
        (*t).next_xy[2].y = (*t).current_xy[2].y;
        (*t).next_xy[3].x = (*t).current_xy[3].x;
        (*t).next_xy[3].y = (*t).current_xy[3].y-1;
        (*t).current_orientation = UP;  
    } else if((*t).current_orientation == UP) {
        (*t).next_xy[0].x = (*t).current_xy[0].x+3;
        (*t).next_xy[0].y = (*t).current_xy[0].y;
        (*t).next_xy[1].x = (*t).current_xy[1].x;
        (*t).next_xy[1].y = (*t).current_xy[1].y;
        (*t).next_xy[2].x = (*t).current_xy[2].x;
        (*t).next_xy[2].y = (*t).current_xy[2].y-1;
        (*t).next_xy[3].x = (*t).current_xy[3].x-3;
        (*t).next_xy[3].y = (*t).current_xy[3].y-1;
        (*t).current_orientation = RIGHT;
    } else if((*t).current_orientation == RIGHT) {
        if(checkRight(*t, matrix)) {
            return;
        }
        (*t).next_xy[0].x = (*t).current_xy[0].x+3;
        (*t).next_xy[0].y = (*t).current_xy[0].y-1;
        (*t).next_xy[1].x = (*t).current_xy[1].x;
        (*t).next_xy[1].y = (*t).current_xy[1].y;
        (*t).next_xy[2].x = (*t).current_xy[2].x;
        (*t).next_xy[2].y = (*t).current_xy[2].y+1;
        (*t).next_xy[3].x = (*t).current_xy[3].x+3;
        (*t).next_xy[3].y = (*t).current_xy[3].y;
        (*t).current_orientation = DOWN;
    } else if((*t).current_orientation == DOWN) {
        (*t).next_xy[0].x = (*t).current_xy[0].x-3;
        (*t).next_xy[0].y = (*t).current_xy[0].y+1;
        (*t).next_xy[1].x = (*t).current_xy[1].x;
        (*t).next_xy[1].y = (*t).current_xy[1].y-1;
        (*t).next_xy[2].x = (*t).current_xy[2].x+3;
        (*t).next_xy[2].y = (*t).current_xy[2].y;
        (*t).next_xy[3].x = (*t).current_xy[3].x;
        (*t).next_xy[3].y = (*t).current_xy[3].y+2;
        (*t).current_orientation = LEFT;
    }
    elim(*t);
    draw(*t);
    for(int i = 0; i < 4; i++) {
        (*t).current_xy[i].x = (*t).next_xy[i].x;
        (*t).current_xy[i].y = (*t).next_xy[i].y;
    }
}

void toggleYellow(struct tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    elim(*t);
    draw(*t);
    for(int i = 0; i < 4; i++) {
        (*t).current_xy[i].x = (*t).next_xy[i].x;
        (*t).current_xy[i].y = (*t).next_xy[i].y;
    }
}

void togglePurple(struct tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    if((*t).current_orientation == LEFT) {
        if(checkLeft(*t, matrix)) {
            return;
        }
        (*t).next_xy[0].x = (*t).current_xy[0].x;
        (*t).next_xy[0].y = (*t).current_xy[0].y;
        (*t).next_xy[1].x = (*t).current_xy[1].x;
        (*t).next_xy[1].y = (*t).current_xy[1].y+1;
        (*t).next_xy[2].x = (*t).current_xy[2].x-3;
        (*t).next_xy[2].y = (*t).current_xy[2].y;
        (*t).next_xy[3].x = (*t).current_xy[3].x;
        (*t).next_xy[3].y = (*t).current_xy[3].y;
        (*t).current_orientation = UP;  
    } else if((*t).current_orientation == UP) {
        (*t).next_xy[0].x = (*t).current_xy[0].x;
        (*t).next_xy[0].y = (*t).current_xy[0].y;
        (*t).next_xy[1].x = (*t).current_xy[1].x;
        (*t).next_xy[1].y = (*t).current_xy[1].y;
        (*t).next_xy[2].x = (*t).current_xy[2].x;
        (*t).next_xy[2].y = (*t).current_xy[2].y;
        (*t).next_xy[3].x = (*t).current_xy[3].x-3;
        (*t).next_xy[3].y = (*t).current_xy[3].y-1;
        (*t).current_orientation = RIGHT;
    } else if((*t).current_orientation == RIGHT) {
        if(checkRight(*t, matrix)) {
            return;
        }
        (*t).next_xy[0].x = (*t).current_xy[0].x;
        (*t).next_xy[0].y = (*t).current_xy[0].y-1;
        (*t).next_xy[1].x = (*t).current_xy[1].x;
        (*t).next_xy[1].y = (*t).current_xy[1].y-1;
        (*t).next_xy[2].x = (*t).current_xy[2].x;
        (*t).next_xy[2].y = (*t).current_xy[2].y;
        (*t).next_xy[3].x = (*t).current_xy[3].x+3;
        (*t).next_xy[3].y = (*t).current_xy[3].y+1;
        (*t).current_orientation = DOWN;
    } else if((*t).current_orientation == DOWN) {
        (*t).next_xy[0].x = (*t).current_xy[0].x;
        (*t).next_xy[0].y = (*t).current_xy[0].y+1;
        (*t).next_xy[1].x = (*t).current_xy[1].x;
        (*t).next_xy[1].y = (*t).current_xy[1].y;
        (*t).next_xy[2].x = (*t).current_xy[2].x+3;
        (*t).next_xy[2].y = (*t).current_xy[2].y;
        (*t).next_xy[3].x = (*t).current_xy[3].x;
        (*t).next_xy[3].y = (*t).current_xy[3].y;
        (*t).current_orientation = LEFT;
    }
    elim(*t);
    draw(*t);
    for(int i = 0; i < 4; i++) {
        (*t).current_xy[i].x = (*t).next_xy[i].x;
        (*t).current_xy[i].y = (*t).next_xy[i].y;
    }
}

void toggleOrange(struct tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    if((*t).current_orientation == LEFT) {
        if(checkRight(*t, matrix)) {
            return;
        }
        (*t).next_xy[0].x = (*t).current_xy[0].x-3;
        (*t).next_xy[0].y = (*t).current_xy[0].y-1;
        (*t).next_xy[1].x = (*t).current_xy[1].x;
        (*t).next_xy[1].y = (*t).current_xy[1].y;
        (*t).next_xy[2].x = (*t).current_xy[2].x;
        (*t).next_xy[2].y = (*t).current_xy[2].y-2;
        (*t).next_xy[3].x = (*t).current_xy[3].x+3;
        (*t).next_xy[3].y = (*t).current_xy[3].y+1;
        (*t).current_orientation = UP;  
    } else if((*t).current_orientation == UP) {
        (*t).next_xy[0].x = (*t).current_xy[0].x+3;
        (*t).next_xy[0].y = (*t).current_xy[0].y+1;
        (*t).next_xy[1].x = (*t).current_xy[1].x;
        (*t).next_xy[1].y = (*t).current_xy[1].y;
        (*t).next_xy[2].x = (*t).current_xy[2].x+3;
        (*t).next_xy[2].y = (*t).current_xy[2].y;
        (*t).next_xy[3].x = (*t).current_xy[3].x;
        (*t).next_xy[3].y = (*t).current_xy[3].y-1;
        (*t).current_orientation = RIGHT;
    } else if((*t).current_orientation == RIGHT) {
        if(checkLeft(*t, matrix)) {
            return;
        }
        (*t).next_xy[0].x = (*t).current_xy[0].x+3;
        (*t).next_xy[0].y = (*t).current_xy[0].y;
        (*t).next_xy[1].x = (*t).current_xy[1].x;
        (*t).next_xy[1].y = (*t).current_xy[1].y;
        (*t).next_xy[2].x = (*t).current_xy[2].x-3;
        (*t).next_xy[2].y = (*t).current_xy[2].y+1;
        (*t).next_xy[3].x = (*t).current_xy[3].x;
        (*t).next_xy[3].y = (*t).current_xy[3].y+1;
        (*t).current_orientation = DOWN;
    } else if((*t).current_orientation == DOWN) {
        (*t).next_xy[0].x = (*t).current_xy[0].x-3;
        (*t).next_xy[0].y = (*t).current_xy[0].y;
        (*t).next_xy[1].x = (*t).current_xy[1].x;
        (*t).next_xy[1].y = (*t).current_xy[1].y;
        (*t).next_xy[2].x = (*t).current_xy[2].x;
        (*t).next_xy[2].y = (*t).current_xy[2].y+1;
        (*t).next_xy[3].x = (*t).current_xy[3].x-3;
        (*t).next_xy[3].y = (*t).current_xy[3].y-1;
        (*t).current_orientation = LEFT;
    }
    elim(*t);
    draw(*t);
    for(int i = 0; i < 4; i++) {
        (*t).current_xy[i].x = (*t).next_xy[i].x;
        (*t).current_xy[i].y = (*t).next_xy[i].y;
    }
}

void hitMatrix(int x, int y, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    matrix[y][matrixtoblock(x)] = true;
}

void updateMatrix(tetrimo t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    for(int i = 0; i < 4; i++) {
        // mvprintw(4+i,0,"%d, %d", currentTetrimo.current_xy[i].x, currentTetrimo.current_xy[i].y);
        hitMatrix(t.current_xy[i].x, t.current_xy[i].y, matrix);
    }
}

void eraseLine(int y, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    for(int i = 0; i < 9; i++) {
        whiteout(y, (i*3)+7);
        matrix[y][i] = FALSE;
    }
}

void replaceLines(int first, int last, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    int final = 26;
    for(int i = 0; i < 25; i++) {
        for(int j = 0; j < 9; j++) {
            if((matrix[i][j]) == TRUE) {
                final = i;
                break;
            }
        }
        if(final != 26){
            break;
        }
    }
    for(int i = first, k = 0; i >= final; i--, k++) {
        eraseLine(i, matrix);
        for(int j = 0; j < 9; j++) {
            if((matrix[last-1-k][j]) == TRUE) {
                (matrix[i][j]) = TRUE;
                paint(i, blocktomatrix(j));
            }
        }
    }
}

void updateScoreLevel(int *score, int *level, int *speedcnt, int *delay) {
    (*score) += 100;
    (*speedcnt) += 100;
    if((*speedcnt) >= 1000) {
        if((*delay) <= 100) {
            (*delay) -= 10;
        } else {
            (*delay) -= 100;
        }
        timeout(*delay);
        (*speedcnt) = 0;
        (*level)++;
    }
    mvprintw(26,21, "     ");
    mvprintw(26,16,"Score: %d", *score);
    mvprintw(27,21, "     ");
    mvprintw(27,16,"Level: %d", *level);
}

bool checkLine(bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    int first = 0;
    int last = 0;
    bool cleared = false;
    
    //problem here
    // for(int i = 24; i >= 0; i--) {
    //     for(int j = 0; j < 9; j++) {
    //         if(matrix[i][j] == FALSE) {
    //             log("x");
    //         }
    //     } 
    // }
    for(int i = 24; i >= 0; i--) {
        for(int j = 0; j < 9; j++) {
            if(matrix[i][j] == FALSE) {
                break;
            }
            if(j == 8) {
                cleared = true;
                last = i;
                if(first == 0) {
                    first = i;
                }
            }
        } 
    }

    if(first != 0 && last != 0) {
        replaceLines(first, last, matrix);
    }
    return cleared;
}

// returns true if checkDown is true
bool shift(Orientation o, tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    switch (o)
    {
    case RIGHT:
        for(int i = 0; i < 4; i++) {
            (*t).next_xy[i].x = (*t).current_xy[i].x+3;
            (*t).next_xy[i].y = (*t).current_xy[i].y;
        }
        if(checkRight(*t, matrix)) {
            for(int i = 0; i < 4; i++) {
                (*t).next_xy[i].x = (*t).current_xy[i].x;
                (*t).next_xy[i].y = (*t).current_xy[i].y;
            }
        }
        break;
    case LEFT:
        for(int i = 0; i < 4; i++) {
                (*t).next_xy[i].x = (*t).current_xy[i].x-3;
                (*t).next_xy[i].y = (*t).current_xy[i].y;
        }
        if(checkLeft(*t, matrix)) {
            for(int i = 0; i < 4; i++) {
                (*t).next_xy[i].x = (*t).current_xy[i].x;
                (*t).next_xy[i].y = (*t).current_xy[i].y;
            }
        }
        break;
    case DOWN:
        for(int i = 0; i < 4; i++) {
                (*t).next_xy[i].x = (*t).current_xy[i].x;
                (*t).next_xy[i].y = (*t).current_xy[i].y+1;
        }
        if(checkDown(*t, matrix)) {
            //mvprintw(*t).next_xy[0].y, (*t).next_xy[0].x, "x");
            //block_flg = true;
            //heldLast = FALSE;
            // for(int i = 0; i < 4; i++) {
            //     (*t).next_xy[i].x = (*t).current_xy[i].x;
            //     (*t).next_xy[i].y = (*t).current_xy[i].y;
            //     if((*t).current_xy[i].y==0) {
            //         gameOver = TRUE;
            //     }
            // }
            return true;
            //updateMatrix(tetrimo t);
        }
        break;
    default:
        break;
    }
    return false;
}

void drawGameOver() {
    clear();
    mvprintw(15,15,"GAME OVER");
}

// returns false if checkDown
bool update(Orientation o, tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH], bool *gameOver) {
    // if checkDown
    if(shift(o, t, matrix)) {
        for(int i = 0; i < 4; i++) {
            (*t).next_xy[i].x = (*t).current_xy[i].x;
            (*t).next_xy[i].y = (*t).current_xy[i].y;
            if((*t).current_xy[i].y==0) {
                (*gameOver) = TRUE;
            }
        }
        return false;
    }
    else {
        elim(*t);
        draw(*t);
        for(int i = 0; i < 4; i++) {
            (*t).current_xy[i].x = (*t).next_xy[i].x;
            (*t).current_xy[i].y = (*t).next_xy[i].y;
        }
    }
    return true;
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

void eraseRed(Display d, int offset) {
    if(d == NEXT) {
        whiteout(4, 41+offset);
        whiteout(4, 44+offset);
        whiteout(3, 44+offset);
        whiteout(3, 47+offset);
    } else {
        whiteout(14, 41+offset);
        whiteout(14, 44+offset);
        whiteout(13, 44+offset);
        whiteout(13, 47+offset);
    }
}

void drawRed(Display d, int offset) {
    if(d == NEXT) {
        paint(4, 41+offset);
        paint(4, 44+offset);
        paint(3, 44+offset);
        paint(3, 47+offset);
    } else {
        paint(14, 41+offset);
        paint(14, 44+offset);
        paint(13, 44+offset);
        paint(13, 47+offset);
    }
}

void eraseGreen(Display d, int offset) {
    if(d == NEXT) {
        whiteout(3, 41+offset);
        whiteout(3, 44+offset);
        whiteout(4, 44+offset);
        whiteout(4, 47+offset);
    } else {
        whiteout(13, 41+offset);
        whiteout(13, 44+offset);
        whiteout(14, 44+offset);
        whiteout(14, 47+offset);
    }
}

void drawGreen(Display d, int offset) {
    if(d == NEXT) {
        paint(3, 41+offset);
        paint(3, 44+offset);
        paint(4, 44+offset);
        paint(4, 47+offset);
    } else {
        paint(13, 41+offset);
        paint(13, 44+offset);
        paint(14, 44+offset);
        paint(14, 47+offset);
    }
}

void drawCyan(Display d, int offset) {
    if(d == NEXT) {
        paint(2, 44+offset);
        paint(3, 44+offset);
        paint(4, 44+offset);
        paint(5, 44+offset);
    } else {
        paint(12, 44+offset);
        paint(13, 44+offset);
        paint(14, 44+offset);
        paint(15, 44+offset);
    }
}

void eraseCyan(Display d, int offset) {
    if(d == NEXT) {
        whiteout(2, 44+offset);
        whiteout(3, 44+offset);
        whiteout(4, 44+offset);
        whiteout(5, 44+offset);
    } else {
        whiteout(12, 44+offset);
        whiteout(13, 44+offset);
        whiteout(14, 44+offset);
        whiteout(15, 44+offset);
    }
}

void drawBlue(Display d, int offset) {
    if(d == NEXT) {
        paint(2, 44+offset);
        paint(3, 44+offset);
        paint(4, 44+offset);
        paint(4, 47+offset);
    } else {
        paint(12, 44+offset);
        paint(13, 44+offset);
        paint(14, 44+offset);
        paint(14, 47+offset);
    }
}

void eraseBlue(Display d, int offset) {
    if(d == NEXT) {
        whiteout(2, 44+offset);
        whiteout(3, 44+offset);
        whiteout(4, 44+offset);
        whiteout(4, 47+offset);
    } else {
        whiteout(12, 44+offset);
        whiteout(13, 44+offset);
        whiteout(14, 44+offset);
        whiteout(14, 47+offset);
    }
}

void drawYellow(Display d, int offset) {
    if(d == NEXT) {
        paint(2, 41+offset);
        paint(2, 44+offset);
        paint(3, 41+offset);
        paint(3, 44+offset);
    } else {
        paint(12, 41+offset);
        paint(12, 44+offset);
        paint(13, 41+offset);
        paint(13, 44+offset);
    }
}

void eraseYellow(Display d, int offset) {
    if(d == NEXT) {
        whiteout(2, 41+offset);
        whiteout(2, 44+offset);
        whiteout(3, 41+offset);
        whiteout(3, 44+offset);
    } else {
        whiteout(12, 41+offset);
        whiteout(12, 44+offset);
        whiteout(13, 41+offset);
        whiteout(13, 44+offset);
    }
}

void drawPurple(Display d, int offset) {
    if(d == NEXT) {
        paint(2, 41+offset);
        paint(3, 41+offset);
        paint(3, 44+offset);
        paint(4, 41+offset);
    } else {
        paint(12, 41+offset);
        paint(13, 41+offset);
        paint(13, 44+offset);
        paint(14, 41+offset);
    }
}

void erasePurple(Display d, int offset) {
    if(d == NEXT) {
        whiteout(2, 41+offset);
        whiteout(3, 41+offset);
        whiteout(3, 44+offset);
        whiteout(4, 41+offset);
    } else {
        whiteout(12, 41+offset);
        whiteout(13, 41+offset);
        whiteout(13, 44+offset);
        whiteout(14, 41+offset);
    }
}

void drawOrange(Display d, int offset) {
    if(d == NEXT) {
        paint(2, 44+offset);
        paint(3, 44+offset);
        paint(4, 44+offset);
        paint(4, 41+offset);
    } else {
        paint(12, 44+offset);
        paint(13, 44+offset);
        paint(14, 44+offset);
        paint(14, 41+offset);
    }
}

void eraseOrange(Display d, int offset) {
    if(d == NEXT) {
        whiteout(2, 44+offset);
        whiteout(3, 44+offset);
        whiteout(4, 44+offset);
        whiteout(4, 41+offset);
    } else {
        whiteout(12, 44+offset);
        whiteout(13, 44+offset);
        whiteout(14, 44+offset);
        whiteout(14, 41+offset);
    }
}