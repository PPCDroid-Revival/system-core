/* libs/pixelflinger/codeflinger/PPCAssemblerProxy.h
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


#ifndef ANDROID_PPCASSEMBLER_PROXY_H
#define ANDROID_PPCASSEMBLER_PROXY_H

#include <stdint.h>
#include <sys/types.h>

#include "codeflinger-powerpc/PPCAssemblerInterface.h"

namespace android {

// ----------------------------------------------------------------------------

class PPCAssemblerProxy : public PPCAssemblerInterface
{
public:
    // PPCAssemblerProxy take ownership of the target

                PPCAssemblerProxy();
                PPCAssemblerProxy(PPCAssemblerInterface* target);
    virtual     ~PPCAssemblerProxy();

    void setTarget(PPCAssemblerInterface* target);

    virtual void    reset();
    virtual int     generate(const char* name);
    virtual void    disassemble(const char* name);

    virtual void    prolog();
    virtual void    epilog(uint32_t touched);
    virtual void    comment(const char* string);

    virtual void label(const char* theLabel);
    uint32_t* pcForLabel(const char* label);
    virtual char* getLocLabel();

    // SPR registers access
    virtual void MTSPR(int Spr, int Rs);

    // Branch instructions
    virtual void B(const char *label);
    virtual void BC(int Bo, int Bi, const char *label);
    virtual void BCLR(int Bo, int Bi, int Bh);

    // Load-store instructions
    virtual void LBZ(int Rt, int D, int Ra);
    virtual void LBZX(int Rt, int Ra, int Rb);
    virtual void LHZ(int Rt, int D, int Ra);
    virtual void LHZX(int Rt, int Ra, int Rb);
    virtual void LWZ(int Rt, int D, int Ra);
    virtual void LWZX(int Rt, int Ra, int Rb);
    virtual void LMW(int Rt, int D, int Ra);
    virtual void STB(int Rs, int D, int Ra);
    virtual void STH(int Rs, int D, int Ra);
    virtual void STW(int Rs, int D, int Ra);
    virtual void STWU(int Rs, int D, int Ra);
    virtual void STMW(int Rs, int D, int Ra);

    //Arithmetic instructions
    virtual void ADDI(int Rt, int Ra, int Si);
    virtual void ADDIS(int Rt, int Ra, int Si);
    virtual void ADD(int Rt, int Ra, int Rb);
    virtual void ADDP(int Rt, int Ra, int Rb);
    virtual void SUBF(int Rt, int Ra, int Rb);
    virtual void NEG(int Rt, int Ra);
    virtual void MULLW(int Rt, int Ra, int Rb);
    virtual void MULHW(int Rt, int Ra, int Rb);
    virtual void MULHWU(int Rt, int Ra, int Rb);

    //compare instructions
    virtual void CMP(int Bf, int L, int Ra, int Rb);
    virtual void CMPL(int Bf, int L, int Ra, int Rb);

    //Logical instructions
    virtual void ANDIP(int Ra, int Rs, int Ui);
    virtual void ORI(int Ra, int Rs, int Ui);
    virtual void AND(int Ra, int Rs, int Rb);
    virtual void XOR(int Ra, int Rs, int Rb);
    virtual void NAND(int Ra, int Rs, int Rb);
    virtual void OR(int Ra, int Rs, int Rb);
    virtual void NOR(int Ra, int Rs, int Rb);
    virtual void EQV(int Ra, int Rs, int Rb);
    virtual void ANDC(int Ra, int Rs, int Rb);
    virtual void ORC(int Ra, int Rs, int Rb);

    //Load-store instructions with reversed order
    virtual void LHBRX(int Rt, int Ra, int Rb);
    virtual void LWBRX(int Rt, int Ra, int Rb);
    virtual void STHBRX(int Rs, int Ra, int Rb);
    virtual void STWBRX(int Rs, int Ra, int Rb);


    //Rotate&Mask instructions
    virtual void RLWINM(int Ra, int Rs, int Sh, int Mb, int Me);

    //Shift instructions
    virtual void SRW(int Ra, int Rs, int Rb);
    virtual void SRAWI(int Ra, int Rs, int Sh);
    virtual void SRAWIP(int Ra, int Rs, int Sh);

private:
    PPCAssemblerInterface*  mTarget;
};

}; // namespace android

#endif //ANDROID_PPCASSEMBLER_PROXY_H
