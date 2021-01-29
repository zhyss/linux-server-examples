#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>


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
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    /* bind */
    int ret = bind(sock,(struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    /* listen */
    ret = listen(sock, 5);
    assert(ret != -1);

    printf("Waiting connection...\n");
    
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (struct sockaddr*)&client, &client_addrlength);
    if(connfd < 0){
        printf("errno is: %d\n", errno);
    } else{

        printf("accept connection...\n");

        int pipefd[2];

        ret = pipe(pipefd);
        assert(ret != -1);

        ret = splice(connfd, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        assert(ret != -1);
        printf("accepted info!\n");
        ret = splice(pipefd[0], NULL, connfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        assert(ret != -1);
        printf("return info yes!\n");
        close(connfd);
    }

    close(sock);

    return 0;

}