NAME = WAIVE-FRONT-V2

FILES_DSP = \
	src/WaiveFrontPlugin.cpp
FILES_UI = \
	src/WaiveFrontPluginUI.cpp \
	dpf-widgets/opengl/DearImGui.cpp

UI_TYPE = opengl3

# --------------------------------------------------------------
# Do some magic

# target_include_directories(${NAME} PUBLIC src)
# target_include_directories(${NAME} PUBLIC dpf/distrho/extra)
# target_include_directories(${NAME} PUBLIC dpf-widgets/opengl)
CXXFLAGS = -Isrc -Idpf/distrho/extra -Idpf-widgets/opengl

include dpf/Makefile.plugins.mk

# --------------------------------------------------------------
# Enable all possible plugin types

TARGETS = jack au

all: $(TARGETS)

# --------------------------------------------------------------

# Copy bin/$(NAME).component to ~/Library/Audio/Plug-Ins/Components/
install: $(TARGETS)
	@cp -a bin/$(NAME).component /Library/Audio/Plug-Ins/Components/

run: $(TARGETS)
	@open bin/$(NAME).app
