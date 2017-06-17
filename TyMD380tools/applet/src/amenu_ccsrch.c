// File:    md380tools/applet/src/amenu_hexmon.c
// Author:  Wolf (DL4YHF) [initial version]
//
// Date:    2017-04-28
//  Simple 'hex monitor' to inspect blocks of memory.
//  Implemented as a callback function for the 'application menu' (app_menu.c) .

#include "config.h"

#if (CONFIG_APP_MENU) // this module is only available along with app_menu.c ...

#include <stm32f4xx.h>
#include <string.h>
#include "irq_handlers.h"
#include "lcd_driver.h"
#include "app_menu.h" // 'simple' alternative menu activated by red BACK-button
#include "printf.h"
#include "spiflash.h" // md380_spiflash_read()
#include "amenu_ccsrch.h" // header for THIS module (to check prototypes,etc)
#include "md380.h"

uint32_t ccsrch_stopwatch = 0;

char ccsrch_buffer[0x100] = { 0 };
uint32_t ccsrch_buffer_index = 0;


uint8_t cc = 20;
uint8_t fMatch = 0;

//---------------------------------------------------------------------------
static void CCSrch_Draw(app_menu_t *pMenu, menu_item_t *pItem)
  // Draws the 'hex monitor' screen. Gets visible after entering/confirming
  //  the start address in the 'test / debug' menu (or whatever the final name will be) .
{ int i, n, bytes_per_line, rd_value, font;
  lcd_context_t dc;
  char tmpStr[0x500];
  char *cp;
  char buffa = 0, buffa2=0;

  // Draw the COMPLETE screen, without clearing it initially to avoid flicker
  LCD_InitContext( &dc ); // init context for 'full screen', no clipping
  Menu_GetColours( SEL_FLAG_NONE, &dc.fg_color, &dc.bg_color );
  
  
  dc.font = LCD_OPT_FONT_8x8;
  

  n = 0;         // count the number of bytes per page (depends on the used font)
  
  if ((ReadStopwatch_ms(&ccsrch_stopwatch) > 100/*ms*/ || cc > 16))
  {
	  LCD_FillRect(0, 0, LCD_SCREEN_WIDTH - 1, LCD_SCREEN_HEIGHT - 1, dc.bg_color);

	  LCD_Printf(&dc, "\r--Color Code Search--\r"); // title, centered, opaque, full line

	  StartStopwatch(&ccsrch_stopwatch);

	  if (!fMatch) {
		  c5000_spi0_readreg(0x0d, &buffa);
		  c5000_spi0_readreg(0x1f, &buffa2);
		  buffa2 = ((buffa2 & 0xF0) >> 4) & 0xF;
	  }

	  //found match!
	  if ((buffa & 0x10 && buffa2 == cc )|| fMatch) {
		  fMatch = 1;
		  LCD_Printf("Found matched cc %d!\r", cc);
	  }
	  else {
		  cc++;
		  if (cc > 15) {
			  cc = 0;
		  }
		  LCD_Printf(&dc, "%02x Testing cc %d\r", buffa, cc);
		  //sprintf(tmpStr, "%02x Testing cc %d\r", buffa, cc);
		  c5000_spi0_writereg(0x1f, (cc & 0xF) << 4);
	  }

	 
	  if (strlen(tmpStr) + strlen(ccsrch_buffer) >= 0x500) {
		  ccsrch_buffer_index = 0;
	  }
	  //strcpy(&ccsrch_buffer[ccsrch_buffer_index], tmpStr);
	  //ccsrch_buffer_index += strlen(tmpStr);
	  //LCD_Printf(&dc, ccsrch_buffer);

	  // If necessary, fill the rest of the screen (at the bottom) with the background colour:
	  //LCD_FillRect(0, dc.y, LCD_SCREEN_WIDTH - 1, LCD_SCREEN_HEIGHT - 1, dc.bg_color);
	  
  }

  pMenu->redraw = FALSE;    // "done" (screen has been redrawn)
} // HexMon_Draw()


//---------------------------------------------------------------------------
int am_cbk_CCSrch(app_menu_t *pMenu, menu_item_t *pItem, int event, int param )
  // Callback function, invoked from the "app menu" framework .
{
  int n;
  uint16_t two_bytes;
  uint32_t addr, checksum; 
  switch( event ) // what happened, why did the menu framework call us ?
   { case APPMENU_EVT_ENTER: // the operator finished or aborted editing,

	     //StartStopwatch(&ccsrch_stopwatch);
	     ccsrch_buffer_index = 0;
		 cc = 20;
		 fMatch = 0;
         return AM_RESULT_OCCUPY_SCREEN;
         // end if < FINISHED (not ABORTED) editing >
     case APPMENU_EVT_PAINT : // paint into the framebuffer here ?
        if( pMenu->visible == APPMENU_USERSCREEN_VISIBLE ) // only if HexMon already 'occupied' the screen !
         { // To minimize QRM from the display cable, only redraw the screen
           // if any of the displayed bytes has been modified.
           // When running at full pace, almost 100 screen updates per second
           // are possible, but you wouldn't hear anyting in FM with the 
           // rubber-duck antenna due to the activity on the 'LCD data bus' !
           // Calculating a checksum causes no QRM because the LCD bus is passive.

         
           if( pMenu->redraw || ReadStopwatch_ms(&ccsrch_stopwatch) > 1000)
            { 
			   pMenu->redraw = FALSE;
			   CCSrch_Draw(pMenu, pItem); 
            }
           return AM_RESULT_OCCUPY_SCREEN; // keep the screen 'occupied'
         }
        break;
     case APPMENU_EVT_KEY : // own keyboard control only if the screen is owned by HexMon : 
        if( pMenu->visible == APPMENU_USERSCREEN_VISIBLE ) // only if HexMon already 'occupied' the screen !
         { switch( (char)param ) // here: message parameter = keyboard code (ASCII)
            {
              case 'M' :  // green "Menu" key : kind of ENTER. But here, EXIT ;)
              case 'B' :  // red "Back"-key : return from this screen.
				 
                 return AM_RESULT_EXIT_AND_RELEASE_SCREEN;
              default:    // Other keys : switch between different editing modes
                  pMenu->redraw = TRUE;
                  
                 break;
            } // end switch < key >
           return AM_RESULT_OCCUPY_SCREEN; // keep the screen 'occupied'
         }
        break;
     default: // all other events are not handled here (let the sender handle them)
        break;
   } // end switch( event )
  pMenu->redraw = TRUE;
  if (pMenu->visible == APPMENU_USERSCREEN_VISIBLE) // only if HexMon already 'occupied' the screen !
  {
	  return AM_RESULT_OCCUPY_SCREEN; // keep the screen 'occupied' 
  }
  else
  {
	  return AM_RESULT_NONE; // "proceed as if there was NO callback function"
  }
} // end am_cbk_HexMon()


#endif // CONFIG_APP_MENU ?
