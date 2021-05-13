// A simple client program for server.c
// To compile: gcc client.c -o client
// To run: start the server, then the client

#define _POSIX_C_SOURCE 200112L
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int sockfd, n, s;
    struct addrinfo hints, *servinfo, *rp;
    char *buffer;
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
    s = getaddrinfo(argv[1], argv[2], &hints, &servinfo);
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

    // // Read message from stdin
    // printf("Please enter the message: ");
    // if (fgets(buffer, 255, stdin) == NULL)
    // {
    //     exit(EXIT_SUCCESS);
    // }
    // // Remove \n that was read by fgets
    // buffer[strlen(buffer) - 1] = 0;

    // Send message to server
    buffer = "dig +tcp -p 5353 @172.26.129.247 AAAA 1.comp30023";
    n = write(sockfd, buffer, strlen(buffer));
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
    printf("%s\n", buffer);

    close(sockfd);
    return 0;
}
