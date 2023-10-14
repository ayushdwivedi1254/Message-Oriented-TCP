#ifndef __MYSOCKET_H
#define __MYSOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#define SOCK_MyTCP SOCK_STREAM
#define MAX_MESSAGE_SIZE 5000
#define MAX_SEND_SIZE 1000
#define MAX_RECEIVE_SIZE 1000
#define T 1
#define SEND_SIZE 10
#define RECEIVE_SIZE 10

typedef struct send_element{
    int sockfd;
    void* buf;
    size_t len;
    int flags;
} send_element;

typedef struct recv_element{
    void* buf;
    size_t len;
} recv_element;

// declaring buffers
send_element **Send_Message;
recv_element **Received_Message;

// parameters for the above buffers
int send_in, send_out, send_count;
int recv_in, recv_out, recv_count;

// socket descriptor of the socket being created
int Sockfd;
int newsockfd;

// thread ids need to be remembered
pthread_t R_tid, S_tid;

// declaring mutex and condition variables
pthread_mutex_t send_mutex;
pthread_mutex_t recv_mutex;
pthread_cond_t send_cond;
pthread_cond_t recv_cond;

int min(int a, int b);
void clear_buffer(void *buf, int n);
void send_string(int sockfd, void *expression,size_t totlen, int flags);
recv_element * receive_string(int sockfd, void * global_buf, int *global_size);
void *R(void *param);
void *S(void *param);
int my_socket(int domain, int type, int protocol);
int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int my_listen(int sockfd, int backlog);
int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int my_connect(int sockfd, const struct sockaddr * addr, socklen_t addrlen);
ssize_t my_send(int sockfd, const void *buf, size_t len, int flags);
ssize_t my_recv(int sockfd, void *buf, size_t len, int flags);
int my_close(int sockfd);

#endif