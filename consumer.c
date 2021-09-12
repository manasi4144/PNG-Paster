#include "paster2.h"

int consumer(int id)
{
	//printf("consumer(%d)\n", id);
	//CONNECT - shmat - connect variables to shared memory
	CONNECT_SHM(p_sem_need_consume, sem_need_consume, NULL, 0);
    CONNECT_SHM(p_sem_pop, sem_pop, NULL, 0);
    CONNECT_SHM(p_sem_slots, sem_slots, NULL, 0);
    CONNECT_SHM(p_sem_exists, sem_exists, NULL, 0);
    CONNECT_SHM(p_queue, queue, NULL, 0);
	
	// printf("Before Consumer update_shm_dq()\n");
	// printf("p_queue->capacity: %d\n", p_queue->capacity);
	// printf("p_queue->head: %d\n", p_queue->head);
	// printf("p_queue->tail: %d\n", p_queue->tail);
	
    update_shm_dq(p_queue);
	
	// printf("After Consumer update_shm_dq()\n");
	// printf("p_queue->capacity: %d\n", p_queue->capacity);
	// printf("p_queue->head: %d\n", p_queue->head);
	// printf("p_queue->tail: %d\n", p_queue->tail);
	//int consumer_count = 0;
	
    CONNECT_SHM(p_buf_all, buf_all, NULL, 0);
	
	Buffer *p_buf = (Buffer *)malloc(sizeof(Buffer));
	//printf("Consumer ID[%d]: init pid[%d]\n", id, getpid());
	
	while (1)
	{
		//printf("consumer_count: %d\n", consumer_count);
		
		if (sem_trywait(p_sem_need_consume))
		{  
			// all down
            return 0;
        }
        //Wait till an image part is downloaded and ready
		CHK_EXIT(sem_wait(p_sem_pop));
        CHK_EXIT(sem_wait(p_sem_exists));
		
		// printf("Before Consumer dq_pop_front()\n");
		// printf("p_queue->capacity: %d\n", p_queue->capacity);
		// printf("p_queue->head: %d\n", p_queue->head);
		// printf("p_queue->tail: %d\n", p_queue->tail);
		
        dq_pop_front(p_queue, p_buf);
		
		// printf("After Consumer dq_pop_front()\n");
		// printf("p_queue->capacity: %d\n", p_queue->capacity);
		// printf("p_queue->head: %d\n", p_queue->head);
		// printf("p_queue->tail: %d\n", p_queue->tail);
		
		CHK_EXIT(sem_post(p_sem_slots));
        CHK_EXIT(sem_post(p_sem_pop));
		
        p_buf_all[p_buf->seq] = *p_buf;
		//consumer_count++;
    }
	
	//DETACH - shmdt
	DETACH_SHM(p_sem_need_consume);
    DETACH_SHM(p_sem_pop);
    DETACH_SHM(p_sem_slots);
    DETACH_SHM(p_sem_exists);
    DETACH_SHM(p_queue);
    DETACH_SHM(p_buf_all);
	
	//printf("Consumer ID[%d]: exit\n", id);
    return 0;
}