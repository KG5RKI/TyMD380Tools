// File:    md380tools/applet/src/amenu_codeplug.c
// Author:  Wolf (DL4YHF) [initial version]
//
// Date:    2017-04-29
//  Highly experimental 'alternative zone list' and similar codeplug-related displays.
//  Most list-like displays are implemented as a callback function 
//  for the 'application menu' (app_menu.c) .

#include "config.h"


#include <stm32f4xx.h>
#include <string.h>
#include "irq_handlers.h"
#include "lcd_driver.h"
#include "app_menu.h" // 'simple' alternative menu activated by red BACK-button
#include "printf.h"
#include "spiflash.h" // md380_spiflash_read()
#include "codeplug.h" // codeplug memory addresses, struct- and array-sizes
#include "usersdb.h"
#include "amenu_lastheard.h" // header for THIS module (to check prototypes,etc)
#include "gfx.h"

uint8_t lhSize = 0;
uint8_t lhRBufIndex = 0;

#define MAX_LASTHEARD_ENTRIES 10

lastheard_user lhdir[MAX_LASTHEARD_ENTRIES];


void LHList_AddEntry(uint32_t src, uint32_t dst) 
{
	if (usr_find_by_dmrid(&lhdir[lhRBufIndex].dbEntry, src) == 0) {
		return;
	}
	lastheard_user* lh = &lhdir[lhRBufIndex];
	lh->src = src;
	lh->dst = dst;
	get_RTC_time(lh->timestamp);

	lhSize++;
	lhRBufIndex++;
	if (lhRBufIndex >= MAX_LASTHEARD_ENTRIES) {

		lhRBufIndex = 0;
	}
}

  //---------------------------------------------------------------------------
static void Lastheard_OnEnter(app_menu_t *pMenu, menu_item_t *pItem)
// Called ONCE when "entering" the 'Zone List' display.
// Tries to find out how many zones exist in the codeplug,
// and the array-index of the currently active zone . 
{
	scroll_list_control_t *pSL = &pMenu->scroll_list;

	ScrollList_Init(pSL); // set all struct members to defaults
	//pSL->current_item = contactIndex; // currently active zone still unknown
	pSL->focused_item = 0;
	//contactIndex = 0;
	int i = (lhRBufIndex < lhSize  ? MAX_LASTHEARD_ENTRIES : lhRBufIndex);
	int b = 0;
	char dir = 0;
	while (((!dir && i >= lhRBufIndex) || (dir && i <lhRBufIndex)) && b<MAX_LASTHEARD_ENTRIES)
	{
		if (!dir && i >= MAX_LASTHEARD_ENTRIES) {
			i = 0;
			dir = 1;
		}
		
		++i;
		++b;
	}

	pSL->num_items = (lhRBufIndex < lhSize ? MAX_LASTHEARD_ENTRIES : lhRBufIndex);
	pSL->focused_item = (lhRBufIndex < lhSize ? (pSL->num_items-1) - lhRBufIndex : (pSL->num_items - 1));
	//ContactsList_Recache(pMenu);
	

	// Begin navigating through the list at the currently active zone:
	if (pSL->focused_item > pSL->num_items)
	{
		pSL->focused_item = 0;
	}

} // end ZoneList_OnEnter()

  //---------------------------------------------------------------------------
static void Lastheard_Draw(app_menu_t *pMenu, menu_item_t *pItem)
// Draws the 'zone list' screen. Gets visible when ENTERING that item in the app-menu.
// 
{
	int i, n_visible_items, sel_flags = SEL_FLAG_NONE;
	lcd_context_t dc;
	char cRadio; // character code for a selected or unselected "radio button"
	scroll_list_control_t *pSL = &pMenu->scroll_list;

	lastheard_user* lhUser;

	pSL->num_items = (lhRBufIndex < lhSize ? MAX_LASTHEARD_ENTRIES : lhRBufIndex);

	// Draw the COMPLETE screen, without clearing it initially to avoid flicker
	LCD_InitContext(&dc); // init context for 'full screen', no clipping
	Menu_GetColours(sel_flags, &dc.fg_color, &dc.bg_color);
	ScrollList_AutoScroll(pSL); // modify pSL->scroll_pos to make the FOCUSED item visible
	dc.font = LCD_OPT_FONT_16x16;
	//LCD_HorzLine(dc.x1, dc.y++, dc.x2, dc.bg_color);
	//LCD_HorzLine(dc.x1, dc.y++, dc.x2, dc.bg_color);
	{
		LCD_Printf(&dc, " LH  %d/%d\r", (int)((pSL->focused_item + 1)), (int)pSL->num_items);
	}
	LCD_HorzLine(dc.x1, dc.y++, dc.x2, dc.fg_color); // spacer between title and scrolling list
	LCD_HorzLine(dc.x1, dc.y++, dc.x2, dc.bg_color);
	i = pSL->scroll_pos;   // zero-based array index of the topmost VISIBLE item
	n_visible_items = 0;   // find out how many items fit on the screen

	//i = (lhSize%MAX_LASTHEARD_ENTRIES < lhRBufIndex ? lhRBufIndex : 0);
	i = lhRBufIndex - 1;
	int rBuf = lhRBufIndex-1;
	if (lhRBufIndex == 0) {
		i = MAX_LASTHEARD_ENTRIES - 1;
		rBuf = lhRBufIndex;
	}

	int b = 0;
	char dir = 0;
	while ((dc.y < (LCD_SCREEN_HEIGHT - 8)) && i < lhSize &&  (b<pSL->num_items) && b<MAX_LASTHEARD_ENTRIES)
	{
		if (i > MAX_LASTHEARD_ENTRIES) {
			i = 0;
			dir = 1;
		}
		if (i < 0 && lhSize > lhRBufIndex)
			i = MAX_LASTHEARD_ENTRIES - 1;

		lhUser = &lhdir[i];

		if (i == pSL->focused_item) // this is the CURRENTLY ACTIVE zone :
		{
			cRadio = 0x1A;  // character code for a 'selected radio button', see applet/src/font_8_8.c 
			sel_flags = SEL_FLAG_FOCUSED;
		}
		else
		{
			cRadio = 0xF9;
			sel_flags = SEL_FLAG_NONE;
		}

	
		Menu_GetColours(sel_flags, &dc.fg_color, &dc.bg_color);
		dc.x = 0;
		dc.font = LCD_OPT_FONT_8x16;
		//LCD_Printf(&dc, " ");
		//dc.font = LCD_OPT_FONT_16x16; // ex: codepage 437, but the useless smileys are radio buttons now,
									  // to imitate Tytera's zone list (at least a bit) ! 
		LCD_Printf(&dc, "%c", cRadio); // 16*16 pixels for a circle, not a crumpled egg
		dc.font = LCD_OPT_FONT_8x16;
		LCD_Printf(&dc, "%s - %d\r", lhUser->dbEntry.callsign, lhUser->dst); // '\r' clears to the end of the line, '\n' doesn't

		n_visible_items++;

		if (i == 0 && b < pSL->num_items - 1) {
			i = MAX_LASTHEARD_ENTRIES - 1;
			dir = 1;
		}

		--i;
		b++;
	}

	// If necessary, fill the rest of the screen (at the bottom) with the background colour:
	Menu_GetColours(SEL_FLAG_NONE, &dc.fg_color, &dc.bg_color);
	LCD_FillRect(0, dc.y, LCD_SCREEN_WIDTH - 1, LCD_SCREEN_HEIGHT - 1, dc.bg_color);
	pMenu->redraw = FALSE;    // "done" (screen has been redrawn).. except:
	if (n_visible_items > pSL->n_visible_items) // more items visible than initially assumed ?
	{
		pSL->n_visible_items = n_visible_items;   // adapt parameters for the scrollbar,
		pMenu->redraw = ScrollList_AutoScroll(pSL); // and redraw everything (KISS)
	}
} // ZoneList_Draw()


  //---------------------------------------------------------------------------
int am_cbk_LastheardList(app_menu_t *pMenu, menu_item_t *pItem, int event, int param)
// Callback function, invoked from the "app menu" framework for the 'ZONES' list.
// (lists ALL ZONES, not THE CHANNELS in a zone)
{
	scroll_list_control_t *pSL = &pMenu->scroll_list;

	// what happened, why did the menu framework call us ?
	if (event == APPMENU_EVT_ENTER) // pressed ENTER (to enter the 'Zone List') ?
	{
		Lastheard_OnEnter(pMenu, pItem);
		return AM_RESULT_OCCUPY_SCREEN; // occupy the entire screen (not just a single line)
	}
	else if (event == APPMENU_EVT_PAINT) // someone wants us to paint into the framebuffer
	{ // To minimize QRM from the display cable, only redraw when necessary (no "dynamic" content here):
		if (pMenu->visible == APPMENU_USERSCREEN_VISIBLE) // only if HexMon already 'occupied' the screen !
		{
			if (pMenu->redraw)
			{
				pMenu->redraw = FALSE;   // don't modify this sequence
				Lastheard_Draw(pMenu, pItem); // <- may decide to draw AGAIN (scroll)
			}
			return AM_RESULT_OCCUPY_SCREEN; // keep the screen 'occupied' 
		}
	}
	else if (event == APPMENU_EVT_KEY) // some other key pressed while focused..
	{
		switch ((char)param) // here: message parameter = keyboard code (ASCII)
		{
		case 'M':  // green "Menu" key : kind of ENTER. But here, "apply & return" .
			if (pSL->focused_item >= 0)
			{
				//ContactList_SetZoneByIndex(pSL->focused_item);
				//contactIndex = pSL->focused_item;
				//fIsTG = (selContact.type == 0xC1 ? 1 : 0);
				//Menu_EnterSubmenu(pMenu, am_Contact_Edit);

				// The above command switched to the new zone, and probably set
				// channel_num = 0 to 'politely ask' the original firmware to 
				// reload whever is necessary from the codeplug (SPI-Flash). 
				// It's unknown when exactly that happens (possibly in another task). 
				// To update the CHANNEL NAME from the *new* zone in our menu, 
				// let a few hundred milliseconds pass before redrawing the screen:
				StartStopwatch(&pMenu->stopwatch_late_redraw);
			}
			return AM_RESULT_EXIT_AND_RELEASE_SCREEN;
		case 'B':  // red "Back"-key : return from this screen, discard changes.
			return AM_RESULT_EXIT_AND_RELEASE_SCREEN;
		case 'U':  // cursor UP
			if (pSL->focused_item < (pSL->num_items - 1))
			{
				++pSL->focused_item;
			}
			else if(pSL->focused_item >= (pSL->num_items - 1)) {
				pSL->focused_item = 0;
				
			}
			//contactIndex = pSL->focused_item;
			//ContactsList_Recache(pMenu);
			break;
		case 'D':  // cursor DOWN
			if (pSL->focused_item > 0 )
			{
				--pSL->focused_item;
			}
			else {
				pSL->focused_item = pSL->num_items - 1;
			}
			//contactIndex = pSL->focused_item;
			//ContactsList_Recache(pMenu);
			break;
		default:    // Other keys .. editing or treat as a hotkey ?
			break;
		} // end switch < key >
		pMenu->redraw = TRUE;

	} // end if < keyboard event >
	if (pMenu->visible == APPMENU_USERSCREEN_VISIBLE) // only if HexMon already 'occupied' the screen !
	{
		return AM_RESULT_OCCUPY_SCREEN; // keep the screen 'occupied' 
	}
	else
	{
		return AM_RESULT_NONE; // "proceed as if there was NO callback function"
	}
} // end am_cbk_ZoneList()


