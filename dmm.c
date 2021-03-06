#include <stdio.h>  // needed for size_t
#include <unistd.h> // needed for sbrk
#include <assert.h> // needed for asserts
#include "dmm.h"

/* You can improve the below metadata structure using the concepts from Bryant
 * and OHallaron book (chapter 9).
 */

typedef struct metadata {
    /* size_t is the return type of the sizeof operator. Since the size of an
     * object depends on the architecture and its implementation, size_t is used
     * to represent the maximum size of any object in the particular
     * implementation. size contains the size of the data object or the number of
     * free bytes
     */
    size_t size;
    struct metadata* next;
    struct metadata* prev;
} metadata_t;

/* freelist maintains all the blocks which are not in use; freelist is kept
 * sorted to improve coalescing efficiency
 */

static metadata_t* freelist = NULL;

void* split(metadata_t* curr, size_t numbytes){
    void* ret = (void*)curr + sizeof(metadata_t);
    /*metadata_t* ret = sizeof(metadata_t) + curr;*/

    /* CASE 1: if free block is EXACTLY the same size the user needs */
    if (curr->size == (sizeof(metadata_t) + numbytes)){
        if(curr->prev != NULL){ /* not at first block */
            curr->prev->next = curr->next;
        }
        else if(curr->prev == NULL){ /* at first block */
            freelist = curr->next;
        }
        if(curr->next != NULL){
            curr->next->prev = curr->prev;
        }
    }
    /* CASE 2: free block is bigger than what is needed, split it */
    else {
        void* rem_add = (void*)curr + sizeof(metadata_t) + numbytes;
        metadata_t *rem = (metadata_t*)rem_add;
        rem->size = curr->size - sizeof(metadata_t) - numbytes; /* calculate the remainder of free block */
        /* EDGE CASE: remaining part is not enough to account for a free block b/c of the needed header space, don't add it to freelist */
        if(rem->size < sizeof(metadata_t) + ALIGN(1)){ 
            if(curr->prev != NULL){ /* not the first block in freelist */
                curr->prev->next = curr->next;  /* take out block to malloc by rearranging pointers */
            }
            else if(curr->prev == NULL){ /* first block in freelist is what want to take out */
                freelist = curr->next;
            }
            if(curr->next != NULL){
                curr->next->prev = curr->prev;
            }
            printf("freelist is empty");
            return ret;
        }
        rem->prev = curr->prev;
        rem->next = curr->next;
        if(curr->prev != NULL){
            curr->prev->next = rem;
        }
        else if(curr->prev == NULL){
            freelist = rem;
        }
        if(curr->next != NULL){
            curr->next->prev = rem;
        }
        curr->size = sizeof(metadata_t) + numbytes; /* requested size */
    }
    curr->prev = NULL;
    curr->next = NULL;
    return ret;
}

void* dmalloc(size_t numbytes) {
    /* initialize through sbrk call first time */
    if(freelist == NULL) {
        if(!dmalloc_init())
            return NULL;
    }
    assert(numbytes > 0);
    numbytes = ALIGN(numbytes);
    metadata_t* curr = freelist;
    void* ret = NULL;
    while(true){
        if (curr->size >= (sizeof(metadata_t) + numbytes)) { /* compare requested size+header to available free block */
            ret = split(curr, numbytes);
            break;
        }
        if (curr->next == NULL){ /* there are no free blocks */
            assert(Error);
            break;
        }
        curr = curr->next; /* iterate to next free block */
    }
    //print_freelist();
    return ret;
}

void coalesce() {
    metadata_t *curr = freelist;
    while(curr->next != NULL){ /* iterate through freelist */

        void* next_address = (void *)curr + curr->size;
        metadata_t* nxt = (metadata_t*)next_address;
        if (curr->next == nxt){ /* find that two blocks are adjacent (via comparing addresses) */
            curr->size += curr->next->size;
            curr->next = curr->next->next;
            if(curr->next != NULL){
                curr->next->prev = curr;
            }
        }
        else {
            curr = curr->next;
        }
    }
}

void dfree(void* ptr) {
    /* your code here */
    /* sort with respect to addresses as you insert into freelist, so that you can coalesce in one pass of list each time you call dfree */
    /* coalesce adjacent free blocks into one */
    /* to coalesce add the space of second block and its metadata to space in first block */
    metadata_t *curr = freelist;
    void* header_address = ptr - sizeof(metadata_t);
    metadata_t *header = (metadata_t*) header_address; 
    /* iterate through freelist to place newly free block in ascending order with respect to address */
    if(curr == NULL){
        freelist = header;
        header->prev = NULL;
        header->next = NULL;
        return;
    }
    while(true){
        /* when you find a free block with a larger address, place newly free block before it */
        if (curr > header){ 
            if(curr->prev != NULL){ /* case in which you are at the beginning of freelist */
                curr->prev->next = header;
            }
            else if(curr->prev == NULL){ /* case in which newly added free block has the smallest address (is first) in freelist */
                freelist = header; /* set new block as head of freelist */
            }
            /* reaaranging pointers */
            header->prev = curr->prev;
            header->next = curr;
            curr->prev = header;
            break;
        }
        /* reached the last free block in freelist, the newly freed block has the largest address */
        if(curr->next == NULL){
            curr->next = header;
            header->prev = curr;
            header->next = NULL;
            break;
        }
        curr = curr->next;
    }
    coalesce(); 
    /*print_freelist();*/  
}

bool dmalloc_init() {
    
    /* Two choices:
     * 1. Append prologue and epilogue blocks to the start and the
     * end of the freelist
     *
     * 2. Initialize freelist pointers to NULL
     *
     * Note: We provide the code for 2. Using 1 will help you to tackle the
     * corner cases succinctly.
     */
    
    size_t max_bytes = ALIGN(MAX_HEAP_SIZE);
    /* returns heap_region, which is initialized to freelist */
    freelist = (metadata_t*) sbrk(max_bytes);
    /* Q: Why casting is used? i.e., why (void*)-1? */
    if (freelist == (void *)-1)
        return false;
    freelist->next = NULL;
    freelist->prev = NULL;
    freelist->size = max_bytes-METADATA_T_ALIGNED;
    return true;
}

/* for debugging; can be turned off through -NDEBUG flag*/
void print_freelist() {
    setvbuf(stdout, NULL, _IONBF, 0);
    metadata_t *freelist_head = freelist;
    while(freelist_head != NULL) {
        printf("\tFreelist Size:%zd, Head:%p, Prev:%p, Next:%p\t",
              freelist_head->size,
              freelist_head,
              freelist_head->prev,
              freelist_head->next);
        freelist_head = freelist_head->next;
    }
    printf("\n");
}
