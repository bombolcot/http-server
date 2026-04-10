#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// parses the buffer with the file path and extracts it to filepath
// to make it usable in fopen
void parseFilePath (char* buffer, char* filepath) {
    char path[256]{};
    char* ptr_start = strchr(buffer, ' ') + 1; // read path start
    char* ptr_end = strchr(ptr_start, ' '); // read path end
    strncpy(path, ptr_start, ptr_end - ptr_start); // put in the entire path into the variable "path"
    path[ptr_end - ptr_start] = '\0'; // null termination
    if (strcmp(path, "/") == 0) { // make sure there is a directory/path to read
        strcat(path, "/index.html");
    }

    snprintf(filepath, 256, "%s", path + 1);
}

// reads the file and writes it to a file buffer
// returns the file size or -1 if the file can't be opened
long readFileToBuffer(char* filepath, char* file_buffer, size_t buffer_size) {
    FILE* fp = fopen(filepath, "rb");
    if (!fp) { // 404 response handled in the loop
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    size_t ret = fread(file_buffer, 1, buffer_size, fp);
    file_buffer[ret] = '\0';
    fclose(fp);
    return file_size;
}

// sends the response, checks the file type (css or html), sets content type accordingly
// the response is sent at the end as well as the content itself
void sendResponse(int client_fd, char* file_buffer, long file_size, char* filepath) {
    char header[256]{};
    const char* content_type;
    if (strstr(filepath, ".css")) {
        content_type = "text/css";
    } else {
        content_type = "text/html";
    }
    // printf(header);
    snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n",content_type, file_size);
    send(client_fd, header, strlen(header), 0);
    send(client_fd, file_buffer, file_size, 0);
}

int main() {
    // create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
        return 1;
    }

    // allows port reuse
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // sets up the address (IP + port)
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; // listen on all interfaces
    addr.sin_port = htons(8080); // port 8080, converted to network byte order

    // binds socket to address
    if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind failed");
        return 1;
    }

    // starts listening (queue up to 10 pending connections)
    if (listen(sockfd, 10) < 0) {
        perror("listen failed");
        return 1;
    }

    // accepts a connection
    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_addr_size = sizeof(client_addr);
        int client_fd = accept(sockfd, (struct sockaddr *) &client_addr, &client_addr_size);
        if (client_fd < 0) {
            perror("accept failed");
            return 1;
        }

        // read the file path, and write it to a variable for opening
        char buffer[1024]{}; // this contains raw http data
        recv(client_fd, buffer, sizeof(buffer), 0);
        char filepath[256]{}; // this contains the path for opening a requested file

        parseFilePath(buffer, filepath);

        char file_buffer[4096]{}; // this is a variable to which the content of a file is written to
        char error_header[256]{};
        const char* error_body = "<h1>404 Not Found</h1>";
        long file_size = readFileToBuffer(filepath, file_buffer, sizeof(file_buffer));

        if (file_size < 0) {
            snprintf(error_header, sizeof(error_header), "HTTP/1.1 404 Not Found\r\nContent-Length: %zu\r\n\r\n", sizeof(error_body));
            send(client_fd, error_header, strlen(error_header), 0);
            send(client_fd, error_body, strlen(error_body), 0);
            close(client_fd);
            continue;
        }

        sendResponse(client_fd, file_buffer, file_size, filepath);

        close(client_fd);
    }
    close(sockfd);
    return 0;
}