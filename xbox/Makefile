XBE_TITLE = OpenJPB
OUTPUT_DIR = build/release
NXDK_DIR ?= /c/nxdk
NXDK_CXX = y
NXDK_SDL = y
NXDK_SDL_TTF = y
SHADER_OBJS = src/gpu.inl src/fragment.inl src/level.inl
include build/sources.mk
VIDEO_ROOT = third_party/jpb-video
VIDEO_CODEC_SRCS = \
	$(VIDEO_ROOT)/ogg/src/bitwise.c \
	$(VIDEO_ROOT)/ogg/src/framing.c \
	$(VIDEO_ROOT)/vorbis/lib/mdct.c \
	$(VIDEO_ROOT)/vorbis/lib/smallft.c \
	$(VIDEO_ROOT)/vorbis/lib/block.c \
	$(VIDEO_ROOT)/vorbis/lib/envelope.c \
	$(VIDEO_ROOT)/vorbis/lib/window.c \
	$(VIDEO_ROOT)/vorbis/lib/lsp.c \
	$(VIDEO_ROOT)/vorbis/lib/lpc.c \
	$(VIDEO_ROOT)/vorbis/lib/analysis.c \
	$(VIDEO_ROOT)/vorbis/lib/synthesis.c \
	$(VIDEO_ROOT)/vorbis/lib/psy.c \
	$(VIDEO_ROOT)/vorbis/lib/info.c \
	$(VIDEO_ROOT)/vorbis/lib/floor1.c \
	$(VIDEO_ROOT)/vorbis/lib/floor0.c \
	$(VIDEO_ROOT)/vorbis/lib/res0.c \
	$(VIDEO_ROOT)/vorbis/lib/mapping0.c \
	$(VIDEO_ROOT)/vorbis/lib/registry.c \
	$(VIDEO_ROOT)/vorbis/lib/codebook.c \
	$(VIDEO_ROOT)/vorbis/lib/sharedbook.c \
	$(VIDEO_ROOT)/vorbis/lib/lookup.c \
	$(VIDEO_ROOT)/vorbis/lib/bitrate.c \
	$(VIDEO_ROOT)/theora/lib/state.c \
	$(VIDEO_ROOT)/theora/lib/info.c \
	$(VIDEO_ROOT)/theora/lib/decinfo.c \
	$(VIDEO_ROOT)/theora/lib/decode.c \
	$(VIDEO_ROOT)/theora/lib/internal.c \
	$(VIDEO_ROOT)/theora/lib/fragment.c \
	$(VIDEO_ROOT)/theora/lib/idct.c \
	$(VIDEO_ROOT)/theora/lib/dequant.c \
	$(VIDEO_ROOT)/theora/lib/bitpack.c \
	$(VIDEO_ROOT)/theora/lib/huffdec.c \
	$(VIDEO_ROOT)/theora/lib/quant.c
SRCS = $(wildcard src/*.c) $(wildcard src/*.cpp) $(SHARED_SRCS) $(VIDEO_CODEC_SRCS)
PROJECT_OBJS = $(addsuffix .obj,$(basename $(SRCS)))
VIDEO_CODEC_OBJS = $(addsuffix .obj,$(basename $(VIDEO_CODEC_SRCS)))
$(PROJECT_OBJS): NXDK_CFLAGS += -DJPB_UFBX_AVAILABLE=1 -I$(UFBX_INCLUDE) -DJPB_STATIC_SDL_TTF=1 -I$(NXDK_DIR)/lib/sdl/SDL_ttf
$(PROJECT_OBJS): NXDK_CFLAGS += -DJPB_THEORAPLAY_CODEC_AVAILABLE=1 -I$(VIDEO_ROOT)/ogg/include -I$(VIDEO_ROOT)/vorbis/include -I$(VIDEO_ROOT)/vorbis/lib -I$(VIDEO_ROOT)/theora/include -I$(VIDEO_ROOT)/theora/lib
$(VIDEO_CODEC_OBJS): NXDK_CFLAGS += -Dalloca=__builtin_alloca
$(PROJECT_OBJS): NXDK_CFLAGS += -nostdlibinc -I../include -U_WIN32 -DJPB_XBOX=1 -O2 -fcommon -Wno-error=implicit-function-declaration -Wno-error=incompatible-pointer-types -Wno-error=return-mismatch -ffunction-sections -fdata-sections
$(VIDEO_CODEC_OBJS): NXDK_CFLAGS += -fno-strict-aliasing -fwrapv
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
