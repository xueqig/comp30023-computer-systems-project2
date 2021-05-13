#define _POSIX_C_SOURCE 200112L
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "helper1.h"

uint8_t *query_server(char *node, char *service, uint8_t buffer[], int buf_len);

int main(int argc, char *argv[])
{
    // Act as a server to accept query from client (dig)
    int sockfd, newsockfd, n, re, s;
    uint8_t req_buf[256];
    struct addrinfo hints, *res;
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size;

    // if (argc < 2)
    // {
    //     fprintf(stderr, "ERROR, no port provided\n");
    //     exit(EXIT_FAILURE);
    // }

    // Create address we're going to listen on (with given port number)
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // for bind, listen, accept
    // node (NULL means any interface), service (port), hints, res
    s = getaddrinfo(NULL, "8053", &hints, &res);
    if (s != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    // Create socket
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Reuse port if possible
    re = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    // Bind address to the socket
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(res);

    // Listen on socket - means we're ready to accept connections,
    // incoming connection requests will be queued, man 3 listen
    if (listen(sockfd, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Accept a connection - blocks until a connection is ready to be accepted
    // Get back a new file descriptor to communicate on
    client_addr_size = sizeof client_addr;
    newsockfd =
        accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_size);
    if (newsockfd < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // Read characters from the connection, then process
    n = read(newsockfd, req_buf, 255); // n is number of characters read
    if (n < 0)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }
    // Null-terminate string
    req_buf[n] = '\0';

    int qr = get_qr(req_buf);
    char *qname = get_qname(req_buf);
    int qtype = get_qtype(req_buf);

    printf("qr: %d\n", qr);
    printf("qname: %s\n", qname);
    printf("qtype: %d\n", qtype);

    /////////////////////////////////////////////////////////////////
    // Act as a client to query upperstream server
    char *us_svr_ip = argv[1];
    char *us_svr_port = argv[2];
    printf("us_svr_ip: %s\n", us_svr_ip);
    printf("us_svr_port: %s\n", us_svr_port);

    uint8_t *res_buf;
    res_buf = query_server(us_svr_ip, us_svr_port, req_buf, n);

    int res_qr = get_qr(res_buf);
    char *res_qname = get_qname(res_buf);
    int res_qtype = get_qtype(res_buf);

    printf("res qr: %d\n", res_qr);
    printf("res qname: %s\n", res_qname);
    printf("res qtype: %d\n", res_qtype);

    //////////////////////////////////////////////////////////////////
    // Write message back
    n = write(newsockfd, res_buf, 70);
    if (n < 0)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }

    printf("after write\n");

    close(sockfd);
    close(newsockfd);
    return 0;
}

// send query to upstream server and return an response
uint8_t *query_server(char *node, char *service, uint8_t buffer[], int buf_len)
{
    int sockfd, n, s;
    struct addrinfo hints, *servinfo, *rp;
    // char buffer[256];

    // if (argc < 3)
    // {
    //     fprintf(stderr, "usage %s hostname port\n", argv[0]);
    //     exit(EXIT_FAILURE);
    // }

    // Create address
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // Get addrinfo of server. From man page:
    // The getaddrinfo() function combines the functionality provided by the
    // gethostbyname(3) and getservbyname(3) functions into a single interface
    // // argv[1] is my ip address, argv[2] is port
    s = getaddrinfo(node, service, &hints, &servinfo);
    if (s != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    // Connect to first valid result
    // Why are there multiple results? see man page (search 'several reasons')
    // How to search? enter /, then text to search for, press n/N to navigate
    for (rp = servinfo; rp != NULL; rp = rp->ai_next)
    {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1)
            continue;

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break; // connection success

        close(sockfd);
    }
    if (rp == NULL)
    {
        // // connection fail
        fprintf(stderr, "client: failed to connect\n");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(servinfo);

    // Send message to server
    n = write(sockfd, buffer, buf_len);
    if (n < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Read message from server
    n = read(sockfd, buffer, 255);
    if (n < 0)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }
    // Null-terminate string
    buffer[n] = '\0';

    printf("n: %d\n", n);

    close(sockfd);

    return buffer;
}
