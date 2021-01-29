#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int timeout_connect( const char* ip, int port, int time)
{
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof( address ));
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr);
    address.sin_port = htons( port );

    int sockfd = socket( PF_INET, SOCK_STREAM, 0);
    assert( sockfd >= 0 );

    struct timeval timeout;
    timeout.tv_sec = time;
    timeout.tv_usec = 0;
    socklen_t len = sizeof( timeout );
    ret = setsockopt( sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
    assert( ret!=-1 );

    ret = connect( sockfd, ( struct sockaddr* )&address, sizeof( address ));
    if( ret == -1 )
    {
        if( errno == EINPROGRESS )
        {
            printf( "connecting timeout, process timeout logic \n");
            return -1;
        }
        printf( "%d error occur when connecting to server\n", errno );
        return -1;
    }
    printf("success\n");

    return sockfd;
}
int main(int argc, char* argv[])
{
    const char* ip = "192.168.31.88";
    int port = 54321;

    int sockfd = timeout_connect( ip, port, 10);
    if(sockfd < 0)
    {
        return 1;
    }
    return 0;
}
