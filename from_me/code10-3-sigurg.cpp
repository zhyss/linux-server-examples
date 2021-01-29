#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define BUF_SIZE 1024

static int connfd;
/* signal chuli */
void sig_urg( int sig )
{
    int save_errno = errno;
    char buffer[ BUF_SIZE ];
    memset( buffer, '\0', BUF_SIZE );
    int ret = recv(connfd, buffer, BUF_SIZE-1, MSG_OOB);
    if( ret==-1 )
    {
        printf("receive MSG_OOB data error\n");
	printf("errno is: %d\n",errno);
    }
    else
    {
        printf( "got %d bytes of oob data '%s'\n", ret, buffer );
    }
    errno = save_errno;
}
/* set signal's chuli function */
void addsig( int sig, void ( *sig_handler )( int ) )
{
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset( &sa.sa_mask);
    assert( sigaction( sig, &sa, NULL ) != -1 );
}

int main(int argc, char* argv[])
{
    const char* ip = "192.168.31.59";
    int port = 54321;

    /* create a ipv4 socket address */
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    /* create a TCP socket */
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    int connfd = accept(sock, ( struct sockaddr* )&client_address, &client_addrlength );
    if( connfd < 0 )
    {
        printf( "errno is: %d\n", errno );
    }
    else
    {
        addsig( SIGURG, sig_urg);
        /* set connfd's suzhu process is this process */
        fcntl( connfd, F_SETOWN, getpid());

        char buffer[ BUF_SIZE ];
        while(1)
        {
            memset( buffer, '\0', BUF_SIZE );
            ret = recv( connfd, buffer, BUF_SIZE-1, 0 );
            if( ret <= 0 )
            {
                break;
            }
            printf( "got %d bytes of normal data '%s'\n", ret, buffer );
        }
        close(connfd);
    }
    close(sock);
    return 0;
}
