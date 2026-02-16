#!/bin/bash
# requires valgrind

gcc -o term-agenda -g3 ../src/main.c

#valgrind ./term-agenda
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./term-agenda "${@}" |& bat
