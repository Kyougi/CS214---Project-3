#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <netdb.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#define HOST_NAME "localhost"
#define SERVICE "5566"
#define QUEUE_SIZE 8

#define X_TURN 1
#define O_TURN 2

#define PLAYER_STATE_WAIT	1
#define CURRENTLY_IN_GAME 2

#define PLAY_CMD	1
#define WAIT_CMD	2

#define OVER_CMD	9

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
volatile int active = 1;
int current_free = QUEUE_SIZE/2;
int server_fd = 0;

typedef struct players{
    char name[50];
    char side;
    int client_fd;
    char buffer[100];
} Playing;

struct game_state {
	Playing X;
	Playing O;
	int x_state;
	int o_state;
	int turn;
	char table[3][3];
	pthread_t game_id;
	bool begin_played;
};


typedef struct clients{
    int socket;
    struct sockaddr_storage addr;
    socklen_t socklength;
    char name[50]; 
} Clients;

Clients *client_list[QUEUE_SIZE];

struct begn_params{
    char name_x[20];
    char name_o[20];
    char x;
    char o;
};

struct move_params{
    int x;
    int y;
};

struct movd_params{
    char msg[256];
    char side;
    int x;
    int y;
};

struct draw_params{
    char accept;
    char reject;
    char name[20];
};

struct rsgn_params{
    char buffer[20];
};

union game_params{
    struct move_params move;
    struct begn_params begn;
    struct movd_params movd;
    struct rsgn_params rsgn;
    struct draw_params draw; 
};

struct game_state pre_def_games[QUEUE_SIZE/2];

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

int send_client(int * client_fd, char * buffer){
    int error;
    int batch;

    batch = strlen(buffer);
    setsockopt(server_fd, IPPROTO_TCP, TCP_NODELAY, &batch, sizeof(batch));
    if((error = write(*client_fd, buffer, sizeof(buffer))) > 0){
        goto im_return;
    } else{
        if(error == EINTR){
            printf("Client suddenly disconnected.\n");
            goto im_return;
        } else if(error == EPIPE){
            printf("Client pipe is closed on the other end.\n");
            goto im_return;
        } else if(error < 0){
            perror("Write error: ");
            goto im_return;
        }
    }

    im_return:
    return error; 
}

int receive_client(int * client_fd, char * buffer){
    int bytes;
    if((bytes = read(*client_fd, buffer, sizeof(buffer))) > 0){
        goto im_return;
    } else {
        if(bytes == 0){
            printf("Read function reached EOF. Immeditately terminating");
            goto im_return;
        } else if(bytes < 0){
            perror("read error: ");
        }
    }

    im_return:
    return bytes;
}

int parse_msg(char * msg, Clients ** con){
    Clients * ptr = *con;
    int cmd = -1;
    char protocol[4];
    if(!strncmp(msg, "PLAY", 4)){
        for(int i = 0; i < 4; i++){
            protocol[i] = msg[i];
        }
        strncpy(ptr->name, msg+5, 50);
        printf("%s|%ld|%s|\n", protocol, sizeof(ptr->name), ptr->name);
        cmd = PLAY_CMD;
    }
    printf("%s|%ld|%s|\n", protocol, sizeof(ptr->name), ptr->name);
    return cmd; 
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

int options(Playing *player_one, Playing * player_two, char (*table)[3]){
    char server_buffer[256], error[100];
    int position_chosen = 0;
    int x = 0, y = 0;
    while(position_chosen == 0){
            if(receive_client(&player_one->client_fd, player_one->buffer) < 0){
                position_chosen = -1;
                break;
            }
            if(strncmp(player_one->buffer, "MOVE", 4) == 0){
                if((sscanf(player_one->buffer, "MOVE %d,%d\n", &x, &y)) == 2 || (sscanf(player_one->buffer, "MOVE %d,%d", &x, &y)) == 2){
                    if((x < 0 || x > 3) && (y >3 || y < 0)){
                        strcpy(error, "Out of bounds!");
                        sprintf(server_buffer, "INVL|%ld|%s|", sizeof(error), error);
                        if(send_client(&player_one->client_fd, server_buffer) < 0){
                            position_chosen = -1;
                            break; 
                        continue;
                        } else if(table[x-1][y-1] == 'X' || table[x-1][y-1] == 'O'){
                        strcpy(error, "Space taken!");
                        sprintf(server_buffer, "INVL|%ld|%s|", sizeof(error), error);
                        if(send_client(&player_one->client_fd, server_buffer) < 0){
                            position_chosen = -1;
                            break; 
                        }
                        continue;
                        } else {
                        table[x-1][y-1] = player_one->side;
                        strcpy(error, " has made a move.");
                        sprintf(server_buffer,"MOVD|%ld|%c|%d,%d|%s %s|", sizeof(error), player_one->side, x, y, player_one->name, error);
                        if(send_client(&player_one->client_fd, server_buffer) < 0){
                            position_chosen = -1;
                            break; 
                        }
                        if(send_client(&player_two->client_fd, server_buffer) < 0){
                            position_chosen = -1;
                            break; 
                        }
                        position_chosen = 1;
                        break;
                        }
                }
            } else if(strncmp(player_one->buffer, "RSGN", 4) == 0){
                position_chosen = 2;
                break;
            } else if(strncmp(player_one->buffer, "DRAW", 4) == 0){
                strcpy(server_buffer, "DRAW|1|S|\n");
                if(send_client(&player_two->client_fd, server_buffer) < 0){
                    position_chosen = -1;
                    break; 
                }
                if(receive_client(&player_two->client_fd, player_two->buffer) < 0){
                    position_chosen = -1;
                    break;
                }
                if(strncmp(player_two->buffer, "DRAW", 4) == 0){
                    char type;
                    if((sscanf(player_one->buffer, "DRAW %c\n", &type)) == 1 || (sscanf(player_one->buffer, "DRAW %c\n", &type)) == 1){
                        if(toupper(type) == 'A'){
                            position_chosen = 3;
                            break;
                        } else if(toupper(type) == 'R'){
                            continue;
                        }
                    }
                }

            } else {
                strcpy(error, "Invalid Protocol!");
                sprintf(server_buffer,"INVL|%ld|%s|", sizeof(error), error);
                if(send_client(&player_one->client_fd, server_buffer)){
                    position_chosen = -1;
                    break;
                }
                continue;
            }
        }
    }
    return position_chosen;
}

void * play_game(void *context){
	struct game_state *game = (struct game_state *)context;
	char msg[256];
	int option = 0, turns = 0;

	//pthread_mutex_lock(&lock);
	//strcpy(game->X.name, game->x_name), strcpy(game->O.name, game->o_name);
	//game->X.client_fd = game->fd_x, game->O.client_fd = game->fd_o;
	game->X.side = 'X', game->O.side = 'O';

	while(1){
		switch(game->turn){
		case X_TURN:
			option = options(&game->X, &game->O, game->table);
			if(option == -1){
				//goto unlock;
				goto game_completed;
			} else if(option == 1){
				int winner = 0;
				winner = check_board(game->table);
				if(winner == 1){
					sprintf(msg, "OVER|%s has won the game!", game->X.name);
					send_client(&game->X.client_fd, msg);
					send_client(&game->O.client_fd, msg);
					//goto unlock;
					goto game_completed;
				} else {
					game->turn = O_TURN;
					turns++;
				}
			} else if(option == 2){
				sprintf(msg, "OVER %s has resigned!", game->X.name);
				send_client(&game->X.client_fd, msg);
				send_client(&game->O.client_fd, msg);
				//goto unlock;
				goto game_completed;
			} else if(option == 3){
				strcpy(msg, "OVER Both players have agreed to draw!");
				send_client(&game->X.client_fd, msg);
				send_client(&game->O.client_fd, msg);
				//goto unlock;
				goto game_completed;
			}
		default:
			option = options(&game->O, &game->X, game->table);
			if(option == -1){
				//goto unlock;
				goto game_completed;
			} else if(option == 1) {
				int winner = 0;
				winner = check_board(game->table);
				if(winner == 1){
					sprintf(msg, "OVER %s has won the game!", game->O.name);
					send_client(&game->X.client_fd, msg);
					send_client(&game->O.client_fd, msg);
					//goto unlock;
					goto game_completed;
				} else {
					game->turn = X_TURN;
					turns++;
				}
			} else if(option == 2){
				sprintf(msg, "OVER %s has resigned!", game->O.name);
				send_client(&game->X.client_fd, msg);
				send_client(&game->O.client_fd, msg);
				//goto unlock;
				goto game_completed;
			} else if(option == 3){
				strcpy(msg, "OVER Both players have agreed to draw!");
				send_client(&game->X.client_fd, msg);
				send_client(&game->O.client_fd, msg);
				//goto unlock;
				goto game_completed;
			}
		}
	}

	//unlock:
	//pthread_mutex_unlock(&lock);
game_completed:
	close(game->X.client_fd);
	close(game->O.client_fd);
	pthread_exit(NULL);
}

int main(int argc, char * argv[argc + 1]){
    struct addrinfo host_hints, *result_list, *results;
    int client_fd, clientNum = 0, error = 0, cmd =0;
    Clients * con;
    char server_buffer[256];
    struct game_state *game, *newgame;  
    //Game * ttt_game;

    memset(&host_hints, 0, sizeof(struct addrinfo));
    host_hints.ai_family = AF_INET;
    host_hints.ai_socktype = SOCK_STREAM;
    host_hints.ai_flags = AI_PASSIVE;

    sigset_t mask; 
    char addr_buf[64];
    int batch = 4;

    install_handlers(&mask);


    if((error = getaddrinfo(HOST_NAME, SERVICE, &host_hints, &result_list)) < 0){
        fprintf(stderr, "Error on getting address: %s\n", strerror(error));
        exit(EXIT_FAILURE);
    }

    for(results = result_list; results != NULL; results = results->ai_next){
	    if (results->ai_family != AF_INET)
		    continue;
	    inet_ntop(AF_INET, &((struct sockaddr_in *)results->ai_addr)->sin_addr, addr_buf, sizeof(addr_buf));
	    fprintf(stdout, "Opening socket on %s\n", addr_buf);
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
	    setsockopt(server_fd, IPPROTO_TCP, TCP_NODELAY, &batch, sizeof(batch));
	    break;
    }
    
    freeaddrinfo(result_list);
    if(results == NULL){
        perror("Cannot bind");
        exit(EXIT_FAILURE);
    }

    if(server_fd < 0){
        exit(EXIT_FAILURE);
    }
    //pthread_mutex_init(&lock, NULL);
    printf("%s is listening for incoming connections on port %s...\n", HOST_NAME, SERVICE);

    while(active){
	    con = (Clients*)malloc(sizeof(Clients));

	    client_fd = accept(server_fd, (struct sockaddr*)&con->addr, &con->socklength);
        if(client_fd > 0){
             char client_buffer[256];
            //pthread_mutex_lock(&lock);
            error = receive_client(&client_fd, client_buffer);
            if(error <= 0){
                close(client_fd);
                //goto escape;
            } else {
                cmd = parse_msg(client_buffer, &con);
                con->socket = client_fd;
                client_list[clientNum] = con;
                //goto escape;
            }
            //escape:
        } else {
            if(client_fd == ECONNABORTED){
                printf("Connection has been aborted.\n");
                sleep(10);
                continue;
            } else if(client_fd == EINTR){
                printf("Connection has been interrupted or immediately cancelled.");
                sleep(10);
                continue;
            }
        }
        switch(cmd){
            case PLAY_CMD:
                newgame = NULL;
                for(int i = 0; i < current_free; i++){
                    if(pre_def_games[i].x_state == PLAYER_STATE_WAIT){
                        newgame = &pre_def_games[i];
                    }
                }
                if(newgame && newgame->x_state == PLAYER_STATE_WAIT){
                    char buf[256];
                    newgame->O.client_fd = client_list[clientNum]->socket;
		    strcpy(newgame->O.name, client_list[clientNum]->name);
		    newgame->o_state = PLAYER_STATE_WAIT;
		    sprintf(buf, "BEGN|%ld|X|%s", sizeof(newgame->X.name), newgame->X.name);
		    write(newgame->X.client_fd, buf, sizeof(buf));
		    sprintf(buf, "BEGN|%ld|O|%s", sizeof(newgame->O.name), newgame->O.name);
		    write(newgame->O.client_fd, buf, sizeof(buf));
		    newgame->turn = X_TURN;
		    /* remove this game from wait list */
                    pthread_sigmask(SIG_BLOCK, &mask, NULL);
		            pthread_create(&newgame->game_id, NULL, play_game, newgame);
                    pthread_detach(newgame->game_id);
                    pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
                } else {
			//pthread_mutex_lock(&lock);
                    char protocol[5];
		    game = &pre_def_games[current_free++];
                    strcpy(game->X.name, client_list[clientNum]->name);
                    game->x_state = PLAYER_STATE_WAIT;
                    game->begin_played = 1;
                    game->X.client_fd = client_list[clientNum]->socket;
                    strcpy(protocol,"WAIT");
                    write(client_list[clientNum]->socket, protocol, sizeof(protocol));
                }
		break;

            default:
                sprintf(server_buffer, "INVL|Invalid protocols and/or format, client %d. Please type in the format specified next time.\n", client_fd);
                write(client_fd, server_buffer, sizeof(server_buffer));
                close(client_fd);
                continue; 
        }

	clientNum++;
    }
    return 0;
}
