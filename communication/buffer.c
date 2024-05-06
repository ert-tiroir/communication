
#include "communication/buffer.h"

unsigned int next (unsigned int value, struct buffer_t buffer) {
    return (value + buffer.pag_size) & buffer.max_mask;
}

unsigned int max_size (struct buffer_t* buffer) {
    return buffer->max_size;
}
unsigned int pag_size (struct buffer_t* buffer) {
    return buffer->pag_size;
}

unsigned int size (struct buffer_t* buffer) {
    unsigned int max_page_mask = buffer->max_size - 1;
    
    return (buffer->right + buffer->max_size - buffer->left) & max_page_mask;
}

unsigned char* writable_page (struct buffer_t* buffer) {
    if (next(buffer->right, *buffer) == buffer->left) return 0;

    return buffer->target + buffer->right;
}
unsigned char* readable_page (struct buffer_t* buffer) {
    if (buffer->left == buffer->right) return 0;

    return buffer->target + buffer->left;
}

void free_write (struct buffer_t* buffer) {
    buffer->right = next(buffer->right, *buffer);
}
void free_read (struct buffer_t* buffer) {
    buffer->left = next(buffer->left, *buffer);
}

struct buffer_t create_buffer (unsigned int page, unsigned int page_count, unsigned char* target) {
    unsigned int size = page * page_count;
    struct buffer_t result = {
        .left  = 0,
        .right = 0,

        .max_size = size,
        .max_mask = size - 1,
        .pag_size = page,

        .target = target
    };

    return result;
}
