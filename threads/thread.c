#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"
#include <stdio.h>
#include <string.h>
/* This is the wait queue structure */
struct wait_queue {
	struct thread* wait;
	/* ... Fill this in Lab 3 ... */
};
enum{
	status_run = -1,
	status_ready = -2,
	status_exit = -3,
	status_sleep = -4,
};
/* This is the thread control block */
struct thread* readyList; //Header of the thread 
struct thread* current; //Current Running Thread
struct thread* exitList; //Header of exit list
struct wait_queue* waitArray[THREAD_MAX_THREADS];
struct thread {
	ucontext_t context;
	int id;
	//char status[10];
	int status;
	long* stack;
	struct thread* next;
	/* ... Fill this in ... */
};
int threadArray[THREAD_MAX_THREADS];
int aaa = 0;
/*void releaseWait(struct wait_queue* w){
	struct thread* head = w->wait;
	struct thread* prev = NULL;
	while(temp!=NULL){
		prev = head->next;
		
	}
	free(w);
}*/
void
thread_init(void)
{	
	int enable = interrupts_set(0);
	struct thread* initThread = (struct thread*)malloc(sizeof(struct thread));
	memset(initThread,0,sizeof(struct thread));
	initThread -> id = 0;
	initThread -> status = status_run;
	current = initThread;
	waitArray[0] = wait_queue_create();
	threadArray[0] = 1;
	for (int i = 1; i < THREAD_MAX_THREADS; i++){
		threadArray[i] = 0;
	}
	interrupts_set(enable);
	/* your optional code here */
}

Tid
thread_id()
{
	int enable = interrupts_set(0);
	if (current != NULL){
		interrupts_set(enable);
		return current -> id;
	}
	interrupts_set(enable);
	return THREAD_INVALID;
}
void thread_stub(void(*thread_main)(void*), void* arg){
	//Tid ret;
	interrupts_set(1);
	thread_main(arg);
	
	thread_exit();
}

Tid
thread_create(void (*fn) (void *), void *parg)
{
	int enable = interrupts_set(0);

	struct thread* tempExit = NULL;
	//struct thread* currentExit = NULL;
	while(exitList != NULL){
		tempExit = exitList;
		exitList = exitList -> next;
		threadArray[tempExit->id] = 0;
		wait_queue_destroy(waitArray[tempExit->id]);
		free(tempExit->stack);
		free(tempExit);
	}
	struct thread* newThread = (struct thread*) malloc(sizeof(struct thread));
	memset(newThread,0,sizeof(struct thread));
	if(newThread == NULL){
		interrupts_set(enable);
		return THREAD_NOMEMORY;
	}
	newThread -> status = status_ready;
	int empty = 0;
	for(; empty < THREAD_MAX_THREADS; empty++){
		if(threadArray[empty] == 0)
			break;
	}
	if(empty == THREAD_MAX_THREADS){
		interrupts_set(enable);
		return THREAD_NOMORE;
	}
	threadArray[empty] = 1;
	newThread->stack = malloc(THREAD_MIN_STACK + sizeof(struct thread));
	if(newThread -> stack == NULL){
		interrupts_set(enable);
		return THREAD_NOMEMORY;
	}
	newThread->id = empty;
	waitArray[newThread->id] = wait_queue_create();
	int get = getcontext(&(newThread->context));
	assert(!get);
	struct thread* head = readyList;
	struct thread* prev;
	if(head == NULL){
		readyList = newThread;
	}else{
		while(head!= NULL){
			prev = head;
			head = head -> next;
		}
		prev -> next = newThread;
	//	head -> next = newThread;
	}
	long rsp =(long long)(newThread->stack + THREAD_MIN_STACK/sizeof(long));

	newThread->context.uc_stack.ss_sp = newThread->stack;
//	long rsp =(long long)(newThread->stack + THREAD_MIN_STACK - 8);
	newThread->context.uc_mcontext.gregs[REG_RDI] = (long long)(fn);
	newThread->context.uc_mcontext.gregs[REG_RSI] = (long long)(parg);
	newThread->context.uc_mcontext.gregs[REG_RIP] = (long long)(&thread_stub);
	newThread->context.uc_mcontext.gregs[REG_RSP] = rsp+24-rsp%16;

	interrupts_set(enable);
//	newThread->context.uc_mcontext.gregs[REG_RSP] = rsp;
	return newThread->id;	
	return THREAD_FAILED;

}
/*void thread_stub(void(*thread_main)(void*), void* arg){
	Tid ret;
	thread_main(arg);
	thread_exit();
}*/
int changeCurrentToTarget(int thread_id){	//Yield function helper	
		//int found = 0;		//Indicator
		//int enable = interrupts_set(0);
		++aaa;
		struct thread* temp = readyList;
		struct thread* prev = NULL;
		if(temp != NULL && temp->id == thread_id){
			readyList = readyList->next;
			temp -> next = NULL;
		}else{
			while(temp != NULL){
				if(temp->id == thread_id){
					if(prev == NULL){
						readyList = temp -> next;
						temp -> next = NULL;
						break;
					}else{
						prev->next = temp->next;	//Find want_tid thread, take it out.
						temp -> next = NULL;
						break;
					}
				}
			prev = temp;
			temp = temp->next;
			}
		}
		//temp -> next = NULL;
		if(temp == NULL){	//Didnt find thread_id in ready queue
		//	interrupts_set(enable);
			return -1;
		}
		struct thread* temp2 = readyList;	//Move current to the tail of the readyList
		if( temp2 == NULL){
			readyList = current;
		}else{
			while(temp2->next != NULL){
				temp2 = temp2 -> next;		//Find tail ready
			}
			temp2 -> next = current;		//Set current at the end of the ready 
		}
		current->next = NULL;
		volatile int count = 0;
		current -> status = status_ready;
		temp2 = current;
		current = temp;				//Change current to want_tid thread
		temp -> status = status_run;

		int get = getcontext(&(temp2->context));	//Save context of current
		struct thread* tempExit = NULL;
		while(exitList != NULL){
			tempExit = exitList;
			exitList = exitList -> next;
			threadArray[tempExit->id] = 0;
			wait_queue_destroy(waitArray[tempExit->id]);
			free(tempExit->stack);
			free(tempExit);
		}

		//current = temp;				//Change current to want_tid thread
		//temp -> status = status_run;

		assert(!get);
		if(count == 0){
			count++;
			int set = setcontext(&(temp->context));
			assert(!set);
		}
		//interrupts_set(enable);
		return 0;

}
Tid
thread_yield(Tid want_tid)
{
	int enable = interrupts_set(0);
	//Tid cur_tid = thread_id();
	if(want_tid == THREAD_ANY){
		if(readyList == NULL){
			interrupts_set(enable);
			return THREAD_NONE;
		}
		int value = readyList -> id;
		int temp = changeCurrentToTarget(value);
		if(temp == 0){		
			interrupts_set(enable);
			return value;
		}	
	}else if(want_tid == THREAD_SELF){
		//changeCurrentToTarget(current);
		interrupts_set(enable);
		return current->id;
	}else if(want_tid < 0 || want_tid >= THREAD_MAX_THREADS){
		interrupts_set(enable);
		return THREAD_INVALID;
	}else{
		if (threadArray[want_tid] == 0){
			interrupts_set(enable);
			return THREAD_INVALID;
		}
		/*if(want_tid == current->id){
			return current->id;
		}
		struct thread* temp = readyList;  //Find if want_tid is in ready state
		struct thread* prev;
		int found = 0; 			 //Indicator
		while(temp != NULL){
			if(temp -> id == want_tid){
				found = 1;
				prev->next = temp->next;	//Find want_tid thread, take it out.
				break;
			}
			prev = temp;
			temp = temp->next;
		}
		if (found == 0){				//If not found
			return THREAD_INVALID;
		}
		
		struct thread* temp2 = readyList;	//Move current to the tail of the readyList
		if( temp2 == NULL){
			temp2 = current;
		}
		while(temp2->next != NULL){
			temp2 = temp2 -> next;		//Find tail ready
		}
		temp2 -> next = current;		//Set current at the end of the ready 
		current -> status = status_ready;
		int get = getcontext(&(current->context));	//Save context of current
		assert(!get);

		current = temp;				//Change current to want_tid thread
		temp -> status = status_run;
		int set = setcontext(&(temp->context));
		assert(!set);
		return want_tid;*/
		if(want_tid == current->id){
			interrupts_set(enable);
			return current->id;
		}
		int temp = changeCurrentToTarget(want_tid);
		if(temp != 0){
			interrupts_set(enable);
			return THREAD_INVALID;
		}
		interrupts_set(enable);
		/*	struct thread* temp = readyList;
	struct thread* prev = NULL;
	while(temp != NULL){
		if(temp -> id == tid){
			break;
		}
		prev = temp;
		temp = temp -> next;
	}*/


		return want_tid;
	}
	interrupts_set(enable);
	//Tid cur_tid = thread_id();

	return THREAD_FAILED;
}

void
thread_exit()
{
	int enable = interrupts_set(0);
	struct thread* head = exitList;
	if(head == NULL){
		exitList = current;
	}else{
		while(head->next != NULL){
			head = head -> next;
		}
		head -> next = current;
	}
	current -> status = status_exit;
	//threadArray[current->id] = 0;
	if(readyList == NULL)
		exit(0);
	else{
		struct thread* temp = readyList;
		current = readyList;
		temp -> status = status_run;
		readyList = readyList -> next;
		temp -> next = NULL;
		int set = setcontext(&(temp->context));
		assert(!set);
	}
	interrupts_set(enable);

} 

Tid
thread_kill(Tid tid)
{
	int enable = interrupts_set(0);
	struct thread* temp = readyList;
	struct thread* prev = NULL;
	while(temp != NULL){
		if(temp -> id == tid){
			break;
		}
		prev = temp;
		temp = temp -> next;
	}

	if (!temp){
		interrupts_set(enable);
		return THREAD_INVALID;
	}
	else if(prev == NULL){
		readyList = readyList -> next;
		temp -> next = NULL;	
	}else{
		prev -> next = temp -> next;
		temp -> next = NULL;
	}
	threadArray[tid] = 0; 
	free(temp -> stack);
	wait_queue_destroy(waitArray[temp->id]);
	free(temp);
	interrupts_set(enable);
	return tid;
	//return THREAD_FAILED;
}

/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* make sure to fill the wait_queue structure defined above */
int changeCurrentToTarget2(int thread_id){	//Yield function helper	
		//int found = 0;		//Indicator
		//int enable = interrupts_set(0);
		++aaa;
		struct thread* temp = readyList;
		struct thread* prev = NULL;
		if(temp != NULL && temp->id == thread_id){
			readyList = readyList->next;
			temp -> next = NULL;
		}else{
			while(temp != NULL){
				if(temp->id == thread_id){
					if(prev == NULL){
						readyList = temp -> next;
						temp -> next = NULL;
						break;
					}else{
						prev->next = temp->next;	//Find want_tid thread, take it out.
						temp -> next = NULL;
						break;
					}
				}
			prev = temp;
			temp = temp->next;
			}
		}
		//temp -> next = NULL;
		if(temp == NULL){	//Didnt find thread_id in ready queue
		//	interrupts_set(enable);
			return -1;
		}
		struct thread* temp2 = readyList;	//Move current to the tail of the readyList
		if( temp2 == NULL){
			readyList = current;
		}else{
			while(temp2->next != NULL){
				temp2 = temp2 -> next;		//Find tail ready
			}
			temp2 -> next = current;		//Set current at the end of the ready 
		}
		current->next = NULL;
		volatile int count = 0;
		current -> status = status_ready;
		temp2 = current;
		current = temp;				//Change current to want_tid thread
		temp -> status = status_run;

		int get = getcontext(&(temp2->context));	//Save context of current
		//current = temp;				//Change current to want_tid thread
		//temp -> status = status_run;
		struct thread* tempExit = NULL;
		assert(!get);

		while(exitList != NULL){
			tempExit = exitList;
			exitList = exitList -> next;
			threadArray[tempExit->id] = 0;
			//releaseWait(waitArray[tempExit->id]);
			wait_queue_destroy(waitArray[tempExit->id]);
			free(tempExit->stack);
			free(tempExit);
		}

		//assert(!get);
		if(count == 0){
			count++;
			int set = setcontext(&(temp->context));
			assert(!set);
		}
		//interrupts_set(enable);
		return 0;

}

Tid
thread_yield_wait(/*Tid want_tid,*/ struct wait_queue* q)
{
	
	//int enable = interrupts_set(1);
	interrupts_off();
	int id;
	struct thread* head = q->wait;
	struct thread* prev = NULL;
	if(head==NULL){			//if queue is empty
		q->wait = current;
		current->status = status_sleep;
		current->next =NULL;
	}else{
		while(head!=NULL){		//find the last node and store in prev
			prev = head;
			head = head -> next;
		}
		prev->next = current;
		current->status = status_sleep;
		current->next = NULL;
	}
	//getcontext(&current->context);
	volatile int try = 0;
	getcontext(&current->context);
	
	struct thread* tempExit = NULL;
	while(exitList != NULL){
		tempExit = exitList;
		exitList = exitList -> next;
		threadArray[tempExit->id] = 0;
		//releaseWait(waitArray[tempExit->id]);
		wait_queue_destroy(waitArray[tempExit->id]);
		free(tempExit->stack);
		free(tempExit);
	}

	if(try == 0){
		try = 1;
		current = readyList;
		readyList = readyList -> next;
		current->next = NULL;
		id = current->id;
		int set = setcontext(&current->context);
		assert(!set);
		
	}
	interrupts_on();
	return id;
}


struct wait_queue *
wait_queue_create()
{
	struct wait_queue *wq;

	wq = malloc(sizeof(struct wait_queue));
	assert(wq);
	wq->wait = NULL;//(struct thread*)malloc(sizeof(struct thread));

	return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{
	struct thread* temp = wq->wait;
	struct thread* next = NULL;
	while(temp!=NULL){
		next = temp->next;
		free(temp->stack);
		free(temp);
		temp = next;	
	}
	//free(wq->wait);
	free(wq);
}

	Tid
thread_sleep(struct wait_queue *queue)
{	
	//printf(" ");
	int enable = interrupts_set(0);
	if(queue == NULL){
		//printf("1111");
		interrupts_set(enable);
		return THREAD_INVALID;
	}
	if(!readyList){
		//printf("2222");
		interrupts_set(enable);
		return THREAD_NONE;
	}
	interrupts_set(enable);
	return thread_yield_wait(queue);
}
/*	struct thread* temp = readyList;
	//printf("before sleep, current thread = %d\n", current->id);
	struct thread* head = queue->wait;
	struct thread* prev = NULL;
	if(head == NULL){			//Find end of wait queue
		current->next = NULL;
		current -> status = status_sleep;
		queue->wait = current;
interrupts_set(enable);
	return thread_yield_wait(queue);


	}else{
		while( head != NULL){	
			prev = head;
			head = head -> next;
		}
		current->next = NULL;
		current -> status = status_sleep;
		prev->next = current;
	}
	int get = getcontext(&(temp->context));
	//struct thread* temp = current;
	//int get = getcontext(&(temp->context)); 

	
	current = readyList;
	printf("thread is now %d after sleep\n", current->id);
	//struct thread* temp = current;
	current -> status = status_run;	
	readyList = readyList -> next;
	current-> next = NULL;

	volatile int try = 0;
	//interrupts_set(enable);
	assert(!get);
	if(try == 0 ){
		try++;
		int set = setcontext(&(temp->context));
		assert(!set);
	}
	//printf("4444");
	interrupts_set(enable);
	return current->id;
	return THREAD_FAILED;
	interrupts_set(enable);
	return thread_yield_wait(queue);
}*/

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
	//printf("ENTER WAKEUP FUNCTION\n");
	int enable = interrupts_set(0);
	if(!queue || !(queue->wait)){	//Queue Empty
		//printf("BBBB");
		interrupts_set(enable);
		return 0;
	}
	
	if(all){		// All wake up
		//printf("CCCC");
		struct thread* head = readyList;
		struct thread* prev = NULL;
		if(head == NULL){		
			readyList = queue->wait;
			struct thread* temp = readyList;	//Find queue first 
			int count = 0;
			while(temp!=NULL){			//Traverse queue and count 
				count++;
				temp->status = status_ready;
				//printf("Thread %d is waken up", temp->id);
				temp = temp->next;
			}
			queue->wait = NULL;			//Queue is now empty
			interrupts_set(enable); 
			return count;
		}else{
			while(head != NULL){
				prev = head;
				head = head->next;
			}
			prev->next = queue->wait;
			struct thread* temp = queue->wait;
			int count = 0;
			while(temp != NULL){
				count++;
				temp->status = status_ready;
				//printf("Thread %d is waken up", temp->id);
				temp = temp->next;
			}
			queue->wait = NULL;
			interrupts_set(enable);
			return count;
		}
	}else{
		//printf("DDDD");
		struct thread* head = readyList;
		struct thread* prev = NULL;
		if(head == NULL){
			//printf("Condition 1 \n");
			queue->wait->status = status_ready;
			readyList = queue->wait;
			queue->wait = queue->wait->next;
			readyList->next = NULL;
			interrupts_set(enable);
			//printf("Thread %d is waken up", readyList->id);
			return 1;
		}else{
			while(head != NULL){
				prev = head;
				head = head->next;
			}
			queue->wait->status = status_ready;
			prev->next = queue->wait;
			queue->wait = queue->wait->next;
			prev->next->next = NULL;
			//printf("Thread %d is waken up", prev->next->id);
			interrupts_set(enable);
			return 1;
		}

	}
	interrupts_set(enable);	
	return 0;
}
/*int
thread_wakeup(struct wait_queue *queue, int all)
{
	int enable = interrupts_set(0);
	if(queue == NULL || queue->wait == NULL){
		interrupts_set(enable);
		return 0;
	}
	//struct thread* head = queue->wait;
	///struct thread* temp = NULL;
	if(all){
		int count = 0;
		while(thread_wakeup(queue,0) == 1){
			count++;
		}
		interrupts_set(0);
		return count;
	}else{//printf("DDDD");
		struct thread* head = readyList;
		struct thread* prev = NULL;
		if(head == NULL){
			//printf("Condition 1 \n");
			queue->wait->status = status_ready;
			readyList = queue->wait;
			queue->wait = queue->wait->next;
			readyList->next = NULL;
			interrupts_set(enable);
			//printf("Thread %d is waken up", readyList->id);
			return 1;
		}else{
			while(head != NULL){
				prev = head;
				head = head->next;
			}
			queue->wait->status = status_ready;
			prev->next = queue->wait;
			queue->wait = queue->wait->next;
			prev->next->next = NULL;
			//printf("Thread %d is waken up", prev->next->id);
			interrupts_set(enable);
			return 1;
		}


	//	temp = head->next;

	}
	interrupts_set(enable);
	return 0;
}*/

/*{
	int enable = interrupts_set(0);
	int count = 0;
	if(!queue || queue->wait == NULL){
		interrupts_set(enable);
		return 0;
	}
	if(all){
		struct thread* head = readyList;
		struct thread* prev = NULL;
		if(readyList == NULL){
			readyList = queue->next;
			
		}
	}else{
	
	}

}*/
/* suspend current thread until Thread tid exits */
Tid
thread_wait(Tid tid)
{

	int enable = interrupts_set(0);
	if(tid < 0|| tid == current->id){
		interrupts_set(enable);
		return THREAD_INVALID;
	}
	int indicator = thread_sleep(waitArray[tid]);
	if(indicator < 0){
		interrupts_set(enable);
		return THREAD_INVALID;
	}
	interrupts_set(enable);
/*	struct thread* tempExit = NULL;
	while(exitList != NULL){
			tempExit = exitList;
			exitList = exitList -> next;
			threadArray[tempExit->id] = 0;
			free(tempExit->stack);
			free(tempExit);
	}*/

	return 0;
}

struct lock {

	/* ... Fill this in ... */
};

struct lock *
lock_create()
{
	struct lock *lock;

	lock = malloc(sizeof(struct lock));
	assert(lock);

	TBD();

	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

	TBD();

	free(lock);
}

void
lock_acquire(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

void
lock_release(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

struct cv {
	/* ... Fill this in ... */
};

struct cv *
cv_create()
{
	struct cv *cv;

	cv = malloc(sizeof(struct cv));
	assert(cv);

	TBD();

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	TBD();

	free(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}
