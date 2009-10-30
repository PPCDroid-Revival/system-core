/* libs/pixelflinger/codeflinger-ppc/PPCAssemblerInterface.h
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


#ifndef ANDROID_PPCASSEMBLER_INTERFACE_H
#define ANDROID_PPCASSEMBLER_INTERFACE_H

#include <stdint.h>
#include <sys/types.h>

namespace android {

// ----------------------------------------------------------------------------

class PPCAssemblerInterface
{
public:
    virtual ~PPCAssemblerInterface();

    enum {
        cr0, cr1, cr2, cr3, cr4, cr5, cr6, cr7
    };

    enum {
	r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12,
	r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23,
	r24, r25, r26, r27, r28, r29, r30, r31,
        stkp = r1
    };

    enum {
        #define LIST(rr) L##rr=1<<rr
        LIST(r13), LIST(r14), LIST(r15), LIST(r16), LIST(r17),
	LIST(r18), LIST(r19), LIST(r20), LIST(r21), LIST(r22),
	LIST(r23), LIST(r24), LIST(r25), LIST(r26), LIST(r27),
	LIST(r28), LIST(r29), LIST(r30), LIST(r31),
        #undef LIST
        LSAVED = Lr13|Lr14|Lr15|Lr16|Lr17|Lr18|Lr19|Lr20|Lr21|Lr22|
                 Lr23|Lr24|Lr25|Lr26|Lr27|Lr28|Lr29|Lr30|Lr31
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
    virtual char* getLocLabel() =  0;

    // ----- Instructions we use -----

    // xxxP means period (.) for instructions that touch CR
    // SPR registers access
    virtual void MTSPR(int Spr, int Rs) = 0;

    // Branch instructions
    virtual void B(const char *label) = 0;
    virtual void BC(int Bo, int Bi, const char *label) = 0;
    virtual void BCLR(int Bo, int Bi, int Bh = 0) = 0;

    // Load-store instructions
    virtual void LBZ(int Rt, int D, int Ra) = 0;
    virtual void LBZX(int Rt, int Ra, int Rb) = 0;
    virtual void LHZ(int Rt, int D, int Ra) = 0;
    virtual void LHZX(int Rt, int Ra, int Rb) = 0;
    virtual void LWZ(int Rt, int D, int Ra) = 0;
    virtual void LWZX(int Rt, int Ra, int Rb) = 0;
    virtual void LMW(int Rt, int D, int Ra) = 0;
    virtual void STB(int Rs, int D, int Ra) = 0;
    virtual void STH(int Rs, int D, int Ra) = 0;
    virtual void STW(int Rs, int D, int Ra) = 0;
    virtual void STWU(int Rs, int D, int Ra) = 0;
    virtual void STMW(int Rs, int D, int Ra) = 0;

    //Load-store instructions with reversed order
    virtual void LHBRX(int Rt, int Ra, int Rb) = 0;
    virtual void LWBRX(int Rt, int Ra, int Rb) = 0;
    virtual void STHBRX(int Rs, int Ra, int Rb) = 0;
    virtual void STWBRX(int Rs, int Ra, int Rb) = 0;

    //Arithmetic instructions
    virtual void ADDI(int Rt, int Ra, int Si) = 0;
    virtual void ADDIS(int Rt, int Ra, int Si) = 0;
    virtual void ADD(int Rt, int Ra, int Rb) = 0;
    virtual void ADDP(int Rt, int Ra, int Rb) = 0; //ADD.
    virtual void SUBF(int Rt, int Ra, int Rb) = 0;
    virtual void NEG(int Rt, int Ra) = 0;
    virtual void MULLW(int Rt, int Ra, int Rb) = 0;
    virtual void MULHW(int Rt, int Ra, int Rb) = 0;
    virtual void MULHWU(int Rt, int Ra, int Rb) = 0;

    //compare instructions
    virtual void CMP(int Bf, int L, int Ra, int Rb) = 0;
    virtual void CMPL(int Bf, int L, int Ra, int Rb) = 0;

    //Logical instructions
    virtual void ANDIP(int Ra, int Rs, int Ui) = 0;
    virtual void ORI(int Ra, int Rs, int Ui) = 0;
    virtual void AND(int Ra, int Rs, int Rb) = 0;
    virtual void XOR(int Ra, int Rs, int Rb) = 0;
    virtual void NAND(int Ra, int Rs, int Rb) = 0;
    virtual void OR(int Ra, int Rs, int Rb) = 0;
    virtual void NOR(int Ra, int Rs, int Rb) = 0;
    virtual void EQV(int Ra, int Rs, int Rb) = 0;
    virtual void ANDC(int Ra, int Rs, int Rb) = 0;
    virtual void ORC(int Ra, int Rs, int Rb) = 0;

    //Rotate&Mask instructions
    virtual void RLWINM(int Ra, int Rs, int Sh, int Mb, int Me) = 0;

    //Shift instructions
    virtual void SRW(int Ra, int Rs, int Rb) = 0;
    virtual void SRAWI(int Ra, int Rs, int Sh) = 0;
    virtual void SRAWIP(int Ra, int Rs, int Sh) = 0;

    // ----- Instructions mnemonics -----

    // Registers access
    inline void MR(int Ra, int Rs) {
        OR(Ra, Rs, Rs);
    }

    inline void MTCTR(int Rs) {
        MTSPR(9, Rs);
    }

    inline void LI(int Rt, int Si) {
        ADDI(Rt, 0, Si);
    }

    inline void LIS(int Rt, int Si) {
        ADDIS(Rt, 0, Si);
    }
    // Branch instructions
    #define CRB(B) (4*CRs + B)
    inline void BDNZ(const char *label) {
        BC(16, 0, label);
    }
    inline void BLT(int CRs, char *label) {
        BC(12, CRB(0), label);
    }
    inline void BLE(int CRs, char *label) {
        BC(4, CRB(1), label);
    }
    inline void BEQ(int CRs, char *label) {
        BC(12, CRB(2), label);
    }
    inline void BGE(int CRs, char *label) {
        BC(4, CRB(0), label);
    }
    inline void BGT(int CRs, char *label) {
        BC(12, CRB(1), label);
    }
    inline void BNL(int CRs, char *label) {
        BC(4, CRB(0), label);
    }
    inline void BNE(int CRs, char *label) {
        BC(4, CRB(2), label);
    }
    inline void BNG(int CRs, char *label) {
        BC(4, CRB(1), label);
    }
    inline void BLR() {
        BCLR(20, 0);
    }
    #undef CRB

    //Arithmetic instructions
    inline void SUBI(int Rt, int Ra, int Si) {
        ADDI(Rt, Ra, -Si);
    }
    inline void SUBIS(int Rt, int Ra, int Si) {
        ADDIS(Rt, Ra, -Si);
    }
    inline void SUB(int Rt, int Ra, int Rb) {
        SUBF(Rt, Rb, Ra);
    }

    //Compare instructions
    inline void CMPW(int CRs, int Ra, int Rb) {
        CMP(CRs, 0, Ra, Rb);
    }
    inline void CMPLW(int CRs, int Ra, int Rb) {
        CMPL(CRs, 0, Ra, Rb);
    }

    //Logical instructions
    inline void NOT(int Ra, int Rs) {
        NOR(Ra, Rs, Rs);
    }
    inline void NOP() {
        ORI(0, 0, 0);
    }

    //Rotate instructions
    inline void ROTLWI(int Ra, int Rs, int N) {
        RLWINM(Ra, Rs, N, 0, 31);
    }
    inline void ROTRWI(int Ra, int Rs, int N) {
        RLWINM(Ra, Rs, 32-N, 0, 31);
    }

    //Shift instructions
    inline void SRWI(int Ra, int Rs, int N) {
        RLWINM(Ra, Rs, 32-N, N, 31);
    }

    inline void SLWI(int Ra, int Rs, int N) {
        RLWINM(Ra, Rs, N, 0, 31-N);
    }

    //Extract instructions
    inline void EXTRWI(int Ra, int Rs, int N, int B) {
        RLWINM(Ra, Rs, B+N, 32-N, 31);
    }

    //Clear instructions
    inline void CLRLWI(int Ra, int Rs, int N) {
	RLWINM(Ra, Rs, 0, N, 31 );
    }
};

}; // namespace android

#endif //ANDROID_PPCASSEMBLER_INTERFACE_H
