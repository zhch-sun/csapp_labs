#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* For colored printf */
#define CNORMAL  "\x1B[0m"
#define CRED     "\x1B[31m"
#define CYELLOW  "\x1B[33m"

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 \
        (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void doit(int fd);
void clienterror(int fd, char *cause, char *errnum, 
                 char *shortmsg, char *longmsg);
void read_requesthdrs(rio_t *rp, char *host, char *headers);  
void to_server(int srcfd, int clientfd, char *headers, char *uri); 
void *thread(void *vargp);


typedef struct CacheLine {
    char name[MAXLINE];
    char object[MAX_OBJECT_SIZE];
} cacheLine_t;

struct Cache {
    int used_cnt;
    cacheLine_t objects[10];
} cache;

int read_cnt = 0;
sem_t mutex, w;

int reader(int fd, char *uri) {
    P(&mutex);
    read_cnt++;
    if (read_cnt == 1)
        P(&w);
    V(&mutex);

    /* read start */
    for (int i = 0; i < cache.used_cnt; i++) {
        if (!strcmp(cache.objects[i].name, uri)) {
            Rio_writen(fd, cache.objects[i].object, MAX_OBJECT_SIZE);
            return 1;
        }
    }
    /* read end */

    P(&mutex);
    read_cnt--;
    if(read_cnt == 0)
        V(&w);
    V(&mutex);
    return 0;
}

void writer(char *uri, char *buf) {
    P(&w);
    strncpy(cache.objects[cache.used_cnt].name, uri, strlen(uri));
    strncpy(cache.objects[cache.used_cnt].object, buf, strlen(buf));
    cache.used_cnt++;
    V(&w);
}

int main(int argc, char **argv)
{
    int listenfd;  // , connfd
    int *connfdp;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    Sem_init(&mutex, 0, 1);
    Sem_init(&w, 0, 1);
    read_cnt = 0;

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);  
        Pthread_create(&tid, NULL, thread, connfdp);  
        // doit(connfd);
        // Close(connfd);
    }
    return 0;
}

void *thread(void *vargp) 
{
    printf("in thread\n");
    int connfd = *((int *) vargp);
    printf("connfd %d\n", connfd);
    Pthread_detach(pthread_self());
    Free(vargp);
    doit(connfd);
    Close(connfd);
    return NULL;
}

/*
 * doit - connect to server, forward/send back transaction
 */
void doit(int fd) 
{
    // int is_static;
    // struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], url[MAXLINE], version[MAXLINE];
    char request_line[MAXLINE], host[MAXLINE], uri[MAXLINE];
    char headers[MAXLINE], host_name[MAXLINE], host_port[MAXLINE];
    // char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))
        return;
    // read_requesthdrs(&rio);
    printf("%s", CYELLOW);
    printf("input request line: %s", buf);  // do not need \n
    sscanf(buf, "%s %s %s", method, url, version);
    if (strcasecmp(method, "GET")) {                
        clienterror(fd, method, "501", "Not Implemented",
                    "Proxy does not implement this method");
        return;
    }
    /* Parse URL */
    char *host_s, *host_e;
    if ((host_s = strstr(url, "//"))) host_s += 2;
    else host_s = url;
    
    if ((host_e = strstr(host_s, "/")) == NULL) {
        // no uri
        strncpy(host, host_s, MAXLINE - 1); 
        strncpy(uri, "/", 2);
    }
    else {  // find uri
        *host_e = '\0';
        strncpy(host, host_s, MAXLINE - 1);
        *host_e = '/';
        strncpy(uri, host_e, MAXLINE - 1);
    }
    if(reader(fd, uri) == 1)  // read cache
        return;
    int len = strlen(version);
    version[len - 1] = '0';
    sprintf(request_line, "%s %s %s\n", method, uri, version);
    printf("%s\n", CNORMAL);   

    /* Parse request headers */
    read_requesthdrs(&rio, host, headers);
    strcat(request_line, headers);
    printf("%s", CRED);
    char *hostp = host + 6;  // exclude Host: 
    sscanf(hostp, "%[^: ]%*[: ]%s", host_name, host_port);
    printf("%s", hostp);
    printf("name: %s port: %s\n", host_name, host_port);

    int clientfd = open_clientfd(host_name, host_port);
    printf("%s\n", CNORMAL);

    to_server(fd, clientfd, request_line, uri);
}

void to_server(int src_fd, int clientfd, char *headers, char *uri) 
{
    char buf[MAXLINE];
    rio_t rio;
    rio_t *rp = &rio;
    char object_cache[MAX_OBJECT_SIZE];
    int object_size = 0;

    printf("%s", CYELLOW);
    // printf("start server\n");

    Rio_readinitb(&rio, clientfd);
    // printf("headers \n%s", headers);
    Rio_writen(clientfd, headers, strlen(headers));

    printf("write finished\n");
    printf("%s\n", CNORMAL);   
    
    // read headers
    int content_type = 0;
    int content_length = 0;
    char tmp1[MAXLINE], tmp2[MAXLINE];
    Rio_readlineb(&rio, buf, MAXLINE);  
    printf("%s", buf);  // HTTP/1.0 200 OK
    Rio_writen(src_fd, buf, strlen(buf));
    strncpy(object_cache, buf, strlen(buf));
    object_size += strlen(buf);

    while(strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
        if (strstr(buf, "length")) {
            sscanf(buf, "%s %s", tmp1, tmp2);
            content_length = atoi(tmp2);
            printf("c len %s\n", tmp2);
        }
        else if (strstr(buf, "type"))
        {
            if (strstr(buf, "html"))
                ;
            else
                content_type = 1;
            printf("c type %d\n", content_type);
        }
        Rio_writen(src_fd, buf, strlen(buf));
        strncpy(object_cache + object_size, buf, strlen(buf));
        object_size += strlen(buf);
    }

    // read content
    printf("%s", CYELLOW);
    if (content_type == 0) {  // text
        int n;
        while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
            Rio_writen(src_fd, buf, strlen(buf));
            strncpy(object_cache + object_size, buf, strlen(buf));
            object_size += strlen(buf);
        }
    }
    else {
        printf("1111\n");
        char *bbuf = (char *) malloc(content_length);
        Rio_readn(clientfd, bbuf, content_length);
        // printf("%s", bbuf);
        Rio_writen(src_fd, bbuf, content_length);
        strncpy(object_cache + object_size, buf, strlen(buf));
        object_size += strlen(buf);
    }
    printf("%s\n", CNORMAL);   
    Close(clientfd);
    if (object_size < MAX_OBJECT_SIZE) {
        writer(uri, object_cache);
    }
}

/*
 * read_requesthdrs - read HTTP request headers
 */
void read_requesthdrs(rio_t *rp, char *host, char *headers)
{
    char buf[MAXLINE], hdr_name[MAXLINE], hdr_content[MAXLINE];
    char tmp[MAXLINE];
    char *fixed_header = "User-Agent:Connection:Proxy-Connection:";
    char *other_header = "Connection: close\nProxy-Connection: close\n\r\n";

    printf("%s", CYELLOW);
    // printf("start read header\n");
    Rio_readlineb(rp, buf, MAXLINE);
    char *start = tmp;
    // printf("%s", buf);
    while(strcmp(buf, "\r\n")) {
        sscanf(buf, "%s %s", hdr_name, hdr_content);
        if (strstr(fixed_header, hdr_name)) {
            // printf("header not used.. %s", buf);
        }
        else if (!strcmp("Host:", hdr_name)) {
            // printf("rewrite host.. %s", buf);
            strncpy(host, buf, MAXLINE);
        }
        else {
            // printf("header used.. %s", buf);
            strncpy(start, buf, MAXLINE);
            start += strlen(buf);
        }
        Rio_readlineb(rp, buf, MAXLINE);
    }
    start = headers;
    strncpy(start, host, MAXLINE);
    start += strlen(host);
    strncpy(start, tmp, MAXLINE);
    start += strlen(tmp);
    strncpy(start, user_agent_hdr, MAXLINE);
    start += strlen(user_agent_hdr);
    strncpy(start, other_header, MAXLINE);
    printf("headers:\n%s", headers);
    printf("%s\n", CNORMAL);       
    return;
}



/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum, 
                 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
