#include "csapp.h"

static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) "
    "Gecko/20120305 Firefox/10.0.3\r\n";

void handle_request(int connfd);
void parse_uri(char *uri, char *hostname, char *port, char *path);
void build_request_headers(rio_t *client_rio, char *host_hdr, char *req_hdrs, char *hostname);

int main(int argc, char **argv) {
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        handle_request(connfd);
        Close(connfd);
    }
}
//handles the request.
void handle_request(int connfd) {
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], port[10], path[MAXLINE];
    char request[MAXLINE], headers[MAXLINE], host_hdr[MAXLINE];
    int serverfd;
    rio_t client_rio, server_rio;

    Rio_readinitb(&client_rio, connfd);
    if (!Rio_readlineb(&client_rio, buf, MAXLINE)) return;

    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) {
        fprintf(stderr, "501 Not Implemented: %s\n", method);
        return;
    }

    parse_uri(uri, hostname, port, path);
    build_request_headers(&client_rio, host_hdr, headers, hostname);

    sprintf(request, "GET %s HTTP/1.0\r\n%s%s\r\n", path, host_hdr, headers);

    serverfd = Open_clientfd(hostname, port);
    if (serverfd < 0) {
        fprintf(stderr, "Failed to connect to %s:%s\n", hostname, port);
        return;
    }

    Rio_readinitb(&server_rio, serverfd);
    Rio_writen(serverfd, request, strlen(request));

    ssize_t n;
    while ((n = Rio_readlineb(&server_rio, buf, MAXLINE)) > 0) {
        Rio_writen(connfd, buf, n);
    }

    Close(serverfd);
}

//Parse thorught URI 
void parse_uri(char *uri, char *hostname, char *port, char *path) {
    char *hostbegin = strstr(uri, "//") ? strstr(uri, "//") + 2 : uri;
    char *hostend = strpbrk(hostbegin, ":/");
    int hostlen = hostend ? hostend - hostbegin : strlen(hostbegin);
    strncpy(hostname, hostbegin, hostlen);
    hostname[hostlen] = '\0';

    if (hostend && *hostend == ':') {
        sscanf(hostend + 1, "%[^/]%s", port, path);
    } else {
        strcpy(port, "80");
        strcpy(path, strchr(hostbegin + hostlen, '/') ? strchr(hostbegin + hostlen, '/') : "/");
    }
}