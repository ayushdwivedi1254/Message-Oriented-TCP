#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "mysocket.h"

// returns minimum of 2 integers
int min(int a, int b)
{
    if(a < b)
    return a;
    return b;
}

void clear_buffer(void *buf, int n)
{
    memset(buf,0,n);
}

// generalized send function which sends a string in chunks of size send_buff_size
void send_string(int sockfd, void *expression, size_t totlen,int flags)
{
    int send_size;
    int send_buff_size = MAX_SEND_SIZE;
    void* send_buff=malloc(send_buff_size);

    int i, pointer = 0;

    // sending header
    clear_buffer(send_buff,send_buff_size);
    memcpy(send_buff,"*#####",6);
    if(send(sockfd, send_buff, 6, flags) != 6)       // sending the header
    {
        perror("send() sent a different number of bytes than expected\n");
        exit(0);
    }

    int count = 0;          // variable to count the number of # occurred consecutively

    // while loop to send the string in chunks of size: send_buff_size
    while(1)
    {
        clear_buffer(send_buff,send_buff_size);
        int len=min(send_buff_size,totlen-pointer);
        for(i = 0; i<len; i++)     // sending the expression in parts
        {
            if(count == 4 && (pointer > 4 && ((char*)expression)[pointer-5] == '*'))
            {
                ((char*)send_buff)[i] = '$';
                count = 0;
                continue;
            }
            if(((char*)expression)[pointer] == '#')
            count++;
            else
            count = 0;
            ((char*)send_buff)[i] = ((char*)expression)[pointer++];
        }
        if((send_size = send(sockfd, send_buff, i, flags)) != i)       // sending the expression to server
        {
            perror("send() sent a different number of bytes than expected\n");
            exit(0);
        }
        if(count == 4 && (pointer > 4 && ((char*)expression)[pointer-5] == '*'))
        {
            count = 0;
            clear_buffer(send_buff,send_buff_size);
            ((char*)send_buff)[0] = '$';
            if(send(sockfd, send_buff, 1, flags) != 1)
            {
                perror("send() sent a different number of bytes than expected\n");
                exit(0);
            }
        }
        if(pointer == totlen)       // break if entire string is sent
        break;
    }

    clear_buffer(send_buff,send_buff_size);
    memcpy(send_buff,"*#####",6);
    if(send(sockfd, send_buff, 6, flags) != 6)       // sending the trailer
    {
        perror("send() sent a different number of bytes than expected\n");
        exit(0);
    }
    // printf("%s\n", send_buff);

}

// generalized function to receive a string: calls recv until null is encountered
recv_element * receive_string(int sockfd, void *global_buf, int *global_size)
{
    int count = 0;      // to count the number of # occurred consecutively
    int msg_read = 0;   // variable to denote if reading the main message or not
    int star_found = 0; // variable to store if a star has been found or not

    int buf_size = MAX_RECEIVE_SIZE;
    void* buf=malloc(buf_size);
    void *answer;      // stores the result of the expression
    recv_element* recv_string=malloc(sizeof(recv_element));
    int i, j, pointer, recv_size, ch;
    size_t len = 4;   // initial length of the answer string
    answer = realloc(NULL, sizeof(*answer)*(len));     // reallocing answer
    if(!answer)
    {
        printf("Error in realloc!\n");
        exit(0);
    }
    pointer = 0;
    int flag_mark = 0;
    do{
        j = 0;
        clear_buffer(buf,buf_size);
        if(*global_size > 0)
        {
            memcpy(buf, global_buf, *global_size);
            recv_size = *global_size;
            clear_buffer(global_buf, MAX_RECEIVE_SIZE);
            *global_size = 0;
        }
        else if((recv_size = recv(sockfd, buf, buf_size, 0)) < 0)     // receiving answer in parts in buffer
        {
            perror("recv() failed\n");
            printf("Value of errno: %d\n", errno);
            exit(0);
        }
        // printf("Received string: %s\n", (char *)buf);
		if(recv_size == 0)
		{
            memcpy(answer,"exit",4);
            recv_string->buf=answer;
            recv_string->len=4;
			return recv_string;
		}
        for(; j<recv_size; j++)     // this loop concatenates the buffer string received to answer, character by character
        {
            if(msg_read == 0)
            {
                if(count == 5 && star_found)
                {
                    count = 0;
                    msg_read = 1;
                    star_found = 0;
                }
                else
                {
                    if(((char*)buf)[j] == '*')
                    {
                        star_found=1;
                        continue;
                    }
                    if(((char*)buf)[j] == '#')
                    {
                        count++;
                        continue;
                    }
                }
            }
            // printf("HEMLO: %d %c\n", j, ((char*)buf)[j]);
            if(count == 4 && ((char*)buf)[j] == '$' && pointer > 4 && ((char*)answer)[pointer-5] == '*')
            {
                count = 0;
                continue;
            }
            if(((char*)buf)[j] == '#')
            {
                count++;
            }
            else
            {
                count = 0;
            }
            ((char*)answer)[pointer++] = ((char*)buf)[j];
            // printf("%s\n", (char *)answer);
            if(count == 5 && pointer > 5 && ((char*)answer)[pointer-6] == '*')
            {
                flag_mark = 1;
                if(j < recv_size -1)
                {
                    memcpy(global_buf, buf+j+1, (recv_size-1 - (j+1) + 1));
                }
                *global_size = (recv_size-1 - (j+1) + 1);
                break;
            }
            if(pointer == len)       // if length of answer reached the current max size of answer, just realloc it.
            {
                answer = realloc(answer, sizeof(*answer)*(len+=16));
            }
            if(!answer)
            {
                printf("Error in realloc!\n");
                exit(0);
            }
        }
        if(flag_mark)     // checks the breaking condition of previous loop and decides if null character was found or not
        {
            break;
        }
    }while(recv_size > 0);      // loop till recv_size > 0
    answer = realloc(answer, sizeof(*answer)*(pointer-6));
    recv_string->buf=answer;
    recv_string->len=pointer-6;
    return recv_string;
}

void *R(void *param)
{
    void *global_buf = malloc(MAX_RECEIVE_SIZE);
    int global_size = 0;
    while(1)
    {
        if(newsockfd == -1)
        continue;
        recv_element *ret_string = receive_string(newsockfd, global_buf, &global_size);
        if(ret_string->len==0)
        continue;

        pthread_mutex_lock(&recv_mutex);

        while(recv_count == RECEIVE_SIZE)
        {
            pthread_cond_wait(&recv_cond, &recv_mutex);
        }
        
        memcpy(Received_Message[recv_in]->buf,ret_string->buf,ret_string->len);
        Received_Message[recv_in]->len = ret_string->len;
        // free(ret_string);
        recv_in = (recv_in + 1) % RECEIVE_SIZE;
        recv_count++;

        pthread_cond_signal(&recv_cond);
        pthread_mutex_unlock(&recv_mutex);
    }
}

void *S(void *param)
{
    while(1)
    {
        sleep(T);
        pthread_mutex_lock(&send_mutex);

        while(send_count == 0)
        {
            pthread_cond_wait(&send_cond, &send_mutex);
        }

        while(send_count)
        {
            // ((char*)Send_Message[send_out]->buf)[Send_Message[send_out]->len] = '\0';
            // printf("%s\n", Send_Message[send_out]->buf);
            send_string(newsockfd, Send_Message[send_out]->buf,Send_Message[send_out]->len, Send_Message[send_out]->flags);
            clear_buffer(Send_Message[send_out]->buf, MAX_MESSAGE_SIZE);
            send_out = (send_out + 1) % SEND_SIZE;
            send_count--;
        }

        pthread_cond_signal(&send_cond);
        pthread_mutex_unlock(&send_mutex);
    }
}

int my_socket(int domain, int type, int protocol)
{
    sleep(5);
    int	sockfd;
    sockfd = socket(domain, type, protocol);

    // if error, no need to create threads and tables, just return
	if(sockfd < 0)
    return sockfd;

    Sockfd = sockfd;
    newsockfd = -1;

    // Initialising mutex and condition variables
    pthread_mutex_init(&send_mutex, NULL);
    pthread_mutex_init(&recv_mutex, NULL);
    pthread_cond_init(&send_cond, NULL);
    pthread_cond_init(&recv_cond, NULL);

    // Creating R and S threads
    pthread_attr_t R_attr, S_attr;
    pthread_attr_init(&R_attr);
    pthread_attr_init(&S_attr);
    pthread_create(&R_tid, &R_attr, R, NULL);
    pthread_create(&S_tid, &S_attr, S, NULL);

    // Creating tables
    Send_Message = (send_element **) malloc(SEND_SIZE * sizeof(send_element *));
    Received_Message = (recv_element **) malloc(RECEIVE_SIZE * sizeof(recv_element *));

    send_in = send_out = send_count = 0;
    recv_in = recv_out = recv_count = 0;

    for(int i = 0; i<SEND_SIZE; i++)
    {
        Send_Message[i] = (send_element *) malloc(sizeof(send_element));
        Send_Message[i]->buf=malloc(MAX_MESSAGE_SIZE);
    }
    for(int i = 0; i<RECEIVE_SIZE; i++)
    {
        Received_Message[i] = (recv_element *) malloc(sizeof(recv_element));
        Received_Message[i]->buf=malloc(MAX_MESSAGE_SIZE);
    }

    return sockfd;
}

int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int return_value = bind(sockfd, addr, addrlen);
    return return_value;
}

int my_listen(int sockfd, int backlog)
{
    int return_value = listen(sockfd, backlog);
    return return_value;
}

int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    int return_value = accept(sockfd, addr, addrlen);
    newsockfd = return_value;
    return return_value;
}

int my_connect(int sockfd, const struct sockaddr * addr, socklen_t addrlen)
{
    int return_value = connect(sockfd, addr, addrlen);
    newsockfd = sockfd;
    return return_value;
}

ssize_t my_send(int sockfd, const void *buf, size_t len, int flags)
{
    pthread_mutex_lock(&send_mutex);

    while(send_count == SEND_SIZE)
    {
        pthread_cond_wait(&send_cond, &send_mutex);
    }
    // printf("In my send: %s\n", (char *)buf);
    Send_Message[send_in]->sockfd = sockfd;
    // for(int i = 0; i<len; i++)
    // Send_Message[send_in]->buf[i] = buf[i];
    memcpy(Send_Message[send_in]->buf,buf,len);
    Send_Message[send_in]->len = len;
    Send_Message[send_in]->flags = flags;
    send_in = (send_in + 1) % SEND_SIZE;
    send_count++;

    pthread_cond_signal(&send_cond);
    pthread_mutex_unlock(&send_mutex);

    return len;
}
 
ssize_t my_recv(int sockfd, void *buf, size_t len, int flags)
{
    pthread_mutex_lock(&recv_mutex);

    while(recv_count == 0)
    {
        pthread_cond_wait(&recv_cond, &recv_mutex);
    }
    recv_element *curr_element = Received_Message[recv_out];
    recv_out = (recv_out + 1) % RECEIVE_SIZE;
    recv_count--;
    int return_value = 0;
    int val=min(curr_element->len,len);
    return_value+=val;
    memcpy(buf,curr_element->buf,len);

    pthread_mutex_unlock(&recv_mutex);

    return return_value;
}

int my_close(int sockfd)
{
    sleep(5);

    // Clean up
    pthread_cancel(R_tid);
    pthread_cancel(S_tid);

    pthread_cond_destroy(&send_cond);
    pthread_mutex_destroy(&send_mutex);
    pthread_cond_destroy(&recv_cond);
    pthread_mutex_destroy(&recv_mutex);

    for(int i = 0; i<SEND_SIZE; i++)
    free(Send_Message[i]);

    for(int i = 0; i<RECEIVE_SIZE; i++)
    free(Received_Message[i]);

    free(Send_Message);
    free(Received_Message);

    int return_value = close(sockfd);

    return return_value;
}