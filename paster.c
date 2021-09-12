#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <curl/curl.h>
#include "multi_thread.c"
#include "catpng.c"

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

int main( int argc, char** argv ) 
{
	int c;
    int t = 1;
    int n = 1;
    char *str = "option requires an argument";
    
    while ((c = getopt (argc, argv, "t:n:")) != -1) {
        switch (c) {
        case 't':
	    t = strtoul(optarg, NULL, 10);
	    //printf("option -t specifies a value of %d.\n", t);
	    if (t <= 0) {
                fprintf(stderr, "%s: %s > 0 -- 't'\n", argv[0], str);
                return -1;
            }
            break;
        case 'n':
            n = strtoul(optarg, NULL, 10);
	    //printf("option -n specifies a value of %d.\n", n);
            if (n <= 0 || n > 3) {
                fprintf(stderr, "%s: %s 1, 2, or 3 -- 'n'\n", argv[0], str);
                return -1;
            }
            break;
        default:
            return -1;
        }
    }
		
	if(t != 1)
	{
		//multi thread implementation - multiple servers and image n
		//printf("t is %d and n is %d\n", t, n);
		//printf("multi_thread\n");
		int result = multi_thread(n, t);
		//printf("multi_thread result is %d\n", result);	
		int catpng_result = catpng();
	}
	else
	{
		//default - single thread implementation - server 1 and image 1
		//printf("t is %d and n is %d\n", t, n);
		//printf("single_thread\n");
		int result = single_thread(n);
		//printf("single_thread result is %d\n", result);
		int catpng_result = catpng();
	}
    return 0;
}