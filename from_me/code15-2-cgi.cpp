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
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "processpool.h"

class cgi_conn
{
public:
    cgi_conn(){}
    ~cgi_conn(){}
    void init(int epollfd, int sockfd, const sockaddr_in& client_addr )
    {
        m_epollfd = epollfd;
        m_sockfd = sockfd;
        m_address = client_addr;
        memset(m_buf, '\0', BUFFER_SIZE );
        m_read_idx = 0;
    }
    void process()
    {
        int idx = 0;
        int ret = -1;
        while(true)
        {
            idx = m_read_idx;
            ret = recv(m_sockfd, m_buf+idx, BUFFER_SIZE-1-idx, 0);
            if(ret<0)
            {
                if(errno!=EAGAIN)
                {
                    removefd(m_epollfd, m_sockfd);
                }
                break;
            }
            else if(ret==0)
            {
                removefd(m_epollfd, m_sockfd);
                break;
            }
            else
            {
                m_read_idx += ret;
                printf("user content is: %s\n", m_buf );
                for(;idx<m_read_idx;++idx)
                {
                    if( (idx>=1) && (m_buf[idx-1]=='\r') && (m_buf[idx]=='\n') )
                    {
                        printf("break\n");
                        break;
                    }
                }
                if(idx==m_read_idx)
                {
                    printf("continue\n");
                    continue;
                }
                m_buf[idx-1]='\0';
                printf("m_buf:%s\n",m_buf);
                char* file_name = m_buf;
                if( access(file_name, F_OK) == -1 )
                {
                    printf("access failed\n");
                    removefd(m_epollfd, m_sockfd);
                    break;
                }
                ret = fork();
                if(ret==-1)
                {
			printf("ret==-1\n");
                    removefd(m_epollfd,m_sockfd);
                    break;
                }
                else if(ret>0)
                {
			printf("ret>0\n");
                    removefd(m_epollfd,m_sockfd);
                    break;
                }
                else
                {
                    printf("this\n");
                    close(STDOUT_FILENO);
                    dup(m_sockfd);
                    execl(m_buf,m_buf,NULL);
                    exit(0);
                }
                
            }
            
        }

    }
private:
    static const int BUFFER_SIZE = 1024;
    static int m_epollfd;
    int m_sockfd;
    sockaddr_in m_address;
    char m_buf[BUFFER_SIZE];
    int m_read_idx;
};


int cgi_conn::m_epollfd = -1;


int main( int argc, char* argv[] )
{

    const char* ip = "192.168.31.59";
    int port = 54321;

    /* create a TCP socket */
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    int ret = 0;
    /* create a ipv4 socket address */
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    processpool< cgi_conn >* pool = processpool< cgi_conn >::create( listenfd );
    if(pool)
    {
        pool->run();
        delete pool;
    }


    close( listenfd );

    return 0;
}
