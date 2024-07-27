#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "wc.h"
#include <string.h>
#include <ctype.h>
#include <math.h>

long map_size = 0;

struct data{
    char* string;
    struct data* next;
    int val;
    /*data(int num, int size){
        val = num;
	string = (char*)malloc(size*sizeof(string));
    }*/
};
struct wc {
    struct data* next;	
/* you can define this struct to have whatever fields you want. */
};
unsigned long
hash(char *str){
    unsigned long hash = 5381;
    int c;
    while ((int)(c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/*int hash(char* string){
	//printf("%d",(*string)%26);
	return abs((*string)%map_size);
}*/
struct wc *
wc_init(char *word_array, long size)
{
	map_size = size/5;
	struct wc *wc;
	wc = (struct wc *)malloc(map_size * sizeof(struct wc));
	memset(wc,0,map_size * sizeof(struct wc));		//v
	assert(wc);
	int head = 0;
	int length = 0;
	int start = 0;
   	for(head = 0 ; head < size + 1; ++head){
		if((!isspace(word_array[head])) && word_array[head] != '\0'){
			length++;
		}else{
			if(start == head){
				start++;
				continue;
			}else{
				//printf("This is true");
				char temp[length+1];
				memset(temp,'\0',length+1);
				strncpy(temp,&word_array[start],length);
				start = head;
				++start;
				unsigned long index = hash(temp)%map_size;
				struct data* curr = wc[index].next;
				struct data* prev = curr;
				int found = 0;
				if(curr == NULL){
					struct data* Node;
					Node = (struct data*) malloc(sizeof(struct data));
					memset(Node,0,sizeof(struct data));
					Node->string = malloc(length+1);
					memset(Node->string,'\0',length+1);
					strcpy(Node->string,temp);
					Node->val = 1;
					wc[index].next = Node;
				}else{
					while(curr != NULL){
						if(strcmp(curr->string,temp) == 0){
							(curr->val)++;
							found = 1;
							break;
						}
						prev = curr;
						curr = curr -> next;
					}
					if(found == 0){
						struct data* Node;
                    				Node = (struct data*) malloc(sizeof(struct data));
                    				Node->string = NULL;
                   				Node -> val = 0;
                   				Node -> next = NULL;
                   				memset(Node,0,sizeof(struct data));
                   				Node->string = (char*)malloc(length+1);
                   				memset(Node->string,'\0',length+1);
						//Node->string =(char*)malloc(size*sizeof(char));
						strcpy(Node->string,temp);
						Node->val = 1;
						prev -> next = Node;
					}
				}
			}
			length = 0;
		}
	}
	return wc;
}

void
wc_output(struct wc *wc)
{
	for(int i = 0 ; i < map_size; i++){
		struct data* temp = wc[i].next;
		while(temp != NULL){
			printf("%s:%d\n", temp->string, temp->val);
			temp = temp -> next;
		}
	}
	
}

void wc_destroy(struct wc *wc)
{
	for(int i = 0 ; i < map_size; i++){
		struct data* node = wc[i].next;
		struct data* temp = node;
		while(node != NULL){
			temp = node -> next;
			free(node);
		/*	node -> next = NULL;
			node -> string = NULL;
			node -> val = 0;*/
			node = temp;
		}
	}
	free(wc);
	wc = NULL;
}
/*
int main() {

	char * test = "aaa aa bbb aa ssss";
	struct wc *wc;
	wc = (struct wc *)malloc(26*sizeof(struct wc));
	memset(wc,0,26*sizeof(struct wc));
	assert(wc);
	wc = wc_init(test,18);
	wc_output(wc);
	wc_destroy(wc);
	return 0;
}
*/
