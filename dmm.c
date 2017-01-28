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

metadata_t* split(metadata_t* curr, size_t numbytes){
    metadata_t* ret = curr + sizeof(metadata_t);
    if (curr->size == numbytes){
        curr->prev->next = curr->next;
        curr->next->prev = curr->prev;
    }
    else {
        size_t request_size = (size_t) numbytes + sizeof(metadata_t);
        curr += request_size;
        curr->prev->next = curr;
        curr->next->prev = curr;
        curr->size = curr->size - request_size;
    }
    return ret;
}

void* dmalloc(size_t numbytes) {
  /* initialize through sbrk call first time */
  if(freelist == NULL) { 			
    if(!dmalloc_init())
      return NULL;
  }

  assert(numbytes > 0);

    metadata_t* curr = freelist;
    metadata_t* ret;
    while(curr->next != NULL){
        if (curr->size >= numbytes) {
            ret = split(curr, numbytes);
            break;
        }	
        curr = curr->next;
    }
  return ret;
}

void* coalesce() {
    metadata_t *curr = freelist;
    while(curr->next != NULL){
        if ((curr + curr->size) == curr->next){
            curr->size += curr->next->size;
            curr->next = curr->next->next;
            curr->next->prev = curr;
        }
        else {
        curr = curr->next;
        }
    }
}

void dfree(void* ptr) {
  /* your code here */
  /* sort freelist with respect to addresses so that you can coalesce in one pass of list */
	/* coalesce adjacent free blocks into one */
	/* coalesce - add the space of second block and its metadata to space in first block */
    
    metadata_t *curr = freelist;
    while(true){
        if (curr > ptr){
            ptr->prev = curr->prev;
            ptr->next = curr;
            curr->prev->next = ptr;
            curr->prev = ptr;
            break;
        }
        if(curr->next == NULL){
            curr->next = ptr;
            ptr->prev = curr;
            ptr->next = NULL;
            break;
        }
        curr = curr->next;
    }
    coalesce();
    
    
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
  metadata_t *freelist_head = freelist;
  while(freelist_head != NULL) {
    DEBUG("\tFreelist Size:%zd, Head:%p, Prev:%p, Next:%p\t",
	  freelist_head->size,
	  freelist_head,
	  freelist_head->prev,
	  freelist_head->next);
    freelist_head = freelist_head->next;
  }
  DEBUG("\n");
}
