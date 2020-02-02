#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include "cachelab.h"

#define BUFSIZE 256
int verbose = 0;

typedef struct cacheLine cline_t, *cline_p;
struct cacheLine {
    bool valid;
    unsigned int tag;  // must unsigned for logical shift
    cline_p prev;
    cline_p next;
};

typedef struct doubleLinkedList dlist_t, *dlist_p;
struct doubleLinkedList {
    int count;
    cline_p head;  // most recently
    cline_p tail;
};

dlist_p dlistInitial(int size) {
    dlist_p ptr = (dlist_p) malloc(sizeof(dlist_t));
    ptr->count = size;
    ptr->head = (cline_p) malloc(sizeof(cline_t));
    ptr->tail = (cline_p) malloc(sizeof(cline_t));
    ptr->head->next = ptr->tail;
    ptr->tail->prev = ptr->head;
    cline_p p = ptr->head, n = ptr->tail;
    for (int i = 0; i < size; ++i) {
        cline_p node = (cline_p) malloc(sizeof(cline_t));
        node->valid = false;
        node->prev = p, node->next = n;
        p->next = node, n->prev = node;
        p = node;  // n is the invariant
    }
    return ptr;
}

void dlistMove2Head(dlist_p ptr, cline_p cur) {
    if (cur == ptr->head->next)
        return;
    cline_p p , n;
    p = cur->prev, n = cur->next;
    p->next = n, n->prev = p;
    p = ptr->head, n = ptr->head->next;
    p->next = cur, n->prev = cur;
    cur->prev = p, cur->next = n;
}

int dlistFind(dlist_p ptr, int tag) {
    cline_p cur = ptr->head;
    while (cur->next != ptr->tail) {
        cur = cur->next;
        if (!cur->valid) {
            cur->valid = true;
            cur->tag = tag;
            dlistMove2Head(ptr, cur);
            return 1;  // miss but not evict
        }
        if (cur->tag == tag) {
            dlistMove2Head(ptr, cur);
            return 0;  // hit
        }
    }
    cur->tag = tag;
    dlistMove2Head(ptr, cur);
    return 2;  // miss and evict
}

void dlistFree(dlist_p ptr) {
    cline_p cur = ptr->head;
    cline_p next = cur->next;
    for (int i = 0; i < ptr->count; ++i) {
        cur = next, next = next->next;
        free(cur);
    }
    free(ptr->head), free(ptr->tail);
    free(ptr);
}

static void usage(char *cmd) {
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n",  cmd);
    printf("\t-h         Print this information.\n");
    printf("\t-v         Optional verbose flag.\n");
    printf("\t-s <num>   Number of set index bits.\n");
    printf("\t-E <num>   Number of lines per set.\n");
    printf("\t-b <num>   Number of block offset bits.\n");
    printf("\t-t <file>  Trace file\n");
    exit(0);
}

void update(int * count, char * dst, char * src) {
    ++*count;
    strncat(dst, " ", 20);
    strncat(dst, src, 20);
}

int main(int argc, char* argv[])
{  
    /************** parse input ***************/
    char buf[BUFSIZE];
    char *file_name = NULL;
    int s, E, b;
    int c;
    while ((c = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch(c) {
        case 'h':
            usage(argv[0]);
            break;
        case 'v':
            verbose = 1;
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            strncpy(buf, optarg, BUFSIZE);
            buf[BUFSIZE - 1] = '\0';
            file_name = buf;
            break;                                      
        default:
            printf("unknown function '%c\n", c);
            usage(argv[0]);
            break;
        }
    }

    /************ initialization ************/
    FILE *fp;
    if ((fp = fopen(file_name, "r")) == NULL) {
        printf("no such file");
        return 1;
    }
    
    dlist_p *cache_array;  // array of pointers
    int set_len = 1 << s;
    int t = 64 - s - b;
    cache_array = (dlist_p*) malloc(sizeof(dlist_p) * set_len);
    for (int i = 0; i < set_len; ++i){
        cache_array[i] = dlistInitial(E);
    } 

    /************** running ***************/
    char line[50], instruct, message[20];
    dlist_p ptr;
    unsigned int addr, block;
    unsigned int tag, set;
    int hit = 0, miss = 0, evict = 0;
    int flag;
    while (fgets(line, 50, fp) != NULL) {
        if (line[0] == 'I')
            continue;
        message[0] = '\0';
        sscanf(line, " %c %x,%d", &instruct, &addr, &block);
        tag = addr >> (s + b);
        set = (addr << t) >> (t + b);
        ptr = cache_array[set];
        flag = dlistFind(ptr, tag);
        if (flag == 0) {
            update(&hit, message, "hit");
            if (instruct == 'M') {
                update(&hit, message, "hit");
            }
        }
        else {
            update(&miss, message, "miss");            
            if (flag == 2)
                update(&evict, message, "eviction");                       
            if (instruct == 'M') 
                update(&hit, message, "hit");
        }
        if (verbose) {
            // printf("%x, %d, %d\n", addr, tag, set);
            printf("%c %d,%d%s\n", instruct, addr, block, message);
        }
    }
    printSummary(hit, miss, evict);

    /**************   clean   ***************/
    for (int i = 0; i < set_len; ++i)
        free(cache_array[i]);
    free(cache_array);
    fclose(fp);
    return 0;
}
