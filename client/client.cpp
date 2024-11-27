#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include<iostream>

using namespace std;
#define SERVER_PORT 3023

void * recieve_from_server(void* thread_data)
{
    int * connect_fd=(int *)thread_data;
    char buffer[1000];
    int ret;
    
    while ( (ret = recv(connect_fd[0], buffer, 1000,0)) > 0)  
    {
    
        string s(buffer);
        cout<<s<<flush;
  
    }
    
    if (ret < 0) 
    {
        perror("Error in reading from server"); 
        exit(1);
    }
  
    if(ret==0)
        cout<<"Sever has disconnected. Terminating the program."<<endl;
        exit(0);
        
    return 0;    
   
}

void * send_to_server(void* thread_data)
{ 
    int * connect_fd=(int *)thread_data;
    char buffer[1000];
    while(1)
    {
        while ( fgets(buffer, 1000, stdin)!=NULL)  
        {
            send(connect_fd[1], buffer, 1000, 0);
        }
    }
    
    return 0;
}

int
main(int argc, char **argv)
{
    int connect_fd[2];
    struct sockaddr_in server_addr;
  
    if (argc !=2) 
    {
        perror("Usage: TCPClient <IP address of the server>");
        exit(-1);
    }                      

 
    if ((connect_fd[0] = socket (AF_INET, SOCK_STREAM, 0)) <0) 
    {
        perror("Client unable to create a socket");
        exit(-1);
    }
    if ((connect_fd[1] = socket (AF_INET, SOCK_STREAM, 0)) <0) \
    {
        perror("Client unable to create a socket");
        exit(-1);
    }

 
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr= inet_addr(argv[1]);
    server_addr.sin_port =  htons(SERVER_PORT); 
 
    if (connect(connect_fd[0], (struct sockaddr *) &server_addr, sizeof(server_addr))<0) 
    {
        perror("Could not connect to the server");
        exit(-1);
    }
    if (connect(connect_fd[1], (struct sockaddr *) &server_addr, sizeof(server_addr))<0) 
    {
        perror("Could not connect to the server");
        exit(-1);
    }

 
    pthread_t thread1,thread2;
    if(pthread_create(&thread1,NULL,recieve_from_server,(void *) connect_fd)!=0)
    {
      	 perror("Could not connect to the server");
         exit(-1);
    }
     
    if(pthread_create(&thread2,NULL,send_to_server,(void *) connect_fd)!=0)
    {
      	 perror("Could not connect to the server");
         exit(-1);
    }
 
    while(1){;}
}

