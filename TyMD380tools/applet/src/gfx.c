/*! \file gfx.c
  \brief Graphics wrapper functions.
*/

#define DEBUG

#include "md380.h"
#include "version.h"
#include "tooldfu.h"
#include "config.h"
#include "gfx.h"
#include "printf.h"
#include "string.h"
#include "addl_config.h"
//#include "display.h"
#include "console.h"
#include "netmon.h"
#include "debug.h"
#include "usersdb.h"
#include "lastheard.h" // reduce number of warnings - use function prototypes
//#include "app_menu.h" // optional 'application' menu, activated by red BACK-button
          // When visible, some gfx-calls must be disabled via our hooks
          // to prevent interference from Tytera's "gfx" (similar as for Netmon)
#include "radiostate.h"

// Needed for LED functions.  Cut dependency.
#include "stm32f4_discovery.h"
#include "stm32f4xx_conf.h" // again, added because ST didn't put it here ?


#define text_height 16
uint8_t GFX_backlight_on=0; // DL4YHF 2017-01-07 : 0="off" (low intensity), 1="on" (high intensity)
                            //   (note: GFX_backlight_on is useless as long as no-one calls lcd_background_led() .
                            //    As long as that's the case, the 'dimmed backlight switcher' 
                            //    in applet/src/irq_handlers.c polls backlight_timer instead of GFX_backlight_on . )

//! Draws text at an address by calling back to the MD380 function.

void swapFGBG() {
	/*uint16_t fg_color = 0, bg_color = 0;
	if (global_addl_config.alt_text) {
		Menu_GetColours(SEL_FLAG_NONE, &fg_color, &bg_color);
		bg_color = rgb16torgb(bg_color);
		fg_color = rgb16torgb(fg_color);
		gfx_set_bg_color(bg_color);
		gfx_set_fg_color(fg_color);
	}*/
}
void swapBG() {
	/*uint16_t fg_color = 0, bg_color = 0;
	if (global_addl_config.alt_text) {
		Menu_GetColours(SEL_FLAG_NONE, &fg_color, &bg_color);
		bg_color = rgb16torgb(bg_color);
		fg_color = rgb16torgb(fg_color);
		gfx_set_bg_color(bg_color);
	}*/
}

void drawtext(wchar_t *text, int x, int y)
{
    //#ifdef CONFIG_GRAPHICS
	
	swapFGBG();
	gfx_drawtext(text, 0, 0, x, y, 15); //strlen(text));
    //#endif
}

//! Draws text at an address by calling back to the MD380 function.

void drawascii(char *ascii, int x, int y)
{
    //Widen the string.  We really ought to do proper lengths.
    wchar_t wide[15];
    for (int i = 0; i < 15; i++)
        wide[i] = ascii[i];

	swapFGBG();
    //#ifdef CONFIG_GRAPHICS
    //Draw the wide string, not the original.
	gfx_drawtext(wide, 0, 0, x, y, 15); //strlen(text));
    //#endif
}

//// TODO: how does this differ from drawascii?
//
//void drawascii2(char *ascii, int x, int y)
//{
//    wchar_t wide[40];
//
//    for (int i = 0; i < 25; i++) {
//        wide[i] = ascii[i];
//        if( ascii[i] == '\0' )
//            break;
//    }
//    //#ifdef CONFIG_GRAPHICS
//    gfx_drawtext2(wide, x, y, 0);
//    //#endif
//}

void green_led(int on) {
  if (on) {
    GPIO_SetBits(GPIOE, GPIO_Pin_0);
  } else {
    GPIO_ResetBits(GPIOE, GPIO_Pin_0);
  }
}



void rx_screen_blue_hook(char *bmp, int x, int y)
{
	user_t usr;

	int y_index = 22;

	usr_find_by_dmrid(&usr, rst_src);
	gfx_set_fg_color(0x00FF00);
	gfx_blockfill(0, 16, 159, 127);
	gfx_set_bg_color(0x00FF00);
	gfx_set_fg_color(0x000000);

	gfx_printf_pos(2, y_index, "%s - %s", usr.callsign, usr.name);
	y_index += text_height;
	gfx_printf_pos(2, y_index, "%s, %s", usr.place, usr.state);
	y_index += text_height * 2;
	gfx_printf_pos(2, y_index, "%s", usr.country);
	y_index += text_height;
	//gfx_printf_pos(2, y_index, "%s", usr.);
	//y_index += text_height;

	return;
}

void red_led(int on) {
  /* The RED LED is supposed to be on pin A0 by the schematic, but in
     point of fact it's on E1.  Expect more hassles like this.
  */

  if (on) {
    GPIO_SetBits(GPIOE, GPIO_Pin_1);
  } else {
    GPIO_ResetBits(GPIOE, GPIO_Pin_1);
  }
}

void lcd_background_led(int on) 
{ // 2017-01-07 : Never called / no effect ? The backlight seems to be controlled "by Tytera only" (backlight_timer).

#if( CONFIG_DIMMED_LIGHT ) 
  GFX_backlight_on = on; // DL4YHF 2017-01-07.  Tried to poll this in irq_handlers.c, but didn't work.
                         // Poll Tytera's 'backlight_timer' instead. Nonzero="bright", zero="dark" . 
                         // With CONFIG_DIMMED_LIGHT=1, the "Lamp" output (PC6) is usually configured 
                         // as UART6_TX, and switching it 'as GPIO' has no effect then.                         
#else // ! CONFIG_DIMMED_LIGHT : only completely on or off ...
 
  if (on) 
   { GPIO_SetBits(GPIOC, GPIO_Pin_6);
   } 
  else 
   { GPIO_ResetBits(GPIOC, GPIO_Pin_6);
   }
#endif // CONFIG_DIMMED_LIGHT ?   
}

/*
void dump_ram_to_spi_flash() {
  static int run = 0;
  if ( run == 100) {
    printf("dump\n");
    for ( int i=0; i < (112+16); i++) {
      md380_spiflash_write((void *) 0x20000000+(1024*i), 0x400000+(1024*i), 1024);
    }
  }
  run++;
}

*/
/**
 * this hooks on a gfx_drawtext2 call.
 */
void print_date_hook(void)
{ // copy from the md380 code

# if (CONFIG_APP_MENU)
   //if( Menu_IsVisible() ) // 'app menu' visible ? Don't allow Tytera to print into the framebuffer !
   // { return; 
   // }
# endif
    if( is_netmon_visible() ) {
        return;
    }

	/*if (global_addl_config.alt_text) {
		uint16_t fg_color = 0, bg_color = 0;
		Menu_GetColours(SEL_FLAG_NONE, &fg_color, &bg_color);
		bg_color = rgb16torgb(bg_color);
		fg_color = rgb16torgb(fg_color);
		gfx_set_bg_color(bg_color);
		gfx_set_fg_color(fg_color);
	}*/

#ifdef CONFIG_GRAPHICS
    wchar_t wide[40];
    RTC_DateTypeDef RTC_DateStruct;
    md380_RTC_GetDate(RTC_Format_BCD, &RTC_DateStruct);

    switch (0) {
        default:
            // fallthrough
        case 0:
            wide[0] = '2';
            wide[1] = '0';
            md380_itow(&wide[2], RTC_DateStruct.RTC_Year);
            wide[4] = '/';
            md380_itow(&wide[5], RTC_DateStruct.RTC_Month);
            wide[7] = '/';
            md380_itow(&wide[8], RTC_DateStruct.RTC_Date);
            break;
        case 1:
            md380_itow(&wide[0], RTC_DateStruct.RTC_Date);
            wide[2] = '.';
            md380_itow(&wide[3], RTC_DateStruct.RTC_Month);
            wide[5] = '.';
            wide[6] = '2';
            wide[7] = '0';
            md380_itow(&wide[8], RTC_DateStruct.RTC_Year);
            break;
        case 2:
            md380_itow(&wide[0], RTC_DateStruct.RTC_Date);
            wide[2] = '/';
            md380_itow(&wide[3], RTC_DateStruct.RTC_Month);
            wide[5] = '/';
            wide[6] = '2';
            wide[7] = '0';
            md380_itow(&wide[8], RTC_DateStruct.RTC_Year);
            break;
        case 3:
            md380_itow(&wide[0], RTC_DateStruct.RTC_Month);
            wide[2] = '/';
            md380_itow(&wide[3], RTC_DateStruct.RTC_Date);
            wide[5] = '/';
            wide[6] = '2';
            wide[7] = '0';
            md380_itow(&wide[8], RTC_DateStruct.RTC_Year);
            break;
        case 4:
            wide[0] = '2';
            wide[1] = '0';
            md380_itow(&wide[2], RTC_DateStruct.RTC_Year);
            wide[4] = '-';
            md380_itow(&wide[5], RTC_DateStruct.RTC_Month);
            wide[7] = '-';
            md380_itow(&wide[8], RTC_DateStruct.RTC_Date);
            break;
    }

    wide[10] = '\0';

	swapFGBG();

    gfx_drawtext2(wide, 0xa, 0x60, 0x5e);
#endif //CONFIG_GRAPHICS
}

void get_RTC_time(char* buffer) {

	wchar_t wide_time[9];

	RTC_TimeTypeDef RTC_TimeStruct;
	md380_RTC_GetTime(RTC_Format_BCD, &RTC_TimeStruct);

	md380_itow(&wide_time[0], RTC_TimeStruct.RTC_Hours);
	wide_time[2] = ':';
	md380_itow(&wide_time[3], RTC_TimeStruct.RTC_Minutes);
	wide_time[5] = ':';
	md380_itow(&wide_time[6], RTC_TimeStruct.RTC_Seconds);
	wide_time[8] = '\0';

	int b = 0;
	for (int i = 0; i < 9; i++)
	{
		if (wide_time[i] == '\0')
			break;
		
		*buffer = wide_time[i];
		buffer++;
	}
}

void print_time_hook(const char log) // ex: void print_time_hook(void), 'log' used by netmon.c
{
    if( is_netmon_visible() ) {
        return;
    }
    wchar_t wide_time[9];

	swapFGBG();

    RTC_TimeTypeDef RTC_TimeStruct;
    md380_RTC_GetTime(RTC_Format_BCD, &RTC_TimeStruct);

    md380_itow(&wide_time[0], RTC_TimeStruct.RTC_Hours);
    wide_time[2] = ':';
    md380_itow(&wide_time[3], RTC_TimeStruct.RTC_Minutes);
    wide_time[5] = ':';
    md380_itow(&wide_time[6], RTC_TimeStruct.RTC_Seconds);
    wide_time[8] = '\0';
 
    for (int i = 0; i < 9; i++) 
     {
        if( wide_time[i] == '\0' )
            break;
        if ( log == 'l' ) {
           lastheard_putch(wide_time[i]);
           // "warning: implicit declaration of function 'lastheard_putch'" - added prototype in *.h
        } else if ( log == 'c' ) {
           clog_putch(wide_time[i]);
        } else if ( log == 's' ) {
           slog_putch(wide_time[i]);
        }
    }
} 

// deprecated, left for other versions.
void print_ant_sym_hook(char *bmp, int x, int y)
{
# if (CONFIG_APP_MENU)
   // if( Menu_IsVisible())  // If the 'app menu' is visible,
   //  { return; // then don't allow Tytera's "gfx" to spoil the framebuffer
   //  }
# endif

    if( is_netmon_visible() ) {
        return ;
    }

	//swapFGBG();

#ifdef CONFIG_GRAPHICS
    gfx_drawbmp(bmp, x, y);
    //draw_eye_opt();
#endif
}

/**
 * 
 * @param x_from 0...159
 * @param y_from 0...127
 * @param x_to   0...159
 * @param y_to   0...127
 */

void gfx_blockfill_hook(int x_from, int y_from, int x_to, int y_to)
{
//    if( ymin == 0 && xmin == 61 ) {
//        if( global_addl_config.promtg ) {
//            return ;
//        }
//    }
    
//    PRINTRET();
//    PRINT( "bf: %d %d %d %d\n", x_from, y_from, x_to, y_to );
    
# if (CONFIG_APP_MENU)
  // if( Menu_IsVisible() )  // If the 'app menu' is visible,
  //  { return; // then don't allow Tytera's "gfx" to spoil the framebuffer
  //  }
# endif


    if( y_from == 0 && x_from == 61 ) {
        con_redraw();
    }
    if( is_netmon_visible() ) {
        // no blockfills
        return ;
    }

	if (gui_opmode2 != OPM2_MENU) {
		swapFGBG();
	}
    
    gfx_blockfill(x_from,y_from,x_to,y_to);
    
    if( y_from == 0 && x_from == 61 ) {
        // if we have stat var for detecting first draw....
        // we could clear by blockfill only once.
        //if( global_addl_config.promtg ) {
        //    draw_eye_opt();
        //}
    }
}

void gfx_drawbmp_hook( void *bmp, int x, int y )
{
//    PRINTRET();
//    PRINT( "db: %d %d\n", x, y );
    
# if (CONFIG_APP_MENU)
  // if( Menu_IsVisible() )  // If the 'app menu' is visible,
  //  { return; // don't allow Tytera's "gfx" to spoil the framebuffer
  //  }
# endif

    swapFGBG();

    // supress bmp drawing in console mode.
    if( is_netmon_visible() ) {
        if( x == D_ICON_ANT_X && y == D_ICON_ANT_Y ) {
            // antenne icon draw.
            con_redraw();
        }
        return ;
    }
    gfx_drawbmp( bmp, x, y );
    // redraw promiscous mode eye icon overlapped by antenna icon
    //if(  ( global_addl_config.promtg ) && ( y == 0 )) 
	{
        //draw_eye_opt();
    }
}

// r0 = str, r1 = x, r2 = y, r3 = xlen
void gfx_drawtext2_hook(wchar_t *str, int x, int y, int xlen)
{
# if (CONFIG_APP_MENU)
  // if( Menu_IsVisible() )  // If the 'app menu' is visible,
  //  { return; // don't allow Tytera's "gfx" to spoil the framebuffer
  //  }
# endif



    // filter datetime (y=96)
    if( y != D_DATETIME_Y ) {
        //PRINTRET();
        //printf("ctd: %d %d %d %S\n", x, y, xlen, str);
    }
    
    if( is_netmon_visible() ) {
        return ;
    }
    
	swapFGBG();

    gfx_drawtext2(str, x, y, xlen);
}

void gfx_drawtext4_hook(wchar_t *str, int x, int y, int xlen, int ylen)
{
   // PRINTRET();
    //printf("dt4: %d %d %d %d %S (%x)\n", x, y, xlen, ylen, str, str);

# if (CONFIG_APP_MENU)
  // if( Menu_IsVisible())  // If the 'app menu' is visible,
  //  { return; // then don't allow Tytera's "gfx" to spoil the framebuffer
  //  }
# endif

    
    if( is_netmon_visible() ) {
        // channel name
        if( x == D_TEXT_CHANNAME_X && y == D_TEXT_CHANNAME_Y ) {
            return ;
        }
        // zonename
        if( x == D_TEXT_ZONENAME_X && y == D_TEXT_ZONENAME_Y ) {
            return ;
        }
    }

	swapFGBG();
    
    gfx_drawtext4(str,x,y,xlen,ylen);

}

extern void gfx_drawchar_pos( int r0, int r1, int r2 );

void gfx_drawchar_pos_hook( int r0, int r1, int r2 )
{

# if (CONFIG_APP_MENU)
  // if( Menu_IsVisible())  // If the 'app menu' is visible,
  //  { return; // don't allow Tytera's "gfx" to spoil the framebuffer
      // (must get rid of all this crazy hooking one fine day)
  //  }
# endif

    if( is_netmon_visible() ) {
        return ;
    }

	swapFGBG();

    gfx_drawchar_pos(r0,r1,r2);

}



// the intention is a string _without_ .. if it is too long.
// and that it only fills background when a char or space is printed.
void gfx_puts_pos(int x, int y, const char *str)
{
	swapFGBG();

    // TODO: optimize, it is not very bad, comparing this with a char to wide expansion.
    gfx_printf_pos(x,y,"%s",str);
}

// the intention is a string _without_ .. if it is too long.
// and that it only fills background when a char or space is printed.
// and that it only fills background when a char or space is printed.
void gfx_printf_pos(int x, int y, const char *fmt, ...)
{

	char buf[50];

	va_list va;
	va_start(va, fmt);

	//swapFGBG();

	va_snprintf(buf, 50, fmt, va);
	gfx_drawtext7(buf, x, y);
	gfx_clear3(0);

	va_end(va);

}
/*
unsigned int rgb(double hue) {
int h = int(hue * 256 * 6);
int x = h % 0x100;

int r = 0, g = 0, b = 0;
switch (h / 256) {
case 0: r = 255; g = x;       break;
case 1: g = 255; r = 255 - x; break;
case 2: g = 255; b = x;       break;
case 3: b = 255; g = 255 - x; break;
case 4: b = 255; r = x;       break;
case 5: r = 255; b = 255 - x; break;
}

return r + (g << 8) + (b << 16);
}
unsigned int color = rgb(fValue / 500.0f);
*/


// the intention is a string shortened with .. if it is too long.
// and that it fills all background
void gfx_printf_pos2(int x, int y, int xlen, const char *fmt, ...)
{
    char buf[50];
    
    va_list va;
    va_start(va, fmt);

	//swapFGBG();
    
    va_snprintf(buf, 50, fmt, va );
    gfx_drawtext7(buf,x,y);
    gfx_clear3( xlen );
    
    va_end(va);        
    
}

void draw_statusline_hook(uint32_t r0)
{

# if (CONFIG_APP_MENU)
	// If the screen is occupied by the optional 'red button menu', 
	// update or even redraw it completely:

	// NOTE: draw_statusline_hook() isn't called when the squelch
	//       is 'open' in FM, i.e. when the channel is BUSY .
	// Of course we don't want to be tyrannized by the radio like that.
	// It's THE OPERATOR'S decision what to do and when to invoke the menu,
	// not the radio's. 
	// Fixed by also calling Menu_DrawIfVisible() from other places .
# endif // CONFIG_APP_MENU ?

	/*if (is_netmon_visible()) {
		con_redraw();
		return;
	}*/
	draw_statusline(r0);
}

void draw_alt_statusline()
{
	int dst;
	int src;
	int grp;
	int fFound = 0;

	gfx_set_fg_color(0);
	gfx_set_bg_color(0xff8032);
	gfx_select_font(gfx_font_small);

	char mode = ' ';
	if (rst_voice_active) {
		if (rst_mycall) {
			mode = '*'; // on my tg            
		}
		else {
			mode = '!'; // on other tg
		}
	}

	user_t usr, usr2;
	src = rst_src;

	//gfx_printf_pos(RX_POPUP_X_START, 96, "%s - %s", usr.callsign, usr.name);

	
	
	if (src == 0) {

		//gfx_printf_pos2(RX_POPUP_X_START, 96, 157, "lh:");
		gfx_printf_pos2(RX_POPUP_X_START, 96, 157, "lh:");
	}
	else {
												// 2017-02-18 otherwise show lastheard in status line

			if (usr_find_by_dmrid(&usr, src) == 0) {
				if (usr_find_by_dmrid(&usr2, rst_dst) == 0) {
					gfx_printf_pos2(RX_POPUP_X_START, 96, 157, "lh:%d->%d %c", src, rst_dst, mode);
				}
				else {
					gfx_printf_pos2(RX_POPUP_X_START, 96, 157, "lh:%d->%s %c", src, usr2.callsign, mode);
				}
			}
			else {

				if (usr_find_by_dmrid(&usr2, rst_dst) == 0) {
					gfx_printf_pos2(RX_POPUP_X_START, 96, 157, "lh:%s->%d %c", usr.callsign, rst_dst, mode);
				}
				else {
					gfx_printf_pos2(RX_POPUP_X_START, 96, 157, "lh:%s->%s %c", usr.callsign, usr2.callsign, mode);
				}
			}
		
	}

	/*gfx_set_fg_color(0);
	gfx_set_bg_color(0xff000000);
	gfx_select_font(gfx_font_norm);*/
}


void draw_datetime_row_hook()
{

	//if (is_netmon_visible()) {
	//	return;
	//}
	/*if (ad_hoc_tg_channel)
	{
		draw_adhoc_statusline();
	}*/
	//if (is_statusline_visible()) 
	{
		draw_alt_statusline();
		//return;
	}
	//draw_datetime_row();
}

