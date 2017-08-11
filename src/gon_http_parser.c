#include <err.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgenc/genc_nstrncasecmp.h>
#include <libgenc/genc_nStrToInt.h>
#include "gon_http_parser.h"

#define GON_HTTP_PARSER_ERROR                          \
    warnx("%s: %u: Parser error", __FILE__, __LINE__); \
    return -1;

char* gon_http_parser_getBufferPosition(struct gon_http_parser* parser) {
    return parser->buffer + parser->bufferOffset;
}

size_t gon_http_parser_getAvailableBufferSize(struct gon_http_parser* parser) {
    return parser->headerBufferCapacity - parser->bufferSize;
}

static inline void gon_http_parser_prepareForNextToken(struct gon_http_parser* parser) {
    parser->tokenOffset = 0;                                                                                                                                                     
    parser->token = parser->buffer + parser->bufferOffset;                                                                                                                       
}

static inline void gon_http_parser_compactBuffer(struct gon_http_parser* parser) {
    memmove(parser->buffer, parser->token, parser->tokenOffset * sizeof(char));
    parser->token = parser->buffer;
    parser->bufferOffset = parser->tokenOffset; 
    parser->bufferSize = parser->tokenOffset;
}

static inline bool gon_http_parser_canProceed(struct gon_http_parser* parser) {
    return parser->bufferOffset < parser->bufferSize;
}

int gon_http_parser_parse(struct gon_http_parser* parser, ssize_t readSize, void* args[]) {
    parser->bufferSize += readSize;
    while(gon_http_parser_canProceed(parser)) {
        switch(parser->state) {
        case GON_HTTP_PARSER_HEADERS_BEGIN:
            parser->callbacks->onRequestStart(args);
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
            if(parser->callbacks->onRequestMethod(parser->token, parser->tokenOffset, args) == -1) {
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
            if(parser->callbacks->onRequestUri(parser->token, parser->tokenOffset, args) == -1) {
                GON_HTTP_PARSER_ERROR;
            } else {
                gon_http_parser_prepareForNextToken(parser);
                parser->state = GON_HTTP_PARSER_HTTP_VERSION_HTTP;
            }
            break;
	case GON_HTTP_PARSER_HTTP_VERSION_HTTP:
	    if(parser->buffer[parser->bufferOffset] == '/') {
                ++parser->bufferOffset;
		gon_http_parser_prepareForNextToken(parser);
		parser->state = GON_HTTP_PARSER_HTTP_VERSION_MAJOR;
	    } else {
                ++parser->bufferOffset;
	    }
	    break;
        case GON_HTTP_PARSER_HTTP_VERSION_MAJOR:
            if(parser->buffer[parser->bufferOffset] == '.') {
                ++parser->bufferOffset;
		if(parser->tokenOffset == 0) {
		    GON_HTTP_PARSER_ERROR;
		} else if(parser->callbacks->onRequestVersionMajor(genc_nStrToInt(parser->token, parser->tokenOffset), args) == -1) {
                    GON_HTTP_PARSER_ERROR;
		} else {
		    gon_http_parser_prepareForNextToken(parser);
		    parser->state = GON_HTTP_PARSER_HTTP_VERSION_MINOR;
		}
            } else if('0' <= parser->buffer[parser->bufferOffset] && parser->buffer[parser->bufferOffset] <= '9') {
                ++parser->tokenOffset;
                ++parser->bufferOffset;
            } else {
                GON_HTTP_PARSER_ERROR;
	    }
            break;
	case GON_HTTP_PARSER_HTTP_VERSION_MINOR:
	    if(parser->buffer[parser->bufferOffset] == '\r') {
                ++parser->bufferOffset;
		parser->state = GON_HTTP_PARSER_HTTP_VERSION_END;
	    } else if('0' <= parser->buffer[parser->bufferOffset] && parser->buffer[parser->bufferOffset] <= '9') {
                ++parser->tokenOffset;
                ++parser->bufferOffset;
            } else {
                GON_HTTP_PARSER_ERROR;
	    }
	    break;
        case GON_HTTP_PARSER_HTTP_VERSION_END:
            if(parser->buffer[parser->bufferOffset] == '\n') {
                ++parser->bufferOffset;
		if(parser->tokenOffset == 0) {
                    GON_HTTP_PARSER_ERROR;
		} else if(parser->callbacks->onRequestVersionMinor(genc_nStrToInt(parser->token, parser->tokenOffset), args) == -1) {
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
                if(genc_nstrncasecmp(parser->token, parser->tokenOffset, "X-Script-Name", sizeof("X-Script-Name") - 1) == 0) {
                    gon_http_parser_prepareForNextToken(parser);
                    parser->state = GON_HTTP_PARSER_SCRIPT_PATH;            
                } else if(genc_nstrncasecmp(parser->token, parser->tokenOffset, "Content-Type", sizeof("Content-Type") - 1) == 0) {
                    gon_http_parser_prepareForNextToken(parser);
                    parser->state = GON_HTTP_PARSER_CONTENT_TYPE;            
                } else if(genc_nstrncasecmp(parser->token, parser->tokenOffset, "Content-Length", sizeof("Content-Length") - 1) == 0) {
                    gon_http_parser_prepareForNextToken(parser);
                    parser->state = GON_HTTP_PARSER_CONTENT_LENGTH;            
                } else if(parser->callbacks->onRequestHeaderField(parser->token, parser->tokenOffset, args) == -1) {
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
                if(parser->callbacks->onRequestScriptPath(parser->token, parser->tokenOffset, args) == -1) {
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
                if(parser->callbacks->onRequestContentType(parser->token, parser->tokenOffset, args) == -1) {
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
                if(parser->callbacks->onRequestContentLength(parser->token, parser->tokenOffset, args) == -1) {
                    GON_HTTP_PARSER_ERROR;
                }
                if((parser->bodyRemainder = genc_nStrToInt(parser->token, parser->tokenOffset)) == -1) {
                    GON_HTTP_PARSER_ERROR;
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
                if(parser->callbacks->onRequestHeaderValue(parser->token, parser->tokenOffset, args) == -1) {
                    GON_HTTP_PARSER_ERROR;
                }
                parser->state = GON_HTTP_PARSER_HEADER_FIELD_BEGIN;
            }
            break;
        case GON_HTTP_PARSER_HEADERS_END:
            if(parser->buffer[parser->bufferOffset] == '\n') {
                ++parser->bufferOffset;
                if(parser->callbacks->onRequestHeadersFinish(args) == -1) {
                    GON_HTTP_PARSER_ERROR;
                }
                if(parser->bodyRemainder == 0) {
                    parser->bufferSize -= parser->bufferOffset;
                    return 0;
                }
                parser->token = parser->buffer + parser->bufferOffset;
                parser->tokenOffset = parser->bufferSize - parser->bufferOffset;
                gon_http_parser_compactBuffer(parser);
                parser->buffer = realloc(parser->buffer, parser->bodyBufferCapacity * sizeof(char));
                parser->bufferOffset = 0;
                parser->tokenOffset = 0;
                if(parser->callbacks->onRequestBodyStart(args) == -1) {
                    GON_HTTP_PARSER_ERROR;
                }
                parser->state = GON_HTTP_PARSER_BODY;
            } else {
                GON_HTTP_PARSER_ERROR;
            }
            break;
        case GON_HTTP_PARSER_BODY:
            if(parser->bufferSize < parser->bodyRemainder) {
                if(parser->callbacks->onRequestBody(parser->buffer, parser->bufferSize, args) == -1) {
                    GON_HTTP_PARSER_ERROR;
                }
                parser->bufferOffset = parser->bufferSize;
                parser->bodyRemainder -= parser->bufferOffset;
                gon_http_parser_prepareForNextToken(parser);
            } else {
                if(parser->callbacks->onRequestBody(parser->buffer, parser->bodyRemainder, args) == -1) {
                    GON_HTTP_PARSER_ERROR;
                }
                parser->bufferOffset = parser->bodyRemainder;
                parser->bodyRemainder -= parser->bufferOffset;
                gon_http_parser_prepareForNextToken(parser);
                if(parser->callbacks->onRequestBodyFinish(args) == -1) {
                    GON_HTTP_PARSER_ERROR;
                }
                return 0;
            }
            break;
        default:
            GON_HTTP_PARSER_ERROR;
        }
    }
    if(parser->headerBufferCapacity - parser->tokenOffset == 0) {
        warnx("%s: %u: A header is bigger than the header buffer", __FILE__, __LINE__);
        GON_HTTP_PARSER_ERROR;
    }
    gon_http_parser_compactBuffer(parser);
    return 1;
}
