# DISTRHO Plugin Framework (DPF)
# Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
# Copyright (C) 2021-2022 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: ISC

cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 14)

set(NAME WAIVE-FRONT-V2)
project(${NAME})

add_subdirectory(dpf)

dpf_add_plugin(${NAME}
  TARGETS vst2 vst3 au
  FILES_DSP
      src/WaiveFrontPlugin.cpp
  FILES_UI
      src/WaiveFrontPluginUI.cpp
	  dpf-widgets/opengl/DearImGui.cpp)

target_include_directories(${NAME} PUBLIC src)
target_include_directories(${NAME} PUBLIC dpf-widgets/generic)
target_include_directories(${NAME} PUBLIC dpf-widgets/opengl)
