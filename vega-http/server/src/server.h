#include "http_parser.h"
#include "trie.h"
#include "vega.h"

// http request Context
struct http_request {
        int socket_id; // Socket ID
        int keepalive; // 1 for keep alive request and 0 for normal request
        int minor_version;  // Http Version
        char body[512*512]; // Size of payload
        struct phr_header headers[100]; // Http Headers
};

// Route Struct This is passed to the NIO threads during initialization
struct route {
        char* type; // What type of request is the route Configured to W,N,F
        char* url; // The Url Path for the route
        char* module_path; // The Location of the shared object File
        char* function_name; // The function name in the .so File
};

struct server_config {
        char *folder_path; // The folder path where the vega server executable is located
        int number_of_worker_thread; // Number of worker threads
        int number_of_network_thread; // Number of Network threads
        int server_port; // Port with which the server should Bind To
        int epoll_init_connection_size; // Initial epoll connection size Should not be 0
        int listen_backlog; // Kernel socket listen backlog size
        int epoll_max_events_to_stop_waiting; // epoll Event trigger size
        int epoll_time_wait; // epoll Time trigger period in milliseconds
        int read_buffer_size; // The read Buffer size for the Requests in Bytes
        int number_of_routes; // The number of routes that are being configured
        int request_max_body_size; //The maximum size of the Request in Bytes
        int max_concurrent_upload_request; //TODO For Multi Part File Upload
        int nio_number_of_filehandlers_for_message_passing; // Number of file handlers for message passing between NIO threads and Worker Threads
        char *error_page_404_path; // The path where the content for 404 page is saved
        char *error_page_404_content_type; // The context Type for the 404 page
        struct route* routes; // List of Routes Configured
};

// Functions are Declared in server/src/config.c
void load_config(struct server_config* config);
void display_config(struct server_config* config);
char *get_vega_executable_folder_path();

// The file_cache Struct for caching Files
struct file_cache {
        char **files; // Array of File's Content
        char **content_types; // Array Http response Content_type for the given File's Content
};

// Functions are Declared in server/src/file_serving_init.c
struct file_cache* load_files_to_trie(const char *path,const char *name,struct trie *trie_map);

// Struct used for Dynamic String Array
struct string_array {
        char **data;
        int max_char_size;
        int used;
        int size;
};

// Function are declared in src/utils.c
void create_string_array(struct string_array *array,int size,int max_char_size);
void append_to_string_array(struct string_array * array,char *value);
void display_string_array(struct string_array * array);
int split ( char *str, char c, char ***arr);
int endswith(const char* withwhat, const char* what);
void concatenate_string(char *original, char *add);
int str_len(const char *str);


extern struct http_context * dequeue();
extern void enqueue(struct http_context *data);

extern struct http_context * wdequeue();
extern void wenqueue(struct http_context *data);
