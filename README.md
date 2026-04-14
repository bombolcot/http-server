# http-server
This is a simple http server, written in C++ from scratch using raw sockets.

## Building
To compile, run `g++ main.cpp -o server` in the terminal.
Run the commands in the project folder directory.
All the commands are for Linux/Unix terminal (WSL on Windows).

## Running
Run `./server` in the terminal to start the server.
Once the server is started, open a browser and go to `http://localhost:8080/index.html`

## Features
- The server provides HTML and CSS files.
- 404 handling for files that don't exist
- 400 handling for malformed requests
- 413 handling for requests that are too large
- Graceful handling of disconnected clients

## Planned improvements

 - Make `file_buffer` size dynamic, refuse to send a response if the requested file size is too big.
 - Add threading
