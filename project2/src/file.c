#include "file.h"



// Open file
int file_open(char *pathname) {
	if((fd = open(pathname,  O_CREAT|O_RDWR, 0777)) == -1) {
		fprintf(stderr, "%s\n", strerror(errno));
		return -1;
	}
	return 0;
}

// Close file
void file_close() {
	close(fd);
}

// File size
int check_size(){
	return lseek(fd, 0, SEEK_END);
}

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page() {
	page_t freepage;
	pagenum_t freepagenum;

	freepagenum = lseek(fd, 0, SEEK_END)/4096;

	lseek(fd, sizeof(page_t)*freepagenum, SEEK_SET);
	write(fd, &freepage, sizeof(page_t));
	sync();

	return freepagenum;
}

// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum) {
	page_t freepage;
	lseek(fd, sizeof(page_t)*pagenum, SEEK_SET);
	write(fd, &freepage, sizeof(page_t));
	sync();
}

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest) {
	lseek(fd, sizeof(page_t)*pagenum, SEEK_SET);
	read(fd, dest, sizeof(page_t));
	sync();
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, page_t* src) {
	lseek(fd, sizeof(page_t)*pagenum, SEEK_SET);
	write(fd, src, sizeof(page_t));
	sync();
}
