# Install script for directory: /home/lxk/Desktop/lite-html/litehtml/src/gumbo

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "libraries" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/lxk/Desktop/lite-html/litehtml/test_build/litehtml-tests-build/litehtml/src/gumbo/libgumbo.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/gumbo" TYPE FILE FILES
    "/home/lxk/Desktop/lite-html/litehtml/src/gumbo/include/gumbo/attribute.h"
    "/home/lxk/Desktop/lite-html/litehtml/src/gumbo/include/gumbo/char_ref.h"
    "/home/lxk/Desktop/lite-html/litehtml/src/gumbo/include/gumbo/error.h"
    "/home/lxk/Desktop/lite-html/litehtml/src/gumbo/include/gumbo.h"
    "/home/lxk/Desktop/lite-html/litehtml/src/gumbo/include/gumbo/insertion_mode.h"
    "/home/lxk/Desktop/lite-html/litehtml/src/gumbo/include/gumbo/parser.h"
    "/home/lxk/Desktop/lite-html/litehtml/src/gumbo/include/gumbo/string_buffer.h"
    "/home/lxk/Desktop/lite-html/litehtml/src/gumbo/include/gumbo/string_piece.h"
    "/home/lxk/Desktop/lite-html/litehtml/src/gumbo/include/gumbo/tag_enum.h"
    "/home/lxk/Desktop/lite-html/litehtml/src/gumbo/include/gumbo/tag_gperf.h"
    "/home/lxk/Desktop/lite-html/litehtml/src/gumbo/include/gumbo/tag_sizes.h"
    "/home/lxk/Desktop/lite-html/litehtml/src/gumbo/include/gumbo/tag_strings.h"
    "/home/lxk/Desktop/lite-html/litehtml/src/gumbo/include/gumbo/token_type.h"
    "/home/lxk/Desktop/lite-html/litehtml/src/gumbo/include/gumbo/tokenizer.h"
    "/home/lxk/Desktop/lite-html/litehtml/src/gumbo/include/gumbo/tokenizer_states.h"
    "/home/lxk/Desktop/lite-html/litehtml/src/gumbo/include/gumbo/utf8.h"
    "/home/lxk/Desktop/lite-html/litehtml/src/gumbo/include/gumbo/util.h"
    "/home/lxk/Desktop/lite-html/litehtml/src/gumbo/include/gumbo/vector.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/gumbo/gumboConfig.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/gumbo/gumboConfig.cmake"
         "/home/lxk/Desktop/lite-html/litehtml/test_build/litehtml-tests-build/litehtml/src/gumbo/CMakeFiles/Export/df09b600d79ede856025bf0d0b984a6e/gumboConfig.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/gumbo/gumboConfig-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/gumbo/gumboConfig.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/gumbo" TYPE FILE FILES "/home/lxk/Desktop/lite-html/litehtml/test_build/litehtml-tests-build/litehtml/src/gumbo/CMakeFiles/Export/df09b600d79ede856025bf0d0b984a6e/gumboConfig.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^()$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/gumbo" TYPE FILE FILES "/home/lxk/Desktop/lite-html/litehtml/test_build/litehtml-tests-build/litehtml/src/gumbo/CMakeFiles/Export/df09b600d79ede856025bf0d0b984a6e/gumboConfig-noconfig.cmake")
  endif()
endif()

