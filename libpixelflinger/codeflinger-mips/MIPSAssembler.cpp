/* libs/pixelflinger/codeflinger/MIPSAssembler.cpp
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

#define LOG_TAG "MIPSAssembler"

#include <stdio.h>
#include <stdlib.h>
#include <cutils/log.h>
#include <cutils/properties.h>

#if defined(WITH_LIB_HARDWARE)
#include <hardware_legacy/qemu_tracing.h>
#endif

#include <private/pixelflinger/ggl_context.h>

#include "codeflinger-mips/MIPSAssembler.h"
#include "codeflinger-mips/CodeCache.h"
#include "codeflinger-mips/disassem.h"

// ----------------------------------------------------------------------------

namespace android {

// ----------------------------------------------------------------------------
#if 0
#pragma mark -
#pragma mark MIPSAssembler...
#endif

MIPSAssembler::MIPSAssembler(const sp<Assembly>& assembly)
    :   MIPSAssemblerInterface(),
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

MIPSAssembler::~MIPSAssembler()
{
    free(mLocSymBase);
}

uint32_t* MIPSAssembler::pc() const
{
    return mPC;
}

uint32_t* MIPSAssembler::base() const
{
    return mBase;
}

void MIPSAssembler::reset()
{
    mBase = mPC = (uint32_t *)mAssembly->membase();
    mBranchTargets.clear();
    mLabels.clear();
    mLabelsInverseMapping.clear();
    mComments.clear();
    mAtValid = false;
    mLocPtr = mLocSymBase;
    mLocCnt = 0;
}

// ----------------------------------------------------------------------------

void MIPSAssembler::disassemble(const char* name)
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

void MIPSAssembler::comment(const char* string)
{
    mComments.add(mPC, string);
}

void MIPSAssembler::label(const char* theLabel)
{
    mLabels.add(theLabel, mPC);
    mLabelsInverseMapping.add(mPC, theLabel);
}

char* MIPSAssembler::getLocLabel()
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


void MIPSAssembler::prolog()
{
    // Allocate space for up to 9 sw instructions plus one stack adjustment
    mPC += 10;
    mPrologPC = mPC;
}

void MIPSAssembler::epilog(uint32_t touched)
{
    uint32_t	regtemp, count;
    uint32_t*	savepc;
    int sreg;

    touched &= LSAVED;
    if (touched) {
        // write prolog code
	// If Spill is used, it assumes it has four register save slots
	// at the base of the stack.

	count = 0;
	regtemp = touched;
	savepc = mPC;
        mPC = mPrologPC;
	mPC--;
	while (regtemp) {
	    sreg = 31 - __builtin_clz(regtemp);
	    regtemp &= ~(1 << sreg);
	    SW(sreg, (count *4)+16, stkp);
//	    SW(sreg, (count *4), stkp);
	    count++;
	    mPC -= 2;
	}
	ADDIU(stkp, stkp, -(count*4));
	mPC--;
        mAssembly->setBase(mPC);

	// Write eplilog
	mPC = savepc;
	count = 0;
	regtemp = touched;
	while (regtemp) {
	    sreg = 31 - __builtin_clz(regtemp);
	    regtemp &= ~(1 << sreg);
	    LW(sreg, (count *4)+16, stkp);
//	    LW(sreg, (count *4), stkp);
	    count++;
	}
        JR(ra, usedelay);
	ADDIU(stkp, stkp, (count*4));

    }
    else {
        mAssembly->setBase(mPrologPC);
        JR(ra);
    }
}

int MIPSAssembler::generate(const char* name)
{
    // fixup all the branches
    size_t count = mBranchTargets.size();
    while (count--) {
        const branch_target_t& bt = mBranchTargets[count];
        uint32_t* target_pc = mLabels.valueFor(bt.label);
        LOG_ALWAYS_FATAL_IF(!target_pc,
                "error resolving branch targets, target_pc is null");
        int32_t offset = int32_t(target_pc - (bt.pc+1));
        *bt.pc |= offset & 0xFFFF;
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

uint32_t* MIPSAssembler::pcForLabel(const char* label)
{
    return mLabels.valueFor(label);
}

bool MIPSAssembler::getAtValue(uint32_t &atv)
{
    atv = mAtValue;
    return mAtValid;
}

void MIPSAssembler::setAtValue(uint32_t atv)
{
    mAtValue = atv;
    mAtValid = true;
}

// ----------------------------------------------------------------------------

#if 0
#pragma mark -
#pragma mark Instructions...
#endif

void MIPSAssembler::ADDIU(int Rt, int Rs, int immval)
{
    *mPC++ =    (0x09 << 26) | (Rs<<21) | (Rt<<16) | (immval & 0xffff);
    if (Rt == at)
    	mAtValid = false;
}

void MIPSAssembler::ADDU(int Rd, int Rs, int Rt)
{
    *mPC++ =    (Rs<<21) | (Rt<<16) | (Rd<<11) | 0x21;
    if (Rt == at)
    	mAtValid = false;
}

void MIPSAssembler::AND(int Rd, int Rs, int Rt)
{
    *mPC++ =    (Rs<<21) | (Rt<<16) | (Rd<<11) | 0x24;
    if (Rd == at)
    	mAtValid = false;
}

void MIPSAssembler::ANDI(int Rt, int Rs, int immval)
{
    LOG_FATAL_IF((immval & 0xffff0000), "ANDI(r%u,r%u,0x%u)", Rt,Rs,immval);
    *mPC++ =    (0x0c << 26) | (Rs<<21) | (Rt<<16) | immval;
    if (Rt == at)
    	mAtValid = false;
}

void MIPSAssembler::BEQ(int Rs, int Rt, const char* label, int slot)
{
    mBranchTargets.add(branch_target_t(label, mPC));
    *mPC++ =    (0x04 << 26) | (Rs<<21) | (Rt<<16);
    if (slot == nopdelay)
    	*mPC++ = 0;
}

void MIPSAssembler::BGEZ(int Rs, const char* label, int slot)
{
    mBranchTargets.add(branch_target_t(label, mPC));
    *mPC++ =    (0x01 << 26) | (Rs<<21) | (0x01<<16);
    if (slot == nopdelay)
    	*mPC++ = 0;
}

void MIPSAssembler::BNE(int Rs, int Rt, const char* label, int slot)
{
    mBranchTargets.add(branch_target_t(label, mPC));
    *mPC++ =    (0x05 << 26) | (Rs<<21) | (Rt<<16);
    if (slot == nopdelay)
    	*mPC++ = 0;
}

void MIPSAssembler::JR(int Rs, int slot)
{
    *mPC++ =    (Rs<<21) | 0x08;
    if (slot == nopdelay)
        *mPC++ = 0;
}

void MIPSAssembler::LBU(int Rt, int offset, int Rbase)
{
    LOG_FATAL_IF((offset & 0xffff0000), "LBU(r%u,r%u,0x%u)", Rt,Rbase,offset);
    *mPC++ =    (0x24 << 26) | (Rbase<<21) | (Rt<<16) | offset;
    if (Rt == at)
    	mAtValid = false;
}

void MIPSAssembler::LHU(int Rt, int offset, int Rbase)
{
    LOG_FATAL_IF((offset & 0xffff0000), "LHU(r%u,r%u,0x%u)", Rt,Rbase,offset);
    *mPC++ =    (0x25 << 26) | (Rbase<<21) | (Rt<<16) | offset;
    if (Rt == at)
    	mAtValid = false;
}

void MIPSAssembler::LUI(int Rt, int immval)
{
    LOG_FATAL_IF((immval & 0xffff0000), "LUI(r%u,0x%u)", Rt,immval);
    *mPC++ =    (0x0f << 26) | (Rt<<16) | immval;
    if (Rt == at)
    	mAtValid = false;
}

void MIPSAssembler::LW(int Rt, int offset, int Rbase)
{
    LOG_FATAL_IF((offset & 0xffff0000), "LW(r%u,r%u,0x%u)", Rt,Rbase,offset);
    *mPC++ =    (0x23 << 26) | (Rbase<<21) | (Rt<<16) | offset;
    if (Rt == at)
    	mAtValid = false;
}

void MIPSAssembler::MFHI(int Rd)
{
    *mPC++ =    (Rd<<11) | 0x10;
    if (Rd == at)
    	mAtValid = false;
}

void MIPSAssembler::MFLO(int Rd)
{
    *mPC++ =    (Rd<<11) | 0x12;
    if (Rd == at)
    	mAtValid = false;
}

void MIPSAssembler::MOVN(int Rd, int Rs, int Rt)
{
    *mPC++ =    (Rs<<21) | (Rt<<16) | (Rd<<11) | 0x0b;
    if (Rd == at)
    	mAtValid = false;
}

void MIPSAssembler::MOVZ(int Rd, int Rs, int Rt)
{
    *mPC++ =    (Rs<<21) | (Rt<<16) | (Rd<<11) | 0x0a;
    if (Rd == at)
    	mAtValid = false;
}

void MIPSAssembler::MUL(int Rd, int Rs, int Rt)
{
    *mPC++ =    (0x1c << 26) | (Rs<<21) | (Rt<<16) | (Rd<<11) | 0x02;
    if (Rd == at)
    	mAtValid = false;
}

void MIPSAssembler::MULT(int Rs, int Rt)
{
    *mPC++ =    (Rs<<21) | (Rt<<16) | 0x18;
}

void MIPSAssembler::NOR(int Rd, int Rs, int Rt)
{
    *mPC++ =    (Rs<<21) | (Rt<<16) | (Rd<<11) | 0x27;
    if (Rd == at)
    	mAtValid = false;
}

void MIPSAssembler::OR(int Rd, int Rs, int Rt)
{
    *mPC++ =    (Rs<<21) | (Rt<<16) | (Rd<<11) | 0x25;
    if (Rd == at)
    	mAtValid = false;
}

void MIPSAssembler::ORI(int Rt, int Rs, int immval)
{
    LOG_FATAL_IF((immval & 0xffff0000), "ORI(r%u,r%u,0x%u)", Rt,Rs,immval);
    *mPC++ =    (0x0d << 26) | (Rs<<21) | (Rt<<16) | immval;
    if (Rt == at)
    	mAtValid = false;
}

void MIPSAssembler::SB(int Rt, int offset, int Rbase)
{
    LOG_FATAL_IF((offset & 0xffff0000), "SB(r%u,r%u,0x%u)", Rt,Rbase,offset);
    *mPC++ =    (0x28 << 26) | (Rbase<<21) | (Rt<<16) | offset;
}

void MIPSAssembler::SH(int Rt, int offset, int Rbase)
{
    LOG_FATAL_IF((offset & 0xffff0000), "SH(r%u,r%u,0x%u)", Rt,Rbase,offset);
    *mPC++ =    (0x29 << 26) | (Rbase<<21) | (Rt<<16) | offset;
}

void MIPSAssembler::SLL(int Rd, int Rt, int shift)
{
    LOG_FATAL_IF((shift > 31), "SLL(r%u,r%u,0x%u)", Rt,Rd,shift);
    *mPC++ =    (Rt<<16) | (Rd<<11) | (shift << 6);
    if (Rd == at)
    	mAtValid = false;
}

void MIPSAssembler::SLT(int Rd, int Rs, int Rt)
{
    *mPC++ =    (Rs << 21) | (Rt<<16) | (Rd<<11) | 0x2a;
    if (Rd == at)
    	mAtValid = false;
}

void MIPSAssembler::SRA(int Rd, int Rt, int shift)
{
    LOG_FATAL_IF((shift > 31), "SRA(r%u,r%u,0x%u)", Rt,Rd,shift);
    *mPC++ =    (Rt<<16) | (Rd<<11) | (shift << 6) | 0x03;
    if (Rd == at)
    	mAtValid = false;
}

void MIPSAssembler::SRL(int Rd, int Rt, int shift)
{
    LOG_FATAL_IF((shift > 31), "SRL(r%u,r%u,0x%u)", Rt,Rd,shift);
    *mPC++ =    (Rt<<16) | (Rd<<11) | (shift << 6) | 0x02;
    if (Rd == at)
    	mAtValid = false;
}

void MIPSAssembler::SUBU(int Rd, int Rs, int Rt)
{
    *mPC++ =    (Rs<<21) | (Rt<<16) | (Rd<<11) | 0x23;
    if (Rd == at)
    	mAtValid = false;
}

void MIPSAssembler::SW(int Rt, int offset, int Rbase)
{
    LOG_FATAL_IF((offset & 0xffff0000), "SW(r%u,r%u,0x%u)", Rt,Rbase,offset);
    *mPC++ =    (0x2b << 26) | (Rbase<<21) | (Rt<<16) | offset;
}

void MIPSAssembler::XOR(int Rd, int Rs, int Rt)
{
    *mPC++ =    (Rs<<21) | (Rt<<16) | (Rd<<11) | 0x26;
    if (Rd == at)
    	mAtValid = false;
}

}; // namespace android

