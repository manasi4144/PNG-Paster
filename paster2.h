#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <curl/curl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>

#define U8 unsigned char
#define U32 unsigned int

#define ECE252_HEADER "X-Ece252-Fragment: "
#define BUF_SIZE 10240 /* 1024*10 = 10K */
#define IMG_URL "http://ece252-1.uwaterloo.ca:2530/image?img=1&part=20"
#define IMG_URL_BASE "http://ece252-1.uwaterloo.ca:2530/image?"
#define URL_LEN 256
#define TOTAL_PART 50
#define MAX_CONSUMER 100
#define SEM_PROC 1

#define CHK_EXIT(cond)    \
	if ((cond)) {      \
		perror(#cond); \
		printf("abort\n");\
	abort();       \
	}                  \

#define CREATE_SHM(id, key, size, flag)   \
	id = shmget(key, size, flag); \
	if (id == -1) {               \
		perror("shmget");         \
		abort();                  \
	}                             \

#define CONNECT_SHM(p, id, addr, flag) \
	p = shmat(id, addr, flag); \
	if (p == (void *)-1) {     \
		perror("shmat");       \
		abort();               \
	}                          \

#define DETACH_SHM(p)        \
    if (shmdt(p) != 0) { \
        perror("shmdt"); \
        abort();         \
    }

#define INIT_SEM(p, value)                        \
    if (sem_init(p, SEM_PROC, value) != 0) {     \
        perror("sem_init [" #p "][" #value "]"); \
        abort();                                 \
    }

size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata);
size_t write_cb_curl(char *p_recv, size_t size, size_t nmemb, void *p_userdata);
int write_file(const char *path, const void *in, size_t len);
//int catpng();
int producer(int id);
int consumer(int id);
int gen_url(char *res_url, int part);

typedef struct Buffer {
    unsigned char buf[BUF_SIZE];
    unsigned int size;
    int seq;
} Buffer;

int buffer_init(Buffer *p_buf);

//Dequeue for managing shared memory regions
typedef struct dq {
    int capacity;
    int size;
    int head, tail;  
    Buffer *items;
} dq;

extern int no_producers, no_consumers, queue_size, random_wait, pic_number; //argv variables
// id of shared memory variables
extern int curr_downloaded;
extern int sem_get_task, sem_need_consume, sem_push, sem_pop, sem_exists, sem_slots;

extern int queue;
extern int buf_all;

// shared memory variables
extern int *p_now_downloaded;
extern sem_t *p_sem_get_task, *p_sem_need_consume, *p_sem_push, *p_sem_pop, *p_sem_exists, *p_sem_slots;

extern  dq *p_queue;
extern Buffer *p_buf_all;

int sizeof_shm_dq(int queue_size);
int update_shm_dq(dq *q);
int init_shm_dq(dq *q, int queue_size);

int dq_push_back(dq *q, Buffer *item);
int dq_pop_front(dq *q, Buffer *item);
// Deque end

int catpng();