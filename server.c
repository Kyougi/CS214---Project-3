#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>


#define MAX_CLIENTS 10
#define BUFFER 1024
#define SERVER_PORT "15000"


typedef struct clients{
    int socket; //client socket
    struct sockaddr_storage address; //their information about their sockets.
    int pthread_position; //what thread are the two players are in
    int active_game; //1 for currently in an active game and 0 otherwise.
} Clients;

int main(int argc, char * argv[argc + 1]){
    //Variables that we're going to be using...
    struct sockaddr_storage client_info[MAX_CLIENTS];
    struct addrinfo host_hints, *result_list; 
    int server_sockfd; //client_sockfd, clientNum = 0; //one sockfd from server and another from client. clientNum to count the number of clients that has joined
    socklen_t client_length; //We're going to use this when we call accept.
    

    //where we set the process of connections 
    memset(&host_hints, 0, sizeof(struct addrinfo));
    host_hints.ai_family = AF_UNSPEC; 
    host_hints.ai_socktype = SOCK_STREAM;
    host_hints.ai_flags = AI_PASSIVE;

    int info = getaddrinfo(NULL, SERVER_PORT, &host_hints, &result_list);
    if(info < 0){
        fprintf(stderr, "Error in getting address info: %s\n", gai_strerror(info));
        exit(EXIT_FAILURE);
    }

    int error = 0;
    //Setting up a connection
    //List used to only serve what internet connections that works with a specific IP Address. Currently in progress as I gain more knowledge.
    for(struct addrinfo *list = result_list; forloop_list != NULL; forloop_list = forloop_list->ai_next){

        
        //returns the socket file descirptor for the server 
        if((server_sockfd = socket(list->ai_family, list->ai_socktype, list->ai_protocol)) < 0){
            continue;
        } 
        //bind socket to an ip address or port
        if((error = bind(server_sockfd, list->ai_addr, list->ai_addrlen)) < 0){
            perror("Bind error: ");
            close(server_sockfd);
            continue;
        } 
        //will execute listen if there aren't any errors
        if((error = listen(server_sockfd, MAX_CLIENTS)) < 0 ){
            perror("Listen error: ");
            close(server_sockfd);
            continue;
        }

    }
    //freeing after we're done with the 
    

    printf("Currently listening for incoming connections...\n");

    while(1){
        Clients * players = malloc(sizeof(Clients) * MAX_CLIENTS);
        pthread lobbies[MAX_CLIENTS];
        host_length = sizeof(struct sockaddr_storage);

        //make sure to use the accept function here. This part of the code is supposed to detect whether or not we got more or equal to 2 connections from the clients. If there are two connections, we should be able to use the select() function and use pthread_create() and pthread_join() in that other. Select() should be able to pick two different file descriptors. If you need help with select(), dm me and I'll be able to search and tell you the information to make this easier. Menendez's lectures are somewhat unreliable at this time. 
    }


    
    return 0; 
}