#define _POSIX_C_SOURCE 200809L
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
#define BUFFER 256
#define SERVER_PORT "4536"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


typedef struct player{
    char *name;
    char side;
    int client_sock;
} Playing;

typedef struct clients{
    int socket; //client socket
    struct sockaddr_storage address; //their information about their sockets.
    int pthread_position; //what thread are the two players are in
    int queue_position; 
    int active_game; //1 for currently in an active game and 0 otherwise.
} Clients;

struct thread_args{
    int client1_fd;
    int client2_fd;
};

void initialize_board(char (table*)[3]){
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
            table[i][j] = '-';
        }
    }
}

void set_player(int client, ){

}


//This is where the tic tac toe game is called. When the game is over, we close the file descriptors of the clients. 
void * start_game(void *arg){
    struct thread_args *clients = arg;
    Playing player_one, player_two;
    char table[3][3];
    player_one.name = malloc(sizeof(char) * BUFFER), player_two.name = malloc(sizeof(char) * BUFFER);
    player_one.client_sock = clients->client1_fd, player_two.client_sock = clients->client2_fd;
    
    initialize_board
}


int main(int argc, char * argv[argc + 1]){
    //Variables that we're going to be using...
    Clients player[MAX_CLIENTS];
    struct addrinfo host_hints, *result_list; 
    int server_sockfd, client_sockfd, clientNum = 0, acceptable = 0, threads = 0; //one sockfd from server and another from client. clientNum to count the number of clients that has joined
    pthread_t games[MAX_CLIENTS/2];
    struct thread_args arg[MAX_CLIENTS/2];

    //Initializing the Clients struct...
    for(int i = 0; i < MAX_CLIENTS; i++){
        player[i].socket = 0;
        player[i].pthread_position = 0;
        player[i].queue_position = 0;
        player[i].active_game = 0;
    }

   

    //where we set the process of connections 
    memset(&host_hints, 0, sizeof(struct addrinfo));
    host_hints.ai_family = AF_UNSPEC; 
    host_hints.ai_socktype = SOCK_STREAM;
    host_hints.ai_flags = AI_PASSIVE;

    int info = getaddrinfo("top.cs.rutgers.edu", SERVER_PORT, &host_hints, &result_list);
    if(info < 0){
        fprintf(stderr, "Error in getting address info: %s\n", gai_strerror(info));
        exit(EXIT_FAILURE);
    }

    int error = 0;
    //Setting up a connection
    //List used to only serve what internet connections that works with a specific IP Address. Currently in progress as I gain more knowledge.
    for(struct addrinfo *list = result_list; list != NULL; list = list->ai_next){

        
        //returns the socket file descirptor for the server 
        if((server_sockfd = socket(list->ai_family, list->ai_socktype, list->ai_protocol)) < 0){
            continue;
        } 
        //bind socket to an ip address or port
        if((error = bind(server_sockfd, list->ai_addr, list->ai_addrlen)) < 0){
            close(server_sockfd);
            continue;
        } 
        //will execute listen if there aren't any errors
        if((error = listen(server_sockfd, MAX_CLIENTS)) < 0 ){
            close(server_sockfd);
            continue;
        }

    }
    //freeing after we're done with the 
    free(result_list);
    pthread_mutex_init(&lock, NULL);
    printf("Listening for incoming connections on port 4536...\n");

    //Waiting for a connection...
    while(1){
        while(acceptable != 2 || clientNum == MAX_CLIENTS){
            if((client_sockfd = accept(server_sockfd, (struct sockaddr*)&player[clientNum].address, NULL)) < 0){
                perror("Accept error: ");
                continue;
            } else {
                //queueing client information while preventing other clients intervening.
                pthread_mutex_lock(&lock);
                printf("PLAY|9|Client %d\n", clientNum + 1);
                player[clientNum].socket = client_sockfd;
                player[clientNum].queue_position = clientNum;
                clientNum++;
                acceptable++;
                pthread_mutex_unlock(&lock); 
            }
            //in the event where acceptable = 2, we have to set them up for the game by checking if they're in the active game. If they aren't, then we collect their file descriptrs 
            if(acceptable == 2){
                
                int sockets[acceptable];
                int position[acceptable];
                int count = 0; 
                for(int i = 0; i < MAX_CLIENTS; i++){
                    if(count == 2){
                        break;
                    }
                    if(player[i].active_game != 1){
                        player[i].active_game = 1;
                        sockets[count] = player[i].socket;
                        position[count] = i;
                        count++;
                    }
                }
                arg[threads].client1_fd = sockets[0];
                arg[threads].client2_fd = sockets[1];
                int error;
            
                if((error = pthread_create(&games[threads], NULL, start_game, (void*)&arg)) < 0){
                    fprintf(stderr, "Error creating a thread: %s\n", strerror(error));
                    continue;
                } else {
                    player[position[0]].pthread_position = threads;
                    player[position[1]].pthread_position = threads;
                }
                if((error = pthread_join(games[threads], NULL)) < 0){
                    fprintf(stderr, "Error joining thread: %s\n", strerror(error));
                    exit(EXIT_FAILURE);
                }
                thread++;
                
                
            }
            acceptable = 0;
            
        }
        
        
    }
    puts("Shutting down\n");
    close(server_sockfd);


    
    return 0; 
}