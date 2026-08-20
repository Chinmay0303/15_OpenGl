#!/bin/bash

# g++ main.cpp -o main -lglut -lGLU -lGL

# echo $1.cpp
# CC=g++
# LDFLAGS=`pkg-config --libs glew`
# LDFLAGS="$LDFLAGS -lglut"

# $CC $1.cpp $LDFLAGS -o $1

# echo build $1.cpp


source ../build_base.sh

LDFLAGS=`pkg-config --libs glew`
LDFLAGS="$LDFLAGS -lglut"

$CC main.cpp ../Common/ogldev_util.cpp $OGL_CPPFLAGS $LDFLAGS -o main

