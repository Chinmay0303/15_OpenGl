#!/bin/bash

# g++ main.cpp -o main -lglut -lGLU -lGL

CC=g++
LDFLAGS=`pkg-config --libs glew`
LDFLAGS="$LDFLAGS -lglut"

$CC main.cpp $LDFLAGS -o main
