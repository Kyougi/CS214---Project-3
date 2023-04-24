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
#include <errno.h>
#include <pthread.h>


#define MAX_CLIENTS 10
#define BUFFER 256
#define SERVER_PORT "4536"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


typedef struct player{
    char player_name[50];
    char side;
    int client_sock;
    char client_buffer[BUFFER/2];
} Playing;

typedef struct clients{
    int socket; //client socket
    struct sockaddr_storage address; //their information about their sockets.
    int pthread_position; //what thread are the two players are in
    int queue_position; 
    int active_game;
    char name[50]; //1 for currently in an active game and 0 otherwise.
} Clients;

struct thread_args{
    Clients *client1;
    Clients *client2;
};

void initialize_board(char (*table)[3]){
    pthread_mutex_lock(&lock);
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
            table[i][j] = '-';
        }
    }
    pthread_mutex_unlock(&lock);
}


int send_player(Playing *player, char * buffer){
    int error;
    if((error = write(player->client_sock, buffer, sizeof(buffer))) < 0){
        perror("Error in sending message to client.\n");
        close(player->client_sock);
    }
    return error;
}

int receive_player(Playing * player, char * buffer){
    int bytes;
    if((bytes = read(player->client_sock, buffer, sizeof(buffer))) < 0){
        perror("Error in reading message to client.\n");
        close(player->client_sock);
    }
    return bytes;
}

void print_board(char (*table)[3], Playing *player){
    char board_buffer[6];
    for(int i = 0; i < 3; i++){
        sprintf(board_buffer, "%c %c %c", table[i][0], table[i][1], table[i][2]);
        int error = send_player(player, board_buffer);
        if(error){
            break;
        }
    }
}

int options(Playing *player_one, Playing * player_two, char (*table)[3]){
    char server_buffer[BUFFER], error[100], protocol[100], draw;
    int position_chosen = 0;
    print_board(table, player_one);
    int x = 0, y = 0;
    while(position_chosen == 0){
        sprintf(server_buffer, "Select an option: %s (Choose either three protocols without spaces except for DRAW S):\n1. MOVE\n2. RSGN\n3. DRAW S\n", player_one->player_name);
        if((send_player(player_one, server_buffer)) < 0){
            close(player_two->client_sock);
            position_chosen = -1;
        } else {
            if(receive_player(player_one, player_one->client_buffer) < 0){
                close(player_two->client_sock);
                position_chosen = -1;
            } else {
                if(strcmp(player_one->client_buffer, "MOVE") == 0){
                    strcpy(protocol, player_one->client_buffer);
                    strcpy(server_buffer, "Choose a position between 1 and 3 (Input in the format x,y without spaces)\n");
                    if(send_player(player_one, server_buffer) < 0){
                        close(player_two->client_sock);
                        position_chosen = -1;
                    } else if(receive_player(player_one, player_one->client_buffer) < 0){
                        close(player_two->client_sock);
                        position_chosen = -1;
                    } else {
                        sscanf(player_one->client_buffer, "%d,%d", &x, &y);
                        if((x <= 0 && y <= 0) || (x > 3 && y > 3)){
                            strcpy(error, "Invalid position in board.");
                            sprintf(server_buffer, "INVL|%ld|%s|\n", sizeof(error), error);
                           if(send_player(player_one, server_buffer) < 0){
                                close(player_two->client_sock);
                                position_chosen = -1;
                            }
                            break;
                        } else if((table[x-1][y-1] == 'X') || (table[x-1][y-1] == 'O')){
                            strcpy(error, "Position has already been taken.");
                            sprintf(server_buffer, "INVL|%ld|%s|\n", sizeof(error), error);
                           if(send_player(player_one, server_buffer) < 0){
                                close(player_two->client_sock);
                                position_chosen = -1;
                            }
                            break;
                        } else{
                            table[x-1][y-1] = player_one->side;
                            printf("%s|3|%c|%d,%d\n", protocol, player_one->side, x, y);
                            sprintf(error, "%s has made a move!", player_one->player_name);
                            sprintf(server_buffer, "MOVD|%ld|%c|%d,%d|%s", sizeof(error), player_one->side, x, y, error);
                            if(send_player(player_one, server_buffer) < 0){
                                close(player_two->client_sock);
                                position_chosen = -1;
                                break;
                            }
                            if(send_player(player_two, server_buffer) < 0){
                                close(player_two->client_sock);
                                position_chosen = -1;
                                break;
                            }

                            position_chosen = 1;

                        }
                        
                    }

                    
                } else if(strcmp(player_one->client_buffer, "RSGN") == 0){
                    printf("%s|0\n", player_one->client_buffer);
                    position_chosen = 2;

                } else if(strcmp(player_one->client_buffer, "DRAW S") == 0){
                    sscanf(player_one->client_buffer, "%s %c", protocol, &draw);
                    printf("%s|1|%c|", protocol, draw);
                    sprintf(server_buffer, "%s|2|%c|", protocol, draw);
                    if(send_player(player_two, server_buffer) < 0){
                        close(player_one->client_sock);
                        position_chosen = -1;
                        break;
                    }
                    strcpy(server_buffer, "Do you DRAW A or DRAW R?\n");
                    if(send_player(player_two, server_buffer) < 0){
                        close(player_one->client_sock);
                        position_chosen = -1;
                        break;
                    }
                    if(receive_player(player_two, player_two->client_buffer) < 0){
                        close(player_one->client_sock);
                        position_chosen = -1;
                        break;
                    }
                    sscanf(player_two->client_buffer, "%s %c", protocol, &draw);
                    if(toupper(draw) == 'A'){
                        printf("%s|2|%c|\n", protocol, draw);
                        position_chosen = 3; 
                    } else if(toupper(draw)== 'R'){
                        printf("%s|2|%c|\n", protocol, draw);
                        sprintf(server_buffer, "%s|2|%c|\n", protocol, draw);
                        if(send_player(player_two, server_buffer) < 0){
                            close(player_one->client_sock);
                            position_chosen = -1;
                            break;
                        }
                        continue;

                    }


                } else {
                    strcpy(error, "Invalid option that doesn't exist in the list of options.\n");
                    sprintf(server_buffer, "INVL|%ld|%s|\n", sizeof(error), error);
                    if(send_player(player_one, server_buffer) < 0){
                        close(player_two->client_sock);
                        position_chosen = -1;
                    }
                    continue;
                }
            }
        }
    }
    return position_chosen;
}

int check_board(char table[3][3]){
    int win = 0;
    if(table[0][0] == table[1][1] && table[0][0] == table[2][2] && (table[0][0] == 'X' || table[0][0] == 'O')){
        win= 1;
    } else if(table[0][2] == table[1][1] && table[0][2] == table[2][0] && (table[0][2] == 'X' || table[0][2] == 'O')){
        win = 1;
    }
    for(int i = 0; i < 3; i++){
        if(table[i][0] == table[i][1] && table[i][0] == table[i][2] && (table[i][0] == 'X' || table[i][0] == 'O')){
            win = 1;
        } else if(table[0][i] == table[1][i] && table[0][i] == table[2][i] && (table[0][i] == 'X' || table[0][i] == 'O')){
            win = 1;
        }
    }
    return win;
}

//This is where the tic tac toe game is called. When the game is over, we close the file descriptors of the clients. 
void * start_game(void *arg){
    struct thread_args *clients = arg;
    Playing player_one, player_two;
    player_one.side = 'X', player_two.side = 'O';
    char server_buffer[BUFFER], table[3][3], confirm[50];
    int error;

    pthread_mutex_lock(&lock);
    strcpy(player_one.player_name, clients->client1->name), strcpy(player_two.player_name, clients->client2->name);
    player_one.client_sock = clients->client1->socket, player_two.client_sock = clients->client2->socket;
    sprintf(server_buffer, "BEGN|%ld|%c|%s|\n", sizeof(player_one.player_name), player_one.side, player_one.player_name);
    if((error = send_player(&player_two, server_buffer)) < 0){
        pthread_mutex_unlock(&lock);
        close(player_one.client_sock);
        return NULL;
    }
    sprintf(server_buffer, "BEGN|%ld|%c|%s|\n", sizeof(player_two.player_name), player_two.side, player_two.player_name);
    if((error = send_player(&player_one, server_buffer)) < 0){
        pthread_mutex_unlock(&lock);
        close(player_two.client_sock);
        return NULL;
    }

    initialize_board(table);

    int winner_decided = 0, choice = 0, turns = 0 ;
    
    while(winner_decided != 1 || turns != 8){
        choice = options(&player_one, &player_two, table);
        if(choice == -1){
            break;
        } else if(choice == 1){
            int win = check_board(table);
            if(win == 1){
                char confirm[50];
                sprintf(confirm, "%s (%c) has Won!\n", player_one.player_name, player_one.side);
                sprintf(server_buffer, "OVER|%ld|%s", sizeof(confirm), confirm);
                send_player(&player_one, server_buffer);
                send_player(&player_two, server_buffer);
                winner_decided = 1;
                continue;
            } else{
                turns++;
            }
        } else if(choice == 3){
            strcpy(confirm, "Both players deicded to draw!\n");
            sprintf(server_buffer, "OVER|%ld|%s|", sizeof(confirm), confirm);
            send_player(&player_one, server_buffer);
            send_player(&player_two, server_buffer);
            winner_decided = 1;
            continue;
        } else if(choice == 2){
            sprintf(confirm, "%s has decided to resign!\n", player_one.player_name);
            sprintf(server_buffer, "OVER|%ld|%s",sizeof(confirm), confirm);
            send_player(&player_one, server_buffer);
            send_player(&player_two, server_buffer);
            winner_decided = 1;
            continue;
        }
        choice = options(&player_two, &player_one, table);
        if(choice == -1){
            break;
        } else if(choice == 1){
            int win = check_board(table);
            if(win == 1){
                sprintf(confirm, "%s (%c) has Won!\n", player_two.player_name, player_two.side);
                sprintf(server_buffer, "OVER|%ld|%s", sizeof(confirm), confirm);
                send_player(&player_one, server_buffer);
                send_player(&player_two, server_buffer);
                winner_decided = 1;
                continue;
            } else{
                turns++;
            }
        } else if(choice == 3){
            strcpy(confirm, "Both players deicded to draw!\n");
            sprintf(server_buffer, "OVER|%ld|%s|", sizeof(confirm), confirm);
            send_player(&player_one, server_buffer);
            send_player(&player_two, server_buffer);
            winner_decided = 1;
            continue;
        } else if(choice == 2){
            sprintf(confirm, "%s has decided to resign!\n", player_two.player_name);
            sprintf(server_buffer, "OVER|%ld|%s",sizeof(confirm), confirm);
            send_player(&player_one, server_buffer);
            send_player(&player_two, server_buffer);
            winner_decided = 1;
            continue;
        }
    }

    if(turns == 8){
        strcpy(confirm, "It's a tie!\n");
        sprintf(server_buffer, "OVER|%ld|%s|", sizeof(confirm), confirm);
        send_player(&player_one, server_buffer);
        send_player(&player_two, server_buffer);
    }
    close(player_one.client_sock);
    close(player_two.client_sock);
    pthread_mutex_unlock(&lock);
    return 1;
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
                char name_buffer[BUFFER], server_buffer[BUFFER];
                strcpy(server_buffer, "Enter your name: ");
                if(write(client_sockfd, server_buffer, sizeof(server_buffer)) < 0){
                    perror("Error in sending message to client\n");
                    close(client_sockfd);
                    continue;
                }
                int bytes = read(client_sockfd, name_buffer, BUFFER);
                if(bytes < 0){
                    perror("Failed in obtaining message from client.");
                    close(client_sockfd);
                    continue;
                } else {
                    char buffer_check[BUFFER];
                    int real_bytes;
                    if(sscanf(name_buffer, "%s|%d|%s", buffer_check, &real_bytes, player[clientNum].name) != 3){
                        perror("Incorrect format.");
                        close(client_sockfd);
                        continue;
                    } else {
                        printf("%s|%d|%s", buffer_check, real_bytes, player[clientNum].name);
                        player[clientNum].socket = client_sockfd;
                        player[clientNum].queue_position = clientNum;
                        clientNum++;
                        acceptable++;
                    }
                }
     
            }
            //in the event where acceptable = 2, we have to set them up for the game by checking if they're in the active game. If they aren't, then we collect their file descriptrs 
            if(acceptable == 2){
                int position[acceptable];
                int count = 0; 
                for(int i = 0; i < MAX_CLIENTS; i++){
                    if(count == 2){
                        break;
                    }
                    if(player[i].active_game != 1){
                        player[i].active_game = 1;
                        position[count] = i;
                        count++;
                    }
                }
                arg[threads].client1 = &player[clientNum - 2];
                arg[threads].client2 = &player[clientNum - 1];
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
                threads++;
                
                
            }
            acceptable = 0;
            
        }
        
        
    }
    puts("Shutting down\n");
    close(server_sockfd);


    
    return 0; 
}