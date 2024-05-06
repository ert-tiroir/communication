
#pragma once

struct buffer_t {
    unsigned int left;
    unsigned int right;

    unsigned int max_size; // max_size must be a power of 2 times pag_size
    unsigned int max_mask; // max_size - 1 precomputed
    unsigned int pag_size; // pag_size must be a power of 2

    unsigned char* target;    
};

unsigned int size     (struct buffer_t* buffer);
unsigned int max_size (struct buffer_t* buffer);
unsigned int pag_size (struct buffer_t* buffer);

unsigned char* writable_page (struct buffer_t* buffer);
unsigned char* readable_page (struct buffer_t* buffer);

void free_write (struct buffer_t* buffer);
void free_read  (struct buffer_t* buffer);

struct buffer_t create_buffer (unsigned int page, unsigned int size, unsigned char* target);
