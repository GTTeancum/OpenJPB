XBE_TITLE = OpenJPB
OUTPUT_DIR = build/release
NXDK_DIR ?= /c/nxdk
NXDK_CXX = y
NXDK_SDL = y
NXDK_SDL_TTF = y
SHADER_OBJS = src/gpu.inl src/fragment.inl src/level.inl
include build/sources.mk
SRCS = $(wildcard src/*.c) $(wildcard src/*.cpp) $(SHARED_SRCS)
PROJECT_OBJS = $(addsuffix .obj,$(basename $(SRCS)))
$(PROJECT_OBJS): NXDK_CFLAGS += -DJPB_UFBX_AVAILABLE=1 -I$(UFBX_INCLUDE) -DJPB_STATIC_SDL_TTF=1 -I$(NXDK_DIR)/lib/sdl/SDL_ttf
$(PROJECT_OBJS): NXDK_CFLAGS += -nostdlibinc -I../include -U_WIN32 -DJPB_XBOX=1 -O2 -fcommon -Wno-error=implicit-function-declaration -Wno-error=incompatible-pointer-types -Wno-error=return-mismatch -ffunction-sections -fdata-sections
src/sdl_xboxaudio.obj: NXDK_CFLAGS += -I$(NXDK_DIR)/lib/sdl/SDL2/src -I$(NXDK_DIR)/lib/sdl/SDL2/src/audio -I$(NXDK_DIR)/lib/sdl/SDL2/src/audio/xbox
$(PROJECT_OBJS): NXDK_CXXFLAGS += -nostdlibinc -I../include -DJPB_XBOX=1 -O2 -fcommon -Wno-error=implicit-function-declaration -Wno-error=incompatible-pointer-types -Wno-error=return-mismatch -ffunction-sections -fdata-sections -std=c++17
NXDK_LDFLAGS += /opt:ref /map:build/OpenJPB.map
include $(NXDK_DIR)/Makefile

src/fragment.inl: src/gpu.ps.cg $(FP20COMPILER)
	$(CGC) -profile fp20 -o build/fragment.asm $<
	$(FP20COMPILER) build/fragment.asm > $@

# Keep shared modules in an archive so unrelated PC entry points are not
# forced into the Xbox image. nxdk libraries retain their normal link order.
SHARED_OBJS = $(addsuffix .obj,$(basename $(SHARED_SRCS)))
$(SHARED_OBJS): NXDK_CFLAGS += -Dfopen=jpb_XboxFopen
$(SHARED_OBJS): NXDK_CXXFLAGS += -Dfopen=jpb_XboxFopen
build/game.lib: $(SHARED_OBJS)

main.exe: build/game.lib
	@echo "[ LD XBOX  ] $@"
	$(VE) $(LD) $(NXDK_LDFLAGS) $(LDFLAGS) -out:'$@' $(filter-out $(SHARED_OBJS),$^) build/game.lib
