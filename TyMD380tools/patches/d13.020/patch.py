#!/usr/bin/env python2
# -*- coding: utf-8 -*-

# Vocoder Patch for MD380 Firmware
# Applies to version D013.020

from Patcher import Patcher

# Match all public calls.
monitormode = False
# Match all private calls.
monitormodeprivate = True

if __name__ == '__main__':
    print("Creating patches from unwrapped.img.")
    patcher = Patcher("unwrapped.img")
	
	#test gps
    #patcher.nopout((0x800C278 + 0))
    #patcher.nopout((0x800C278 + 2))

    # bypass vocoder copy protection on D013.020
	
	#test manual dial group callable
    #patcher.sethword(0x08023170, 0x2204)
    #patcher.sethword(0x08012912, 0x2804)
    #patcher.sethword(0x0801290A, 0x221C)
	
	
	

    patcher.nopout((0x08033f30 + 0x18))
    patcher.nopout((0x08033f30 + 0x1a))
    patcher.nopout((0x08033f30 + 0x2e))
    patcher.nopout((0x08033f30 + 0x30))
    patcher.nopout((0x08033f30 + 0x44))
    patcher.nopout((0x08033f30 + 0x46))
    patcher.nopout((0x08033f30 + 0x5a))
    patcher.nopout((0x08033f30 + 0x5c))
    patcher.nopout((0x08033f30 + 0x70))
    patcher.nopout((0x08033f30 + 0x72))
    patcher.nopout((0x08033f30 + 0x86))
    patcher.nopout((0x08033f30 + 0x88))
    patcher.nopout((0x0804915c + 0x12))
    patcher.nopout((0x0804915c + 0x14))
    patcher.nopout((0x080499e2 + 0x12))
    patcher.nopout((0x080499e2 + 0x14))
    patcher.nopout((0x08049ca8 + 0x10))
    patcher.nopout((0x08049ca8 + 0x12))
    patcher.nopout((0x0804a134 + 0x10))
    patcher.nopout((0x0804a134 + 0x12))
    patcher.nopout((0x0804abc0 + 0x10))
    patcher.nopout((0x0804abc0 + 0x12))

    patcher.nopout((0x080204CC))
    patcher.nopout((0x080204CC + 0x2))
    patcher.nopout((0x080204CC + 0x4))
    patcher.nopout((0x080204CC + 0x6))
    patcher.nopout((0x080204CC + 0x8))
    patcher.nopout((0x080204CC + 0xA))
    # Color Code check
	
	#test
    
    #remove password for program radio
    patcher.nopout((0x080136E0))
    patcher.nopout((0x080136E0 + 0x2))
    
    #patcher.nopout((0x0802D580))
    #patcher.nopout((0x0802D580 + 0x2))
    #patcher.nopout((0x0802D59A))
    #patcher.nopout((0x0802D59A + 0x2))
	
    #patcher.nopout((0x0802D6D2))
    #patcher.nopout((0x0802D6D2 + 0x2))
	
    #patcher.nopout((0x0802D6D8))
    #patcher.nopout((0x0802D6D8 + 0x2))
	
    #patcher.nopout((0x0802D6E0))
    #patcher.nopout((0x0802D6E0 + 0x2))
	
    #patcher.nopout((0x0802D6F2))
    #patcher.nopout((0x0802D6F2 + 0x2))
	
    #patcher.nopout((0x0802D70E))
    #patcher.nopout((0x0802D70E + 0x2))
	
    #patcher.nopout((0x0802D7B2))
    #patcher.nopout((0x0802D7B2 + 0x2))
	
    #patcher.nopout((0x0802D7C6))
    #patcher.nopout((0x0802D7C6 + 0x2))
	
	
    #patcher.nopout((0x0802D6BE))
    #patcher.nopout((0x0802D6BE + 0x2))
	
    #patcher.nopout((0x0802D6B2))
    #patcher.nopout((0x0802D6B2 + 0x2))
	
    #patcher.nopout((0x0802D6A6))
    #patcher.nopout((0x0802D6A6 + 0x2))
	
    #patcher.nopout((0x0802D69A))
    #patcher.nopout((0x0802D69A + 0x2))
	
    #patcher.nopout((0x0802D682))
    #patcher.nopout((0x0802D682 + 0x2))
	
	# remove volume screen
    patcher.nopout((0x0801FED2))
    patcher.nopout((0x0801FED2 + 0x2))

    # freeing ~200k for code patches
    patcher.ffrange(0x0809aee8, 0x080cf754)

    # This mirrors the RESET vector to 0x080C020, for use in booting.
    patcher.setword(0x0800C020,
                    patcher.getword(0x0800C004),
                    0x00000000)

    # This makes RESET point to our stub below.
    patcher.setword(0x0800C004,
                    0x0809af00 + 1)

    # This stub calls the target RESET vector,
    # if it's not FFFFFFFF.
    patcher.sethword(0x0809af00, 0x4840)
    patcher.sethword(0x0809af02, 0x2100)
    patcher.sethword(0x0809af04, 0x3901)
    patcher.sethword(0x0809af06, 0x4508)
    patcher.sethword(0x0809af08, 0xd100)
    patcher.sethword(0x0809af0a, 0x483c)
    patcher.sethword(0x0809af0c, 0x4700)

    # Stores the RESET handler for our stub.
    patcher.setword(0x0809affc,
                    patcher.getword(0x0800C020),
                    0xFFFFFFFF)

    # Marks the version as "md380tools"
    patcher.setwstring(0x080cfcac,
                       "MD380Tools Ver.")

    # Change the manufacturer string.
    patcher.setstring(0x080f86c8,
                      "Travis Goodspeed KK4VCZ")
    # Change the device name.
    patcher.setstring(0x080cfff4,
                      "Patched MD380")

    patcher.export("patched.img")
