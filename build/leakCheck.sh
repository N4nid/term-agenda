#!/bin/bash
# requires valgrind

bash build.sh

valgrind ./term-agenda
#valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./term-agenda
