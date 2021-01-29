#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>

#define BUFFER_SIZE 1023

int setnonblocking( int fd )
{
    int old_option = fcntl( fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option);
    return old_option;
}

int unblock_connect( const char* ip, int port, int time)
{
    int ret = 0;
    /* create a ipv4 socket address */
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    /* create a socket */
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    int fdopt = setnonblocking( sockfd );
    ret = connect(sockfd, (struct sockaddr*)&address, sizeof(address));
    if(ret == 0)
    {
        printf("connect with server immediately\n");
        fcntl(sockfd, F_SETFL, fdopt);
        return sockfd;
    }
    else if(errno != EINPROGRESS)
    {
        printf("unblock connect not support\n");
        return -1;
    }
    fd_set readfds;
    fd_set writefds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_SET(sockfd, &writefds);

    timeout.tv_sec = time;
    timeout.tv_usec = 0;

    ret = select(sockfd + 1, NULL, &writefds, NULL, NULL);
    if(ret <= 0)
    {
        printf("connection time out\n");
        close(sockfd);
        return -1;
    }

    if(!FD_ISSET(sockfd, &writefds))
    {
        printf("no events on sockfd found\n");
        close(sockfd);
        return -1;
    }

    int error = 0;
    socklen_t length = sizeof(error);
    if( getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &length ) < 0 )
    {
        printf( "get socket option failed\n" );
        close(sockfd);
        return -1;
    }

    if(error != 0)
    {
        printf( "connection failed after select with the error: %d\n", error );
        close(sockfd);
        return -1;
    }

    printf( "connection ready after select with the socket: %d \n", sockfd );
    fcntl(sockfd, F_SETFL, fdopt );
    return sockfd;
    
}
int main(int argc, char* argv[] )
{
    const char* ip = "192.168.31.88";
    int port = 54321;

    int sockfd = unblock_connect(ip, port, 10000);
    if(sockfd < 0)
    {
        return 1;
    }
    close(sockfd);
    return 0;
}