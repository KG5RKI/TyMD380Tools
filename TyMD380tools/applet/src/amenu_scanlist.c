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
#include "amenu_scanlist.h" // header for THIS module (to check prototypes,etc)
#include "amenu_set_tg.h"
#include "amenu_channels.h"
#include "syslog.h"

scanlist_t selScanList;
int selScanListIndex = 0;
int selScanListChan = 0;

void am_cbk_DeleteFromScanList(app_menu_t *pMenu, menu_item_t *pItem, int event, int param);
void am_cbk_SetPriorityCh1(app_menu_t *pMenu, menu_item_t *pItem, int event, int param);
void am_cbk_SetPriorityCh2(app_menu_t *pMenu, menu_item_t *pItem, int event, int param);
void am_cbk_DeleteScanList(app_menu_t *pMenu, menu_item_t *pItem, int event, int param);

const menu_item_t am_ScanList_Manage[] = { { "[-]Remove", DTYPE_NONE, APPMENU_OPT_BACK, 0, NULL, 0, 0,  NULL, am_cbk_DeleteFromScanList },
										 { "Priority 1", DTYPE_NONE, APPMENU_OPT_BACK, 0, NULL, 0, 0,  NULL, am_cbk_SetPriorityCh1 },
										 { "Priority 2", DTYPE_NONE, APPMENU_OPT_BACK, 0, NULL, 0, 0,  NULL, am_cbk_SetPriorityCh2 },

										{ "Back ", DTYPE_NONE, APPMENU_OPT_BACK, 0, NULL, 0, 0, NULL, NULL },
										// End of the list marked by "all zeroes" :
										{ NULL, 0/*dt*/, 0/*opt*/, 0/*ov*/, NULL/*pValue*/, 0,0, NULL, NULL } };

//---------------------------------------------------------------------------
BOOL ScanList_ReadByIndex(int index,             // [in] zero-based zone index
	scanlist_t *tList) // [out] zone name as a C string
						 // Return value : TRUE when successfully read a zone name for this index,
						 //                FALSE when failed or not implemented for a certain firmware.
{
	if (index >= 0 && index < CODEPLUG_MAX_SCANLIST)
	{
		md380_spiflash_read(tList, (index * CODEPLUG_SIZEOF_SCANLIST_ENTRY)
			+ CODEPLUG_SPIFLASH_ADDR_SCANLIST,
			sizeof(scanlist_t));
		
		tList->name[15] = '\0';
		return tList->name[0] != '\0';
	}
	return 0;
} // end ScanList_ReadNameByIndex()

BOOL ScanList_WriteByIndex(int index,             // [in] zero-based zone index
	scanlist_t *tList) // [out] zone name as a C string
					   // Return value : TRUE when successfully read a zone name for this index,
					   //                FALSE when failed or not implemented for a certain firmware.
{
	if (index >= 0 && index < CODEPLUG_MAX_SCANLIST)
	{
		md380_spiflash_write(tList, (index * CODEPLUG_SIZEOF_SCANLIST_ENTRY)
			+ CODEPLUG_SPIFLASH_ADDR_SCANLIST, sizeof(scanlist_t));
		
		return TRUE;
	}
	return FALSE;
} // end ScanList_WriteByIndex()


void am_cbk_SaveScanList(app_menu_t *pMenu, menu_item_t *pItem, int event, int param)
{ // Simple example for a 'user screen' opened from the application menu
	if (event == APPMENU_EVT_ENTER) // pressed ENTER (to launch the colour test) ?
	{
		md380_spiflash_write(&selScanList, (selScanListIndex * CODEPLUG_SIZEOF_SCANLIST_ENTRY)
			+ CODEPLUG_SPIFLASH_ADDR_SCANLIST, sizeof(scanlist_t));
		return AM_RESULT_EXIT_AND_RELEASE_SCREEN; // screen now 'occupied' by the colour test screen
	}
	return AM_RESULT_NONE; // "proceed as if there was NO callback function"
} // end am_cbk_SaveScanList()

void am_cbk_DeleteFromScanList(app_menu_t *pMenu, menu_item_t *pItem, int event, int param)
{ // Simple example for a 'user screen' opened from the application menu
	if (event == APPMENU_EVT_ENTER) // pressed ENTER (to launch the colour test) ?
	{
		if (selScanListChan < CODEPLUG_MAX_SCANLIST_CHANNELS - 1) {
			int b = selScanListChan;
			for (int i = selScanListChan + 1; i < CODEPLUG_MAX_SCANLIST_CHANNELS; i++) {
				selScanList.channels[b++] = selScanList.channels[i];
				if (!selScanList.channels[i] || i == CODEPLUG_MAX_SCANLIST_CHANNELS - 1) {
					selScanList.channels[i] = 0;
					break;
				}
			}
		}
		else {
			selScanList.channels[selScanListChan] = 0;
		}
		ScanList_WriteByIndex(selScanListIndex, &selScanList);
		return AM_RESULT_EXIT_AND_RELEASE_SCREEN; // screen now 'occupied' by the colour test screen
	}
	return AM_RESULT_NONE; // "proceed as if there was NO callback function"
} // end am_cbk_SaveScanList()

void am_cbk_DeleteScanList(app_menu_t *pMenu, menu_item_t *pItem, int event, int param)
{ // Simple example for a 'user screen' opened from the application menu
	if (event == APPMENU_EVT_ENTER) // pressed ENTER (to launch the colour test) ?
	{
		scanlist_t tempList;
		ScanList_ReadByIndex(selScanListIndex, &tempList);
		if (selScanListIndex < CODEPLUG_MAX_SCANLIST - 1) {
			
			int b = selScanListIndex;
			for (int i = selScanListIndex + 1; i < CODEPLUG_MAX_SCANLIST; i++) {
				ScanList_ReadByIndex(i, &tempList);
				ScanList_WriteByIndex(b++, &tempList);
				if (tempList.name[0] == '\0'  || i == CODEPLUG_MAX_SCANLIST - 1) {
					memset(tempList.name, 0, sizeof(tempList.name));
					ScanList_WriteByIndex(i, &tempList);
					break;
				}
			}
		}
		else {
			memset(tempList.name, 0, sizeof(tempList.name));
			ScanList_WriteByIndex(selScanListIndex, &tempList);
		}
		return AM_RESULT_EXIT_AND_RELEASE_SCREEN; // screen now 'occupied' by the colour test screen
	}
	return AM_RESULT_NONE; // "proceed as if there was NO callback function"
} // end am_cbk_SaveScanList()

void am_cbk_SetPriorityCh1(app_menu_t *pMenu, menu_item_t *pItem, int event, int param)
{ // Simple example for a 'user screen' opened from the application menu
	if (event == APPMENU_EVT_ENTER) // pressed ENTER (to launch the colour test) ?
	{
		selScanList.primary = selScanListIndex;
		ScanList_WriteByIndex(selScanListIndex, &selScanList);
		return AM_RESULT_EXIT_AND_RELEASE_SCREEN; // screen now 'occupied' by the colour test screen
	}
	return AM_RESULT_NONE; // "proceed as if there was NO callback function"
} // end am_cbk_SetPriorityCh1()

void am_cbk_SetPriorityCh2(app_menu_t *pMenu, menu_item_t *pItem, int event, int param)
{ // Simple example for a 'user screen' opened from the application menu
	if (event == APPMENU_EVT_ENTER) // pressed ENTER (to launch the colour test) ?
	{
		selScanList.secondary = selScanListIndex;
		ScanList_WriteByIndex(selScanListIndex, &selScanList);
		return AM_RESULT_EXIT_AND_RELEASE_SCREEN; // screen now 'occupied' by the colour test screen
	}
	return AM_RESULT_NONE; // "proceed as if there was NO callback function"
} // end am_cbk_SetPriorityCh2()


  //---------------------------------------------------------------------------
static void ScanList_OnEnter(app_menu_t *pMenu, menu_item_t *pItem)
// Called ONCE when "entering" the 'Zone List' display.
// Tries to find out how many zones exist in the codeplug,
// and the array-index of the currently active zone . 
{
	scanlist_t tCont;
	scroll_list_control_t *pSL = &pMenu->scroll_list;

	ScrollList_Init(pSL); // set all struct members to defaults
	//pSL->current_item = contactIndex; // currently active zone still unknown
	pSL->focused_item = 0;
	//contactIndex = 0;
	int start = 0;
	int i = start, b = 0;
	
	while (i < CODEPLUG_MAX_SCANLIST)
	{
		scanlist_t* pCont = &tCont;
		if (!ScanList_ReadByIndex(i, pCont)) // guess all zones are through
		{
			break;
		}
		++i;
		++b;
	}

	pSL->num_items = i+1;

	// Begin navigating through the list at the currently active zone:
	if (pSL->focused_item > pSL->num_items)
	{
		pSL->focused_item = 0;
	}

} // end ScanList_OnEnter()

  //---------------------------------------------------------------------------
static void ScanList_Draw(app_menu_t *pMenu, menu_item_t *pItem)
// Draws the 'zone list' screen. Gets visible when ENTERING that item in the app-menu.
// 
{
	int i, n_visible_items, sel_flags = SEL_FLAG_NONE;
	lcd_context_t dc;
	char cRadio; // character code for a selected or unselected "radio button"
	scroll_list_control_t *pSL = &pMenu->scroll_list;

	scanlist_t tCont;

	// Draw the COMPLETE screen, without clearing it initially to avoid flicker
	LCD_InitContext(&dc); // init context for 'full screen', no clipping
	Menu_GetColours(sel_flags, &dc.fg_color, &dc.bg_color);
	ScrollList_AutoScroll(pSL); // modify pSL->scroll_pos to make the FOCUSED item visible
	dc.font = LCD_OPT_FONT_16x16;
	{
		LCD_Printf(&dc, "Scan List\r");
	}
	LCD_HorzLine(dc.x1, dc.y++, dc.x2, dc.fg_color); // spacer between title and scrolling list
	LCD_HorzLine(dc.x1, dc.y++, dc.x2, dc.bg_color);
	i = pSL->scroll_pos;   // zero-based array index of the topmost VISIBLE item
	n_visible_items = 0;   // find out how many items fit on the screen

	{
		Menu_GetColours((pSL->focused_item==0 ? SEL_FLAG_FOCUSED : SEL_FLAG_NONE), &dc.fg_color, &dc.bg_color);
		dc.font = LCD_OPT_FONT_8x16;
		LCD_Printf(&dc, "  Create New\r");
		LCD_HorzLine(dc.x1, dc.y++, dc.x2, dc.fg_color); // spacer between title and scrolling list
		LCD_HorzLine(dc.x1, dc.y++, dc.x2, dc.bg_color);
		i++;
		n_visible_items++;
	}

	while ((dc.y < (LCD_SCREEN_HEIGHT - 8)) && (i<pSL->num_items))
	{
		scanlist_t* pCont = &tCont;
		if (!ScanList_ReadByIndex(i-1, pCont)) // oops.. shouldn't have reached the last item yet !
		{
			break;
		}
		
		//curCon = &sz20[i];

		if (i == pSL->focused_item) // this is the CURRENTLY ACTIVE zone :
		{
			cRadio = 0x1A;  // character code for a 'selected radio button', see applet/src/font_8_8.c 
		}
		else
		{
			//if (pCont->type == 0xC1) {
			//	cRadio = 0xF9;
			//}
			//else 
			{
				cRadio = 0xF9;
			}
		}

		

		if (i == pSL->focused_item)
		{
			sel_flags = SEL_FLAG_FOCUSED;
		}
		//else if (i == pSL->current_item) // this is the CURRENTLY ACTIVE zone :
		//{
		//	sel_flags = SEL_FLAG_CURRENT;  // additional highlighting (besides the selected button)
		//}
		else
		{
			sel_flags = SEL_FLAG_NONE;
		}
		Menu_GetColours(sel_flags, &dc.fg_color, &dc.bg_color);
		dc.x = 0;
		dc.font = LCD_OPT_FONT_8x16;
		LCD_Printf(&dc, " ");
		dc.font = LCD_OPT_FONT_16x16; // ex: codepage 437, but the useless smileys are radio buttons now,
									  // to imitate Tytera's zone list (at least a bit) ! 
		LCD_Printf(&dc, "%c", cRadio); // 16*16 pixels for a circle, not a crumpled egg
		dc.font = LCD_OPT_FONT_8x16;
		LCD_Printf(&dc, " %S\r", (wchar_t*)pCont->name); // '\r' clears to the end of the line, '\n' doesn't
		i++;
		n_visible_items++;
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
} // ScanList_Draw()

void ChannelListView_Draw(app_menu_t *pMenu, menu_item_t *pItem)
// Draws the 'zone list' screen. Gets visible when ENTERING that item in the app-menu.
// 
{
	int i, n_visible_items, sel_flags = SEL_FLAG_NONE;
	lcd_context_t dc;
	char cRadio; // character code for a selected or unselected "radio button"
	scroll_list_control_t *pSL = &pMenu->scroll_list;

	channel_t tCont;

	// Draw the COMPLETE screen, without clearing it initially to avoid flicker
	LCD_InitContext(&dc); // init context for 'full screen', no clipping
	Menu_GetColours(sel_flags, &dc.fg_color, &dc.bg_color);
	ScrollList_AutoScroll(pSL); // modify pSL->scroll_pos to make the FOCUSED item visible
	dc.font = LCD_OPT_FONT_12x12;
	{
		LCD_Printf(&dc, " Edit %d/%d\r", (int)(pSL->focused_item + 1), (int)pSL->num_items);
	}
	LCD_HorzLine(dc.x1, dc.y++, dc.x2, dc.fg_color); // spacer between title and scrolling list
	LCD_HorzLine(dc.x1, dc.y++, dc.x2, dc.bg_color);
	i = pSL->scroll_pos;   // zero-based array index of the topmost VISIBLE item
	n_visible_items = 0;   // find out how many items fit on the screen
	while ((dc.y < (LCD_SCREEN_HEIGHT - 8)) && (i<pSL->num_items))
	{
		channel_t* pCont = &tCont;
		
		if (!ChannelList_ReadNameByIndex(selScanList.channels[i]-1, pCont)) // oops.. shouldn't have reached the last item yet !
		{
			break;
		}

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
		LCD_Printf(&dc, " ");
		dc.font = LCD_OPT_FONT_16x16; // ex: codepage 437, but the useless smileys are radio buttons now,
									  // to imitate Tytera's zone list (at least a bit) ! 
		LCD_Printf(&dc, "%c", cRadio); // 16*16 pixels for a circle, not a crumpled egg
		dc.font = LCD_OPT_FONT_8x16;
		LCD_Printf(&dc, " %S\r", (wchar_t*)pCont->name); // '\r' clears to the end of the line, '\n' doesn't
		i++;
		n_visible_items++;
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
} // ChannelListView_Draw()


void am_cbk_ChannelListView_OnEnter(app_menu_t *pMenu, menu_item_t *pItem)
// Called ONCE when "entering" the 'Zone List' display.
// Tries to find out how many zones exist in the codeplug,
// and the array-index of the currently active zone . 
{
	channel_t tChan;
	scroll_list_control_t *pSL = &pMenu->scroll_list;

	ScrollList_Init(pSL); // set all struct members to defaults
						  //pSL->current_item = contactIndex; // currently active zone still unknown
	pSL->focused_item = 0;
	//contactIndex = 0;
	int start = 0;

	int i = 0;
	for (; i < CODEPLUG_MAX_SCANLIST_CHANNELS; i++)
	{
		if (!selScanList.channels[i]) {
			break;
		}
	}
	pSL->num_items = i;

	// Begin navigating through the list at the currently active zone:
	if (pSL->focused_item > pSL->num_items)
	{
		pSL->focused_item = 0;
	}
} // end am_cbk_ChannelListView_OnEnter()


int am_cbk_ChannelListView(app_menu_t *pMenu, menu_item_t *pItem, int event, int param)
{
	scroll_list_control_t *pSL = &pMenu->scroll_list;

	// what happened, why did the menu framework call us ?
	if (event == APPMENU_EVT_ENTER) // pressed ENTER (to enter the 'Zone List') ?
	{
		am_cbk_SaveScanList(pMenu, pItem, event, param);
		am_cbk_ChannelListView_OnEnter(pMenu, pItem);
		return AM_RESULT_OCCUPY_SCREEN; // occupy the entire screen (not just a single line)
	}
	else if (event == APPMENU_EVT_PAINT) // someone wants us to paint into the framebuffer
	{ // To minimize QRM from the display cable, only redraw when necessary (no "dynamic" content here):
		if (pMenu->visible == APPMENU_USERSCREEN_VISIBLE) // only if HexMon already 'occupied' the screen !
		{
			if (pMenu->redraw)
			{
				pMenu->redraw = FALSE;   // don't modify this sequence
				ChannelListView_Draw(pMenu, pItem); // <- may decide to draw AGAIN (scroll)
			}
			return AM_RESULT_OCCUPY_SCREEN; // keep the screen 'occupied' 
		}
	}
	else if (event == APPMENU_EVT_KEY) // some other key pressed while focused..
	{
		switch ((char)param) // here: message parameter = keyboard code (ASCII)
		{
		case 'M':  // green "Menu" key : kind of ENTER. But here, "apply & return" .
			
			selScanListChan = pSL->focused_item;
			Menu_EnterSubmenu(pMenu, &am_ScanList_Manage);
			return AM_RESULT_EXIT_AND_RELEASE_SCREEN;

		case 'B':  // red "Back"-key : return from this screen, discard changes.
			return AM_RESULT_EXIT_AND_RELEASE_SCREEN;

		case 'U':  // cursor UP
			if (pSL->focused_item > 0)
			{
				--pSL->focused_item;
			}
			else if (pSL->focused_item == 0) {
				pSL->focused_item = pSL->num_items - 1;
			}
			break;
		case 'D':  // cursor DOWN
			if (pSL->focused_item < (pSL->num_items - 1))
			{
				++pSL->focused_item;
			}
			else {
				pSL->focused_item = 0;
			}
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
} // end am_cbk_ChannelListView()

int am_cbk_ChannelListAdd(app_menu_t *pMenu, menu_item_t *pItem, int event, int param)
// Callback function, invoked from the "app menu" framework for the 'ZONES' list.
// (lists ALL ZONES, not THE CHANNELS in a zone)
{
	scroll_list_control_t *pSL = &pMenu->scroll_list;

	// what happened, why did the menu framework call us ?
	if (event == APPMENU_EVT_ENTER) // pressed ENTER (to enter the 'Zone List') ?
	{
		am_cbk_SaveScanList(pMenu, pItem, event, param);
		ChannelList_OnEnter(pMenu, pItem);
		return AM_RESULT_OCCUPY_SCREEN; // occupy the entire screen (not just a single line)
	}
	else if (event == APPMENU_EVT_PAINT) // someone wants us to paint into the framebuffer
	{ // To minimize QRM from the display cable, only redraw when necessary (no "dynamic" content here):
		if (pMenu->visible == APPMENU_USERSCREEN_VISIBLE) // only if HexMon already 'occupied' the screen !
		{
			if (pMenu->redraw)
			{
				pMenu->redraw = FALSE;   // don't modify this sequence
				ChannelList_Draw(pMenu, pItem); // <- may decide to draw AGAIN (scroll)
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
				
				for (int i = 0; i < CODEPLUG_MAX_SCANLIST_CHANNELS; i++) {
					if (!selScanList.channels[i]) {
						selScanList.channels[i] = pSL->focused_item+1;
						ScanList_WriteByIndex(selScanListIndex, &selScanList);
						break;
					}
				}
				return AM_RESULT_EXIT_AND_RELEASE_SCREEN;
			}
			return AM_RESULT_EXIT_AND_RELEASE_SCREEN;
		case 'B':  // red "Back"-key : return from this screen, discard changes.
			return AM_RESULT_EXIT_AND_RELEASE_SCREEN;
		case 'U':  // cursor UP
			if (pSL->focused_item > 0)
			{
				--pSL->focused_item;
			}
			else if (pSL->focused_item == 0) {
				pSL->focused_item = pSL->num_items - 1;

			}
			//ContactsList_Recache(pMenu);
			break;
		case 'D':  // cursor DOWN
			if (pSL->focused_item < (pSL->num_items - 1))
			{
				++pSL->focused_item;
			}
			else {
				pSL->focused_item = 0;
			}
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
} // end am_cbk_ChannelListAdd()

const menu_item_t am_ScanList_Edit[]        = { { "[-]Name:", DTYPE_WSTRING, APPMENU_OPT_NONE, 0, selScanList.name, 0, 0,  NULL, NULL },
												{ "Hold(ms):", DTYPE_UNS8, APPMENU_OPT_EDITABLE | APPMENU_OPT_FACTOR, 25, &selScanList.holdtime, 50, 5000,  NULL, NULL },
												{ "Sample(ms):", DTYPE_UNS8, APPMENU_OPT_EDITABLE | APPMENU_OPT_FACTOR, 250, &selScanList.sampletime, 750, 5000,  NULL, NULL },
												{ "[-]Edit List", DTYPE_NONE, APPMENU_OPT_NONE, 0, NULL, 0, 0, NULL, am_cbk_ChannelListView },
												{ "Add To List", DTYPE_NONE, APPMENU_OPT_NONE, 0, NULL, 0, 0, NULL, am_cbk_ChannelListAdd },
												{ "Delete", DTYPE_NONE, APPMENU_OPT_BACK, 0, NULL, 0, 0, NULL, am_cbk_DeleteScanList },
												{ "[-]Back (Save)", DTYPE_NONE, APPMENU_OPT_BACK, 0, NULL, 0, 0, NULL, am_cbk_SaveScanList },
												// End of the list marked by "all zeroes" :
												{ NULL, 0/*dt*/, 0/*opt*/, 0/*ov*/, NULL/*pValue*/, 0,0, NULL, NULL } };



  //---------------------------------------------------------------------------
int am_cbk_ScanList(app_menu_t *pMenu, menu_item_t *pItem, int event, int param)
// Callback function, invoked from the "app menu" framework for the 'ZONES' list.
// (lists ALL ZONES, not THE CHANNELS in a zone)
{
	scroll_list_control_t *pSL = &pMenu->scroll_list;

	// what happened, why did the menu framework call us ?
	if (event == APPMENU_EVT_ENTER) // pressed ENTER (to enter the 'Zone List') ?
	{
		ScanList_OnEnter(pMenu, pItem);
		return AM_RESULT_OCCUPY_SCREEN; // occupy the entire screen (not just a single line)
	}
	else if (event == APPMENU_EVT_PAINT) // someone wants us to paint into the framebuffer
	{ // To minimize QRM from the display cable, only redraw when necessary (no "dynamic" content here):
		if (pMenu->visible == APPMENU_USERSCREEN_VISIBLE) // only if HexMon already 'occupied' the screen !
		{
			if (pMenu->redraw)
			{
				pMenu->redraw = FALSE;   // don't modify this sequence
				ScanList_Draw(pMenu, pItem); // <- may decide to draw AGAIN (scroll)
			}
			return AM_RESULT_OCCUPY_SCREEN; // keep the screen 'occupied' 
		}
	}
	else if (event == APPMENU_EVT_KEY) // some other key pressed while focused..
	{
		switch ((char)param) // here: message parameter = keyboard code (ASCII)
		{
		case 'M':  // green "Menu" key : kind of ENTER. But here, "apply & return" .
			if (pSL->focused_item > 0)
			{
				selScanListIndex = pSL->focused_item - 1;
				ScanList_ReadByIndex(selScanListIndex, &selScanList);
				
				Menu_EnterSubmenu(pMenu, &am_ScanList_Edit);
				
				StartStopwatch(&pMenu->stopwatch_late_redraw);
			}
			else if (pSL->focused_item == 0) {
				memset(&selScanList, 0, sizeof(scanlist_t));
				snprintfw(selScanList.name, 16, "ScanList%d\0", (pSL->num_items));
				md380_spiflash_write(selScanList.name, ((pSL->num_items - 1) * CODEPLUG_SIZEOF_SCANLIST_ENTRY)
					+ CODEPLUG_SPIFLASH_ADDR_SCANLIST, 32);
			}
			return AM_RESULT_EXIT_AND_RELEASE_SCREEN;
		case 'B':  // red "Back"-key : return from this screen, discard changes.
			return AM_RESULT_EXIT_AND_RELEASE_SCREEN;
		case 'U':  // cursor UP
			if (pSL->focused_item > 0)
			{
				--pSL->focused_item;
			}
			else if(pSL->focused_item == 0) {
				pSL->focused_item = pSL->num_items - 1;
				
			}
			break;
		case 'D':  // cursor DOWN
			if (pSL->focused_item < (pSL->num_items - 1))
			{
				++pSL->focused_item;
			}
			else {
				pSL->focused_item = 0;
			}
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
} // end am_cbk_ScanList()


