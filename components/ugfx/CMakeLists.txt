set(COMPONENT_SRCS src/gfx_mk.c)
set(COMPONENT_ADD_INCLUDEDIRS .)

set(COMPONENT_REQUIRES main)

register_component()

target_compile_options(${COMPONENT_LIB} PRIVATE -fno-strict-aliasing)
