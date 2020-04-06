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
void to_server(int clientfd, char *request_line);               

int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    // printf("%s", user_agent_hdr);
    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);    
        doit(connfd);
        Close(connfd);
    }
    return 0;
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
    
    int len = strlen(version);
    version[len - 1] = '0';
    sprintf(request_line, "%s %s %s", method, uri, version);
    printf("request line: %s", request_line);
    printf("%s\n", CNORMAL);   

    /* Parse request headers */
    read_requesthdrs(&rio, host, headers);
    char *hostp = host + 6;  // exclude Host: 
    sscanf(hostp, "%[^: ]%*[: ]%s", host_name, host_port);
    printf("%s", hostp);
    printf("name: %s port: %s\n", host_name, host_port);

    int clientfd = open_clientfd(host_name, host_port);
    to_server(clientfd, request_line);

    // if (is_static) { /* Serve static content */          
    //     if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { 
    //         clienterror(fd, filename, "403", "Forbidden",
    //                     "Tiny couldn't read the file");
    //         return;
    //     }
    //     serve_static(fd, filename, sbuf.st_size);        
    // }
    // else { /* Serve dynamic content */
    //     if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { 
    //         clienterror(fd, filename, "403", "Forbidden",
    //                     "Tiny couldn't run the CGI program");
    //         return;
    //     }
    //     serve_dynamic(fd, filename, cgiargs);           
    // }
}

void to_server(int clientfd, char *request_line) 
{
    char buf[MAXLINE];
    rio_t rio;
    rio_t *rp = &rio;

    Rio_readinitb(rp, clientfd);
    Rio_writen(clientfd, request_line, strlen(request_line));
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
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
