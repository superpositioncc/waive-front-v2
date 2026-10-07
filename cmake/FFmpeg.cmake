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
# ---------- FFmpeg ----------- #
# ----------------------------- #
#
# Finds or builds FFmpeg and sets:
#   FFMPEG_INCLUDE_DIRS  headers
#   FFMPEG_LIBRARIES     libraries to link
#   FFMPEG_DLLS          DLLs to copy next to the binaries (Windows, shared builds only)
#   FFMPEG_DEPENDS       target that has to be built first (bundled build only)
#
# By default FFmpeg is linked statically, so users do not need to install it:
#   - macOS: a small FFmpeg is built from source with scripts/build-ffmpeg.sh.
#   - Windows: pass WAIVE_FFMPEG_ROOT, a prefix made with scripts/build-ffmpeg.sh
#     (see .github/workflows/build.yml). Without it, a shared FFmpeg is downloaded
#     and its DLLs are copied next to the binaries. Fine for development, but large.
#
# Options:
#   WAIVE_FFMPEG_ROOT=<prefix>  use a prebuilt FFmpeg (static, or shared with DLLs in bin/)
#   WAIVE_SYSTEM_FFMPEG=ON      link against the FFmpeg on this system, e.g. from Homebrew

set(FFMPEG_VERSION 8.1.3)
set(FFMPEG_URL https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.xz)
set(FFMPEG_SHA256 7138d28c96d9d3e3af4ee3d8cad72741f8ffb40da90c1112235dea3ecd3178a3)

set(FFMPEG_WINDOWS_URL https://github.com/GyanD/codexffmpeg/releases/download/8.1/ffmpeg-8.1-full_build-shared.zip)
set(FFMPEG_WINDOWS_SHA256 e184e012ca9a3527ed45d61f8e250c50b07b8661d90849820ea754239049efb4)

set(FFMPEG_COMPONENTS avfilter avformat avcodec swscale avutil)

option(WAIVE_SYSTEM_FFMPEG "Link against the FFmpeg installed on this system instead of a static one" OFF)
set(WAIVE_FFMPEG_ROOT "" CACHE PATH "Prefix of a prebuilt FFmpeg")

set(FFMPEG_INCLUDE_DIRS "")
set(FFMPEG_LIBRARIES "")
set(FFMPEG_DLLS "")
set(FFMPEG_DEPENDS "")

function(ffmpeg_use_prefix ROOT)
    set(libraries "")
    set(dlls "")

    foreach (component ${FFMPEG_COMPONENTS})
        set(found "")
        foreach (candidate ${ROOT}/lib/lib${component}.a ${ROOT}/lib/${component}.lib)
            if (EXISTS ${candidate} AND NOT found)
                set(found ${candidate})
            endif()
        endforeach()

        if (NOT found)
            message(FATAL_ERROR "FFmpeg library ${component} not found in ${ROOT}/lib")
        endif()

        list(APPEND libraries ${found})

        file(GLOB dll ${ROOT}/bin/${component}-*.dll)
        list(APPEND dlls ${dll})
    endforeach()

    # A shared FFmpeg also needs the DLLs its own DLLs depend on.
    if (dlls)
        file(GLOB swresample ${ROOT}/bin/swresample-*.dll)
        list(APPEND dlls ${swresample})
    endif()

    set(FFMPEG_INCLUDE_DIRS ${ROOT}/include PARENT_SCOPE)
    set(FFMPEG_LIBRARIES ${libraries} PARENT_SCOPE)
    set(FFMPEG_DLLS ${dlls} PARENT_SCOPE)
endfunction()

if (WAIVE_SYSTEM_FFMPEG)
    find_path(FFMPEG_INCLUDE_DIR libavcodec/avcodec.h)
    set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})

    foreach (component ${FFMPEG_COMPONENTS})
        find_library(FFMPEG_${component}_LIBRARY ${component})

        if (NOT FFMPEG_${component}_LIBRARY)
            message(FATAL_ERROR "FFmpeg library ${component} was not found. On macOS, install FFmpeg with: brew install ffmpeg")
        endif()

        list(APPEND FFMPEG_LIBRARIES ${FFMPEG_${component}_LIBRARY})
    endforeach()

    message("Using the FFmpeg installed on this system. The binaries will only run where the same FFmpeg is installed.")

elseif (WAIVE_FFMPEG_ROOT)
    ffmpeg_use_prefix(${WAIVE_FFMPEG_ROOT})
    message("Using FFmpeg from ${WAIVE_FFMPEG_ROOT}")

elseif (WINDOWS)
    set(root ${CMAKE_BINARY_DIR}/ffmpeg-shared)

    if (NOT EXISTS ${root})
        message("Downloading FFmpeg from ${FFMPEG_WINDOWS_URL}")
        file(DOWNLOAD ${FFMPEG_WINDOWS_URL} ${CMAKE_BINARY_DIR}/ffmpeg-shared.zip
            EXPECTED_HASH SHA256=${FFMPEG_WINDOWS_SHA256} SHOW_PROGRESS)
        file(ARCHIVE_EXTRACT INPUT ${CMAKE_BINARY_DIR}/ffmpeg-shared.zip DESTINATION ${CMAKE_BINARY_DIR}/ffmpeg-shared-tmp)
        file(GLOB extracted ${CMAKE_BINARY_DIR}/ffmpeg-shared-tmp/*)
        file(RENAME ${extracted} ${root})
        file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/ffmpeg-shared-tmp)
        file(REMOVE ${CMAKE_BINARY_DIR}/ffmpeg-shared.zip)
    endif()

    ffmpeg_use_prefix(${root})
    message("Using a shared FFmpeg. Its DLLs are copied next to the binaries. For a release, build with WAIVE_FFMPEG_ROOT.")

else()
    include(ExternalProject)

    set(root ${CMAKE_BINARY_DIR}/ffmpeg-static)
    set(script ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-ffmpeg.sh)

    set(architectures ${CMAKE_OSX_ARCHITECTURES})
    if (NOT architectures)
        set(architectures ${CMAKE_HOST_SYSTEM_PROCESSOR})
    endif()

    set(target "")
    if (CMAKE_OSX_DEPLOYMENT_TARGET)
        set(target MACOSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
    endif()

    # Build once per architecture, then merge the libraries into universal ones.
    set(build_commands "")
    foreach (arch ${architectures})
        list(APPEND build_commands COMMAND ${CMAKE_COMMAND} -E env ${target}
            bash ${script} <SOURCE_DIR> ${root}-${arch} ${arch})
    endforeach()

    list(GET architectures 0 first)
    list(APPEND build_commands COMMAND ${CMAKE_COMMAND} -E copy_directory ${root}-${first}/include ${root}/include)
    list(APPEND build_commands COMMAND ${CMAKE_COMMAND} -E make_directory ${root}/lib)

    set(byproducts "")
    foreach (component ${FFMPEG_COMPONENTS})
        set(slices "")
        foreach (arch ${architectures})
            list(APPEND slices ${root}-${arch}/lib/lib${component}.a)
        endforeach()

        if (APPLE)
            list(APPEND build_commands COMMAND lipo -create ${slices} -output ${root}/lib/lib${component}.a)
        else()
            list(APPEND build_commands COMMAND ${CMAKE_COMMAND} -E copy ${slices} ${root}/lib/lib${component}.a)
        endif()

        list(APPEND byproducts ${root}/lib/lib${component}.a)
        list(APPEND FFMPEG_LIBRARIES ${root}/lib/lib${component}.a)
    endforeach()

    ExternalProject_Add(ffmpeg
        URL ${FFMPEG_URL}
        URL_HASH SHA256=${FFMPEG_SHA256}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${build_commands}
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS ${byproducts}
    )

    set(FFMPEG_INCLUDE_DIRS ${root}/include)
    set(FFMPEG_DEPENDS ffmpeg)
    message("FFmpeg ${FFMPEG_VERSION} will be built from source for: ${architectures}")
endif()
