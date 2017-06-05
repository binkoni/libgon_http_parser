#ifndef _GON_HTTP_PARSER_H
#define _GON_HTTP_PARSER_H

#include <limits.h>
#include <stdbool.h>

enum gon_http_parser_state {
    GON_HTTP_PARSER_HEADERS_BEGIN,
    GON_HTTP_PARSER_METHOD,
    GON_HTTP_PARSER_METHOD_END,
    GON_HTTP_PARSER_REQUEST_URI,
    GON_HTTP_PARSER_REQUEST_URI_END,
    GON_HTTP_PARSER_PROTOCOL,
    GON_HTTP_PARSER_PROTOCOL_END,
    GON_HTTP_PARSER_SCRIPT_PATH,
    GON_HTTP_PARSER_SCRIPT_PATH_END,
    GON_HTTP_PARSER_CONTENT_TYPE,
    GON_HTTP_PARSER_CONTENT_TYPE_END,
    GON_HTTP_PARSER_CONTENT_LENGTH,
    GON_HTTP_PARSER_CONTENT_LENGTH_END,
    GON_HTTP_PARSER_HEADER_FIELD_BEGIN,
    GON_HTTP_PARSER_HEADER_FIELD,
    GON_HTTP_PARSER_HEADER_FIELD_END,
    GON_HTTP_PARSER_HEADER_VALUE_BEGIN,
    GON_HTTP_PARSER_HEADER_VALUE,
    GON_HTTP_PARSER_HEADER_VALUE_END,
    GON_HTTP_PARSER_HEADERS_END,
    GON_HTTP_PARSER_BODY,
};

struct gon_http_parser {
    enum gon_http_parser_state state;
    size_t headerBufferCapacity;
    size_t bodyBufferCapacity;
    char* buffer;
    size_t bufferOffset;
    size_t bufferSize;
    char* token;
    size_t tokenOffset;
    ssize_t bodyRemainder;

    int (*onRequestStart)(void* args[]);
    int (*onRequestMethod)(char*, ssize_t, void* args[]);
    int (*onRequestUri)(char*, ssize_t, void* args[]);
    int (*onRequestProtocol)(char*, ssize_t, void* args[]);
    int (*onRequestScriptPath)(char*, ssize_t, void* args[]);
    int (*onRequestContentType)(char*, ssize_t, void* args[]);
    int (*onRequestContentLength)(char*, ssize_t, void* args[]);

    int (*onRequestHeaderField)(char*, ssize_t, void* args[]);
    int (*onRequestHeaderValue)(char*, ssize_t, void* args[]);
    int (*onRequestHeadersFinish)(void* args[]);
    int (*onRequestBodyStart)(void* args[]);
    int (*onRequestBody)(char*, ssize_t, void* args[]);
    int (*onRequestBodyFinish)(void* args[]);
    int (*onRequestFinish)(void* args[]);
};

static inline int gon_http_parser_init(struct gon_http_parser* parser, size_t headerBufferCapacity, size_t bodyBufferCapacity) {
    parser->state = GON_HTTP_PARSER_HEADERS_BEGIN;
    parser->headerBufferCapacity = headerBufferCapacity;
    parser->bodyBufferCapacity = bodyBufferCapacity;
    parser->buffer = malloc(1 * headerBufferCapacity * sizeof(char)); 
    parser->bufferOffset = 0;
    parser->bufferSize = 0;
    parser->token = parser->buffer;
    parser->tokenOffset = 0; 
    parser->bodyRemainder = 0;
    return 0;
}

static inline int gon_http_parser_reset(struct gon_http_parser* parser) {
    parser->state = GON_HTTP_PARSER_HEADERS_BEGIN;
    parser->buffer = realloc(parser->buffer, parser->headerBufferCapacity * sizeof(char)); 
    parser->bufferOffset = 0;
    parser->token = parser->buffer;
    parser->tokenOffset = 0; 
    parser->bodyRemainder = 0;
    return 0;
}

int gon_http_parser_parse(struct gon_http_parser* parser, ssize_t readSize, void* args[]);

#endif
