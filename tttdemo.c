#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct players{
    char name[20];
    char side;
    int wins;
    int lossess;
} Player;


void print_table(char (*table)[3]){
    printf("  %c | %c  | %c \n", table[0][0], table[0][1], table[0][2]);
    printf("----+----+----\n");
    printf("  %c | %c  | %c \n", table[1][0], table[1][1], table[1][2]);
    printf("----+----+----\n");
    printf("  %c | %c  | %c \n", table[2][0], table[2][1], table[2][2]);
}



void options(Player * player, char (*table)[3]){
    int position_chosen = 0, option = 0, x = 1, y = 1; 
    printf("Choose Between (1-5) Below, %s:\n", current_player->player_names);
    while(position_chosen != 1){
        printf("Current Positions: (%d, %d)\n", positionY, positionX);
        printf("1. LEFT\n2. RIGHT\n3. UP\n4. DOWN\n5. INSERT SIDE\n\nOption: ");
        scanf("%d", &option);
        printf("\n");
        if(option == 1){
            positionX -= 1;
            if(positionX < 1){
                printf("Out of bounds!\n\n");
                positionX += 1;
            } 
            print_table(table);
        } else if(option == 2){
            positionX += 1;
            if(positionX > 3){
                printf("Out of bounds!\n\n");
                
                positionX -= 1;
            }
            print_table(table);
        } else if(option == 3){
            positionY -= 1;
            if(positionY == 0){
                printf("Out of bounds!\n\n");
                positionY+=1;
            } 
            print_table(table);
        } else if(option == 4){
            positionY+=1;
            if(positionY > 3){
                printf("Out of bounds!\n\n");
                positionY -= 1;
            }
            print_table(table); 
        } else if(option == 5){
            if(table[positionY - 1][positionX -1 ] == 'X' || table[positionY - 1][positionX - 1] == 'O'){
                printf("Space already taken!\n\n");
                print_table(table);
            } else {
                table[positionY- 1][positionX - 1] = current_player->side;
                position_chosen = 1;
                
            }
        } else {
            //Where either inputs or character inputs gets checked.
            printf("Invalid option! Please choose between (1-5)!\n\n");
            print_table(table);
        }

    }

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

void random(Player ** player_one, Player ** player_two){
    int quarter = rand() % 2;
    
    if(quarter == 1){
        *player_one->side = 'X';
        *player_two->side = 'O';
    } else {
        *player_one->side = 'O';
        *player_two->side = 'X';
    }
}

void play_game(Player * player_one, Player * player_two){
    printf("Welcome to Tic Tac Toe!\n\n");
    char table[3][3];
    int turns = 0, winner = 0, choice = 0;
    random(&player_one, &player_two);
    printf("%s: %c\n%s: %c\n", player_one->names, player_one->side, player_one->names, player_one->side);

    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
            table[i][j] = ' ';
        }
    }

    while(choice != 2){
        while(turns != 8){
            print_table(table);
            options(player_one, table);
            winner = check_board(table);
            if(winner == 1){
                player_one->wins += 1;
                player_two->lossess += 1;
                turns = 0;
                break;
            }
            turns++;
            print_table(table);
            options(player_two, table);
            winner = check_board(table);
            if(winner == 1){
                player_two->wins += 1;
                player_one->lossess += 1;
                turns = 0;
                break;
            }
            turns++; 
            
        }
        if(turns == 8){
            printf("Both players tied!\n");
        }
        for(int i = 0; i < 3; i++){
            for(int i = 0; i < 3; i++)
        }
        printf("Player One: %d Wins, %d Lossess\n", player_one->wins, player_one->lossess);
        printf("Player Two: %d Wins, %d Lossess\n\n", player_two->wins, player_two ->lossess);
        printf("Continue? Choose the Numbered Action:\n");
        while(choice == 0 && (choice != 1 || choice != 2)){
            printf("1. CONTINUE\n2. EXIT\n Option: ");
            scanf("%d", &choice);
            if(choice < 0 || choice >= 3){
                printf("Invalid option! Choose a valid option!\n");
            }
        }
        
    }
}

int main(int argc, char * argv[argc + 1]){
    Player player_one, player_two;
    printf("Input the name of the first player: "), fscanf(stdin, "%s", player_one.names), printf("\n"), printf("Input the name of the second player: "), fscanf(stdin, "%s", player_two.names), printf("\n");
    player_one.wins = 0, player_two.wins = 0, player_one.lossess = 0, player_two.lossess = 0, tic_tac_toe_game(&player_one, &player_two);
    return 0;
}