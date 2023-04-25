#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <netdb.h>
#include <pthread.h>
#include <errno.h>


#define BUFFER 256
#define QUEUE_SIZE 8
#define SERVICE "5566"
#define HOST_NAME "localhost"

#define X_TURN 1
#define O_TURN 2



typedef struct game_setting{
    char *x_name;
    char *o_name;
    int client_xfd;
    int client_ofd;
    int x_state;
    int o_state;
    pthread_t game;
} Game;

typedef struct clients{
    int socket;
    struct sockaddr_storage addr;
    socklen_t socklength; 
    int active_game; 
} Clients;



int main(int argc, char * argv[argc + 1]){
    struct addrinfo host_hints, *result_list, *results;
    int server_fd, client_fd, clientNum = 0, error = 0;
    //Game * ttt_game;
 

    memset(&host_hints, 0, sizeof(struct addrinfo));
    host_hints.ai_family = AF_UNSPEC;
    host_hints.ai_socktype = SOCK_STREAM;
    host_hints.ai_flags = AI_PASSIVE;


    if((error = getaddrinfo(HOST_NAME, SERVICE, &host_hints, &result_list)) < 0){
        fprintf(stderr, "Error on getting address: %s\n", strerror(error));
        exit(EXIT_FAILURE);
    }
    for(results = result_list; results != NULL; results = results->ai_next){
        server_fd = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
        if(server_fd < 0){
            continue;
        }
        error = bind(server_fd, results->ai_addr, results->ai_addrlen);
        if(error){
            close(server_fd);
            continue;
        }
        error = listen(server_fd, QUEUE_SIZE);
        if(error){
            close(server_fd);
            continue;
        }
        break;
    }
    
    freeaddrinfo(result_list);
    
    if(results == NULL){
        fprintf(stderr, "Could not bind");
        exit(EXIT_FAILURE);
    }

    if(server_fd < 0){
        exit(EXIT_FAILURE);
    }
    printf("%s is currently listening on port %s.\n", HOST_NAME, SERVICE);
    while(1){
        Clients * con = (Clients*)malloc(sizeof(Clients));
        con->socklength = sizeof(struct sockaddr_storage);

        if((client_fd = accept(server_fd, (struct sockaddr*)&con->addr, &con->socklength)) < 0){
            perror("Accept error: ");
            continue; 
        } else {
            printf("Connection has been made with %d!\n", client_fd);
            clientNum++;
        }
        printf("Something something blah blah\n");
        printf("This project blah blah\n");
    }
    return 0;
}