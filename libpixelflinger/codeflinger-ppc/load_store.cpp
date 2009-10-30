/* libs/pixelflinger/codeflinger/load_store.cpp
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <assert.h>
#include <stdio.h>
#include <cutils/log.h>

#include "codeflinger-ppc/GGLAssembler.h"

namespace android {

// ----------------------------------------------------------------------------

void GGLAssembler::store(const pointer_t& addr, const pixel_t& s, uint32_t flags)
{
    const int bits = addr.size;
    const int inc = (flags & WRITE_BACK)?1:0;
    switch (bits) {
    case 32:
	// 32-bit formats are stored in little-endian order
        STWBRX(s.reg, 0, addr.reg);
        if (inc)
	    ADDI(addr.reg, addr.reg, 4);
        break;
    case 24:
        // 24 bits formats are a little special and used only for RGB
        // 0x00BBGGRR is unpacked as R,G,B
	MR(r0, s.reg);		//R
        STB(r0, 0, addr.reg);
	SRWI(r0, r0, 8);	//G
        STB(r0, 1, addr.reg);
	SRWI(r0, r0, 8);
        STB(r0, 2, addr.reg);	//B
        if (inc)
            ADDI(addr.reg, addr.reg, 3);
        break;
    case 16:
        STH(s.reg, 0, addr.reg);
        if (inc)
            ADDI(addr.reg, addr.reg, 2);
        break;
    case  8:
        STB(s.reg, 0, addr.reg);
        if (inc)
            ADDI(addr.reg, addr.reg, 1);
        break;
    }
}

void GGLAssembler::load(const pointer_t& addr, const pixel_t& s, uint32_t flags)
{
    Scratch scratches(registerFile());
    int s0;

    const int bits = addr.size;
    const int inc = (flags & WRITE_BACK)?1:0;
    switch (bits) {
    case 32:
	// 32-bit formats are stored in little-endian order
        LWBRX(s.reg, 0, addr.reg);
        if (inc)
            ADDI(addr.reg, addr.reg, 4);
        break;
    case 24:
        // 24 bits formats are a little special and used only for RGB
        // R,G,B is packed as 0x00BBGGRR
        if (s.reg != addr.reg)
	    s0 = s.reg;
	else
            s0 = scratches.obtain();
        LBZ(s0, 2, addr.reg);         // B
        LBZ(r0, 1, addr.reg);         // G
	SLWI(s0, s0, 8);
        OR(s0, s0, r0);
        LBZ(r0, 0, addr.reg);         // R
	SLWI(s0, s0, 8);
        OR(s.reg, s0, r0);
        if (inc)
            ADDI(addr.reg, addr.reg, 3);
        break;
    case 16:
        LHZ(s.reg, 0, addr.reg);
        if (inc)
            ADDI(addr.reg, addr.reg, 2);
        break;
    case  8:
        LBZ(s.reg, 0, addr.reg);
        if (inc)
            ADDI(addr.reg, addr.reg, 1);
        break;
    }
}

void GGLAssembler::extract(integer_t& d, int s, int h, int l, int bits)
{
    const int maskLen = h-l;

    assert(maskLen<=8);
    assert(h);

    // Extract bits [h-1 .. l] from the source register and put
    // the right justified result to the destination register.
    // PPC has a reversed register bits numeration
    EXTRWI(d.reg, s, maskLen, 32-h);
    d.s = maskLen;
}

void GGLAssembler::extract(integer_t& d, const pixel_t& s, int component)
{
    extract(d,  s.reg,
                s.format.c[component].h,
                s.format.c[component].l,
                s.size());
}

void GGLAssembler::extract(component_t& d, const pixel_t& s, int component)
{
    integer_t r(d.reg, 32, d.flags);
    extract(r,  s.reg,
                s.format.c[component].h,
                s.format.c[component].l,
                s.size());
    d = component_t(r);
}


void GGLAssembler::expand(integer_t& d, const component_t& s, int dbits)
{
    if (s.l || (s.flags & CLEAR_HI)) {
        extract(d, s.reg, s.h, s.l, 32);
        expand(d, d, dbits);
    } else {
        expand(d, integer_t(s.reg, s.size(), s.flags), dbits);
    }
}

void GGLAssembler::expand(component_t& d, const component_t& s, int dbits)
{
    integer_t r(d.reg, 32, d.flags);
    expand(r, s, dbits);
    d = component_t(r);
}

void GGLAssembler::expand(integer_t& dst, const integer_t& src, int dbits)
{
    assert(src.size());

    int sbits = src.size();
    int s = src.reg;
    int d = dst.reg;

    // be sure to set 'dst' after we read 'src' as they may be identical
    dst.s = dbits;
    dst.flags = 0;

    if (dbits<=sbits) {
        if (s != d) {
            MR(d, s);
        }
        return;
    }

    if (sbits == 1) {
	SLWI(r0, s, dbits);
	SUB(d, r0, s);
            // d = (s<<dbits) - s;
        return;
    }

    if (dbits % sbits) {
        SLWI(d, s, dbits-sbits);
            // d = s << (dbits-sbits);
        dbits -= sbits;
        do {
            SRWI(r0, d, sbits);
            OR(d, d, r0);
                // d |= d >> sbits;
            dbits -= sbits;
            sbits *= 2;
        } while(dbits>0);
        return;
    }

    dbits -= sbits;
    do {
        SLWI(r0, s, sbits);
        OR(d, s, r0);
            // d |= s<<sbits;
        s = d;
        dbits -= sbits;
        if (sbits*2 < dbits) {
            sbits *= 2;
        }
    } while(dbits>0);
}

void GGLAssembler::downshift(
        pixel_t& d, int component, component_t s, const reg_t& dither)
{
    const needs_t& needs = mBuilderContext.needs;
    Scratch scratches(registerFile());

    int sh = s.h;
    int sl = s.l;
    int maskHiBits = (sh!=32) ? ((s.flags & CLEAR_HI)?1:0) : 0;
    int maskLoBits = (sl!=0)  ? ((s.flags & CLEAR_LO)?1:0) : 0;
    int sbits = sh - sl;

    int dh = d.format.c[component].h;
    int dl = d.format.c[component].l;
    int dbits = dh - dl;
    int dithering = 0;

    LOGE_IF(sbits<dbits, "sbits (%d) < dbits (%d) in downshift", sbits, dbits);

    if (sbits>dbits) {
        // see if we need to dither
        dithering = mDithering;
    }

    int ireg = d.reg;
    if (!(d.flags & FIRST)) {
        if (s.flags & CORRUPTIBLE)  {
            ireg = s.reg;
        } else {
            ireg = scratches.obtain();
        }
    }
    d.flags &= ~FIRST;

    if (maskHiBits) {
        // we need to mask the high bits (and possibly the lowbits too)
        // and we might be able to use immediate mask.
        if (!dithering) {
            // we don't do this if we only have maskLoBits because we can
            // do it more efficiently below (in the case where dl=0)
            const int offset = sh - dbits;
            if (dbits<=8 && offset >= 0) {
                const uint32_t mask = ((1<<dbits)-1) << offset;
                build_and_immediate(ireg, s.reg, mask, 32);
                sl = offset;
                s.reg = ireg;
                sbits = dbits;
                maskLoBits = maskHiBits = 0;
            }
        } else {
            // in the dithering case though, we need to preserve the lower bits
            const uint32_t mask = ((1<<sbits)-1) << sl;
            build_and_immediate(ireg, s.reg, mask, 32);
            s.reg = ireg;
            maskLoBits = maskHiBits = 0;
        }
    }

    // XXX: we could special case (maskHiBits & !maskLoBits)
    // like we do for maskLoBits below, but it happens very rarely
    // that we have maskHiBits only and the conditions necessary to lead
    // to better code (like doing d |= s << 24)

    if (maskHiBits) {
        SLWI(ireg, s.reg, 32-sh);
        sl += 32-sh;
        sh = 32;
        s.reg = ireg;
        maskHiBits = 0;
    }

    //	Downsampling should be performed as follows:
    //  V * ((1<<dbits)-1) / ((1<<sbits)-1)
    //	V * [(1<<dbits)/((1<<sbits)-1)	-	1/((1<<sbits)-1)]
    //	V * [1/((1<<sbits)-1)>>dbits	-	1/((1<<sbits)-1)]
    //	V/((1<<(sbits-dbits))-(1>>dbits))	-	(V>>sbits)/((1<<sbits)-1)>>sbits
    //	V/((1<<(sbits-dbits))-(1>>dbits))	-	(V>>sbits)/(1-(1>>sbits))
    //
    //	By approximating (1>>dbits) and (1>>sbits) to 0:
    //
    //		V>>(sbits-dbits)	-	V>>sbits
    //
	//  A good approximation is V>>(sbits-dbits),
    //  but better one (needed for dithering) is:
    //
    //		(V>>(sbits-dbits)<<sbits	-	V)>>sbits
    //		(V<<dbits	-	V)>>sbits
    //		(V	-	V>>dbits)>>(sbits-dbits)

    // Dithering is done here
    if (dithering) {
        comment("dithering");
        if (sl) {
            SRWI(ireg, s.reg, sl);
            sh -= sl;
            sl = 0;
            s.reg = ireg;
        }
        // scaling (V-V>>dbits)
        SRWI(r0, s.reg, dbits);
        SUB(ireg, s.reg, r0);
        const int shift = (GGL_DITHER_BITS - (sbits-dbits));
	int addreg = r0;
        if (shift>0)        SRWI(r0, dither.reg, shift);
        else if (shift<0)   SLWI(r0, dither.reg, -shift);
        else                addreg = dither.reg;
        ADD(ireg, ireg, addreg);

        s.reg = ireg;
    }

    if ((maskLoBits|dithering) && (sh > dbits)) {
        int shift = sh-dbits;
        if (dl) {
            SRWI(ireg, s.reg, shift);
            if (ireg == d.reg) {
                SLWI(d.reg, ireg, dl);
            } else {
                SLWI(r0, ireg, dl);
                OR(d.reg, d.reg, r0);
            }
        } else {
            if (ireg == d.reg) {
                SRWI(d.reg, s.reg, shift);
            } else {
                SRWI(r0, s.reg, shift);
                OR(d.reg, d.reg, r0);
            }
        }
    } else {
        int shift = sh-dh;
        if (shift>0) {
            if (ireg == d.reg) {
                SRWI(d.reg, s.reg, shift);
            } else {
                SRWI(r0, s.reg, shift);
                OR(d.reg, d.reg, r0);
            }
        } else if (shift<0) {
            if (ireg == d.reg) {
                SLWI(d.reg, s.reg, -shift);
            } else {
                SLWI(r0, s.reg, -shift);
                OR(d.reg, d.reg, r0);
            }
        } else {
            if (ireg == d.reg) {
                if (s.reg != d.reg) {
                    MR(d.reg, s.reg);
                }
            } else {
                OR(d.reg, d.reg, s.reg);
            }
        }
    }
}

}; // namespace android
