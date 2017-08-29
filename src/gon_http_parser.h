#ifndef _GON_HTTP_PARSER_H
#define _GON_HTTP_PARSER_H

#include <limits.h>
#include <stdbool.h>

/**
 * @file gon_http_parser.h
 * @author Byeonggon Lee
 * @brief This file contains functions and structs for libgon_http_parser
 */

enum gon_http_parser_state {
    GON_HTTP_PARSER_HEADERS_BEGIN,
    GON_HTTP_PARSER_METHOD,
    GON_HTTP_PARSER_METHOD_END,
    GON_HTTP_PARSER_REQUEST_URI,
    GON_HTTP_PARSER_REQUEST_URI_END,
    GON_HTTP_PARSER_HTTP_VERSION_HTTP,
    GON_HTTP_PARSER_HTTP_VERSION_MAJOR,
    GON_HTTP_PARSER_HTTP_VERSION_MINOR,
    GON_HTTP_PARSER_HTTP_VERSION_END,
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

/**
 * @brief
 * HTTP parser struct.
 */

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
    struct gon_http_parser_callbacks* callbacks;
};

/**
 * @brief
 * Parser callbacks struct.
 */

struct gon_http_parser_callbacks {
    int (*onRequestStart)(void* args[]);
    int (*onRequestMethod)(char*, ssize_t, void* args[]);
    int (*onRequestUri)(char*, ssize_t, void* args[]);
    int (*onRequestVersionMajor)(int, void* args[]);
    int (*onRequestVersionMinor)(int, void* args[]);
    int (*onRequestScriptPath)(char*, ssize_t, void* args[]);
    int (*onRequestContentType)(char*, ssize_t, void* args[]);
    int (*onRequestContentLength)(char*, ssize_t, void* args[]);
    int (*onRequestHeaderField)(char*, ssize_t, void* args[]);
    int (*onRequestHeaderValue)(char*, ssize_t, void* args[]);
    int (*onRequestHeadersFinish)(void* args[]);
    int (*onRequestBodyStart)(void* args[]);
    int (*onRequestBody)(char*, ssize_t, void* args[]);
    int (*onRequestBodyFinish)(void* args[]);
};

/**
 * @brief
 * Macro that helps initializing the parser callbacks struct.
 * @param callbacks
 * Pointer of struct gon_http_parser_callbacks.
 * @param prefix
 * Common prefix of callback function names.
 */

#define GON_HTTP_PARSER_CALLBACKS_INIT(callbacks, prefix)             \
(callbacks)->onRequestStart = prefix##onRequestStart;                 \
(callbacks)->onRequestMethod = prefix##onRequestMethod;               \
(callbacks)->onRequestUri = prefix##onRequestUri;                     \
(callbacks)->onRequestVersionMajor = prefix##onRequestVersionMajor;   \
(callbacks)->onRequestVersionMinor = prefix##onRequestVersionMinor;   \
(callbacks)->onRequestScriptPath = prefix##onRequestScriptPath;       \
(callbacks)->onRequestContentType = prefix##onRequestContentType;     \
(callbacks)->onRequestContentLength = prefix##onRequestContentLength; \
(callbacks)->onRequestHeaderField = prefix##onRequestHeaderField;     \
(callbacks)->onRequestHeaderValue = prefix##onRequestHeaderValue;     \
(callbacks)->onRequestHeadersFinish = prefix##onRequestHeadersFinish; \
(callbacks)->onRequestBodyStart = prefix##onRequestBodyStart;         \
(callbacks)->onRequestBody = prefix##onRequestBody;                   \
(callbacks)->onRequestBodyFinish = prefix##onRequestBodyFinsih;       \

/**
 * @brief
 * Initialize HTTP parser struct.
 * @param parser
 * Pointer of HTTP parser.
 * @param headerBufferCapacity
 * Capacity of HTTP header buffer.
 * @param bodyBufferCapacity
 * Capacity of HTTP body buffer.
 * @returns
 * 0: Initialization was successful
 * 1: Initialization was not successful
 */


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

/**
 * @brief
 * Reset HTTP Parser struct.
 * @details
 * Reset parser state to the start state, reset all internal offsets.
 * Useful when HTTP connection is keep-alive.
 * @param parser
 * Pointer of HTTP parser.
 * @returns
 * -1: Reset was not successful
 * 0: Reset was successful
 */

static inline int gon_http_parser_reset(struct gon_http_parser* parser) {
    parser->state = GON_HTTP_PARSER_HEADERS_BEGIN;
    parser->buffer = realloc(parser->buffer, parser->headerBufferCapacity * sizeof(char)); 
    parser->bufferOffset = 0;
    parser->token = parser->buffer;
    parser->tokenOffset = 0; 
    parser->bodyRemainder = 0;
    return 0;
}

/**
 * @brief
 * Get current buffer position.
 * @details
 * Use this function like this:
 * @code
 * read(fd, gon_http_parser_getBufferPosition(parser), gon_http_parser_getAvailableBufferSize(parser));
 * @endcode
 * @param parser
 * Pointer of HTTP parser.
 * @returns
 * Pointer of current buffer position.
 */

char* gon_http_parser_getBufferPosition(struct gon_http_parser* parser);

/**
 * @brief
 * Get currently available buffer size.
 * @details
 * Use this function like this:
 * @code
 * read(fd, gon_http_parser_getBufferPosition(parser), gon_http_parser_getAvailableBufferSize(parser));
 * @endcode
 * @param parser
 * Pointer of HTTP parser.
 * @returns
 * Available buffer size.
 */

size_t gon_http_parser_getAvailableBufferSize(struct gon_http_parser* parser);

/**
 * @brief
 * Parse the data in the buffer.
 * @param readSize
 * Size of last read data.
 * @param args
 * Any type of data that you want to pass to every callback functions.
 * @returns
 * -1: Failed to parse the data.
 *  0: Parsing is finished.
 *  1: Needs more data to finish parsing.
 */

int gon_http_parser_parse(struct gon_http_parser* parser, ssize_t readSize, void* args[]);

#endif
