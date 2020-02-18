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

/*---------------   debug macros   ---------------*/
#ifdef DEBUG
#define DBG_PRINTF(...) fprintf(stderr, __VA_ARGS__)
#define VERB 2
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

/*---------------   explicit macros   ---------------*/
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)
#define ALIGN(size) (((size) + (DSIZE-1)) & ~0x7)
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// core is PUT and GET, all pointer dereference
// #define PACK(size, alloc) ((size) | (alloc))
#define PACKP(size, alloc, palloc) ((size) | (alloc) | (palloc << 1))
#define PUT(p, val)  (*(unsigned int*)(p) = (unsigned int)(val))
#define GET(p)       (*(unsigned int*)(p))
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PALLOC(p) ((GET(p) >> 1) & 0x1)

#define HDRP(bp) ((char *)(bp) - WSIZE) // header p
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

#define SECT_NEXT(bp) ((char *)(bp))
#define SECT_PREV(bp) ((char *)(bp) + WSIZE)
#define NEXT_FREP(bp) ((char *)GET(SECT_NEXT(bp)))
#define PREV_FREP(bp) ((char *)GET(SECT_PREV(bp)))

static char * heap_listp;  // pointer to byte
static char * free_listp;  // pointer diff to heap_listp

/*--------------- segregated macros   ---------------*/
#define NUM_BINS 128
#define NUM_SMALL_BINS (NUM_BINS / 2)  // 64bins [0, 8, 16, ..., 504] bytes


/*---------------function prototypes---------------*/
static void *expand_heap(size_t size);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t in_size);
static void add_free_list(void *bp);
static void delete_free_list(void *bp);

#ifdef DEBUG
static int mm_check(int verbose);
#endif

/* 
 * mm_init - initialize the malloc package.
 * return 0 if norm -1 if error
 */
int mm_init(void) {
    DBG_PRINTF("start mm_init\n");
    if ((free_listp = (char *)mem_sbrk((NUM_SMALL_BINS * 2 + 4) * WSIZE)) \
        == (void*) -1)
        return -1;
    // initial 64 small bins [0, 8, 16, ..., 504], first 2 art not used
    for(int i = 0; i < NUM_SMALL_BINS; i++) {
        PUT(free_listp + (i*2  ) * WSIZE, free_listp + (i*2) * WSIZE);
        PUT(free_listp + (i*2+1) * WSIZE, free_listp + (i*2) * WSIZE);
    }
    heap_listp = free_listp + NUM_SMALL_BINS * 2 * WSIZE;
    PUT(heap_listp + 0 * WSIZE, 0);  // for align    
    PUT(heap_listp + 1 * WSIZE, PACKP(DSIZE, 1, 1));
    PUT(heap_listp + 2 * WSIZE, PACKP(DSIZE, 1, 1));
    PUT(heap_listp + 3 * WSIZE, PACKP(0, 1, 1));
    heap_listp += 2 * WSIZE;
    if (expand_heap(CHUNKSIZE) == NULL)
        return -1;
    CHECKHEAP(VERB);       
    return 0;
}

/* 
 * mm_malloc - 
 *    Allocate a block by incrementing the brk pointer.
 *    Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    DBG_PRINTF("start mm_malloc %d\n", malloc_count);
    int asize = ALIGN(size + WSIZE);  // Dsize is head
    if (size == 0)  // ignore wrong inputs
        return NULL;
    void *bp = find_fit(asize);
    if (bp == NULL && (bp = expand_heap(MAX(asize + DSIZE, CHUNKSIZE))) == NULL)
        return NULL;
    place(bp, asize);
    CHECKHEAP(VERB);
    MALLOC_ADD;
    return bp;
}

/*
 * mm_free - Freeing a block. then coalesce. 
 */
void mm_free(void *bp) {
    DBG_PRINTF("start mm_free %d\n", free_count);
    // free current block
    unsigned int size = GET_SIZE(HDRP(bp));
    unsigned int palloc = GET_PALLOC(HDRP(bp));
    PUT(HDRP(bp), PACKP(size, 0, palloc));
    PUT(FTRP(bp), PACKP(size, 0, palloc));
    // change next block
    void *nbp = NEXT_BLKP(bp);  // next block
    unsigned int nsize = GET_SIZE(HDRP(nbp));
    unsigned int nalloc = GET_ALLOC(HDRP(nbp));
    PUT(HDRP(nbp), PACKP(nsize, nalloc, 0));
    if (nsize > 0)
        PUT(FTRP(nbp), PACKP(nsize, nalloc, 0)); 
    // coalesce
    coalesce(bp);
    FREE_ADD;
    CHECKHEAP(VERB);
}

/*
 * mm_realloc - 
 * Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t in_size) {
    DBG_PRINTF("start mm_realloc %d\n", realloc_count);
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
    REALLOC_ADD;
    CHECKHEAP(VERB);
    return new_bp;       
}

/*
 * expand_heap - expand heap by bytes, ceil to 8 bytes
 * return NULL if error else block pointer
 */
static void *expand_heap(size_t size) {
    void *bp;
    unsigned int asize = ALIGN(size);
    if ((bp = mem_sbrk(asize)) == (void *)-1)
        return NULL;
    // Note can't use GET_ALLOC(HDRP(PREV_BLCK(bp)))!
    unsigned int palloc = GET_PALLOC(HDRP(bp));
    PUT(HDRP(bp), PACKP(asize, 0, palloc));
    PUT(FTRP(bp), PACKP(asize, 0, palloc));
    PUT(HDRP(NEXT_BLKP(bp)), PACKP(0, 1, 0));
    bp = coalesce(bp);
    return (void *)bp;
}

/*
 * coalesce - merge free block with adjacent blocks
 * input: block pointer that not in free list
 * return the new block pointer
 */
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_PALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    if (prev_alloc & next_alloc) {
        add_free_list(bp);
    }
    else if (prev_alloc && !next_alloc) {
        delete_free_list(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACKP(size, 0, 1)); // put right size at header
        PUT(FTRP(bp), PACKP(size, 0, 1)); // correct footer  
        add_free_list(bp);
    }
    else if (!prev_alloc && next_alloc) {
        delete_free_list(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        unsigned int ppalloc = GET_PALLOC(HDRP(bp));
        PUT(HDRP(bp), PACKP(size, 0, ppalloc)); // put right size at header
        PUT(FTRP(bp), PACKP(size, 0, ppalloc)); // correct footer 
        add_free_list(bp); 
    }
    else {
        delete_free_list(NEXT_BLKP(bp));
        delete_free_list(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + \
            GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);   
        unsigned int ppalloc = GET_PALLOC(HDRP(bp));
        PUT(HDRP(bp), PACKP(size, 0, ppalloc)); // put right size at header
        PUT(FTRP(bp), PACKP(size, 0, ppalloc)); // correct footer  
        add_free_list(bp); 
    }
    void *nbp = NEXT_BLKP(bp);  // next block
    unsigned int nsize = GET_SIZE(HDRP(nbp));
    unsigned int nalloc = GET_ALLOC(HDRP(nbp));
    PUT(HDRP(nbp), PACKP(nsize, nalloc, 0));
    if (nsize != 0)  // the last block don't have footer
        PUT(FTRP(nbp), PACKP(nsize, nalloc, 0));     
    return bp;
}

/*
 * find_fit - find first fit block given an aligned size
 * return NULL if no such block else block pointer
 */
static void *find_fit(size_t asize) {
    void *bp, *fp;
    if (asize < 512) {
        for(int i = asize / 8; i < NUM_SMALL_BINS; ++i) {
            bp = fp = (void *) (free_listp + i * DSIZE);
            while ((bp = NEXT_FREP(bp)) != fp) {
                if (GET_SIZE(HDRP(bp)) >= asize)
                    return (void *)bp;
            }            
        }
    }
    // not found in small bins or >= 512 bytes;
    bp = free_listp;
    while ((bp = NEXT_FREP(bp)) != free_listp) {
        if (GET_SIZE(HDRP(bp)) >= asize)
            return (void *)bp;
    }
    return NULL;
}

/*
 * place - convert block to allocated, split if necessary
 * input aligned target block asize
 * return nothing
 */
static void place(void *bp, size_t target_size) {
    size_t real_size = GET_SIZE(HDRP(bp));  // real block size
    size_t diff = real_size - target_size;  // must > 0
    // Note can't use GET_ALLOC(HDRP(PREV_BLKP(bp))) !!
    unsigned int palloc = GET_PALLOC(HDRP(bp)); 
    delete_free_list(bp);
    if (diff >= 2 * DSIZE) {
        PUT(HDRP(bp), PACKP(target_size, 1, palloc));
        PUT(FTRP(bp), PACKP(target_size, 1, palloc));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACKP(diff, 0, 1));
        PUT(FTRP(bp), PACKP(diff, 0, 1));
        coalesce(bp);
    }
    else {
        PUT(HDRP(bp), PACKP(real_size, 1, palloc));
        PUT(FTRP(bp), PACKP(real_size, 1, palloc));

        void *nbp = NEXT_BLKP(bp);  // next block
        unsigned int nsize = GET_SIZE(HDRP(nbp));
        unsigned int nalloc = GET_ALLOC(HDRP(nbp));
        PUT(HDRP(nbp), PACKP(nsize, nalloc, 1));
        if (nsize != 0)  // the last block don't have footer
            PUT(FTRP(nbp), PACKP(nsize, nalloc, 1));         
    }
}

/*
 * add_free_list - add block pointer to free lsit
 * return nothing
 */
static void add_free_list(void *bp) {
    // get correct free list
    void *fp;
    unsigned int size = GET_SIZE(HDRP(bp));
    if (size >= 512)
        fp = free_listp;
    else
        fp = free_listp + size / 8 * DSIZE;

    // LIFO  faster
    // char *a = fp;
    // char *b = NEXT_FREP(fp);
    // FIFO  better space
    char *a = PREV_FREP(free_listp);
    char *b = free_listp;  

    PUT(SECT_NEXT(bp), b);
    PUT(SECT_PREV(bp), a);
    PUT(SECT_NEXT(a), bp);
    PUT(SECT_PREV(b), bp);    
}

/*
 * delete_free_list - remove block pointer from free lsit
 * return nothing
 */
static void delete_free_list(void *bp) {
    char *a = PREV_FREP(bp);
    char *b = NEXT_FREP(bp);   
    PUT(SECT_NEXT(a), b);
    PUT(SECT_PREV(b), a);     
}

/*
 * mm_check(void) - check any consistensy problem
 * return nothing
 */
#ifdef DEBUG
static int mm_check(int verbose) {
    if (verbose == 0)
        return 1;
    char * bp = heap_listp;
    int num_block = 0;
    int num_free = 0, num_free_node = 0;
    // check heap sequetially
    while (GET_SIZE(HDRP(bp)) >= 0) {
        if (!GET_ALLOC(HDRP(bp))) {
            ++num_free;
        }
        if (verbose > 1) {
            DBG_PRINTF("block size: %d, alloc %d, palloc %d\n", \
                GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)), GET_PALLOC(HDRP(bp))); 
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
        if (!GET_ALLOC(HDRP(bp)) && GET(HDRP(bp)) != GET(FTRP(bp))) {
            DBG_PRINTF("header %d not equal to footer %d\n", \
                GET(HDRP(bp)), GET(FTRP(bp)));
            // bp = 0;
            // return *bp;
        }
        // if (GET_PALLOC(HDRP(bp)) != GET_ALLOC(HDRP(PREV_BLKP(bp))))
        //     DBG_PRINTF("palloc wrong!\n");
        if (GET_SIZE(HDRP(bp)) > mem_heapsize())
            DBG_PRINTF("size overflow!!!!\n");
        DBG_PRINTF("\033[0m");   
        if (GET_SIZE(HDRP(bp)) == 0)
            break;         
        bp = NEXT_BLKP(bp);
        ++num_block;
    } 
    // check free lists
    DBG_PRINTF("\033[0;31m");
    char *fp;
    for (int i = 0; i < NUM_SMALL_BINS; ++i) {
        fp = free_listp + i * DSIZE;
        void *bp = NEXT_FREP(fp);
        while (bp != fp) {
            if (GET_ALLOC(HDRP(bp)))
                DBG_PRINTF("allocated block in free list\n");
            if (SECT_NEXT(bp) == NULL || SECT_PREV(bp) == NULL)
                DBG_PRINTF("NULL next or prev pointer\n");
            ++num_free_node;     
            bp = NEXT_FREP(bp); 
        }        
    }
    if (num_free_node != num_free)
        DBG_PRINTF("actual free: %d, free list: %d\n", \
            num_free, num_free_node);
    DBG_PRINTF("\033[0m");            
    if (verbose > 1) {
        DBG_PRINTF("new check %d with %d blocks and %d "
            "free blocks \n\n", verbose, num_block + 1, num_free);
    }
    return 1;
}
#endif
