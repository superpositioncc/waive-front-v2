NAME = WAIVE-FRONT-V2

#--------------------------------------------------------------

FILES_DSP = \
	src/WaiveFrontPlugin.cpp
FILES_UI = \
	src/WaiveFrontPluginUI.cpp \
	src/data/DataSources.cpp \
	src/data/DataSource.cpp \
	dpf-widgets/opengl/DearImGui.cpp
	

UI_TYPE = opengl3

#--------------------------------------------------------------

CXXFLAGS = -Isrc -Idpf/distrho/extra -Idpf-widgets/opengl -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lavcodec -lavformat -lavutil -lswscale


#--------------------------------------------------------------

include dpf/Makefile.plugins.mk

#--------------------------------------------------------------

TARGETS = jack au

#--------------------------------------------------------------

all: $(TARGETS)

install: $(TARGETS)
	@cp -a bin/$(NAME).component /Library/Audio/Plug-Ins/Components/

run: $(TARGETS)
	@bin/$(NAME).app/Contents/MacOS/$(NAME)
