#define _POSIX_C_SOURCE 200112L
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "helper1.h"

#define AAAA_ID 28
#define PORT "8053"
#define BUF_SIZE 1024

uint8_t *query_server(char *node, char *service, uint8_t buffer[], int buf_len, int *res_buf_len);
void handle_sigint(int sig);
int get_req_len(uint8_t *query_buf);
void respond_client(int newsockfd, uint8_t *res_buf, int res_buf_len);
int accept_request(int *sockfd, int *newsockfd, uint8_t *req_buf);
void handle_non_AAAA_req(int *newsockfd, uint8_t *req_buf, int req_buf_len);

int main(int argc, char *argv[])
{
    signal(SIGINT, handle_sigint);

    while (1)
    {
        // Act as a server to accept request from client (dig)
        int sockfd, newsockfd, req_buf_len;
        uint8_t req_buf[BUF_SIZE];
        req_buf_len = accept_request(&sockfd, &newsockfd, req_buf);

        int qr = get_qr(req_buf);
        char *qname = get_qname(req_buf);
        int qtype = get_qtype(req_buf);

        write_log(qr, qname, qtype, NULL);

        // Check if request is AAAA
        if (qtype != AAAA_ID)
        {
            handle_non_AAAA_req(&newsockfd, req_buf, req_buf_len);
            // // Send request back to dig

            // // Change qr to 0
            // char qr_str[3];
            // sprintf(qr_str, "%02x", req_buf[4]);
            // qr_str[0] = '8';
            // uint8_t new_qr = (int)strtol(qr_str, NULL, 16);
            // req_buf[4] = new_qr;

            // // Change ra to 1 and rcode to 4
            // char ra_rcode_str[3];
            // sprintf(ra_rcode_str, "%02x", req_buf[5]);
            // ra_rcode_str[0] = '8';
            // ra_rcode_str[1] = '4';
            // uint8_t new_ra_rcode = (int)strtol(ra_rcode_str, NULL, 16);
            // req_buf[5] = new_ra_rcode;

            // // Write message back
            // respond_client(newsockfd, req_buf, req_buf_len);

            close(sockfd);
            close(newsockfd);
            continue;
        }

        /////////////////////////////////////////////////////////////////
        // Act as a client to send request to upperstream server
        char *us_svr_ip = argv[1];
        char *us_svr_port = argv[2];

        uint8_t *res_buf;
        int res_buf_len;
        res_buf = query_server(us_svr_ip, us_svr_port, req_buf, req_buf_len, &res_buf_len);

        int res_qr = get_qr(res_buf);
        char *res_qname = get_qname(res_buf);
        int res_qtype = get_qtype(res_buf);
        char *res_ipv6_addr = get_ipv6_addr(res_buf);

        write_log(res_qr, res_qname, res_qtype, res_ipv6_addr);

        //////////////////////////////////////////////////////////////////
        // Act as a server to send response back to dig (client)
        respond_client(newsockfd, res_buf, res_buf_len);

        close(sockfd);
        close(newsockfd);
    }
    return 0;
}

int get_req_len(uint8_t *query_buf)
{
    int query_len = 0;
    uint8_t len_buf[2];
    len_buf[0] = query_buf[0];
    len_buf[1] = query_buf[1];
    query_len = (len_buf[0] << 8) | (len_buf[1]);
    return query_len + 2;
}

void handle_non_AAAA_req(int *newsockfd, uint8_t *req_buf, int req_buf_len)
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

    // Write message back
    respond_client(newsockfd, req_buf, req_buf_len);
}

int accept_request(int *sockfd, int *newsockfd, uint8_t *req_buf)
{
    int bytes_read, re, s;
    struct addrinfo hints, *res;
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size;

    // Create address we're going to listen on (with given port number)
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // for bind, listen, accept
    // node (NULL means any interface), service (port), hints, res
    s = getaddrinfo(NULL, PORT, &hints, &res);
    if (s != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    // Create socket
    *sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (*sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Reuse port if possible
    re = 1;
    if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    // Bind address to the socket
    int enable = 1;
    if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        perror("setsockopt");
        exit(1);
    }

    if (bind(*sockfd, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(res);

    // Listen on socket - means we're ready to accept connections,
    // incoming connection requests will be queued, man 3 listen
    if (listen(*sockfd, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Accept a connection - blocks until a connection is ready to be accepted
    // Get back a new file descriptor to communicate on
    client_addr_size = sizeof client_addr;
    *newsockfd =
        accept(*sockfd, (struct sockaddr *)&client_addr, &client_addr_size);
    if (*newsockfd < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // Read characters from the connection, then process
    bytes_read = read(*newsockfd, req_buf, BUF_SIZE - 1); // n is number of characters read
    if (bytes_read < 0)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }

    // Continue reading if full request has not been received
    while (bytes_read != get_req_len(req_buf))
    {
        bytes_read += read(*newsockfd, req_buf + bytes_read, BUF_SIZE - 1);

        if (bytes_read < 0)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }
    }

    // Null-terminate string
    req_buf[bytes_read] = '\0';

    return bytes_read;
}

// Send response back to dig (client)
void respond_client(int newsockfd, uint8_t *res_buf, int res_buf_len)
{
    int n = write(newsockfd, res_buf, res_buf_len);
    if (n < 0)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }
}

// send query to upstream server and return an response
uint8_t *query_server(char *node, char *service, uint8_t buffer[], int buf_len, int *res_buf_len)
{
    int sockfd, n, s;
    struct addrinfo hints, *servinfo, *rp;

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
            break;

        close(sockfd);
    }
    if (rp == NULL)
    {
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
    *res_buf_len = read(sockfd, buffer, BUF_SIZE - 1);
    if (*res_buf_len < 0)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }
    // Null-terminate string
    buffer[*res_buf_len] = '\0';

    close(sockfd);

    return buffer;
}

void handle_sigint(int sig)
{
    exit(0);
}
