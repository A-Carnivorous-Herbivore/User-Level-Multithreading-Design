#include "request.h"
#include "server_thread.h"
#include "common.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdatomic.h>
struct server {
	int nr_threads;
	int max_requests;
	int max_cache_size;
	int exiting;
	int* buffer;
	int buffSize;
	pthread_t* arr;
	int in;
	int out;
	int threadNumber;
	pthread_mutex_t lock;
	pthread_mutex_t RAMLock;
	pthread_cond_t full;
	pthread_cond_t empty;

	/* add any other parameters you need */
};
struct FIFO_node{		//DDL list
	char* name;
	struct FIFO_node* next;
	struct FIFO_node* prev;
};
struct FIFO_node* FIFO_head = NULL;
struct FIFO_node* FIFO_tail = NULL;
struct data{
    char* string;
    struct data* next;
    char* content;
    int size;
    atomic_int using;
    /*data(int num, int size){
        val = num;
	string = (char*)malloc(size*sizeof(string));
    }*/
};
struct wc {
    struct data* next;	
};
int maxSize = 0;
struct wc* hashtable;
int memoryLeft = 1;
unsigned long
hash(char *str){
    unsigned long hash = 5381;
    int c;
    while ((int)(c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/* static functions */
struct data* cache_insert(struct data* insertion, int index, int size);
struct data* cache_lookup(char* file);
void insertAtEnd(struct data* head, struct data* insertion);
void removeNode(struct FIFO_node* rm);
void releaseMemory();
void FIFO_push_head(struct FIFO_node* insert);
int evict(int size);
/* initialize file data */
static struct file_data *
file_data_init(void)
{
	struct file_data *data;

	data = Malloc(sizeof(struct file_data));
	data->file_name = NULL;
	data->file_buf = NULL;
	data->file_size = 0;
	return data;
}

/* free all file data */
static void
file_data_free(struct file_data *data)
{
	free(data->file_name);
	free(data->file_buf);
	free(data);
}

static void
do_server_request(struct server *sv, int connfd)
{
	int ret;
	struct request *rq;
	struct file_data *data;

	data = file_data_init();

	/* fill data->file_name with name of the file being requested */
	rq = request_init(connfd, data);
	if (!rq) {
		file_data_free(data);
		return;
	}
	/* read file, 
	 * fills data->file_buf with the file contents,
	 * data->file_size with file size. */
	if(sv->max_cache_size<=0){				//No RAM space
		ret = request_readfile(rq);
		if (ret == 0) { /* couldn't read file */
			goto out;
		}
		/* send file to client */
		request_sendfile(rq);
	}else{
		struct data* temp;
		pthread_mutex_lock(&sv->RAMLock);
		temp = cache_lookup(data->file_name);

		if(temp == NULL){
			pthread_mutex_unlock(&sv->RAMLock);		//File is not found in RAM
			ret = request_readfile(rq);
			if(ret == 0){
				goto out;
			}
			pthread_mutex_lock(&sv->RAMLock);
			struct data* insert = (struct data*)malloc(sizeof(struct data));
			//strcpy(insert->string,data->file_name);
			//strcpy(insert->content,data->file_buf);
			insert->string = data->file_name;
			insert->content = data->file_buf;
			insert->size = data->file_size;
			insert->using = 0;
			int index = hash(insert->string)%sv->max_cache_size;
			temp = cache_insert(insert,index,data->file_size);
			if(temp!=NULL){
				temp->using++;
				//struct FIFO_node* node = (struct FIFO_node*)malloc(sizeof(struct FIFO_node));
				//node -> name = temp->string;
				//FIFO_push_head(node);
				//FIFO_ADD();
			}
			//request_set_data(rq,data);
			pthread_mutex_unlock(&sv->RAMLock);
			//request_sendfile(rq);	
			//if(temp!=NULL)temp->using--;
			request_sendfile(rq);
			if(temp!=NULL){
				pthread_mutex_lock(&sv->RAMLock);
				temp->using--;
				pthread_mutex_unlock(&sv->RAMLock);
			}
		}else{
			temp->using++;
			data->file_buf = temp->content;
			data->file_size = temp->size;
			//data->file_name = temp->string;
			//FIFO_ADD();
			//request_set_data(rq,data);
			pthread_mutex_unlock(&sv->RAMLock);

			request_sendfile(rq);

			pthread_mutex_lock(&sv->RAMLock);
			//request_sendfile(rq);
			temp->using--;
			pthread_mutex_unlock(&sv->RAMLock);

			goto out;
		}
	}
out:
	request_destroy(rq);
	//file_data_free(data);
	//temp->using--;
}
void releaseMemory(){
	for(int i = 0 ; i < maxSize; i++){
		struct data* temp = hashtable[i].next;
		struct data* prev;
		while(temp!=NULL){
			prev = temp;
			temp = temp->next;
			free(prev->string);
			free(prev->content);
			free(prev);
		}
	}
	free(hashtable);
}
/* entry point functions */
void stub(struct server* sv){
	//pthread_mutex_lock(&sv->lock);
	//int n = sv->buffSize
	while(1){
		pthread_mutex_lock(&sv->lock);
		while(sv->in == sv->out){
		/*	if(sv->exiting == 1){
				pthread_exit(NULL);
			}*/
			pthread_cond_wait(&sv->empty,&sv->lock);
			if(sv->exiting == 1){
				pthread_mutex_unlock(&sv->lock);
				//pthread_exit(NULL);
				return ;
			}

		}
		int out = sv->buffer[sv->out];
		sv->out = (sv->out+1)%sv->buffSize;
		pthread_cond_signal(&sv->full);
		pthread_mutex_unlock(&sv->lock);
		do_server_request(sv, out);
		//pthread_mutex_unlock(&sv->lock);
	}
	//pthread_mutex_unlock(sv->lock);	
}
struct server *
server_init(int nr_threads, int max_requests, int max_cache_size)
{
	struct server *sv;

	sv = Malloc(sizeof(struct server));
	sv->nr_threads = nr_threads;
	sv->max_requests = max_requests;
	sv->max_cache_size = max_cache_size;
	sv->exiting = 0;
	pthread_cond_init(&sv->full, NULL);
	pthread_cond_init(&sv->empty, NULL);
	pthread_mutex_init(&sv->lock, NULL);
	maxSize = max_cache_size;
	memoryLeft = max_cache_size;
	if (nr_threads > 0 || max_requests > 0 || max_cache_size > 0) {
		if(max_requests > 0){
			sv->buffer = (int*)malloc(sizeof(int) * (max_requests + 1));
			sv->buffSize = max_requests+1;
			sv->in = 0;
			sv->out = 0;
			/*pthread_cond_init(&sv->full, NULL);
			pthread_cond_init(&sv->empty, NULL);
			pthread_mutex_init(&sv->lock, NULL);*/
		}
		if(nr_threads > 0){
			sv->arr = (pthread_t*) malloc(sizeof(pthread_t)*nr_threads);
			sv->threadNumber = nr_threads;
			for(int i = 0 ; i < nr_threads; i++){
				pthread_create(&(sv->arr[i]), NULL, (void*)stub, sv);
				/*int error = pthread_create(&(sv->arr[i]),NULL ,stub ,sv);
				if(error < 0){
					exit(1);
				}*/
			}
		}	
		if(max_cache_size > 0){
			hashtable = (struct wc*)malloc(sizeof(struct wc)*max_cache_size);
			memoryLeft = max_cache_size;
		}
	}
	

	/* Lab 4: create queue of max_request size when max_requests > 0 */

	/* Lab 5: init server cache and limit its size to max_cache_size */

	/* Lab 4: create worker threads when nr_threads > 0 */

	return sv;
}
void FIFO_push_head(struct FIFO_node* insert){
	insert->next = FIFO_head;
	insert->prev = NULL;
	if(!FIFO_tail)	FIFO_tail = insert;	//Initial case list empty
	else FIFO_head->prev = insert;
	FIFO_head = insert;
}
struct FIFO_node* FIFO_pop_tail(){
	if(FIFO_tail == NULL) return NULL;
	struct FIFO_node* temp;
	if(FIFO_tail -> prev == NULL){
		FIFO_head = NULL;
		temp = FIFO_tail;
		FIFO_tail = NULL;
		return temp;
	}else{
		temp = FIFO_tail;
		FIFO_tail = FIFO_tail -> prev;
		return temp;
	}
}
/*struct FIFO_node* FIFO_remove(struct FIFO_node* remove){
	
}*/
struct FIFO_node* FIFO_find(char* filename){
	struct FIFO_node* h = FIFO_head;
	while(h != NULL){
		if(!strcmp(h->name, filename)){
			return h;	
		}
		h = h->next;	
	}
	return NULL;
}
struct data* cache_remove(char* filename){
	int index = hash(filename)%maxSize;
	struct data* temp = hashtable[index].next;
	struct data* prev = NULL;
	while(temp!=NULL){
		if(!strcmp(filename,temp->string)){
			break;
		}
		prev = temp;
		temp = temp->next;
	}
	if(temp == NULL){	//NOT FOUND IN CACHE
		return NULL;
	}else if(prev ==NULL){
		memoryLeft += temp->size;
		hashtable[index].next = NULL;
		free(temp);
	}else{
		memoryLeft += temp->size;
		prev->next = temp->next;	//SKIP THE FOUND NODE
		free(temp);
	}
	return temp;
}
struct data* cache_lookup(char* file){			//DO NOT UPDATE FIFO
	int index = hash(file) % maxSize;
	struct data* temp = hashtable[index].next;
	//int found = 0;
	while(temp!=NULL){
		if(!strcmp(temp->string,file)){
		//	found = 1;
		//	break;
			return temp;
		}
		temp = temp->next;	
	}
	/*if(found){
		return temp;
	}else{
		return NULL;				//Did not find it
	}*/
	return NULL;
}
struct data* cache_insert(struct data* insertion, int index, int size){
	//int index = hash(insertion)%maxSize;
	if(size>maxSize) return NULL;
	//while(memoryLeft < size){
		//cache_evit();
	//}
	struct data* find = cache_lookup(insertion->string);
	if(find != NULL){					//if already exist in cache
		return NULL;
	}							//Has not been stored
	int evi = 0;
	if(memoryLeft < size){
		 evi = evict(size);
	}
	if(evi != 0){			//Not enough space
		return NULL;
	}
	struct data* temp = hashtable[index].next;
	if(temp == NULL){
		hashtable[index].next = insertion;
		insertion->next = NULL;
	}else{
		insertAtEnd(temp, insertion);
	}
	memoryLeft -= size;					
	
	//Update FIFO list
	
	struct FIFO_node* add = (struct FIFO_node*) malloc(sizeof(struct FIFO_node));
	//strcpy(add->name,insertion->string);
	add->name = insertion->string;
	add->prev = NULL;
	FIFO_push_head(add);

	return insertion;
	
}

void insertAtEnd(struct data* head, struct data* insertion){
	while(head->next != NULL){
		head = head -> next;
	}
	head->next = insertion;
	insertion->next = NULL;
}

int evict(int size){
	struct FIFO_node* temp = FIFO_tail;
	if(FIFO_tail == NULL){
		return 2;
	}
	//int indicator = 0;
	while(memoryLeft < size && temp != NULL){
		
		struct data* cacheRemove = cache_lookup(temp->name);
		if(cacheRemove == NULL){	//Not found in cache, should not be the case...
			return -1;
		}
		if(cacheRemove->using != 0){		//In use, move forward
			temp = temp->prev;
			//indicator = 0;
		}else{
			struct FIFO_node* remove = temp;
			temp = temp->prev;
			char* name = remove -> name;
			removeNode(remove);
			cache_remove(name);
			//free(name);
		}
	}
	if(memoryLeft < size){
		return 1;
	}else{
		return 0;
	}

}
void  removeNode(struct FIFO_node* rm){
	struct FIFO_node* temp = rm;
	/*if(FIFO_head == rm){
		FIFO_head = rm->next;
	}
	if(FIFO_tail == rm){
		FIFO_tail = rm->prev;
	}*/
	if(rm->next != NULL && rm->prev != NULL){
		rm->next->prev = rm->prev;
		rm->prev->next = rm->next;
		free(temp);
		return ;
	}else if(rm->next != NULL && rm->prev == NULL){
		FIFO_head = rm->next;
		rm->next->prev = NULL;
		free(temp);
		return ;
	}else if(rm->next == NULL && rm->prev !=NULL){
		FIFO_tail = rm->prev;
		rm->prev->next = NULL;
		free(temp);
		return ;
	}else{
		FIFO_tail = NULL;
		FIFO_head = NULL;
		free(temp);
		return ;
	}
	/*if(rm->prev != NULL){
		rm->prev->next = rm->next;
	}*/
}
void
server_request(struct server *sv, int connfd)
{
	if (sv->nr_threads == 0) { /* no worker threads */
		do_server_request(sv, connfd);
	} else {
		/*  Save the relevant info in a buffer and have one of the
		 *  worker threads do the work. */
		pthread_mutex_lock(&sv->lock);
		while((sv->in-sv->out+sv->buffSize)%sv->buffSize == sv->buffSize - 1){
			pthread_cond_wait(&sv->full,&sv->lock);
		}
		sv->buffer[sv->in] = connfd;
		sv->in = (sv->in + 1)%sv->buffSize;
		pthread_cond_signal(&sv->empty);
		pthread_mutex_unlock(&sv->lock);

	}
}

void
server_exit(struct server *sv)
{
	/* when using one or more worker threads, use sv->exiting to indicate to
	 * these threads that the server is exiting. make sure to call
	 * pthread_join in this function so that the main server thread waits
	 * for all the worker threads to exit before exiting. */
	sv->exiting = 1;
	//printf("HI");
	pthread_cond_broadcast(&sv->full);
	pthread_cond_broadcast(&sv->empty);
	for(int i = 0; i< sv->threadNumber; i++){
		//printf("HI");
		pthread_join(sv->arr[i], NULL);
		/*pthread_cond_broadcast(&sv->full);
		pthread_cond_broadcast(&sv->empty);
		*/
	}
	/*pthread_cond_broadcast(&sv->full);
	pthread_cond_broadcast(&sv->empty);*/
	/* make sure to free any allocated resources */
	free(sv->buffer);
	free(sv->arr);
	free(sv);
}
