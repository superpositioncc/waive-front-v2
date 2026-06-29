# ----------------------------------------------------------------------------- #
# Minimal, decode-only FFmpeg for self-contained ("specific user") builds.
#
# Enabled with -DBUNDLE_FFMPEG=ON. Builds a stripped FFmpeg from source and links
# it statically, so the resulting binaries need no system/Homebrew FFmpeg and are
# version-independent (they carry their own copy). Only what WAIVE actually needs
# is enabled: mov/mp4 demuxing, common video decoders, swscale RGB conversion and
# the palettegen filter graph. No encoders are built, so there are no GPL codecs
# (x264/x265) and essentially no external dependencies.
#
# This module is used for Apple/Unix builds. Windows bundling uses a prebuilt
# LGPL shared FFmpeg (handled in CMakeLists.txt).
# ----------------------------------------------------------------------------- #

include(ExternalProject)

set(FFMPEG_VERSION 7.1)
set(FFMPEG_PREFIX ${CMAKE_BINARY_DIR}/ffmpeg-min)

# Components WAIVE needs (see src/video/VideoLoader.cpp):
#  - demuxer mov: covers .mp4/.m4v/.mov
#  - decoders: the codecs found in the archive footage (all internal/LGPL, no x264/x265 needed)
#  - parsers/bsf: for feeding packets through av_parser_parse2 + the mp4->annexb conversion
#  - swscale: decoded frame -> RGB24
#  - filters buffer/buffersink/palettegen (+ auto-inserted format/scale): 5-colour palette extraction
set(FFMPEG_CONFIGURE_FLAGS
    --prefix=<INSTALL_DIR>
    --disable-shared --enable-static --enable-pic
    --disable-programs --disable-doc --disable-htmlpages --disable-manpages
    --disable-network --disable-debug --disable-autodetect
    --disable-avdevice --disable-postproc
    --disable-everything
    --enable-avcodec --enable-avformat --enable-avfilter --enable-swscale --enable-swresample
    --enable-demuxer=mov
    --enable-protocol=file
    --enable-decoder=h264,hevc,mpeg4,mpeg2video,mjpeg,vp8,vp9,prores
    --enable-parser=h264,hevc,mpeg4video,mpeg2video,vp8,vp9,mjpeg
    --enable-bsf=h264_mp4toannexb,hevc_mp4toannexb
    --enable-filter=buffer,buffersink,palettegen,format,scale,null)

ExternalProject_Add(ffmpeg_min
    URL https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    INSTALL_DIR ${FFMPEG_PREFIX}
    CONFIGURE_COMMAND <SOURCE_DIR>/configure ${FFMPEG_CONFIGURE_FLAGS}
    BUILD_COMMAND make -j8
    INSTALL_COMMAND make install
    BUILD_BYPRODUCTS
        ${FFMPEG_PREFIX}/lib/libavfilter.a
        ${FFMPEG_PREFIX}/lib/libavformat.a
        ${FFMPEG_PREFIX}/lib/libavcodec.a
        ${FFMPEG_PREFIX}/lib/libswscale.a
        ${FFMPEG_PREFIX}/lib/libswresample.a
        ${FFMPEG_PREFIX}/lib/libavutil.a)

# The headers do not exist until ffmpeg_min builds; create the dir so the include
# path is valid at configure time.
file(MAKE_DIRECTORY ${FFMPEG_PREFIX}/include)

set(FFMPEG_INCLUDE_DIR ${FFMPEG_PREFIX}/include CACHE INTERNAL "Bundled FFmpeg include dir")
set(FFMPEG_TARGET ffmpeg_min CACHE INTERNAL "Bundled FFmpeg build target")

# Static link order matters: most-dependent first so the linker can resolve symbols.
set(FFMPEG_LIBRARIES
    ${FFMPEG_PREFIX}/lib/libavfilter.a
    ${FFMPEG_PREFIX}/lib/libavformat.a
    ${FFMPEG_PREFIX}/lib/libavcodec.a
    ${FFMPEG_PREFIX}/lib/libswscale.a
    ${FFMPEG_PREFIX}/lib/libswresample.a
    ${FFMPEG_PREFIX}/lib/libavutil.a)

if (APPLE)
    # System libraries/frameworks the static FFmpeg references.
    list(APPEND FFMPEG_LIBRARIES
        "-liconv" "-lbz2" "-lz" "-lm"
        "-framework CoreFoundation" "-framework CoreMedia" "-framework CoreVideo"
        "-framework VideoToolbox" "-framework AudioToolbox" "-framework Security")
else()
    list(APPEND FFMPEG_LIBRARIES "-lz" "-lm" "-lpthread")
endif()

set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} CACHE INTERNAL "Bundled FFmpeg link libraries")
