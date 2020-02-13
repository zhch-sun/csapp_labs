/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "config.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Hahaha",
    /* First member's full name */
    "zhicheng sun",
    /* First member's email address */
    "zhicheng@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/*---------------       macros       ---------------*/
#ifdef DEBUG
#define DBG_PRINTF(...) fprintf(stderr, __VA_ARGS__)
#define CHECKHEAP(verbose) mm_check(verbose)
#define MALLOC_ADD ++malloc_count
#define FREE_ADD ++free_count
#define REALLOC_ADD ++realloc_count
static int malloc_count;
static int free_count;
static int realloc_count;

#else
#define DBG_PRINTF(...)
#define CHECKHEAP(verbose)
#define MALLOC_ADD 
#define FREE_ADD
#define REALLOC_ADD
#endif

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)
#define ALIGN(size) (((size) + (DSIZE-1)) & ~0x7)
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// core is PUT and GET, all pointer dereference
#define PACK(size, alloc) ((size) | (alloc))
#define PUT(p, val)  (*(unsigned int*)(p) = (unsigned int)(val))
#define GET(p)       (*(unsigned int*)(p))
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_BLOCKP(p)  ((char *)(p) + WSIZE)

#define HDRP(bp) ((char *)(bp) - WSIZE) // header p
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

#define SECT_NEXT(p) ((char *)(p) + WSIZE)
#define SECT_PREV(p) ((char *)(p) + DSIZE)
#define NEXT_FREP(p) ((char *)GET(SECT_NEXT(p)))
#define PREV_FREP(p) ((char *)GET(SECT_PREV(p)))

static char * heap_listp;  // pointer to byte
static char * free_listp;  // pointer diff to heap_listp

/*---------------function prototypes---------------*/
static void *expand_heap(size_t size);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t in_size);
static void add_free_list(void *p);
static void delete_free_list(void *p);

#ifdef DEBUG
static int mm_check(int verbose);
#endif

/*
 * add_free_list - add pointer to free lsit
 * return nothing
 */
static void add_free_list(void *p) {
    if (free_listp == NULL) {
        free_listp = p;
        PUT(SECT_NEXT(p), p);
        PUT(SECT_PREV(p), p);
    }
    else {
        char *a = free_listp;
        char *b = NEXT_FREP(free_listp);
        PUT(SECT_NEXT(p), b);
        PUT(SECT_PREV(p), a);
        PUT(SECT_NEXT(a), p);
        PUT(SECT_PREV(b), p);
    }
}

/*
 * add_free_list - add pointer to free lsit
 * return nothing
 */
static void delete_free_list(void *p) {
    char *a = PREV_FREP(p);
    char *b = NEXT_FREP(p);
    if (p == free_listp && a == free_listp) {  // difficult...
        free_listp = NULL;  // corner case..
    }
    else {
        PUT(SECT_NEXT(a), b);
        PUT(SECT_PREV(b), a);
        if (p == free_listp)
            free_listp = a;  // difficult.. update free_listp..
    }
}

/* 
 * mm_init - initialize the malloc package.
 * return 0 if norm -1 if error
 */
int mm_init(void) {
    DBG_PRINTF("start mm_init\n");
    if ((heap_listp = (char *)mem_sbrk(4 * WSIZE)) == (void*) -1)
        return -1;
    PUT(heap_listp + 0 * WSIZE, 0);  // for align
    PUT(heap_listp + 1 * WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp + 2 * WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp + 3 * WSIZE, PACK(0, 1));  // epilogue size 0 for find_fit
    heap_listp += 2 * WSIZE;  // as if bp of prologue
    if (expand_heap(CHUNKSIZE) == NULL)
        return -1;
    CHECKHEAP(1);       
    return 0;
}

/* 
 * mm_malloc - 
 *    Allocate a block by incrementing the brk pointer.
 *    Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    DBG_PRINTF("start mm_malloc %d\n", malloc_count);
    MALLOC_ADD;
    int asize = ALIGN(size + DSIZE);  // Dsize is head+foot
    if (size == 0)  // ignore wrong inputs
        return NULL;
    void *bp = find_fit(asize);
    if (bp == NULL && (bp = expand_heap(MAX(asize, CHUNKSIZE))) == NULL)
        return NULL;
    place(bp, asize);
    CHECKHEAP(1);
    return bp;
}

/*
 * mm_free - Freeing a block. then coalesce. 
 */
void mm_free(void *bp) {
    DBG_PRINTF("start mm_free %d\n", free_count);
    FREE_ADD;
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    // add_free_list(HDRP(bp));
    coalesce(bp);
    CHECKHEAP(1);
}

/*
 * mm_realloc - 
 * Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t in_size) {
    DBG_PRINTF("start mm_realloc %d\n", realloc_count);
    REALLOC_ADD;
    if (in_size == 0) {
        free(bp);
        return NULL;
    }
    if (bp == NULL) {
        return malloc(in_size);
    }

    void *new_bp = mm_malloc(in_size);
    if (new_bp == NULL)
        return NULL;
    size_t copy_size = GET_SIZE(HDRP(bp));
    if (in_size < copy_size)
        copy_size = in_size;
    memcpy(new_bp, bp, copy_size);
    mm_free(bp);
    CHECKHEAP(1);
    return new_bp;       
}

/*
 * expand_heap - expand heap by bytes, ceil to 8 bytes
 * return NULL if error else block pointer
 */
static void *expand_heap(size_t size) {
    void *bp;
    size_t asize = ALIGN(size);
    if ((bp = mem_sbrk(asize)) == (void *)-1)
        return NULL;
    PUT(HDRP(bp), PACK(asize, 0));
    PUT(FTRP(bp), PACK(asize, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    bp = coalesce(bp);
    // add_free_list(HDRP(bp));
    return (void *)bp;
}

/*
 * coalesce - merge free block with adjacent blocks
 * input: block pointer that not in free list
 * return the new block pointer
 */
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    if (prev_alloc & next_alloc) {
        add_free_list(HDRP(bp));
        // return bp;  // shortcut, can be ommited
    }
    else if (prev_alloc && !next_alloc) {
        delete_free_list(HDRP(NEXT_BLKP(bp)));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0)); // put right size at header
        PUT(FTRP(bp), PACK(size, 0)); // correct footer  
        add_free_list(HDRP(bp));
    }
    else if (!prev_alloc && next_alloc) {
        delete_free_list(HDRP(PREV_BLKP(bp)));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0)); // put right size at header
        PUT(FTRP(bp), PACK(size, 0)); // correct footer 
        add_free_list(HDRP(bp)); 
    }
    else {
        delete_free_list(HDRP(NEXT_BLKP(bp)));
        delete_free_list(HDRP(PREV_BLKP(bp)));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + \
            GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);   
        PUT(HDRP(bp), PACK(size, 0)); // put right size at header
        PUT(FTRP(bp), PACK(size, 0)); // correct footer  
        PUT(FTRP(bp), PACK(size, 0)); // correct footer  
        PUT(FTRP(bp), PACK(size, 0)); // correct footer  
        PUT(FTRP(bp), PACK(size, 0)); // correct footer  
        PUT(FTRP(bp), PACK(size, 0)); // correct footer  
        add_free_list(HDRP(bp)); 
    }
    return bp;
}

/*
 * find_fit - find first fit block given an aligned size
 * return NULL if no such block else block pointer
 */
static void *find_fit(size_t asize) {
    // void *bp = NEXT_BLKP(heap_listp);
    // while (GET_SIZE(HDRP(bp)) > 0) {  // forget HERP...
    //     if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize)
    //         return bp;
    //     bp = NEXT_BLKP(bp);    
    // }
    // return NULL;
    if (free_listp == NULL)
        return NULL;
    void *p = free_listp;
    do {
        p = NEXT_FREP(p);
        if (GET_SIZE(p) >= asize)
            return (void *) GET_BLOCKP(p);
    } while (p != free_listp);
    return NULL;
}

/*
 * place - convert block to allocated, split if necessary
 * input aligned target block asize
 * return nothing
 */
static void place(void *bp, size_t target_size) {
    void *p = HDRP(bp);
    size_t real_size = GET_SIZE(HDRP(bp));  // real block size
    size_t diff = real_size - target_size;  // must > 0
    delete_free_list(p);
    if (diff >= 2 * DSIZE) {
        PUT(HDRP(bp), PACK(target_size, 1));
        PUT(FTRP(bp), PACK(target_size, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(diff, 0));
        PUT(FTRP(bp), PACK(diff, 0));
        // add_free_list(HDRP(bp));
        coalesce(bp);
    }
    else {
        PUT(HDRP(bp), PACK(real_size, 1));
        PUT(FTRP(bp), PACK(real_size, 1));
    }
}

/*
 * mm_check(void) - check any consistensy problem
 * return nothing
 */
#ifdef DEBUG
static int mm_check(int verbose) {
    char * bp = heap_listp;
    int num_block = 0;
    int num_free = 0, num_free_node = 0;
    
    do {
        if (!GET_ALLOC(HDRP(bp))) {
            ++num_free;
        }
        DBG_PRINTF("\033[0;31m");
        if ((void *)(HDRP(bp)) > mem_heap_hi())
            DBG_PRINTF("header overflow!!!!\n");
        if ((void *)(HDRP(bp)) < mem_heap_lo())
            DBG_PRINTF("header underflow!!!!\n");            
        if ((void *)(FTRP(bp)) > mem_heap_hi())
            DBG_PRINTF("footer overflow!!!!\n");     
        if ((void *)(FTRP(bp)) < mem_heap_lo())
            DBG_PRINTF("footer underflow!!!!\n");                       
        if (GET(HDRP(bp)) != GET(FTRP(bp)))
            DBG_PRINTF("header not equal to footer!!!!\n");
        if (GET_SIZE(HDRP(bp)) > mem_heapsize())
            DBG_PRINTF("size overflow!!!!\n");
        DBG_PRINTF("\033[0m");    
        DBG_PRINTF("block size: %d, alloc %d\n", \
            GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)));
        bp = NEXT_BLKP(bp);
        ++num_block;
    } while (GET_SIZE(HDRP(bp)) > 0);
    DBG_PRINTF("block size: %d, alloc %d\n", \
        GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)));

    DBG_PRINTF("\033[0;31m");
    if (free_listp != NULL) {
        void *p = free_listp;
        do {
            if (GET_ALLOC(p))
                DBG_PRINTF("allocated block in free list\n");
            if (SECT_NEXT(p) == NULL || SECT_PREV(p) == NULL)
                DBG_PRINTF("NULL next or prev pointer\n");
            ++num_free_node;     
            p = NEXT_FREP(p);            
        } while(p != free_listp);
    }
    if (num_free_node != num_free)
        DBG_PRINTF("actual free: %d, free list: %d\n", \
            num_free, num_free_node);
    DBG_PRINTF("\033[0m");            

    DBG_PRINTF("new check %d with %d blocks and %d "
        "free blocks \n\n", verbose, num_block + 1, num_free);
    return 1;
}
#endif
