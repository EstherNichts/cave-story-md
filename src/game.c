#include "gamemode.h"

#include <genesis.h>
#include "resources.h"
#include "input.h"
#include "system.h"
#include "player.h"
#include "stage.h"
#include "camera.h"
#include "tables.h"
#include "tsc.h"
#include "vdp_ext.h"
#include "effect.h"
#include "hud.h"
#include "weapon.h"
#include "window.h"
#include "audio.h"
#include "ai.h"

Sprite *itemSprite[MAX_ITEMS];
u8 selectedItem = 0;
//Sprite *selectSprite;

void itemcursor_move(u8 oldindex, u8 index) {
	// Erase old position
	u16 x = 3 + (oldindex % 8) * 4;
	u16 y = 11 + (oldindex / 8) * 2;
	VDP_setTileMapXY(PLAN_WINDOW, TILE_FONTINDEX, x,   y);
	VDP_setTileMapXY(PLAN_WINDOW, TILE_FONTINDEX, x+3, y);
	VDP_setTileMapXY(PLAN_WINDOW, TILE_FONTINDEX, x,   y+1);
	VDP_setTileMapXY(PLAN_WINDOW, TILE_FONTINDEX, x+3, y+1);
	// Draw new position
	x = 3 + (index % 8) * 4;
	y = 11 + (index / 8) * 2;
	VDP_setTileMapXY(PLAN_WINDOW, TILE_FACEINDEX,   x,   y);
	VDP_setTileMapXY(PLAN_WINDOW, TILE_FACEINDEX+1, x+3, y);
	VDP_setTileMapXY(PLAN_WINDOW, TILE_FACEINDEX+2, x,   y+1);
	VDP_setTileMapXY(PLAN_WINDOW, TILE_FACEINDEX+3, x+3, y+1);
}

bool update_pause() {
	if(joy_pressed(BUTTON_START)) {
		// Unload graphics
		for(u16 i = 0; i < MAX_ITEMS; i++) {
			SPR_SAFERELEASE(itemSprite[i]);
		}
		selectedItem = 0;
		//SPR_SAFERELEASE(selectSprite);
		// Reload TSC Events for the current stage
		tsc_load_stage(stageID);
		// Put the sprites for player/entities/HUD back
		player_unpause();
		player_unlock_controls();
		entities_unpause();
		hud_show();
		VDP_setWindowPos(0, 0);
		return false;
	} else {
		// Weapons are 1000 + ID
		// Items are 5000 + ID
		// Item descriptions are 6000 + ID
		if(tsc_running()) {
			tsc_update();
		} else if(joy_pressed(BUTTON_C) && playerInventory[selectedItem] > 0) {
			tsc_call_event(6000 + playerInventory[selectedItem]);
		} else if(joy_pressed(BUTTON_LEFT)) {
			itemcursor_move(selectedItem, (selectedItem - 1) % 32);
			sound_play(SND_MENU_MOVE, 5);
			selectedItem--;
			selectedItem %= 32;
			tsc_call_event(5000 + playerInventory[selectedItem]);
		} else if(joy_pressed(BUTTON_UP)) {
			itemcursor_move(selectedItem, selectedItem < 8 ? selectedItem + 24 : selectedItem - 8);
			sound_play(SND_MENU_MOVE, 5);
			selectedItem = selectedItem < 8 ? selectedItem + 24 : selectedItem - 8;
			selectedItem %= 32;
			tsc_call_event(5000 + playerInventory[selectedItem]);
		} else if(joy_pressed(BUTTON_RIGHT)) {
			itemcursor_move(selectedItem, (selectedItem + 1) % 32);
			sound_play(SND_MENU_MOVE, 5);
			selectedItem++;
			selectedItem %= 32;
			tsc_call_event(5000 + playerInventory[selectedItem]);
		} else if(joy_pressed(BUTTON_DOWN)) {
			itemcursor_move(selectedItem, (selectedItem + 8) % 32);
			sound_play(SND_MENU_MOVE, 5);
			selectedItem += 8;
			selectedItem %= 32;
			tsc_call_event(5000 + playerInventory[selectedItem]);
		} 
	}
	return true;
}

void game_reset(bool load) {
	camera_init();
	tsc_init();
	gameFrozen = false;
	if(load) {
		system_load();
	} else {
		system_new();
		tsc_call_event(GAME_START_EVENT);
	}
	// Load up the main palette
	VDP_setCachedPalette(PAL0, PAL_Main.data);
	VDP_setCachedPalette(PAL1, PAL_Sym.data);
	VDP_setPaletteColors(0, PAL_FadeOut, 64);
	//VDP_setPaletteColors(0, VDP_getCachedPalette(), 64);
}

void draw_itemmenu() {
	SYS_disableInts();
	VDP_fillTileMap(VDP_PLAN_WINDOW, TILE_FONTINDEX, 0, 64 * 20);
	VDP_loadTileSet(&TS_ItemSel, TILE_FACEINDEX, true);
	//window_draw_area(2, 1, 36, 18);
	window_clear();
	VDP_drawTextWindow("--ARMS--", 16, 3);
	for(u16 i = 0; i < MAX_ITEMS; i++) {
		u16 item = playerInventory[i];
		if(item > 0) {
			// Wonky workaround to use either PAL_Sym or PAL_Main
			const SpriteDefinition *sprDef = &SPR_ItemImage;
			u16 pal = PAL1;
			if(ITEM_PAL[item]) {
				sprDef = &SPR_ItemImageG;
				pal = PAL0;
			}
			itemSprite[i] = SPR_addSprite(sprDef, 
				24 + (i % 8) * 32, 88 + (i / 8) * 16, TILE_ATTR(pal, 1, 0, 0));
			SPR_SAFEANIM(itemSprite[i], item);
		} else {
			itemSprite[i] = NULL;
		}
	}
	VDP_drawTextWindow("--ITEM--", 16, 10);
	itemcursor_move(0, 0);
	player_pause();
	entities_pause();
	hud_hide();
	VDP_setWindowPos(0, 28);
	SYS_enableInts();
}

void vblank() {
	stage_update();
	if(hudRedrawPending) hud_update_vblank();
	if(water_screenlevel != WATER_DISABLE) vblank_water();
	//if(debuggingEnabled) {
	//	char str[34];
	//	sprintf(str, "%05u %05u E#:%03hu/%03hu MEM:%05hu", playerProf, entityProf, 
	//		entities_count_active(), entities_count(), MEM_getFree());
	//	VDP_drawTextWindow(str, 1, 27);
	//}
}

u8 game_main(bool load) {
	// If player chooses continue with no save, start a new game
	if(load && !system_checkdata()) {
		load = false;
	}
	SYS_disableInts();
	VDP_setEnable(false);
	
	VDP_resetScreen();
	// This is the SGDK font with a blue background for the message window
	VDP_loadTileSet(&TS_MsgFont, TILE_FONTINDEX, true);
	SYS_setVIntCallback(vblank);
	SYS_setHIntCallback(hblank_water);
	// A couple backgrounds (clouds) use tile scrolling
	VDP_setScrollingMode(HSCROLL_TILE, VSCROLL_PLANE);
	effects_init();
	game_reset(load);
	
	VDP_setEnable(true);
	VDP_setWindowPos(0, 0);
	SYS_enableInts();
	
	if(load) {
		hud_show();
		VDP_fadeTo(0, 63, VDP_getCachedPalette(), 20, true);
	}
	
	paused = false;
	u8 ending = 0;
	
	while(true) {
		input_update();
		if(paused) {
			paused = update_pause();
		} else {
			if(!tsc_running() && joy_pressed(BUTTON_START)) {
				draw_itemmenu();
				tsc_load_stage(255);
				paused = true;
			} else {
				// Don't update this stuff if script is using <PRI
				if(!gameFrozen) {
					camera_update();
					player_update();
					entities_update();
					if(showingBossHealth) tsc_update_boss_health();
				}
				hud_update();
				u8 rtn = tsc_update();
				if(rtn > 0) {
					if(rtn == 1) {
						ending = 0; // No ending, return to title
						break;
					} else if(rtn == 2) {
						game_reset(true); // Reload save
						hud_show();
						VDP_fadeTo(0, 63, VDP_getCachedPalette(), 20, true);
						continue;
					} else if(rtn == 3) {
						game_reset(false); // Start from beginning
						continue;
					} else if(rtn == 4) {
						ending = 1; // Normal ending
						break;
					} else if(rtn == 5) {
						ending = 2; // Good ending
						break;
					}
				}
				effects_update();
				system_update();
			}
		}
		SPR_update();
		VDP_waitVSync();
	}
	
	if(ending) { // You are a winner
		
	} else { // Going back to title screen
		// Sometimes after restart stage will not load any tileset, instead
		// showing the Cave Story title. Unset to prevent that
		stageID = 0;
		stageTileset = 0;
		stageBackground = 0;
		// Title screen uses built in font not blue background font
		SYS_disableInts();
		VDP_loadFont(&font_lib, 0);
		SYS_setVIntCallback(NULL);
		SYS_setHIntCallback(NULL);
		SYS_enableInts();
	}
	return ending;
}
