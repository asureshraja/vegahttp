struct http_context {
        struct http_request *req;
        char *response_buffer;
        char *response_body;
        char *response_headers;
        char *response_content_type;
        void* (*func_ptr)(struct http_context *);
};
