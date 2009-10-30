/* libs/pixelflinger/codeflinger/PPCAssembler.cpp
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

#define LOG_TAG "PPCAssembler"

#include <stdio.h>
#include <stdlib.h>
#include <cutils/log.h>
#include <cutils/properties.h>

#if defined(WITH_LIB_HARDWARE)
#include <hardware_legacy/qemu_tracing.h>
#endif

#include <private/pixelflinger/ggl_context.h>

#include "codeflinger-ppc/PPCAssembler.h"
#include "codeflinger-ppc/CodeCache.h"
#include "codeflinger-ppc/disassem.h"

// ----------------------------------------------------------------------------

namespace android {

// ----------------------------------------------------------------------------
#if 0
#pragma mark -
#pragma mark PPCAssembler...
#endif

PPCAssembler::PPCAssembler(const sp<Assembly>& assembly)
    :   PPCAssemblerInterface(),
        mAssembly(assembly)
{
    mBase = mPC = (uint32_t *)assembly->base();
    mDuration = ggl_system_time();
    mLocSymBase = mLocPtr = (char *)malloc(512);
    mLocCnt = 0;
#if defined(WITH_LIB_HARDWARE)
    mQemuTracing = true;
#endif
}

PPCAssembler::~PPCAssembler()
{
    free(mLocSymBase);
}

uint32_t* PPCAssembler::pc() const
{
    return mPC;
}

uint32_t* PPCAssembler::base() const
{
    return mBase;
}

void PPCAssembler::reset()
{
    mBase = mPC = (uint32_t *)mAssembly->base();
    mBranchTargets.clear();
    mLabels.clear();
    mLabelsInverseMapping.clear();
    mComments.clear();
    mLocPtr = mLocSymBase;
    mLocCnt = 0;
}

// ----------------------------------------------------------------------------

void PPCAssembler::disassemble(const char* name)
{
    if (name) {
        printf("%s:\n", name);
    }
    size_t count = pc()-base();
    uint32_t* i = base();
    while (count--) {
        ssize_t label = mLabelsInverseMapping.indexOfKey(i);
        if (label >= 0) {
            printf("%s:\n", mLabelsInverseMapping.valueAt(label));
        }
        ssize_t comment = mComments.indexOfKey(i);
        if (comment >= 0) {
            printf("; %s\n", mComments.valueAt(comment));
        }
        printf("%08x:    %08x    ", int(i), int(i[0]));
        ::disassemble((u_int)i);
        i++;
    }
}

void PPCAssembler::comment(const char* string)
{
    mComments.add(mPC, string);
}

void PPCAssembler::label(const char* theLabel)
{
    mLabels.add(theLabel, mPC);
    mLabelsInverseMapping.add(mPC, theLabel);
}

char* PPCAssembler::getLocLabel()
{
    char *rv;

    mLocCnt++;
    rv = mLocPtr;
    mLocPtr += sprintf(rv, ".L%d", mLocCnt) + 1;
    return rv;
}

#if 0
#pragma mark -
#pragma mark Prolog/Epilog & Generate...
#endif

// Allocate space for two software instructions
// stwu	r1, -96(r1)
// stmw rN, (96-4*(32-N))(r1)
// Where N is a numer of registers to save: R[31..31-N+1]
// We allocate 96 Bytes stack frame now to be able to save
// all 19 volatile registers (r13 - r31)
// If there are no registers to save just put NOP

void PPCAssembler::prolog()
{
    mPrologPC = mPC;
    mPC += 2;
}

void PPCAssembler::epilog(uint32_t touched)
{
    uint32_t*	savepc;

    savepc = mPC;
    mPC = mPrologPC;

    touched &= LSAVED;
    if (touched) {
        int start = ffs(touched) - 1;
        int offset = 96 - 4 * (32 - start);
        STWU(stkp, -96, stkp);
	STMW(start, offset, stkp);
        mPC = savepc;
        LMW(start, offset, stkp);
        ADDI(stkp, stkp, 96);
        BLR();
    }
    else {
        STWU(stkp, -96, stkp);
        NOP();
        mPC = savepc;
        ADDI(stkp, stkp, 96);
        BLR();
    }
}

int PPCAssembler::generate(const char* name)
{
    // fixup all the branches
    size_t count = mBranchTargets.size();
    while (count--) {
        const branch_target_t& bt = mBranchTargets[count];
        uint32_t* target_pc = mLabels.valueFor(bt.label);
        LOG_ALWAYS_FATAL_IF(!target_pc,
                "error resolving branch targets, target_pc is null");
        int32_t offset = int32_t((uint32_t)target_pc - (uint32_t)bt.pc);
        *bt.pc |= offset & 0xFFFC;
    }

    mAssembly->resize( int(pc()-base())*4 );

    // the instruction cache is flushed by CodeCache
    const int64_t duration = ggl_system_time() - mDuration;
    const char * const format = "generated %s (%d ins) at [%p:%p] in %lld ns\n";
    LOGI(format, name, int(pc()-base()), base(), pc(), duration);

#if defined(WITH_LIB_HARDWARE)
    if (__builtin_expect(mQemuTracing, 0)) {
        int err = qemu_add_mapping(int(base()), name);
        mQemuTracing = (err >= 0);
    }
#endif

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.pf.disasm", value, "0");
    if (atoi(value) != 0) {
        printf(format, name, int(pc()-base()), base(), pc(), duration);
        disassemble(name);
    }

    return NO_ERROR;
}

uint32_t* PPCAssembler::pcForLabel(const char* label)
{
    return mLabels.valueFor(label);
}

// ----------------------------------------------------------------------------

#if 0
#pragma mark -
#pragma mark Instructions...
#endif

// Here we implement PowerPC instructions

// Instruction formats
uint32_t PPCAssembler::B_form(int Op, int f1, int f2, int f3, int f4, int f5)
{
	uint32_t instr;
	instr = ((Op & 0x3f) << 26) | ((f1 & 0x1f) << 21) |
                ((f2 & 0x1f) << 16) | ((f3 & 0x3fff) << 2) |
                ((f4 & 0x1) << 1)   | (f5 & 0x1);

	return instr;
}

uint32_t PPCAssembler::D_form(int Op, int f1, int f2, int f3)
{
	uint32_t instr;
	instr = ((Op & 0x3f) << 26) | ((f1 & 0x1f) << 21) |
                ((f2 & 0x1f) << 16) | (f3 & 0xffff);

	return instr;
}

uint32_t PPCAssembler::I_form(int Op, int f1, int f2, int f3)
{
	uint32_t instr;
	instr = ((Op & 0x3f) << 26) | ((f1 & 0xffffff) << 2) |
                ((f2 & 0x1) << 1)   | (f3 & 0x1);

	return instr;
}

uint32_t PPCAssembler::M_form(int Op, int f1, int f2,
                              int f3, int f4, int f5, int f6)
{
	uint32_t instr;
	instr = ((Op & 0x3f) << 26) | ((f1 & 0x1f) << 21) |
                ((f2 & 0x1f) << 16) | ((f3 & 0x1f) << 11) |
                ((f4 & 0x1f) << 6)  | ((f5 & 0x1f) << 1) |
                (f6 & 0x1);

	return instr;
}

uint32_t PPCAssembler::X_form(int Op, int f1, int f2, int f3, int f4, int f5)
{
	uint32_t instr;
	instr = ((Op & 0x3f) << 26) | ((f1 & 0x1f) << 21) |
                ((f2 & 0x1f) << 16) | ((f3 & 0x1f) << 11) |
                ((f4 & 0x3ff) << 1) | (f5 & 0x1);

	return instr;
}

uint32_t PPCAssembler::XL_form(int Op, int f1, int f2, int f3, int f4, int f5)
{
	uint32_t instr;
	instr = X_form(Op, f1, f2, f3, f4, f5);
	return instr;
}

uint32_t PPCAssembler::XO_form(int Op, int f1, int f2,
                               int f3, int f4, int f5, int f6)
{
	uint32_t instr;
	instr = ((Op & 0x3f) << 26) | ((f1 & 0x1f) << 21) |
                ((f2 & 0x1f) << 16) | ((f3 & 0x1f) << 11) |
                ((f4 & 0x1) << 10)  | ((f5 & 0x1ff) << 1) |
                (f6 & 0x1);

	return instr;
}

uint32_t PPCAssembler::XFX_form(int Op, int f1, int f2, int f3)
{
	uint32_t instr;
	instr = ((Op & 0x3f) << 26) | ((f1 & 0x1f) << 21) |
                ((f2 & 0x3ff) << 11) | ((f3 & 0x3ff) << 1);

	return instr;
}

// SPR registers access
void PPCAssembler::MTSPR(int Spr, int Rs)
{
    *mPC++ = XFX_form(31, Rs, ((Spr & 0x1f) << 5) | ((Spr >> 5) & 0x1f), 467);
}

// Branch instructions
void PPCAssembler::B(const char *label)
{
    mBranchTargets.add(branch_target_t(label, mPC));
    *mPC++ = I_form(18, 0, 0, 0);
}

void PPCAssembler::BC(int Bo, int Bi, const char *label)
{
    mBranchTargets.add(branch_target_t(label, mPC));
    *mPC++ = B_form(16, Bo, Bi, 0, 0, 0);
}

void PPCAssembler::BCLR(int Bo, int Bi, int Bh)
{
    *mPC++ = XL_form(19, Bo, Bi, Bh & 0x3, 16, 0);
}

// Load-store instructions
void PPCAssembler::LBZ(int Rt, int D, int Ra)
{
    *mPC++ = D_form(34, Rt, Ra, D);
}

void PPCAssembler::LBZX(int Rt, int Ra, int Rb)
{
    *mPC++ = X_form(31, Rt, Ra, Rb, 87, 0);
}

void PPCAssembler::LHZ(int Rt, int D, int Ra)
{
    *mPC++ = D_form(40, Rt, Ra, D);
}

void PPCAssembler::LHZX(int Rt, int Ra, int Rb)
{
    *mPC++ = X_form(31, Rt, Ra, Rb, 279, 0);
}

void PPCAssembler::LWZ(int Rt, int D, int Ra)
{
    *mPC++ = D_form(32, Rt, Ra, D);
}

void PPCAssembler::LWZX(int Rt, int Ra, int Rb)
{
    *mPC++ = X_form(31, Rt, Ra, Rb, 23, 0);
}

void PPCAssembler::LMW(int Rt, int D, int Ra)
{
    *mPC++ = D_form(46, Rt, Ra, D);
}

void PPCAssembler::STB(int Rs, int D, int Ra)
{
    *mPC++ = D_form(38, Rs, Ra, D);
}

void PPCAssembler::STH(int Rs, int D, int Ra)
{
    *mPC++ = D_form(44, Rs, Ra, D);
}

void PPCAssembler::STW(int Rs, int D, int Ra)
{
    *mPC++ = D_form(36, Rs, Ra, D);
}

void PPCAssembler::STWU(int Rs, int D, int Ra)
{
    *mPC++ = D_form(37, Rs, Ra, D);
}

void PPCAssembler::STMW(int Rs, int D, int Ra)
{
    *mPC++ = D_form(47, Rs, Ra, D);
}

//Load-store instructions with reversed order
void PPCAssembler::LHBRX(int Rt, int Ra, int Rb)
{
    *mPC++ = X_form(31, Rt, Ra, Rb, 790, 0);
}

void PPCAssembler::LWBRX(int Rt, int Ra, int Rb)
{
    *mPC++ = X_form(31, Rt, Ra, Rb, 534, 0);
}

void PPCAssembler::STHBRX(int Rs, int Ra, int Rb)
{
    *mPC++ = X_form(31, Rs, Ra, Rb, 918, 0);
}

void PPCAssembler::STWBRX(int Rs, int Ra, int Rb)
{
    *mPC++ = X_form(31, Rs, Ra, Rb, 662, 0);
}

// Arithmetic instructions
void PPCAssembler::ADDI(int Rt, int Ra, int Si)
{
    *mPC++ = D_form(14, Rt, Ra, Si);
}

void PPCAssembler::ADDIS(int Rt, int Ra, int Si)
{
    *mPC++ = D_form(15, Rt, Ra, Si);
}

void PPCAssembler::ADD(int Rt, int Ra, int Rb)
{
    *mPC++ = XO_form(31, Rt, Ra, Rb, 0, 266, 0);
}

void PPCAssembler::ADDP(int Rt, int Ra, int Rb)
{
    *mPC++ = XO_form(31, Rt, Ra, Rb, 0, 266, 1);
}

void PPCAssembler::SUBF(int Rt, int Ra, int Rb)
{
    *mPC++ = XO_form(31, Rt, Ra, Rb, 0, 40, 0);
}

void PPCAssembler::NEG(int Rt, int Ra)
{
    *mPC++ = XO_form(31, Rt, Ra, 0, 0, 104, 0);
}

void PPCAssembler::MULLW(int Rt, int Ra, int Rb)
{
    *mPC++ = XO_form(31, Rt, Ra, Rb, 0, 235, 0);
}

void PPCAssembler::MULHW(int Rt, int Ra, int Rb)
{
    *mPC++ = XO_form(31, Rt, Ra, Rb, 0, 75, 0);
}

void PPCAssembler::MULHWU(int Rt, int Ra, int Rb)
{
    *mPC++ = XO_form(31, Rt, Ra, Rb, 0, 11, 0);
}

// Compare instructions
void PPCAssembler::CMP(int Bf, int L, int Ra, int Rb)
{
    *mPC++ = X_form(31, ((Bf & 0x7) << 2) | (L & 0x1), Ra, Rb, 0, 0);
}

void PPCAssembler::CMPL(int Bf, int L, int Ra, int Rb)
{
    *mPC++ = X_form(31, ((Bf & 0x7) << 2) | (L & 0x1), Ra, Rb, 32, 0);
}

// Logical instructions
void PPCAssembler::ANDIP(int Ra, int Rs, int Ui)
{
	*mPC++ = D_form(28, Rs, Ra, Ui);
}

void PPCAssembler::ORI(int Ra, int Rs, int Ui)
{
    *mPC++ = D_form(24, Rs, Ra, Ui);
}

void PPCAssembler::AND(int Ra, int Rs, int Rb)
{
    *mPC++ = X_form(31, Rs, Ra, Rb, 28, 0);
}

void PPCAssembler::OR(int Ra, int Rs, int Rb)
{
    *mPC++ = X_form(31, Rs, Ra, Rb, 444, 0);
}

void PPCAssembler::XOR(int Ra, int Rs, int Rb)
{
    *mPC++ = X_form(31, Rs, Ra, Rb, 316, 0);
}

void PPCAssembler::NAND(int Ra, int Rs, int Rb)
{
    *mPC++ = X_form(31, Rs, Ra, Rb, 476, 0);
}

void PPCAssembler::NOR(int Ra, int Rs, int Rb)
{
    *mPC++ = X_form(31, Rs, Ra, Rb, 124, 0);
}

void PPCAssembler::EQV(int Ra, int Rs, int Rb)
{
    *mPC++ = X_form(31, Rs, Ra, Rb, 284, 0);
}

void PPCAssembler::ANDC(int Ra, int Rs, int Rb)
{
    *mPC++ = X_form(31, Rs, Ra, Rb, 60, 0);
}

void PPCAssembler::ORC(int Ra, int Rs, int Rb)
{
    *mPC++ = X_form(31, Rs, Ra, Rb, 412, 0);
}

// Rotate instructions
void PPCAssembler::RLWINM(int Ra, int Rs, int Sh, int Mb, int Me)
{
    *mPC++ = M_form(21, Rs, Ra, Sh, Mb, Me, 0);
}

// Shift instructions
void PPCAssembler::SRW(int Ra, int Rs, int Rb)
{
    *mPC++ = X_form(31, Rs, Ra, Rb, 536, 0);
}

void PPCAssembler::SRAWI(int Ra, int Rs, int Sh)
{
    *mPC++ = X_form(31, Rs, Ra, Sh, 824, 0);
}

void PPCAssembler::SRAWIP(int Ra, int Rs, int Sh)
{
    *mPC++ = X_form(31, Rs, Ra, Sh, 824, 1);
}

}; // namespace android
