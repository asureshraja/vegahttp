#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <netinet/in.h>
#include <stdio.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/epoll.h>
#include "server.h"
#include <fcntl.h>
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <dlfcn.h>
#include <glib.h>

#define DEFAULT_BUFFER_SIZE 1024
#define RESPONSE_BUFFER_SIZE 1048576
#define MAX_KEY_PATH_SIZE 100
#define CONTENT_LENGTH_MAX_DIGIT_SIZE 12
#define MAX_NO_OF_HEADERS 10
#define MAX_ROUTES 1000
#define MAX_MESSAGE_PASSERS 100

int number_of_active_sockets = 0;
int server_socket;
int _eventfd[MAX_MESSAGE_PASSERS];
int epfd;
int client_socket;
char *notif;
int worker_epfd; //epoll for message passing between worker and network threads
int msg_passer[MAX_MESSAGE_PASSERS];
struct server_config *server_config;

struct trie *trie_for_NIO; //trie for default or in-thread mode routing
struct trie *trie_for_worker; //trie for worker-thread mode routing
struct trie *trie_for_files; //trie for file-serve mode routing
struct file_cache *files; //file cache with contents and meta data
void *inserted; //status variable for trie insertion
int z;

//trie pointer
void *(*func[MAX_ROUTES])(struct http_context *);

int load_file(char *file_path, char *file_body);

char *error_page_404_body;
char *error_page_404_content_type;

void send_response(struct http_context *http_ctx) {

        //this function always gets called after function controller execution
        char res_buf[RESPONSE_BUFFER_SIZE]; //buffer to hold response body with headers
        http_ctx->response_buffer = &res_buf[0];
        http_ctx->response_buffer[0] = '\0';

        struct http_request *req = http_ctx->req;
        char *response;
        response = http_ctx->response_buffer;
        response[0] = '\0';
        char *response_body = http_ctx->response_body;
        //response sending code

        //Prepares the response context text with proper headers
        if (req->keepalive == 1) {
                // Code Block if the Request is a keepalive request
                if (req->minor_version == 1) {
                        sprintf(response, "HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length:%d\n%s\n\n%s",
                                http_ctx->response_content_type, str_len(response_body), http_ctx->response_headers, response_body);
                        send(req->socket_id, response, str_len(response), MSG_DONTWAIT | MSG_NOSIGNAL);
                } else {

                        sprintf(response, "HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length:%d\n%s\n\n%s",
                                http_ctx->response_content_type, str_len(response_body), http_ctx->response_headers, response_body);
                        send(req->socket_id, response, str_len(response), MSG_DONTWAIT | MSG_NOSIGNAL);
                }

        }
        else {
                // Code Block if the Request is not a keepalive request
                if (req->minor_version == 1) {
                        sprintf(response, "HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length:%d\n%s\n\n%s",
                                http_ctx->response_content_type, str_len(response_body), http_ctx->response_headers, response_body);
                        send(req->socket_id, response, str_len(response), MSG_DONTWAIT | MSG_NOSIGNAL);

                } else {
                        sprintf(response, "HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length:%d\n%s\n\n%s",
                                http_ctx->response_content_type, str_len(response_body), http_ctx->response_headers, response_body);
                        send(req->socket_id, response, str_len(response), MSG_DONTWAIT | MSG_NOSIGNAL);
                }
                close(req->socket_id);
        }


        http_ctx->func_ptr = NULL;

        //freeing all allocated memory from the Http_context and the request object for the current request
        free(req);
        free(http_ctx->response_headers);
        free(http_ctx->response_content_type);
        free(response_body);
        free(http_ctx);

}

void send_file_response(struct http_context *http_ctx) {
        //Note: While file serving we are creating response context each time instead of caching the response context Directly.
        //This is to support multiple http versions in future.

        char res_buf[RESPONSE_BUFFER_SIZE]; //buffer to hold response body with headers
        http_ctx->response_buffer = &res_buf[0];
        http_ctx->response_buffer[0] = '\0';

        struct http_request *req = http_ctx->req;
        char *response;
        response = http_ctx->response_buffer;
        response[0] = '\0';

        char *response_body = http_ctx->response_body;


        int response_length = strlen(http_ctx->response_body);

        //Prepares the response context text with proper headers
        if (req->keepalive == 1) {
                // Code Block if the Request is a keepalive request
                if (req->minor_version == 1) {
                        char str_length[CONTENT_LENGTH_MAX_DIGIT_SIZE];//response body content length max digit size
                        sprintf(str_length, "%d", response_length);
                        strcat(response, "HTTP/1.1 200 OK\nContent-Type: ");
                        strcat(response, http_ctx->response_content_type);
                        strcat(response, "\nContent-Length:");
                        strcat(response, &str_length[0]);
                        strcat(response, "\n");
                        int lens = strlen(response);
                        if (http_ctx->response_headers != NULL) {
                                if (str_len(http_ctx->response_headers) != 0) {
                                        strcat(response, http_ctx->response_headers);
                                }
                        }
                        strcat(response, "\n\n");
                        strcat(response, response_body);
                        send(req->socket_id, response, lens + response_length + 2, MSG_DONTWAIT | MSG_NOSIGNAL);
                } else {

                        char str_length[CONTENT_LENGTH_MAX_DIGIT_SIZE];
                        sprintf(str_length, "%d", response_length);
                        strcat(response, "HTTP/1.0 200 OK\nConnection: keep-alive\nContent-Type: text/html\nContent-Length:");
                        strcat(response, str_length);
                        strcat(response, "\n");
                        int lens = strlen(response);
                        if (http_ctx->response_headers != NULL) {
                                if (str_len(http_ctx->response_headers) != 0) {
                                        strcat(response, http_ctx->response_headers);
                                }
                        }
                        strcat(response, "\n\n");
                        strcat(response, response_body);
                        send(req->socket_id, response, lens + response_length + 2, MSG_DONTWAIT | MSG_NOSIGNAL);
                }

        }
        else {
                // Code Block if the Request is not a keepalive request
                if (req->minor_version == 1) {

                        char str_length[CONTENT_LENGTH_MAX_DIGIT_SIZE];
                        sprintf(str_length, "%d", response_length);

                        strcat(response, "HTTP/1.1 200 OK\nConnection: close\nContent-Type: text/html\nContent-Length:");
                        strcat(response, &str_length[0]);
                        strcat(response, "\n");
                        int lens = strlen(response);
                        if (http_ctx->response_headers != NULL) {
                                if (str_len(http_ctx->response_headers) != 0) {
                                        strcat(response, http_ctx->response_headers);
                                }
                        }
                        strcat(response, "\n\n");
                        strcat(response, response_body);
                        send(req->socket_id, response, lens + response_length + 2, MSG_DONTWAIT | MSG_NOSIGNAL);

                } else {

                        char str_length[CONTENT_LENGTH_MAX_DIGIT_SIZE];

                        sprintf(str_length, "%d", response_length);
                        strcat(response, "HTTP/1.0 200 OK\nConnection: close\nContent-Type: text/html\nContent-Length:");
                        strcat(response, &str_length[0]);
                        strcat(response, "\n");
                        int lens = strlen(response);
                        if (http_ctx->response_headers != NULL) {
                                if (str_len(http_ctx->response_headers) != 0) {
                                        strcat(response, http_ctx->response_headers);
                                }
                        }
                        strcat(response, "\n\n");
                        strcat(response, response_body);
                        send(req->socket_id, response, lens + response_length + 2, MSG_DONTWAIT | MSG_NOSIGNAL);
                }
                close(req->socket_id);
        }


        http_ctx->func_ptr = NULL;
        //freeing all allocated memory from the request object for the current request
        // we are not freeing file contents memory because its a cache pointer that is being reused
        free(req);
        free(http_ctx->response_headers);
        free(http_ctx);


}

void *get_function(char *module_name, char *func_name) {
        //function to load given func_name from .so file
        void *handle;
        void *func; // The Function Pointer which will hold the function from the .so file
        handle = dlopen(module_name, RTLD_NOW);
        if (!handle) {
                printf("%s\n", "error is from handle in linking");
                fputs(dlerror(), stderr);
                exit(1);
        }
        char *error;
        func = dlsym(handle, func_name);
        if ((error = dlerror()) != NULL) {
                printf("%s\n", "error is from sym linking");
                fputs(error, stderr);
                exit(1);
        }
        return func;
}

void inthread_work(struct http_context *http_ctx) {
        // Default Mode or In-Thread Mode of processing for request
        http_ctx->response_body = malloc(sizeof(char) * DEFAULT_BUFFER_SIZE);
        http_ctx->response_body[0] = '\0';

        http_ctx->response_headers = malloc(sizeof(char) * DEFAULT_BUFFER_SIZE);
        http_ctx->response_headers[0] = '\0';

        http_ctx->response_content_type = malloc(sizeof(char) * DEFAULT_BUFFER_SIZE);
        strcpy(http_ctx->response_content_type, "text/html\0");

        http_ctx->func_ptr(http_ctx); //executing controller function
        send_response(http_ctx); //sending response to client
}

void pass_msg(struct http_context *http_ctx) {
        // Worker Thread Mode for processing for request

        static int counter = 0; //counter to send event notification to n/w thread in round robin.
        http_ctx->func_ptr(http_ctx);//controller execution.

        enqueue(http_ctx);//pushing to lock free queue.
        counter++;

        //event notification to n/w threads that queue has data.
        eventfd_write(_eventfd[counter % server_config->nio_number_of_filehandlers_for_message_passing], 1);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void worker_thread_work() {
        // Worker Thread Mode for processing for request
        int number_of_ready_events; //epoll collected events counts
        struct epoll_event *events; //epoll events
        //maximum  events epoll should collect before returning in batch
        int max_events_to_stop_waiting = server_config->nio_number_of_filehandlers_for_message_passing / 2;
        //maximum  time epoll should collect before returning in batch
        int epoll_time_wait = 10000; //time wai for message passing
        events = malloc(sizeof(struct epoll_event) * max_events_to_stop_waiting); //epoll event allocation for multiplexing
        int i;
        struct http_context *arg;
        while (1) {

                number_of_ready_events = epoll_wait(worker_epfd, events, max_events_to_stop_waiting, epoll_time_wait);

                for (i = 0; i < number_of_ready_events; i++) {
                        eventfd_t val;
                        eventfd_read(events[i].data.fd, &val);
                        while ((arg = wdequeue()) != NULL) {
                                pass_msg(arg);
                                arg = NULL;
                        }

                }

        }

}
#pragma clang diagnostic pop

void submit_to_worker_thread(struct http_context *arg) {
        static int counter = 0;
        wenqueue(arg);
        counter++;
        eventfd_write(msg_passer[counter % server_config->nio_number_of_filehandlers_for_message_passing], 1);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

void network_thread_function() {

        int number_of_ready_events; //epoll collected events counts
        struct epoll_event *events; //epoll events
        //maximum  events epoll should collect before returning in batch
        int max_events_to_stop_waiting = server_config->epoll_max_events_to_stop_waiting;
        //maximum  time epoll should collect before returning in batch
        int epoll_time_wait = server_config->epoll_time_wait;
        events = malloc(sizeof(struct epoll_event) * max_events_to_stop_waiting); //epoll event allocation for multiplexing
        int i = 0, j = 0; //iterators
        char read_buffer[server_config->read_buffer_size]; //read buffer to load http request
        char header_key[MAX_NO_OF_HEADERS], header_value[MAX_NO_OF_HEADERS];
        int minor_version; //http minor version of received http request
        struct phr_header headers[MAX_NO_OF_HEADERS];
        size_t read_buffer_length = 0, method_length, path_length, num_headers;
        ssize_t received_bytes;
        char *method, *path;
        int body_index = 0;
        int headerover = 0;
        int header_end = 0;
        struct http_request *req;
        struct http_context *arg = NULL;


        //Main Block for reading requests using epoll
        while (1) {

                number_of_ready_events = epoll_wait(epfd, events, max_events_to_stop_waiting, epoll_time_wait);
                if (number_of_ready_events == 0) {
                        // for flushing rem req from queue if request not recevied
                        while ((arg = dequeue()) != NULL) {
                                send_response(arg);
                                arg = NULL;
                        }
                        continue;
                }
                // operation over epoll collected events
                for (i = 0; i < number_of_ready_events; i++) {
                        //if the collected event is event notification
                        // work with queue.
                        if (events[i].data.ptr != NULL) {
                                if (events[i].data.ptr == notif) {
                                        eventfd_t val;
                                        eventfd_read(events[i].data.fd, &val);
                                        while ((arg = dequeue()) != NULL) {
                                                send_response(arg);
                                                arg = NULL;
                                        }
                                        continue;
                                }

                        }


                        read_buffer[0] = '\0';
                        received_bytes = -1;

                        //read from socket
                        received_bytes = recv(events[i].data.fd, read_buffer, server_config->read_buffer_size, MSG_DONTWAIT);

                        if (received_bytes <= 0) {
                                close(events[i].data.fd);
                                number_of_active_sockets--;
                                //if received bytes < 0 ignore request
                                // before ending, send response to req context in queue
                                if ((arg = dequeue()) != NULL) {
                                        send_response(arg);
                                        arg = NULL;
                                }
                                continue;
                        }
                        req = malloc(sizeof(struct http_request));
                        req->socket_id = events[i].data.fd;


                        //splitting header and body;
                        header_end = received_bytes - 4; //header_end is an index to use in read_buffer. 4 indicates \r\n\r\n in http request to split body and header.
                        body_index = 0;
                        headerover = 0; //body array iterator and headerover is a flag to use inside below loop
                        for (j = 0; j < header_end; j++) {
                                if (headerover == 0) {
                                        if (read_buffer[j] == '\r' && read_buffer[j + 1] == '\n' && read_buffer[j + 2] == '\r' &&
                                            read_buffer[j + 3] == '\n') {
                                                j = j + 4;
                                                header_end = received_bytes;
                                                headerover = 1;
                                                break;
                                        }
                                }
                        }

                        if (headerover == 1) {

                                while (j < header_end) {
                                        req->body[body_index++] = read_buffer[j];
                                        j++;
                                }
                        }
                        //end of splitting header and body. now body holds full data payload in http request.


                        //parsing headers
                        num_headers = sizeof(req->headers) / sizeof(req->headers[0]);
                        phr_parse_request(read_buffer, received_bytes, &method, &method_length, &path, &path_length,
                                          &req->minor_version, req->headers, &num_headers, 0);
                        //end of parsing headers


                        // req->keepalive=1;
                        // Set if request is keepalive or not
                        if (req->minor_version == 1) {
                                req->keepalive = 1;
                        }

                        //TODO: HTTP 1.0 Support
                        //Below code is for http 1.0 support

                        //    for (j = 0; j != num_headers; j++) {
                        //
                        //         sprintf(header_key,"%.*s", (int)req->headers[j].name_len, req->headers[j].name);
                        //         sprintf(header_value,"%.*s", (int)req->headers[j].value_len, req->headers[j].value);
                        //
                        //         if (strcmp(header_key,"Connection")==0) {
                        //                 if (header_value[0]=='K'||header_value[1]=='K'||header_value[0]=='k'||header_value[1]=='k') {
                        //                         req->keepalive=1;
                        //                 }
                        //         }
                        //    }




                        if (req->keepalive != 1) {
                                epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
                        }

                        struct http_context *http_ctx = malloc(sizeof(struct http_context));
                        http_ctx->req = req;


                        char *key_path = malloc(MAX_KEY_PATH_SIZE);
                        sprintf(key_path, "%.*s", (int) path_length, path);

                        // Checking Route if it is configured to run on Worker Mode
                        int *trie_value = trie_lookup(trie_for_worker, key_path);
                        if (trie_value != NULL) {
                                http_ctx->response_body = malloc(sizeof(char) * DEFAULT_BUFFER_SIZE);
                                http_ctx->response_body[0] = '\0';
                                http_ctx->response_headers = malloc(sizeof(char) * DEFAULT_BUFFER_SIZE);
                                http_ctx->response_headers[0] = '\0';
                                http_ctx->response_content_type = malloc(sizeof(char) * DEFAULT_BUFFER_SIZE);
                                strcpy(http_ctx->response_content_type, "text/html\0");
                                int y = *trie_value;
                                http_ctx->func_ptr = func[y];
                                submit_to_worker_thread(http_ctx);
                                if ((arg = dequeue()) != NULL) {
                                        send_response(arg);
                                        arg = NULL;
                                }
                                free(key_path);
                                continue;
                        }



                        // Checking Route if it is configured to run on Default or InThread Mode
                        trie_value = trie_lookup(trie_for_NIO, key_path);
                        if (trie_value != NULL) {
                                int y = *trie_value;
                                http_ctx->func_ptr = func[y];
                                inthread_work(http_ctx);
                                free(key_path);
                                continue;
                        }


                        // Checking Route if it is configured for File Serving
                        trie_value = trie_lookup(trie_for_files, key_path);
                        if (trie_value != NULL) {
                                http_ctx->func_ptr = func[*trie_value];
                                char *tmp = files->files[*trie_value];
                                http_ctx->response_body = tmp;


                                http_ctx->response_headers = malloc(sizeof(char) * DEFAULT_BUFFER_SIZE); //1kb for headers
                                http_ctx->response_headers[0] = '\0';
                                http_ctx->response_content_type = files->content_types[*trie_value];
                                send_file_response(http_ctx);
                                free(key_path);
                                continue;
                        }

                        http_ctx->response_body = error_page_404_body;
                        http_ctx->response_headers = NULL;
                        http_ctx->response_content_type = error_page_404_content_type;
                        free(key_path);
                        send_file_response(http_ctx);

                        //flush request if queue has data
                        if ((arg = dequeue()) != NULL) {
                                send_response(arg);
                                arg = NULL;
                        }


                }
        }
}

#pragma clang diagnostic pop


int main() {
        int ret, i;
        notif = malloc(10);
        strcpy(notif, "EVENTNOTE");
        //initializing server configuration
        server_config = malloc(sizeof(struct server_config));
        load_config(server_config);

        // Loading error pages
        error_page_404_body = malloc(1);
        load_file(server_config->error_page_404_path, error_page_404_body);
        error_page_404_content_type = server_config->error_page_404_content_type;
        GAsyncQueue *queue_of_client_sockets = g_async_queue_new(); //queue for maintaining active sockets


        //Initializes the caches
        //ie) routing cache and function pointer caches.
        trie_for_NIO = trie_create(); //trie for default or in-thread mode routing
        trie_for_worker = trie_create(); //trie for worker-thread mode routing
        trie_for_files = trie_create(); //trie for file-serve mode routing

        //Loading the Routes and preparing the trie cache
        for (z = 0; z < server_config->number_of_routes - 1; z++) {
                int *zp = malloc(sizeof(int));
                *zp = z;

                //In the route[z].type W represents worker mode, N represents default mode and F represents file serving mode

                if (strcmp(server_config->routes[z].type, "W") == 0) {

                        inserted = trie_insert(trie_for_worker, server_config->routes[z].url, zp);

                        func[z] = get_function(server_config->routes[z].module_path, server_config->routes[z].function_name);

                } else if (strcmp(server_config->routes[z].type, "N") == 0) {

                        inserted = trie_insert(trie_for_NIO, server_config->routes[z].url, zp);

                        func[z] = get_function(server_config->routes[z].module_path, server_config->routes[z].function_name);

                } else if (strcmp(server_config->routes[z].type, "F") == 0) {

                        files = load_files_to_trie(server_config->routes[z].url, server_config->routes[z].module_path,
                                                   trie_for_files);

                } else {
                        printf("INVALID ROUTE %s \t %s \t %s \t %s\n", server_config->routes[z].type, server_config->routes[z].url,
                               server_config->routes[z].module_path, server_config->routes[z].function_name);
                        continue;
                }
        }

        //setting up epoll for message passing between worker and network threads
        worker_epfd = epoll_create(server_config->nio_number_of_filehandlers_for_message_passing); //file descriptor referring to the new epoll instance

        for (i = 0; i < server_config->nio_number_of_filehandlers_for_message_passing; i++) {
                msg_passer[i] = eventfd(0, EFD_NONBLOCK);
                struct epoll_event msg_passer_evnt;
                msg_passer_evnt.data.fd = msg_passer[i];
                msg_passer_evnt.events = EPOLLIN | EPOLLET;
                epoll_ctl(worker_epfd, EPOLL_CTL_ADD, msg_passer[i], &msg_passer_evnt);
        }

        socklen_t addrlen, clientaddresslen;
        struct sockaddr_in address, clientaddress;
        memset((void *) &address, (int) '\0', sizeof(address));
        memset((void *) &clientaddress, (int) '\0', sizeof(clientaddress));

        //creating server sokect
        if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) > 0) {
                printf("The socket has been created\n");
        }
        int opt = 1;
        setsockopt(server_socket, IPPROTO_TCP, TCP_NODELAY, (void *) &opt, sizeof(opt));
        setsockopt(server_socket, IPPROTO_TCP, TCP_QUICKACK, (void *) &opt, sizeof(opt));


        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(server_config->server_port);
        int optval = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_LINGER, &optval, sizeof(optval));
        //binding socket to port and address
        if (bind(server_socket, (struct sockaddr *) &address, sizeof(address)) == 0) {
                printf("Server is binded to the port %d \n", server_config->server_port);
        } else {
                printf("Binding to port %d Failed\n", server_config->server_port);
                exit(75);
        }



        //epfd initialization for message passing
        epfd = epoll_create(server_config->epoll_init_connection_size - 1);
        struct epoll_event event;
        for (i = 0; i < server_config->nio_number_of_filehandlers_for_message_passing; i++) {
                _eventfd[i] = eventfd(0, EFD_NONBLOCK);
                struct epoll_event evnt;
                evnt.data.fd = _eventfd[i];
                evnt.data.ptr = notif;
                evnt.events = EPOLLIN | EPOLLET;
                epoll_ctl(epfd, EPOLL_CTL_ADD, _eventfd[i], &evnt);
        }

        // network thread creation
        pthread_t threads[server_config->number_of_network_thread];
        for (i = 0; i < server_config->number_of_network_thread; i++) {
                pthread_create(&threads[i], NULL, &network_thread_function, NULL);
        }

        // worker thread creation
        pthread_t wthreads[server_config->number_of_worker_thread];
        for (i = 0; i < server_config->number_of_worker_thread; i++) {
                pthread_create(&wthreads[i], NULL, &worker_thread_work, NULL);

        }

        //receiving new requests.
        while (1) {
                if (listen(server_socket, server_config->listen_backlog) < 0) {
                        perror("server: listen");
                        //exit(1);
                }

                clientaddresslen = sizeof(clientaddress);
                if ((client_socket = accept4(server_socket, (struct sockaddr *) &clientaddress, &clientaddresslen,
                                             SOCK_NONBLOCK)) < 0) {
                        perror("server: accept");
                        //exit(1);
                }

                if (client_socket > 0) {

                        setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY, (void *) &opt, sizeof(opt));
                        setsockopt(client_socket, IPPROTO_TCP, TCP_QUICKACK, (void *) &opt, sizeof(opt));

                        event.data.fd = client_socket;
                        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                        ret = epoll_ctl(epfd, EPOLL_CTL_ADD, event.data.fd, &event);

                        int *sock_id_val = malloc(4);
                        *sock_id_val = client_socket;
                        g_async_queue_push(queue_of_client_sockets, sock_id_val);
                        number_of_active_sockets = number_of_active_sockets + 1;

                        if (number_of_active_sockets > server_config->epoll_init_connection_size) {
                                int *temp = g_async_queue_pop(queue_of_client_sockets);

                                ret = epoll_ctl(epfd, EPOLL_CTL_DEL, *temp, NULL);
                                printf("%d\n", ret);
                                printf("%d\n", *temp);
                                close(*temp);
                                free(temp);
                                number_of_active_sockets = number_of_active_sockets - 1;
                        }

                }

        }
        close(server_socket);
        return 0;
}

int load_file(char *file_path, char *file_body) {
        int input_fd, file_length;
        input_fd = open(file_path, O_RDONLY);
        file_length = (int) lseek(input_fd, (off_t) 0, SEEK_END);
        lseek(input_fd, (off_t) 0, SEEK_SET);
        realloc(file_body, file_length);
        read(input_fd, file_body, file_length);
        return file_length;
}
