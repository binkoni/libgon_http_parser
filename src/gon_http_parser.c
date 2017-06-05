#include <err.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgon_c/gon_c_nstrncasecmp.h>
#include <libgon_c/gon_c_nstrtoi.h>
#include "gon_http_parser.h"

#define GON_HTTP_PARSER_ERROR                          \
    warnx("%s: %u: Parser error", __FILE__, __LINE__); \
    return -1;

static inline char* gon_http_parser_getBufferPosition(struct gon_http_parser* parser) {
    if(parser->state < GON_HTTP_PARSER_BODY_BEGIN)
        return parser->buffer + parser->tokenOffset;
    return parser->buffer;
}

static inline size_t gon_http_parser_getAvailableBufferSize(struct gon_http_parser* parser) {
    if(parser->state < GON_HTTP_PARSER_BODY_BEGIN)
        return parser->headerBufferCapacity - parser->tokenOffset;
    return parser->bodyRemainder;
}

int gon_http_parser_read(struct gon_http_parser* parser, int* clientSocket) {
    return read(
        *clientSocket,
        gon_http_parser_getBufferPosition(parser),
        gon_http_parser_getAvailableBufferSize(parser)
    );
}

static inline void gon_http_parser_prepareForNextToken(struct gon_http_parser* parser) {
    parser->tokenOffset = 0;                                                                                                                                                     
    parser->token = parser->buffer + parser->bufferOffset;                                                                                                                       
}

static inline void gon_http_parser_compactBuffer(struct gon_http_parser* parser) {
    memmove(parser->buffer, parser->token, parser->tokenOffset * sizeof(char));
    parser->bufferOffset = parser->tokenOffset;
    parser->token = parser->buffer;
    parser->bufferSize = parser->tokenOffset;
}

static inline bool gon_http_parser_canProceed(struct gon_http_parser* parser) {
   if(parser->state < GON_HTTP_PARSER_BODY_BEGIN)
        return parser->bufferOffset < parser->bufferSize;
   return parser->bodyRemainder > 0 && parser->bufferSize > 0;
}

int gon_http_parser_parse(struct gon_http_parser* parser, void* args[]) {
    parser->bufferSize += readSize;
    while(parser->canProceed(parser)) {
        switch(parser->state) {
        case GON_HTTP_PARSER_HEADERS_BEGIN:
            parser->onRequestStart(args);
            gon_http_parser_prepareForNextToken(parser);
            parser->state = GON_HTTP_PARSER_METHOD;
            break;
        case GON_HTTP_PARSER_METHOD:
            if(parser->buffer[parser->bufferOffset] == ' ') {
                ++parser->bufferOffset;
                parser->state = GON_HTTP_PARSER_METHOD_END;
            } else if(parser->buffer[parser->bufferOffset] < 'A' || parser->buffer[parser->bufferOffset] > 'z') {
                GON_HTTP_PARSER_ERROR;
            } else {
                ++parser->tokenOffset;
                ++parser->bufferOffset;
            }
            break;
        case GON_HTTP_PARSER_METHOD_END:
            if(parser->onRequestMethod(parser->token, parser->tokenOffset, args) == -1) {
                GON_HTTP_PARSER_ERROR;
            } else {
                gon_http_parser_prepareForNextToken(parser);
                parser->state = GON_HTTP_PARSER_REQUEST_URI;
            }
            break;
        case GON_HTTP_PARSER_REQUEST_URI:
            if(parser->buffer[parser->bufferOffset] == ' ') {
                ++parser->bufferOffset;
                parser->state = GON_HTTP_PARSER_REQUEST_URI_END;
            } else {
                ++parser->tokenOffset;
                ++parser->bufferOffset;
            }
            break;
        case GON_HTTP_PARSER_REQUEST_URI_END:
            if(parser->onRequestUri(parser->token, parser->tokenOffset, args) == -1) {
                GON_HTTP_PARSER_ERROR;
            } else {
                gon_http_parser_prepareForNextToken(parser);
                parser->state = GON_HTTP_PARSER_PROTOCOL;
            }
            break;
        case GON_HTTP_PARSER_PROTOCOL:
            if(parser->buffer[parser->bufferOffset] == '\r') {
                ++parser->bufferOffset;
                parser->state = GON_HTTP_PARSER_PROTOCOL_END;
            } else {
                ++parser->tokenOffset;
                ++parser->bufferOffset;
            }
            break;
        case GON_HTTP_PARSER_PROTOCOL_END:
            if(parser->buffer[parser->bufferOffset] == '\n') {
                ++parser->bufferOffset;
                if(parser->onRequestProtocol(parser->token, parser->tokenOffset, args) == -1) {
                    GON_HTTP_PARSER_ERROR;
                } else
                    parser->state = GON_HTTP_PARSER_HEADER_FIELD_BEGIN;
            } else {
                GON_HTTP_PARSER_ERROR;
            }
            break;
        case GON_HTTP_PARSER_HEADER_FIELD_BEGIN:
            if(parser->buffer[parser->bufferOffset] == '\r') {
                ++parser->bufferOffset;
                parser->state = GON_HTTP_PARSER_HEADERS_END;
            } else {
                gon_http_parser_prepareForNextToken(parser);
                parser->state = GON_HTTP_PARSER_HEADER_FIELD;
            }
            break;
        case GON_HTTP_PARSER_HEADER_FIELD:
            if(parser->buffer[parser->bufferOffset] == ':') {
                ++parser->bufferOffset;
                parser->state = GON_HTTP_PARSER_HEADER_FIELD_END;
            } else {
                ++parser->tokenOffset;
                ++parser->bufferOffset;
            }
            break;
        case GON_HTTP_PARSER_HEADER_FIELD_END:
            if(parser->buffer[parser->bufferOffset] == ' ')
                ++parser->bufferOffset;
            else {
                if(gon_c_nstrncasecmp(parser->token, parser->tokenOffset, "X-Script-Name", sizeof("X-Script-Name") - 1) == 0) {
                    gon_http_parser_prepareForNextToken(parser);
                    parser->state = GON_HTTP_PARSER_SCRIPT_PATH;            
                } else if(gon_c_nstrncasecmp(parser->token, parser->tokenOffset, "Content-Type", sizeof("Content-Type") - 1) == 0) {
                    gon_http_parser_prepareForNextToken(parser);
                    parser->state = GON_HTTP_PARSER_CONTENT_TYPE;            
                } else if(gon_c_nstrncasecmp(parser->token, parser->tokenOffset, "Content-Length", sizeof("Content-Length") - 1) == 0) {
                    gon_http_parser_prepareForNextToken(parser);
                    parser->state = GON_HTTP_PARSER_CONTENT_LENGTH;            
                } else if(parser->onRequestHeaderField(parser->token, parser->tokenOffset, args) == -1) {
                    GON_HTTP_PARSER_ERROR;
                } else {
                    gon_http_parser_prepareForNextToken(parser);
                    parser->state = GON_HTTP_PARSER_HEADER_VALUE;
                }
            }
            break;
        case GON_HTTP_PARSER_SCRIPT_PATH:
            if(parser->buffer[parser->bufferOffset] == '\r') {
                ++parser->bufferOffset;
                parser->state = GON_HTTP_PARSER_SCRIPT_PATH_END;
            } else {
                ++parser->tokenOffset;
                ++parser->bufferOffset;
            }
            break;
        case GON_HTTP_PARSER_SCRIPT_PATH_END:
            if(parser->buffer[parser->bufferOffset] == '\n') {
                ++parser->bufferOffset;
                if(parser->onRequestScriptPath(parser->token, parser->tokenOffset, args) == -1) {
                    GON_HTTP_PARSER_ERROR;
                }
                parser->state = GON_HTTP_PARSER_HEADER_FIELD_BEGIN;
            }
            break;
        case GON_HTTP_PARSER_CONTENT_TYPE:
            if(parser->buffer[parser->bufferOffset] == '\r') {
                ++parser->bufferOffset;
                parser->state = GON_HTTP_PARSER_CONTENT_TYPE_END;
            } else {
                ++parser->tokenOffset;
                ++parser->bufferOffset;
            }
            break;
        case GON_HTTP_PARSER_CONTENT_TYPE_END:
            if(parser->buffer[parser->bufferOffset] == '\n') {
                ++parser->bufferOffset;
                if(parser->onRequestContentType(parser->token, parser->tokenOffset, args) == -1) {
                    GON_HTTP_PARSER_ERROR;
                }
                parser->state = GON_HTTP_PARSER_HEADER_FIELD_BEGIN;
            }
            break;
        case GON_HTTP_PARSER_CONTENT_LENGTH:
            if(parser->buffer[parser->bufferOffset] == '\r') {
                ++parser->bufferOffset;
                parser->state = GON_HTTP_PARSER_CONTENT_LENGTH_END;
            } else {
                ++parser->tokenOffset;
                ++parser->bufferOffset;
            }
            break;
        case GON_HTTP_PARSER_CONTENT_LENGTH_END:
            if(parser->buffer[parser->bufferOffset] == '\n') {
                ++parser->bufferOffset;
                if(parser->onRequestContentLength(parser->token, parser->tokenOffset, args) == -1) {
                    GON_HTTP_PARSER_ERROR;
                }
                if((parser->bodyRemainder = gon_c_nstrtoi(parser->token, parser->tokenOffset)) == -1) {
                    GON_HTTP_PARSER_ERROR;
                }
                if(parser->bodyRemainder == 0) {
                    if(parser->onRequestFinish(args) == -1) {
                        GON_HTTP_PARSER_ERROR;
                    }
                    parser->bufferSize -= parser->bufferOffset;
                    return 0;
                }
                if(parser->bodyBufferCapacity < parser->bodyRemainder) {
                    warnx("%s: %u: Request body is bigger than parser body buffer", __FILE__, __LINE__);
                    GON_HTTP_PARSER_ERROR;
                }
                parser->state = GON_HTTP_PARSER_HEADER_FIELD_BEGIN;
            }
            break;
        case GON_HTTP_PARSER_HEADER_VALUE:
            if(parser->buffer[parser->bufferOffset] == '\r') {
                ++parser->bufferOffset;
                parser->state = GON_HTTP_PARSER_HEADER_VALUE_END;
            } else {
                ++parser->tokenOffset;
                ++parser->bufferOffset;
            }
            break;
        case GON_HTTP_PARSER_HEADER_VALUE_END:
            if(parser->buffer[parser->bufferOffset] == '\n') {
                ++parser->bufferOffset;
                if(parser->onRequestHeaderValue(parser->token, parser->tokenOffset, args) == -1) {
                    GON_HTTP_PARSER_ERROR;
                }
                parser->state = GON_HTTP_PARSER_HEADER_FIELD_BEGIN;
            }
            break;
        case GON_HTTP_PARSER_HEADERS_END:
            if(parser->buffer[parser->bufferOffset] == '\n') {
                ++parser->bufferOffset;
                if(parser->onRequestHeadersFinish(args) == -1) {
                    GON_HTTP_PARSER_ERROR;
                }
                memmove(parser->buffer, parser->buffer + parser->bufferOffset, (parser->bufferSize - parser->bufferOffset) * sizeof(char));
                parser->buffer = realloc(parser->buffer, parser->bodyBufferCapacity * sizeof(char));
                parser->bufferSize -= parser->bufferOffset;
                parser->bufferOffset = 0;
                parser->token = parser->buffer;
                parser->tokenOffset = 0;
                if(parser->onRequestBodyStart(args) == -1) {
                    GON_HTTP_PARSER_ERROR;
                }
                parser->state = GON_HTTP_PARSER_BODY;
            } else {
                GON_HTTP_PARSER_ERROR;
            }
            break;
        case GON_HTTP_PARSER_BODY:
            if(parser->bufferSize < parser->bodyRemainder) {
                if(parser->onRequestBody(parser->buffer, parser->bufferSize, args) == -1) {
                    GON_HTTP_PARSER_ERROR;
                }
                parser->bodyRemainder -= parser->bufferSize;
                parser->bufferOffset += parser->bufferSize - 1;
                parser->bufferSize = 0;
            } else {
                if(parser->onRequestBody(parser->buffer, parser->bodyRemainder, args) == -1) {
                    GON_HTTP_PARSER_ERROR;
                }
                parser->bufferSize -= parser->bodyRemainder;
                parser->bufferOffset += parser->bodyRemainder - 1;
                parser->bodyRemainder = 0;
                parser->state = GON_HTTP_PARSER_BODY_END;
            }
        break;
        case GON_HTTP_PARSER_BODY_END:
            if(parser->onRequestBodyFinish(args) == -1) {
                GON_HTTP_PARSER_ERROR;
            }
            if(parser->onRequestFinish(args) == -1) {
                GON_HTTP_PARSER_ERROR;
            }
            return 0;
        default:
            GON_HTTP_PARSER_ERROR;
        }
    }

    if(parser->headerBufferCapacity - parser->tokenOffset == 0) {
        warnx("%s: %u: A token is bigger than http_buffer", __FILE__, __LINE__);
        GON_HTTP_PARSER_ERROR;
    }
    gon_http_parser_compactBuffer(parser);
    return 1;
}
