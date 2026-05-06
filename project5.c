#include "raylib.h"
#include <mosquitto.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_INPUT_CHARS 64

volatile char board[9] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
char urlInput[MAX_INPUT_CHARS] = "2600cppcs.duckdns.org"; //or any duckdns/IP
int letterCount = 21; 
bool is2PMode = true;
struct mosquitto *mosq = NULL;

// MQTT Callback: Updates the board when the server sends "Turn:..." or status updates
void on_message(struct mosquitto *m, void *obj, const struct mosquitto_message *msg) {
    printf("Topic: %s | Len: %d\n", msg->topic, msg->payloadlen);
     if (strcmp(msg->topic, "game/board") == 0 && msg->payloadlen >= 9) {
        
        char *incoming = (char *)msg->payload;
        for (int i = 0; i < 9; i++) {
            board[i] = incoming[i];
        }

       
        printf("GUI Board Updated: [%.9s]\n", (char*)board);
    } 
    else if (strcmp(msg->topic, "game/status") == 0) {
        printf("Status Update: %.*s\n", msg->payloadlen, (char*)msg->payload);
    }
  
}
void on_connect(struct mosquitto *m, void *obj, int rc) {
    if(rc == 0) {
        printf("Connected successfully! Subscribing now...\n");
        mosquitto_subscribe(m, NULL, "game/status", 0);
        mosquitto_subscribe(m, NULL, "game/board", 0);
    } else {
        printf("Connect failed with code %d\n", rc);
    }
}


bool DrawButton(Rectangle rect, const char* text, Color baseColor) {
    Vector2 mousePoint = GetMousePosition();
    bool hovering = CheckCollisionPointRec(mousePoint, rect);
    Color drawColor = hovering ? SKYBLUE : baseColor;
    DrawRectangleRec(rect, drawColor);
    DrawRectangleLinesEx(rect, 2, DARKGRAY);
    DrawText(text, rect.x + (rect.width/2 - MeasureText(text, 20)/2), rect.y + 10, 20, BLACK);
    return (hovering && IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
}

int main() {
    // Initialize MQTT
    mosquitto_lib_init();
    mosq = mosquitto_new("Laptop_Client", true, NULL);
    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_connect_callback_set(mosq, on_connect);

    mosquitto_loop_start(mosq);

    printf("Connecting to %s...\n", urlInput);
    mosquitto_connect(mosq, urlInput, 1883, 60);

    SetConfigFlags(FLAG_WINDOW_HIGHDPI); 
    InitWindow(800, 600, "Tic-Tac-Toe Laptop GUI");
    SetTargetFPS(60);

    Rectangle textBox = { 50, 50, 400, 40 };
    bool mouseOnText = false;

    while (!WindowShouldClose()) {
        mouseOnText = CheckCollisionPointRec(GetMousePosition(), textBox);
        if (mouseOnText) {
            SetMouseCursor(MOUSE_CURSOR_IBEAM);
            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= 32) && (key <= 125) && (letterCount < MAX_INPUT_CHARS - 1)) {
                    urlInput[letterCount] = (char)key;
                    urlInput[letterCount+1] = '\0';
                    letterCount++;
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && (letterCount > 0)) {
                letterCount--;
                urlInput[letterCount] = '\0';
            }
        } else SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("Enter DuckDNS Server URL:", 50, 25, 20, DARKGRAY);
        DrawRectangleRec(textBox, LIGHTGRAY);
        DrawRectangleLinesEx(textBox, 2, mouseOnText ? RED : DARKGRAY);
        DrawText(urlInput, textBox.x + 5, textBox.y + 8, 24, MAROON);

        if (DrawButton((Rectangle){ 500, 50, 120, 40 }, "1 Player", LIGHTGRAY)) {
            is2PMode = false;
            mosquitto_publish(mosq, NULL, "game/config", 7, "MODE:1P", 0, false);
        }
        if (DrawButton((Rectangle){ 630, 50, 120, 40 }, "2 Player", LIGHTGRAY)) {
            is2PMode = true;
            mosquitto_publish(mosq, NULL, "game/config", 7, "MODE:2P", 0, false);
        }

        DrawText(is2PMode ? "Mode: 2 Player (vs ESP32)" : "Mode: 1 Player (vs Bash Script)", 50, 100, 20, BLUE);

            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    int index = i * 3 + j;
                    Rectangle cell = { 250 + j*100, 200 + i*100, 100, 100 };
                    DrawRectangleLinesEx(cell, 3, BLACK);
                    
                    if (CheckCollisionPointRec(GetMousePosition(), cell) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (board[index] == ' '){
                
                            char moveStr[2];
                            sprintf(moveStr, "%d", index);
                            mosquitto_publish(mosq, NULL, "tictactoe/move", 1, moveStr, 0, false);
                            printf("Sent Move: %s\n", moveStr);
                
                        }
                    }
                    if (board[index] == 'X' || board[index] == 'x') {
                        DrawText("X", cell.x + 30, cell.y + 15, 80, RED);
                    } 
                    else if (board[index] == 'O' || board[index] == 'o' || board[index] == '0') {
                        DrawText("O", cell.x + 30, cell.y + 15, 80, BLUE);
                    }

                }
            }

        EndDrawing();
    }
    mosquitto_loop_stop(mosq, true);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    CloseWindow();
    return 0;
}
