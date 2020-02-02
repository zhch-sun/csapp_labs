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
};

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

    cline_p *cache_array;  // array of pointers
    int set_len = 1 << s;
    int t = 64 - s - b;
    cache_array = (cline_p*) malloc(sizeof(cline_p) * set_len);
    for (int i = 0; i < set_len; ++i){
        cache_array[i] = (cline_p) malloc(sizeof(cline_t));
        cache_array[i]->valid = false;
        cache_array[i]->tag = 0;
    }

    /************** running ***************/
    char line[50], instruct, message[20];
    cline_p ptr;
    unsigned int addr, block;
    unsigned int tag, set;
    int hit = 0, miss = 0, evict = 0;
    while (fgets(line, 50, fp) != NULL) {
        if (line[0] == 'I')
            continue;
        message[0] = '\0';
        sscanf(line, " %c %x,%d", &instruct, &addr, &block);
        tag = addr >> (s + b);
        set = (addr << t) >> (t + b);
        ptr = cache_array[set];
        if (ptr->valid && tag == ptr->tag) {
            update(&hit, message, "hit");
            if (instruct == 'M') {
                update(&hit, message, "hit");
            }
        }
        else {
            ptr->tag = tag;
            update(&miss, message, "miss");            
            if (ptr->valid)
                update(&evict, message, "eviction");                       
            else 
                ptr->valid = true;
            if (instruct == 'M') 
                update(&hit, message, "hit");
        }
        if (verbose) {
            // printf("%x, %d, %d\n", addr, tag, set);
            printf("%c %d,%d%s\n", instruct, addr, block, message);
        }
    }
    if (verbose)
        printf("s: %d E: %d b: %d t: %s\n", s, E, b, file_name);
    printSummary(hit, miss, evict);

    /**************   clean   ***************/
    for (int i = 0; i < set_len; ++i)
        free(cache_array[i]);
    free(cache_array);
    fclose(fp);
    return 0;
}
