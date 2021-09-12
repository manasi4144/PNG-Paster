#include "paster2.h"

int gen_url(char *res_url, int part)
{
	int result = snprintf(res_url, URL_LEN, "%simg=%d&part=%d", IMG_URL_BASE, pic_number, part);
	if (result < 0)
	{
		printf("In the gen_url error\n");
		return -1;
	}
	return 0;
}

int producer(int id)
{
	//printf("producer(%d)\n", id);
	int now_task = 0;
	char url[256];
	CURL *curl_handle;
    CURLcode res;
	
	Buffer *p_recv_buf = malloc(sizeof(Buffer));
	
	//CONNECT - shmat - connect variables to shared memory
	CONNECT_SHM(p_sem_get_task, sem_get_task, NULL, 0);
    CONNECT_SHM(p_sem_push, sem_push, NULL, 0);
    CONNECT_SHM(p_sem_slots, sem_slots, NULL, 0);
    CONNECT_SHM(p_sem_exists, sem_exists, NULL, 0);
    CONNECT_SHM(p_queue, queue, NULL, 0);
	
	// printf("Before Producer update_shm_dq()\n");
	// printf("p_queue->capacity: %d\n", p_queue->capacity);
	// printf("p_queue->head: %d\n", p_queue->head);
	// printf("p_queue->tail: %d\n", p_queue->tail);
	
    update_shm_dq(p_queue);
	
	// printf("After Producer update_shm_dq()\n");
	// printf("p_queue->capacity: %d\n", p_queue->capacity);
	// printf("p_queue->head: %d\n", p_queue->head);
	// printf("p_queue->tail: %d\n", p_queue->tail);
	//int producer_count = 0;
	
	while(1)
	{
		//printf("producer_count: %d\n", producer_count);
		//initialize receive buffer
		buffer_init(p_recv_buf);
		
		// get the current task number: which is the image part to be downloaded
		if ((sem_wait(p_sem_get_task)))
		{
			perror("sem_wait(p_sem_get_task)"); 
			printf("abort\n");
			abort();       
		} 
		
        if (*p_now_downloaded == TOTAL_PART)
		{
            now_task = 0;
        }
		else
		{
            (*p_now_downloaded)++;
            now_task = *p_now_downloaded;
        }
		
		if ((sem_post(p_sem_get_task)))
		{
			perror("sem_post(p_sem_get_task)"); 
			printf("abort\n");
			abort();       
		} 

        //if all parts are downloaded, then exit the loop
        if (!now_task) 
        {
			//printf("All images downloaded - breaking out of loop now\n");
			break;
		}
        //CHK_EXIT(gen_url(url, now_task - 1));
		//printf("gen_url(url, now_task - 1): %d\n", gen_url(url, now_task - 1));
		gen_url(url, now_task - 1);
		
		//download the current image part
		/* init a curl session */
		curl_handle = curl_easy_init();
		if (curl_handle == NULL) {
			fprintf(stderr, "curl_easy_init: returned NULL\n");
			return 1;
		}

		/* specify URL to get */
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		/* register write call back function to process received data */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl); 
		/* user defined data structure passed to the call back function */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)p_recv_buf);
		/* register header call back function to process received header data */
		curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
		/* user defined data structure passed to the call back function */
		curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)p_recv_buf);
		/* some servers requires a user-agent field */
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		
		/* get it! */
		res = curl_easy_perform(curl_handle);
		if( res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		
		// Wait for semaphore to add this to the queue.
    	CHK_EXIT(sem_wait(p_sem_push));
        CHK_EXIT(sem_wait(p_sem_slots));
		
		// printf("Before Producer dq_push_back()\n");
		// printf("p_queue->capacity: %d\n", p_queue->capacity);
		// printf("p_queue->head: %d\n", p_queue->head);
		// printf("p_queue->tail: %d\n", p_queue->tail);
		
        CHK_EXIT(dq_push_back(p_queue, p_recv_buf));
		
		// printf("After Producer dq_push_back()\n");
		// printf("p_queue->capacity: %d\n", p_queue->capacity);
		// printf("p_queue->head: %d\n", p_queue->head);
		// printf("p_queue->tail: %d\n", p_queue->tail);
       
        // semaphore post notifies consumer
        CHK_EXIT(sem_post(p_sem_exists));
        CHK_EXIT(sem_post(p_sem_push));
		//producer_count++;
	}
	
	/* cleaning up */
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
	
	//DETACH - shmdt
	DETACH_SHM(p_sem_get_task)
    DETACH_SHM(p_sem_push);
    DETACH_SHM(p_sem_slots);
    DETACH_SHM(p_sem_exists);
    DETACH_SHM(p_queue);
	
	//printf("Producer ID[%d]: exit\n", id);
    return 0;
}