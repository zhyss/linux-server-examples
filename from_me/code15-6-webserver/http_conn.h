#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "locker.h"

class http_conn
{
public:
    /* filename maxlength */
    static const int FILENAME_LEN = 200;
    /* readbuf size */
    static const int READ_BUFFER_SIZE = 2048;
    /* writebuf size */
    static const int WRITE_BUFFER_SIZE = 1024;
    /* http request method, only for GET */
    enum METHOD {GET=0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS,
                 CONNECT, PATCH};
    /* main statemachine */
    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER,
                        CHECK_STATE_CONTENT };
    /* request result */
    enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE,
            FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, 
            CLOSED_CONNECTION };
    /* slave state */
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN};
public:
    http_conn(){}
    ~http_conn(){}
public:
    /* init new received connection */
    void init( int sockfd, const sockaddr_in& addr );
    /* close connection */
    void close_conn( bool read_close = true );
    /* process client's request */
    void process();
    /* nonblock read */
    bool read();
    /* nonblock write */
    bool write();
private:
    /* init */
    void init();
    /* parse http request */
    HTTP_CODE process_read();
    /* fill http response */
    bool process_write( HTTP_CODE ret );

    /* used by process_read() */
    HTTP_CODE parse_request_line( char* text );
    HTTP_CODE parse_headers( char* text );
    HTTP_CODE parse_content( char* text );
    HTTP_CODE do_request();
    char* get_line(){return m_read_buf + m_start_line;}
    LINE_STATUS parse_line();

    /* used by process_write() */
    void unmap();
    bool add_response( const char* format, ... );
    bool add_content( const char* content );
    bool add_status_line( int status, const char* title );
    bool add_headers( int content_length );
    bool add_content_length( int content_length );
    bool add_linger();
    bool add_blank_line();

public:
    static int m_epollfd;
    static int m_user_count;

private:
    int m_sockfd;
    sockaddr_in m_address;

    /* readbuf */
    char m_read_buf[ READ_BUFFER_SIZE ];
    /* received data's next position */
    int m_read_idx;
    /* parsing char's position */
    int m_checked_idx;
    /* parsing line's start position */
    int m_start_line;
    /* writebuf */
    char m_write_buf[ WRITE_BUFFER_SIZE ];
    /* needing sending's bytes' number */
    int m_write_idx;

    /* state of main statemachine */
    CHECK_STATE m_check_state;
    /* request method */
    METHOD m_method;

    /* requests target file's full path, = doc_root + m_url */
    char m_real_file[ FILENAME_LEN ];
    /* requests target file's name */
    char* m_url;
    /* version */
    char* m_version;
    /* host */
    char* m_host;
    /* requests message length */
    int m_content_length;
    /* keep alive ? */
    bool m_linger;

    /* file mmaped positin in ram */
    char* m_file_address;
    /* file stat */
    struct stat m_file_stat;
    /* use writev */
    struct iovec m_iv[2];
    int m_iv_count;
};

#endif
