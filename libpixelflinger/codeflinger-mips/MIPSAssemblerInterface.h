/* libs/pixelflinger/codeflinger/MIPSAssemblerInterface.h
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


#ifndef ANDROID_MIPSASSEMBLER_INTERFACE_H
#define ANDROID_MIPSASSEMBLER_INTERFACE_H

#include <stdint.h>
#include <sys/types.h>

namespace android {

// ----------------------------------------------------------------------------

class MIPSAssemblerInterface
{
public:
    virtual ~MIPSAssemblerInterface();

    enum {
        zero, at, v0, v1, a0, a1, a2, a3, t0, t1, t2, t3, t4, t5, t6, t7,
        s0, s1, s2, s3, s4, s5, s6, s7, t8, t9, kt0, kt1, gp, stkp, s8, ra
    };
    enum {
        #define LIST(rr) L##rr=1<<rr
        LIST(s0), LIST(s1), LIST(s2), LIST(s3), LIST(s4),
	LIST(s5), LIST(s6), LIST(s7), LIST(s8),
        #undef LIST
        LSAVED = Ls0|Ls1|Ls2|Ls3|Ls4|Ls5|Ls6|Ls7|Ls8
    };
    enum {
    	nopdelay,
	usedelay,
    };

    // -----------------------------------------------------------------------
    // basic instructions & code generation
    // -----------------------------------------------------------------------

    // generate the code
    virtual void reset() = 0;
    virtual int  generate(const char* name) = 0;
    virtual void disassemble(const char* name) = 0;
    
    // construct prolog and epilog
    virtual void prolog() = 0;
    virtual void epilog(uint32_t touched) = 0;
    virtual void comment(const char* string) = 0;

    virtual void label(const char* theLabel) = 0;
    virtual uint32_t* pcForLabel(const char* label) = 0;
    virtual bool getAtValue(uint32_t &atv) = 0;
    virtual void setAtValue(uint32_t atv) = 0;
    virtual char* getLocLabel() = 0;

    // Instructions we use
    virtual void ADDIU(int Rd, int Rs, int immval) = 0;
    virtual void ADDU(int Rd, int Rs, int Rt) = 0;
    virtual void AND(int Rd, int Rs, int Rt) = 0;
    virtual void ANDI(int Rd, int Rs, int immval) = 0;
    virtual void BEQ(int Rs, int Rt, const char* label,
    		int slot = nopdelay) = 0;
    virtual void BGEZ(int Rs, const char* label, int slot = nopdelay) = 0;
    virtual void BNE(int Rs, int Rt, const char* label,
    		int slot = nopdelay) = 0;
    virtual void LBU(int Rt, int offset, int Rbase) = 0;
    virtual void LHU(int Rt, int offset, int Rbase) = 0;
    virtual void LUI(int Rt, int immval) = 0;
    virtual void LW(int Rt, int offset, int Rbase) = 0;
    virtual void MFHI(int Rd) = 0;
    virtual void MFLO(int Rd) = 0;
    virtual void MOVN(int Rd, int Rs, int Rt) = 0;
    virtual void MOVZ(int Rd, int Rs, int Rt) = 0;
    virtual void MUL(int Rd, int Rs, int Rt) = 0;
    virtual void MULT(int Rs, int Rt) = 0;
    virtual void NOR(int Rd, int Rs, int Rt) = 0;
    virtual void OR(int Rd, int Rs, int Rt) = 0;
    virtual void ORI(int Rd, int Rs, int immval) = 0;
    virtual void SB(int Rt, int offset, int Rbase) = 0;
    virtual void SH(int Rt, int offset, int Rbase) = 0;
    virtual void SLL(int Rt, int Rd, int shift) = 0;
    virtual void SLT(int Rd, int Rs, int Rt) = 0;
    virtual void SRA(int Rt, int Rd, int shift) = 0;
    virtual void SRL(int Rt, int Rd, int shift) = 0;
    virtual void SUBU(int Rd, int Rs, int Rt) = 0;
    virtual void SW(int Rt, int offset, int Rbase) = 0;
    virtual void XOR(int Rd, int Rs, int Rt) = 0;

    // Pseudo OPS
    inline void
    BNEZ(int Rs, const char* label, int slot = nopdelay) {
    	BNE(Rs, zero, label, slot);
    }
    inline void
    BEQZ(int Rs, const char* label, int slot = nopdelay) {
    	BEQ(Rs, zero, label, slot);
    }
    inline void
    LI(int Rt, int immval) {
    	ORI(Rt, zero, immval);
    }
    inline void
    MOVE(int Rd, int Rs) {
    	ADDU(Rd, Rs, zero);
    }
    inline void
    ROR(int Rd, int Rt, int rot) {
    	// Can't be called with 'at' as either source or destination.
	SRL(at, Rt, rot);
	SLL(Rd, Rt, 32-rot);
	OR(Rd, Rd, at);
    }
};

}; // namespace android

#endif //ANDROID_MIPSASSEMBLER_INTERFACE_H
