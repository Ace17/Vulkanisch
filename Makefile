all: true_all


BIN?=bin

TARGETS+=$(BIN)/vulkanisch.exe

SRCS:=\
	src/common/main.cpp\
	src/common/util.cpp\
	src/common/vkutil.cpp\
	src/common/linalg.cpp\
	glad/src/vulkan.c\

CXXFLAGS+=-Wall -Wextra -Werror -std=c++14
CXXFLAGS+=-Isrc
CXXFLAGS+=-Iglad/include

CXXFLAGS+=$(shell pkg-config sdl2 --cflags)
LDFLAGS+=$(shell pkg-config sdl2 --libs)

SHADERS:=

GetMyDir=$(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
SUBS=$(wildcard src/*/*.mk)
include $(SUBS)

TARGETS+=$(SHADERS:%.glsl=$(BIN)/%.spv)

true_all: $(TARGETS)

$(BIN)/vulkanisch.exe: $(SRCS:%=$(BIN)/%.o)

#------------------------------------------------------------------------------
# Generic rules
#------------------------------------------------------------------------------

include $(shell test -d $(BIN) && find $(BIN) -name "*.dep")

$(BIN)/%.exe:
	@mkdir -p $(dir $@)
	$(CXX) -o "$@" $^ $(LDFLAGS)

$(BIN)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -MMD -MT "$@" -MF "$@.dep" -o "$@" $<

$(BIN)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o "$@" $<

$(BIN)/%.vert.spv: %.vert.glsl
	@mkdir -p $(dir $@)
	glslangValidator -V -o "$@" -S vert "$<" --quiet

$(BIN)/%.frag.spv: %.frag.glsl
	@mkdir -p $(dir $@)
	glslangValidator -V -o "$@" -S frag "$<" --quiet

clean:
	rm -rf $(BIN)

