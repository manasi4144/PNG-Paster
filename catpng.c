#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "is_png.c"
#include <errno.h>
#include "zutil.h"

typedef unsigned char U8;
typedef unsigned int  U32;
typedef unsigned long int U64;

U64 get_IDAT_data(char* bname)
{
	FILE* p1file;
    p1file = fopen(bname,"rb");
    U64 num_bytes = 0;
    if (p1file==NULL) {fputs ("File error",stderr); exit (1);}
    U64 num_bytes_test = 0;
	fseek(p1file, 33, SEEK_SET);
	fread(&num_bytes_test, 4, 1, p1file);
	num_bytes = htonl(num_bytes_test);


    fclose(p1file);
    return num_bytes;

}


int catpng() 
{
    //char* names_array[4];
	char fname[256];
	//sprintf(fname, "./output_%d_%d.png", i, pid);
	
	U32 height[4];
    U32 total_height = 0;
    U32 width;

    U64 total_dest_size = 0;
    for (int i = 0; i < 50; i++)
    {
        sprintf(fname, "./output_%d.png", i);
		FILE *pfile;
        pfile = fopen(fname,"rb"); 
	    if (pfile==NULL) {fputs ("File error",stderr); exit (1);}

        //printf("\neval w and h of %s\n", fname);
        if (i == 0)
        {
            width = get_wh_IHDR(fname, 3);
            //printf("width is %u \n", width);
        }

        U32 h = get_wh_IHDR(fname, 2);
        total_height += h;
        height[i] = h;
		//printf("name: %s\n", fname);
        //printf("height of %d is %u \n", i, h);

        total_dest_size += (((h * width) * 4)+ h);
        fclose(pfile);
    }
    //printf("total buffer size %lu", total_dest_size);

    U8 *total_dest_buffer = (unsigned char*)malloc(total_dest_size);
    U64 count = 0;
    U64 len_source = 0;

    for (int i = 0; i < 50; i++)
    {
        sprintf(fname, "./output_%d.png", i);
		//printf("\ncompressing %s", fname);
        U64 dest_len = ((height[i] * width) * 4)+ height[i];
        //printf("\ndest_len is %lu \n", dest_len);

        U64 size_IDATdata = get_IDAT_data(fname); //calculate the source for mem_inf
        len_source += size_IDATdata; //calcuate the source for mem_def
        //printf("len_source is %lu \n", len_source);
          
        FILE *newfile;
        newfile = fopen(fname,"rb");
        if (newfile==NULL) {fputs ("File error",stderr); exit (1);}

        //int total_readbytes = 33 + len_source; //33 + 12 + num_bytes -> to get full IHDR + IDAT chunks
        //printf("total_readbytes is %d \n", total_readbytes);

	    U8 *source_test = (unsigned char*)malloc(size_IDATdata);
	    fseek(newfile, 41, SEEK_SET);
        fread(source_test, size_IDATdata, 1, newfile);

        U8 *dest_buffer = (unsigned char*)malloc(dest_len);

        int m_inf_r = mem_inf(dest_buffer, &dest_len, source_test, size_IDATdata);
        //printf("\n m_inf returned %d \n", m_inf_r);

        int i = count;
        //printf("count is %lu\n", count);
         for (int j = 0; j < dest_len; j++)
         {
             total_dest_buffer[i] = dest_buffer[j];
            // printf("%02x", total_dest_buffer[i]);
             i++;
         }

        count += dest_len;
        //printf("count is %lu\n", count);

        free(dest_buffer);
        //printf("\n");
        fclose(newfile);

    }


    U8 *deflate_buffer = (unsigned char*)malloc(len_source);
  //  printf("\n len_source is %lu", len_source);
    int m_def_r = mem_def(deflate_buffer, &len_source, total_dest_buffer, total_dest_size, Z_DEFAULT_COMPRESSION);

    FILE *sourcefile;
    sourcefile = fopen(fname, "rb");
    U8 header[8];
    U8 IHDR_ltw[12];
    U8 IHDR_rest[5];
    
    fread(&header, 8, 1, sourcefile);
    fread(&IHDR_ltw, 12, 1, sourcefile);

    FILE *pfile;
    pfile = fopen("all.png", "wb+");
    if (pfile==NULL) {fputs ("File error",stderr); exit (1);}
    fwrite(&header, 8, 1, pfile); //add PNG header signature
    fwrite(&IHDR_ltw, 12, 1, pfile); // add IHDR length, type and width 
    //convert total_height into hex values
    U8 total_height_ar[4];
    total_height_ar[3] = total_height & 0xff;
    total_height_ar[2] = (total_height >> 8) & 0xff;
    total_height_ar[1] = (total_height >> 16) & 0xff;
    total_height_ar[0] = (total_height >> 24) & 0xff;

   // printf("\nthe total height is %u \n", total_height);


    fwrite(&total_height_ar, 4, 1, pfile);

    fseek(sourcefile, 4, SEEK_CUR);  //go to to bid depth
    fread(&IHDR_rest, 5, 1, sourcefile);
    fwrite(&IHDR_rest, 5, 1, pfile);

    fclose(pfile); 
    pfile = fopen("all.png", "rb");
    //CRC calculation
    U8 calc[17];
    fseek(pfile, 12, SEEK_SET);
    fread(&calc, 17, 1, pfile);
    U32 IHDR_crc = 0; 	
	IHDR_crc = crc(calc,17);

 //   printf("\nthe IDHR crc value is %u \n", IHDR_crc);

    fclose(pfile);

    pfile = fopen("all.png", "ab+");
    //fseek(pfile, 29, SEEK_SET);

    U8 crc_val[4];
    crc_val[3] = IHDR_crc & 0xff;
    crc_val[2] = (IHDR_crc >> 8) & 0xff;
    crc_val[1] = (IHDR_crc >> 16) & 0xff;
    crc_val[0] = (IHDR_crc >> 24) & 0xff;

    fwrite(&crc_val, 4, 1, pfile);

 //   printf("\n len_source is %lu", len_source);
    U8 IDAT_length[4];
    IDAT_length[3] = len_source & 0xff;
    IDAT_length[2] = (len_source >> 8) & 0xff;
    IDAT_length[1] = (len_source >> 16) & 0xff;
    IDAT_length[0] = (len_source >> 24) & 0xff;

    fwrite(&IDAT_length, 4, 1, pfile);

    U8 IDAT_type[4];
    fseek(sourcefile, 37, SEEK_SET); 
    fread(&IDAT_type, 4, 1, sourcefile);
    fwrite(&IDAT_type, 4, 1, pfile);
    fwrite(deflate_buffer, len_source, 1, pfile);

    U8 *IDAT_calc= (unsigned char*)malloc(len_source + 4);

    for (int i = 0 ; i < (len_source + 4); i++)
    {
        if (i < 4)
        {
            IDAT_calc[i] = IDAT_type[i];
        }
        else
        {
            IDAT_calc[i] = deflate_buffer[i - 4];
        }
    }

    U32 IDAT_crc = 0; 	
	IDAT_crc = crc(IDAT_calc, len_source + 4);

 //   printf("\nthe IDAT crc value is %u \n", IDAT_crc);

    U8 crc_val_IDAT[4];
    crc_val_IDAT[3] = IDAT_crc & 0xff;
    crc_val_IDAT[2] = (IDAT_crc >> 8) & 0xff;
    crc_val_IDAT[1] = (IDAT_crc >> 16) & 0xff;
    crc_val_IDAT[0] = (IDAT_crc >> 24) & 0xff;

    fwrite(&crc_val_IDAT, 4, 1, pfile);

    U8 IEND_length[4];
    U8 IEND_type[4];
    fseek(sourcefile, -12 , SEEK_END);
    fread(&IEND_length, 4, 1, sourcefile);
    fread(&IEND_type, 4, 1, sourcefile);
    fwrite(&IEND_length, 4, 1, pfile);
    fwrite(&IEND_type, 4, 1, pfile);

    U32 IEND_crc = 0; 	
	IEND_crc = crc(IEND_type, 4);

  //  printf("\nthe IEND crc value is %u \n", IEND_crc);

    U8 crc_val_IEND[4];
    crc_val_IEND[3] = IEND_crc & 0xff;
    crc_val_IEND[2] = (IEND_crc >> 8) & 0xff;
    crc_val_IEND[1] = (IEND_crc >> 16) & 0xff;
    crc_val_IEND[0] = (IEND_crc >> 24) & 0xff;

    fwrite(&crc_val_IEND, 4, 1, pfile);

    fclose(pfile);
    fclose(sourcefile);
    free(IDAT_calc);
    free(deflate_buffer);
    free(total_dest_buffer);
    return 0;

}


