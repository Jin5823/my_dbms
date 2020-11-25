#ifndef FILE_H_
#define FILE_H_


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>




int fd;
typedef uint64_t pagenum_t;
typedef struct page_t {
	pagenum_t num[512];
}page_t;

// Open file
int file_open(char *pathname);

// Close file
void file_close();

// File size
int check_size();

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page();

// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum);

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest);

// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, page_t* src);


#endif /* FILE_H_ */
