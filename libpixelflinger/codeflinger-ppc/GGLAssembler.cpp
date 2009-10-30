/* libs/pixelflinger/codeflinger-ppc/GGLAssembler.cpp
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

#define LOG_TAG "GGLAssembler"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <cutils/log.h>

#include "codeflinger-ppc/GGLAssembler.h"

namespace android {

// ----------------------------------------------------------------------------

GGLAssembler::GGLAssembler(PPCAssemblerInterface* target)
    : PPCAssemblerProxy(target), RegisterAllocator(), mOptLevel(7)
{
}

GGLAssembler::~GGLAssembler()
{
}

void GGLAssembler::prolog()
{
    PPCAssemblerProxy::prolog();
}

void GGLAssembler::epilog(uint32_t touched)
{
    PPCAssemblerProxy::epilog(touched);
}

void GGLAssembler::reset(int opt_level)
{
    PPCAssemblerProxy::reset();
    RegisterAllocator::reset();
    mOptLevel = opt_level;
}

// ---------------------------------------------------------------------------

int GGLAssembler::scanline(const needs_t& needs, context_t const* c)
{
    int err = 0;
    int opt_level = mOptLevel;
    while (opt_level >= 0) {
        reset(opt_level);
        err = scanline_core(needs, c);
        if (err == 0)
            break;
        opt_level--;
    }

    // XXX: in theory, pcForLabel is not valid before generate()
    uint32_t* fragment_start_pc = pcForLabel("fragment_loop");
    uint32_t* fragment_end_pc = pcForLabel("epilog");
    const int per_fragment_ops = int(fragment_end_pc - fragment_start_pc);

    // build a name for our pipeline
    char name[64];
    sprintf(name,
            "scanline__%08X:%08X_%08X_%08X [%3d ipp]",
            needs.p, needs.n, needs.t[0], needs.t[1], per_fragment_ops);

    if (err) {
        LOGE("Error while generating ""%s""\n", name);
        disassemble(name);
        return -1;
    }

#if 0
    return generate(name);
#else
    int r = generate(name);
    disassemble(name);
    return r;
#endif
}

int GGLAssembler::scanline_core(const needs_t& needs, context_t const* c)
{
    int64_t duration = ggl_system_time();

    mBlendFactorCached = 0;
    mBlending = 0;
    mMasking = 0;
    mAA        = GGL_READ_NEEDS(P_AA, needs.p);
    mDithering = GGL_READ_NEEDS(P_DITHER, needs.p);
    mAlphaTest = GGL_READ_NEEDS(P_ALPHA_TEST, needs.p) + GGL_NEVER;
    mDepthTest = GGL_READ_NEEDS(P_DEPTH_TEST, needs.p) + GGL_NEVER;
    mFog       = GGL_READ_NEEDS(P_FOG, needs.p) != 0;
    mSmooth    = GGL_READ_NEEDS(SHADE, needs.n) != 0;
    mBuilderContext.needs = needs;
    mBuilderContext.c = c;
    mBuilderContext.Rctx = reserveReg(r3); // context always in r3
    mCbFormat = c->formats[ GGL_READ_NEEDS(CB_FORMAT, needs.n) ];

    // ------------------------------------------------------------------------

    decodeLogicOpNeeds(needs);

    decodeTMUNeeds(needs, c);

    mBlendSrc  = ggl_needs_to_blendfactor(GGL_READ_NEEDS(BLEND_SRC, needs.n));
    mBlendDst  = ggl_needs_to_blendfactor(GGL_READ_NEEDS(BLEND_DST, needs.n));
    mBlendSrcA = ggl_needs_to_blendfactor(GGL_READ_NEEDS(BLEND_SRCA, needs.n));
    mBlendDstA = ggl_needs_to_blendfactor(GGL_READ_NEEDS(BLEND_DSTA, needs.n));

    if (!mCbFormat.c[GGLFormat::ALPHA].h) {
        if ((mBlendSrc == GGL_ONE_MINUS_DST_ALPHA) ||
            (mBlendSrc == GGL_DST_ALPHA)) {
            mBlendSrc = GGL_ONE;
        }
        if ((mBlendSrcA == GGL_ONE_MINUS_DST_ALPHA) ||
            (mBlendSrcA == GGL_DST_ALPHA)) {
            mBlendSrcA = GGL_ONE;
        }
        if ((mBlendDst == GGL_ONE_MINUS_DST_ALPHA) ||
            (mBlendDst == GGL_DST_ALPHA)) {
            mBlendDst = GGL_ONE;
        }
        if ((mBlendDstA == GGL_ONE_MINUS_DST_ALPHA) ||
            (mBlendDstA == GGL_DST_ALPHA)) {
            mBlendDstA = GGL_ONE;
        }
    }

    // if we need the framebuffer, read it now
    const int blending =    blending_codes(mBlendSrc, mBlendDst) |
                            blending_codes(mBlendSrcA, mBlendDstA);

    // XXX: handle special cases, destination not modified...
    if ((mBlendSrc==GGL_ZERO) && (mBlendSrcA==GGL_ZERO) &&
        (mBlendDst==GGL_ONE) && (mBlendDstA==GGL_ONE)) {
        // Destination unmodified (beware of logic ops)
    } else if ((mBlendSrc==GGL_ZERO) && (mBlendSrcA==GGL_ZERO) &&
        (mBlendDst==GGL_ZERO) && (mBlendDstA==GGL_ZERO)) {
        // Destination is zero (beware of logic ops)
    }

    int fbComponents = 0;
    const int masking = GGL_READ_NEEDS(MASK_ARGB, needs.n);
    for (int i=0 ; i<4 ; i++) {
        const int mask = 1<<i;
        component_info_t& info = mInfo[i];
        int fs = i==GGLFormat::ALPHA ? mBlendSrcA : mBlendSrc;
        int fd = i==GGLFormat::ALPHA ? mBlendDstA : mBlendDst;
        if (fs==GGL_SRC_ALPHA_SATURATE && i==GGLFormat::ALPHA)
            fs = GGL_ONE;
        info.masked =   !!(masking & mask);
        info.inDest =   !info.masked && mCbFormat.c[i].h &&
                        ((mLogicOp & LOGIC_OP_SRC) || (!mLogicOp));
        if (mCbFormat.components >= GGL_LUMINANCE &&
                (i==GGLFormat::GREEN || i==GGLFormat::BLUE)) {
            info.inDest = false;
        }
        info.needed =   (i==GGLFormat::ALPHA) &&
                        (isAlphaSourceNeeded() || mAlphaTest != GGL_ALWAYS);
        info.replaced = !!(mTextureMachine.replaced & mask);
        info.iterated = (!info.replaced && (info.inDest || info.needed));
        info.smooth =   mSmooth && info.iterated;
        info.fog =      mFog && info.inDest && (i != GGLFormat::ALPHA);
        info.blend =    (fs != int(GGL_ONE)) || (fd > int(GGL_ZERO));

        mBlending |= (info.blend ? mask : 0);
        mMasking |= (mCbFormat.c[i].h && info.masked) ? mask : 0;
        fbComponents |= mCbFormat.c[i].h ? mask : 0;
    }

    mAllMasked = (mMasking == fbComponents);
    if (mAllMasked) {
        mDithering = 0;
    }

    fragment_parts_t parts;

    // ------------------------------------------------------------------------
    prolog();
    // ------------------------------------------------------------------------

    build_scanline_prolog(parts, needs);

    if (registerFile().status())
        return registerFile().status();

    // ------------------------------------------------------------------------

    label("fragment_loop");
    // ------------------------------------------------------------------------
    {
        Scratch regs(registerFile());

        if (mDithering) {
            // update the dither index.
            ROTRWI(parts.count.reg, parts.count.reg, GGL_DITHER_ORDER_SHIFT);
       	    if ((1 << (32 - GGL_DITHER_ORDER_SHIFT)) & 0xffff0000) {
                ADDIS(parts.count.reg, parts.count.reg,
                      (1 << (32 - GGL_DITHER_ORDER_SHIFT)) >> 16);
	    }
	    else {
                ADDI(parts.count.reg, parts.count.reg,
	             1 << (32 - GGL_DITHER_ORDER_SHIFT));
	    }
            ROTLWI(parts.count.reg, parts.count.reg, GGL_DITHER_ORDER_SHIFT);
        }

        // XXX: could we do an early alpha-test here in some cases?
        // It would probaly be used only with smooth-alpha and no texture
        // (or no alpha component in the texture).

        // Early z-test
        if (mAlphaTest==GGL_ALWAYS) {
            build_depth_test(parts, Z_TEST|Z_WRITE);
        } else {
            // we cannot do the z-write here, because
            // it might be killed by the alpha-test later
            build_depth_test(parts, Z_TEST);
        }

        { // texture coordinates
            Scratch scratches(registerFile());

            // texel generation
            build_textures(parts, regs);
        }

        if ((blending & (FACTOR_DST|BLEND_DST)) ||
                (mMasking && !mAllMasked) ||
                (mLogicOp & LOGIC_OP_DST))
        {
            // blending / logic_op / masking need the framebuffer
            mDstPixel.setTo(regs.obtain(), &mCbFormat);

            // load the framebuffer pixel
            comment("fetch color-buffer");
            load(parts.cbPtr, mDstPixel);
        }

        if (registerFile().status())
            return registerFile().status();

        pixel_t pixel;
        int directTex = mTextureMachine.directTexture;
        if (directTex | parts.packed) {
            // note: we can't have both here
            // iterated color or direct texture
            pixel = directTex ? parts.texel[directTex-1] : parts.iterated;
            pixel.flags &= ~CORRUPTIBLE;
        } else {
            if (mDithering) {
                const int ctxtReg = mBuilderContext.Rctx;
                const int mask = GGL_DITHER_SIZE-1;
                parts.dither = reg_t(regs.obtain());
                ANDIP(parts.dither.reg, parts.count.reg, mask);
                ADD(parts.dither.reg, parts.dither.reg, ctxtReg);
                LBZ(parts.dither.reg, GGL_OFFSETOF(ditherMatrix),
                    parts.dither.reg);
            }

            // allocate a register for the resulting pixel
            pixel.setTo(regs.obtain(), &mCbFormat, FIRST);

            build_component(pixel, parts, GGLFormat::ALPHA,    regs);

            if (mAlphaTest!=GGL_ALWAYS) {
                // only handle the z-write part here. We know z-test
                // was successful, as well as alpha-test.
                build_depth_test(parts, Z_WRITE);
            }

            build_component(pixel, parts, GGLFormat::RED,      regs);
            build_component(pixel, parts, GGLFormat::GREEN,    regs);
            build_component(pixel, parts, GGLFormat::BLUE,     regs);

            pixel.flags |= CORRUPTIBLE;
        }

        if (registerFile().status())
            return registerFile().status();

        if (pixel.reg == -1) {
            // be defensive here. if we're here it's probably
            // that this whole fragment is a no-op.
            pixel = mDstPixel;
        }

        if (!mAllMasked) {
            // logic operation
            build_logic_op(pixel, regs);

            // masking
            build_masking(pixel, regs);

            comment("store");
            store(parts.cbPtr, pixel, WRITE_BACK);
        }
    }

    if (registerFile().status())
        return registerFile().status();

    // update the iterated color...
    if (parts.reload != 3) {
        build_smooth_shade(parts);
    }

    // update iterated z
    build_iterate_z(parts);

    // update iterated fog
    build_iterate_f(parts);

    SUBIS(parts.count.reg, parts.count.reg, 1);
    BDNZ("fragment_loop");
    label("epilog");
    epilog(registerFile().touched());

    if ((mAlphaTest!=GGL_ALWAYS) || (mDepthTest!=GGL_ALWAYS)) {
        if (mDepthTest!=GGL_ALWAYS) {
            label("discard_before_textures");
            build_iterate_texture_coordinates(parts);
        }
        label("discard_after_textures");
        build_smooth_shade(parts);
        build_iterate_z(parts);
        build_iterate_f(parts);
        if (!mAllMasked) {
            ADDI(parts.cbPtr.reg, parts.cbPtr.reg, parts.cbPtr.size>>3);
        }
        SUBIS(parts.count.reg, parts.count.reg, 1);
        BDNZ("fragment_loop");
        epilog(registerFile().touched());
    }

    return registerFile().status();
}

// ---------------------------------------------------------------------------

void GGLAssembler::build_scanline_prolog(
    fragment_parts_t& parts, const needs_t& needs)
{
    Scratch scratches(registerFile());
    int Rctx = mBuilderContext.Rctx;

    // compute count
    comment("compute ct (# of pixels to process)");
    parts.count.setTo(obtainReg());
    int Rx = scratches.obtain();
    int Ry = scratches.obtain();
    CONTEXT_LOAD(Rx, iterators.xl);
    CONTEXT_LOAD(parts.count.reg, iterators.xr);
    CONTEXT_LOAD(Ry, iterators.y);

    // parts.count = iterators.xr - Rx
    SUB(parts.count.reg, parts.count.reg, Rx);

    // Easy optimization: use CTR register for loop
    // Save counter now before it's decremented and
    // shifted below
    MTCTR(parts.count.reg);

    SUBI(parts.count.reg, parts.count.reg, 1);

    if (mDithering) {
        // parts.count.reg = 0xNNNNXXDD
        // NNNN = count-1
        // DD   = dither offset
        // XX   = 0xxxxxxx (x = garbage)
        Scratch scratches(registerFile());
        int tx = scratches.obtain();
        int ty = scratches.obtain();
        ANDIP(tx, Rx, GGL_DITHER_MASK);
        ANDIP(ty, Ry, GGL_DITHER_MASK);
        SLWI(ty, ty, GGL_DITHER_ORDER_SHIFT);
	OR(tx, tx, ty);
        SLWI(parts.count.reg, parts.count.reg, 16);
        OR(parts.count.reg, parts.count.reg, tx);
    } else {
        // parts.count.reg = 0xNNNN0000
        // NNNN = count-1
        SLWI(parts.count.reg, parts.count.reg, 16);
    }
    if (!mAllMasked) {
        // compute dst ptr
        comment("compute color-buffer pointer");
        const int cb_bits = mCbFormat.size*8;
        int Rs = scratches.obtain();
        parts.cbPtr.setTo(obtainReg(), cb_bits);
        CONTEXT_LOAD(Rs, state.buffers.color.stride);
        CONTEXT_LOAD(parts.cbPtr.reg, state.buffers.color.data);
        // Rs = Rx + Ry*Rs
	MULLW(r0, Rs, Ry);
	ADD(Rs, Rx, r0);
        base_offset(parts.cbPtr, parts.cbPtr, Rs);
        scratches.recycle(Rs);
    }

    // init fog
    const int need_fog = GGL_READ_NEEDS(P_FOG, needs.p);
    if (need_fog) {
        comment("compute initial fog coordinate");
        Scratch scratches(registerFile());
        int dfdx = scratches.obtain();
        int ydfdy = scratches.obtain();
        int f = ydfdy;
        CONTEXT_LOAD(dfdx,  generated_vars.dfdx);
        CONTEXT_LOAD(ydfdy, iterators.ydfdy);
        // f = (Rx * dfdx) + ydfdy
        MULLW(r0, Rx, dfdx);
	ADD(f, r0, ydfdy);
        CONTEXT_STORE(f, generated_vars.f);
    }

    // init Z coordinate
    if ((mDepthTest != GGL_ALWAYS) || GGL_READ_NEEDS(P_MASK_Z, needs.p)) {
        parts.z = reg_t(obtainReg());
        comment("compute initial Z coordinate");
        Scratch scratches(registerFile());
        int dzdx = scratches.obtain();
        int ydzdy = parts.z.reg;
        CONTEXT_LOAD(dzdx,  generated_vars.dzdx);   // 1.31 fixed-point
        CONTEXT_LOAD(ydzdy, iterators.ydzdy);       // 1.31 fixed-point
        MULLW(r0, Rx, dzdx);
        ADD(parts.z.reg, r0, ydzdy);

        // we're going to index zbase of parts.count
        // zbase = base + (xl-count + stride*y)*2
        int Rs = dzdx;
        int zbase = scratches.obtain();
        CONTEXT_LOAD(Rs, state.buffers.depth.stride);
        CONTEXT_LOAD(zbase, state.buffers.depth.data);
        MULLW(r0, Ry, Rs);
        ADD(Rs, r0, Rx);
        SRWI(r0, parts.count.reg, 16);
        ADD(Rs, Rs, r0);
        SLWI(r0, Rs, 1);
        ADD(zbase, zbase, r0);
        CONTEXT_STORE(zbase, generated_vars.zbase);
    }

    // init texture coordinates
    init_textures(parts.coords, reg_t(Rx), reg_t(Ry));
    scratches.recycle(Ry);

    // iterated color
    init_iterated_color(parts, reg_t(Rx));

    // init coverage factor application (anti-aliasing)
    if (mAA) {
        parts.covPtr.setTo(obtainReg(), 16);
        CONTEXT_LOAD(parts.covPtr.reg, state.buffers.coverage);
        SLWI(r0, Rx, 1);
        // XXX: Vlad - subtract 2 from covPtr and then use
        // LHZU instruction in a loop to do an automatic increment
        // ARM has a post-increment option while PPC does only pre-increment
        // in part of LHZU
        ADD(parts.covPtr.reg, parts.covPtr.reg, r0);
    }
}


// ---------------------------------------------------------------------------

void GGLAssembler::build_component( pixel_t& pixel,
                                    const fragment_parts_t& parts,
                                    int component,
                                    Scratch& regs)
{
    static char const * comments[] = {"alpha", "red", "green", "blue"};
    comment(comments[component]);

    // local register file
    Scratch scratches(registerFile());
    const int dst_component_size = pixel.component_size(component);

    component_t temp(-1);
    build_incoming_component( temp, dst_component_size,
            parts, component, scratches, regs);

    if (mInfo[component].inDest) {

        // blending...
        build_blending( temp, mDstPixel, component, scratches );

        // downshift component and rebuild pixel...
        downshift(pixel, component, temp, parts.dither);
    }
}

void GGLAssembler::build_incoming_component(
                                    component_t& temp,
                                    int dst_size,
                                    const fragment_parts_t& parts,
                                    int component,
                                    Scratch& scratches,
                                    Scratch& global_regs)
{
    const uint32_t component_mask = 1<<component;

    // Figure out what we need for the blending stage...
    int fs = component==GGLFormat::ALPHA ? mBlendSrcA : mBlendSrc;
    int fd = component==GGLFormat::ALPHA ? mBlendDstA : mBlendDst;
    if (fs==GGL_SRC_ALPHA_SATURATE && component==GGLFormat::ALPHA) {
        fs = GGL_ONE;
    }

    // Figure out what we need to extract and for what reason
    const int blending = blending_codes(fs, fd);

    // Are we actually going to blend?
    const int need_blending = (fs != int(GGL_ONE)) || (fd > int(GGL_ZERO));

    // expand the source if the destination has more bits
    int need_expander = false;
    for (int i=0 ; i<GGL_TEXTURE_UNIT_COUNT-1 ; i++) {
        texture_unit_t& tmu = mTextureMachine.tmu[i];
        if ((tmu.format_idx) &&
            (parts.texel[i].component_size(component) < dst_size)) {
            need_expander = true;
        }
    }

    // do we need to extract this component?
    const bool multiTexture = mTextureMachine.activeUnits > 1;
    const int blend_needs_alpha_source = (component==GGLFormat::ALPHA) &&
                                        (isAlphaSourceNeeded());
    int need_extract = mInfo[component].needed;
    if (mInfo[component].inDest)
    {
        need_extract |= ((need_blending ?
                (blending & (BLEND_SRC|FACTOR_SRC)) : need_expander));
        need_extract |= (mTextureMachine.mask != mTextureMachine.replaced);
        need_extract |= mInfo[component].smooth;
        need_extract |= mInfo[component].fog;
        need_extract |= mDithering;
        need_extract |= multiTexture;
    }

    if (need_extract) {
        Scratch& regs = blend_needs_alpha_source ? global_regs : scratches;
        component_t fragment;

        // iterated color
        build_iterated_color(fragment, parts, component, regs);

        // texture environement (decal, modulate, replace)
        build_texture_environment(fragment, parts, component, regs);

        // expand the source if the destination has more bits
        if (need_expander && (fragment.size() < dst_size)) {
            // we're here only if we fetched a texel
            // (so we know for sure fragment is CORRUPTIBLE)
            expand(fragment, fragment, dst_size);
        }

        // We have a few specific things to do for the alpha-channel
        if ((component==GGLFormat::ALPHA) &&
            (mInfo[component].needed || fragment.size()<dst_size))
        {
            // convert to integer_t first and make sure
            // we don't corrupt a needed register
            if (fragment.l) {
                component_t incoming(fragment);
                modify(fragment, regs);
                SRWI(fragment.reg, incoming.reg, incoming.l);
                fragment.h -= fragment.l;
                fragment.l = 0;
            }

            // coverage factor application
            build_coverage_application(fragment, parts, regs);

            // alpha-test
            build_alpha_test(fragment, parts);

            if (blend_needs_alpha_source) {
                // We keep only 8 bits for the blending stage
                const int shift = fragment.h <= 8 ? 0 : fragment.h-8;
                if (fragment.flags & CORRUPTIBLE) {
                    fragment.flags &= ~CORRUPTIBLE;
                    mAlphaSource.setTo(fragment.reg,
                            fragment.size(), fragment.flags);
                    if (shift) {
                        SRWI(mAlphaSource.reg, mAlphaSource.reg, shift);
                    }
                } else {
                    // XXX: it would better to do this in build_blend_factor()
                    // so we can avoid the extra MOV below.
                    mAlphaSource.setTo(regs.obtain(),
                            fragment.size(), CORRUPTIBLE);
                    if (shift) {
                        SRWI(mAlphaSource.reg, fragment.reg, shift);
                    } else {
                        MR(mAlphaSource.reg, fragment.reg);
                    }
                }
                mAlphaSource.s -= shift;
            }
        }

        // fog...
        build_fog( fragment, component, regs );

        temp = fragment;
    } else {
        if (mInfo[component].inDest) {
            // extraction not needed and replace
            // we just select the right component
            if ((mTextureMachine.replaced & component_mask) == 0) {
                // component wasn't replaced, so use it!
                temp = component_t(parts.iterated, component);
            }
            for (int i=0 ; i<GGL_TEXTURE_UNIT_COUNT ; i++) {
                const texture_unit_t& tmu = mTextureMachine.tmu[i];
                if ((tmu.mask & component_mask) &&
                    ((tmu.replaced & component_mask) == 0)) {
                    temp = component_t(parts.texel[i], component);
                }
            }
        }
    }
}

bool GGLAssembler::isAlphaSourceNeeded() const
{
    // XXX: also needed for alpha-test
    const int bs = mBlendSrc;
    const int bd = mBlendDst;
    return  bs==GGL_SRC_ALPHA_SATURATE ||
            bs==GGL_SRC_ALPHA || bs==GGL_ONE_MINUS_SRC_ALPHA ||
            bd==GGL_SRC_ALPHA || bd==GGL_ONE_MINUS_SRC_ALPHA ;
}

// ---------------------------------------------------------------------------

void GGLAssembler::build_smooth_shade(const fragment_parts_t& parts)
{
    if (mSmooth && !parts.iterated_packed) {
        // update the iterated color in a pipelined way...
        comment("update iterated color");
        Scratch scratches(registerFile());

        const int reload = parts.reload;
        for (int i=0 ; i<4 ; i++) {
            if (!mInfo[i].iterated)
                continue;

            int c = parts.argb[i].reg;
            int dx = parts.argb_dx[i].reg;

            if (reload & 1) {
                c = scratches.obtain();
                CONTEXT_LOAD(c, generated_vars.argb[i].c);
            }
            if (reload & 2) {
                dx = scratches.obtain();
                CONTEXT_LOAD(dx, generated_vars.argb[i].dx);
            }

            if (mSmooth) {
                ADD(c, c, dx);
            }

            if (reload & 1) {
                CONTEXT_STORE(c, generated_vars.argb[i].c);
                scratches.recycle(c);
            }
            if (reload & 2) {
                scratches.recycle(dx);
            }
        }
    }
}

// ---------------------------------------------------------------------------

void GGLAssembler::build_coverage_application(component_t& fragment,
        const fragment_parts_t& parts, Scratch& regs)
{
    // here fragment.l is guarenteed to be 0
    if (mAA) {
        // coverages are 1.15 fixed-point numbers
        comment("coverage application");

        component_t incoming(fragment);
        modify(fragment, regs);

        Scratch scratches(registerFile());
        int cf = scratches.obtain();
        LHZ(cf, 0, parts.covPtr.reg);
        ADDI(parts.covPtr.reg, parts.covPtr.reg, 2);
        if (fragment.h > 31) {
            fragment.h--;
            MULLW(fragment.reg, incoming.reg, cf);
        } else {
            SLWI(fragment.reg, incoming.reg, 1);
            MULLW(fragment.reg, fragment.reg, cf);
        }
    }
}

// ---------------------------------------------------------------------------

void GGLAssembler::build_alpha_test(component_t& fragment,
                                    const fragment_parts_t& parts)
{
    if (mAlphaTest != GGL_ALWAYS) {
        comment("Alpha Test");
        Scratch scratches(registerFile());
        int ref = scratches.obtain();
        const int shift = GGL_COLOR_BITS-fragment.size();

	// This is a reverse logic test.  For example, if mAlphaTest
	// is GGL_LESS, we test alpha<ref.  If true, we continue, otherwise
	// jump to "discard_after_testures."

        CONTEXT_LOAD(ref, state.alpha_test.ref);
	if (shift)
            SRWI(ref, ref, shift);

        CMPW(cr7, fragment.reg, ref);
        switch (mAlphaTest) {
        case GGL_LESS:
            BGE(cr7, "discard_after_textures");
            break;
        case GGL_LEQUAL:
            BGT(cr7, "discard_after_textures");
            break;
        case GGL_EQUAL:
            BNE(cr7, "discard_after_textures");
            break;
        case GGL_NOTEQUAL:
            BEQ(cr7, "discard_after_textures");
            break;
         case GGL_GEQUAL:
            BLT(cr7, "discard_after_textures");
            break;
        case GGL_GREATER:
            BLE(cr7, "discard_after_textures");
            break;
       case GGL_NEVER:
            B("discard_after_textures");
            break;
        }
    }
}

// ---------------------------------------------------------------------------

void GGLAssembler::build_depth_test(
        const fragment_parts_t& parts, uint32_t mask)
{
    mask &= Z_TEST|Z_WRITE;
    const needs_t& needs = mBuilderContext.needs;
    const int zmask = GGL_READ_NEEDS(P_MASK_Z, needs.p);
    Scratch scratches(registerFile());

    if (mDepthTest != GGL_ALWAYS || zmask) {
        if  (mDepthTest == GGL_NEVER) {
            // this never happens, because it's taken care of when
            // computing the needs. but we keep it for completness.
            comment("Depth Test (NEVER)");
            B("discard_before_textures");
            return;
	}

        if (mDepthTest == GGL_ALWAYS) {
            // we're here because zmask is enabled
            mask &= ~Z_TEST;    // test always passes.
        }

        if ((mask & Z_WRITE) && !zmask) {
            mask &= ~Z_WRITE;
        }

        if (!mask)
            return;

        comment("Depth Test");

        int zbase = scratches.obtain();
        int depth = scratches.obtain();
        int z = parts.z.reg;

        CONTEXT_LOAD(zbase, generated_vars.zbase);
        SRWI(r0, parts.count.reg, 15);
        SUB(zbase, zbase, r0);
            // above does zbase = zbase + ((count >> 16) << 1)

        if (mask & Z_TEST) {
	    // Mirror and invert
	    // For exmaple if mDepthTest is GGL_LESS then
	    // < is mirrored to > and then inverted to <=
            LHZ(depth, 0, zbase);
            SRWI(r0, z, 16);
            CMPW(cr7, depth, r0);
            switch (mDepthTest) {
            case GGL_LESS:
		// branch if depth <= z
                BLE(cr7, "discard_before_textures");
                break;
            case GGL_LEQUAL:
		// branch if depth < z
                BLT(cr7, "discard_before_textures");
                break;
            case GGL_EQUAL:
		// branch if depth != z
                BNE(cr7, "discard_before_textures");
                break;
            case GGL_NOTEQUAL:
		// branch if depth == z
                BEQ(cr7, "discard_before_textures");
                break;
            case GGL_GEQUAL:
		// branch if depth > z
                BGT(cr7, "discard_before_textures");
                break;
            case GGL_GREATER:
                // branch if depth >= z
                BGE(cr7, "discard_before_textures");
                break;
            }
        }

        if (mask & Z_WRITE) {
            // if Z_TEST is set then r0 already contains z >> 16
            if (mask == Z_WRITE)
                SRWI(r0, z, 16);

            // XXX: Vlad - ARM has a conditional store here, which doesn't
            // exist on PPC. However according to logic flow in ARM code this
            // store operation always happens here:
            // if we're here then mask is either Z_WRITE or (Z_WRITE | Z_TEST)
            // Z_WRITE case: ARM code sets "ic" to "AL"
            // (Z_WRITE | Z_TEST) case: "ic" is met <=> "cc" is not met
            // but if "cc" is met than we should have jumped to "discard_before_textures"
            // so if we are here then "cc" is not met => "ic" is met, store always happens
            STH(r0, 0, zbase);
	}
    }
}

void GGLAssembler::build_iterate_z(const fragment_parts_t& parts)
{
    const needs_t& needs = mBuilderContext.needs;
    if ((mDepthTest != GGL_ALWAYS) || GGL_READ_NEEDS(P_MASK_Z, needs.p)) {
        Scratch scratches(registerFile());
        int dzdx = scratches.obtain();
        CONTEXT_LOAD(dzdx, generated_vars.dzdx);    // stall
        ADD(parts.z.reg, parts.z.reg, dzdx);
    }
}

void GGLAssembler::build_iterate_f(const fragment_parts_t& parts)
{
    const needs_t& needs = mBuilderContext.needs;
    if (GGL_READ_NEEDS(P_FOG, needs.p)) {
        Scratch scratches(registerFile());
        int dfdx = scratches.obtain();
        int f = scratches.obtain();
        CONTEXT_LOAD(f,     generated_vars.f);
        CONTEXT_LOAD(dfdx,  generated_vars.dfdx);   // stall
        ADD(f, f, dfdx);
        CONTEXT_STORE(f,    generated_vars.f);
    }
}

// ---------------------------------------------------------------------------

void GGLAssembler::build_logic_op(pixel_t& pixel, Scratch& regs)
{
    const needs_t& needs = mBuilderContext.needs;
    const int opcode = GGL_READ_NEEDS(LOGIC_OP, needs.n) | GGL_CLEAR;
    if (opcode == GGL_COPY)
        return;

    comment("logic operation");

    pixel_t s(pixel);
    if (!(pixel.flags & CORRUPTIBLE)) {
        pixel.reg = regs.obtain();
        pixel.flags |= CORRUPTIBLE;
    }

    pixel_t d(mDstPixel);
    switch(opcode) {
    case GGL_CLEAR:         LI(pixel.reg, 0);    	    	break;
    case GGL_AND:           AND(pixel.reg, s.reg, d.reg);   	break;
    case GGL_AND_REVERSE:   ANDC(pixel.reg, s.reg, d.reg);      break;
    case GGL_COPY:                                          	break;
    case GGL_AND_INVERTED:  ANDC(pixel.reg, d.reg, s.reg);      break;
    case GGL_NOOP:          MR(pixel.reg, d.reg);         	break;
    case GGL_XOR:           XOR(pixel.reg, s.reg, d.reg);   	break;
    case GGL_OR:            OR(pixel.reg, s.reg, d.reg);    	break;
    case GGL_NOR:           NOR(pixel.reg, s.reg, d.reg);   	break;
    case GGL_EQUIV:         EQV(pixel.reg, s.reg, d.reg);       break;
    case GGL_INVERT:        NOT(pixel.reg, d.reg);       	break;
    case GGL_OR_REVERSE:    ORC(pixel.reg, s.reg, d.reg);       break;
    case GGL_COPY_INVERTED: NOT(pixel.reg, s.reg);              break;
    case GGL_OR_INVERTED:   ORC(pixel.reg, d.reg, s.reg);       break;
    case GGL_NAND:          NAND(pixel.reg, s.reg, d.reg);      break;
    case GGL_SET:           LI(pixel.reg, -1);          	break;
    };
}

// ---------------------------------------------------------------------------

static uint32_t find_bottom(uint32_t val)
{
    uint32_t i = 0;
    while (!(val & (3<<i)))
        i+= 2;
    return i;
}

static void normalize(uint32_t& val, uint32_t& rot)
{
    rot = 0;
    while (!(val&3)  || (val & 0xFC000000)) {
        uint32_t newval;
        newval = val >> 2;
        newval |= (val&3) << 30;
        val = newval;
        rot += 2;
        if (rot == 32) {
            rot = 0;
            break;
        }
    }
}

void GGLAssembler::build_and_immediate(int d, int s, uint32_t mask, int bits)
{

    // XXX Vlad: looks like a bug here, bits = 32 is a valid value
    // to mask all 32 bits in a word

    // uint32_t size = ((bits>=32) ? 0 : (1LU << bits)) - 1;
    uint32_t size = ((bits>32) ? 0 : (1LU << bits)) - 1;
    mask &= size;

    if (mask == size) {
        if (d != s)
            MR(d, s);
        return;
    }

    if (mask) {
        if ((mask & 0xffff0000) != 0) {
            LIS(r0, mask >> 16);
            if (mask & 0xffff) {
                ORI(r0, r0, mask & 0xffff);
            }
            AND(d, s, r0);
	}
	else {
            ANDIP(d, s, mask);
	}
    }
    else {
        LI(d, 0);
    }
}

void GGLAssembler::build_masking(pixel_t& pixel, Scratch& regs)
{
    if (!mMasking || mAllMasked) {
        return;
    }

    comment("color mask");

    pixel_t fb(mDstPixel);
    pixel_t s(pixel);
    if (!(pixel.flags & CORRUPTIBLE)) {
        pixel.reg = regs.obtain();
        pixel.flags |= CORRUPTIBLE;
    }

    int mask = 0;
    for (int i=0 ; i<4 ; i++) {
        const int component_mask = 1<<i;
        const int h = fb.format.c[i].h;
        const int l = fb.format.c[i].l;
        if (h && (!(mMasking & component_mask))) {
            mask |= ((1<<(h-l))-1) << l;
        }
    }

    // There is no need to clear the masked components of the source
    // (unless we applied a logic op), because they're already zeroed
    // by construction (masked components are not computed)

    if (mLogicOp) {
        const needs_t& needs = mBuilderContext.needs;
        const int opcode = GGL_READ_NEEDS(LOGIC_OP, needs.n) | GGL_CLEAR;
        if (opcode != GGL_CLEAR) {
            // clear masked component of source
            build_and_immediate(pixel.reg, s.reg, mask, fb.size());
            s = pixel;
        }
    }

    // clear non masked components of destination
    build_and_immediate(fb.reg, fb.reg, ~mask, fb.size());

    // or back the channels that were masked
    if (s.reg == fb.reg) {
         // this is in fact a MOV
        if (s.reg == pixel.reg) {
            // ugh. this in in fact a nop
        } else {
            MR(pixel.reg, fb.reg);
        }
    } else {
        OR(pixel.reg, s.reg, fb.reg);
    }
}

// ---------------------------------------------------------------------------

void GGLAssembler::base_offset(
        const pointer_t& d, const pointer_t& b, const reg_t& o)
{
    switch (b.size) {
    case 32:
        SLWI(r0, o.reg, 2);
        ADD(d.reg, b.reg, r0);
        break;
    case 24:
        if (d.reg == b.reg) {
            SLWI(r0, o.reg, 1);
            ADD(d.reg, b.reg, r0);
            ADD(d.reg, d.reg, o.reg);
        } else {
            SLWI(r0, o.reg, 1);
            ADD(d.reg, o.reg, r0);
            ADD(d.reg, d.reg, b.reg);
        }
        break;
    case 16:
        SLWI(r0, o.reg, 1);
        ADD(d.reg, b.reg, r0);
        break;
    case 8:
        ADD(d.reg, b.reg, o.reg);
        break;
    }
}

// ----------------------------------------------------------------------------
// cheezy register allocator...
// ----------------------------------------------------------------------------

void RegisterAllocator::reset()
{
    mRegs.reset();
}

int RegisterAllocator::reserveReg(int reg)
{
    return mRegs.reserve(reg);
}

int RegisterAllocator::obtainReg()
{
    return mRegs.obtain();
}

void RegisterAllocator::recycleReg(int reg)
{
    mRegs.recycle(reg);
}

RegisterAllocator::RegisterFile& RegisterAllocator::registerFile()
{
    return mRegs;
}

// ----------------------------------------------------------------------------

// The following register should not be used
// r0 - can't be used with any instruction, do not allocate it but can be
//      used explicitely as a scratch register
// r1 - stack pointer
// r2 - reserved according to the ABI
RegisterAllocator::RegisterFile::RegisterFile()
    : mRegs(0), mTouched(0), mStatus(0)
{
    reserve(PPCAssemblerInterface::r0);
    reserve(PPCAssemblerInterface::r2);
    reserve(PPCAssemblerInterface::stkp);
}

RegisterAllocator::RegisterFile::RegisterFile(const RegisterFile& rhs)
    : mRegs(rhs.mRegs), mTouched(rhs.mTouched)
{
}

RegisterAllocator::RegisterFile::~RegisterFile()
{
}

bool RegisterAllocator::RegisterFile::operator == (const RegisterFile& rhs) const
{
    return (mRegs == rhs.mRegs);
}

void RegisterAllocator::RegisterFile::reset()
{
    mRegs = mTouched = mStatus = 0;
    reserve(PPCAssemblerInterface::r0);
    reserve(PPCAssemblerInterface::r2);
    reserve(PPCAssemblerInterface::stkp);
}

int RegisterAllocator::RegisterFile::reserve(int reg)
{
    LOG_ALWAYS_FATAL_IF(isUsed(reg),
                        "reserving register %d, but already in use",
                        reg);
    mRegs |= (1<<reg);
    mTouched |= (1<<reg);

    return reg;
}

void RegisterAllocator::RegisterFile::reserveSeveral(uint32_t regMask)
{
    mRegs |= regMask;
    mTouched |= regMask;
}

int RegisterAllocator::RegisterFile::isUsed(int reg) const
{
    LOG_ALWAYS_FATAL_IF(reg>=32, "invalid register %d", reg);
    return mRegs & (1<<reg);
}

int RegisterAllocator::RegisterFile::obtain()
{
    // r0	 - Can't be used with any instruction
    // r1 	 - Stack pointer
    // r2 	 - Reserved
    // r3	 - Parameter
    // r4 - 12   - Volatile registers
    // r13 - r31 - Nonvolatile registers, must be saved
    // We allocate the nonvolatile registers in reversed order to use
    // stmw/lmw instructions to save them on stack with a single command
    const char priorityList[28] = {  4,  5,  6,  7,  8,  9, 10, 11, 12,
                                    31, 30, 29, 28, 27, 26, 25, 24, 23,
                                    22, 21, 20, 19, 18, 17, 16, 15, 14,
				    13};

    const int nbreg = sizeof(priorityList);
    int i, r;
    for (i=0 ; i<nbreg ; i++) {
        r = priorityList[i];
        if (!isUsed(r)) {
            break;
        }
    }
    // this is not an error anymore because, we'll try again with
    // a lower optimization level.
    LOGE_IF(i >= nbreg, "pixelflinger ran out of registers\n");
    if (i >= nbreg) {
        mStatus |= OUT_OF_REGISTERS;
        // we return SP so we can more easily debug things
        // the code will never be run anyway.
        return PPCAssemblerInterface::stkp;
    }
    reserve(r);
    return r;
}

bool RegisterAllocator::RegisterFile::hasFreeRegs() const
{
    return ((mRegs & 0xFFFFFFFF) == 0xFFFFFFFF) ? false : true;
}

int RegisterAllocator::RegisterFile::countFreeRegs() const
{
    int f = ~mRegs & 0xFFFFFFFF;
    // now count number of 1
    f = (f & 0x55555555) + ((f>>1) & 0x55555555);
    f = (f & 0x33333333) + ((f>>2) & 0x33333333);
    f = (f & 0x0F0F0F0F) + ((f>>4) & 0x0F0F0F0F);
    f = (f & 0x00FF00FF) + ((f>>8) & 0x00FF00FF);
    f = (f & 0x0000FFFF) + ((f>>16) & 0x0000FFFF);
    return f;
}

void RegisterAllocator::RegisterFile::recycle(int reg)
{
    LOG_FATAL_IF(!isUsed(reg),
            "recycling unallocated register %d",
            reg);
    mRegs &= ~(1<<reg);
}

void RegisterAllocator::RegisterFile::recycleSeveral(uint32_t regMask)
{
    LOG_FATAL_IF((mRegs & regMask)!=regMask,
            "recycling unallocated registers "
            "(recycle=%08x, allocated=%08x, unallocated=%08x)",
            regMask, mRegs, mRegs&regMask);
    mRegs &= ~regMask;
}

uint32_t RegisterAllocator::RegisterFile::touched() const
{
    return mTouched;
}

// ----------------------------------------------------------------------------

}; // namespace android
