# The program's own files besides main.c (port/app): the window, presentation, input and screenshots. The screens they
# show are drawn by port/gfx/screens.c, which the port_gfx target builds.
target_sources(port_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/app/present.c")
