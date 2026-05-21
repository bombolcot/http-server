#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include <ctime>
#include <map>
#include <string>

const long MAX_FILE_SIZE = 10 * 1024 * 1024;
// parses the buffer with the file path and extracts it to filepath
// to make it usable in fopen
// checks if the request is not malformed
bool parseFilePath (char* buffer, char* filepath) {
    char path[256]{};

    char* ptr_start = strchr(buffer, ' '); // read path start
    if (ptr_start == nullptr) return false;
    ptr_start += 1;

    char* ptr_end = strchr(ptr_start, ' '); // read path end
    if (ptr_end == nullptr) return false;

    if (ptr_end - ptr_start >= 256) return false;
    strncpy(path, ptr_start, ptr_end - ptr_start); // put in the entire path into the variable "path"
    path[ptr_end - ptr_start] = '\0'; // null termination
    if (strcmp(path, "/") == 0) { // make sure there is a directory/path to read
        strcat(path, "/index.html");
    }

    snprintf(filepath, 256, "%s", path + 1);
    return true;
}

// reads the file and writes it to a file buffer
// returns the file size or -1 if the file can't be opened
std::vector<char> readFileToBuffer(FILE* fp, long file_size) {
    std::vector<char> file_buffer;
    file_buffer.resize(file_size);
    fseek(fp, 0, SEEK_SET);
    fread(file_buffer.data(), sizeof(char), file_size, fp);
    fclose(fp);
    return file_buffer;
}

// sends the response, checks the file type (css or html), sets content type accordingly
// the response is sent at the end as well as the content itself
void sendResponse(int client_fd, std::vector<char>& file_buffer, char* filepath) {
    static std::map<std::string, const char*> content_type_map{
        {".css", "text/css"},
        {".html", "text/html"},
        {".js", "text/js"}
    };
    char header[256]{};
    const char* ext = strrchr(filepath, '.');
    const char* content_type = "application/octet-stream";
    if (ext != nullptr) {
        std::string extension(ext);
        auto it = content_type_map.find(extension);
        if (it != content_type_map.end()) {
            content_type = it->second;
        }
    }
    snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n", content_type, file_buffer.size());
    send(client_fd, header, strlen(header), 0);
    send(client_fd, file_buffer.data(), file_buffer.size(), 0);
}

// sends error messages, closing client_fd handled in the main loop
void sendError(int client_fd, int status) {
    char error_header[256]{};
    const char* error_body = nullptr;
    const char* status_line = nullptr;
    if (status == 404) {
        status_line = "404 Not Found";
        error_body = "<h1>404 Not Found</h1>";
    } else if (status == 400) {
        status_line = "400 Bad Request";
        error_body = "<h1>400 Bad Request</h1>";
    } else if (status == 413) {
        status_line = "413 Request Entity Too Large";
        error_body = "<h1>413 Request Entity Too Large</h1>";
    } else {
        status_line = "500 Internal Server Error";
        error_body = "<h1>500 Internal Server Error</h1>";
    }
    snprintf(error_header, sizeof(error_header), "HTTP/1.1 %s\r\nContent-Length: %zu\r\n\r\n", status_line, strlen(error_body));
    send(client_fd, error_header, strlen(error_header), 0);
    send(client_fd, error_body, strlen(error_body), 0);
}

// logs a response in the terminal. shows the requested path, response code and a timestamp
void logRequest(const char* filepath, int status_code) {
    time_t t = time(nullptr);
    struct tm timeinfo{};
    localtime_r(&t, &timeinfo);
    char timestr[256]{};
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %T", &timeinfo);
    printf("[%s] %d %s\n", timestr, status_code, filepath);
}

void handleClient(int client_fd) {
    // read the file path, and write it to a variable for opening
    char buffer[1024]{}; // this contains raw http data
    int bytes_recieved = recv(client_fd, buffer, sizeof(buffer), 0);
    if (bytes_recieved == 0 || bytes_recieved == -1) {
        close(client_fd);
        return;
    }
    // check if recv didn't return more than buffer size
    if (bytes_recieved == sizeof(buffer)) {
        sendError(client_fd, 413);
        logRequest("unknown", 413);
        close(client_fd);
        return;
    }

    char filepath[256]{}; // this contains the path for opening a requested file
    if (!parseFilePath(buffer, filepath)) {
        sendError(client_fd, 400);
        logRequest("malformed request", 400);
        close(client_fd);
        return;
    }

    FILE* fp = fopen(filepath, "rb");
    if (!fp) {
        sendError(client_fd, 404);
        logRequest(filepath, 404);
        close(client_fd);
        return;
    }
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    if (file_size > MAX_FILE_SIZE) {
        sendError(client_fd, 413);
        logRequest(filepath, 413);
        fclose(fp);
        close(client_fd);
        return;
    }
    std::vector<char> file_buffer = readFileToBuffer(fp, file_size);
    if (file_buffer.empty()) {
        sendError(client_fd, 404);
        logRequest(filepath, 404);
        close(client_fd);
        return;
    }

    sendResponse(client_fd, file_buffer, filepath);
    logRequest(filepath, 200);

    close(client_fd);
}

int main(int argc, char* argv[]) {
    int port = 8080; // 8080 is the default port
    if (argc > 1) {
        std::string passedPort = argv[1];
        port = std::stoi(passedPort);
    }
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
    addr.sin_port = htons(port); // port converted to network byte order

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
        std::thread(handleClient, client_fd).detach();
    }
    close(sockfd);
    return 0;
}