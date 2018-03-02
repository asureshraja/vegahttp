extern "C" void hello(struct http_context *http_ctx);

#include "../src/vega.h"
#include <string.h>

void hello(struct http_context *http_ctx){
        strcpy(http_ctx->response_headers,"Access-Control-Allow-Origin: *\0");
        strcpy(http_ctx->response_body,"Hello World\0");
        strcpy(http_ctx->response_content_type,"text/html\0");
}
