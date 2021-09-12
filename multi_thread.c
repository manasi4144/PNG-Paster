/*
 * The code is derived from cURL example and paster.c base code.
 * The cURL example is at URL:
 * https://curl.haxx.se/libcurl/c/getinmemory.html
 * Copyright (C) 1998 - 2018, Daniel Stenberg, <daniel@haxx.se>, et al..
 *
 * The paster.c code is 
 * Copyright 2013 Patrick Lam, <p23lam@uwaterloo.ca>.
 *
 * Modifications to the code are
 * Copyright 2018-2019, Yiqing Huang, <yqhuang@uwaterloo.ca>.
 * 
 * This software may be freely redistributed under the terms of the X11 license.
 */

/** 
 * @file main_wirte_read_cb.c
 * @brief cURL write call back to save received data in a user defined memory first
 *        and then write the data to a file for verification purpose.
 *        cURL header call back extracts data sequence number from header.
 * @see https://curl.haxx.se/libcurl/c/getinmemory.html
 * @see https://curl.haxx.se/libcurl/using/
 * @see https://ec.haxx.se/callback-write.html
 */ 


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <curl/curl.h>
#include <pthread.h>
#include <errno.h>

#define IMG_URL "http://ece252-1.uwaterloo.ca:2520/image?img=1"
#define IMG_URL_2 "http://ece252-1.uwaterloo.ca:2520/image?img=2"
#define IMG_URL_3 "http://ece252-1.uwaterloo.ca:2520/image?img=3"
#define ECE252_HEADER "X-Ece252-Fragment: "
#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define BUF_INC  524288   /* 1024*512  = 0.5M */
#define MAX_PARTS 50

#define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

typedef struct recv_buf2 {
    char *buf;       /* memory to hold a copy of received data */
    size_t size;     /* size of valid data in buf in bytes*/
    size_t max_size; /* max capacity of buf in bytes*/
    int seq;         /* >=0 sequence number extracted from http header */
                     /* <0 indicates an invalid seq number */
} RECV_BUF;


size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata);
size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata);
int recv_buf_init(RECV_BUF *ptr, size_t max_size);
int recv_buf_cleanup(RECV_BUF *ptr);
int write_file(const char *path, const void *in, size_t len);


/**
 * @brief  cURL header call back function to extract image sequence number from 
 *         http header data. An example header for image part n (assume n = 2) is:
 *         X-Ece252-Fragment: 2
 * @param  char *p_recv: header data delivered by cURL
 * @param  size_t size size of each memb
 * @param  size_t nmemb number of memb
 * @param  void *userdata user defined data structurea
 * @return size of header data received.
 * @details this routine will be invoked multiple times by the libcurl until the full
 * header data are received.  we are only interested in the ECE252_HEADER line 
 * received so that we can extract the image sequence number from it. This
 * explains the if block in the code.
 */
 
int parts_received[50] = {0};
int parts_count = 0;

size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata)
{
    int realsize = size * nmemb;
    RECV_BUF *p = userdata;
    
    if (realsize > strlen(ECE252_HEADER) && strncmp(p_recv, ECE252_HEADER, strlen(ECE252_HEADER)) == 0)
	{
		/* extract img sequence number */
		p->seq = atoi(p_recv + strlen(ECE252_HEADER));
		/*if(parts_received[p->seq] == 0)
		{
			parts_received[p->seq] == 1;
			parts_count++;
			
			if(parts_count == MAX_PARTS)
			{
				printf("All parts received\n");
			}
			else
			{
				printf("Part count: %d\n", parts_count);
			}
		}
		else
		{
			printf("File already received\n");
		}*/
    }

    return realsize;
}


/**
 * @brief write callback function to save a copy of received data in RAM.
 *        The received libcurl data are pointed by p_recv, 
 *        which is provided by libcurl and is not user allocated memory.
 *        The user allocated memory is at p_userdata. One needs to
 *        cast it to the proper struct to make good use of it.
 *        This function maybe invoked more than once by one invokation of
 *        curl_easy_perform().
 */

size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata)
{
    size_t realsize = size * nmemb;
    RECV_BUF *p = (RECV_BUF *)p_userdata;
 
    if (p->size + realsize + 1 > p->max_size) {/* hope this rarely happens */ 
        /* received data is not 0 terminated, add one byte for terminating 0 */
        size_t new_size = p->max_size + max(BUF_INC, realsize + 1);   
        char *q = realloc(p->buf, new_size);
        if (q == NULL) {
            perror("realloc"); /* out of memory */
            return -1;
        }
        p->buf = q;
        p->max_size = new_size;
    }

    memcpy(p->buf + p->size, p_recv, realsize); /*copy data from libcurl*/
    p->size += realsize;
    p->buf[p->size] = 0;

    return realsize;
}


int recv_buf_init(RECV_BUF *ptr, size_t max_size)
{
    void *p = NULL;
    
    if (ptr == NULL) {
        return 1;
    }

    p = malloc(max_size);
    if (p == NULL) {
	return 2;
    }
    
    ptr->buf = p;
    ptr->size = 0;
    ptr->max_size = max_size;
    ptr->seq = -1;              /* valid seq should be non-negative */
    return 0;
}

int recv_buf_cleanup(RECV_BUF *ptr)
{
    if (ptr == NULL) {
	return 1;
    }
    
    free(ptr->buf);
    ptr->size = 0;
    ptr->max_size = 0;
    return 0;
}


/**
 * @brief output data in memory to a file
 * @param path const char *, output file path
 * @param in  void *, input data to be written to the file
 * @param len size_t, length of the input data in bytes
 */

int write_file(const char *path, const void *in, size_t len)
{
    FILE *fp = NULL;

    if (path == NULL) {
        fprintf(stderr, "write_file: file name is null!\n");
        return -1;
    }

    if (in == NULL) {
        fprintf(stderr, "write_file: input data is null!\n");
        return -1;
    }

    fp = fopen(path, "wb");
    if (fp == NULL) {
        perror("fopen");
        return -2;
    }

    if (fwrite(in, 1, len, fp) != len) {
        fprintf(stderr, "write_file: imcomplete write!\n");
        return -3; 
    }
    return fclose(fp);
}

pthread_mutex_t mutex;

typedef struct img_data {
	int server_id;
	int image_id;
}img_data;

void* get_png_part(void *arg) {
	CURL *curl_handle;
	CURLcode res;
	char url[256] = {0};
	RECV_BUF recv_buf;
	char fname[256];
	img_data* data = (img_data*) arg;

	sprintf(url,"http://ece252-%d.uwaterloo.ca:2520/image?img=%d",data->server_id, data->image_id);
	
	recv_buf_init(&recv_buf, BUF_SIZE);
	
	//printf("URL is %s\n", url);

	//curl_global_init(CURL_GLOBAL_DEFAULT);

	/* init a curl session */
	curl_handle = curl_easy_init();

	if (curl_handle == NULL) {
		fprintf(stderr, "curl_easy_init: returned NULL\n");
		return NULL;
	}

	/* specify URL to get */
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);

	/* register write call back function to process received data */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3); 
	/* user defined data structure passed to the call back function */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&recv_buf);

	/* register header call back function to process received header data */
	curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
	/* user defined data structure passed to the call back function */
	curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&recv_buf);

	/* some servers requires a user-agent field */
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	
	/* get it! */				
	res = curl_easy_perform(curl_handle);
	
	if( res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	} else {
	//printf("%lu bytes received in memory %p, seq=%d.\n", \
				recv_buf.size, recv_buf.buf, recv_buf.seq);
	}

	int should_write_file = 0;
	pthread_mutex_lock(&mutex);
	//critical region begins
	if(parts_received[recv_buf.seq] == 0)
	{
		should_write_file = 1;
		parts_received[recv_buf.seq] = 1;
		parts_count++;
	}
	//critical region ends
	pthread_mutex_unlock(&mutex);

	if (should_write_file) {
		sprintf(fname, "./output_%d.png", recv_buf.seq);
		//printf("Saving data to file: %s\n", fname);
		write_file(fname, recv_buf.buf, recv_buf.size);
	}
	
	/* cleaning up */
	curl_easy_cleanup(curl_handle);
	//curl_global_cleanup();
	recv_buf_cleanup(&recv_buf);
	free(arg);
	return (NULL);
}

int multi_thread(int number, int threadcnt) 
{
	pthread_t threads[threadcnt];

	curl_global_init(CURL_GLOBAL_DEFAULT);
	
	int server_id = 1;
	
	do
	{
		for (int i = 0; i < threadcnt; i++)
		{
			img_data *data = (img_data*) malloc(sizeof(img_data));
			data->server_id = server_id;
			data->image_id = number;
			int res = pthread_create(&threads[i],NULL,get_png_part,data);
			if (res != 0) {
				perror("Error creating thread");
				exit(1);
			}
			server_id++;
			if (server_id > 3) {
				server_id = 1;
			}
		}
		for (int i = 0; i < threadcnt; i++)
		{
			void *status;
			pthread_join(threads[i],&status);
		}		
	}
	while(parts_count < MAX_PARTS);

	//printf("All parts received\n");
	
	curl_global_cleanup();	
	return 0;
}

int single_thread(int number) 
{
	curl_global_init(CURL_GLOBAL_DEFAULT);
	while(parts_count < MAX_PARTS)
	{
		CURL *curl_handle;
		CURLcode res;
		char url[256] = {0};
		RECV_BUF recv_buf;
		char fname[256];
		
		recv_buf_init(&recv_buf, BUF_SIZE);
		
		if (number == 1) {
			strcpy(url, IMG_URL); 
		}else if (number == 2) {
			strcpy(url, IMG_URL_2); 
		}else {
			strcpy(url, IMG_URL_3);
		}
		//printf("URL is %s\n", url);

		//curl_global_init(CURL_GLOBAL_DEFAULT);

		/* init a curl session */
		curl_handle = curl_easy_init();

		if (curl_handle == NULL) {
			fprintf(stderr, "curl_easy_init: returned NULL\n");
			return 1;
		}

		/* specify URL to get */
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);

		/* register write call back function to process received data */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3); 
		/* user defined data structure passed to the call back function */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&recv_buf);

		/* register header call back function to process received header data */
		curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
		/* user defined data structure passed to the call back function */
		curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&recv_buf);

		/* some servers requires a user-agent field */
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

		
		/* get it! */
		res = curl_easy_perform(curl_handle);
		
		if( res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		} else {
		//printf("%lu bytes received in memory %p, seq=%d.\n", \
				   recv_buf.size, recv_buf.buf, recv_buf.seq);
		}
		
		if(parts_received[recv_buf.seq] == 0)
		{
			parts_received[recv_buf.seq] = 1;
			parts_count++;
			//printf("Part count: %d\n", parts_count);
			//printf("parts_received[recv_buf.seq]: %d\n", parts_received[recv_buf.seq]);
		}
		else
		{
			//printf("File already received\n");
		}

		sprintf(fname, "./output_%d.png", recv_buf.seq);
		//printf("Saving data to file: %s\n", fname);
		write_file(fname, recv_buf.buf, recv_buf.size);
		
		/* cleaning up */
		curl_easy_cleanup(curl_handle);
		//curl_global_cleanup();
		recv_buf_cleanup(&recv_buf);
	}
	//printf("All parts received\n");
	
	curl_global_cleanup();	
	return 0;
}