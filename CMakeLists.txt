
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
cmake_minimum_required(VERSION 3.7)

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
    download_and_extract(ffmpeg https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl-shared.zip ${CMAKE_BINARY_DIR})
elseif (MACOS)
    find_library(AVCODEC_LIBRARY avcodec)

    if (NOT AVCODEC_LIBRARY)
        find_program(HOMEBREW brew)

        if (HOMEBREW)
            message(FATAL_ERROR "FFMPEG libraries were not found. Install FFMPEG using homebrew.\n\nbrew install ffmpeg")
        else()
            message(FATAL_ERROR "FFMPEG libraries were not found. The best way to install FFMPEG on MacOS is using homebrew, but it is not installed on your system. Install it using the following command:\n\n/bin/bash -c \"$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\"\n\nThen install FFMPEG using:\n\nbrew install ffmpeg")
        endif()
    endif()
endif()

# --------------------------------- #
# --- Set up the project itself --- #
# --------------------------------- #

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WINDOWS)
    set(GLEW_USE_STATIC_LIBS ON)

    set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/glew;${CMAKE_BINARY_DIR}/ffmpeg)
    set(CMAKE_LIBRARY_PATH ${CMAKE_BINARY_DIR}/glew/lib/;${CMAKE_BINARY_DIR}/ffmpeg/lib/)

    find_package(GLEW REQUIRED)
endif()

find_library(AVCODEC_LIBRARY avcodec)
find_library(AVFILTER_LIBRARY avfilter)
find_library(AVFORMAT_LIBRARY avformat)
find_library(AVDEVICE_LIBRARY avdevice)
find_library(AVUTIL_LIBRARY avutil)
find_library(SWSCALE_LIBRARY swscale)

add_subdirectory(${CMAKE_BINARY_DIR}/dpf)

if (WINDOWS)
    set(TARGETS jack vst2 vst3)
else()
    set(TARGETS jack vst2 vst3 au)
endif()

if (MACOS)
    add_definitions(-DDGL_USE_OPENGL3)
else()
	add_definitions(-DIMGUI_IMPL_OPENGL_ES3 -DGLEW_STATIC -D_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING -D_WINSOCKAPI_)
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
)

# ----------------------------- #
# --- Include dependencies ---- #
# ----------------------------- #

target_include_directories(${NAME} PUBLIC src)
target_include_directories(${NAME} PUBLIC ${CMAKE_BINARY_DIR}/dpf-widgets/opengl)
target_include_directories(${NAME} PUBLIC ${CMAKE_BINARY_DIR}/tinyosc)
target_include_directories(${NAME} PUBLIC ${CMAKE_BINARY_DIR}/json/include)

if (MACOS)
    find_path(FFMPEG_INCLUDE_DIR libavcodec/avcodec.h)
    target_include_directories(${NAME} PUBLIC ${FFMPEG_INCLUDE_DIR})
elseif(WINDOWS)
    target_include_directories(${NAME} PUBLIC ${GLEW_INCLUDE_DIRS})
    target_include_directories(${NAME} PUBLIC ${CMAKE_BINARY_DIR}/dirent/include)
    target_include_directories(${NAME} PUBLIC ${CMAKE_BINARY_DIR}/ffmpeg/include)
    # target_include_directories(${NAME} PUBLIC glew/include)
endif()


# ----------------------------- #
# ----- Link dependencies ----- #
# ----------------------------- #

target_link_libraries(${NAME} PUBLIC ${AVCODEC_LIBRARY})
target_link_libraries(${NAME} PUBLIC ${AVFILTER_LIBRARY})
target_link_libraries(${NAME} PUBLIC ${AVFORMAT_LIBRARY})
target_link_libraries(${NAME} PUBLIC ${SWSCALE_LIBRARY})
target_link_libraries(${NAME} PUBLIC ${AVUTIL_LIBRARY})
target_link_libraries(${NAME} PUBLIC ${AVDEVICE_LIBRARY})

if (WINDOWS)
    target_link_libraries(${NAME} PUBLIC ${GLEW_LIBRARIES})
    target_link_libraries(${NAME} PUBLIC Ws2_32)
endif()
