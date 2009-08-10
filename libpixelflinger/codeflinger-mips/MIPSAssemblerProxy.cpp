/* libs/pixelflinger/codeflinger/MIPSAssemblerProxy.cpp
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


#include <stdint.h>
#include <sys/types.h>

#include "codeflinger-mips/MIPSAssemblerProxy.h"

namespace android {

// ----------------------------------------------------------------------------

MIPSAssemblerProxy::MIPSAssemblerProxy()
    : mTarget(0)
{
}

MIPSAssemblerProxy::MIPSAssemblerProxy(MIPSAssemblerInterface* target)
    : mTarget(target)
{
}

MIPSAssemblerProxy::~MIPSAssemblerProxy()
{
    delete mTarget;
}

void MIPSAssemblerProxy::setTarget(MIPSAssemblerInterface* target)
{
    delete mTarget;
    mTarget = target;
}

void MIPSAssemblerProxy::reset() {
    mTarget->reset();
}
int MIPSAssemblerProxy::generate(const char* name) {
    return mTarget->generate(name);
}
void MIPSAssemblerProxy::disassemble(const char* name) {
    return mTarget->disassemble(name);
}
void MIPSAssemblerProxy::prolog() {
    mTarget->prolog();
}
void MIPSAssemblerProxy::epilog(uint32_t touched) {
    mTarget->epilog(touched);
}
void MIPSAssemblerProxy::comment(const char* string) {
    mTarget->comment(string);
}


void MIPSAssemblerProxy::ADDIU(int Rd, int Rs, int immval) {
    mTarget->ADDIU(Rd, Rs, immval);
}
void MIPSAssemblerProxy::ADDU(int Rd, int Rs, int Rt) {
    mTarget->ADDU(Rd, Rs, Rt);
}
void MIPSAssemblerProxy::AND(int Rd, int Rs, int Rt) {
    mTarget->AND(Rd, Rs, Rt);
}
void MIPSAssemblerProxy::ANDI(int Rd, int Rs, int immval) {
    mTarget->ANDI(Rd, Rs, immval);
}
void MIPSAssemblerProxy::BEQ(int Rs, int Rt, const char* label, int slot) {
    mTarget->BEQ(Rs, Rt, label, slot);
}
void MIPSAssemblerProxy::BGEZ(int Rs, const char* label, int slot) {
    mTarget->BGEZ(Rs,label, slot);
}
void MIPSAssemblerProxy::BNE(int Rs, int Rt, const char* label, int slot) {
    mTarget->BNE(Rs, Rt, label, slot);
}
void MIPSAssemblerProxy::LBU(int Rt, int offset, int Rbase) {
    mTarget->LBU(Rt, offset, Rbase);
}
void MIPSAssemblerProxy::LHU(int Rt, int offset, int Rbase) {
    mTarget->LHU(Rt, offset, Rbase);
}
void MIPSAssemblerProxy::LUI(int Rt, int immval) {
    mTarget->LUI(Rt, immval);
}
void MIPSAssemblerProxy::LW(int Rt, int offset, int Rbase) {
    mTarget->LW(Rt, offset, Rbase);
}
void MIPSAssemblerProxy::MFHI(int Rd) {
    mTarget->MFHI(Rd);
}
void MIPSAssemblerProxy::MFLO(int Rd) {
    mTarget->MFLO(Rd);
}
void MIPSAssemblerProxy::MOVN(int Rd, int Rs, int Rt) {
    mTarget->MOVN(Rd, Rs, Rt);
}
void MIPSAssemblerProxy::MOVZ(int Rd, int Rs, int Rt) {
    mTarget->MOVZ(Rd, Rs, Rt);
}
void MIPSAssemblerProxy::MUL(int Rd, int Rs, int Rt) {
    mTarget->MUL(Rd, Rs, Rt);
}
void MIPSAssemblerProxy::MULT(int Rs, int Rt) {
    mTarget->MULT(Rs, Rt);
}
void MIPSAssemblerProxy::NOR(int Rd, int Rs, int Rt) {
    mTarget->NOR(Rd, Rs, Rt);
}
void MIPSAssemblerProxy::OR(int Rd, int Rs, int Rt) {
    mTarget->OR(Rd, Rs, Rt);
}
void MIPSAssemblerProxy::ORI(int Rd, int Rs, int immval) {
    mTarget->ORI(Rd, Rs, immval);
}
void MIPSAssemblerProxy::SB(int Rt, int offset, int Rbase) {
    mTarget->SB(Rt, offset, Rbase);
}
void MIPSAssemblerProxy::SH(int Rt, int offset, int Rbase) {
    mTarget->SH(Rt, offset, Rbase);
}
void MIPSAssemblerProxy::SLL(int Rd, int Rt, int shift) {
    mTarget->SLL(Rd, Rt, shift);
}
void MIPSAssemblerProxy::SLT(int Rd, int Rs, int Rt) {
    mTarget->SLT(Rd, Rs, Rt);
}
void MIPSAssemblerProxy::SRA(int Rd, int Rt, int shift) {
    mTarget->SRA(Rd, Rt, shift);
}
void MIPSAssemblerProxy::SRL(int Rd, int Rt, int shift) {
    mTarget->SRL(Rd, Rt, shift);
}
void MIPSAssemblerProxy::SUBU(int Rd, int Rs, int Rt) {
    mTarget->SUBU(Rd, Rs, Rt);
}
void MIPSAssemblerProxy::SW(int Rt, int offset, int Rbase) {
    mTarget->SW(Rt, offset, Rbase);
}
void MIPSAssemblerProxy::XOR(int Rd, int Rs, int Rt) {
    mTarget->XOR(Rd, Rs, Rt);
}

void MIPSAssemblerProxy::label(const char* theLabel) {
    mTarget->label(theLabel);
}

uint32_t* MIPSAssemblerProxy::pcForLabel(const char* label) {
    return mTarget->pcForLabel(label);
}

bool MIPSAssemblerProxy::getAtValue(uint32_t &atv) {
    return mTarget->getAtValue(atv);
}

void MIPSAssemblerProxy::setAtValue(uint32_t atv) {
    mTarget->setAtValue(atv);
}

char* MIPSAssemblerProxy::getLocLabel() {
    return mTarget->getLocLabel();
}


}; // namespace android

