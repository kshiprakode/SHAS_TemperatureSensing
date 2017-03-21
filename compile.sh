#!/bin/bash
gcc gateway.c -o gateway -lpthread
gcc sensor.c -o sensor -lpthread
gcc smartdevice.c -o device -lpthread