#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "server.h"
extern const char *__progname;
int parse_server_config_file(char *file_path, struct server_config *config);
int parse_routes_config_file(char *file_path, struct server_config *config);
void display_config(struct server_config *config);
void load_config(struct server_config *config);


void load_config(struct server_config *config) {
        /* Getting the Installation Folder Path*/
        config->folder_path = get_vega_executable_folder_path();

        /* Loading the Server Configuration File*/
        char *server_conf_path = malloc(str_len(config->folder_path)+21);
        strcpy(server_conf_path,config->folder_path);
        concatenate_string(server_conf_path,"conf/vega-server.conf");
        parse_server_config_file(server_conf_path,config);
        free(server_conf_path);

        /* Loading the Routes Configuration File*/
        char *server_routes_path = malloc(str_len(config->folder_path) + 21);
        strcpy(server_routes_path, config->folder_path);
        concatenate_string(server_routes_path, "conf/vega-routes.conf");
        parse_routes_config_file(server_routes_path, config);
        free(server_routes_path);
}

void display_config(struct server_config *config) {
        printf("Configuration Folder Location : %sconf/\n", config->folder_path);
        printf("number_of_worker_thread -> %d\n",
               (int)config->number_of_worker_thread);
        printf("number_of_network_thread -> %d\n",
               (int)config->number_of_network_thread);
        printf("server_port -> %d\n", (int)config->server_port);
        printf("epoll_init_connection_size -> %d\n",
               (int)config->epoll_init_connection_size);
        printf("listen_backlog -> %d\n", (int)config->listen_backlog);
        printf("epoll_max_events_to_stop_waiting -> %d\n",
               (int)config->epoll_max_events_to_stop_waiting);
        printf("epoll_time_wait -> %d\n", (int)config->epoll_time_wait);
        printf("read_buffer_size -> %d\n", (int)config->read_buffer_size);
        printf("nio_number_of_filehandlers_for_message_passing -> %d\n", (int)config->nio_number_of_filehandlers_for_message_passing);
        printf("request_max_body_size -> %d\n", (int)config->request_max_body_size);
        printf("max_concurrent_upload_request -> %d\n",
               (int)config->max_concurrent_upload_request);

        int i = 0;
        for (i = 0; i < config->number_of_routes - 1; i++) {
                printf("%s \t %s \t %s \t %s\n", config->routes[i].type, config->routes[i].url,config->routes[i].module_path,config->routes[i].function_name);
        }
}

char *get_vega_executable_folder_path() {
        char *file_path = malloc(128);
        readlink("/proc/self/exe", file_path, 50);
        int len = str_len(file_path);
        int exec_len = str_len(__progname);
        char *folder_path = malloc((len - exec_len) * sizeof(int));
        int i = 0;
        int j = 0;
        for (j = i; j < len - exec_len; j++)
                folder_path[j] = file_path[j];
        free(file_path);
        return folder_path;
}

int parse_server_config_file(char *file_path, struct server_config *config) {
        printf("Loading Configuration file %s \n", file_path);
        int file_length = 0;
        int input_fd;
        ssize_t ret_in;
        input_fd = open(file_path, O_RDONLY);
        if (input_fd == -1) {
                perror("Error in Opening Configuration");
                return;
        }
        file_length = (int)lseek(input_fd, (off_t)0, SEEK_END);
        lseek(input_fd, (off_t)0, SEEK_SET);
        char buffer[file_length];

        ret_in = read(input_fd, &buffer[0], file_length);
        if (ret_in == -1) {
                perror("Error in reading Configuration");
                return;
        }
        const char line_delimeter = '\n';
        const char equal_delimeter = '=';
        char **line = NULL;

        int number_of_lines = split(buffer, line_delimeter, &line);
        int i = 0;
        for (i = 0; i < number_of_lines; i++) {
                if (line[i][0] != '#') {
                        char **elements = NULL;
                        if (split(line[i], equal_delimeter, &elements) == 2) {
                                if (strcmp(elements[0], "server.number_of_worker_thread") == 0) {
                                        config->number_of_worker_thread = atoi(elements[1]);
                                        continue;
                                }
                                if (strcmp(elements[0], "server.number_of_network_thread") == 0) {
                                        config->number_of_network_thread = atoi(elements[1]);
                                        continue;
                                }
                                if (strcmp(elements[0], "server.server_port") == 0) {
                                        config->server_port = atoi(elements[1]);
                                        continue;
                                }
                                if (strcmp(elements[0], "server.epoll_init_connection_size") == 0) {
                                        config->epoll_init_connection_size = atoi(elements[1]);
                                        continue;
                                }
                                if (strcmp(elements[0], "server.listen_backlog") == 0) {
                                        config->listen_backlog = atoi(elements[1]);
                                        continue;
                                }
                                if (strcmp(elements[0], "server.epoll_max_events_to_stop_waiting") ==
                                    0) {
                                        config->epoll_max_events_to_stop_waiting = atoi(elements[1]);
                                        continue;
                                }
                                if (strcmp(elements[0], "server.epoll_time_wait") == 0) {
                                        config->epoll_time_wait = atoi(elements[1]);
                                        continue;
                                }
                                if (strcmp(elements[0], "server.read_buffer_size") == 0) {
                                        config->read_buffer_size = atoi(elements[1]);
                                        continue;
                                }
                                if (strcmp(elements[0], "server.nio_number_of_filehandlers_for_message_passing") == 0) {
                                        config->nio_number_of_filehandlers_for_message_passing = atoi(elements[1]);
                                        continue;
                                }
                                if (strcmp(elements[0], "server.request_max_body_size") == 0) {
                                        config->request_max_body_size = atoi(elements[1]);
                                        continue;
                                }
                                if (strcmp(elements[0], "server.max_concurrent_upload_request") == 0) {
                                        config->max_concurrent_upload_request = atoi(elements[1]);
                                        continue;
                                }
                                if (strcmp(elements[0], "server.error.404.file_path") == 0) {
                                        char *tmp = malloc(strlen(elements[1])*sizeof(char));
                                        strcpy(tmp,elements[1]);
                                        config->error_page_404_path =tmp;
                                        printf("errror config path %s\n",config->error_page_404_path);
                                        continue;
                                }
                                if (strcmp(elements[0], "server.error.404.content_type") == 0) {
                                        char *tmp = malloc(strlen(elements[1])*sizeof(char));
                                        strcpy(tmp,elements[1]);
                                        config->error_page_404_content_type =tmp;
                                        continue;
                                }
                        } else {
                                if (line[i][0] >= 32 && line[i][0] <= 126)
                                        printf("Ignoring Configuration line : %s\n", line[i]);
                        }
                }
        }
}

int parse_routes_config_file(char *file_path, struct server_config *config) {
        printf("Loading Routes from %s\n", file_path);
        int file_length = 0;
        int input_fd;
        ssize_t ret_in;
        input_fd = open(file_path, O_RDONLY);
        if (input_fd == -1) {
                perror("Error in Opening Routes Configuration");
                return;
        }
        file_length = (int)lseek(input_fd, (off_t)0, SEEK_END);
        lseek(input_fd, (off_t)0, SEEK_SET);
        char buffer[file_length];

        ret_in = read(input_fd, &buffer[0], file_length);
        if (ret_in == -1) {
                perror("Error in reading Routes Configuration");
                return;
        }
        const char line_delimeter = '\n';
        const char comma_delimeter = ',';
        char **line = NULL;

        int number_of_lines = split(buffer, line_delimeter, &line);

        struct route routes[number_of_lines];

        int routeIndex = 0;

        int i = 0;
        for (i = 0; i < number_of_lines; i++) {
                if (line[i][0] != '#') {
                        char **elements = NULL;
                        if (split(line[i], comma_delimeter, &elements) == 4) {
                                struct route current_route;
                                current_route.type = elements[0];
                                current_route.url = elements[1];
                                current_route.module_path = elements[2];
                                current_route.function_name = elements[3];
                                routes[routeIndex++] = current_route;
                        } else {
                                if (line[i][0] >= 32 && line[i][0] <= 126)
                                        printf("Ignoring Configuration line : %s\n", line[i]);
                        }
                }
        }

        struct route *final_routes = malloc(sizeof(struct route) * routeIndex + 1);
        for (i = 0; i < routeIndex; i++) {
                final_routes[i].type = malloc(str_len(routes[i].type));
                strcpy(final_routes[i].type, routes[i].type);
                final_routes[i].url = malloc(str_len(routes[i].url));
                strcpy(final_routes[i].url, routes[i].url);
                final_routes[i].module_path = malloc(str_len(routes[i].module_path));
                strcpy(final_routes[i].module_path,routes[i].module_path);
                final_routes[i].function_name = malloc(str_len(routes[i].function_name));
                strcpy(final_routes[i].function_name,routes[i].function_name);
        }

        config->number_of_routes = routeIndex + 1;
        config->routes = final_routes;
}
