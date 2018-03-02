#ifndef __USE_GNU
#define __USE_GNU
#endif
#define _GNU_SOURCE

#include "server.h"
#include <unistd.h>
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// function to split character array from a character
int split(char *str, char c, char ***arr) {
    int count = 1;
    int token_len = 1;
    int i = 0;
    char *p;
    char *t;

    p = str;
    while (*p != '\0') {
        if (*p == c)
            count++;
        p++;
    }

    *arr = (char **) malloc(sizeof(char *) * count);
    if (*arr == NULL)
        exit(1);

    p = str;
    while (*p != '\0') {
        if (*p == c) {
            (*arr)[i] = (char *) malloc(sizeof(char) * token_len);
            if ((*arr)[i] == NULL)
                exit(1);

            token_len = 0;
            i++;
        }
        p++;
        token_len++;
    }
    (*arr)[i] = (char *) malloc(sizeof(char) * token_len);
    if ((*arr)[i] == NULL)
        exit(1);

    i = 0;
    p = str;
    t = ((*arr)[i]);
    while (*p != '\0') {
        if (*p != c && *p != '\0') {
            *t = *p;
            t++;
        }
        else {
            *t = '\0';
            i++;
            t = ((*arr)[i]);
        }
        p++;
    }

    return count;
}

// function for str_len
int str_len(const char *str) {
    const char *s;
    for (s = str; *s; ++s);
    return (s - str);
}

// function for string endswith check
int endswith(const char *withwhat, const char *what) {
    int l1 = str_len(withwhat);
    int l2 = str_len(what);
    if (l1 > l2)
        return 0;

    return strcmp(withwhat, what + (l2 - l1));
}

// function for concatenating strings
void concatenate_string(char *original, char *add) {
    while (*original)
        original++;

    while (*add) {
        *original = *add;
        add++;
        original++;
    }
    *original = '\0';
}


//create string_array struct type
void create_string_array(struct string_array *array, int size, int max_char_size) {
    array->data = malloc(size * sizeof(char *));
    array->used = 0;
    array->size = size;
    array->max_char_size = max_char_size;
}

//append to string array
void append_to_string_array(struct string_array *array, char *value) {
    if (array->used < array->size) {
        char *tmp = malloc(array->max_char_size * sizeof(char));
        strcpy(tmp, value);
        array->data[array->used] = tmp;
        array->used += 1;
    } else {
        char **tdata = malloc((array->size + 1) * sizeof(char *));
        int i;
        for (i = 0; i < array->used; i++) {
            char *tmp = malloc(array->max_char_size * sizeof(char));
            strcpy(tmp, array->data[i]);
            free(array->data[i]);
            tdata[i] = tmp;
        }
        char *tmp = malloc(array->max_char_size * sizeof(char));

        strcpy(tmp, value);
        tdata[array->used] = tmp;
        array->used += 1;
        array->size += 1;
        free(array->data);
        array->data = tdata;
    }
}

//function to print string array
void display_string_array(struct string_array *array) {
    int i;
    for (i = 0; i < array->used; i++) {
        printf("%s\n", array->data[i]);
    }
}
