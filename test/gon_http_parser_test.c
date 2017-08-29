static int onRequestStart(void* args[]) {
}

static int onRequestMethod(char* token, ssize_t tokenSize, void* args[]) {
}

static int onRequestUri(char* token, ssize_t tokenSize, void* args[]) {
}

static int onRequestVersionMajor(int major, void* args[]) {
}

static int onRequestVersionMinor(int minor, void* args[]) {
}

static int onRequestScriptPath(char* token, ssize_t tokenSize, void* args[]) {
}

static int onRequestContentType(char* token, ssize_t tokenSize, void* args[]) {
}

static int onRequestContentLength(char* token, ssize_t tokenSize, void* args[]) {
}

static int onRequestHeaderField(char* token, ssize_t tokenSize, void* args[]) {
}

static int onRequestHeaderValue(char* token, ssize_t tokenSize, void* args[]) {
}

static int onRequestHeadersFinish(void* args[]) {
}

static int onRequestBodyStart(void* args[]) {
}

static int onRequestBody(char* token, ssize_t tokenSize, void* args[]) {
}

static int onRequestBodyFinish(void* args[]) {
}

void gon_http_parser_test(void** state) {
    struct gon_http_parser_callbacks* callbacks = malloc(sizeof(struct gon_http_parser_callbacks));
    GON_HTTP_PARSER_CALLBACKS_INIT(callbacks, "");
    struct gon_http_parser* parser = malloc(sizeof(struct gon_http_parser));
    gon_http_parser_init(parser, 1024 * 1024, 1024 * 1024);
    parser->callbacks = callbacks;
    free(callbacks);
    free(parser);
}

int main() {
    const struct CMUnitTest tests[] = {cmocka_unit_test(gon_http_parser_test)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}

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

