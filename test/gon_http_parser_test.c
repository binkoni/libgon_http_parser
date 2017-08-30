#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
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
    int inputFd = open("input0.text", O_RDONLY);
    assert_int_not_equal(inputFd, -1);
    int readSize;
    while((readSize = read(inputFd, gon_http_parser_getBufferPosition(parser), gon_http_parser_getAvailableBufferSize(parser))) > 0) {
        int result = gon_http_parser_parse(parser, readSize, (void*[]){NULL});
        assert_int_not_equal(result, -1);
        if(result == 0)
            break;
    }
    close(inputFd);
    free(callbacks);
    free(parser);
}

int main() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(gon_http_parser_test)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}

/*
    while((readSize = TUCUBE_LOCAL_CLIENT_IO->methods->read(
              TUCUBE_LOCAL_CLIENT_IO,
              gon_http_parser_getBufferPosition(TUCUBE_LOCAL_PARSER),
              gon_http_parser_getAvailableBufferSize(TUCUBE_LOCAL_PARSER)
           )) > 0) {
        int result;
        if((result = gon_http_parser_parse(TUCUBE_LOCAL_PARSER, readSize, (void*[]){GENC_LIST_ELEMENT_NEXT(module), GENC_LIST_ELEMENT_NEXT(clData)})) == -1) {
            warnx("%s: %u: Parser error", __FILE__, __LINE__);
            return -1;
        } else if(result == 0)
            break;
    }
    if(readSize == -1) {
        if(errno == EAGAIN) {
//            warnx("%s: %u: Client socket EAGAIN", __FILE__, __LINE__);
            return 1;
        } else if(errno == EWOULDBLOCK) {
            warnx("%s: %u: Client socket EWOULDBLOCK", __FILE__, __LINE__);
            return 1;
        } else
            warn("%s: %u", __FILE__, __LINE__);
        return -1;
    }
    else if(readSize == 0) {
        warnx("%s: %u: Client socket has been closed", __FILE__, __LINE__);
        return -1;
    }
    
    const char* connectionHeaderValue;
    if(TUCUBE_LOCAL_MODULE->onGetRequestStringHeader(
              GENC_LIST_ELEMENT_NEXT(module), GENC_LIST_ELEMENT_NEXT(clData), "Connection", &connectionHeaderValue
      ) != -1) {
        if(strncasecmp(connectionHeaderValue, "Keep-Alive", sizeof("Keep-Alive")) == 0) {
            warnx("%s: %u: Keep-Alive Connection", __FILE__, __LINE__);
            TUCUBE_LOCAL_CLDATA->isKeepAlive = false;
            gon_http_parser_reset(TUCUBE_LOCAL_PARSER);
            return 2; // request finished but this is keep-alive
        }
        TUCUBE_LOCAL_CLDATA->isKeepAlive = false;
    }
*/
