source ../build_base.sh

LDFLAGS=`pkg-config --libs glew`
LDFLAGS="$LDFLAGS -lglut"

$CC -O2 main.cpp ../Common/ogldev_util.cpp world.cpp view.cpp project.cpp slicing.cpp volume.cpp colourmap.cpp $OGL_CPPFLAGS $LDFLAGS -o main

