#! /bin/sh

valgrind -v --leak-check=yes --show-reachable=yes $@
