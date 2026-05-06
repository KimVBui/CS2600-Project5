#!/bin/bash
BROKER="localhost"
CURRENT_MODE="2P"
BOT_MOVED=false

echo "Mode-Aware Bot started. Waiting for 1P mode..."


mosquitto_sub -h $BROKER -t "game/#" | while IFS= read -r msg
do

    if [[ "$msg" == *"MODE:1P"* ]]; then
        CURRENT_MODE="1P"
        BOT_MOVED=false
        echo "Bot Activated: 1P Mode."
    elif [[ "$msg" == *"MODE:2P"* ]]; then
        CURRENT_MODE="2P"
        BOT_MOVED=false
        echo "Bot Deactivated: 2P Mode."
    fi


    if [[ ${#msg} -eq 9 && "$msg" != *"MODE"* && "$msg" != *"Turn"* ]]; then
        current_board="$msg"
    fi


    if [[ "$msg" == *"Turn:X"* ]]; then
        BOT_MOVED=false
        echo "Human's turn. Bot unlocked."
    fi

    if [[ "$CURRENT_MODE" == "1P" ]] && [[ "$msg" == *"Turn:O"* ]] && [ "$BOT_MOVED" = false ]; then
        available=""
        for i in {0..8}; do
            char="${current_board:$i:1}"
            if [[ "$char" == " " ]] || [[ -z "$char" ]]; then
                available+="$i "
            fi
        done

        random_move=$(echo $available | xargs -n1 | shuf -n1)

        if [[ -n "$random_move" ]]; then
            BOT_MOVED=true
            echo "Bot moving to: $random_move"
            msg=""
            sleep 1
            mosquitto_pub -h $BROKER -t "tictactoe/move" -m "$random_move"
        fi
    fi
done