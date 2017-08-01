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
#include "amenu_contacts.h" // header for THIS module (to check prototypes,etc)
#include "amenu_set_tg.h"

contact_t selContact;
int contactIndex = 0;

char fIsTG = 0;

int am_cbk_SetType(app_menu_t *pMenu, menu_item_t *pItem, int event, int param);
int am_cbk_SetID(app_menu_t *pMenu, menu_item_t *pItem, int event, int param);
int am_cbk_CallContact(app_menu_t *pMenu, menu_item_t *pItem, int event, int param);

const am_stringtable_t am_stringtab_contact_types[] =
{
	//{ 0xC1, "Private" },
	//{ 0x0A, "Groupcall" },
	{ 0, "Private" },
	{ 1, "Groupcall" },
	{ 0, NULL }
};

menu_item_t am_Contact_Edit[] = // setup menu, nesting level 1 ...
{ // { "Text__max__13", data_type,  options,opt_value,
  //     pvValue,iMinValue,iMaxValue, string table, callback }


	// { "Text__max__13", data_type,  options,opt_value,
	//    pvValue,iMinValue,iMaxValue,           string table, callback }
	
	{ "[-]Name",             DTYPE_WSTRING, APPMENU_OPT_NONE,0,
	selContact.name ,0,0,          NULL,         NULL },

	{ "ID", DTYPE_INTEGER, APPMENU_OPT_EDITABLE,0,
	0/*pvValue*/,0/*min*/,0x00FFFFFF/*max:24 bit*/, NULL,am_cbk_SetID },

	{ "Type", DTYPE_UNS8,
	APPMENU_OPT_EDITABLE | APPMENU_OPT_BITMASK,
	1, // <- here: bitmask !
	&fIsTG,0,1, am_stringtab_contact_types, am_cbk_SetType },

	{ "[-]Set as TG",   DTYPE_NONE, APPMENU_OPT_BACK,0,
	NULL ,0,0,          NULL,         am_cbk_CallContact },
	
	{ "Back",       DTYPE_NONE, APPMENU_OPT_BACK,0,
	NULL,0,0,                  NULL,  am_cbk_ContactsList },

	// End of the list marked by "all zeroes" :
	{ NULL, 0/*dt*/, 0/*opt*/, 0/*ov*/, NULL/*pValue*/, 0,0, NULL, NULL }

}; // end am_Setup[]

//---------------------------------------------------------------------------
BOOL ContactsList_ReadNameByIndex(int index,             // [in] zero-based zone index
	contact_t *tContact) // [out] zone name as a C string
						 // Return value : TRUE when successfully read a zone name for this index,
						 //                FALSE when failed or not implemented for a certain firmware.
{
	if (index >= 0 && index < CODEPLUG_MAX_DIGITAL_CONTACT_ENTIES)
	{
		md380_spiflash_read(tContact, (index * CODEPLUG_SIZEOF_DIGITAL_CONTACT_ENTRY)
			+ CODEPLUG_SPIFLASH_ADDR_DIGITAL_CONTACT_LIST,
			sizeof(contact_t));
		//char wc16Temp[16+1];
		//wide_to_C_string(tContact->name, wc16Temp, 16);
		//int len = strlen(wc16Temp);
		//tContact->name[len] = '\0';
		//wc16Temp[16] = '\0';
		
		//memcpy(tContact->name, wc16Temp, len);


		// unused entries in the codeplug appeared 'zeroed' (filled with 0x00),
		// thus 0x00 in the first character seems to mark the end of the list:
		//return wc16Temp[0] != 0;
		tContact->name[15] = '\0';
		return tContact->name[0] != '\0' && !(tContact->id_h == 0xFF && tContact->id_l == 0xFF && tContact->id_m == 0xFF);
	}
	return 0;
} // end ZoneList_ReadNameByIndex()

int am_cbk_SetID(app_menu_t *pMenu, menu_item_t *pItem, int event, int param)
{
	int val = 0;
	switch (event) // what happened, why did the menu framework call us ?
	{

	case APPMENU_EVT_GET_VALUE: // called to retrieve the current value
								// How to retrieve the talkgroup number ? Inspired by Brad's PR #708 :
		
		return ((int)selContact.id_h << 16) | ((int)selContact.id_m << 8) | (int)selContact.id_l;


	case APPMENU_EVT_END_EDIT: // the operator finished or aborted editing,

							   //if (param) // "finished", not "aborted" -> write back the new ("edited") value
	{
		selContact.id_l = pMenu->iEditValue & 0xFF;
		selContact.id_m = (pMenu->iEditValue >> 8) & 0xFF;
		selContact.id_h = (pMenu->iEditValue >> 16) & 0xFF;

		md380_spiflash_write(&selContact, (contactIndex*CODEPLUG_SIZEOF_DIGITAL_CONTACT_ENTRY)
			+ CODEPLUG_SPIFLASH_ADDR_DIGITAL_CONTACT_LIST, 3);
	} // end if < FINISHED (not ABORTED) editing >
	return AM_RESULT_OK; // "event was processed HERE"
	default: // all other events are not handled here (let the sender handle them)
		break;
	} // end switch( event )
	return AM_RESULT_NONE; // "proceed as if there was NO callback function"
} // end am_cbk_SetTalkgroup()

int am_cbk_SetType(app_menu_t *pMenu, menu_item_t *pItem, int event, int param)
{
	switch (event) // what happened, why did the menu framework call us ?
	{
	case APPMENU_EVT_GET_VALUE: // called to retrieve the current value
								// How to retrieve the talkgroup number ? Inspired by Brad's PR #708 :
		return fIsTG;
	case APPMENU_EVT_END_EDIT: // the operator finished or aborted editing,
		
		//if (param) // "finished", not "aborted" -> write back the new ("edited") value
		{
			selContact.type = (fIsTG == 0 ? 0x0A : 0xC1);
			md380_spiflash_write(&selContact.type, (contactIndex*CODEPLUG_SIZEOF_DIGITAL_CONTACT_ENTRY)
				+ CODEPLUG_SPIFLASH_ADDR_DIGITAL_CONTACT_LIST + 3, 1);
		} // end if < FINISHED (not ABORTED) editing >
		return AM_RESULT_OK; // "event was processed HERE"
	default: // all other events are not handled here (let the sender handle them)
		break;
	} // end switch( event )
	return AM_RESULT_NONE; // "proceed as if there was NO callback function"
} // end am_cbk_SetTalkgroup()

int am_cbk_CallContact(app_menu_t *pMenu, menu_item_t *pItem, int event, int param)
{
	switch (event) // what happened, why did the menu framework call us ?
	{
	
	case APPMENU_EVT_ENTER: // the operator finished or aborted editing,

	{
		ad_hoc_call_type = selContact.type;
		ad_hoc_talkgroup = ((int)selContact.id_h << 16) | ((int)selContact.id_m << 8) | (int)selContact.id_l;
		ad_hoc_tg_channel = channel_num;
		channel_num = 0;
		CheckTalkgroupAfterChannelSwitch();
		
	} // end if < FINISHED (not ABORTED) editing >
	return AM_RESULT_OK; // "event was processed HERE"
	default: // all other events are not handled here (let the sender handle them)
		break;
	} // end switch( event )
	return AM_RESULT_NONE; // "proceed as if there was NO callback function"
} // end am_cbk_CallContact()


/*
static int ContactsList_Recache(app_menu_t *pMenu) {
	scroll_list_control_t *pSL = &pMenu->scroll_list;
	int start = pSL->focused_item;
	if (start >= 16)
		start -= 16;
	int i = start, b=0;
	memset(sz20, 0, 16*sizeof(contact_t));

	while (i < start+16 && i < CODEPLUG_MAX_DIGITAL_CONTACT_ENTIES)
	{
		if (!ContactsList_ReadNameByIndex(i, &sz20[b])) // guess all zones are through
		{
			break;
		}
		++i;
		++b;
	}
}*/

  //---------------------------------------------------------------------------
static void ContactsList_OnEnter(app_menu_t *pMenu, menu_item_t *pItem)
// Called ONCE when "entering" the 'Zone List' display.
// Tries to find out how many zones exist in the codeplug,
// and the array-index of the currently active zone . 
{
	contact_t tCont;
	char sz20CurrZone[16];
	scroll_list_control_t *pSL = &pMenu->scroll_list;

	ScrollList_Init(pSL); // set all struct members to defaults
	//pSL->current_item = contactIndex; // currently active zone still unknown
	pSL->focused_item = contactIndex;
	//contactIndex = 0;
	int start = 0;
	int i = start, b = 0;
	
	while (i < CODEPLUG_MAX_DIGITAL_CONTACT_ENTIES)
	{
		contact_t* pCont = (i == contactIndex ? &selContact : &tCont);
		if (!ContactsList_ReadNameByIndex(i, pCont)) // guess all zones are through
		{
			break;
		}
		++i;
		++b;
	}

	pSL->num_items = i;
	//ContactsList_Recache(pMenu);
	

	// Begin navigating through the list at the currently active zone:
	if (pSL->focused_item > pSL->num_items)
	{
		pSL->focused_item = 0;
	}

} // end ZoneList_OnEnter()

  //---------------------------------------------------------------------------
static void ContactsList_Draw(app_menu_t *pMenu, menu_item_t *pItem)
// Draws the 'zone list' screen. Gets visible when ENTERING that item in the app-menu.
// 
{
	int i, n_visible_items, sel_flags = SEL_FLAG_NONE;
	lcd_context_t dc;
	char cRadio; // character code for a selected or unselected "radio button"
	scroll_list_control_t *pSL = &pMenu->scroll_list;

	contact_t tCont;

	// Draw the COMPLETE screen, without clearing it initially to avoid flicker
	LCD_InitContext(&dc); // init context for 'full screen', no clipping
	Menu_GetColours(sel_flags, &dc.fg_color, &dc.bg_color);
	ScrollList_AutoScroll(pSL); // modify pSL->scroll_pos to make the FOCUSED item visible
	dc.font = LCD_OPT_FONT_16x16;
	{
		LCD_Printf(&dc, "  %d/%d\r", (int)(pSL->focused_item + 1), (int)pSL->num_items);
	}
	LCD_HorzLine(dc.x1, dc.y++, dc.x2, dc.fg_color); // spacer between title and scrolling list
	LCD_HorzLine(dc.x1, dc.y++, dc.x2, dc.bg_color);
	i = pSL->scroll_pos;   // zero-based array index of the topmost VISIBLE item
	n_visible_items = 0;   // find out how many items fit on the screen
	while ((dc.y < (LCD_SCREEN_HEIGHT - 8)) && (i<pSL->num_items))
	{
		contact_t* pCont = (i == contactIndex ? &selContact : &tCont);
		if (!ContactsList_ReadNameByIndex(i, pCont)) // oops.. shouldn't have reached the last item yet !
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
			if (pCont->type == 0xC1) {
				cRadio = 0xF9;
			}
			else {
				cRadio = 0xE9;
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
} // ZoneList_Draw()


  //---------------------------------------------------------------------------
int am_cbk_ContactsList(app_menu_t *pMenu, menu_item_t *pItem, int event, int param)
// Callback function, invoked from the "app menu" framework for the 'ZONES' list.
// (lists ALL ZONES, not THE CHANNELS in a zone)
{
	scroll_list_control_t *pSL = &pMenu->scroll_list;

	// what happened, why did the menu framework call us ?
	if (event == APPMENU_EVT_ENTER) // pressed ENTER (to enter the 'Zone List') ?
	{
		ContactsList_OnEnter(pMenu, pItem);
		return AM_RESULT_OCCUPY_SCREEN; // occupy the entire screen (not just a single line)
	}
	else if (event == APPMENU_EVT_PAINT) // someone wants us to paint into the framebuffer
	{ // To minimize QRM from the display cable, only redraw when necessary (no "dynamic" content here):
		if (pMenu->visible == APPMENU_USERSCREEN_VISIBLE) // only if HexMon already 'occupied' the screen !
		{
			if (pMenu->redraw)
			{
				pMenu->redraw = FALSE;   // don't modify this sequence
				ContactsList_Draw(pMenu, pItem); // <- may decide to draw AGAIN (scroll)
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
				contactIndex = pSL->focused_item;
				fIsTG = (selContact.type == 0xC1 ? 1 : 0);
				Menu_EnterSubmenu(pMenu, am_Contact_Edit);

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
			if (pSL->focused_item > 0)
			{
				--pSL->focused_item;
			}
			else if(pSL->focused_item == 0) {
				pSL->focused_item = pSL->num_items - 1;
				
			}
			contactIndex = pSL->focused_item;
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
			contactIndex = pSL->focused_item;
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


