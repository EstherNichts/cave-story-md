#include "common.h"

#include "dma.h"
#include "memory.h"
#include "resources.h"
#include "tools.h"
#include "vdp.h"
#include "vdp_pal.h"
#include "vdp_tile.h"

#include "kanji.h"

// Allocate kanji tiles to:
// TILE_FONTINDEX (96 tiles)
// Unused end of window plane mapping (28 tiles)
// TILE_NUMBERINDEX (16 tiles)

const uint8_t tileorder[] = {
	15, 31, 14, 30, 13, 29, 12, 28, 11, 27, 10, 26, 9, 25, 8, 24,
	7, 23, 6, 22, 5, 21, 4, 20, 3, 19, 2, 18, 1, 17, 0, 16
};

void kanji_draw(VDPPlan plan, uint16_t vramIndex, uint16_t chr, uint16_t x, uint16_t y, uint16_t backCol) {
	// Read the 1bpp bitmap to create a glyph, note bitmaps are bottom to top
	const uint8_t *bmp;
	if(chr > 0xFF) {
		chr -= 0x100;
		bmp = BMP_Kanji + sizeof(BMP_Kanji) - (chr * 32);
	} else {
		chr -= 0x20;
		bmp = BMP_Ascii + sizeof(BMP_Ascii) - (chr * 32);
	}
	uint32_t tiles[32] = {};
	for(uint16_t i = 0; i < 32; i++) {
		uint32_t row = *--bmp;
		for(uint16_t col = 0; col < 8; col++) {
			uint32_t color = (row & (1 << col)) ? 15 : backCol;
			tiles[tileorder[i]] = color << (col*4);
		}
	}
	if(vramIndex >= 0xB000 >> 5 && vramIndex < 0xC000 >> 5) {
		// Between gaps in the window mapping, can't load in sequence
		VDP_loadTileData(tiles, vramIndex,   1, TRUE);
		VDP_loadTileData(tiles, vramIndex+4, 1, TRUE);
		VDP_loadTileData(tiles, vramIndex+8, 1, TRUE);
		VDP_loadTileData(tiles, vramIndex+12,1, TRUE);
		VDP_setTileMapXY(plan, TILE_ATTR_FULL(PAL0,1,0,0,vramIndex),   x,  y);
		VDP_setTileMapXY(plan, TILE_ATTR_FULL(PAL0,1,0,0,vramIndex+4), x+1,y);
		VDP_setTileMapXY(plan, TILE_ATTR_FULL(PAL0,1,0,0,vramIndex+8), x,  y+1);
		VDP_setTileMapXY(plan, TILE_ATTR_FULL(PAL0,1,0,0,vramIndex+12),x+1,y+1);
	} else {
		VDP_loadTileData(tiles, vramIndex, 4, TRUE);
		VDP_fillTileMapRectInc(plan, TILE_ATTR_FULL(PAL0,1,0,0,vramIndex), x, y, 2, 2);
	}
}
