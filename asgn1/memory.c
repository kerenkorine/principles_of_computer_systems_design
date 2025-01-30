#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdbool.h>

// 512 KB
#define FILE_BUFFER_SIZE (512 * 1024)

/********************************************************
This is a function that takes a filename as a parameter
and reads the contents of that file to standard output.
********************************************************/
void get_file(const char *filename) {

    char a, b, c;

    // verify no additional input or it is EOF
    if (read(STDIN_FILENO, &a, 1) && read(STDIN_FILENO, &b, 1) && read(STDIN_FILENO, &c, 1)
        && a != '^' && b != 'D' && c != '\n') {
        fprintf(stderr, "Invalid Command\n");
        exit(1);
    }

    // checks if the file exists.
    // checks if the file is a directory

    struct stat file_stat;
    if (stat(filename, &file_stat) != 0) {
        fprintf(stderr, "Invalid Command\n");
        exit(1);
    } else if (S_ISDIR(file_stat.st_mode)) {
        fprintf(stderr, "Invalid Command\n");
        exit(1);
    }

    // opens the file for reading.
    int fd = open(filename, O_RDONLY, 0);
    if (fd == -1) {
        perror(filename);
        exit(1);
    }

    // reads the contents of the file in chunks of PATH_MAX bytes and writes them to standard output
    char buffer[PATH_MAX];
    ssize_t read_bytes = 0;
    while ((read_bytes = read(fd, buffer, PATH_MAX)) > 0) {
        ssize_t written_bytes = 0;
        while (written_bytes < read_bytes) {
            ssize_t result
                = write(STDOUT_FILENO, buffer + written_bytes, read_bytes - written_bytes);
            if (result == -1) {
                perror("Error writing to standard output");
                close(fd);
                exit(1);
            }
            written_bytes += result;
        }
    }

    if (read_bytes == -1) {
        perror("Error reading from file");
        close(fd);
        exit(1);
    }

    close(fd);
}

void set_file(const char *filename) {
    char content[FILE_BUFFER_SIZE];
    struct stat file_stat;

    // check file is not a dir
    if (stat(filename, &file_stat) == 0 && S_ISDIR(file_stat.st_mode)) {
        fprintf(stderr, "Invalid Command\n");
        exit(1);
    }

    // open the file at fullpath with write-only mode, create it if it does not exist,
    // and truncate it to zero length if it already exists.
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        fprintf(stderr, "Invalid Command\n");
        exit(1);
    }

    unsigned int content_len = 0;
    unsigned int count = 0;
    ssize_t bytes_read;

    // read and write the content using local buffer
    do {
        count = 0;

        do {
            bytes_read = read(STDIN_FILENO, content + count, sizeof(content) - count);
            if (bytes_read == -1) {
                perror("Error reading content");
                exit(1);
            }

            count += bytes_read;

        } while (bytes_read > 0 && count < sizeof(content));

        if (count)
            write(fd, content, count);

        content_len += count;

    } while (bytes_read > 0);

    close(fd);

    fd = open(filename, O_RDWR, 0);
    if (fd == -1) {
        fprintf(stderr, "Invalid Command\n");
        exit(1);
    }

    // trim stdin EOF (^D + \n)
    if (content_len >= 3) {
        char a, b, c;

        if (lseek(fd, -3, SEEK_END) == -1) {
            perror("Error setting content");
            exit(1);
        }

        if (read(fd, &a, 1) <= 0) {
            perror("Error reading content 2");
            exit(1);
        }

        if (read(fd, &b, 1) <= 0) {
            perror("Error reading content");
            exit(1);
        }

        if (read(fd, &c, 1) <= 0) {
            perror("Error reading content");
            exit(1);
        }

        if (a == '^' && b == 'D' && c == '\n')
            ftruncate(fd, content_len - 3);
    }

    close(fd);

    printf("OK\n");
}

int main() {
    char command[PATH_MAX], filename[PATH_MAX], remaining[PATH_MAX];

    // Reads a line of input from the standard input and stores it in the command array.
    ssize_t bytes_read;
    unsigned int count = 0;

    do {
        bytes_read = read(STDIN_FILENO, command + count, 1);
        if (bytes_read == -1) {
            perror("Error reading command");

            return 1;
        }

        count += bytes_read;
    } while (bytes_read > 0 && command[count - 1] != '\n');

    // Ensure the read data is null-terminated
    command[count] = '\0';

    // Check if the command has a newline at the end
    if (command[count - 1] != '\n') {
        fprintf(stderr, "Invalid Command\n");

        return 1;
    }

    // Remove any trailing newline character from the command
    command[strcspn(command, "\n")] = '\0';

    // Parse the input from the user and determine whether to call the get_file or set_file function.
    int sscanf_result = sscanf(command, "get %s %[^\n]", filename, remaining);
    if (sscanf_result == 1) {
        get_file(filename);

        return 0;
    } else if (sscanf_result == 2) {
        fprintf(stderr, "Invalid Command\n");

        return 1;
    } else if (sscanf(command, "set %s", filename) == 1) {
        set_file(filename);

        return 0;
    } else if (sscanf(command, "set %s %[^\n]", filename, remaining) == 2) {
        fprintf(stderr, "Invalid Command\n");

        return 1;
    } else {
        fprintf(stderr, "Invalid Command\n");

        return 1;
    }
}
