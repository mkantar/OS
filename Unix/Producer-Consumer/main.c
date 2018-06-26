#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#define TRUE 1
#define FALSE 0
#define AVAILABLE TRUE
#define UNAVAILABLE FALSE
#define BUFFSIZE 10


typedef int _bool;

typedef enum ThreadTypes {
    TypeA, 
    TypeB, 
    TypeC
} ThreadType;

typedef struct Threads {
    pthread_t* tid;
    pthread_attr_t* attr;
    ThreadType type;
    size_t size;
} Thread;

typedef struct Resources {
    int availability;
    int precedence;
} Resource;

Thread* thread_new(ThreadType type, size_t size);
int size(Thread* th);
void delete_thread(Thread* th);
void run(Thread* th, void* (*thread_func) (void*), void* param);
void* thread_job_A(void* param);
void* thread_job_B(void* param);
void* thread_job_C(void* param);
_bool is_prime(int number);

// Global variables

Resource type1[2];
Resource type2;

int main(int argc, char* argv[]) {

    type1[0].availability = AVAILABLE;
    type1[0].precedence = 1;
    type1[1].availability = AVAILABLE;
    type1[1].precedence = 1;

    type2.availability = AVAILABLE;
    type2.precedence = 2;

    Thread* typeA = thread_new(TypeA, 4);
    Thread* typeB = thread_new(TypeB, 1);
    Thread* typeC = thread_new(TypeC, 1);

    run(typeA, thread_job_A, &tParam);
    run(typeB, thread_job_B, &tParam);
    run(typeC, thread_job_C, &tParam);

    sleep(2);

    delete_thread(typeA);
    delete_thread(typeB);
    delete_thread(typeC);

    return EXIT_SUCCESS; 
} 

Thread* thread_new(ThreadType type, size_t size) {
    if( size < 1) {
        fprintf(stderr, "ERROR: Invalid argument value!\n");
        return NULL;
    }
    
    Thread* new_thread = (Thread*) malloc(sizeof(Thread));

    if(new_thread == NULL) {
        fprintf(stderr, "ERROR: Not enough memory!\n");
        return NULL;
    }

    new_thread->tid = (pthread_t*) malloc(sizeof(pthread_t) * size);
    if(new_thread->tid == NULL) {
        fprintf(stderr, "ERROR: Not enough memory!\n");
        return NULL;
    }

    new_thread->attr = (pthread_attr_t*) malloc(sizeof(pthread_attr_t) * size);
    if(new_thread->attr == NULL) {
        fprintf(stderr, "ERROR: Not enough memory!\n");
        return NULL;
    }
    
    new_thread->type = type;
    new_thread->size = size;

    return new_thread;
}

int size(Thread* th) {
    return th->size;
}

void delete_thread(Thread* th) {
	if(th == NULL)
		return;
    free(th->attr);
    free(th->tid);
	free(th);
}

void run(Thread* th, void* (*thread_func)(void*), void* param) {
	for(int i = 0; i < th->size; i++) {
		pthread_attr_init(&th->attr[i]);
		pthread_create(&th->tid[i], &th->attr[i], thread_func, param);
        pthread_attr_destroy(&th->attr[i]);
	}
}

void* thread_job_A(void* param) {
	printf("Hello World B!\n");
	pthread_exit(NULL);
}

void* thread_job_B(void* param) {
 
}

void* thread_job_C(void* param) {
	printf("Hello World C!\n");
	pthread_exit(NULL);
}

_bool is_prime(int n) {
    if (n <= 1)
        return FALSE;
    else if (n <= 3)
        return TRUE;
    else if (((n % 2) == 0) || ((n % 3) == 0))
        return FALSE;
    
    int i = 5;
    while((i * i) <= n) {
        if(((n % i) == 0) || ((n % (i + 2)) == 0))
            return FALSE;
        i += 6;
    }

    return TRUE;
}
