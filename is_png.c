#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "crc.h"
#include "crc.c"

typedef unsigned char U8;

typedef unsigned int  U32;

#define BUF_LEN  (256*16)
#define BUF_LEN2 (256*32)

U8 gp_buf_def[BUF_LEN2]; /* output buffer for mem_def() */
U8 gp_buf_inf[BUF_LEN2]; /* output buffer for mem_inf() */

int is_png(U8 *buf, size_t n)
{
		if (n != 8)
	{
		//printf("n is not eight bytes, value of n:%02lx \n", n);
		return -1;
	}

	unsigned char signature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

	for (int i = 0; i < n; i++)
	{
		if (signature[i] != buf[i])
	{
		//printf("Not a PNG file \n");
			return -1;
	}
	}

	return 0;

}

int get_IEND_chunk(char *bname, long int num_bytes, int print_val1)
{
	FILE* pfile;
        pfile = fopen(bname,"rb");
        if (pfile==NULL) {fputs ("File error",stderr); exit (1);}

        U8 signature[57 + num_bytes];

        int result2 = fread(&signature, (57 + num_bytes), 1, pfile); //not required &

        //U8 calc[17];

        U8 length_data[4];

	int ind = num_bytes + 45;

//	printf("\nIEND num_bytes %ld", num_bytes);
	//printf("\n IEND ind %d", ind);

        int j = 0;
        for (int i = ind; i < (ind + 4); i++)
        {
                length_data[j] = signature[i];
                //printf("\n IEND length_data %02x", length_data[j]);

        }


    U8 crcexp[4]; // CRC

	U8 calc[4]; //type

	j = 0;
        int l = 3;
        for (int i = (49 + num_bytes); i < (57 + num_bytes); i++)
        {
                //printf("\n IEND sig_data %02x %d", signature[i], i);

                if (i > (48 + num_bytes) && (i < (53 + num_bytes)))
                {
                        calc[j] = signature[i];
                        //printf("\n IEND calc is %02x", calc[j]);
                        j++;
                }
                else if (i > (52 + num_bytes) && (i < (57 + num_bytes)))
		{
                        crcexp[l] = signature[i];
  //                      printf("\n IEND crcexp is %02x", crcexp[l]);
                        l--;
                }

        }
		
	fclose(pfile);
	
	FILE* sfile;
	sfile = fopen(bname,"rb");
	if (pfile==NULL) {fputs ("File error",stderr); exit (1);}
	
	U32 crc_exp_test = 0;
	int start_read = 53 + num_bytes;
	fseek(sfile, start_read, SEEK_SET);
	fread(&crc_exp_test, 4, 1, sfile);
	U32 crc_expected = htonl(crc_exp_test);

	if (print_val1 == 1)
	{
		 U32 crc_val = 0;
        // Step 3: Demo how to use the crc utility
        crc_val = crc(calc,4); // down cast the return val to U32	
        if (crc_expected != crc_val)
        {
                printf("IEND chunk CRC error: computed %x, expected %x\n", crc_val, crc_expected);
                return -1;
        }
	}

       return 0;

}

int get_IDAT_chunk(char* bname, int print_val2)
{
	FILE* pfile;
        pfile = fopen(bname,"rb");
        if (pfile==NULL) {fputs ("File error",stderr); exit (1);}
		
		U32 read_bytes = 0;
		fseek(pfile, 33, SEEK_SET);
		fread(&read_bytes, 4, 1, pfile);
		U32 num_bytes = htonl(read_bytes);
		//printf("\nIDAT num_bytes: %d", num_bytes);

	const long int c_bytes = 37+ num_bytes + 8;

	U8 chunk[c_bytes];
	U8 crcexp[4];
	
	//printf("\n c_bytes %ld", c_bytes);

	fclose(pfile);

	FILE* pngfile;

	pngfile = fopen(bname,"rb");
    if (pngfile==NULL) {fputs ("File error",stderr); exit (1);}

	int r1 = fread(chunk, c_bytes, 1, pngfile);

	//printf("\n r1 %d", r1);

	const int calcsize = num_bytes + 4;
    //printf("\n calc size is %d", calcsize);
	U8 calc[calcsize];
	//calc stores the type and data fields while crcexp is storing the CRC field

	int j = 0;
	int l = 3;
	for (int i = 37; i < c_bytes; i++)
	{
		//printf("\n IDAT chunk_data %02x %d", chunk[i], i);

		if (i > 36 && (i < (37 + num_bytes + 4)))
		{
			calc[j] = chunk[i];
            //printf("\n IDAT calc is %02x", calc[j]);
            j++;
			//break;
		}
		else if (i > (37 + num_bytes + 3) && (i < c_bytes))
		{
			crcexp[l] = chunk[i];
    //           		printf("\n IDAT crcexp is %02x", crcexp[l]);
                	l--;
		}

	}
	
	fclose(pngfile);
	
	FILE* sfile;
	sfile = fopen(bname,"rb");
	if (pfile==NULL) {fputs ("File error",stderr); exit (1);}
	
	U32 crc_exp_test = 0;
	int start_read = 37 + num_bytes + 4;
	fseek(sfile, start_read, SEEK_SET);
	fread(&crc_exp_test, 4, 1, sfile);
	U32 crc_expected = htonl(crc_exp_test);

	if (print_val2 == 1)
	{
		U32 crc_val = 0;
		// Step 3: Demo how to use the crc utility 
		crc_val = crc(calc,(num_bytes + 4)); // down cast the return val to U32

		//printf("\nIDAT crc_exp_test : %u", crc_expected);
		//printf("\nIDAT crc_val = %u", crc_val);
		
		if (crc_expected != crc_val)
		{
			printf("IDAT chunk CRC error: computed %x, expected %x\n", crc_val, crc_expected);
		    return -1;

		}
	}

	return num_bytes;
}


int get_wh_IHDR(char* bname, int print_val)
{
	FILE* mfile;
	mfile = fopen(bname,"rb");
	if (mfile==NULL) {fputs ("File error",stderr); exit (1);}
	
	U32 width = 0;
	fseek(mfile, 16, SEEK_SET);
	fread(&width, 4, 1, mfile);
	U32 w = htonl(width);
	
	U32 height = 0;
	//fseek(sfile, 29, SEEK_SET);
	fread(&height, 4, 1, mfile);
	U32 h = htonl(height);
	//printf("\nw is %ld", w);
	//printf("\nh is %ld", h);
	
	fclose(mfile);
	
	FILE* pfile;
	pfile = fopen(bname,"rb");
	if (pfile==NULL) {fputs ("File error",stderr); exit (1);}

	U8 signature[32];

    int result2 = fread(&signature, 33, 1, pfile); //not required &

	U8 calc[17];

	U8 length_data[4];

	int j = 0;
	for (int i = 12; i < 29; i++)
	{
		calc[j] = signature[i];
		//printf("\n IHDR calc %02x", calc[j]);
		j++;
	}

	j = 0;
	U8 crc_exp[4];
	for (int i = 29; i < 33; i++)
	{
		crc_exp[j] = signature[i];
//		printf("\n IHDR crc_exp %02x", crc_exp[j]);
		j++;
	}
	
	/*Updated Area*/
	fclose(pfile);
	
	FILE* sfile;
	sfile = fopen(bname,"rb");
	if (pfile==NULL) {fputs ("File error",stderr); exit (1);}
	
	U32 crc_exp_test = 0;
	fseek(sfile, 29, SEEK_SET);
	fread(&crc_exp_test, 4, 1, sfile);
	U32 crc_expected = htonl(crc_exp_test);
	//printf("\ncrc_exp_test: %d", crc_expected);
	
	/*Updated Area Close*/

	if (print_val == 1)
	{
		printf("%s: %u x %u\n", bname, w, h);

		U32 crc_val = 0; 	
	 
    	crc_val = crc(calc,17); // down cast the return val to U32
    	//printf("\n crc_val = %x\n", crc_val);
		//printf("\n CRC exp: %x", htonl(crc_exp));
        if (crc_expected != crc_val)
        {
			printf("IDHR chunk CRC error: computed %x, expected %x\n", crc_val, crc_expected);
			return -1;
        }
	}
	else if (print_val == 2)
	{
		return h;
	}
	else
	{
		return w;
	}

return 0;

}




