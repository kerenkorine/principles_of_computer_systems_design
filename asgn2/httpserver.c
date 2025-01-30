#include <error.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#include "asgn2_helper_funcs.h"

const unsigned int MAX_PORT_VALUE = 65535;
const unsigned int MIN_PORT_VALUE = 1;
const unsigned int REQ_SIZE = 4 * 1024;
const unsigned int METHOD_LEN_MAX = 9;
const unsigned int MAX_URI = 64;
const unsigned int MAX_SUPPORTED_VERSION_LEN = 9;
const unsigned int CONTENT_LENGTH = 50;

const char SPACE = ' ';
const char SLASH = '/';
const char BACK_R = '\r';
const char BACK_N = '\n';
const char VERSION_STRING[] = "HTTP/1.1";
const char CONTENT_STRING[] = "Content-Length";

static int socket_fd = -1;

int check_port_in_range(unsigned long port);
void server_logic(Listener_Socket *sock);
void handle_message(int sock_fd);
int handle_request(char *request_line, char *response);
void create_response(char *buffer, int code);
int handle_get(char *request, int current_index, char *response);
int handle_put(char *request, int current_index);
char *itoa(int val);

int main(int argc, char *argv[]) {
    Listener_Socket sock;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    unsigned long port_number = atol(argv[1]); // creating variable with port.

    if (!check_port_in_range(port_number)) {
        fprintf(stderr, "Invalid Port\n");
        exit(1);
    }

    if (0 != listener_init(&sock, port_number)) {
        fprintf(stderr, "Invalid Port\n");
        exit(1);
    }

    server_logic(&sock);
}

char *itoa(int val) {
    int base = 10;
    static char buf[32] = { 0 };

    int i = 30;

    for (; val && i; --i, val /= base)

        buf[i] = "0123456789abcdef"[val % base];

    return &buf[i + 1];
}

void server_logic(Listener_Socket *sock) {
    while (1) {
        int conn_status = listener_accept(sock);
        if (-1 == conn_status) {
            close(sock->fd);
            return;
        }
        socket_fd = conn_status;
        handle_message(conn_status);

        close(conn_status);
    }
}

int check_port_in_range(unsigned long port) {
    return MIN_PORT_VALUE <= port && MAX_PORT_VALUE >= port;
}

void handle_message(int sock_fd) {
    char request[REQ_SIZE];
    memset(request, 0, REQ_SIZE);
    char response[REQ_SIZE];
    memset(request, 0, REQ_SIZE);
    read(sock_fd, request, REQ_SIZE);
    int code = handle_request(request, response);
    create_response(response, code);
    write_all(sock_fd, response, strlen(response));
}

int handle_request(char *request_line, char *response) {
    char method[METHOD_LEN_MAX];
    memset(method, 0, sizeof(METHOD_LEN_MAX));

    int current_index = 0;

    for (unsigned int i = 0; i < METHOD_LEN_MAX; i++) {
        if (SPACE == request_line[i]) {
            current_index = i;
            method[i] = 0;
            break;
        }
        memcpy(method + i, request_line + i, 1);
    }

    if (0 == strcmp("GET", method)) {
        return handle_get(request_line, current_index, response);
    } else if (0 == strcmp("PUT", method)) {
        response[0] = 0;
        return handle_put(request_line, current_index);
    } else {
        return 501;
    }
}

int handle_get(char *request, int current_index, char *response) {
    char uri[MAX_URI];
    char version[MAX_SUPPORTED_VERSION_LEN];
    memset(uri, 0, sizeof(MAX_URI));
    memset(version, 0, sizeof(MAX_SUPPORTED_VERSION_LEN));

    current_index++;

    if (SLASH != request[current_index]) {
        return 400;
    }
    current_index++;

    for (unsigned int i = current_index; i < MAX_URI + current_index; i++) {
        if (SPACE == request[i]) {
            uri[i - current_index] = 0;
            current_index = i;
            break;
        }

        memcpy(uri + i - current_index, request + i, 1);
    }

    current_index++;

    for (unsigned int i = current_index; i < MAX_SUPPORTED_VERSION_LEN + current_index; i++) {

        if (BACK_R == request[i] && BACK_N == request[i + 1]) {
            version[i - current_index] = 0;
            current_index = i;
            break;
        }

        memcpy(version + i - current_index, request + i, 1);
    }

    if (strlen(version) > 8) {
        return 400;
    }

    if (strcmp(version, VERSION_STRING)) {
        return 505;
    }

    if (access(uri, F_OK) == 0) {
        // file exists
        struct stat sb;
        stat(uri, &sb);
        int fd = open(uri, O_RDWR, 777);

        if (-1 == fd) {
            return 403;
        }

        if (sb.st_size > REQ_SIZE) {
            memcpy(response, "HTTP/1.1 200 OK\r\nContent-Length: ", 34);
            char *length = itoa(sb.st_size);
            memcpy(response + 32, length, strlen(length));
            memcpy(response + 32 + strlen(length), "\r\n\r\n", 4);
            write_all(socket_fd, response, 36 + strlen(length));
            int curr_offset = 0;
            while (curr_offset < sb.st_size) {
                ssize_t read_bytes = pread(fd, response, REQ_SIZE, curr_offset);
                curr_offset += read_bytes;
                write_all(socket_fd, response, read_bytes);
            }
            close(fd);
            return 1;
        }

        read(fd, response, sb.st_size);
        response[sb.st_size] = 0;

        close(fd);
        return 200;
    } else {
        // file doesn't exist
        return 404;
    }
}

int handle_put(char *request, int current_index) {
    char uri[MAX_URI];
    char version[MAX_SUPPORTED_VERSION_LEN];
    memset(uri, 0, sizeof(MAX_URI));
    memset(version, 0, sizeof(MAX_SUPPORTED_VERSION_LEN));

    current_index++;

    if (SPACE == request[current_index] || SPACE == request[current_index + 1]) {
        return 400;
    }

    if (SLASH != request[current_index]) {
        return 400;
    }

    current_index++;

    for (unsigned int i = current_index; i < MAX_URI + current_index; i++) {
        if (SPACE == request[i]) {
            uri[i - current_index] = 0;
            current_index = i;
            break;
        }

        memcpy(uri + i - current_index, request + i, 1);
    }

    current_index++;

    for (unsigned int i = current_index; i < MAX_SUPPORTED_VERSION_LEN + current_index; i++) {

        if (BACK_R == request[i] && BACK_N == request[i + 1]) {
            version[i - current_index] = 0;
            current_index = i;
            break;
        }

        memcpy(version + i - current_index, request + i, 1);
    }

    if (strlen(version) > 8) {
        return 400;
    }

    if (strcmp(VERSION_STRING, version)) {
        return 505;
    }

    char contents[CONTENT_LENGTH];
    int content_length = 0;
    char *needle = NULL;

    if (access(uri, F_OK) == 0) {
        // file exists
        needle = strstr(request, CONTENT_STRING);

        if (!needle) {
            return 400;
        }

        char *needle_end = strstr(needle + 16, "\r\n");
        memcpy(contents, needle + 16, needle_end - needle);
        contents[needle_end - needle] = 0;
        content_length = atoi(contents);
        needle = strstr(request, "\r\n\r\n");
        needle += 4;

        int fd = open(uri, O_TRUNC | O_RDWR, 0644);

        if (-1 == fd) {
            return 403;
        }

        size_t remaining_data_size = content_length;

        if (needle[0] != 0) {
            if (content_length < REQ_SIZE - (needle - request)) {
                write(fd, needle, content_length);
                close(fd);
                return 200;
            } else {
                write(fd, needle, REQ_SIZE - (needle - request));
            }
        }

        if (content_length < REQ_SIZE - (needle - request)) {
            read(socket_fd, request, content_length);
            write(fd, request, content_length);
        } else {
            ssize_t written = 0;
            while (remaining_data_size > 0) {
                remaining_data_size -= written;
                read(socket_fd, request, REQ_SIZE);
                written = write(fd, request, REQ_SIZE);
            }
        }
        close(fd);
        return 200;
    } else {
        // file doesn't exist
        needle = strstr(request, CONTENT_STRING);

        if (!needle) {
            return 400;
        }

        char *needle_end = strstr(needle + 16, "\r\n");
        memcpy(contents, needle + 16, needle_end - needle);
        contents[needle_end - needle] = 0;
        content_length = atoi(contents);
        needle = strstr(request, "\r\n\r\n");
        needle += 4;

        int fd = open(uri, O_CREAT | O_RDWR, 0644);

        if (-1 == fd) {

            return 403;
        }
        size_t remaining_data_size = content_length;
        if (content_length < REQ_SIZE - (needle - request)) {
            read(socket_fd, request, content_length);
            write(fd, request, content_length);
        } else {
            ssize_t written = 0;
            while (remaining_data_size > 0) {
                remaining_data_size -= written;
                read(socket_fd, request, REQ_SIZE);
                written = write(fd, request, REQ_SIZE);
            }
        }
        close(fd);
        return 201;
    }
}

void create_response(char *buffer, int code) {
    char temp_buffer[REQ_SIZE];
    memset(temp_buffer, 0, REQ_SIZE);
    switch (code) {
    case (200):
        if (buffer[0] == 0) {
            memcpy(buffer, "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n", 43);
            break;
        }
        memcpy(temp_buffer, buffer, strlen(buffer));
        temp_buffer[strlen(buffer)] = 0;
        memcpy(buffer, "HTTP/1.1 200 OK\r\nContent-Length: ", 34);
        char *length = itoa(strlen(temp_buffer));
        memcpy(buffer + 32, length, strlen(length));
        memcpy(buffer + 32 + strlen(length), "\r\n\r\n", 4);
        memcpy(buffer + 36 + strlen(length), temp_buffer, strlen(temp_buffer));
        buffer[36 + strlen(length) + strlen(temp_buffer)] = 0;
        break;
    case (201):
        memcpy(buffer, "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\n", 53);
        break;
    case (400):
        memcpy(buffer, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n", 61);
        break;
    case (403):
        memcpy(buffer, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n", 58);
        break;
    case (404):
        memcpy(buffer, "HTTP/1.1 404 Not Found\r\nContent-Length: 10\r\n\r\nNot Found\n", 58);
        break;
    case (500):
        memcpy(buffer,
            "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 22\r\n\r\nInternal Server "
            "Error\n",
            82);
        break;
    case (501):
        memcpy(buffer,
            "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n", 70);
        break;
    case (505):
        memcpy(buffer,
            "HTTP/1.1 505 Version Not Supported\r\nContent-Length: 22\r\n\r\nVersion Not "
            "Supported\n",
            82);
        break;
    default: break;
    }
}
