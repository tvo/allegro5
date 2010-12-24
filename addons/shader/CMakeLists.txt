set(SHADER_INCLUDE_FILES allegro5/allegro_shader.h)

set_our_header_properties(${SHADER_INCLUDE_FILES})

set(SHADER_SOURCES
    shader.c
    )

set(OUR_DEFINES "")
set(OUR_LIBS "${ALLEGRO_LINK_WITH}")

if(WANT_OPENGL)
    list(APPEND SHADER_SOURCES shader_glsl.c)
    set(OUR_DEFINES "${OUR_DEFINES} -DALLEGRO_CFG_WANT_GLSL_SHADERS")
endif(WANT_OPENGL)

if(WIN32 AND WANT_D3D)
    find_library(D3DX9 d3dx9)
    mark_as_advanced(D3DX9)
    if(D3DX9)
        list(APPEND SHADER_SOURCES shader_hlsl.cpp)
        set(OUR_DEFINES "${OUR_DEFINES} -DALLEGRO_CFG_WANT_HLSL_SHADERS")
        set(OUR_LIBS "${D3DX9};${OUR_LIBS}")
    endif(D3DX9)
endif(WIN32 AND WANT_D3D)

find_package(Cg)
if(CG_FOUND)
    list(APPEND SHADER_SOURCES shader_cg.c)
    set(OUR_DEFINES "${OUR_DEFINES} -DALLEGRO_CFG_WANT_CG_SHADERS")
    set(OUR_LIBS "${CG_LIBRARY};${OUR_LIBS}")
    if(WANT_OPENGL)
        set(OUR_LIBS "${CG_GL_LIBRARY};${OUR_LIBS}")
    endif(WANT_OPENGL)

    foreach(arg ${CG_CFLAGS})
        set(CG_CFLAGS_STRING "${CG_CFLAGS_STRING} ${arg}")
    endforeach(arg)

    set_source_files_properties(
        shader_cg.c
        PROPERTIES
        COMPILE_FLAGS "${CG_CFLAGS_STRING}"
        )
endif(CG_FOUND)

configure_file(
   allegro5/internal/aintern_shader_cfg.h.cmake
   ${CMAKE_BINARY_DIR}/include/allegro5/internal/aintern_shader_cfg.h
   )

add_our_library(allegro_shader
   "${SHADER_SOURCES};${SHADER_INCLUDE_FILES}"
   "${OUR_DEFINES}"
   "${OUR_LIBS}"
   )

set_our_framework_properties(allegro_shader AllegroShader-${ALLEGRO_SOVERSION})

install_our_library(allegro_shader)
install_our_headers(${SHADER_INCLUDE_FILES})

set(SHADER_LINK_WITH allegro_shader PARENT_SCOPE)

# vim: set sts=4 sw=4 et: