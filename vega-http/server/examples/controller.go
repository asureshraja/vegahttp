package main

/*
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../src/vega.h"
extern void handler(struct http_context *args);
*/
import "C"
import "unsafe"

//export handler
func handler(args *C.struct_http_context) {
	cs := C.CString("go handler")
	defer C.free(unsafe.Pointer(cs))
	C.strcpy(args.response_body, cs)
}

func main() {}
