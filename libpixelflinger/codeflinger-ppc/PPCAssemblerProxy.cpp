/* libs/pixelflinger/codeflinger-ppc/PPCAssemblerProxy.cpp
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License")
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

#include "codeflinger-ppc/PPCAssemblerProxy.h"

namespace android {

// ----------------------------------------------------------------------------

PPCAssemblerProxy::PPCAssemblerProxy()
    : mTarget(0)
{
}

PPCAssemblerProxy::PPCAssemblerProxy(PPCAssemblerInterface* target)
    : mTarget(target)
{
}

PPCAssemblerProxy::~PPCAssemblerProxy()
{
    delete mTarget;
}

void PPCAssemblerProxy::setTarget(PPCAssemblerInterface* target)
{
    delete mTarget;
    mTarget = target;
}

void PPCAssemblerProxy::reset() {
    mTarget->reset();
}

int PPCAssemblerProxy::generate(const char* name) {
    return mTarget->generate(name);
}

void PPCAssemblerProxy::disassemble(const char* name) {
    mTarget->disassemble(name);
}

void PPCAssemblerProxy::prolog() {
    mTarget->prolog();
}

void PPCAssemblerProxy::epilog(uint32_t touched) {
    mTarget->epilog(touched);
}

void PPCAssemblerProxy::comment(const char* string) {
    mTarget->comment(string);
}

void PPCAssemblerProxy::label(const char* theLabel) {
    mTarget->label(theLabel);
}

uint32_t* PPCAssemblerProxy::pcForLabel(const char* label) {
    return mTarget->pcForLabel(label);
}

char* PPCAssemblerProxy::getLocLabel() {
    return mTarget->getLocLabel();
}

void PPCAssemblerProxy::MTSPR(int Spr, int Rs) {
    mTarget->MTSPR(Spr, Rs);
}

void PPCAssemblerProxy::B(const char *label) {
    mTarget->B(label);
}

void PPCAssemblerProxy::BC(int Bo, int Bi, const char *label) {
    mTarget->BC(Bo, Bi, label);
}

void PPCAssemblerProxy::BCLR(int Bo, int Bi, int Bh) {
    mTarget->BCLR(Bo,Bi, Bh);
}

void PPCAssemblerProxy::LBZ(int Rt, int D, int Ra) {
    mTarget->LBZ(Rt, D, Ra);
}

void PPCAssemblerProxy::LBZX(int Rt, int Ra, int Rb) {
    mTarget->LBZX(Rt, Ra, Rb);
}

void PPCAssemblerProxy::LHZ(int Rt, int D, int Ra) {
    mTarget->LHZ(Rt, D, Ra);
}

void PPCAssemblerProxy::LHZX(int Rt, int Ra, int Rb) {
    mTarget->LHZX(Rt, Ra, Rb);
}

void PPCAssemblerProxy::LWZ(int Rt, int D, int Ra) {
    mTarget->LWZ(Rt, D, Ra);
}

void PPCAssemblerProxy::LWZX(int Rt, int Ra, int Rb) {
    mTarget->LWZX(Rt, Ra, Rb);
}

void PPCAssemblerProxy::LMW(int Rt, int D, int Ra) {
    mTarget->LMW(Rt, D, Ra);
}

void PPCAssemblerProxy::STB(int Rs, int D, int Ra) {
    mTarget->STB(Rs, D, Ra);
}

void PPCAssemblerProxy::STH(int Rs, int D, int Ra) {
    mTarget->STH(Rs, D, Ra);
}

void PPCAssemblerProxy::STW(int Rs, int D, int Ra) {
    mTarget->STW(Rs, D, Ra);
}

void PPCAssemblerProxy::STWU(int Rs, int D, int Ra) {
    mTarget->STWU(Rs, D, Ra);
}

void PPCAssemblerProxy::STMW(int Rs, int D, int Ra) {
    mTarget->STMW(Rs, D, Ra);
}

void PPCAssemblerProxy::LHBRX(int Rt, int Ra, int Rb) {
    mTarget->LHBRX(Rt, Ra, Rb);
}

void PPCAssemblerProxy::LWBRX(int Rt, int Ra, int Rb) {
    mTarget->LWBRX(Rt, Ra, Rb);
}

void PPCAssemblerProxy::STHBRX(int Rs, int Ra, int Rb) {
    mTarget->STHBRX(Rs, Ra, Rb);
}

void PPCAssemblerProxy::STWBRX(int Rs, int Ra, int Rb) {
    mTarget->STWBRX(Rs, Ra, Rb);
}


void PPCAssemblerProxy::ADDI(int Rt, int Ra, int Si) {
    mTarget->ADDI(Rt, Ra, Si);
}

void PPCAssemblerProxy::ADDIS(int Rt, int Ra, int Si) {
    mTarget->ADDIS(Rt, Ra, Si);
}

void PPCAssemblerProxy::ADD(int Rt, int Ra, int Rb) {
    mTarget->ADD(Rt, Ra, Rb);
}

void PPCAssemblerProxy::ADDP(int Rt, int Ra, int Rb) {
    mTarget->ADDP(Rt, Ra, Rb);
}

void PPCAssemblerProxy::SUBF(int Rt, int Ra, int Rb) {
    mTarget->SUBF(Rt, Ra, Rb);
}

void PPCAssemblerProxy::NEG(int Rt, int Ra) {
    mTarget->NEG(Rt, Ra);
}

void PPCAssemblerProxy::MULLW(int Rt, int Ra, int Rb) {
    mTarget->MULLW(Rt, Ra, Rb);
}

void PPCAssemblerProxy::MULHW(int Rt, int Ra, int Rb) {
    mTarget->MULHW(Rt, Ra, Rb);
}

void PPCAssemblerProxy::MULHWU(int Rt, int Ra, int Rb) {
    mTarget->MULHWU(Rt, Ra, Rb);
}

void PPCAssemblerProxy::CMP(int Bf, int L, int Ra, int Rb) {
    mTarget->CMP(Bf, L, Ra, Rb);
}

void PPCAssemblerProxy::CMPL(int Bf, int L, int Ra, int Rb) {
    mTarget->CMPL(Bf, L, Ra, Rb);
}

void PPCAssemblerProxy::ANDIP(int Ra, int Rs, int Ui) {
    mTarget->ANDIP(Ra, Rs, Ui);
}

void PPCAssemblerProxy::ORI(int Ra, int Rs, int Ui) {
    mTarget->ORI(Ra, Rs, Ui);
}

void PPCAssemblerProxy::AND(int Ra, int Rs, int Rb) {
    mTarget->AND(Ra, Rs, Rb);
}

void PPCAssemblerProxy::XOR(int Ra, int Rs, int Rb) {
    mTarget->XOR(Ra, Rs, Rb);
}

void PPCAssemblerProxy::NAND(int Ra, int Rs, int Rb) {
    mTarget->NAND(Ra, Rs, Rb);
}

void PPCAssemblerProxy::OR(int Ra, int Rs, int Rb) {
    mTarget->OR(Ra, Rs, Rb);
}

void PPCAssemblerProxy::NOR(int Ra, int Rs, int Rb) {
    mTarget->NOR(Ra, Rs, Rb);
}

void PPCAssemblerProxy::EQV(int Ra, int Rs, int Rb) {
    mTarget->EQV(Ra, Rs, Rb);
}

void PPCAssemblerProxy::ANDC(int Ra, int Rs, int Rb) {
    mTarget->ANDC(Ra, Rs, Rb);
}

void PPCAssemblerProxy::ORC(int Ra, int Rs, int Rb) {
    mTarget->ORC(Ra, Rs, Rb);
}

void PPCAssemblerProxy::RLWINM(int Ra, int Rs, int Sh, int Mb, int Me) {
    mTarget->RLWINM(Ra, Rs, Sh, Mb, Me);
}

void PPCAssemblerProxy::SRW(int Ra, int Rs, int Rb) {
    mTarget->SRW(Ra, Rs, Rb);
}

void PPCAssemblerProxy::SRAWI(int Ra, int Rs, int Sh) {
    mTarget->SRAWI(Ra, Rs, Sh);
}

void PPCAssemblerProxy::SRAWIP(int Ra, int Rs, int Sh) {
    mTarget->SRAWIP(Ra, Rs, Sh);
}

}; // namespace android
