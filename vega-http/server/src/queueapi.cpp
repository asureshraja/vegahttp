#include <iostream>
#include <string>
#include "concurrentqueue.h"
#include <string.h>
#include "vega.h"
moodycamel::ConcurrentQueue<struct http_context *> q(10000000);
extern "C" void enqueue(struct http_context *arg){
        while(q.try_enqueue(arg)==false) {
                std::cout << "failing in q elem allocation" << std::endl;
        }

}
extern "C" struct http_context * dequeue(){
        struct http_context *item;
        bool retval = q.try_dequeue(item);
        if(!retval) {return NULL; }
        return item;
}
moodycamel::ConcurrentQueue<struct http_context *> wq(10000000);
extern "C" void wenqueue(struct http_context *arg){
        while(wq.try_enqueue(arg)==false) {
                std::cout << "failing in q elem allocation" << std::endl;
        }

}
extern "C" struct http_context * wdequeue(){
        struct http_context *item;
        bool retval = wq.try_dequeue(item);
        if(!retval) {return NULL; }
        return item;
}
