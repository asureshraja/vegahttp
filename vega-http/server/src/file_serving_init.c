#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include "server.h"
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

/*
   This File Provides Functions for Listing directories and loading all the files under the Directory recursively to the Files Cache.
 */

void list_dir(const char *name,struct string_array *array);

void load_mime_types(struct trie *trie_map);
char *get_extension_name(char *str);

void load_mime_types(struct trie *trie_map){

        char *filePath = malloc(60);
        filePath[0]='\0';
        strcat(filePath,get_vega_executable_folder_path());
        strcat(filePath,"conf/miscellaneous/MIME_Types.tsv");

        char *file_body;
        int input_fd,file_length;
        input_fd = open(filePath, O_RDONLY);
        file_length = (int)lseek(input_fd, (off_t)0, SEEK_END);
        lseek(input_fd, (off_t)0, SEEK_SET);
        file_body=malloc(file_length);
        read(input_fd, file_body, file_length);

        const char line_delimeter = '\n';
        const char equal_delimeter = '\t';
        char **line = NULL;

        int number_of_lines = split(file_body, line_delimeter, &line);
        int i = 0;
        for (i = 0; i < number_of_lines; i++) {
                if (line[i][0] != '#') {
                        char **elements = NULL;
                        if (split(line[i], equal_delimeter, &elements) == 2) {
                                if(elements[0][0]=='.') {
                                        trie_insert(trie_map,&elements[0][1],elements[1]);
                                }else{
                                        trie_insert(trie_map,elements[0],elements[1]);
                                }

                        }
                }
        }
}

struct file_cache* load_files_to_trie(const char *path,const char *name,struct trie *trie_map){
        struct trie *trie_for_mime = trie_create();
        load_mime_types(trie_for_mime);

        struct string_array *array = malloc(sizeof(struct string_array));
        struct file_cache *files = malloc(sizeof(struct file_cache));
        create_string_array(array,2,1024);
        list_dir(name,array);
        int name_length = strlen(name);

        files->files=malloc(array->used * sizeof(char*));
        // file_cache = malloc(array->used * sizeof(char*));
        files->content_types = malloc(array->used * sizeof(char*));
        int i;
        for(i=0; i < array->used; i++) {
                int *zp=malloc(sizeof(int));
                *zp=i;
                // char *tmp  = malloc((strlen(array->data[i])-name_length)*sizeof(char));
                char *tmp  = malloc(2048*sizeof(char));
                tmp[0] = '\0';
                concatenate_string(tmp,path);
                concatenate_string(tmp,&array->data[i][name_length]);
                trie_insert(trie_map,tmp,zp);

                char *file_body;
                int input_fd,file_length;
                input_fd = open(array->data[i], O_RDONLY);
                file_length = (int)lseek(input_fd, (off_t)0, SEEK_END);
                lseek(input_fd, (off_t)0, SEEK_SET);
                file_body=malloc(file_length);
                read(input_fd, file_body, file_length);

                char *content_type = trie_lookup(trie_for_mime,get_extension_name(array->data[i]));

                if(content_type==NULL) {
                        content_type=malloc(26);
                        strcpy(content_type,"application/octet-stream\0");
                }else{
                }

                files->files[i] = file_body;
                files->content_types[i] = content_type;
        }
        return files;
}



void list_dir(const char *name,struct string_array *array){
        DIR *dir;
        struct dirent *entry;

        if(!(dir = opendir(name))) {
                return;
        }
        if(!(entry = readdir(dir))) {
                return;
        }
        do {
                if(entry->d_type == DT_DIR) {
                        char folder_name[1024];
                        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                                continue;
                        folder_name[0]='\0';
                        strcat(folder_name,name);
                        strcat(folder_name,"/");
                        strcat(folder_name,entry->d_name);
                        list_dir(folder_name,array);
                }else{
                        char file_path[1024];
                        file_path[0]='\0';
                        strcat(file_path,name);
                        strcat(file_path,"/");
                        strcat(file_path,entry->d_name);
                        append_to_string_array(array,file_path);
                }
        } while (entry = readdir(dir));
}


char *get_extension_name(char *str) {
        char *result;
        char *last;
        if ((last = strrchr(str, '.')) != NULL) {
                if ((*last == '.') && (last == str))
                        return "";
                else {
                        result = (char*) malloc(10);
                        snprintf(result, sizeof result, "%s", last + 1);
                        return result;
                }
        } else {
                return ""; // Empty/NULL string
        }
}
