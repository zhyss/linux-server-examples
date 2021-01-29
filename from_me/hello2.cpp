#include <iostream>
#include <stdio.h>
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
using namespace std;


int main()
{
    char m_buf[] = "hello";
	char* file_name = m_buf;
    if( access(file_name, F_OK) == -1 )
    {
        printf("access failed\n");
        return 0;
    }
    int ret = fork();
    if(ret==-1)
    {
        //printf("ret==-1\n");
        return 0;
    }
    else if(ret>0)
    {
        //printf("ret>0\n");
        return 0;
    }
    else
    {
        //printf("this\n");
        execl(m_buf,m_buf,NULL);
        exit(0);
    }
	return 1;
}


