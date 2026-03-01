# --- Toolchain ---
Z88DK ?= $(HOME)/z88dk
ZCC ?= $(Z88DK)/bin/zcc
ZCCCFG ?= $(Z88DK)/lib/config
HOSTCC ?= cc
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
FUSE ?= open -a Fuse
FUSE_RUN = $(FUSE) viewer.tap
else
FUSE ?= fuse-sdl
FUSE_RUN = $(FUSE) viewer.tap &
endif

# --- Config ---
CONFIG_MK ?= config/basic_config.mk
include $(CONFIG_MK)

CFLAGS=+zx -vn -SO3 -zorg=32768 -startup=31 --opt-code-speed -compiler=sdcc -mz80 \
       --reserve-regs-iy --allow-unsafe-read -Cc--max-allocs-per-node=50000
USER_CFLAGS ?=
LDFLAGS=-lm -create-app

# --- Top-level targets ---
all: viewer.tap

.PHONY: all run clean

run: viewer.tap
	$(FUSE_RUN)

# --- Host tools ---
generate_tiles: generate_tiles.c
	$(HOSTCC) -O2 -o $@ $<

generate_map: generate_map.c
	$(HOSTCC) -O2 -o $@ $<

# --- Asset generation ---
tiles_data.asm: $(CONFIG_MK) $(TILES_ZXP) generate_tiles
	./generate_tiles $(TILES_ZXP) tiles_data.asm $(TILE_WIDTH_PX) $(TILE_HEIGHT_PX)

tiles_data.h: $(CONFIG_MK) $(TILES_ZXP) generate_tiles
	./generate_tiles $(TILES_ZXP) tiles_data.h $(TILE_WIDTH_PX) $(TILE_HEIGHT_PX)

map.bin: $(CONFIG_MK) $(MAP_CSV) generate_map
	./generate_map $(MAP_CSV) $(MAP_WIDTH_TILES) $(MAP_HEIGHT_TILES)

map_data.h: map.bin
	xxd -i map.bin > map_data.h

# --- Binary assembly ---
tiles_data.bin: tiles_data.asm
	$(Z88DK)/bin/z88dk-z80asm -b tiles_data.asm

contended_data.bin: tiles_data.bin map.bin
	cat tiles_data.bin map.bin > contended_data.bin

# --- Compile, link & package ---
viewer.tap: viewer.c assets/dodecahedron.h
	PATH=$(Z88DK)/bin:$$PATH Z88DK=$(Z88DK) ZCCCFG=$(ZCCCFG) $(ZCC) $(CFLAGS) $(USER_CFLAGS) -o viewer viewer.c $(LDFLAGS)

# --- Clean ---
clean:
	rm -f viewer viewer.tap viewer_CODE.bin viewer_data_user.bin viewer_code.tap contended_data.tap loader.tap tiles_data.bin contended_data.bin tiles_data.o *.o *.map map.bin map_data.h tiles_data.asm tiles_data.h generate_tiles generate_map
