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
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)
#define ALIGN(size) (((size) + (DSIZE-1)) & ~0x7)
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define PACK(size, alloc) ((size) | (alloc))
#define PUT(p, val)  (*(unsigned int*)(p) = (val))
#define GET(p)       (*(unsigned int*)(p))
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp) - WSIZE) // header p
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

static char * heap_listp;  // pointer to byte

/*---------------function prototypes---------------*/
static void *expand_heap(size_t size);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t in_size);
static int mm_check(void);

/* 
 * mm_init - initialize the malloc package.
 * return 0 if norm -1 if error
 */
int mm_init(void) {
    if ((heap_listp = (char *)mem_sbrk(4 * WSIZE)) == (void*) -1)
        return -1;
    PUT(heap_listp + 0 * WSIZE, 0);  // for align
    PUT(heap_listp + 1 * WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp + 2 * WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp + 3 * WSIZE, PACK(0, 1));
    heap_listp += 2 * WSIZE;  // as if bp of prologue
    if (expand_heap(CHUNKSIZE) == NULL)
        return -1;
    // mm_check();        
    return 0;
}

/* 
 * mm_malloc - 
 *    Allocate a block by incrementing the brk pointer.
 *    Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    int asize = ALIGN(size + DSIZE);  // Dsize is head+foot
    if (size == 0)  // ignore wrong inputs
        return NULL;
    void *bp = find_fit(asize);
    if (bp == NULL && (bp = expand_heap(MAX(asize, CHUNKSIZE))) == NULL)
        return NULL;
    place(bp, asize);
    // mm_check();
    return bp;
}

/*
 * mm_free - Freeing a block. then coalesce. 
 */
void mm_free(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
    // mm_check();
}

/*
 * mm_realloc - 
 * Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t in_size) {
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
    return new_bp;       
}

/*
 * expand_heap - expand heap by bytes, ceil to 8 bytes
 * return NULL if error else block pointer
 */
static void *expand_heap(size_t size) {
    // note no pointer +- and dereference!
    void *bp;
    size_t asize = ALIGN(size);
    if ((bp = mem_sbrk(asize)) == (void *)-1)
        return NULL;
    PUT(HDRP(bp), PACK(asize, 0));
    PUT(FTRP(bp), PACK(asize, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    return (void *)bp;
}

/*
 * coalesce - merge free block with adjacent blocks
 * return the original block pointer
 */
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    if (prev_alloc & next_alloc)
        return bp;  // shortcut, can be ommited
    if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    }
    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
    }
    else {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + \
            GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);   
    }
    // conditional move is error prone
    // size += !prev_alloc ? GET_SIZE(HDRP(PREV_BLKP(bp))) : 0;
    // size += !next_alloc ? GET_SIZE(HDRP(NEXT_BLKP(bp))) : 0;
    // bp = !prev_alloc ? PREV_BLKP(bp) : bp;
    PUT(HDRP(bp), PACK(size, 0)); // put right size at header
    PUT(FTRP(bp), PACK(size, 0)); // correct footer  
    return bp;
}

/*
 * find_fit - find first fit block given an aligned size
 * return NULL if no such block else block pointer
 */
static void *find_fit(size_t asize) {
    void *bp = NEXT_BLKP(heap_listp);
    while (GET_SIZE(HDRP(bp)) > 0) {  // forget HERP...
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize)
            return bp;
        bp = NEXT_BLKP(bp);    
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
    if (diff >= 2 * DSIZE) {
        PUT(HDRP(bp), PACK(target_size, 1));
        PUT(FTRP(bp), PACK(target_size, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(diff, 0));
        PUT(FTRP(bp), PACK(diff, 0));
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
static int mm_check(void) {
    char * bp = heap_listp;
    int i = 0;
    printf("start checking\n");
    while (GET_SIZE(HDRP(bp)) > 0) {
        if ((void *)(HDRP(bp)) > mem_heap_hi())
            printf("header overflow!!!!\n");
        if ((void *)(HDRP(bp)) < mem_heap_lo())
            printf("header underflow!!!!\n");            
        if ((void *)(FTRP(bp)) > mem_heap_hi())
            printf("footer overflow!!!!\n");     
        if ((void *)(FTRP(bp)) < mem_heap_lo())
            printf("footer underflow!!!!\n");                       
        if (GET(HDRP(bp)) != GET(FTRP(bp)))
            printf("header not equal to footer!!!!\n");
        if (GET_SIZE(HDRP(bp)) > mem_heapsize())
            printf("size overflow!!!!\n");
        printf("block size: %d, alloc %d\n", \
            GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)));
        bp = NEXT_BLKP(bp);
        ++i;
    }
    printf("block size: %d, alloc %d\n", \
            GET_SIZE(HDRP(bp)), GET_ALLOC(HDRP(bp)));
    printf("new check with %d blocks\n\n", i + 1);
    return 1;
}
