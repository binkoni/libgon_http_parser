#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include "../src/gon_http_parser.h"
#include <err.h>
#include <stdio.h>

static int test_onRequestStart(void* args[]) {
}

static int test_onRequestMethod(char* token, ssize_t tokenSize, void* args[]) {
}

static int test_onRequestUri(char* token, ssize_t tokenSize, void* args[]) {
}

static int test_onRequestVersionMajor(int major, void* args[]) {
}

static int test_onRequestVersionMinor(int minor, void* args[]) {
}

static int test_onRequestScriptPath(char* token, ssize_t tokenSize, void* args[]) {
}

static int test_onRequestContentType(char* token, ssize_t tokenSize, void* args[]) {
}

static int test_onRequestContentLength(char* token, ssize_t tokenSize, void* args[]) {
}

static int test_onRequestHeaderField(char* token, ssize_t tokenSize, void* args[]) {
}

static int test_onRequestHeaderValue(char* token, ssize_t tokenSize, void* args[]) {
}

static int test_onRequestHeadersFinish(void* args[]) {
}

static int test_onRequestBodyStart(void* args[]) {
}

static int test_onRequestBody(char* token, ssize_t tokenSize, void* args[]) {
}

static int test_onRequestBodyFinish(void* args[]) {
}

void gon_http_parser_test(void** state) {
    struct gon_http_parser_callbacks* callbacks = malloc(sizeof(struct gon_http_parser_callbacks));
    GON_HTTP_PARSER_CALLBACKS_INIT(callbacks, test_);
    struct gon_http_parser* parser = malloc(sizeof(struct gon_http_parser));
    gon_http_parser_init(parser, 1024 * 1024, 1024 * 1024);
    parser->callbacks = callbacks;

    DIR* directory = opendir(".");
    struct dirent* dirent;
    while((dirent = readdir(directory)) != NULL) {
        if(strncmp("input", dirent->d_name, sizeof("input") - 1) == 0) {
            int inputFd = open(dirent->d_name, O_RDONLY);
            assert_int_not_equal(inputFd, -1);
            int readSize;
            while((readSize = read(inputFd, gon_http_parser_getBufferPosition(parser), gon_http_parser_getAvailableBufferSize(parser))) > 0) {
                int result = gon_http_parser_parse(parser, readSize, (void*[]){NULL});
                assert_int_not_equal(result, -1);
                if(result == 0)
                    break;
            }
            close(inputFd);
            gon_http_parser_reset(parser);
        }
    }
    closedir(directory);
    free(callbacks);
    free(parser);
}

int main() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(gon_http_parser_test)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}
