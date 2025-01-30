#include "asgn4_helper_funcs.h"
#include "connection.h"
#include "debug.h"
#include "request.h"
#include "response.h"
#include "bool.h"
#include "threadpool.h"
#include "queue.h"

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#define DEFAULT_THREAD_POOL_SIZE (4)

void audit_log(conn_t *conn, unsigned int status);
void handle_connection(Thread_Task_Payload *payload);
const Response_t *handle_get(conn_t *, Thread_Task_Payload *);
const Response_t *handle_put(conn_t *, Thread_Task_Payload *);
const Response_t *handle_unsupported(void);
int parse_args(int, char **, size_t *, size_t *);
int get_full_path(conn_t *, char *);

int main(int argc, char **argv) {
    char *endptr = NULL;
    size_t num_of_threads = DEFAULT_THREAD_POOL_SIZE;
    // process 'threads' argument
    int opt;
    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
        case 't':
            num_of_threads = strtoull(optarg, &endptr, 10);
            if (endptr && *endptr != '\0') {
                warnx("invalid threads value: %s", optarg);
                return EXIT_FAILURE;
            }
            endptr = NULL;
            break;
        }
    }

    if (argc <= optind) {
        warnx("wrong arguments: %s port_num", argv[0]);
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    endptr = NULL;
    size_t port = (size_t) strtoull(argv[optind], &endptr, 10);
    if (endptr && *endptr != '\0') {
        warnx("invalid port number: %s", argv[optind]);
        return EXIT_FAILURE;
    }

    Thread_Pool *threadpool = threadpool_init(num_of_threads);
    if (threadpool == NULL) {
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);

    Listener_Socket sock;
    listener_init(&sock, port);

    while (TRUE) {
        int connfd = listener_accept(&sock);
        if (connfd == -1) {
            break;
        }
        threadpool_dispatch(
            threadpool, (void (*)(Thread_Task_Payload *)) handle_connection, connfd);
    }

    threadpool_destroy(threadpool);

    return EXIT_SUCCESS;
}

void audit_log(conn_t *conn, unsigned int status_code) {
    const Request_t *req = conn_get_request(conn);
    const char *req_str = request_get_str(req);
    const char *uri = conn_get_uri(conn);
    const char *header = conn_get_header(conn, "Request-Id");
    fprintf(stderr, "%s,%s,%u,%s\n", req_str, uri, status_code, header);
}

void handle_connection(Thread_Task_Payload *payload) {
    int connfd = payload->connfd;
    conn_t *conn = conn_new(connfd);
    const Response_t *res = conn_parse(conn);

    if (res == NULL) {

        const Request_t *req = conn_get_request(conn);

        if (req == &REQUEST_GET) {
            res = handle_get(conn, payload);
        } else if (req == &REQUEST_PUT) {
            res = handle_put(conn, payload);
        } else {
            res = handle_unsupported();
        }
    }

    if (res != NULL) {
        conn_send_response(conn, res);
    }

    conn_delete(&conn);
    close(connfd);
}

static int is_file(char *path) {
    struct stat path_info;
    return lstat(path, &path_info) != -1 && S_ISREG(path_info.st_mode);
}

const Response_t *handle_get(conn_t *conn, Thread_Task_Payload *payload) {
    int connfd = payload->connfd;
    const char *request_id = conn_get_header(conn, "Request-Id");
    char *uri = conn_get_uri(conn);
    /* char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return &RESPONSE_INTERNAL_SERVER_ERROR;
    }*/

    const Response_t *res = NULL;
    char *file_path = uri;

    pthread_mutex_lock(payload->mutex);
    int file_exists = is_file(file_path);
    pthread_mutex_unlock(payload->mutex);
    if (file_exists) {
        int fd = open(file_path, O_RDONLY);
        if (fd != -1) {
            flock(fd, LOCK_SH);
            off_t file_size = lseek(fd, 0, SEEK_END);
            lseek(fd, 0, SEEK_SET);
            debug("id=%s connfd=%d reading file of size %lu\n", request_id, connfd, file_size);

            conn_send_file(conn, fd, file_size);
            audit_log(conn, response_get_code(&RESPONSE_OK));
            flock(fd, LOCK_UN);
            close(fd);
            return NULL;
        } else if (errno == EACCES) {
            debug("id=%s connfd=%d cannot access file\n", request_id, connfd);
            res = &RESPONSE_FORBIDDEN;
        } else { //if (errno == ENOENT) {
            debug("id=%s connfd=%d file does not exist\n", request_id, connfd);
            res = &RESPONSE_NOT_FOUND;
        }
    } else {
        debug("id=%s connfd=%d could not find file %s\n", request_id, connfd, file_path);
        res = &RESPONSE_NOT_FOUND;
    }

    audit_log(conn, response_get_code(res));
    return res;
}

const Response_t *handle_put(conn_t *conn, Thread_Task_Payload *payload) {
    int connfd = payload->connfd;
    const char *request_id = conn_get_header(conn, "Request-Id");
    char *uri = conn_get_uri(conn);

    /*char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return &RESPONSE_INTERNAL_SERVER_ERROR;
    }*/

    char *file_path = uri;

    pthread_mutex_lock(payload->mutex);
    int fd = open(file_path, O_WRONLY | O_CREAT | O_EXCL, 0777);
    int create = fd != -1;
    if (!create) {
        fd = open(file_path, O_WRONLY, 0777);
        if (fd == -1 && errno == EACCES) {
            pthread_mutex_unlock(payload->mutex);
            return &RESPONSE_FORBIDDEN;
        }
    }
    flock(fd, LOCK_EX);
    pthread_mutex_unlock(payload->mutex);

    if (create) {
        // file creation
        debug("id=%s connfd=%d creating file\n", request_id, connfd);
        create = 1;
        ftruncate(fd, 0);
        conn_recv_file(conn, fd);
        audit_log(conn, response_get_code(&RESPONSE_CREATED));
        flock(fd, LOCK_UN);
        close(fd);
        return &RESPONSE_CREATED;
    } else {
        // file overwrite
        debug("id=%s connfd=%d writing file\n", request_id, connfd);
        ftruncate(fd, 0);
        conn_recv_file(conn, fd);
        audit_log(conn, response_get_code(&RESPONSE_OK));
        flock(fd, LOCK_UN);
        close(fd);
        return &RESPONSE_OK;
    }
}

const Response_t *handle_unsupported(void) {

    return &RESPONSE_NOT_IMPLEMENTED;
}

int parse_args(int argc, char **argv, size_t *port, size_t *num_of_threads) {
    char *endptr = NULL;
    *num_of_threads = DEFAULT_THREAD_POOL_SIZE;
    // process 'threads' argument
    int opt;
    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
        case 't':
            *num_of_threads = strtoull(optarg, &endptr, 10);
            if (endptr && *endptr != '\0') {
                warnx("invalid threads value: %s", optarg);
                return EXIT_FAILURE;
            }
            endptr = NULL;
            break;
        }
    }

    if (argc <= optind) {
        warnx("wrong arguments: %s port_num", argv[0]);
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    endptr = NULL;
    *port = (size_t) strtoull(argv[optind], &endptr, 10);
    if (endptr && *endptr != '\0') {
        warnx("invalid port number: %s", argv[optind]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int get_full_path(conn_t *conn, char *buf) {
    char *uri = conn_get_uri(conn);
    /*if (getcwd(buf, sizeof(buf)) == NULL) {
        return EXIT_FAILURE;
    }*/
    strncat(strcat(buf, "/"), uri, strlen(uri));
    return EXIT_SUCCESS;
}
