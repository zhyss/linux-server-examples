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

#define BUFFER_SIZE 4096
/* main state */
enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER };

/* slave state */
enum LINE_STATUE { LINE_OK = 0, LINE_BAD, LINE_OPEN};

/* request result */
enum HTTP_CODE { NO_REQURST, GET_REQUEST, BAD_REQUEST, 
            FORBIDDEN_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };

/* return info */
static const char* szret[] = {"I get a correct result\n", "Something wrong\n" };
/*
* slave state machine
*/
LINE_STATUE parse_line(char* buffer, int& checked_index, int& read_index)
{
    char temp;
    for(;checked_index<read_index; ++checked_index)
    {
        temp = buffer[checked_index];
        if(temp=='\r')
        {
            if((checked_index+1)==read_index)
            {
                return LINE_OPEN;
            }
            else if(buffer[checked_index+1]=='\n')
            {
                buffer[checked_index++] = '\0';
                buffer[checked_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if(temp=='\n')
        {
            if((checked_index>1)&&buffer[checked_index-1]=='\r')
            {
                buffer[checked_index-1] = '\0';
                buffer[checked_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}
/* 
* parse requestline
*/
HTTP_CODE parse_requestline(char* temp, CHECK_STATE& checkstate)
{
    char* url = strpbrk(temp," \t");
    if(!url)
    {
        return BAD_REQUEST;
    }
    *url++ = '\0';
    char* method = temp;
    if(strcasecmp(method, "GET")==0)
    {
        printf("The request method is GET\n");
    }
    else
    {
        return BAD_REQUEST;
    }

    url += strspn(url, " \t");
    char* version = strpbrk(url, " \t");
    if(!version)
    {
        return BAD_REQUEST;
    }
    *version++ = '\0';
    version += strspn(version, " \t");
    if(strcasecmp(version, "HTTP/1.1")!=0)
    {
        return BAD_REQUEST;
    }
    if(strncasecmp(url, "http://",7)==0)
    {
        url += 7;
        printf("-----%s\n",url);
        url = strchr(url, '/');
    }
    if(!url || url[0] != '/')
    {
        return BAD_REQUEST;
    }
    printf("The request URL is: %s\n", url);
    checkstate = CHECK_STATE_HEADER;
    return NO_REQURST;
    
}
/*
* parse header
*/
HTTP_CODE parse_headers(char* temp)
{
    if(temp[0]=='\0')
    {
        return GET_REQUEST;
    }
    else if(strncasecmp(temp, "Host:", 5)==0)
    {
        temp += 5;
        temp += strspn(temp, " \t");
        printf("the request host is: %s\n", temp);
    }
    else
    {
        printf("I cant handle this header\n");
    }
    return NO_REQURST;
    
}
/*
* parse_content
*/
HTTP_CODE parse_content(char* buffer, int& checked_index, CHECK_STATE& checkstate, int& read_index, int& start_line)
{
    LINE_STATUE linestatue = LINE_OK;
    HTTP_CODE retcode = NO_REQURST;
    while((linestatue = parse_line(buffer,checked_index, read_index)) == LINE_OK)
    {
        char* temp = buffer+start_line;
        start_line = checked_index;
        switch(checkstate)
        {
            case CHECK_STATE_REQUESTLINE:
            {
                retcode = parse_requestline(temp, checkstate);
                if(retcode == BAD_REQUEST)
                {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER:
            {
                retcode = parse_headers(temp);
                if(retcode == BAD_REQUEST)
                {
                    return BAD_REQUEST;
                }
                else if(retcode == GET_REQUEST)
                {
                    return GET_REQUEST;
                }
                break;
            }
            default:
            {
                return INTERNAL_ERROR;
            }
        }
    }
    if(linestatue == LINE_OPEN)
    {
        return NO_REQURST;
    }
    else
    {
        return BAD_REQUEST;
    }
    
}

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
    int fd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
    if(fd < 0){
        printf("errno is: %d\n", errno);
    } else{
        printf("accept connection\n");
        char buffer[ BUFFER_SIZE ];
        memset(buffer, '\0', BUFFER_SIZE);
        int data_read = 0;
        int read_index = 0;
        int checked_index = 0;
        int start_line = 0;
        /* set initial state of main state machine */
        CHECK_STATE checkstate = CHECK_STATE_REQUESTLINE;
        while(1)
        {
            data_read = recv(fd, buffer+read_index,BUFFER_SIZE-read_index,0);
            if(data_read == -1)
            {
                printf("reading failed\n");
                break;
            }
            else if(data_read == 0)
            {
                printf("remote client has closed the connection\n");
                break;
            }
            read_index += data_read;
            /* parse received data */
            HTTP_CODE result = parse_content(buffer, checked_index, checkstate, 
                                            read_index, start_line);
            if(result == NO_REQURST)
            {
                continue;
            }
            else if(result == GET_REQUEST)
            {
                send(fd, szret[0], strlen(szret[0]), 0);
                break;
            }
            else
            {
                send(fd, szret[1],strlen(szret[1]),0);
                break;
            }
        }
        close(fd);
    }

    close(listenfd);

    return 0;

}