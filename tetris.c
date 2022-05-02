#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>

#include "tcp_client.h"
#include "tcp_server.h"

// to compile for windows: gcc -I/mingw64/include/ncurses -o tetris.exe tetris.c tcp_client.c tcp_server.c -lncurses -lws2_32 -lpthread -L/mingw64/bin -static
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
void updateScoreLevel(int mult, int *score, int *level, int *speedcnt, int *delay);
bool update(Orientation o, tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]);
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
int checkLine(bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]);
void save(bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH], Color current_color, int delay, int speedcnt, int level, int score, Color next_tetrimo, bool heldExists, bool heldLast, Color held_tetrimo);
void load(bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH], Color *current_color, int *delay, int *speedcnt, int *level, int *score, Color *next_tetrimo, bool *heldExists, bool *heldLast, Color *held_tetrimo);
void drawHeld(Color c, int offset);
void eraseHeld(Color c, int offset);
void drawTitle(bool isSave);
void drawOptions(int level);
void drawControls();
void drawScoreLevel(int score, int level, int offset);
void drawGameOver();
bool checkDown(tetrimo t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]);
void updateMatrix(tetrimo t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]);
void *play(void *id);
void *server(void *port);
void *client(void *con);
void getIpAddr2(char *ip);
void getPort(char *port);
int hostOrClient();
void drawSecondPlayer(char *second);

int max_y = 0;
int max_x = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

bool gameOver;
int level;
int score;
int speedcnt;
int delay;

tetrimo currentTetrimo;

    // FILE *err;

bool matrix_g[MATRIX_LENGTH-1][MATRIX_WIDTH];

int main(int argc, char *argv[]) {
    // err = fopen("err.txt", "w+");

    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    timeout(INITIAL_DELAY);
    level = 1;
    score = 0;
    speedcnt = 0;
    delay = 1000;
    while(1) {
        getmaxyx(stdscr, max_y, max_x);
        gameOver = false;
        START:
        int numOptions = 4;
        if(!fopen("savefiles/save.txt", "r")){
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
        int game;
        switch(option) {
            case 0:
                clear();
                game = 0;
                play((void *)&game);
                clear();
                drawGameOver();
                while(wgetch(stdscr) != ESC_KEY) {}
                break;
            case 1:
                clear();
                game = 1;
                play((void *)&game);
                clear();
                drawGameOver();
                while(wgetch(stdscr) != ESC_KEY) {}
                break;
            case 2:
                clear();
                //play(true, false);
                int isClient = hostOrClient();
                currentTetrimo.current_xy[0].x = 0;
                if(isClient == 1) {
                    game = 2;
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
                    // tcp_client_close(sock);pthread_t server_id;
                    FILE *fp;
                    fp = fopen("data/adddr.txt", "w+");
                    Config con;
                    clear();
                    char host[40];
                    getIpAddr2(host);
                    con.host = host;
                    if(host == NULL) {
                        break;
                    }
                    clear();
                    //con.host = "10.0.0.2";
                    char port[6];
                    getPort(port);
                    if(port == NULL) {
                        break;
                    }
                    con.port = port;
                    fputs(con.host, fp);
                    fputs(con.port, fp);
                    fclose(fp);
                    clear();
                    pthread_t client_id;
                    pthread_t play_id;
                    pthread_create(&client_id, NULL, client, (void *)&con);
                    pthread_create(&play_id, NULL, play, (void *)&game);
                    pthread_join(play_id, NULL);
                    gameOver = true;
                    pthread_join(client_id, NULL);
                    clear();
                    drawGameOver();
                    while(wgetch(stdscr) != ESC_KEY) {}
                    //play((void *)&game);
                } else if(isClient == 0) {
                    game = 3;
                    clear();
                    char port[6];
                    getPort(port);
                    if(port == NULL) {
                        break;
                    }
                    clear();
                    pthread_t server_id;
                    pthread_t play_id;
                    pthread_create(&server_id, NULL, server, (void *)port);
                    pthread_create(&play_id, NULL, play, (void *)&game);
                    pthread_join(play_id, NULL);
                    gameOver = true;
                    pthread_join(server_id, NULL);
                    clear();
                    drawGameOver();
                    while(wgetch(stdscr) != ESC_KEY) {}
                    // tcp_server_close(csock, lsock);
                } else {
                    break;
                }
                break;
            case 3:
                clear();
                drawOptions(level);
                bool options_flg;
                int new_level = level;
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
                    updateScoreLevel(1, &score, &level, &speedcnt, &delay);
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
        clear();
        gameOver = false;
        title_flg = 0;
        option = 0;
    }
    endwin();
}

void *play(void *id) {
    // char s[2];
    // sprintf(s, "%d", *p);
    // log(s);
    int *i = (int *)id;
    int game = *i;
    // bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH];
    // int score = 0;
    Color held_tetrimo;
    // int delay = 1000;
    // int speedcnt = 0;

    bool pause_flg = false;
    bool toggle_flg = false;
    bool block_flg = false;
    
    bool heldExists = false;
    bool heldLast = false;
    
    // drawScoreLevel(0);
    initMatrix(matrix_g);
    time_t ti;
    srand((unsigned) time(&ti));
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
    // SOCKET csock;
    // SOCKET lsock;
    // Config con;
    
    if(game == 0) {
        drawBoard(score, level, 0);
    } else if(game == 1) {
        drawBoard(score, level, 0);
        load(matrix_g, &c, &delay, &speedcnt, &level, &score, &next_tetrimo, &heldExists, &heldLast, &held_tetrimo);
        timeout(delay);
    }else {
        offset = 55;
        drawBoard(score, level, 0);
        drawBoard(score, level, offset);
        // if(game == 2) {
        //     con.host = "10.0.0.2";
        //     con.port = "8085";
        // } else {
        //     // tcp_server_create(&lsock);
        // }
    
    }
    
  
    int idle_cnt = 0;
    
    
    while(1) {
        if(pause_flg) {
            pthread_mutex_lock(&mutex);
            mvprintw(13,18, "PAUSED");
            while(wgetch(stdscr) != 'p'){}
            mvprintw(13,18, "      ");
            pthread_mutex_unlock(&mutex);
            pause_flg = false;
        }
        Orientation movement = DOWN;
        int key = wgetch((stdscr));
        switch (key)
        {
        case 'p':
            pause_flg = ~pause_flg;
            break;
        case '\t':
            if(cnt++ == 2) {
                cnt = 0;
                //idle_cnt++;
                pthread_mutex_lock(&mutex);
                update(movement, &t, matrix_g);
                pthread_mutex_unlock(&mutex);
            }
            pthread_mutex_lock(&mutex);
            t.toggle(&t, matrix_g);
            pthread_mutex_unlock(&mutex);
            toggle_flg = true;
            break;
        case KEY_RIGHT:
            if(cnt++ == 2) {
                cnt = 0;
                //idle_cnt++;
                pthread_mutex_lock(&mutex);
                update(movement, &t, matrix_g);
                pthread_mutex_unlock(&mutex);
            }
            // next_x = current_x + 3;
            movement = RIGHT;
            break;
        case KEY_LEFT:
            if(cnt++ == 2) {
                cnt = 0;
                //idle_cnt++;
                pthread_mutex_lock(&mutex);
                update(movement, &t, matrix_g);
                pthread_mutex_unlock(&mutex);
            }
            movement = LEFT;
            // next_x = current_x - 3;
            break;
        case 's':
            save(matrix_g, t.color, delay, speedcnt, level, score, next_tetrimo, heldExists, heldLast, held_tetrimo);
            break;
        case 'z':
            save(matrix_g, t.color, delay, speedcnt, level, score, next_tetrimo, heldExists, heldLast, held_tetrimo);
            endwin();
            exit(EXIT_SUCCESS);
            // f
            break;
        case ' ':
            if(heldLast) {
                break;
            }
            heldLast = TRUE;
            pthread_mutex_lock(&mutex);
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
            pthread_mutex_unlock(&mutex);
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
            pthread_mutex_lock(&mutex);
            gameOver = true;
            pthread_mutex_unlock(&mutex);
            // pthread_mutex_lock(&mutex);
            // clear();
            // pthread_mutex_unlock(&mutex);
            // fclose(err);
            // if(game == 2) {
            //     tcp_client_close(csock);
            // } else if (game == 3) {
            //     // tcp_server_close(csock, lsock);
            // }
            return 0;
            break;
        default:
            cnt = 0;
            break;
        }
        idle_cnt++;
        if(!toggle_flg){
            pthread_mutex_lock(&mutex);
            if(!update(movement, &t, matrix_g)) {
                updateMatrix(t, matrix_g);
                t = newBlock(RANDOM, &next_tetrimo, 0);
                heldLast = false;
            }
            pthread_mutex_unlock(&mutex);
        }
        currentTetrimo = t;
        if(gameOver) {
            // FILE *err;
            // err = fopen("err.log", "w+");
            // fputs("gamover", err);
            // pthread_mutex_lock(&mutex);
            // clear();
            // drawGameOver();
            // pthread_mutex_unlock(&mutex);
            // gameOver = FALSE;
            // while(wgetch(stdscr) != ESC_KEY) {}
            // clear();
            return 0;
        }
        toggle_flg = false;
        // mvprintw(0, 0, "%d, %d", current_x, current_y);
        // mvprintw(2, 0, "%d, %d", next_x, next_y);
        // mvprintw(4, 0, "%d", current_orientation);
        // mvprintw(6, 0, "%d", next_orientation);
        pthread_mutex_lock(&mutex);
        int endLine = checkLine(matrix_g);
        pthread_mutex_unlock(&mutex);
        if(endLine) {
            pthread_mutex_lock(&mutex);
            updateScoreLevel(endLine, &score, &level, &speedcnt, &delay);
            pthread_mutex_unlock(&mutex);
        }
    }
    refresh();
    // fclose(err);
}

void *client(void *con) {
    SOCKET c;
    char receive[234];
    char send[234];
    Config *conf = (Config *)con;
    Config config = *conf;
    while(1) {
        FILE *s;
        s = fopen("data/client_err.txt", "w+");
        
        for(int i = 0; i < 25; i++) {
            for(int j = 0; j < 9; j++) {
                if(matrix_g[i][j] == TRUE){
                    send[(i*9)+j] = '1';
                } else {
                    send[(i*9)+j] = '0';
                }
            }
        }
        
        if(currentTetrimo.current_xy[0].x != 0) {
            for(int i = 0; i < 4; i++) {
                send[(currentTetrimo.current_xy[i].y*9)+matrixtoblock(currentTetrimo.current_xy[i].x)] = '1';
            }
        }
        char lev[3];
        sprintf(lev, "%02d", level);
        send[225] = lev[0];
        send[226] = lev[1];
        char sc[6];
        sprintf(sc, "%5d", score);
        send[227] = sc[0];
        send[228] = sc[1];
        send[229] = sc[2];
        send[230] = sc[3];
        send[231] = sc[4];
        char o = '0';
        if(gameOver) {
            o = '1';
        }
        send[232] = o;
        send[233]= '\0';
        if(tcp_client_connect(config, &c)) {
            fputs("connect error", s);
        }        
        if(tcp_client_send_request(&c, send)) {
            fputs("send request error", s);
        }
        
        if(tcp_client_receive_response(&c, receive)) {
            fputs("receive response rror", s);
        }
        fclose(s);
        tcp_client_close(c);
        if(receive[232] == '1') {
            pthread_mutex_lock(&mutex);
            gameOver = true;
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_lock(&mutex);
        drawSecondPlayer(receive);
        pthread_mutex_unlock(&mutex);
        if(gameOver) {
            break;
        }
    }
}

void *server(void *port) {
    char *p = (char *)port;
    SOCKET l;
    SOCKET c;
    char receive[234];
    char send[234];
    bool firstConnect = false;
    tcp_server_create(&l, p);
    while(1) {
        FILE *q;
        q = fopen("data/server_err.txt", "w+");
        if(firstConnect == false) {
            pthread_mutex_lock(&mutex);
        }
        if(tcp_server_accept_connection(&l, &c)) {
            fputs("accept connection errror", q); 
        }
        if(firstConnect == false) {
            pthread_mutex_unlock(&mutex);
            firstConnect = true;
        }
        if(tcp_server_receive_request(&c, receive)) {
            fputs("receive request errror", q);
        }
        for(int i = 0; i < 25; i++) {
            for(int j = 0; j < 9; j++) {
                if(matrix_g[i][j] == TRUE){
                    send[(i*9)+j] = '1';
                } else {
                    send[(i*9)+j] = '0';
                }
            }
        }  
        if(currentTetrimo.current_xy[0].x != 0) {
            for(int i = 0; i < 4; i++) {
                send[(currentTetrimo.current_xy[i].y*9)+matrixtoblock(currentTetrimo.current_xy[i].x)] = '1';
            }
        }
        char lev[3];
        sprintf(lev, "%02d", level);
        send[225] = lev[0];
        send[226] = lev[1];
        char sc[6];
        sprintf(sc, "%5d", score);
        send[227] = sc[0];
        send[228] = sc[1];
        send[229] = sc[2];
        send[230] = sc[3];
        send[231] = sc[4];
        char o = '0';
        if(gameOver) {
            o = '1';
        }
        send[232] = o;
        send[233]= '\0';
        if(receive[232] == '1') {
            pthread_mutex_lock(&mutex);
            gameOver = true;
            pthread_mutex_unlock(&mutex);
            break;
        }
        if(tcp_server_send_response(&c, send)) {
            fputs("send error", q);
        }
        pthread_mutex_lock(&mutex);
        drawSecondPlayer(receive);
        pthread_mutex_unlock(&mutex);
        fclose(q);
        if(gameOver) {
            break;
        }
    }
    
    tcp_server_close(c, l);
}

void drawSecondPlayer(char *second) {
    for(int i = 0; i < 25; i++) {
        for(int j = 0; j < 9; j++) {
            if(second[(i*9)+j] == '1'){
                paint(i, blocktomatrix2(j));
            } else if(second[(i*9)+j] == '0') {
                whiteout(i, blocktomatrix2(j));
            }
        }
    }
    // char lev[3];
    // lev[0] = second[225];
    // lev[1] = second[226];
    // lev[2] = '\0';
    // int l = atoi(lev);
    mvprintw(27,21+55, "     ");
    mvprintw(27,16+55,"Level:    %c%c", second[225], second[226]);
    if(second[227] == '0') {
        if(second[228] == '0') {
            if(second[229] == '0') {
                return;
            }
            mvprintw(26,21+55, "     ");
            mvprintw(26,16+55,"Score: %c%c%c", second[229], second[230], second[231]);
            return;
        }
        mvprintw(26,21+55, "     ");
        mvprintw(26,16+55,"Score: %c%c%c%c", second[228], second[229], second[230], second[231]);
        return;
    }
    mvprintw(26,21+55, "     ");
    mvprintw(26,16+55,"Score: %c%c%c%c%c", second[227], second[228], second[229], second[230], second[231]);
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
        mvprintw(15, 26, "2-PLAYER");
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
    mvprintw(13, 26, "Level: %2d", level);
    mvprintw(15, 22, "(ESC to go back)");
}

int hostOrClient() {
    mvprintw(13, ARROW_X, "->");
    mvprintw(13, 26, "HOST");
    mvprintw(15, 26, "JOIN");
    mvprintw(17, 22, "(ESC to go back)");
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
                break;
            case ESC_KEY:
                option = 2;
            default:
                break;
        }
    }
    clear();
    return option;
}

void getPort(char *port) {
    char port_str[6];
    mvprintw(13, 26,  "Enter Port:");
    bool select_flg = false;
    int pos = 0;
    while(!select_flg) {
        char c = wgetch(stdscr);
        if(pos == 6) {
            select_flg = true;
        }
        
        if (c >= '0' && c <= '9') {
            port_str[pos++] = c;
            // move(pos++, 15);
            // addchr(c);
        } else if(c == '\n') {
            select_flg = true;
            
        } else if(c == '\b') {
            if(pos != 0) {
                pos--;
            }
        } else if( c == ESC_KEY) {
            return;
        }
        port_str[pos] = '\0';
        mvprintw(15, 26, "         ");
        mvprintw(15, 26, port_str);
    }
    strcpy(port, port_str);
}

void getIpAddr2(char *ip) {
    mvprintw(13, 26, "ENTER HOST IP ADDRESS:");
    mvprintw(17, 26, "(ENTER TO PROCEED)");
    char ip_str[40];
    bool select_flg = false;
    int pos = 0;
    while(!select_flg) {
        char c = wgetch(stdscr);
        if(pos == 30) {
            select_flg = true;
        }
        
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || c == ':' || c == '.') {
            ip_str[pos++] = c;
            // move(pos++, 15);
            // addchr(c);
        } else if(c == '\n') {
            select_flg = true;
            
        } else if(c == '\b') {
            if(pos != 0) {
                pos--;
            }
        } else if( c == ESC_KEY) {
            return;
        }
        ip_str[pos] = '\0';
        mvprintw(15, 26, "                                  ");
        mvprintw(15, 26, ip_str);
    }
    strcpy(ip, ip_str);
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
            new_tetrimo.current_xy[0].x = 19+offset;
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
    drawScoreLevel(score, level, offset);
    if(!offset) {
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
    savefp = fopen("savefiles/save.txt", "w+");
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
    if(!(loadfp = fopen("savefiles/save.txt", "r+"))) {
        FILE *err;
        fopen("data/err.txt", "w+");
        fputs("Unable to open file save.txt", err);
        fclose(err);
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

void drawScoreLevel(int score, int level, int offset) {
    mvprintw(26,21+offset, "     ");
    mvprintw(26,16+offset,"Score: %5d", score);
    mvprintw(27,21+offset, "     ");
    mvprintw(27,16+offset,"Level:    %02d", level);
}
void updateScoreLevel(int mult, int *score, int *level, int *speedcnt, int *delay) {
    (*score) += (100 * mult);
    (*speedcnt) += (100 * mult);
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
    drawScoreLevel(*score, *level, 0);
}

int checkLine(bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    int first = 0;
    int last = 0;
    // bool cleared = false;
    int numLines = 0;
    
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
                last = i;
                if(first == 0) {
                    first = i;
                }
            }
        } 
    }

    if(first != 0 && last != 0) {
        numLines = first - last + 1;
        replaceLines(first, last, matrix);
    }
    return numLines;
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
bool update(Orientation o, tetrimo *t, bool matrix[MATRIX_LENGTH-1][MATRIX_WIDTH]) {
    // if checkDown
    if(shift(o, t, matrix)) {
        for(int i = 0; i < 4; i++) {
            (*t).next_xy[i].x = (*t).current_xy[i].x;
            (*t).next_xy[i].y = (*t).current_xy[i].y;
            if((*t).current_xy[i].y==0) {
                (gameOver) = TRUE;
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