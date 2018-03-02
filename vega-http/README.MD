An old Code Developed in an interest to make fastest HTTP server.

# Vega-http Server

A Simple Lightweight Web Server for High Performance Applications

### Features

* Lightweight http-server
* Simple Routing Mechanism
* Language Supports C, C++, and Go compiled with gcc
\


### [Installation Guide](/INSTALLATION.md)

##### Handler Function C Example

```
#include "../src/arg.h"
#include <string.h>
// The function name used here is required to reference the handler function to the routes
void simple_handler(struct worker_args *args) {
  //To respond with a message we simple assign it to the worker_args
  strcpy(args->response_body, "this is a sample response \0");
}
```

##### TO create the shared object file from *.c

```
# First we need to generate the *.o file for the given *.c file
gcc -c -fPIC examples/simple_handler.c -o examples/simple_handler.o

# to create the *.so file from the previously generated *.o file
gcc -shared -o examples/simple_handler.so examples/simple_handler.o

```

##### Handler Function Go-lang Example

```
package main
/*
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../src/arg.h"
extern void handler(struct worker_args *args);
*/
import "C"
import "unsafe"

//export handler
func handler(args *C.struct_worker_args) {
	cs := C.CString("go handler")
	defer C.free(unsafe.Pointer(cs))
	C.strcpy(args.response_body, cs)
}

func main() {}
```

##### TO create the shared object file from *.go

```
# compiles the .go file and generates the .so file that is to be used in the routes
go build -compiler gc -o examples/controller.so -buildmode=c-shared examples/controller.go
```


### Server Configuration

There are two configurations files for vega-http are located in the conf directory
* vega-server.conf
* vega-routes.conf

#### Server Configuration vega-server.conf

```
# Server Configuration
# Inline comments are Not Allowed
# The First Character of the line has to be # if the line is to be ignored

# server.number_of_worker_thread assigns the number of threads to be allocated to the worker thread
server.number_of_worker_thread=4

# server.number_of_network_thread assigns the number of threads to be allocated to the network thread Group
server.number_of_network_thread=2

# server.server_port The port on which the Server is supposed to Bind with
server.server_port=5000

server.epoll_init_connection_size=100000
server.listen_backlog=10000
server.epoll_max_events_to_stop_waiting=50
server.epoll_time_wait=25
server.read_buffer_size=262144
server.response_buffer_size=24576
server.request_max_body_size=26214400
server.response_max_body_size=1073741824
server.max_concurrent_upload_request=10000
```


### Server Routes Configuration vega-routes.conf


Any Given route has three different working modes

#### W - Worker Thread Mode
means that the given route will be dispatched to the worker thread for processing.
The request will be initially loaded in the network thread and then automatically dispatched based on the configuration. It is recommended to have all blocking requests and heavy processing in the worker thread.

#### N - Network Thread Mode
The network thread mode will run the given in the same network thread in which it has been received this is suitable for small tasks and any lightweight operations which would not require any block for precessing.

#### F - File Serving Mode
File serving mode is still in development. Requires some changes for serving heavy files.

```
# Server Routes Configuration
# Inline comments are Not Allowed
# The First Character of the line has to be # if the line is to be ignored
W,/ok,examples/sample.so,hello
N,/test,examples/sample.so,hello
W,/go,examples/controller.so,handler
#F,/hello,conf/sample.so,hello

```
