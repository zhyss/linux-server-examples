#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>


int main(int argc, char* argv[]){
    /* info */
    const char* ip = "192.168.31.59";
    int port = 54321;

    /* create a ipv4 socket address */
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    /* create a socket */
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    /* bind */
    int ret = bind(listenfd,(struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    /* listen */
    ret = listen(listenfd, 5);
    assert(ret != -1);

    printf("Waiting connection...\n");
    
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
    if(connfd < 0){
        printf("errno is: %d\n", errno);
        close(listenfd);
    }
    char buf[1024];
    fd_set read_fds;
    fd_set exception_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&exception_fds);

    while(1)
    {
        memset(buf, '\0', sizeof(buf));
        FD_SET(connfd, &read_fds);
        FD_SET(connfd, &exception_fds);
        ret = select(connfd + 1, &read_fds, NULL, &exception_fds, NULL);
        if(ret < 0 )
        {
            printf("selection failure\n");
            break;
        }
        if(FD_ISSET(connfd, &read_fds))
        {
            ret = recv(connfd, buf, sizeof(buf)-1,0);
            if(ret <= 0)
            {
                break;
            }
            printf("get %d bytes of normal data: %s\n", ret, buf);
        }
        else if(FD_ISSET(connfd, &exception_fds))
        {
            ret = recv(connfd, buf, sizeof(buf)-1, MSG_OOB);
            if(ret <= 0)
            {
                break;
            }
            printf("get %d bytes of oob data: %s\n",ret,buf);
        }
    }
    close(connfd);
    close(listenfd);
    return 0;

}