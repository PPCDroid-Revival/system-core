/* libs/pixelflinger/codeflinger/MIPSAssembler.h
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

#ifndef ANDROID_MIPSASSEMBLER_H
#define ANDROID_MIPSASSEMBLER_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/Vector.h>
#include <utils/KeyedVector.h>

#include "tinyutils/smartpointer.h"
#include "codeflinger-mips/MIPSAssemblerInterface.h"
#include "codeflinger-mips/CodeCache.h"

namespace android {

// ----------------------------------------------------------------------------

class MIPSAssembler : public MIPSAssemblerInterface
{
public:
                MIPSAssembler(const sp<Assembly>& assembly);
    virtual     ~MIPSAssembler();

    uint32_t*   base() const;
    uint32_t*   pc() const;


    void        disassemble(const char* name);

    // ------------------------------------------------------------------------
    // MIPSAssemblerInterface...
    // ------------------------------------------------------------------------

    virtual void    reset();

    virtual int     generate(const char* name);

    virtual void    prolog();
    virtual void    epilog(uint32_t touched);
    virtual void    comment(const char* string);

    virtual void label(const char* theLabel);
    virtual uint32_t* pcForLabel(const char* label);
    virtual bool getAtValue(uint32_t &atv);
    virtual void setAtValue(uint32_t atv);
    virtual char* getLocLabel();

    virtual void ADDIU(int Rd, int Rs, int immval);
    virtual void ADDU(int Rd, int Rs, int Rt);
    virtual void AND(int Rd, int Rs, int Rt);
    virtual void ANDI(int Rd, int Rs, int immval);
    virtual void BEQ(int Rs, int Rt, const char* label,
    		int slot = nopdelay);
    virtual void BGEZ(int Rs, const char* label, int slot = nopdelay);
    virtual void BNE(int Rs, int Rt, const char* label, int slot = nopdelay);
    virtual void JR(int Rs, int slot = nopdelay);
    virtual void LBU(int Rt, int offset, int Rbase);
    virtual void LHU(int Rt, int offset, int Rbase);
    virtual void LUI(int Rt, int immval);
    virtual void LW(int Rt, int offset, int Rbase);
    virtual void MFHI(int Rd);
    virtual void MFLO(int Rd);
    virtual void MOVN(int Rd, int Rs, int Rt);
    virtual void MOVZ(int Rd, int Rs, int Rt);
    virtual void MUL(int Rd, int Rs, int Rt);
    virtual void MULT(int Rs, int Rt);
    virtual void NOR(int Rd, int Rs, int Rt);
    virtual void OR(int Rd, int Rs, int Rt);
    virtual void ORI(int Rd, int Rs, int immval);
    virtual void SB(int Rt, int offset, int Rbase);
    virtual void SH(int Rt, int offset, int Rbase);
    virtual void SLL(int Rt, int Rd, int shift);
    virtual void SLT(int Rd, int Rs, int Rt);
    virtual void SRA(int Rt, int Rd, int shift);
    virtual void SRL(int Rt, int Rd, int shift);
    virtual void SUBU(int Rd, int Rs, int Rt);
    virtual void SW(int Rt, int offset, int Rbase);
    virtual void XOR(int Rd, int Rs, int Rt);

private:
                MIPSAssembler(const MIPSAssembler& rhs);
                MIPSAssembler& operator = (const MIPSAssembler& rhs);

    sp<Assembly>    mAssembly;
    uint32_t*       mBase;
    uint32_t*       mPC;
    uint32_t*       mPrologPC;
    int64_t         mDuration;
    bool	    mAtValid;
    uint32_t	    mAtValue;
    char*	    mLocSymBase;
    char*	    mLocPtr;
    uint32_t	    mLocCnt;
#if defined(WITH_LIB_HARDWARE)
    bool            mQemuTracing;
#endif
    
    struct branch_target_t {
        inline branch_target_t() : label(0), pc(0) { }
        inline branch_target_t(const char* l, uint32_t* p)
            : label(l), pc(p) { }
        const char* label;
        uint32_t*   pc;
    };
    
    Vector<branch_target_t>                 mBranchTargets;
    KeyedVector< const char*, uint32_t* >   mLabels;
    KeyedVector< uint32_t*, const char* >   mLabelsInverseMapping;
    KeyedVector< uint32_t*, const char* >   mComments;
};

}; // namespace android

#endif //ANDROID_MIPSASSEMBLER_H
