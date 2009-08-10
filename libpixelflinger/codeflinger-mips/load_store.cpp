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

#include "codeflinger-mips/GGLAssembler.h"

namespace android {

// ----------------------------------------------------------------------------

void GGLAssembler::store(const pointer_t& addr, const pixel_t& s, uint32_t flags)
{    
    const int bits = addr.size;
    const int inc = (flags & WRITE_BACK)?1:0;
    switch (bits) {
    case 32:
        SW(s.reg, 0, addr.reg);
        if (inc)
	    ADDIU(addr.reg, addr.reg, 4);
        break;
    case 24:
        // 24 bits formats are a little special and used only for RGB
        // 0x00BBGGRR is unpacked as R,G,B
	MOVE(at, s.reg);
        SB(at, 0, addr.reg);
	SRL(at, at, 8);
        SB(at, 1, addr.reg);
	SRL(at, at, 8);
        SB(at, 2, addr.reg);
        if (inc)
            ADDIU(addr.reg, addr.reg, 3);
        break;
    case 16:
        SH(s.reg, 0, addr.reg);
        if (inc)
            ADDIU(addr.reg, addr.reg, 2);
        break;
    case  8:
        SB(s.reg, 0, addr.reg);
        if (inc)
            ADDIU(addr.reg, addr.reg, 1);
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
        LW(s.reg, 0, addr.reg);
        if (inc)
            ADDIU(addr.reg, addr.reg, 4);
        break;
    case 24:
        // 24 bits formats are a little special and used only for RGB
        // R,G,B is packed as 0x00BBGGRR 
        if (s.reg != addr.reg)
	    s0 = s.reg;
	else
            s0 = scratches.obtain();
        LBU(s0, 2, addr.reg);         // B
        LBU(at, 1, addr.reg);         // G
	SLL(s0, s0, 8);
        OR(s0, s0, at);
        LBU(at, 0, addr.reg);         // R
	SLL(s0, s0, 8);
        OR(s.reg, s0, at);
        if (inc)
            ADDIU(addr.reg, addr.reg, 3);
        break;        
    case 16:
        LHU(s.reg, 0, addr.reg);
        if (inc)
            ADDIU(addr.reg, addr.reg, 2);
        break;
    case  8:
        LBU(s.reg, 0, addr.reg);
        if (inc)
            ADDIU(addr.reg, addr.reg, 1);
        break;
    }
}

void GGLAssembler::extract(integer_t& d, int s, int h, int l, int bits)
{
    const int maskLen = h-l;

    assert(maskLen<=8);
    assert(h);
    
    if (h != bits) {
        const int mask = ((1<<maskLen)-1) << l;
	uint32_t atval;
	bool	 atvalid;


	if (mask & 0xffff0000) {
	    atvalid = getAtValue(atval);
	    if ((atvalid == false) || (atval != (uint32_t)mask)) {
	        LUI(at, mask >> 16);
	        if (mask & 0xffff)
		    ORI(at, at, mask & 0xffff);
	        setAtValue(mask);
	    }
	    AND(d.reg, s, at);       // component = packed & mask;
	}
	else {
            ANDI(d.reg, s, mask);    // component = packed & mask;
	}
	s = d.reg;
    }
    
    if (l) {
        SRL(d.reg, s, l);       // component = packed >> l;
    }
    
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
            MOVE(d, s);
        }
        return;
    }

    if (sbits == 1) {
	SLL(at, s, dbits);
	SUBU(d, at, s);
            // d = (s<<dbits) - s;
        return;
    }

    if (dbits % sbits) {
        SLL(d, s, dbits-sbits);
            // d = s << (dbits-sbits);
        dbits -= sbits;
        do {
            SRL(at, d, sbits);
            OR(d, d, at);
                // d |= d >> sbits;
            dbits -= sbits;
            sbits *= 2;
        } while(dbits>0);
        return;
    }
    
    dbits -= sbits;
    do {
        SLL(at, s, sbits);
        OR(d, s, at);
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
        SLL(ireg, s.reg, 32-sh);
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
            SRL(ireg, s.reg, sl);
            sh -= sl;
            sl = 0;
            s.reg = ireg; 
        }
        // scaling (V-V>>dbits)
        SRL(at, s.reg, dbits);
        SUBU(ireg, s.reg, at);
        const int shift = (GGL_DITHER_BITS - (sbits-dbits));
	int addreg = at;
        if (shift>0)        SRL(at, dither.reg, shift);
        else if (shift<0)   SLL(at, dither.reg, -shift);
        else                addreg = dither.reg;
        ADDU(ireg, ireg, addreg);

        s.reg = ireg; 
    }

    if ((maskLoBits|dithering) && (sh > dbits)) {
        int shift = sh-dbits;
        if (dl) {
            SRL(ireg, s.reg, shift);
            if (ireg == d.reg) {
                SLL(d.reg, ireg, dl);
            } else {
                SLL(at, ireg, dl);
                OR(d.reg, d.reg, at);
            }
        } else {
            if (ireg == d.reg) {
                SRL(d.reg, s.reg, shift);
            } else {
                SRL(at, s.reg, shift);
                OR(d.reg, d.reg, at);
            }
        }
    } else {
        int shift = sh-dh;
        if (shift>0) {
            if (ireg == d.reg) {
                SRL(d.reg, s.reg, shift);
            } else {
                SRL(at, s.reg, shift);
                OR(d.reg, d.reg, at);
            }
        } else if (shift<0) {
            if (ireg == d.reg) {
                SLL(d.reg, s.reg, -shift);
            } else {
                SLL(at, s.reg, -shift);
                OR(d.reg, d.reg, at);
            }
        } else {
            if (ireg == d.reg) {
                if (s.reg != d.reg) {
                    MOVE(d.reg, s.reg);
                }
            } else {
                OR(d.reg, d.reg, s.reg);
            }
        }
    }
}

}; // namespace android
