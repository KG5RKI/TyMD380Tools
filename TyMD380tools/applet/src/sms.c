/*
 *  sms.c
 * 
 */

#include <string.h>

#include "sms.h"

#include "md380.h"
#include "syslog.h"
#include "printf.h"
#include "codeplug.h"

OS_EVENT *mbox_msg = (OS_EVENT *)0x20017438 ;

inline int is_fm()
{
#if defined(FW_D13_020)        
    int m = current_channel_info.mode & 0x3 ;
    if( m == 1 ) {
        return 1 ;
    } else {
        return 0 ;
    }
#else
    return 0 ;
#endif    
}

void sms_test()
{
    sms_hdr_t hdr ;    
    sms_bdy_t bdy ;
  
#if 0
    // 3142223
    hdr.dstadr[0] = 0x4F ;
    hdr.dstadr[1] = 0xF2 ;
    hdr.dstadr[2] = 0x2F ;
#endif    
#if 0      
    // all
    hdr.dstadr[0] = 0xFF ;
    hdr.dstadr[1] = 0xFF ;
    hdr.dstadr[2] = 0xFF ;
#endif    
#if 1      
    // Null
    hdr.dstadr[0] = 0x0 ;
    hdr.dstadr[1] = 0x0 ;
    hdr.dstadr[2] = 0x0 ;
#endif    
    
    hdr.flags = SMS_TYPE_SINGLECAST ;
    //hdr.flags = SMS_TYPE_MULTICAST ;
    
    snprintfw(bdy.txt,144,"md380toolz TyFW");
    
    sms_send( &hdr, &bdy );
}

void sms_test2(int m)
{
    LOGR("sms_test2 %d\n", m);
    sms_hdr_t hdr ;    
    sms_bdy_t bdy ;
  
#if 0    
    // 204
    hdr.dstadr[0] = 0xCC ;
    hdr.dstadr[1] = 0x00 ;
    hdr.dstadr[2] = 0x00 ;
#endif    
#if 1      
    // all
    hdr.dstadr[0] = 0xFF ;
    hdr.dstadr[1] = 0xFF ;
    hdr.dstadr[2] = 0xFF ;
#endif    
    
    hdr.flags = m ; 
    
    snprintfw(bdy.txt,144,"md380tools rulez");
        
    sms_send( &hdr, &bdy );
}

int msg_event ; // must be global.

void sms_send( sms_hdr_t *hdr, sms_bdy_t *body )
{
#if defined(FW_D13_020)        
    if( is_fm() ) {
        // TODO: beep
        return ;
    }
    
    sms_hdr_t *hp = (void*)0x2001e1d0 ;
    sms_bdy_t *bp = (void*)0x2001cefc ;
    
    memcpy(hp,hdr,sizeof(sms_hdr_t));
    memcpy(bp,body,sizeof(sms_bdy_t));
    
    msg_event = 3 ;
    md380_OSMboxPost(mbox_msg, &msg_event);    
#endif    
}
