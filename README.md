# http-server
This is a simple http server, written in C++ from scratch using raw sockets.

## Building
To compile, run `g++ main.cpp -o server` in the terminal.
Run the commands in the project folder directory.

## Running
Run `./server` in the terminal to start the server.

## Features
The server provides HTML and CSS files. It has 404 Not Found error handling.

## Planned improvements

 - Check the return values of `strchr` for `null`, add handling for it.
 - Handle `recv` return value `0` and `-1`. Check if it didn't return more than the buffer size.
 - Make `file_buffer` size dynamic, refuse to send a response if the requested file size is too big.
 - Add threading
