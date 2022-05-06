# tetris
Tetris in ncurses

Link to download zip for convenience:
https://github.com/levifleming/tetris/archive/refs/heads/main.zip

Tetris written in c using ncurses for terminal drawing. Includes saves, option to change start level, and 2-Player over LAN made possible by TCP sockets.

Compiled for windows using WinGW:

gcc -I/mingw64/include/ncurses -o tetris.exe tetris.c tcp_client.c tcp_server.c -lncurses -lws2_32 -lpthread -L/mingw64/bin -static

I've included a windows executable for convenience.

Includes a startup menu, navigate with arrow keys and enter to select, ESC to go back.

Controls for tetris game are in Controls option of startup menu.

For 2-Player:
Works great over LAN, make sure the client knows the local ip address of the host. For WAN, it only works so far if port forwarding is set up on the host's
network, the client would then connect to host's Public IPV4 address.

TODO:

Squash bugs

Figure out tcp sockets for Linux version

Have same random seed for blocks in 2-Player


