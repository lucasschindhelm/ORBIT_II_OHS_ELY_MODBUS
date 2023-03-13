#!/bin/bash
gcc -o testclient test_client.c -lm `pkg-config --cflags --libs libmodbus`
sudo ./testclient "192.168.0.1" "0" "1" "502" "0" "0" "0"
#args ip slave_id debug port anpassung_registernummer(-!) byteordnerändern? bitorder_in statuswort
