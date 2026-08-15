#!/bin/bash

# g++ main.cpp -o main -lglut -lGLU -lGL

# echo $1.cpp
CC=g++
LDFLAGS=`pkg-config --libs glew`
LDFLAGS="$LDFLAGS -lglut"

$CC $1.cpp $LDFLAGS -o $1
