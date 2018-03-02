#include "../src/vega.h"
#include <string.h>
void hello(struct http_context *http_ctx){
        strcpy(http_ctx->response_headers,"Access-Control-Allow-Origin: *\0");
        strcpy(http_ctx->response_body,"Hello World\0");
        strcpy(http_ctx->response_content_type,"text/html\0");

}

void defaultPage(struct http_context *http_ctx) {
  strcpy(http_ctx->response_body,"<!DOCTYPE html><html><head><meta charset=utf-8><title>Vega-http</title><style></style></head><body id=preview><h1><a id=Vega_HTTP_0></a>Vega HTTP</h1><h3><a id=This_is_the_default_page_2></a>This is the default page.</h3><p>Please change the routes configuration from the conf/vega-routes.conf file in your installation Directory.</p></body></html>  \0");
  strcpy(http_ctx->response_content_type,"text/html\0");
}
