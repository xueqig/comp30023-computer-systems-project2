#define _POSIX_C_SOURCE 200112L
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "helper1.h"

#define AAAA_ID 28

uint8_t *query_server(char *node, char *service, uint8_t buffer[], int buf_len, int *res_buf_len);
void handle_sigint(int sig);
int get_query_len(uint8_t *query_buf);

int main(int argc, char *argv[])
{
    signal(SIGINT, handle_sigint);

    while (1)
    {
        // Act as a server to accept query from client (dig)
        int sockfd, newsockfd, n, re, s, i;
        uint8_t req_buf[2048], buf[2048];
        struct addrinfo hints, *res;
        struct sockaddr_storage client_addr;
        socklen_t client_addr_size;

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
        int enable = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        {
            perror("setsockopt");
            exit(1);
        }

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
        n = read(newsockfd, req_buf, 2047); // n is number of characters read
        if (n < 0)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }

        int req_buf_len = get_query_len(req_buf);
        printf("query len: %d\n", req_buf_len);

        while (n != req_buf_len)
        {
            n += read(newsockfd, req_buf + n, 2047);

            if (n < 0)
            {
                perror("read");
                exit(EXIT_FAILURE);
            }
        }

        // Null-terminate string
        req_buf[n] = '\0';

        printf("req buf: \n");
        for (i = 0; i < n; i++)
        {
            printf("%02x ", req_buf[i]);
        }
        printf("\n");

        int qr = get_qr(req_buf);
        char *qname = get_qname(req_buf);
        int qtype = get_qtype(req_buf);

        printf("qr: %d\n", qr);
        printf("qname: %s\n", qname);
        printf("qtype: %d\n", qtype);
        write_log(qr, qname, qtype, NULL);

        // Check if request is AAAA
        if (qtype != AAAA_ID)
        {
            // Send request back to dig

            // Change qr to 0
            char qr_str[3];
            sprintf(qr_str, "%02x", req_buf[4]);
            qr_str[0] = '8';
            uint8_t new_qr = (int)strtol(qr_str, NULL, 16);
            req_buf[4] = new_qr;

            // Change ra to 1 and rcode to 4
            char ra_rcode_str[3];
            sprintf(ra_rcode_str, "%02x", req_buf[5]);
            ra_rcode_str[0] = '8';
            ra_rcode_str[1] = '4';
            uint8_t new_ra_rcode = (int)strtol(ra_rcode_str, NULL, 16);
            req_buf[5] = new_ra_rcode;

            printf("new req buf: \n");
            for (i = 0; i < n; i++)
            {
                printf("%02x ", req_buf[i]);
            }
            printf("\n");

            // Write message back
            n = write(newsockfd, req_buf, n);
            if (n < 0)
            {
                perror("write");
                exit(EXIT_FAILURE);
            }

            printf("after write\n");

            close(sockfd);
            close(newsockfd);
            continue;
        }

        /////////////////////////////////////////////////////////////////
        // Act as a client to query upperstream server
        char *us_svr_ip = argv[1];
        char *us_svr_port = argv[2];
        printf("us_svr_ip: %s\n", us_svr_ip);
        printf("us_svr_port: %s\n", us_svr_port);

        uint8_t *res_buf;
        int res_buf_len;
        res_buf = query_server(us_svr_ip, us_svr_port, req_buf, n, &res_buf_len);

        int res_qr = get_qr(res_buf);
        char *res_qname = get_qname(res_buf);
        int res_qtype = get_qtype(res_buf);
        char *res_ipv6_addr = get_ipv6_addr(res_buf);

        printf("res qr: %d\n", res_qr);
        printf("res qname: %s\n", res_qname);
        printf("res qtype: %d\n", res_qtype);
        write_log(res_qr, res_qname, res_qtype, res_ipv6_addr);

        //////////////////////////////////////////////////////////////////
        // Write message back
        n = write(newsockfd, res_buf, res_buf_len);
        if (n < 0)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }

        printf("after write\n");

        close(sockfd);
        close(newsockfd);
    }
    return 0;
}

int get_query_len(uint8_t *query_buf)
{
    int query_len = 0;
    uint8_t len_buf[2];
    len_buf[0] = query_buf[0];
    len_buf[1] = query_buf[1];
    query_len = (len_buf[0] << 8) | (len_buf[1]);
    return query_len + 2;
}

// send query to upstream server and return an response
uint8_t *query_server(char *node, char *service, uint8_t buffer[], int buf_len, int *res_buf_len)
{
    int sockfd, n, s;
    struct addrinfo hints, *servinfo, *rp;
    // char buffer[2048];

    // Create address
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // Get addrinfo of server. From man page:
    // The getaddrinfo() function combines the functionality provided by the
    // gethostbyname(3) and getservbyname(3) functions into a single interface
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
    *res_buf_len = read(sockfd, buffer, 2047);
    if (*res_buf_len < 0)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }
    // Null-terminate string
    buffer[*res_buf_len] = '\0';

    printf("res from upstream server n: %d\n", *res_buf_len);
    int i;
    for (i = 0; i < *res_buf_len; i++)
    {
        printf("%02x ", buffer[i]);
    }
    printf("\n");

    close(sockfd);

    return buffer;
}

void handle_sigint(int sig)
{
    printf("Caught signal %d\n", sig);
    exit(0);
}
