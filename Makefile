NAME = WAIVE-FRONT-V2

#--------------------------------------------------------------

FILES_DSP = \
	src/WaiveFrontPlugin.cpp
FILES_UI = \
	src/WaiveFrontPluginUI.cpp \
	src/data/DataSources.cpp \
	src/data/DataSource.cpp \
	dpf-widgets/opengl/DearImGui.cpp \
	oscpack/osc/OscReceivedElements.cpp \
	oscpack/ip/posix/UdpSocket.cpp \

	

UI_TYPE = opengl3

#--------------------------------------------------------------

# enable debugging
CXXFLAGS = -Isrc -Idpf/distrho/extra -Idpf-widgets/opengl -Ioscpack -I/opt/homebrew/include -g -O0
LDFLAGS = -L/opt/homebrew/lib -lavcodec -lavformat -lavutil -lswscale -lavfilter


#--------------------------------------------------------------

include dpf/Makefile.plugins.mk

#--------------------------------------------------------------

OS := $(shell uname)
ifeq ($(OS),Darwin)
TARGETS = jack vst vst3 au
else
TARGETS = jack vst vst3
endif

ifeq ($(OS),Darwin) # macOS
AU_DESTINATION = /Library/Audio/Plug-Ins/Components/
VST_DESTINATION = /Library/Audio/Plug-Ins/VST/
VST3_DESTINATION = /Library/Audio/Plug-Ins/VST3/
else ifeq ($(OS),Linux) # Linux
VST_DESTINATION = /usr/lib/vst/
VST3_DESTINATION = /usr/lib/vst3/
else # Windows
VST_DESTINATION = C:/Program Files/Common Files/VST2/
VST3_DESTINATION = C:/Program Files/Common Files/VST3/
endif

#--------------------------------------------------------------

all: $(TARGETS)

ifeq ($(OS),Darwin) # macOS
install: $(TARGETS)
	@cp -a bin/$(NAME).component $(AU_DESTINATION)
	@cp -a bin/$(NAME).vst $(VST_DESTINATION)
	@cp -a bin/$(NAME).vst3 $(VST3_DESTINATION)

run: $(TARGETS)
	@bin/$(NAME).app/Contents/MacOS/$(NAME)
else ifeq ($(OS),Linux) # Linux
install: $(TARGETS)
	@cp -a bin/$(NAME).so $(VST_DESTINATION)
	@cp -a bin/$(NAME).so $(VST3_DESTINATION)

run: $(TARGETS)
	@bin/$(NAME)
else # Windows
install: $(TARGETS)
	@cp -a bin/$(NAME).dll $(VST_DESTINATION)
	@cp -a bin/$(NAME).dll $(VST3_DESTINATION)

run: $(TARGETS)
	@bin/$(NAME).exe
endif
