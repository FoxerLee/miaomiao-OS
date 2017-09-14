#include "stdio.h"
// w--1 a--2 s--3 d--4
int main() {
    char map[10][10] = {
        '#','#','#','#','#','#','#','#','#','#',
        '#','@','.','.','.','.','.','#','#','#',
        '#','#','.','#','#','#','.','.','@','#',
        '#','#','.','#','#','.','.','#','.','#',
        '#','.','.','#','#','.','#','#','.','#',
        '#','#','.','.','.','.','.','.','.','#',
        '#','.','.','#','#','.','#','#','.','#',
        '#','#','.','#','#','.','#','#','.','#',
        '#','#','.','.','.','.','#','#','.','#',
        '#','#','#','#','#','&','#','#','#','#',
        
    };
    
    int is_finished = -1;
    int monster_1[2] = {1, 1};
    int monster_2[2] = {2, 8};
    int player[2] = {9, 5};
    int next_place[2];
    int end[2] = {9, 5};
    char com[2];
    int move[2] = {-1, 1};
    memset(com, 0, 2);

    printf("=============================================\n"
           "                 escape\n"
           "=============================================\n"
           "game rule:\n"
           "  1. control little '&' to escape monster'@'\n"
           "  2. eat all to bean '.'\n"
           "  3. back to where you start and escape!\n"
           "control:\n"
           "  4. w a s d to move\n"
           "  5. q to quit\n"
           "=============================================\n");

    while (com[0] != 'q') {
        int n, m;
        for(n = 0; n < 10; n++) {
            for(m = 0; m < 10; m++)
                printf("%c", map[n][m]);
            printf("\n");
        }

        if ((player[0] == monster_1[0] && player[1] == monster_1[1]) || 
            (player[0] == monster_2[0] && player[1] == monster_2[1])) {
            
            printf("You die~\n");
            com[0] = 'q';
            return 0;
        }
        is_finished = 0;
        if (player[0] == end[0] && player[1] == end[1]) {
            for (n = 0; n < 10; n++) {
                for (m = 0; m < 10; m++) {
                    if (map[n][m] == '.')
                        is_finished = -1;
                }
            }
            
            if (is_finished == 0) {
                printf("You win~\n");
                com[0] = 'q';
                return 0;
            }
        }

        printf("move: ");
        int i = read(0, com, 2);
        if (com[0] == 'w') {
            int x = player[0] - 1;
            int y = player[1];
            if (map[x][y] != '#') {
                map[x+1][y] = ' ';
                map[x][y] = '&';
                player[0] = x;
                
            }
            else {
                printf("You hit the wall!\n");
            }
        }
        if (com[0] == 'a') {
            int x = player[0];
            int y = player[1] - 1;
            if (map[x][y] != '#') {
                map[x][y+1] = ' ';
                map[x][y] = '&';
                player[1] = y;
                
            }
            else {
                printf("You hit the wall!\n");
            }
        }
        if (com[0] == 's') {
            int x = player[0] + 1;
            int y = player[1];
            if (map[x][y] != '#') {
                map[x-1][y] = ' ';
                map[x][y] = '&';
                player[0] = x;
                
            }
            else {
                printf("You hit the wall!\n");
            }
        }
        if (com[0] == 'd') {
            int x = player[0];
            int y = player[1] + 1;
            if (map[x][y] != '#') {
                map[x][y-1] = ' ';
                map[x][y] = '&';
                player[1] = y;
                
            }
            else {
                printf("You hit the wall!\n");
            }
        }
        
        else if (com[0] == 'q') {
            printf("quit game!\n");
            break;
        }

        //monster
        int m_x_1, m_y_1, m_x_2, m_y_2;
        int temp;
        for (temp = 0; temp < 2; temp++) {
            int x_1 = monster_1[0] + move[temp]; int y_1 = monster_1[1];
            int x_2 = monster_2[0] + move[temp]; int y_2 = monster_2[1];
            if (map[x_1][y_1] != '#') {
                m_x_1 = x_1;
                m_y_1 = y_1;
            }
            if (map[x_2][y_2] != '#') {
                m_x_2 = x_2;
                m_y_2 = y_2;
            }
        }
        for (temp = 0; temp < 2; temp++) {
            int x_1 = monster_1[0]; int y_1 = monster_1[1] + move[temp];
            int x_2 = monster_2[0]; int y_2 = monster_2[1] + move[temp];
            if (map[x_1][y_1] != '#') {
                m_x_1 = x_1;
                m_y_1 = y_1;
            }
            if (map[x_2][y_2] != '#') {
                m_x_2 = x_2;
                m_y_2 = y_2;
            }
        }

        int temp_1, temp_2;
        temp_1 = monster_1[0]; temp_2 = monster_1[1];
        map[temp_1][temp_2] = '.';
        map[m_x_1][m_y_1] = '@';
        monster_1[0] = m_x_1; monster_1[1] = m_y_1;

        temp_1 = monster_2[0]; temp_2 = monster_2[1];
        map[temp_1][temp_2] = '.';
        map[m_x_2][m_y_2] = '@';
        monster_2[0] = m_x_2; monster_2[1] = m_y_2;
    }

    return 0;
    
}

