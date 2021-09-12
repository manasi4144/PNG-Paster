#include "paster2.h"

// global variables
int no_producers, no_consumers, queue_size, random_wait, pic_number; //argv variables

// id of shared memory variables
int curr_downloaded;
int sem_get_task, sem_need_consume, sem_push, sem_pop, sem_exists, sem_slots;
int queue;
int buf_all;

// shared memory variables
int *p_now_downloaded;
sem_t *p_sem_get_task, *p_sem_need_consume, *p_sem_push, *p_sem_pop, *p_sem_exists, *p_sem_slots;
dq *p_queue;
Buffer *p_buf_all;
pid_t cpid = 0;

unsigned int flag = IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR;

/*Extra Dequeue Functions*/
int sizeof_shm_dq(int queue_size)
{
    int total_size = sizeof(dq) + sizeof(Buffer) * queue_size;
	//printf("total_size of sizeof_shm_dq(): %d\n", total_size);
	return total_size;
}

int update_shm_dq(dq *q) {
    q->items = (Buffer *)((char *)q + sizeof(dq));
    return 0;
}
int init_shm_dq(dq *q, int queue_size)
{
    if (q == NULL || queue_size == 0)
	{
        return 1;
    }
    q->capacity = queue_size;
    q->size = 0;
    q->head = 0;
    q->tail = 0;
    update_shm_dq(q);
    return 0;
}

int dq_push_back(dq *q, Buffer *item)
{
    CHK_EXIT(q == NULL);
    q->items[q->tail++] = *item;
    if (q->tail >= q->capacity)
	{
		q->tail = q->tail - q->capacity;
	}
    return 0;
}
int dq_pop_front(dq *q, Buffer *item)
{
    CHK_EXIT(q == NULL);
    *item = q->items[q->head++];
    if (q->head >= q->capacity)
	{
		q->head = q->head - q->capacity;
	}
    return 0;
}
/*End of Dequeue Functions*/

//create semaphores
void create_shm() {
    CREATE_SHM(curr_downloaded, IPC_PRIVATE, sizeof(int),flag);
    CREATE_SHM(sem_get_task, IPC_PRIVATE, sizeof(sem_t), flag);
    CREATE_SHM(sem_need_consume, IPC_PRIVATE, sizeof(sem_t),flag);
    CREATE_SHM(sem_push, IPC_PRIVATE, sizeof(sem_t), flag);
    CREATE_SHM(sem_pop, IPC_PRIVATE, sizeof(sem_t), flag);
    CREATE_SHM(sem_exists, IPC_PRIVATE, sizeof(sem_t), flag);
    CREATE_SHM(sem_slots, IPC_PRIVATE, sizeof(sem_t), flag);
    CREATE_SHM(queue, IPC_PRIVATE, sizeof_shm_dq(queue_size),flag);
    CREATE_SHM(buf_all, IPC_PRIVATE, sizeof(Buffer) * TOTAL_PART,flag);
}

//connect to shared memory regions
void connect_shm() {
    CONNECT_SHM(p_now_downloaded, curr_downloaded, NULL, 0);
    CONNECT_SHM(p_sem_get_task, sem_get_task, NULL, 0);
    CONNECT_SHM(p_sem_need_consume, sem_need_consume, NULL, 0);
    CONNECT_SHM(p_sem_push, sem_push, NULL, 0);
    CONNECT_SHM(p_sem_pop, sem_pop, NULL, 0);
    CONNECT_SHM(p_sem_exists, sem_exists, NULL, 0);
    CONNECT_SHM(p_sem_slots, sem_slots, NULL, 0);
    CONNECT_SHM(p_queue, queue, NULL, 0);
    // CONNECT(p_buf_all, si_buf_all, NULL, 0);

    // init shm variables
    *p_now_downloaded = 0;
    INIT_SEM(p_sem_get_task, 1);
    INIT_SEM(p_sem_need_consume, TOTAL_PART);
    INIT_SEM(p_sem_push, 1);
    INIT_SEM(p_sem_pop, 1);
    INIT_SEM(p_sem_exists, 0);
    INIT_SEM(p_sem_slots, queue_size);
    init_shm_dq(p_queue, queue_size);

    // DETACH
    DETACH_SHM(p_now_downloaded);
    DETACH_SHM(p_sem_get_task);
    DETACH_SHM(p_sem_need_consume);
    DETACH_SHM(p_sem_push);
    DETACH_SHM(p_sem_pop);
    DETACH_SHM(p_sem_exists);
    DETACH_SHM(p_sem_slots);
    DETACH_SHM(p_queue);
}

//create producer processes
int create_producers()
{
    int i;
    for (i = 0; i < no_producers; i++)
	{
        cpid = fork();
        if (cpid > 0)
		{
			/*Parent process*/
        }
		else if (cpid == 0)
		{
            /*Child process*/
			producer(i);
            return 0;
        }
		else
		{
			/*Fork failed*/
			perror("fork producer");
            return -1;
        }
    }
    return cpid;
}

//PID array for consumers
pid_t cons_pids[MAX_CONSUMER];

//create consumer processes
int create_consumers()
{
	int i;
    for (i = 0; i < no_consumers; i++) {
        cpid = fork();
        if (cpid > 0)
		{
            /*Parent process*/
			cons_pids[i] = cpid;
        }
		else if (cpid == 0)
		{
            /*Child process*/
			consumer(i);
            return 0;
        }
		else
		{
            /*Fork failed*/
			perror("fork consumer");
            return -1;
        }
    }
    return cpid;
}

//initialize buffer
int buffer_init(Buffer *p_buf) 
{
	if ((p_buf == NULL))
	{
		perror("p_buf == NULL)"); 
		printf("abort\n");
		abort();       
	}
    p_buf->size = 0;
    p_buf->seq = -1;
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 6)
	{
        queue_size = atoi(argv[1]);
        no_producers = atoi(argv[2]);
        no_consumers = atoi(argv[3]);
        random_wait = atoi(argv[4]);
        pic_number = atoi(argv[5]);
    }
	else
	{
        printf("invalid parameter\n");
        return 0;
    }
	
	double times[2];
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        perror("gettimeofday");
        abort();
    }
    times[0] = (tv.tv_sec) + tv.tv_usec / 1000000.;
	
	create_shm();
    connect_shm();
	
	//printf("After create_shm() and connect_shm()\n");
  
    curl_global_init(CURL_GLOBAL_ALL);
    int ret = create_producers();
    if (ret <= 0)
	{
		return ret;
	}
	
	//printf("After create_producers()\n");

    int ret2 = create_consumers();
    if (ret2 <= 0)
	{
		return ret2;
	}
	
	//printf("After create_consumers()\n");

    
	// now in parent process
    int state;
    for (int i = 0; i < no_consumers; i++)
	{
		// wait consumers end
        waitpid(cons_pids[i], &state, 0);
        if (WIFEXITED(state))
		{
            //printf("Consumer cpid[%d]=%d terminated with state: %d.\n", i, cons_pids[i], state);
        }
    }
	
	//Code for timing stuff and catpng
	CONNECT_SHM(p_buf_all, buf_all, NULL, 0);
    unsigned char **bufs = (unsigned char **)malloc(sizeof(char *) * TOTAL_PART);
    unsigned int *lens = (unsigned int *)malloc(sizeof(int) * TOTAL_PART);
	
	for (int i = 0; i < TOTAL_PART; i++)
	{
        char fname[256];
		sprintf(fname, "./output_%d.png", i);
		write_file(fname, p_buf_all[i].buf, p_buf_all[i].size);
		
	    usleep(random_wait);
    }
	
	int catpng_result = catpng();
	if (gettimeofday(&tv, NULL) != 0) {
        perror("gettimeofday");
        abort();
    }
	times[1] = (tv.tv_sec) + tv.tv_usec / 1000000.;
	
	//printf("times[0]: %lf seconds\n", times[0]);
	//printf("times[1]: %lf seconds\n", times[1]);
	
    printf("paster2 execution time: %lf seconds\n", times[1] - times[0]);

    return 0;
}

