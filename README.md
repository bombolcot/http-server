# http-server
This is a simple http server, written in C++ from scratch using raw sockets.

## Building
To compile, run `g++ main.cpp -o server` in the terminal.
Run the commands in the project folder directory.
This project is written for Linux/Unix systems. Windows users can use WSL.

## Running
Run `./server` in the terminal to start the server.
Once the server is started, open a browser and go to `http://localhost:8080/index.html`

## Features
- 404 handling for files that don't exist
- 400 handling for malformed requests
- 413 handling for requests that are too large
- Graceful handling of disconnected clients
- Dynamic file buffer sizing
- Multi-threaded connection handling
- Logs each requested path, response code, and timestamp
- Supports multiple file types (HTML, CSS, JavaScript) with correct Content-Type headers
