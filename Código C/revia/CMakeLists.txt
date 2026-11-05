cmake_minimum_required(VERSION 3.16)
project(revia C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# --- Raylib: usa a instalada (CMake/pkg-config) ou baixa e compila (precisa de internet) ---
find_package(raylib 4.5 QUIET)
if(NOT raylib_FOUND)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(RAYLIB QUIET raylib)
  endif()
endif()

if(NOT raylib_FOUND AND NOT RAYLIB_FOUND)
  message(STATUS "Raylib não encontrada: baixando a versão 5.0 com FetchContent")
  include(FetchContent)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(raylib GIT_REPOSITORY https://github.com/raysan5/raylib.git GIT_TAG 5.0)
  FetchContent_MakeAvailable(raylib)
endif()

file(GLOB_RECURSE REVIA_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.c)
add_executable(revia ${REVIA_SOURCES})
target_include_directories(revia PRIVATE ${CMAKE_SOURCE_DIR}/src)

if(TARGET raylib)
  target_link_libraries(revia PRIVATE raylib)
elseif(RAYLIB_FOUND)
  target_include_directories(revia PRIVATE ${RAYLIB_INCLUDE_DIRS})
  target_link_directories(revia PRIVATE ${RAYLIB_LIBRARY_DIRS})
  target_link_libraries(revia PRIVATE ${RAYLIB_LIBRARIES})
endif()

if(NOT MSVC)
  target_compile_options(revia PRIVATE -Wall -Wextra)
  target_link_libraries(revia PRIVATE m)
endif()

# As fases são lidas de levels/ em tempo de execução: copia para junto do executável.
add_custom_command(TARGET revia POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/levels $<TARGET_FILE_DIR:revia>/levels)

# --- Testes de lógica (não precisam da Raylib) ---
enable_testing()
add_executable(test_logic tests/test_logic.c
  src/core/config.c src/player/player.c src/player/physics.c src/world/level.c src/world/collision.c
  src/entities/entities.c src/narrative/dialogue.c src/narrative/missions.c src/narrative/decisions.c
  src/education/quiz.c src/education/bias_game.c src/education/codex.c
  src/systems/progression.c src/systems/save.c)
target_include_directories(test_logic PRIVATE ${CMAKE_SOURCE_DIR}/src)
if(NOT MSVC)
  target_link_libraries(test_logic PRIVATE m)
endif()
add_test(NAME logic COMMAND test_logic WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
