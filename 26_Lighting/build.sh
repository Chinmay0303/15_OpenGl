source ../build_base.sh

LDFLAGS=`pkg-config --libs glew`
LDFLAGS="$LDFLAGS -lglut"

$CC main.cpp ../Common/ogldev_util.cpp world.cpp view.cpp project.cpp $OGL_CPPFLAGS $LDFLAGS -o main

