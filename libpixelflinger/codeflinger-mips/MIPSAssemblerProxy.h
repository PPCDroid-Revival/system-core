/* libs/pixelflinger/codeflinger/MIPSAssemblerProxy.h
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


#ifndef ANDROID_MIPSASSEMBLER_PROXY_H
#define ANDROID_MIPSASSEMBLER_PROXY_H

#include <stdint.h>
#include <sys/types.h>

#include "codeflinger-mips/MIPSAssemblerInterface.h"

namespace android {

// ----------------------------------------------------------------------------

class MIPSAssemblerProxy : public MIPSAssemblerInterface
{
public:
    // MIPSAssemblerProxy take ownership of the target

                MIPSAssemblerProxy();
                MIPSAssemblerProxy(MIPSAssemblerInterface* target);
    virtual     ~MIPSAssemblerProxy();

    void setTarget(MIPSAssemblerInterface* target);

    virtual void    reset();
    virtual int     generate(const char* name);
    virtual void    disassemble(const char* name);

    virtual void    prolog();
    virtual void    epilog(uint32_t touched);
    virtual void    comment(const char* string);

    virtual void label(const char* theLabel);
    uint32_t* pcForLabel(const char* label);
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
    MIPSAssemblerInterface*  mTarget;
};

}; // namespace android

#endif //ANDROID_MIPSASSEMBLER_PROXY_H
