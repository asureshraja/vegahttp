OBJ=./obj
LIB=/usr/local/lib
CURRENT=.
SOURCE=./src
SERVER_SRC=|HttpServer.c|http_parser.c|utils.c|trie.c|conf.c|file_serving_init.c
SERVER_FLAGS= -g -w -lpthread -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include  -lglib-2.0 -ldl
SERVER_BIN=vega
SERVERCPP_SRC=|queueapi.cpp
SERVERCPP_BIN=queueapi.o
SERVERCPP_FLAGS= -c -fPIC -Wall
SERVER_OBJECTS=|queueapi.o
SERVER_SHARED_OBJECT=libq.so
SERVER_SHARED_LIB=q
allmake:
	mkdir -p obj
	g++ $(subst |, ${SOURCE}/,${SERVERCPP_SRC}) -o ${OBJ}/${SERVERCPP_BIN} -std=c++11 ${SERVERCPP_FLAGS}
	g++ -shared -o ${LIB}/${SERVER_SHARED_OBJECT} $(subst |, ${OBJ}/,${SERVER_OBJECTS})
	gcc $(subst |, ${SOURCE}/,${SERVER_SRC}) -o ${CURRENT}/${SERVER_BIN} ${SERVER_FLAGS} -L${shell pwd}/lib -l${SERVER_SHARED_LIB}
	gcc src/conf.c src/utils.c src/conf_test.c -o conf_test

prepare_examples:
	#Compiling and Preparing .so files for c
	gcc -c -fPIC examples/sample.c -o examples/sample.o
	gcc -shared -o examples/sample.so examples/sample.o

	#Compiling and Preparing .so files for c++
	g++ -c -fPIC examples/cplus_example.cpp -o examples/cplus_example.o
	g++ -shared -o examples/cplus_example.so examples/cplus_example.o

	#Compiling and Preparing .so files for go requires go gcc compiler
	# go build -compiler gc -o examples/controller.so -buildmode=c-shared examples/controller.go
