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

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
volatile int active = 1;

#define BUFFER 256
#define QUEUE_SIZE 8
#define SERVICE "5566"
#define HOST_NAME "localhost"

#define X_TURN 1
#define O_TURN 2

#define ONE "PLAY\n"
#define TWO "WAIT"
#define THREE "MOVD"
#define FOUR "INVL"
#define FIVE "DRAW"
#define SIX "OVER"



typedef struct game_setting{
    char x_name[50];
    char o_name[50];
    char x_side;
    char o_side;
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
    char name[50]; 
    int active_game;
    int refuse_play; 
} Clients;

void handler(int signum){
    active = 0;
}

void install_handlers(sigset_t *mask)
{
    struct sigaction act;
    act.sa_handler = handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    
    sigemptyset(mask);
    sigaddset(mask, SIGINT);
    sigaddset(mask, SIGTERM);
}

void * type_hello(void *arg){
    Clients * new_client = arg; 
    int error, bytes;
    char buffer[BUFFER/4], protocol[BUFFER/4], names[20];
    printf("Client %d has connected to the server!\n", new_client->socket);
    while((strcmp(protocol, ONE) != 0)){
        pthread_mutex_lock(&lock);
        strcpy(buffer, "Enter PLAY without spaces: ");
        error = write((new_client)->socket, buffer, sizeof(buffer));
        if(error < 0){
            perror("Write error: ");
            (new_client)->refuse_play = 1;
            return NULL;
        }
        error = read((new_client)->socket, protocol, BUFFER/4);
        if(error < 0){
            printf("Error in getting read: %s\n", strerror(error));
            (new_client)->refuse_play = 1;
            return NULL;
        } else if(error == 0){
            printf("EOF from read.\n");
            (new_client)->refuse_play = 1;
            return NULL;
        } 
    }
    strcpy(buffer, "Enter your name");
    error = write((new_client)->socket, buffer, sizeof(buffer));
    if(error < 0){
        perror("Write error: ");
        (new_client)->refuse_play = 1;
        return NULL;
    }
    bytes = read((new_client)->socket, names, 20);
    if(bytes < 0){
        printf("Error in getting read: %s\n", strerror(error));
        (new_client)->refuse_play = 1;
        return NULL;
    } else if(bytes == 0){
        printf("EOF from read.\n");
        (new_client)->refuse_play = 1;
        return NULL;
    } else if(sizeof(names) > 20) {
        strcpy(buffer, "Name too long\n");
        write((new_client)->socket, buffer, sizeof(buffer));
        (new_client)->refuse_play = 1;
        return NULL;
    } else {
        for(int i = 0; i < strlen(protocol); i++){
            if(i == (strlen(protocol) - 1)){
                break;
            }
            protocol[i] = protocol[i];
        }
        for(int i = 0; i < strlen(names); i++){
            if(i == (strlen(names) - 1)){
                break;
            }
            names[i] = names[i];
        }
        printf("%s|%d|%s|", protocol, bytes, names);
        strcpy((new_client)->name, names);
    }
    pthread_mutex_unlock(&lock);
    return NULL;
}



int main(int argc, char * argv[argc + 1]){
    struct addrinfo host_hints, *result_list, *results;
    int server_fd, client_fd, clientNum = 0, error = 0, enough = 0;
    pthread_t say_hello;
    Clients ** client_list[QUEUE_SIZE]; 
    //Game * ttt_game;
 

    memset(&host_hints, 0, sizeof(struct addrinfo));
    host_hints.ai_family = AF_UNSPEC;
    host_hints.ai_socktype = SOCK_STREAM;
    host_hints.ai_flags = AI_PASSIVE;

    sigset_t mask; 

    install_handlers(&mask);


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
    

    if(server_fd < 0){
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&lock, NULL);

    printf("%s is currently listening on port %s.\n", HOST_NAME, SERVICE);
    while(1){
        int index = (QUEUE_SIZE - 1) - clientNum;
        Clients * con = (Clients*)malloc(sizeof(Clients));
        con->socklength = sizeof(struct sockaddr_storage);

        if((client_fd = accept(server_fd, (struct sockaddr*)&con->addr, &con->socklength)) < 0){
            perror("Accept error: ");
            return EXIT_FAILURE; 
        } else {
            con->socket = client_fd;
            error = pthread_sigmask(SIG_BLOCK, &mask, NULL);
            if (error != 0) {
        	    fprintf(stderr, "sigmask: %s\n", strerror(error));
        	    exit(EXIT_FAILURE);
            }
            if((error = pthread_create(&say_hello, NULL, type_hello, con)) != 0){
                fprintf(stderr, "Error in creating thread: %s\n", strerror(error));
                close(client_fd);
                free(con);
                return EXIT_FAILURE;
            }
            pthread_detach(say_hello);
            error = pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
            if (error != 0) {
        	    fprintf(stderr, "sigmask: %s\n", strerror(error));
        	    exit(EXIT_FAILURE);
            }
            if(con->refuse_play == 1){
                close(con->socket);
                free(con);
                continue;
            } else {
                client_list[index] = &con;
                clientNum++; 
                enough++;
            }
        }
        if(enough == 2){
            //setting up the player in this one. 

        } else {
            //this one accoutns for only one player
            int server_buffer[BUFFER/4];
            sprintf(server_buffer, "%s|0|", TWO);
            error = write(con->socket, server_buffer, sizeof(server_buffer));
            if(error){
                perror("Write error: ");
                return EXIT_FAILURE;
            }
        }
    }
    return 0;
}