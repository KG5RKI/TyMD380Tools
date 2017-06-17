#!/usr/bin/env python2
# -*- coding: utf-8 -*-

from __future__ import print_function

import argparse
import binascii
import struct
import sys


class MD380FW(object):
    # The stream cipher of MD-380 OEM firmware updates boils down
    # to a cyclic, static XOR key block, and here it is:
    key = '00aa89891f4beccf424514540065eb66417d4c88495a210df2f5c8e638edbcb9fb357133010a7f9e3b2903b6493e42b83f9f90bdaa3a7146cecdfd183255894a5fc8839ce4069e0a9d0d2fa1356dd792eafd638590cbf02fd9595327d306b8f5b2ca886cd026913bf25b61becddbf21ac9fd8d8804f4e8f19a0292bc24e990e47ea3494dce529f58c17a5fb5186edb786408d56f0d9d9fb39930817e2be65b3c4aba8be5e472118993d2f12d1e0fd9d5438704e2b4ae5e9e5f4ce916f2e65f289f7919dc1d6f2ff8efcbe1cee8a73659efe0e28800106dda73bc922b81cfe6ce0447badb7e2f41ca5dcdf6417d1c382bef7d370af9a9148e5dea4466de8b3656018c358a119b8f2a6540f72ee6582874cbc4cb0fa7629be3a63cc76f130097e91eb15390dd9b613e908cad3c29414e5b0c1f664013244900d51ce2ed281853afe41cdc96ea18fe2e6519e01450c1f10939f5cf4544d5680d72f15f8823b9b1cfda36984341f8af236d50575e62bf5aa5daadcfc5425e3e34062304e90ecdf87189674e40e925bc452e97dcc16822d25877b12e6916a8149b181a9ab8f03b71bf7718c834ea856dbb325735e569d49f4a9968b4d8c79a316a303de89cd2eb64de2eafccc84d0209ae01f92b736dbc09a2c73a28ba5d1bdfcad6f6b83ebbc518f9369623a41983da4521e386137dc25a898a8f54b9e11564e393add046b3b1d7361533956f56ef26a91c7f0e6c9fced82669cffe7b5a6f09dceec8f95bc397e7bd55f0e9d10c3036017a348b27ddc8cda2ec62efa8d01116dd70b0fb25f15f91b77d34e974442d5276c169c4eb3f987f249bb1efe94be3d3109fcd9e4e47f11d4c16665bfd06cec2307b888261cc2737d5ff22c6e6d4cc879b0687aa7bcd35d3a3a7f0081758fbcd562ff88d318c5b3cdc9f1e3b4672b77ca62a47e6568a14fbe5b839b868449cbc106621ad02871dd862030e17b12e89f85a95731b878674dc39d2a93298d199d788a76baa7cc656518fb45822d10f2b44dece7511b6c93fbfc87ca8405007da66e37a3e4b4850ecf08a3966244b1d85a85b5db3908a5c5becba3e9ea838ef48b14c6702590e2dc9fd7c1a9ee5ca607f6bf9cb9760ab46b2ab36a0f333f790c900e9f71f9d7566d3c08ce06a2cf4e102d7df9e8748c28f2a44642b0fa936f3469ae2b1fddc2602f480e31231c371a7f4323661ed127740adfe6d665bd29c1ea8c8601e04e1c9091387a8385a70eaba3fc525993084715f222379d93d76d21bd5d28bc49d730584171b04db4ffc0723c9d8d5d0b86759f770f9af0d1e5c7ff2b7008a2d2e59827aea851f82772f6fe97cb36e8ded82d60d81c93889674d4ca9359986e1215ce9f3730d20b53ad0cb143e9d1759379f91ab3cda3cd57e11e04a36e7a666dc44e2f79afa30fc00a9c2adf9e0f8bbfe8431d88976e2'.decode('hex')

    def __init__(self, base_address=0x800c000):
        self.magic = b'OutSecurityBin'
        self.jst = b'MD-9600\x00\x00'
        self.foo = '\x30\x02\x00\x30\x00\x40\x00\x47'
        self.bar = ('\x02\x19\x0C\x03\x04\x05\x06\x07'
                    '\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f'
                    '\x10\x11\x12\x13\x14\x15\x16\x17'
                    '\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f'
                    '\x20')
        self.foob = ('\x02\x00\x00\x00\x00\x00\x06\x00')
        self.start = base_address
        self.app = None
        self.footer = 'OutputBinDataEnd'
        self.header_fmt = '<16s9s7s16s33s43s8sLLL112s'
        self.footer_fmt = '<240s16s'
        self.rsrcSize = 0x5D400

    def pad(self, align=512, byte=b'\xff'):
        pad_length = (align - len(self.app) % align) % align
        self.app += byte * pad_length

    def wrap(self):
        bin = b''
        header = struct.Struct(self.header_fmt)
        footer = struct.Struct(self.footer_fmt)
        self.pad()
        app = self.crypt(self.app)
        bin += header.pack(
            self.magic, self.jst, b'\xff' * 7, self.foo,
            self.bar, b'\xff' * 43, self.foob, self.rsrcSize, self.start, len(app)-self.rsrcSize,
                                  b'\xff' * 112)
        bin += self.crypt(self.app)
        bin += footer.pack(b'\xff' * 240, self.footer)
        return bin

    def unwrap(self, img):
        header = struct.Struct(self.header_fmt)
        header = header.unpack(img[:256])

        print(header)
        self.start = header[6]
        rsrc_len = header[7]
        print('rsrc_len %x' % rsrc_len)
        prg_len = header[9]
        print('prg_len %x' % prg_len)
        app_len = rsrc_len + prg_len
        print('applen %x' % app_len)
        self.app = self.crypt(img[256:256 + app_len])

        #assert header[0].startswith(self.magic)
        #assert header[1].startswith(self.jst)
        #assert header[3].startswith(self.foo)
        #assert header[4] == self.bar
        #assert 0x8000000 <= header[6] < 0x8200000
        #assert header[7] == len(img) - 512
        self.app = self.app[rsrc_len:]

    def crypt(self, data):
        return self.xor(data, self.key)

    @staticmethod
    def xor(a, b):
        # FIXME: optimized version
        out = b''
        l = max(len(a), len(b))
        for i in range(l):
            out += chr(ord(a[i % len(a)]) ^ ord(b[i % len(b)]))
        return out


def main():
    def hex_int(x):
        return int(x, 0)

    parser = argparse.ArgumentParser(description='Wrap and unwrap MD-380 firmware')
    parser.add_argument('--wrap', '-w', dest='wrap', action='store_true',
                        default=False,
                        help='wrap app into firmware image')
    parser.add_argument('--unwrap', '-u', dest='unwrap', action='store_true',
                        default=False,
                        help='unwrap app from firmware image')
    parser.add_argument('--addr', '-a', dest='addr', type=hex_int,
                        default=0x800c000,
                        help='base address in flash')
    parser.add_argument('--offset', '-o', dest='offset', type=hex_int,
                        default=0,
                        help='offset to skip in app binary')
    parser.add_argument('input', nargs=1, help='input file')
    parser.add_argument('output', nargs=1, help='output file')
    args = parser.parse_args()

    if not (args.wrap ^ args.unwrap):
        sys.stderr.write('ERROR: --wrap or --unwrap?')
        sys.exit(5)

    print('DEBUG: reading "%s"' % args.input[0])
    with open(args.input[0], 'rb') as f:
        input = f.read()

    

    if args.wrap:
	
        with open('C:/tyt/FW_2017_f.bin', 'rb') as f:
            front = f.read()
            input = front + input;
	
        if args.offset > 0:
            print('INFO: skipping 0x%x bytes in input file' % args.offset)
        md = MD380FW(args.addr)
        md.app = input[args.offset:]
        if len(md.app) == 0:
            sys.stderr.write('ERROR: seeking beyond end of input file\n')
            sys.exit(5)
        output = md.wrap()
        print('INFO: base address 0x{0:x}'.format(md.start))
        print('INFO: length 0x{0:x}'.format(len(md.app)))

    elif args.unwrap:
        md = MD380FW()
        try:
            md.unwrap(input)
        except AssertionError:
            sys.stderr.write('WARNING: Funky header:\n')
            for i in range(0, 256, 16):
                hl = binascii.hexlify(input[i:i + 16])
                hl = ' '.join(hl[i:i + 2] for i in range(0, 32, 2))
                sys.stderr.write(hl + '\n')
            sys.stderr.write('Trying anyway.\n')
        output = md.app
        #print('INFO: base address 0x{0:x}'.format(md.start))
        print('INFO: length 0x{0:x}'.format(len(md.app)))

    print('DEBUG: writing "%s"' % args.output[0])

    with open(args.output[0], 'wb') as f:
        f.write(output)



if __name__ == "__main__":
    main()
