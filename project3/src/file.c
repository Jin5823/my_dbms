#include "file.h"



// Open file
int file_open(char *pathname, int utableid) {
	if((fd[utableid-1] = open(pathname,  O_CREAT|O_RDWR, 0777)) == -1) {
		fprintf(stderr, "%s\n", strerror(errno));
		return -1;
	}
	return 0;
}

// Close file
void file_close(int utableid) {
	close(fd[utableid-1]);
}

// File size
int check_size(int utableid){
	return lseek(fd[utableid-1], 0, SEEK_END);
}

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(int utableid) {
	page_t freepage;
	pagenum_t freepagenum;

	freepagenum = lseek(fd[utableid-1], 0, SEEK_END)/4096;

	lseek(fd[utableid-1], sizeof(page_t)*freepagenum, SEEK_SET);
	write(fd[utableid-1], &freepage, sizeof(page_t));
	sync();

	return freepagenum;
}

// Free an on-disk page to the free page list
void file_free_page(int utableid, pagenum_t pagenum) {
	page_t freepage;
	lseek(fd[utableid-1], sizeof(page_t)*pagenum, SEEK_SET);
	write(fd[utableid-1], &freepage, sizeof(page_t));
	sync();
}

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(int utableid, pagenum_t pagenum, page_t* dest) {
	lseek(fd[utableid-1], sizeof(page_t)*pagenum, SEEK_SET);
	read(fd[utableid-1], dest, sizeof(page_t));
	sync();
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(int utableid, pagenum_t pagenum, page_t* src) {
	lseek(fd[utableid-1], sizeof(page_t)*pagenum, SEEK_SET);
	write(fd[utableid-1], src, sizeof(page_t));
	sync();
}
