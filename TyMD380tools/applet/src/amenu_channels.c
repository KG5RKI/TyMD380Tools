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
#include "amenu_channels.h" // header for THIS module (to check prototypes,etc)
#include "amenu_codeplug.h"
#include "amenu_contacts.h"

channel_t selChan;
channel_easy selChanE;
int selIndex = 0;
int numChannels = 0;
char Contact_Name[50];

#define CHANNEL_LIST_EXTRA_OPTIONS 1

int SaveChannel(channel_t* chan, channel_easy* chanE);
int ParseChannel(channel_t* chan, channel_easy* chanE);
int am_cbk_Channel_AddToZone(app_menu_t *pMenu, menu_item_t *pItem, int event, int param);
void ChannelList_WriteByIndex(int index, channel_t *tChannel);
int am_cbk_Channel_CloneToZone(app_menu_t *pMenu, menu_item_t *pItem, int event, int param);
int am_cbk_Channel_Save(app_menu_t *pMenu, menu_item_t *pItem, int event, int param);

menu_item_t am_Channel_Edit[] = // setup menu, nesting level 1 ...
{
	{ "Edit Channel",             DTYPE_NONE, APPMENU_OPT_NONE,0,
	NULL,0,0,          NULL,         NULL },
	{ "[-]Name",             DTYPE_WSTRING, APPMENU_OPT_NONE,0,
	selChanE.name ,0,0,          NULL,         NULL },
	{ "Contact",             DTYPE_STRING, APPMENU_OPT_NONE,0,
	Contact_Name ,0,0,          NULL,         NULL },


	{ "RX",             DTYPE_STRING, APPMENU_OPT_NONE,0,
	selChanE.rxFreq.text ,0,0,          NULL,         NULL },
	{ "TX",             DTYPE_STRING, APPMENU_OPT_NONE,0,
	selChanE.rxFreq.text ,0,0,          NULL,         NULL },

	{ "Color Code",      DTYPE_UNS8, APPMENU_OPT_EDITABLE | APPMENU_OPT_IMM_UPDATE,0,
	&selChanE.CC ,0,15,          NULL,         NULL },
	{ "Timeslot",      DTYPE_UNS8, APPMENU_OPT_EDITABLE | APPMENU_OPT_IMM_UPDATE,0,
	&selChanE.Slot ,1,2,          NULL,         NULL },
	{ "TOT",      DTYPE_UNS8, APPMENU_OPT_EDITABLE | APPMENU_OPT_IMM_UPDATE | APPMENU_OPT_FACTOR, 15,
	&selChanE.TOT ,0,60,          NULL,         NULL },
	{ "ScanList",      DTYPE_UNS8, APPMENU_OPT_NONE,0,
	&selChanE.ScanListIndex ,0,0,          NULL,         NULL },
	{ "GroupList",      DTYPE_UNS8, APPMENU_OPT_NONE,0,
	&selChanE.GroupListIndex ,0,0,          NULL,         NULL },

	{ "Clone CH",      DTYPE_NONE, APPMENU_OPT_BACK,0,
	NULL,0,0,                  NULL, am_cbk_Channel_CloneToZone },

	{ "Set CH",      DTYPE_NONE, APPMENU_OPT_BACK,0,
	NULL,0,0,                  NULL, am_cbk_Channel_AddToZone },

	{ "Save",      DTYPE_NONE, APPMENU_OPT_BACK,0,
	NULL,0,0,                  NULL, am_cbk_Channel_Save },

	

	{ "Back",       DTYPE_NONE, APPMENU_OPT_BACK,0,
	NULL,0,0,                  NULL,  am_cbk_ChannelList },

	// End of the list marked by "all zeroes" :
	{ NULL, 0, 0, 0, NULL, 0,0, NULL, NULL }

}; // end am_Setup[]




int am_cbk_Channel_Save(app_menu_t *pMenu, menu_item_t *pItem, int event, int param)
{ // Simple example for a 'user screen' opened from the application menu
	if (event == APPMENU_EVT_ENTER) // pressed ENTER (to launch the colour test) ?
	{
		SaveChannel(&selChan, &selChanE);
		ChannelList_WriteByIndex(selIndex, &selChan);
		return AM_RESULT_OK; // screen now 'occupied' by the colour test screen
	}
	return AM_RESULT_NONE; // "proceed as if there was NO callback function"
} // end am_cbk_ColorTest()


int am_cbk_Channel_CloneToZone(app_menu_t *pMenu, menu_item_t *pItem, int event, int param)
{ // Simple example for a 'user screen' opened from the application menu
	if (event == APPMENU_EVT_ENTER) // pressed ENTER (to launch the colour test) ?
	{
		if (numChannels >= CODEPLUG_MAX_DIGITAL_CONTACT_ENTIES)
			return AM_RESULT_OK;

		SaveChannel(&selChan, &selChanE);

		ChannelList_WriteByIndex(numChannels, &selChan);
		
		overwriteChannel(numChannels +1); // only draw the colour test pattern ONCE...
		numChannels++;
		return AM_RESULT_OK; // screen now 'occupied' by the colour test screen
	}
	return AM_RESULT_NONE; // "proceed as if there was NO callback function"
} // end am_cbk_ColorTest()

int am_cbk_Channel_AddToZone(app_menu_t *pMenu, menu_item_t *pItem, int event, int param)
{ // Simple example for a 'user screen' opened from the application menu
	if (event == APPMENU_EVT_ENTER) // pressed ENTER (to launch the colour test) ?
	{
		
		//ChannelList_WriteByIndex(selIndex, &selChan);
		SaveChannel(&selChan, &selChanE);
		overwriteChannel(selIndex + 1); // only draw the colour test pattern ONCE...
		numChannels++;
		return AM_RESULT_OK; // screen now 'occupied' by the colour test screen
	}
	return AM_RESULT_NONE; // "proceed as if there was NO callback function"
} // end am_cbk_ColorTest()


uint8_t getCC(channel_t* ch)
{
	return ((((uint8_t*)ch)[1] & 0xF0) >> 4) & 0xF;
}
void setCC(channel_t* ch, uint8_t cc)
{
	((uint8_t*)ch)[1] = (((uint8_t*)ch)[1] & 0x0F) | ((cc << 4) & 0xF0);
}

uint8_t getSlot(channel_t* ch)
{
	return ((((uint8_t*)ch)[1] & 0x04) != 0 ? 1 : 2);
}
void setSlot(channel_t* ch, uint8_t slot)
{
	((uint8_t*)ch)[1] = (((uint8_t*)ch)[1] & (~0x04)) | (slot<=1 ? 0x04: 0);
}

uint16_t getContactIndex(channel_t* ch)
{
	return *((uint16_t*)&(((uint8_t*)ch)[6]));
}
void setContactIndex(channel_t* ch, uint16_t contIndex)
{
	*((uint16_t*)&(((uint8_t*)ch)[6])) = contIndex;
}

uint8_t getTOT(channel_t* ch)
{
	return ch->settings[8];
}
void setTOT(channel_t* ch, uint8_t tot)
{
	ch->settings[8] = tot;
}

uint8_t getTOTRekeyDelay(channel_t* ch)
{
	return ch->settings[9];
}

uint8_t getEmergencyIndex(channel_t* ch)
{
	return ch->settings[10];
}

uint8_t getScanListIndex(channel_t* ch)
{
	return ch->settings[11];
}

uint8_t getGroupListIndex(channel_t* ch)
{
	return ch->settings[12];
}


int readFrequency(channel_t* chan, frequency_t* freq, char fRx)
{
	for (int i = 0; i < 8; i++) {
		if ((i + 1) % 2 != 0) {
			freq->digits[i] = chan->settings[(fRx ? 16 : 20) + (i / 2)] & 0xF;
		}
		else {
			freq->digits[i] = (chan->settings[(fRx ? 16 : 20) + (i / 2)] >> 4) & 0xF;
		}
	}
	sprintf(freq->text, "%d%d%d.%d%d%d%d%d\0\0", freq->digits[7], freq->digits[6], freq->digits[5], freq->digits[4], freq->digits[3], freq->digits[2], freq->digits[1], freq->digits[0]);
	return 0;
}


//---------------------------------------------------------------------------
BOOL ChannelList_ReadNameByIndex(int index,            
	channel_t *tChannel) 
{
	if (index >= 0 && index < CODEPLUG_MAX_CHANNELS)
	{
		md380_spiflash_read(tChannel, (index * CODEPLUG_SIZEOF_CHANNEL_ENTRY)
			+ CODEPLUG_SPIFLASH_ADDR_CHANNEL,
			CODEPLUG_SIZEOF_CHANNEL_ENTRY);
	
		tChannel->name[15] = '\0';
		return !(tChannel->settings[3] & 0x01);
	}
	return 0;
} // end ZoneList_ReadNameByIndex()

void ChannelList_WriteByIndex(int index,
	channel_t *tChannel)
{
	if (index >= 0 && index < CODEPLUG_MAX_CHANNELS)
	{
		md380_spiflash_write(tChannel, (index * CODEPLUG_SIZEOF_CHANNEL_ENTRY)
			+ CODEPLUG_SPIFLASH_ADDR_CHANNEL,
			CODEPLUG_SIZEOF_CHANNEL_ENTRY);
	}
} // end ZoneList_ReadNameByIndex()


  //---------------------------------------------------------------------------
static void ChannelList_OnEnter(app_menu_t *pMenu, menu_item_t *pItem)
// Called ONCE when "entering" the 'Zone List' display.
// Tries to find out how many zones exist in the codeplug,
// and the array-index of the currently active zone . 
{
	channel_t tChan;
	scroll_list_control_t *pSL = &pMenu->scroll_list;

	ScrollList_Init(pSL); // set all struct members to defaults
	//pSL->current_item = contactIndex; // currently active zone still unknown
	pSL->focused_item = selIndex;
	//contactIndex = 0;
	int start = 0;
	int i = start, b = 0;
	
	while (i < CODEPLUG_MAX_CHANNELS)
	{
		channel_t* pCont = (i == selIndex ? &selChan : &tChan);
		if (!ChannelList_ReadNameByIndex(i, pCont)) // guess all zones are through
		{
			break;
		}
		if (i == selIndex)
			ParseChannel(pCont, &selChanE);
		++i;
		++b;
	}
	pSL->num_items = i;
	numChannels = i;
	//ContactsList_Recache(pMenu);
	

	// Begin navigating through the list at the currently active zone:
	if (pSL->focused_item > pSL->num_items)
	{
		pSL->focused_item = 0;
	}

} // end ZoneList_OnEnter()

  //---------------------------------------------------------------------------
static void ChannelList_Draw(app_menu_t *pMenu, menu_item_t *pItem)
// Draws the 'zone list' screen. Gets visible when ENTERING that item in the app-menu.
// 
{
	int i, n_visible_items, sel_flags = SEL_FLAG_NONE;
	lcd_context_t dc;
	char cRadio; // character code for a selected or unselected "radio button"
	scroll_list_control_t *pSL = &pMenu->scroll_list;

	channel_t tCont;
	//channel_easy tContE;

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
		channel_t* pCont = (i == selIndex ? &selChan : &tCont);
		if (!ChannelList_ReadNameByIndex(i, pCont)) // oops.. shouldn't have reached the last item yet !
		{
			break;
		}

		if (i == selIndex) {
			ParseChannel(pCont, &selChanE);
		}
		//curCon = &sz20[i];

		if (i == pSL->focused_item) // this is the CURRENTLY ACTIVE zone :
		{
			cRadio = 0x1A;  // character code for a 'selected radio button', see applet/src/font_8_8.c 
		}
		else
		{
			//if (pCont->type == 0xC1) {
				cRadio = 0xF9;
			//}
			//else {
			//	cRadio = 0xE9;
			//}
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
int am_cbk_ChannelList(app_menu_t *pMenu, menu_item_t *pItem, int event, int param)
// Callback function, invoked from the "app menu" framework for the 'ZONES' list.
// (lists ALL ZONES, not THE CHANNELS in a zone)
{
	scroll_list_control_t *pSL = &pMenu->scroll_list;

	// what happened, why did the menu framework call us ?
	if (event == APPMENU_EVT_ENTER) // pressed ENTER (to enter the 'Zone List') ?
	{
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
				//ContactList_SetZoneByIndex(pSL->focused_item);
				selIndex = pSL->focused_item;
				//fIsTG = (selContact.type == 0xC1 ? 1 : 0);
				contact_t cont;
				ContactsList_ReadNameByIndex(selChanE.ContactIndex-1, &cont);
				wide_to_C_string(cont.name, Contact_Name, 50);
				Menu_EnterSubmenu(pMenu, am_Channel_Edit);

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
			selIndex = pSL->focused_item;
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
			selIndex = pSL->focused_item;
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


int ParseChannel(channel_t* chan, channel_easy* chanE)
{
	
	memcpy(chanE->name, chan->name, 32);
	chanE->name[15] = '\0';

	//printf("Channel Name: %S\r\n", chanE->name);

	chanE->CC = getCC(chan);

	chanE->TOT = getTOT(chan);
	//printf("Color Code: %d\r\n", chanE->CC);

	chanE->Slot = getSlot(chan);
	//printf("Repeater Slot: %d\r\n", chanE->Slot);

	chanE->ContactIndex = getContactIndex(chan);
	//printf("ContactIndex: %d\r\n", chanE->ContactIndex);

	chanE->EmergencyIndex = getEmergencyIndex(chan);
	//printf("EmergencyIndex: %d\r\n", chanE->EmergencyIndex);

	chanE->GroupListIndex = getGroupListIndex(chan);
	//printf("GroupListIndex: %d\r\n", chanE->GroupListIndex);

	chanE->ScanListIndex = getScanListIndex(chan);
	//printf("ScanListIndex: %d\r\n", chanE->ScanListIndex);


	readFrequency(chan, &chanE->rxFreq, 1);
	//printf("RX: %s\r\n", chanE->rxFreq.text);
	readFrequency(chan, &chanE->txFreq, 1);
	//printf("TX: %s\r\n", chanE->txFreq.text);
}

int SaveChannel(channel_t* chan, channel_easy* chanE)
{

	memcpy(chan->name, chanE->name, 32);
	chan->name[15] = '\0';

	//printf("Channel Name: %S\r\n", chanE->name);

	setCC(chan, chanE->CC);
	//printf("Color Code: %d\r\n", chanE->CC);

	setTOT(chan, chanE->TOT);

	setSlot(chan, chanE->Slot);
	//printf("Repeater Slot: %d\r\n", chanE->Slot);

	setContactIndex(chan, chanE->ContactIndex);
	//printf("ContactIndex: %d\r\n", chanE->ContactIndex);

	//*getEmergencyIndex(chan) = chanE->EmergencyIndex;
	//printf("EmergencyIndex: %d\r\n", chanE->EmergencyIndex);

	//*getGroupListIndex(chan) = chanE->GroupListIndex;
	//printf("GroupListIndex: %d\r\n", chanE->GroupListIndex);

	//*getScanListIndex(chan) = chanE->ScanListIndex;
	//printf("ScanListIndex: %d\r\n", chanE->ScanListIndex);

	

	//readFrequency(chan, &chanE->rxFreq, 1);
	//printf("RX: %s\r\n", chanE->rxFreq.text);
	//readFrequency(chan, &chanE->txFreq, 1);
	//printf("TX: %s\r\n", chanE->txFreq.text);
}