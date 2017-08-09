#! python2.7
# -*- coding: utf-8 -*-


# This script implements our old methods for merging an MD380 firmware
# image with its patches.  It is presently being rewritten to require
# fewer explicit addresses, so that we can target our patches to more
# than one version of the MD380 firmware.

from __future__ import print_function

import sys


class Symbols(object):
    addresses = {}
    names = {}

    def __init__(self, filename):
        print("Loading symbols from %s" % filename)
        fsyms = open(filename)
        for l in fsyms:
            try:
                r = l.strip().split('\t')
                if len(r) == 2 and r[0].split(' ')[7] == '.text':
                    adr = r[0].split(' ')[0].strip()
                    name = r[1].split(' ')[1]  # .strip();
                    # print("%s is at %s" % (name, adr))
                    self.addresses[name] = int(adr, 16)
                    self.names[int(adr, 16)] = name
            except IndexError:
                pass;
    def getadr(self,name):
        return self.addresses[name];
    def try_getadr(self,name): # DL4YHF 2017-01, used to CHECK if a symbol exists
        try:                   # to perform patches for 'optional' C functions 
            return self.addresses[name];
        except KeyError:
            return None;
    def getname(self,adr):
        return self.names[adr];

class Merger(object):
    def __init__(self, filename, offset=0x0800C000):
        """Opens the input file."""
        self.offset = offset
        self.file = open(filename, "rb")
        self.bytes = bytearray(self.file.read())
        self.length = len(self.bytes)

    def setbyte(self, adr, new, old=None):
        """Patches a single byte from the old value to the new value."""
        self.bytes[adr - self.offset] = new

    def getbyte(self, adr):
        """Reads a byte from the firmware address."""
        b = self.bytes[adr - self.offset]
        return b

    def export(self, filename):
        """Exports to a binary file."""
        outfile = open(filename, "wb")
        outfile.write(self.bytes)

    def assertbyte(self, adr, val):
        """Asserts that a byte has a given value."""
        assert self.getbyte(adr) == val
        return

    def getword(self, adr):
        """Reads a byte from the firmware address."""
        w = (
            self.bytes[adr - self.offset] +
            (self.bytes[adr - self.offset + 1] << 8) +
            (self.bytes[adr - self.offset + 2] << 16) +
            (self.bytes[adr - self.offset + 3] << 24)
        )

        return w

    def setword(self, adr, new, old=None):
        """Patches a 32-bit word from the old value to the new value."""
        if old is not None:
            self.assertbyte(adr, old & 0xFF)
            self.assertbyte(adr + 1, (old >> 8) & 0xFF)
            self.assertbyte(adr + 2, (old >> 16) & 0xFF)
            self.assertbyte(adr + 3, (old >> 24) & 0xFF)

        # print("Patching word at %08x to %08x" % (adr, new))
        self.bytes[adr - self.offset] = new & 0xFF
        self.bytes[adr - self.offset + 1] = (new >> 8) & 0xFF
        self.bytes[adr - self.offset + 2] = (new >> 16) & 0xFF
        self.bytes[adr - self.offset + 3] = (new >> 24) & 0xFF
        self.assertbyte(adr, new & 0xFF)
        self.assertbyte(adr + 1, (new >> 8) & 0xFF)

    def sethword(self, adr, new, old=None):
        """Patches a byte pair from the old value to the new value."""
        if old is not None:
            self.assertbyte(adr, old & 0xFF)
            self.assertbyte(adr + 1, (old >> 8) & 0xFF)
        # print("Patching hword at %08x to %04x" % (adr, new))
        self.bytes[adr - self.offset] = new & 0xFF
        self.bytes[adr - self.offset + 1] = (new >> 8) & 0xFF
        self.assertbyte(adr, new & 0xFF)
        self.assertbyte(adr + 1, (new >> 8) & 0xFF)

    def hookstub(self, adr, handler):
        """Hooks a function by placing an unconditional branch at adr to
           handler.  The recipient function must have an identical calling
           convention. """
        adr &= ~1  # Address must be even.
        handler |= 1  # Destination address must be odd.
        # print("Inserting a stub hook at %08x to %08x." % (adr, handler))

        # FIXME This clobbers r0, should use a different register.
        self.sethword(adr, 0x4801)  # ldr r0, [pc, 4]
        self.sethword(adr + 2, 0x4700)  # bx r0
        self.sethword(adr + 4, 0x4600)  # NOP
        self.sethword(adr + 6, 0x4600)  # NOP, might be overwritten
        if adr & 2 > 0:
            self.setword(adr + 6, handler)  # bx r0
        else:
            self.setword(adr + 8, handler)  # bx r0

    def hookstub2(self, adr, handler):
        """Hooks a function by placing an unconditional branch at adr to
           handler.  The recipient function must have an identical calling
           convention. """
        adr &= ~1  # Address must be even.
        handler |= 1  # Destination address must be odd.
        print("Inserting a stub hook at %08x to %08x." % (adr, handler))

        # insert trampoline
        # rasm2 -a arm -b 16 '<asm code>'
        self.sethword(adr, 0xb401)  # push {r0}
        self.sethword(adr + 2, 0xb401)  # push {r0}
        self.sethword(adr + 4, 0x4801)  # ldr r0, [pc, 4]
        self.sethword(adr + 6, 0x9001)  # str r0, [sp, 4] (pc)
        self.sethword(adr + 8, 0xbd01)  # pop {r0,pc}
        self.sethword(adr + 10, 0x4600)  # NOP, might be overwritten
        if adr & 2 > 0:
            self.setword(adr + 10, handler)
        else:
            self.setword(adr + 12, handler)

    def calcbl(self, adr, target):
        """Calculates the Thumb code to branch to a target."""
        offset = target - adr
        # print("offset=%08x" % offset)
        offset -= 4  # PC points to the next ins.
        offset = (offset >> 1)  # LSBit is ignored.
        hi = 0xF000 | ((offset & 0xfff800) >> 11)  # Hi address setter, but at lower adr.
        lo = 0xF800 | (offset & 0x7ff)  # Low adr setter goes next.
        # print("%04x %04x" % (hi, lo))
        word = ((lo << 16) | hi)
        # print("%08x" % word)
        return word

    def hookbl(self, adr, handler, oldhandler=None):
        """Hooks a function by replacing a 32-bit relative BL."""

        # print("Redirecting a bl at %08x to %08x." % (adr, handler))

        # TODO This is sometimes tricked by old data.
        # Fix it by ensuring no old data.
        # if oldhandler!=None:
        #    #Verify the old handler.
        #    if self.calcbl(adr,oldhandler)!=self.getword(adr):
        #        print("The old handler looks wrong.")
        #        print("Damn, we're in a tight spot!")
        #        sys.exit(1);

        self.setword(adr,
                     self.calcbl(adr, handler))


if __name__ == '__main__':
    print("Merging an applet.")
    if len(sys.argv) != 4:
        print("Usage: python merge.py firmware.img patch.img offset")
        sys.exit(1)

    # Open the firmware image.
    merger = Merger(sys.argv[1])

    # Open the applet.
    fapplet = open(sys.argv[2], "rb")
    bapplet = bytearray(fapplet.read())
    index = int(sys.argv[3], 16)

    # Open the applet symbols
    sapplet = Symbols("%s.sym" % sys.argv[2])
	
    #merger.hookstub(0x080E50C6,  # USB manufacturer string handler function.
    #                sapplet.getadr("getmfgstr"))
	
    merger.hookbl(0x08030D76, sapplet.getadr("rx_screen_blue_hook"), 0)
    merger.hookbl(0x08030DF0, sapplet.getadr("rx_screen_blue_hook"), 0)
    merger.hookbl(0x08027F7A, sapplet.getadr("rx_screen_blue_hook"), 0)
	
    merger.hookbl(0x08030D3C, sapplet.getadr("rx_screen_blue_hook"), 0)
    merger.hookbl(0x08030D9A, sapplet.getadr("rx_screen_blue_hook"), 0)
	
    #merger.hookbl(0x080D8A20, sapplet.getadr("usb_upld_hook"), 0x080D94BC)  # Old handler adr.
	
    # keyboard
    merger.hookbl(0x0806CE92, sapplet.getadr("kb_handler_hook"));
	
    #merger.hookbl(0x0800E4B8, sapplet.getadr("print_date_hook"), 0)
   # merger.hookbl(0x08029844, sapplet.getadr("draw_statusline_hook"))
	
    draw_datetime_row_list = [
        0x08028A2C,
        0x08028FE4,
        0x0802904A,
        0x08029142,
        0x080291A6,
        0x080291F8,
        0x0802925E,
        0x0803AFF0,
        0x0803B828,
    ]
    for adr in draw_datetime_row_list:
        merger.hookbl(adr, sapplet.getadr("draw_datetime_row_hook"))
	
    dmr_call_start_hook_list = [0x804C3DC, 0x0804BE34, 0x804BE0C, 0x804BDA6]
    for adr in dmr_call_start_hook_list:
        merger.hookbl(adr, sapplet.getadr("dmr_call_start_hook"))
		
    merger.hookbl(0x0804C3E4, sapplet.getadr("dmr_call_end_hook"))
	
    merger.hookbl(0x0804C3EC, sapplet.getadr("dmr_CSBK_handler_hook"))
	
    merger.hookbl(0x08011444, sapplet.getadr("f_1444_hook"))
	
    dmr_handle_data_hook_list = [0x0804C418, 0x0804C422, 0x0804C480]
    for adr in dmr_handle_data_hook_list:
        merger.hookbl(adr, sapplet.getadr("dmr_handle_data_hook"))
				  
    drwbmplist = [
        0x0800D010,
        0x0800EC48,
        0x0800EC56,
        0x0800EC72,
        0x0800EC80,
        0x0800ECFE,
        0x0800ED0C,
        0x0800ED28,
        0x0800ED36,
        0x0800ED92,
        0x0800EDBE,
        0x0800EDEA,
        0x0800EE16,
        0x0800EE42,
        0x0800EE6E,
        0x0800EF28,
        0x0800EF36,
        0x0800EF86,
        0x0800F000,
        0x0800F00E,
        0x0800F05E,
        0x08027F7A,
        0x08029350,
        0x0802935E,
        0x0802986A,
        0x08029876,
        0x08030D3C,
        0x08030D6A,
        0x08030D76,
        0x08030D9A,
        0x08030DD8,
        0x08030DF0,
        0x0803368C,
        0x080336C2,
        0x080336EC,
        0x08033716,
        0x08033734,
        0x0803380C,
        0x08033854,
        0x08033878,
        0x080338D6,
        0x080338EA,
        0x0803395C,
        0x08033970,
        0x080339A8,
        0x080339E6,
        0x08033A4A,
        0x08033A8C,
        0x08033B28,
        0x080375D6,
        0x08037680,
        0x08037768,
        0x080378CE,
        0x08037962,
        0x08037A7A,
        0x08037B5E,
        0x08037BE8,
        0x08037D3C,
        0x08037D48,
        0x08037E6A,
        0x08037EF6,
        0x08037FE6,
        0x0803804C,
        0x080380CA,
        0x08038C32,
        0x08047300,
        0x08047328,
        0x0804737C,
        0x080473B8,
        0x08053108,
        0x0805313A,
        0x08063F10,
        0x0806A7DC,
    ]
    for adr in drwbmplist:
        merger.hookbl(adr, sapplet.getadr("gfx_drawbmp_hook"))
		
    gfxblockfill = [
        0x0800CAA6,
        0x0800CAB2,
        0x0800CABE,
        0x0800CACA,
        0x0800CADE,
        0x0800CAEA,
        0x0800CAF6,
        0x0800CB02,
        0x0800CB0E,
        0x0800CB28,
        0x0800CB4E,
        0x0800CB5A,
        0x0800CB6E,
        0x0800CC4E,
        0x0800CC5A,
        0x0800CC66,
        0x0800CC78,
        0x0800CC96,
        0x0800CCA2,
        0x0800CCB6,
        0x0800CD4C,
        0x0800CD58,
        0x0800CD64,
        0x0800CDDC,
        0x0800CDE8,
        0x0800CDF4,
        0x0800CE00,
        0x0800CE14,
        0x0800CE20,
        0x0800CE34,
        0x0800CE40,
        0x0800CE54,
        0x0800CECA,
        0x0800D258,
        0x0800D28A,
        0x0800D450,
        0x0800D45C,
        0x0800D4C0,
        0x0800D5EC,
        0x0800D5F8,
        0x0800D604,
        0x0800D610,
        0x0800D722,
        0x0800D72E,
        0x0800D73A,
        0x0800D746,
        0x0800D7FA,
        0x0800D87A,
        0x0800D886,
        0x0800D892,
        0x0800D89E,
        0x0800DB3A,
        0x0800DC82,
        0x0800DC9C,
        0x0800DCA8,
        0x0800DCF8,
        0x0800DD04,
        0x0800DD10,
        0x0800DFF2,
        0x0800E27E,
        0x0800E292,
        0x0800E29E,
        0x0800E2AA,
        0x0800E2B6,
        0x0800E558,
        0x0800E5CE,
        0x0800E5DC,
        0x0800E5E8,
        0x0800E5F4,
        0x0800E608,
        0x0800E6E2,
        0x0800E74C,
        0x0800E774,
        0x0800E90C,
        0x0800E920,
        0x0800E92C,
        0x0800E938,
        0x0800E944,
        0x0800ECBE,
        0x0800ECCC,
        0x0800EE84,
        0x0800EF9C,
        0x0800F074,
        0x0800F0DE,
        0x0800F0EC,
        0x0800F1E4,
        0x0800F2CE,
        0x0800F3C6,
        0x0800F702,
        0x0800F71C,
        0x0800F75E,
        0x0800F76A,
        0x0800F776,
        0x0800F788,
        0x0800F79C,
        0x0800F7A8,
        0x0800F87E,
        0x0800F8A2,
        0x0800F8BC,
        0x0800F8DE,
        0x0800F8F8,
        0x08025D4E,
        0x08025D5C,
        0x08025D6C,
        0x08025EFE,
        0x08025F0E,
        0x08027A04,
        0x08028200,
        0x08028486,
        0x08028C6C,
        0x08028EAE,
        0x08028FC8,
        0x08028FF6,
        0x0802905A,
        0x08029066,
        0x08029072,
        0x0802907E,
        0x0802908A,
        0x08029096,
        0x08029128,
        0x08029154,
        0x080291B8,
        0x0802920A,
        0x0802926E,
        0x0802927A,
        0x08029286,
        0x08029292,
        0x0802929E,
        0x080292AA,
        0x080305A4,
        0x080305B0,
        0x08030642,
        0x080306CC,
        0x0803076E,
        0x08030848,
        0x08030912,
        0x080309F2,
        0x080309FE,
        0x08030A90,
        0x08030A9C,
        0x08030C00,
        0x08030C8A,
        0x08030CA4,
        0x08030D00,
        0x08030D0C,
        0x08030D18,
        0x08030DBC,
        0x08030E0C,
        0x08030E18,
        0x08030EBA,
        0x08036AB8,
        0x08036ACC,
        0x080379F4,
        0x08037B48,
        0x08037E80,
        0x08037F52,
        0x08038040,
        0x080383BC,
        0x08038434,
        0x080387D2,
        0x08039722,
        0x08039734,
        0x08039740,
        0x08039754,
        0x08039760,
        0x0803AE3E,
        0x0803AE4A,
        0x0803AE56,
        0x0803AEA4,
        0x0803AEB0,
        0x0803AEBC,
        0x0803AEC8,
        0x0803AED4,
        0x0803AEE0,
        0x0803AEEC,
        0x0803AFE0,
        0x0803AFEC,
        0x0803B004,
        0x0803B018,
        0x0803B024,
        0x0803B030,
        0x0803B130,
        0x0803B13C,
        0x0803B148,
        0x0803B15C,
        0x0803B168,
        0x0803B174,
        0x0803B180,
        0x0803B18C,
        0x0803B198,
        0x0803B1D6,
        0x0803B1E2,
        0x0803B264,
        0x0803B270,
        0x0803B2A2,
        0x0803B2C2,
        0x0803B2CE,
        0x0803B2FE,
        0x0803B30A,
        0x0803B342,
        0x0803B372,
        0x0803B37E,
        0x0803B3B6,
        0x0803B3F8,
        0x0803B43C,
        0x0803B47C,
        0x0803B4C0,
        0x0803B4CC,
        0x0803B4FE,
        0x0803B51E,
        0x0803B52A,
        0x0803B55A,
        0x0803B566,
        0x0803B59E,
        0x0803B5CE,
        0x0803B5DA,
        0x0803B612,
        0x0803B654,
        0x0803B698,
        0x0803B6D6,
        0x0803B710,
        0x0803B732,
        0x0803B73E,
        0x0803B74A,
        0x0803B758,
        0x0803B764,
        0x0803B772,
        0x0803B794,
        0x0803B7A2,
        0x0803B7AE,
        0x0803B7BA,
        0x0803B7D2,
        0x0803B818,
        0x0803B824,
        0x0803C1F0,
        0x0803C1FC,
        0x0803CA0E,
        0x0803CA1A,
        0x08047348,
        0x080473A6,
        0x080511DE,
        0x080511FE,
        0x0806BABC,
    ]
    for adr in gfxblockfill:
        merger.hookbl(adr, sapplet.getadr("gfx_blockfill_hook"))
		
    #dt2list = [
    #    0x0800CBA0,
    #    0x0800CBC2,
    #    0x0800CCE8,
    #    0x0800CE86,
    #    0x0800CEA8,
    #    0x0800CF82,
    #    0x0800CFCE,
    #    0x0800D08C,
    #    0x0800D0D8,
    #    0x0800D166,
    #    0x0800D1B6,
    #    0x0800D2E2,
    #    0x0800D3E0,
    #    0x0800D48E,
    #    0x0800D7BC,
    #    0x0800E826,
    #    0x0800E846,
    #    0x0800F74A,
    #    0x0800F7D8,
    #    0x0800F7F8,
    #    0x08027922,
    #    0x08027A2A,
    #    0x080284B0,
    #    0x080284CE,
    #    0x0802872C,
    #    0x08028782,
    #    0x08029576,
    #    0x080295C2,
    #    0x080295F6,
    #    0x08029636,
    #    0x08029686,
    #    0x08029698,
    #    0x080296D8,
    #    0x08030AD6,
    #    0x08030B04,
    #    0x08030B58,
    #    0x08030C74,
    #    0x08030EA4,
    #    0x08038724,
    #    0x08039790,
    #    0x080397B0,
    #    0x08063488,
    #    0x080634A8,
    #]
    #for adr in dt2list:
    #    merger.hookbl(adr, sapplet.getadr("gfx_drawtext10_hook"))
		
    dt4list = [
        0x0800F1D0,
        0x0800F20A,
        0x0800F27A,
        0x0800F2BA,
        0x0800F344,
        0x0800F384,
        0x0800F3B2,
        0x0800F3F8,
        0x0800F466,
        0x0803097A,
        0x080309D6,
    ]
    for adr in dt4list:
        merger.hookbl(adr, sapplet.getadr("gfx_drawtext4_hook"))

    gfxdrawcharpos = [
        0x0800CF6C,
        0x0800CFB8,
        0x0800D076,
        0x0800D0C2,
        0x0800D14C,
        0x0800D19C,
        0x0800D270,
        0x0800D316,
        0x0800D386,
        0x0800DBB4,
        0x08030CBC,
        0x08030CC6,
        0x08030CE0,
        0x08030CEC,
        0x0803AF34,
        0x0803AF58,
        0x0803AF74,
        0x0803AF8E,
        0x0803AFB0,
        0x0803AFCC,
        0x0803B280,
        0x0803B2E6,
        0x0803B328,
        0x0803B364,
        0x0803B39C,
        0x0803B3DE,
        0x0803B41A,
        0x0803B4DC,
        0x0803B542,
        0x0803B584,
        0x0803B5C0,
        0x0803B5F8,
        0x0803B63A,
        0x0803B676,
    ]
    for adr in gfxdrawcharpos:
        merger.hookbl(adr, sapplet.getadr("gfx_drawchar_pos_hook"))
    
    gfxdt10 = [
        0x0800CB3A,
        0x0800CD24,
        0x0800CF36,
        0x0800CF54,
        0x0800CFA0,
        0x0800D040,
        0x0800D05E,
        0x0800D0AA,
        0x0800D116,
        0x0800D134,
        0x0800D184,
        0x0800D2A2,
        0x0800D2B4,
        0x0800D34C,
        0x0800D3BC,
        0x0800D4AC,
        0x0800D4E6,
        0x0800D504,
        0x0800D554,
        0x0800DC14,
        0x0800DC32,
        0x0800DCDA,
        0x0800DD36,
        0x0800DD54,
        0x0800E4B8,
        0x0800E532,
        0x0800E724,
        0x0800E740,
        0x0800E768,
        0x0800E790,
        0x0800F86A,
        0x08026152,
        0x0802687A,
        0x08026892,
        0x080268C8,
        0x080268E0,
        0x0803062E,
        0x08030834,
        0x080308A4,
        0x080308F6,
        0x08030A6C,
        0x0803AE82,
        0x0803B1B4,
    ]
    for adr in gfxdt10:
        merger.hookbl(adr, sapplet.getadr("gfx_drawtext2_hook"))
	
	# f_4315
    merger.hookbl(0x08027E3A, sapplet.getadr("f_4315_hook"))
    merger.hookbl(0x08027E70, sapplet.getadr("f_4315_hook"))
	
    merger.hookbl(0x0803B964, sapplet.getadr("f_4225_hook"), 0)
    merger.hookbl(0x0806402E, sapplet.getadr("f_4225_hook"), 0)
	
    print("Merging %s into %s at %08x" % (
        sys.argv[2],
        sys.argv[1],
        index))

    i = 0
    for b in bapplet:
        merger.setbyte(index + i, bapplet[i])
        i += 1

    merger.export(sys.argv[1])
