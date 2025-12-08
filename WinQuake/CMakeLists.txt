cmake_minimum_required(VERSION 3.22.1)

project(Quake VERSION 1.09 LANGUAGES C)

#
# misc stuff
#

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
    add_compile_definitions(LINUX)
endif()

# the Quake source code refers to this a little (as macOS I think)
if(APPLE)
  add_compile_definitions(NeXT)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DEBUG TRUE)
    add_compile_definitions(DEBUG)
endif()

# make the .so / .dylib generation path more deterministic (I think)
set(CMAKE_INSTALL_RPATH "@executable_path")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# this is how you tell emscripten to produce the final .html
if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

#
# CFLAGS and LDFLAGS
#

set(DRY_CFLAGS
    "-trigraphs" # Quake source uses this archaic character thing
    "-fno-common" # reduce scope of globals a bit
    "-Wall" # warn for everything
    "-Wextra" # warn for more than everything?
    "-Werror" # error for all warnings
    # disable a selection of warnings that Quake source contravenes
    "-Wno-comment"
    "-Wno-dangling-else"
    "-Wno-deprecated-declarations"
    "-Wno-deprecated-non-prototype"
    "-Wno-format-security"
    "-Wno-misleading-indentation"
    "-Wno-missing-field-initializers"
    "-Wno-pointer-sign"
    "-Wno-pointer-to-int-cast"
    "-Wno-shift-overflow"
    "-Wno-sign-compare"
    "-Wno-switch"
    "-Wno-trigraphs"
    "-Wno-unused-but-set-variable"
    "-Wno-unused-parameter"
    "-Wno-unused-variable"
    "-Wno-void-pointer-to-int-cast"
    "-Wno-error=unused-result"
    "-Wno-error=format-overflow"
    "-Wno-error=array-bounds"
    "-Wno-error=strict-aliasing"
    "-Wno-absolute-value"
    "-Wno-int-conversion"
    "-Wno-int-to-void-pointer-cast"
    "-Wno-cast-function-type-mismatch"
    "-Wno-semicolon-before-method-body"
)

set(DRY_LDFLAGS
)

if (EMSCRIPTEN)
  set(DRY_CFLAGS ${DRY_CFLAGS}
    "-sUSE_SDL=2" # compile the emscripten-provided SDL2
  )

  set(DRY_LDFLAGS ${DRY_LDFLAGS}
    "-sUSE_SDL=2" # link the emscripten-provided SDL2
    "-sINITIAL_MEMORY=1024MB" # bit of a thumb-suck
    "-sTOTAL_STACK=256MB" # bit of a thumb-suck
    "-sSTACK_SIZE=32MB"
    "-lidbfs.js"
    "-lwebsocket.js"
    "--shell-file=../shell.html"
    "-lopenal"
    "-sEXIT_RUNTIME"
    "-sASYNCIFY=1" # needed because the connect loop and probably other parts don't yield to the main loop
    "-sALLOW_BLOCKING_ON_MAIN_THREAD=1" # related to the above
    "-Wno-limited-postlink-optimizations"
  )
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DRY_CFLAGS ${DRY_CFLAGS}
    "-g" # debug symbols
  )
else()
  set(DRY_CFLAGS ${DRY_CFLAGS}
    "-O3"
    # note: don't enable this- we've been here buddy; it causes stack overflows with the QC VM (because it uses floats carefully)
    # "-ffast-math"
    )

  set(DRY_LDFLAGS ${DRY_LDFLAGS}
    "-O3"
    "-flto"
  )
endif()

if (EMSCRIPTEN)
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DRY_CFLAGS ${DRY_CFLAGS}
      # "-fsanitize=address,undefined,null"
      "-gsource-map"
    )

    set(DRY_LDFLAGS ${DRY_LDFLAGS}
      # "-fsanitize=address,undefined,null"
      "-gsource-map"
      "--source-map-base=./"
      "-sSTACK_OVERFLOW_CHECK=2"
      "-sCHECK_NULL_WRITES=1"
      "-sASSERTIONS=2"
    )
  endif()
else()
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DRY_CFLAGS ${DRY_CFLAGS}
        # "-fsanitize=address,undefined,null,pointer-compare,pointer-subtract"
    )

    set(DRY_LDFLAGS ${DRY_LDFLAGS}
      # "-fsanitize=address,undefined,null,pointer-compare,pointer-subtract"
    )
    endif()
endif()

string(JOIN " " DRY_CFLAGS_STR ${DRY_CFLAGS})
set(CMAKE_C_FLAGS ${DRY_CFLAGS_STR})
set(CMAKE_EXE_LINKER_FLAGS ${DRY_CFLAGS_STR})

string(JOIN " " DRY_LDFLAGS_STR ${DRY_LDFLAGS})
set(CMAKE_EXE_LINKER_FLAGS ${DRY_LDFLAGS_STR})

#
# third party libs
#

# this seems to be how you do "package management"
include(FetchContent)

# m = math
set(DRY_LIBS
  "m"
)

# emscripten bundles SDL2, so we only need to actually pull it for native builds
if (NOT EMSCRIPTEN)
  FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.32.4
  )

  FetchContent_MakeAvailable(SDL2)

  set(DRY_LIBS ${DRY_LIBS}
    "SDL2::SDL2"
  )
endif()

string(JOIN " " DRY_LIBS_STR ${DRY_LIBS})

#
# QuakeLib - most of the actual game; note: a bunch of files are intentionally
# left out here to ensure we're only compiling the things we need to support
# the SDL2 platform (platform = sys / vid / snd)
#

set(QUAKE_SOURCES
    adivtab.h
    anorm_dots.h
    anorms.h
    asm_draw.h
    asm_i386.h
    block16.h
    block8.h
    bspfile.h
    cd_null.c
    cdaudio.h
    chase.c
    cl_demo.c
    cl_input.c
    cl_main.c
    cl_parse.c
    cl_tent.c
    client.h
    cmd.c
    cmd.h
    common.c
    common.h
    console.c
    console.h
    crc.c
    crc.h
    cvar.c
    cvar.h
    d_edge.c
    d_fill.c
    d_iface.h
    d_ifacea.h
    d_init.c
    d_local.h
    d_modech.c
    d_part.c
    d_polyse.c
    d_scan.c
    d_sky.c
    d_sprite.c
    d_surf.c
    d_vars.c
    d_zpoint.c
    draw.c
    draw.h
    host.c
    host_cmd.c
    in_sdl2.c
    input.h
    keys.c
    keys.h
    mathlib.c
    mathlib.h
    menu.c
    menu.h
    model.c
    model.h
    modelgen.h
    mpdosock.h
    net.h
    net_bsd.c
    net_dgrm.c
    net_dgrm.h
    net_ipx.h
    net_loop.c
    net_loop.h
    net_main.c
    net_udp.c
    net_udp.h
    net_vcr.c
    net_vcr.h
    nonintel.c
    pr_cmds.c
    pr_comp.h
    pr_edict.c
    pr_exec.c
    progdefs.h
    progs.h
    protocol.h
    quakeasm.h
    quakedef.h
    r_aclip.c
    r_alias.c
    r_bsp.c
    r_draw.c
    r_edge.c
    r_efrag.c
    r_light.c
    r_local.h
    r_main.c
    r_misc.c
    r_part.c
    r_shared.h
    r_sky.c
    r_sprite.c
    r_surf.c
    r_vars.c
    render.h
    resource.h
    sbar.c
    sbar.h
    screen.c
    screen.h
    server.h
    snd_dma.c
    snd_mem.c
    snd_mix.c
    snd_sdl2.c
    sound.h
    spritegn.h
    sv_main.c
    sv_move.c
    sv_phys.c
    sv_user.c
    sys.h
    sys_sdl2.c
    sys_sdl2.h
    vgamodes.h
    vid.h
    vid_sdl2.c
    view.c
    view.h
    wad.c
    wad.h
    winquake.h
    world.c
    world.h
    zone.c
    zone.h
)

if(EMSCRIPTEN)
    list(APPEND QUAKE_SOURCES
        websockets/websockets.h
        websockets/websockets.c
        net_ws.c
        net_ws.h
    )
endif()

add_library(QuakeLib ${QUAKE_SOURCES})

set_target_properties(QuakeLib PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
if (NOT EMSCRIPTEN)
  target_link_libraries(QuakeLib PUBLIC m SDL2::SDL2)
else()
  target_link_libraries(QuakeLib PUBLIC m)
endif()

target_compile_options(QuakeLib PUBLIC ${DRY_CFLAGS})
target_link_options(QuakeLib PUBLIC)

#
# Tests
#

if (EMSCRIPTEN)
  add_executable(tests
    websockets/test_websockets.c
    websockets/websockets.h
    websockets/websockets.c
  )

  # need -sASYNCIFY=1 so we can write synchronous-looking code in our tests and use emscripten_sleep
  # need -sNO_EXIT_RUNTIME=1 just to avoid (harmless) explosions if events happen after we finish the tests
  set_target_properties(tests PROPERTIES
    LINK_FLAGS "-sNODERAWFS=1 -sASYNCIFY=1 -sNO_EXIT_RUNTIME=1 -Wno-limited-postlink-optimizations -gsource-map --source-map-base ./"
  )
endif()

#
# Quake - the main executable; this is mostly just a main() function that was
# extracted from sys_sdl2.c (which is largely copypasta from sys_linux.c)
#

add_executable(Quake
  main.c
)

set_target_properties(Quake PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
if (NOT EMSCRIPTEN)
  target_link_libraries(Quake PUBLIC QuakeLib m SDL2::SDL2)
else()
  target_link_libraries(Quake PUBLIC QuakeLib m)
endif()

target_compile_options(Quake PUBLIC ${DRY_CFLAGS})
target_link_options(Quake PUBLIC)

if (EMSCRIPTEN)
  set_target_properties(Quake PROPERTIES LINK_FLAGS "--preload-file ${CMAKE_SOURCE_DIR}/id1@./id1")
endif()

if (NOT EMSCRIPTEN)
  add_custom_command(TARGET Quake POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:SDL2::SDL2>" "$<TARGET_FILE_DIR:Quake>")
endif()

if(LINUX)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_command(TARGET Quake POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "libSDL2-2.0.so.0.3200.4" "$<TARGET_FILE_DIR:Quake>/libSDL2-2.0.so.0")
  else()
    add_custom_command(TARGET Quake POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "libSDL2-2.0d.so.0.3200.4" "$<TARGET_FILE_DIR:Quake>/libSDL2-2.0d.so.0")
  endif()
endif()
