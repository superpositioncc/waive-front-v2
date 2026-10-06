
# WAIVE-FRONT
# Copyright (C) 2024  Bram Bogaerts, Superposition

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# ----------------------------- #
# ----- Initial settings ------ #
# ----------------------------- #
cmake_minimum_required(VERSION 3.18)

# std::filesystem needs macOS 10.15. Set before project() so it applies everywhere.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

set(NAME WAIVE-FRONT-V2)
project(${NAME})

# ----------------------------- #
# --------- Check OS ---------- #
# ----------------------------- #
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(WINDOWS TRUE)
    message("Building for Windows")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOS TRUE)
    message("Building for MacOS")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(LINUX TRUE)
    message(FATAL_ERROR "Building for Linux has not been tested yet, but should be straightforward. Check out CMakeLists.txt and make adjustments as needed, and feel free to contribute with a pull request.")
endif()

# ----------------------------- #
# --- Download dependencies --- #
# ----------------------------- #
function (download_and_extract NAME URL DESTINATION)
    if (EXISTS ${DESTINATION}/${NAME})
        message("${NAME} already downloaded")
        return()
    endif()

    message("Downloading ${NAME} from ${URL}")
    file(DOWNLOAD ${URL} ${DESTINATION}/${NAME}.zip)

    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar tf ${DESTINATION}/${NAME}.zip
        OUTPUT_VARIABLE ZIP_CONTENTS
    )
    
    string(REGEX MATCH "^[^\n]+" EXTRACTED_NAME ${ZIP_CONTENTS})

    message("Extracting ${NAME}")
    file(ARCHIVE_EXTRACT INPUT ${DESTINATION}/${NAME}.zip DESTINATION ${DESTINATION})
    file(REMOVE ${DESTINATION}/${NAME}.zip)

    message("Renaming ${EXTRACTED_NAME} to ${NAME}")
    file(RENAME ${DESTINATION}/${EXTRACTED_NAME} ${DESTINATION}/${NAME})
endfunction()

download_and_extract(tinyosc https://github.com/mhroth/tinyosc/archive/7acc37ad4ea555c1ab8b89c4e94eac84e6af8d3a.zip ${CMAKE_BINARY_DIR})
download_and_extract(json https://github.com/nlohmann/json/archive/960b763ecd144f156d05ec61f577b04107290137.zip ${CMAKE_BINARY_DIR})
download_and_extract(dpf https://github.com/DISTRHO/DPF/archive/f5815166356e85a5fe244f6024c2e401f04b10fa.zip ${CMAKE_BINARY_DIR})
download_and_extract(dpf https://github.com/DISTRHO/DPF/archive/f5815166356e85a5fe244f6024c2e401f04b10fa.zip ${CMAKE_BINARY_DIR})

# check if pugl-upstream folder is empty
file(GLOB PUGL_UPSTREAM ${CMAKE_BINARY_DIR}/dpf/dgl/src/pugl-upstream/*)
if (NOT PUGL_UPSTREAM)
    file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/dpf/dgl/src/pugl-upstream)
endif()

download_and_extract(pugl-upstream https://github.com/DISTRHO/pugl/archive/e33b2f6b0cea6d6263990aa9abe6a69fdfba5973.zip ${CMAKE_BINARY_DIR}/dpf/dgl/src)
download_and_extract(dpf-widgets https://github.com/superpositioncc/DPF-Widgets/archive/880ce983b170a71c9fe400435b9b8185ca4fc8ed.zip ${CMAKE_BINARY_DIR})

if (WINDOWS)
    download_and_extract(glew https://sourceforge.net/projects/glew/files/glew/2.1.0/glew-2.1.0-win32.zip ${CMAKE_BINARY_DIR})
    download_and_extract(dirent https://github.com/tronkko/dirent/archive/47480bb729db38120a5c0b6f1e201abf0982f6a8.zip ${CMAKE_BINARY_DIR})
endif()

include(cmake/FFmpeg.cmake)

# --------------------------------- #
# --- Set up the project itself --- #
# --------------------------------- #

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WINDOWS)
    # Link the C/C++ runtime statically, so users need no Visual C++ redistributable
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    set(GLEW_USE_STATIC_LIBS ON)

    set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/glew)
    set(CMAKE_LIBRARY_PATH ${CMAKE_BINARY_DIR}/glew/lib/)

    find_package(GLEW REQUIRED)
endif()

# DPF links the standalone app against SDL2 when pkg-config finds it, e.g. from
# Homebrew, and the app then only runs where SDL2 is installed. On macOS and
# Windows the built-in audio fallback (RtAudio) is enough, so hide pkg-config packages.
if (MACOS OR WINDOWS)
    set(ENV{PKG_CONFIG_LIBDIR} ${CMAKE_BINARY_DIR}/no-pkg-config)
endif()

add_subdirectory(${CMAKE_BINARY_DIR}/dpf ${CMAKE_BINARY_DIR}/dpf-build)

if (WINDOWS)
    set(TARGETS jack vst2 vst3)
else()
    set(TARGETS jack vst2 vst3 au)
endif()

if (MACOS)
    add_definitions(-DDGL_USE_OPENGL3)
else()
	add_definitions(-DIMGUI_IMPL_OPENGL_ES3 -DGLEW_STATIC -D_WINSOCKAPI_)
endif()

dpf_add_plugin(${NAME}
    TARGETS ${TARGETS}
    FILES_DSP
        src/WaiveFrontPlugin.cpp
    FILES_UI
        ${CMAKE_BINARY_DIR}/dpf-widgets/opengl/DearImGui.cpp
        src/WaiveFrontPluginUI.cpp
        src/data/DataSources.cpp
        src/data/DataSource.cpp
        ${CMAKE_BINARY_DIR}/tinyosc/tinyosc.c
        src/util/DelayLoad.cpp
)

if (FFMPEG_DEPENDS)
    add_dependencies(${NAME}-ui ${FFMPEG_DEPENDS})
    add_dependencies(${NAME}-dsp ${FFMPEG_DEPENDS})
endif()

# ----------------------------- #
# --- Include dependencies ---- #
# ----------------------------- #

target_include_directories(${NAME} PUBLIC src)
target_include_directories(${NAME} PUBLIC ${CMAKE_BINARY_DIR}/dpf-widgets/opengl)
target_include_directories(${NAME} PUBLIC ${CMAKE_BINARY_DIR}/tinyosc)
target_include_directories(${NAME} PUBLIC ${CMAKE_BINARY_DIR}/json/include)

target_include_directories(${NAME} PUBLIC ${FFMPEG_INCLUDE_DIRS})

if (WINDOWS)
    target_include_directories(${NAME} PUBLIC ${GLEW_INCLUDE_DIRS})
    target_include_directories(${NAME} PUBLIC ${CMAKE_BINARY_DIR}/dirent/include)
endif()


# ----------------------------- #
# ----- Link dependencies ----- #
# ----------------------------- #

target_link_libraries(${NAME} PUBLIC ${FFMPEG_LIBRARIES})

if (WINDOWS)
    target_link_libraries(${NAME} PUBLIC ${GLEW_LIBRARIES})
    target_link_libraries(${NAME} PUBLIC Ws2_32)
    # A static FFmpeg uses BCryptGenRandom
    target_link_libraries(${NAME} PUBLIC bcrypt)
endif()

# A shared FFmpeg on Windows: load its DLLs from the folder of each binary
# (see src/util/DelayLoad.cpp) and copy them there.
if (FFMPEG_DLLS)
    target_link_libraries(${NAME} PUBLIC delayimp)

    foreach (dll ${FFMPEG_DLLS})
        get_filename_component(dll_name ${dll} NAME)
        target_link_options(${NAME} INTERFACE /DELAYLOAD:${dll_name})
    endforeach()

    foreach (format jack vst2 vst3)
        if (TARGET ${NAME}-${format})
            add_custom_command(TARGET ${NAME}-${format} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FFMPEG_DLLS} $<TARGET_FILE_DIR:${NAME}-${format}>)
        endif()
    endforeach()
endif()
