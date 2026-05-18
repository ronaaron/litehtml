# Install script for directory: /home/lxk/Desktop/lite-html/litehtml

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/lxk/Desktop/lite-html/litehtml/test_build/litehtml-tests-build/litehtml/liblitehtml.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/litehtml" TYPE FILE FILES
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/background.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/borders.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/codepoint.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/css_length.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/css_margins.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/css_offsets.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/css_position.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/css_selector.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/css_parser.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/css_tokenizer.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/document.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/document_container.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_anchor.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_base.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_before_after.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_body.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_break.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_cdata.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_comment.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_div.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_font.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_image.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_link.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_para.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_script.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_space.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_style.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_table.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_td.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_text.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_title.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/el_tr.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/element.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/encodings.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/html.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/html_tag.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/html_microsyntaxes.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/iterators.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/media_query.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/os_types.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/style.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/stylesheet.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/table.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/tstring_view.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/types.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/url.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/url_path.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/utf8_strings.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/web_color.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/num_cvt.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/css_properties.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/line_box.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/render_item.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/render_flex.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/render_image.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/render_inline.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/render_table.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/render_inline_context.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/render_block_context.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/render_block.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/master_css.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/string_id.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/formatting_context.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/flex_item.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/flex_line.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/gradient.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/font_description.h"
    "/home/lxk/Desktop/lite-html/litehtml/include/litehtml/scroll_view.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/litehtml" TYPE FILE FILES "/home/lxk/Desktop/lite-html/litehtml/cmake/litehtmlConfig.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/litehtml/litehtmlTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/litehtml/litehtmlTargets.cmake"
         "/home/lxk/Desktop/lite-html/litehtml/test_build/litehtml-tests-build/litehtml/CMakeFiles/Export/1858d3296707c77b4f85418fd0121701/litehtmlTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/litehtml/litehtmlTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/litehtml/litehtmlTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/litehtml" TYPE FILE FILES "/home/lxk/Desktop/lite-html/litehtml/test_build/litehtml-tests-build/litehtml/CMakeFiles/Export/1858d3296707c77b4f85418fd0121701/litehtmlTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^()$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/litehtml" TYPE FILE FILES "/home/lxk/Desktop/lite-html/litehtml/test_build/litehtml-tests-build/litehtml/CMakeFiles/Export/1858d3296707c77b4f85418fd0121701/litehtmlTargets-noconfig.cmake")
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/lxk/Desktop/lite-html/litehtml/test_build/litehtml-tests-build/litehtml/src/gumbo/cmake_install.cmake")

endif()

