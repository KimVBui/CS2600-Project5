#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mosquitto.h>

char board[9] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
int mode = 2;
char current_player = 'X';

char check_winner() {
    int wins[8][3] = {{0,1,2}, {3,4,5}, {6,7,8}, {0,3,6}, {1,4,7}, {2,5,8}, {0,4,8}, {2,4,6}};
    for(int i = 0; i < 8; i++) {
        if(board[wins[i][0]] != ' ' && board[wins[i][0]] == board[wins[i][1]] && board[wins[i][0]] == board[wins[i][2]])
            return board[wins[i][0]];
    }
    return ' ';
}

void on_connect(struct mosquitto *mosq, void *obj, int rc) {
    if(rc == 0) {
        printf("Connected! Subscribing...\n");
        mosquitto_subscribe(mosq, NULL, "tictactoe/move", 0);
        mosquitto_subscribe(mosq, NULL, "game/config", 0);
    }
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    if (msg->payloadlen == 0) return;

    if (strcmp(msg->topic, "game/config") == 0) {
        if (strstr((char*)msg->payload, "MODE:1P")) mode = 1;
        else mode = 2;
        memset(board, ' ', 9);
        current_player = 'X';
        mosquitto_publish(mosq, NULL, "game/board", 9, board, 0, false);
        mosquitto_publish(mosq, NULL, "game/status", 7, "RESET X", 0, false);
        printf("Reset: Mode %d\n", mode);
    }
    else if (strcmp(msg->topic, "tictactoe/move") == 0) {

        char* payload = (char*)msg->payload;
        int pos = payload[0] - '0';

        if (pos >= 0 && pos <= 8 && board[pos] == ' ') {
            board[pos] = current_player;
            char status[128] = "";
            char winner = check_winner();

            if (winner != ' ') {
                sprintf(status, "WINNER: %c", winner);
            } else {
                current_player = (current_player == 'X') ? 'O' : 'X';
                sprintf(status, "Turn:%c Avail:", current_player);

                for(int i = 0; i < 9; i++) {
                     if(board[i] == ' ') {
                        char tmp[4];
                        sprintf(tmp, " %d", i);
                        strcat(status, tmp);
                     }
                }
            }

            mosquitto_publish(mosq, NULL, "game/board", 9, board, 0, false);
            mosquitto_publish(mosq, NULL, "game/status", strlen(status), status, 0, false);
            printf("Move: %d by %c. Board: [%.9s]\n", pos, board[pos], board);
        }
    }
}

int main() {
    mosquitto_lib_init();
    struct mosquitto *mosq = mosquitto_new("GCP_MAIN_ENGINE_V1", true, NULL);
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    mosquitto_connect(mosq, "localhost", 1883, 60);
    mosquitto_loop_forever(mosq, -1, 1);
    return 0;
}
