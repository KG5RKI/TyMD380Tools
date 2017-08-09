/*
 *  keyb.c
 * 
 */

#include "config.h"


#include "md380.h"
#include "debug.h"
#include "netmon.h"
//#include "mbox.h"
#include "console.h"
#include "syslog.h"
#include "lastheard.h"
#include "radio_config.h"
//#include "sms.h"
//#include "beep.h"
#include "codeplug.h"
#include "radiostate.h"
#include "printf.h"
#include "menu.h" // create_menu_entry_set_tg_screen() called from keyb.c !

#include <stdint.h>
#include "spiflash.h"
#include "keyb.h"



uint8_t kb_backlight=0; // flag to disable backlight via sidekey.
// Other keyboard-related variables belong to the original firmware,
// e.g. kb_keypressed, address defined in symbols_d13.020 (etc).

int Menu_IsVisible() { return 0; }

// Values for kp
// 1 = pressed
// 2 = release within timeout
// 1+2 = pressed during rx
// 4+1 = pressed timeout
// 8 = rearm
// 0 = none pressed
inline int get_main_mode()
{
    return gui_opmode1 & 0x7F ;
}

void reset_backlight()
{
	// struct @ 0x2001dadc
	backlight_timer = 5 * 500;

	// enabling backlight again.
	void(*f)(uint32_t, uint32_t) = (void*)(0x080374CE + 1); // S: ??? 0x0802BAE6
	f(0x40021000, 0x10);
}

void copy_dst_to_contact()
{

	int dst = rst_dst;
	extern wchar_t channel_name[];

	extern int adhocTG;

	contact.id_l = dst & 0xFF;
	contact.id_m = (dst >> 8) & 0xFF;
	contact.id_h = (dst >> 16) & 0xFF;

	contact2.id_l = dst & 0xFF;
	contact2.id_m = (dst >> 8) & 0xFF;
	contact2.id_h = (dst >> 16) & 0xFF;

	adhocTG = dst;
	syslog_printf("Set AdhocTG: %d\r\n", adhocTG);

	//wchar_t *p = (void*)0x2001e1f4 ;
	wchar_t *p = (void*)channel_name;

	if (rst_grp) {
		contact.type = CONTACT_GROUP;
		contact2.type = CONTACT_GROUP;
		snprintfw(p, 16, "TG %d", dst);
	}
	else {
		snprintfw(p, 16, "U %d", dst);
		contact.type = CONTACT_USER;
		contact2.type = CONTACT_USER;
	}
}

int beep_event_probe = 0 ;

void switch_to_screen( int scr )
{
	nm_screen = scr;

    // cause transient -> switch back to idle screen.
    gui_opmode2 = OPM2_MENU ;
    gui_opmode1 = SCR_MODE_IDLE | 0x80 ;
}



//#if defined(FW_D13_020) || defined(FW_S13_020)
#if defined(FW_S13_020)
extern void gui_control( int r0 );
#else
#define gui_control(x)
#endif

extern void draw_zone_channel(); // TODO.

void handle_hotkey( int keycode )
{
	char lat[23] = { 0 };
	char lng[23] = { 0 };
    reset_backlight();
	

    switch( keycode ) {
		case 0 :
			clog_redraw();
            switch_to_screen(6);
            break ;

		case 1:
			//draw_zone_channel();
			break;

		case 2 :
			slog_redraw();
            switch_to_screen(5);
            break ;
        case 3 :
            copy_dst_to_contact();
			//switch_to_screen(9);
            break ;
        case 4 :
            lastheard_redraw();
            switch_to_screen(4);
            break ;
        case 5 :
            syslog_clear();
			lastheard_clear();
			slog_clear();
			clog_clear();
			slog_redraw();
			nm_started = 0;	// reset nm_start flag used for some display handling
			nm_started5 = 0;	// reset nm_start flag used for some display handling
			nm_started6 = 0;	// reset nm_start flag used for some display handling
            break ;
        case 6 :
        {
            static int cnt = 0 ;
            syslog_printf("=dump %d=\n",cnt++);
        }
            syslog_dump_dmesg();
            break ;
		case 13 : //end call
            //bp_send_beep(BEEP_TEST_1);
			if(nm_screen){
				//channel_num = 0;
				switch_to_screen(0);
				//if(Menu_IsVisible()){
				//	channel_num = 0;
				//}
			}else if(!Menu_IsVisible()){
				switch_to_screen(9);
				switch_to_screen(0);
			}
            break ;
		case 7 :
			//Let 7 disable ad-hoc tg mode;
			if (!nm_screen && !Menu_IsVisible()) {
				//ad_hoc_tg_channel = 0;
			}
			if(nm_screen){
				//bp_send_beep(BEEP_TEST_1);
				switch_to_screen(0);
				//channel_num = 0;
			}else if(!Menu_IsVisible()){
				switch_to_screen(9);
				switch_to_screen(0);
			}
			
			break;
        case 8 :
            //bp_send_beep(BEEP_TEST_2);
            switch_to_screen(1);
            break ;
        case 9 :
            //bp_send_beep(BEEP_TEST_3);
			syslog_redraw();
			switch_to_screen(3);
			//switch_to_screen(2);
            break ;
        case 11 :
            //gui_control(1);
            //bp_send_beep(BEEP_9);
            //beep_event_probe++ ;
            //sms_test2(beep_event_probe);
            //mb_send_beep(beep_event_probe);
            break ;
        case 12 :
            //gui_control(241);
            //bp_send_beep(BEEP_25);
            //beep_event_probe-- ;
            //sms_test2(beep_event_probe);
            //mb_send_beep(beep_event_probe);
            break ;
		case 14 :
			switch_to_screen(9);
			//channel_num=0;
			//draw_rx_screen(0xff8032);
			break;
			
			// key '#'
        case 15 :
			if( !Menu_IsVisible() && nm_screen){
				syslog_redraw();
				switch_to_screen(3);
			}
            break ;
    }    
}



void trace_keyb(int sw)
{
    static uint8_t old_kp = -1 ;
    uint8_t kp = kb_keypressed ;
    
    if( old_kp != kp ) {
        //LOGB("kp: %d %02x -> %02x (%04x) (%d)\n", sw, old_kp, kp, kb_row_col_pressed, kb_keycode );
        old_kp = kp ;
    }
}

int is_intercept_allowed()
{
    if( !is_netmon_enabled() || Menu_IsVisible()) {
        return 0 ;
    }
    
//    switch( get_main_mode() ) {
//        case 28 :
//            return 1 ;
//        default:
//            return 0 ;
//    }
    
    switch( gui_opmode2 ) {
        case OPM2_MENU :
            return 0 ;
        //case 2 : // voice
        //case 4 : // terminate voice
        //    return 1 ;
        default:
            return 1 ;
    }
}

int is_intercepted_keycode( int kc )
{
    switch( kc ) {
		//case 0 :
        case 1 :
		case 2 :
        case 3 :
        case 4 :
        case 5 :
        case 6 :
        case 7 :
        case 8 :
        case 9 :
        //case 11 :
        //case 12 :
		//case 13 : //end call
		//case 14 : // *
        //case 15 :
            return 1 ;
        default:
            return 0 ;
    }    
}

int is_intercepted_keycode2(int kc)
{
	switch (kc) {
	
	case 20:
	case 21:
	case 13: //end call
		return 1;
	default:
		return 0;
	}
}

extern void kb_handler();

static int nextKey = -1;

void kb_handle(int key) {
	int kp = kb_keypressed;
	int kc = key;

	if (is_intercept_allowed()) {
		if (is_intercepted_keycode2(kc)) {
			//if ((kp & 2) == 2) {
				//kb_keypressed = 8;
				handle_hotkey(kc);
				return;
			//}
		}
	}

	if (key == 11 || key == 12) {
		kb_keycode = key;
		kb_keypressed = 2;
	}

}

void kb_handler_hook()
{

    trace_keyb(0);

    kb_handler();

    trace_keyb(1);

	//Menu_OnKey(KeyRowColToASCII(kb_row_col_pressed));

	

	if (nextKey > 0) {
		kb_keypressed = 2;
		kb_keycode = nextKey;
		nextKey = -1;
	}
    
    int kp = kb_keypressed ;
    int kc = kb_keycode ;

	/*if (kc == 20 || kc == 21) {
		kb_keypressed = 8;
		return;
	}*/

    // allow calling of menu during qso.
    // not working correctly.
	//if (kc == 3) {
	//	copy_dst_to_contact();
	//}
	//copy_dst_to_contact();
    //if( is_intercept_allowed() ) 
	{
        if( is_intercepted_keycode(kc) ) {
            if( (kp & 2) == 2 ) {
                kb_keypressed = 8 ;
                handle_hotkey(kc);
                return ;
            }
        }
    }

   /* if ( kc == 17 || kc == 18 ) {
      if ( (kp & 2) == 2 || kp == 5 ) { // The reason for the bitwise AND is that kp can be 2 or 3
        //handle_sidekey(kc, kp);         // A 2 means it was pressed while radio is idle, 3 means the radio was receiving
        return;
      }
    }*/



}
