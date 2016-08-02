// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/operand.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::Operand - Test]
// ============================================================================

#if defined(ASMJIT_TEST)
UNIT(base_operand) {
  INFO("Checking operand sizes");
  EXPECT(sizeof(Operand) == 16);
  EXPECT(sizeof(Reg)     == 16);
  EXPECT(sizeof(Mem)     == 16);
  EXPECT(sizeof(Imm)     == 16);
  EXPECT(sizeof(Label)   == 16);

  INFO("Checking basic functionality of Operand");
  Operand a, b;
  Operand dummy;

  EXPECT(a.isNone() == true);
  EXPECT(a.isReg() == false);
  EXPECT(a.isMem() == false);
  EXPECT(a.isImm() == false);
  EXPECT(a.isLabel() == false);
  EXPECT(a == b);

  EXPECT(a._any.reserved8_4  == 0, "Default constructed Operand should zero its 'reserved8_4' field");
  EXPECT(a._any.reserved12_4 == 0, "Default constructed Operand should zero its 'reserved12_4' field");

  INFO("Checking basic functionality of Label");
  Label label;
  EXPECT(label.isValid() == false);
  EXPECT(label.getId() == kInvalidValue);

  INFO("Checking basic functionality of Reg");
  EXPECT(Reg().isValid() == false,
    "Default constructed Reg() should not be valid");
  EXPECT(Reg()._any.reserved8_4  == 0,
    "A default constructed Reg() should zero its 'reserved8_4' field");
  EXPECT(Reg()._any.reserved12_4 == 0,
    "A default constructed Reg() should zero its 'reserved12_4' field");

  EXPECT(Reg().isReg() == false,
    "Default constructed register should not isReg()");
  EXPECT(static_cast<const Reg&>(dummy).isValid() == false,
    "Default constructed Operand casted to Reg should not be valid");

  // Create some register (not specific to any architecture).
  uint32_t rSig = Operand::makeRegSignature(1, 2, 8);
  Reg r1(Init, rSig, 5);

  EXPECT(r1.isValid()      == true);
  EXPECT(r1.isReg()        == true);
  EXPECT(r1.isReg(1)       == true);
  EXPECT(r1.isPhysReg()    == true);
  EXPECT(r1.isVirtReg()    == false);
  EXPECT(r1.getSignature() == rSig);
  EXPECT(r1.getRegType()   == 1);
  EXPECT(r1.getRegClass()  == 2);
  EXPECT(r1.getSize()      == 8);
  EXPECT(r1.getId()        == 5);
  EXPECT(r1.isReg(1, 5)    == true); // RegType and Id.

  EXPECT(r1._any.reserved8_4  == 0, "Reg should have 'reserved8_4' zero");
  EXPECT(r1._any.reserved12_4 == 0, "Reg should have 'reserved12_4' zero");

  // The same type of register having different id.
  Reg r2(r1, 6);
  EXPECT(r2.isValid()      == true);
  EXPECT(r2.isReg()        == true);
  EXPECT(r2.isReg(1)       == true);
  EXPECT(r2.isPhysReg()    == true);
  EXPECT(r2.isVirtReg()    == false);
  EXPECT(r2.getSignature() == rSig);
  EXPECT(r2.getRegType()   == r1.getRegType());
  EXPECT(r2.getRegClass()  == r1.getRegClass());
  EXPECT(r2.getSize()      == r1.getSize());
  EXPECT(r2.getId()        == 6);
  EXPECT(r2.isReg(1, 6)    == true);

  r1.reset();
  EXPECT(!r1.isValid(),
    "Reset register should not be valid");
  EXPECT(!r1.isReg(),
    "Reset register should not isReg()");

  INFO("Checking basic functionality of Mem");
  Mem m;
  EXPECT(m.isMem()                       , "Default constructed Mem() should isMem()");
  EXPECT(m == Mem()                      , "Two default constructed Mem() operands should be equal");
  EXPECT(m.hasBase()        == false     , "Default constructed Mem() should not have base specified");
  EXPECT(m.hasIndex()       == false     , "Default constructed Mem() should not have index specified");
  EXPECT(m.has64BitOffset() == true      , "Default constructed Mem() should report 64-bit offset");
  EXPECT(m.getOffset()      == 0         , "Default constructed Mem() should have be zero offset / address");

  m.setOffset(-1);
  EXPECT(m.getOffsetLo32()  == -1        , "Memory operand must hold a 32-bit offset");
  EXPECT(m.getOffset()      == -1        , "32-bit offset must be sign extended to 64 bits");

  int64_t x = int64_t(ASMJIT_UINT64_C(0xFF00FF0000000001));
  m.setOffset(x);
  EXPECT(m.getOffset()      == x         , "Memory operand must hold a 64-bit offset");
  EXPECT(m.getOffsetLo32()  == 1         , "Memory operand must return correct low offset DWORD");
  EXPECT(m.getOffsetHi32()  == 0xFF00FF00, "Memory operand must return correct high offset DWORD");

  INFO("Checking basic functionality of Imm");
  EXPECT(Imm(-1).getInt64() == int64_t(-1),
    "Immediate values should by default sign-extend to 64-bits");
}
#endif // ASMJIT_TEST

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
