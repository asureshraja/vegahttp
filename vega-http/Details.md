
Abstract

In this paper we present our new high performance web server vega-http and compare it with other existing popular open source web-servers and what makes it different from them. We will also present our existing platform and things we are going to add to it in the future in this given paper.

1) Introduction

A Web Server is a computer program that is responsible for accepting HTTP requests from clients and serving them HTTP responses along with optional data contents, which usually are web pages such as HTML documents and linked objects (images, etc).

2) Performance Comparison  

The Performance Comparison Test Environment

- CPU : intel i3 3.3Ghz (2 cores)
- RAM : 8GB
- OS  : Xubuntu 14.04 x86_64
- Test Suit : [WRK](https://github.com/wg/wrk)

Http-Server                                               | FileServing 100c | FileServing 1000c | HelloWorld workerMode 100c | HelloWorld workerMode 1000c | HelloWorld Default Mode 100c | HelloWorld Default Mode 1000c
--------------------------------------------------------- | ---------------- | ----------------- | -------------------------- | --------------------------- | ---------------------------- | -----------------------------
**Vega-http**                                             | **192129**       | **160389**        | **106568**                 | **114070**                  | **201620**                   | **173791**
[NxWeb](https://bitbucket.org/yarosla/nxweb/wiki/Home)    | 173742           | 140142            | 88297                      | 86894                       | 178489                       | 148050
[Netty](http://netty.io/)                                 | NA               | NA                | NA                         | NA                          | 139940                       | 131685
[Undertow](http://undertow.io/)                           | 38297            | 38198             | 105883                     | 106257                      | 158920                       | 137508
[RestExpress](https://github.com/RestExpress/RestExpress) | NA               | NA                | NA                         | NA                          | 80093                        | 78940
[Nginx](https://github.com/nginx/nginx/)                  | 64854            | 60383             | NA                         | NA                          | NA                           | NA
[Apache2](https://httpd.apache.org/)                      | 33410            | 32730             | NA                         | NA                          | NA                           | NA
[Tomcat-Servlet](https://tomcat.apache.org)               | NA               | NA                | NA                         | NA                          | 31826                        | 29953
[Tomcat-JSP](https://tomcat.apache.org)                   | NA               | NA                | NA                         | NA                          | 12499                        | 10302

3) Core library

Vega-htttp directly interacts with the Linux Kernel using Epoll command to receive and respond to new requests thus making it fast and highly customizable. The server also comes with all the epoll configurations that can be tuned for higher utilization of the server. The Vega-http server is built to scale vertically utilizing the highest possible of available resources unlike other servers. The absence of a GarbageCollector helps us to allocate and deallocate the memory manually thus keeping a low memory usage compared to the actions that the system performs.

The Library is built to serve three main purposes
- Lightweight high performance http-server where the controllers can be written in multiple languages.
- A simple File Serving Server.
- A Proxy-Pass server for load balancing.

4) Epoll Configuration

The Vega-http server has only one dependency that it should run in a system with a linux kernel version 2.0+ since only after that does it have Epoll for socket pooling. The main epoll configurations are
- epoll_init_connection_size
-   you are required to initialize the epoll pooling with a initial connection size as load increases or decreases the kernel tunes itself for the required load for performing better and reducing the throughput.
- epoll_time_wait
-   In epoll you do not listen you check for new connections in batches. The configuration parameter epoll_time_wait is specified in milliseconds the time between checking for new requests in the socket connection.
- epoll_max_events_to_stop_waiting
-   The are two instances where epoll_wait command exists one is when the given epoll_time_wait period is over and the other is when they have reached a minimum number of connections when they are done. It is important that this number is not to small since it will hamper performance.

Code example for epoll_wait in the main thread.

```
// Example of the epoll_wait command.
while(1) {
        number_of_ready_events = epoll_wait (epfd, events, max_events_to_stop_waiting, epoll_time_wait);
        // Code to handle the events.
      }
```

5) Thread modes

The vega-http has two main thread Pools for processing the requests. the first one is the NIO or the network threads which is responsible is reading the events and dispatching them or processing them depending on the route configurations.

The Second thread pool is the worker thread pool which is mainly used for heavy tasks such as IO operations that is best when performing heavy operations that can be handled or processed without having a lock in the NIO or network thread.

NIO threads can never be set to zero since they are the ones that initially process the request and dispatch it if required. If there are no routes configured with the Worker Mode, then you can set the worker thread count to 0 since it will never be used.

6) Routes and File Serving mappings

To improve the performance we are using a tire map which would have an index stored as the response of a trie lookup. The given index is used to access the data Object which is stored in an fixed size array. The change from a traditional hashMap implementation to a trieMap for routing increased the performance of route lookups by 10% which is a significant number for a such a small change in data Structure.

The trie map  is an ordered tree data structure that is used to store a dynamic set or associative array where the keys are usually strings. Unlike a binary search tree, no node in the tree stores the key associated with that node; instead, its position in the tree defines the key with which it is associated. All the descendants of a node have a common prefix of the string associated with that node, and the root is associated with the empty string. Values are not necessarily associated with every node. Rather, values tend only to be associated with leaves, and with some inner nodes that correspond to keys of interest.

Initially when the server is being started it loads the routes into three different trie Maps one for each mode W - for the Worker mode of execution N - for the Network Thread mode of execution and F - for the file handler mode of execution.

All the files are loaded into memory for quicker delivery (need to implement a on-demand loading of files). The route path and the files in the given directory is stored in the trie map for delivery. The index of the trie lookup is is pointed to the location where the files are stored in memory.

7) Controllers

The vega-routes.conf for a given controller

```
N,/helloWorld,examples/sample.so,hello_world_controller
```

The above route configuration sets the controller for the route /helloWorld to the function hello_world_controller in the examples/sample.so

A simple Controller example

```
#include "../src/arg.h"
#include <string.h>
void hello_world_controller(struct http_ctx *args){
        strcpy(args->response_headers,"Access-Control-Allow-Origin: *\0");
        strcpy(args->response_body,"hello  world\0");
}
```

Parameters in the http_ctx

```
// http_request structure
struct http_request {
        int socket_id;
        int keepalive;
        int minor_version;
        char body[512*512];
        struct phr_header headers[100];
};

// http_ctx structure
struct http_ctx {
        struct http_request *req;
        char *response_buffer;
        int core_id;
        char *response_body;
        char *response_headers;
        void* (*func_ptr)(struct http_ctx *);
};
```

The http_ctx is available for the controller
