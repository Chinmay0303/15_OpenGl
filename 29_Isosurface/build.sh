source ../build_base.sh

LDFLAGS=`pkg-config --libs glew`
LDFLAGS="$LDFLAGS -lglut"

IMGUI_DIR="../Common/3rdparty/ImGui/GLFW"
IMGUI_OBJS="imgui.o imgui_draw.o imgui_tables.o imgui_widgets.o imgui_impl_opengl3.o"

for obj in $IMGUI_OBJS; do
    if [ ! -f "$obj" ]; then
        src="${obj%.o}.cpp"
        echo "Compiling $IMGUI_DIR/$src -> $obj..."
        $CC -O2 -c "$IMGUI_DIR/$src" $OGL_CPPFLAGS -o "$obj"
    fi
done

$CC -O2 main.cpp isohist.cpp ../Common/ogldev_util.cpp world.cpp view.cpp project.cpp volume.cpp colourmap.cpp $IMGUI_OBJS $OGL_CPPFLAGS $LDFLAGS -o main
