// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// ----------------------------------------------------------------------------
// IMPORTANT: AsmJit now uses an external instruction database to populate
// static tables within this file. Perform the following steps to regenerate
// all tables enclosed by ${...}:
//
//   1. Install node.js environment <https://nodejs.org>
//   2. Go to asmjit/tools directory
//   3. Install either asmdb package by executing `npm install asmdb` or get
//      the latest asmdb from <https://github.com/asmjit/asmdb> and copy both
//      `x86data.js` and `x86util.js` files into the asmjit/tools directory
//   4. Execute `node generate-x86.js`
//
// Instruction encoding and opcodes were added to the `x86inst.cpp` database
// manually in the past and they are not updated by the script as they seem
// consistent. However, everything else is updated including instruction
// operands and tables required to validate them, instruction read/write
// information (including registers and flags), and all indexes to extra tables.
// ----------------------------------------------------------------------------

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../build.h"
#if defined(ASMJIT_BUILD_X86)

// [Dependencies]
#include "../base/utils.h"
#include "../x86/x86inst.h"
#include "../x86/x86operand.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [Enums (Internal)]
// ============================================================================

//! \internal
enum ODATA_ {
  // PREFIX.
  ODATA_000000  = X86Inst::kOpCode_PP_00 | X86Inst::kOpCode_MM_00,
  ODATA_000F00  = X86Inst::kOpCode_PP_00 | X86Inst::kOpCode_MM_0F,
  ODATA_000F01  = X86Inst::kOpCode_PP_00 | X86Inst::kOpCode_MM_0F01,
  ODATA_000F38  = X86Inst::kOpCode_PP_00 | X86Inst::kOpCode_MM_0F38,
  ODATA_000F3A  = X86Inst::kOpCode_PP_00 | X86Inst::kOpCode_MM_0F3A,
  ODATA_660000  = X86Inst::kOpCode_PP_66 | X86Inst::kOpCode_MM_00,
  ODATA_660F00  = X86Inst::kOpCode_PP_66 | X86Inst::kOpCode_MM_0F,
  ODATA_660F38  = X86Inst::kOpCode_PP_66 | X86Inst::kOpCode_MM_0F38,
  ODATA_660F3A  = X86Inst::kOpCode_PP_66 | X86Inst::kOpCode_MM_0F3A,
  ODATA_F20000  = X86Inst::kOpCode_PP_F2 | X86Inst::kOpCode_MM_00,
  ODATA_F20F00  = X86Inst::kOpCode_PP_F2 | X86Inst::kOpCode_MM_0F,
  ODATA_F20F38  = X86Inst::kOpCode_PP_F2 | X86Inst::kOpCode_MM_0F38,
  ODATA_F20F3A  = X86Inst::kOpCode_PP_F2 | X86Inst::kOpCode_MM_0F3A,
  ODATA_F30000  = X86Inst::kOpCode_PP_F3 | X86Inst::kOpCode_MM_00,
  ODATA_F30F00  = X86Inst::kOpCode_PP_F3 | X86Inst::kOpCode_MM_0F,
  ODATA_F30F38  = X86Inst::kOpCode_PP_F3 | X86Inst::kOpCode_MM_0F38,
  ODATA_F30F3A  = X86Inst::kOpCode_PP_F3 | X86Inst::kOpCode_MM_0F3A,
  ODATA_000F0F  = X86Inst::kOpCode_PP_00 | X86Inst::kOpCode_MM_0F, // 3DNOW, special case.

  ODATA_FPU_00  = X86Inst::kOpCode_PP_00,
  ODATA_FPU_9B  = X86Inst::kOpCode_PP_9B,

  ODATA_XOP_M8  = X86Inst::kOpCode_MM_XOP08,
  ODATA_XOP_M9  = X86Inst::kOpCode_MM_XOP09,

  ODATA_O__     = 0,
  ODATA_O_0     = 0 << X86Inst::kOpCode_O_Shift,
  ODATA_O_1     = 1 << X86Inst::kOpCode_O_Shift,
  ODATA_O_2     = 2 << X86Inst::kOpCode_O_Shift,
  ODATA_O_3     = 3 << X86Inst::kOpCode_O_Shift,
  ODATA_O_4     = 4 << X86Inst::kOpCode_O_Shift,
  ODATA_O_5     = 5 << X86Inst::kOpCode_O_Shift,
  ODATA_O_6     = 6 << X86Inst::kOpCode_O_Shift,
  ODATA_O_7     = 7 << X86Inst::kOpCode_O_Shift,

  // REX/VEX.
  ODATA_LL__    = 0,                                  // L is unspecified.
  ODATA_LL_x    = 0,                                  // L is based on operand(s).
  ODATA_LL_I    = 0,                                  // L is ignored (LIG).
  ODATA_LL_0    = 0,                                  // L has to be zero (L.128).
  ODATA_LL_1    = X86Inst::kOpCode_LL_256,            // L has to be one (L.256).
  ODATA_LL_2    = X86Inst::kOpCode_LL_512,            // L has to be two (L.512).

  ODATA_W__     = 0,                                  // W is unspecified.
  ODATA_W_x     = 0,                                  // W is based on operand(s).
  ODATA_W_I     = 0,                                  // W is ignored (WIG).
  ODATA_W_0     = 0,                                  // W has to be zero (W0).
  ODATA_W_1     = X86Inst::kOpCode_W,                 // W has to be one (W1).

  // EVEX.
  ODATA_EvexW__ = 0,                                  // Not EVEX instruction.
  ODATA_EvexW_x = 0,                                  // EVEX.W is based on operand(s).
  ODATA_EvexW_I = 0,                                  // EVEX.W is ignored     (EVEX.WIG).
  ODATA_EvexW_0 = 0,                                  // EVEX.W has to be zero (EVEX.W0).
  ODATA_EvexW_1 = X86Inst::kOpCode_EW,                // EVEX.W has to be one  (EVEX.W1).

  ODATA_N__      = 0,                                 // Base element size not used.
  ODATA_N_0      = 0 << X86Inst::kOpCode_CDSHL_Shift, // N << 0 (BYTE).
  ODATA_N_1      = 1 << X86Inst::kOpCode_CDSHL_Shift, // N << 1 (WORD).
  ODATA_N_2      = 2 << X86Inst::kOpCode_CDSHL_Shift, // N << 2 (DWORD).
  ODATA_N_3      = 3 << X86Inst::kOpCode_CDSHL_Shift, // N << 3 (QWORD).
  ODATA_N_4      = 4 << X86Inst::kOpCode_CDSHL_Shift, // N << 4 (OWORD).
  ODATA_N_5      = 5 << X86Inst::kOpCode_CDSHL_Shift, // N << 5 (YWORD).

  ODATA_TT__     = 0,
  ODATA_TT_FV    = X86Inst::kOpCode_CDTT_FV,
  ODATA_TT_HV    = X86Inst::kOpCode_CDTT_HV,
  ODATA_TT_FVM   = X86Inst::kOpCode_CDTT_FVM,
  ODATA_TT_T1S   = X86Inst::kOpCode_CDTT_T1S,
  ODATA_TT_T1F   = X86Inst::kOpCode_CDTT_T1F,
  ODATA_TT_T1W   = X86Inst::kOpCode_CDTT_T1W,
  ODATA_TT_T2    = X86Inst::kOpCode_CDTT_T2,
  ODATA_TT_T4    = X86Inst::kOpCode_CDTT_T4,
  ODATA_TT_T8    = X86Inst::kOpCode_CDTT_T8,
  ODATA_TT_HVM   = X86Inst::kOpCode_CDTT_HVM,
  ODATA_TT_OVM   = X86Inst::kOpCode_CDTT_OVM,
  ODATA_TT_QVM   = X86Inst::kOpCode_CDTT_QVM,
  ODATA_TT_128   = X86Inst::kOpCode_CDTT_128,
  ODATA_TT_DUP   = X86Inst::kOpCode_CDTT_DUP
};

// ============================================================================
// [asmjit::X86Inst]
// ============================================================================

// Instruction opcode definitions:
//   - `O` encodes X86|MMX|SSE instructions.
//   - `V` encodes VEX|XOP|EVEX instructions.
#define O_ENCODE(VEX, PREFIX, OPCODE, O, L, W, EvexW, N, TT) \
  ((PREFIX) | (OPCODE) | (O) | (L) | (W) | (EvexW) | (N) | (TT) | \
   (VEX && ((PREFIX) & X86Inst::kOpCode_MM_Mask) != X86Inst::kOpCode_MM_0F ? int(X86Inst::kOpCode_MM_ForceVex3) : 0))

#define O(PREFIX, OPCODE, O, LL, W, EvexW, N, TT) (O_ENCODE(0, ODATA_##PREFIX, 0x##OPCODE, ODATA_O_##O, ODATA_LL_##LL, ODATA_W_##W, ODATA_EvexW_##EvexW, ODATA_N_##N, ODATA_TT_##TT))
#define V(PREFIX, OPCODE, O, LL, W, EvexW, N, TT) (O_ENCODE(1, ODATA_##PREFIX, 0x##OPCODE, ODATA_O_##O, ODATA_LL_##LL, ODATA_W_##W, ODATA_EvexW_##EvexW, ODATA_N_##N, ODATA_TT_##TT))

#define O_FPU(PREFIX, OPCODE, O) (ODATA_FPU_##PREFIX | (0x##OPCODE & 0xFFU) | ((0x##OPCODE >> 8) << X86Inst::kOpCode_FPU_2B_Shift) | ODATA_O_##O)

#define F(FLAG) X86Inst::kInstFlag##FLAG            // Instruction Base Flag(s) `F(...)`.
#define EF(EFLAGS) 0                                // Instruction EFLAGS `EF(OSZAPCDX)`.
#define Enc(ENCODING) X86Inst::kEncoding##ENCODING  // Instruction Encoding `Enc(...)`.

#define A512(CPU, VL, MASKING, SAE_RC, BROADCAST) \
  (X86Inst::kInstFlagEvex              | \
   X86Inst::kInstFlagEvexSet_##CPU     | \
   (VL ? X86Inst::kInstFlagEvexVL : 0) | \
   X86Inst::kInstFlagEvex##MASKING     | \
   X86Inst::kInstFlagEvex##SAE_RC      | \
   X86Inst::kInstFlagEvex##BROADCAST   )

// Defines an X86/X64 instruction.
#define INST(id, name, encoding, opcode0, opcode1, instFlags, eflags, writeIndex, writeSize, simdDstSize, simdSrcSize, signatureIndex, signatureCount, extendedIndex) \
  { opcode0, signatureIndex, signatureCount, extendedIndex, 0 }

const X86Inst _x86InstData[] = {
  //                                                                                                                                                                                      (Autogenerated)
  // <-----------------+-------------------+------------------------+------------------+--------+------------------+--------+---------------------------------------+-------------+-------+-------+-------------+
  //                   |                   |                        |  Primary OpCode  |#0 EVEX | Secondary OpCode |#1 EVEX |         Instruction Flags             |   E-FLAGS   | Write |SimdElm|  Sign. |    |
  //  Instruction Id   | Instruction Name  |  Instruction Encoding  |                  +--------+                  +--------+----------------+----------------------+-------------+---+---+---+---+----+---+Ext.+
  //                   |                   |                        |#0:PP-MMM OP/O L|W|W|N|TT. |#1:PP-MMM OP/O L|W|W|N|TT. | Global Flags   |A512(CPU_|V|kz|rnd|b) | EF:OSZAPCDX |Idx|Cnt|Dst|Src| Idx|Cnt|    |
  // <-----------------+-------------------+------------------------+------------------+--------+------------------+--------+----------------+----------------------+-------------+---+---+---+---+----+---+----+
// ${X86InstData:Begin}
  INST(None            , ""                , Enc(None)              , 0                         , 0                         , 0                                     , EF(________), 0 , 0 , 0 , 0 , 0  , 0 , 0  ),
  INST(Adc             , "adc"             , Enc(X86Arith)          , O(000000,10,2,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWWWX__), 0 , 0 , 0 , 0 , 13 , 10, 1  ),
  INST(Adcx            , "adcx"            , Enc(X86Rm)             , O(660F38,F6,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____X__), 0 , 0 , 0 , 0 , 21 , 2 , 2  ),
  INST(Add             , "add"             , Enc(X86Arith)          , O(000000,00,0,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWWWW__), 0 , 0 , 0 , 0 , 13 , 10, 3  ),
  INST(Addpd           , "addpd"           , Enc(ExtRm)             , O(660F00,58,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 288, 1 , 4  ),
  INST(Addps           , "addps"           , Enc(ExtRm)             , O(000F00,58,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Addsd           , "addsd"           , Enc(ExtRm)             , O(F20F00,58,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 345, 1 , 4  ),
  INST(Addss           , "addss"           , Enc(ExtRm)             , O(F30F00,58,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 346, 1 , 5  ),
  INST(Addsubpd        , "addsubpd"        , Enc(ExtRm)             , O(660F00,D0,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 288, 1 , 4  ),
  INST(Addsubps        , "addsubps"        , Enc(ExtRm)             , O(F20F00,D0,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Adox            , "adox"            , Enc(X86Rm)             , O(F30F38,F6,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(X_______), 0 , 0 , 0 , 0 , 21 , 2 , 6  ),
  INST(Aesdec          , "aesdec"          , Enc(ExtRm)             , O(660F38,DE,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 0 , 0 , 288, 1 , 7  ),
  INST(Aesdeclast      , "aesdeclast"      , Enc(ExtRm)             , O(660F38,DF,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 0 , 0 , 288, 1 , 7  ),
  INST(Aesenc          , "aesenc"          , Enc(ExtRm)             , O(660F38,DC,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 0 , 0 , 288, 1 , 7  ),
  INST(Aesenclast      , "aesenclast"      , Enc(ExtRm)             , O(660F38,DD,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 0 , 0 , 288, 1 , 7  ),
  INST(Aesimc          , "aesimc"          , Enc(ExtRm)             , O(660F38,DB,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 0 , 0 , 63 , 1 , 8  ),
  INST(Aeskeygenassist , "aeskeygenassist" , Enc(ExtRmi)            , O(660F3A,DF,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 0 , 0 , 70 , 1 , 9  ),
  INST(And             , "and"             , Enc(X86Arith)          , O(000000,20,4,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWUWW__), 0 , 0 , 0 , 0 , 13 , 10, 3  ),
  INST(Andn            , "andn"            , Enc(VexRvm_Wx)         , V(000F38,F2,_,0,x,_,_,_  ), 0                         , F(RW)                                 , EF(WWWUUW__), 0 , 0 , 0 , 0 , 245, 2 , 10 ),
  INST(Andnpd          , "andnpd"          , Enc(ExtRm)             , O(660F00,55,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 288, 1 , 4  ),
  INST(Andnps          , "andnps"          , Enc(ExtRm)             , O(000F00,55,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Andpd           , "andpd"           , Enc(ExtRm)             , O(660F00,54,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 288, 1 , 4  ),
  INST(Andps           , "andps"           , Enc(ExtRm)             , O(000F00,54,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Bextr           , "bextr"           , Enc(VexRmv_Wx)         , V(000F38,F7,_,0,x,_,_,_  ), 0                         , F(RW)                                 , EF(WUWUUW__), 0 , 0 , 0 , 0 , 247, 2 , 11 ),
  INST(Blcfill         , "blcfill"         , Enc(VexVm_Wx)          , V(XOP_M9,01,1,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , 0 , 0 , 153, 2 , 12 ),
  INST(Blci            , "blci"            , Enc(VexVm_Wx)          , V(XOP_M9,02,6,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , 0 , 0 , 153, 2 , 12 ),
  INST(Blcic           , "blcic"           , Enc(VexVm_Wx)          , V(XOP_M9,01,5,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , 0 , 0 , 153, 2 , 12 ),
  INST(Blcmsk          , "blcmsk"          , Enc(VexVm_Wx)          , V(XOP_M9,02,1,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , 0 , 0 , 153, 2 , 12 ),
  INST(Blcs            , "blcs"            , Enc(VexVm_Wx)          , V(XOP_M9,01,3,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , 0 , 0 , 153, 2 , 12 ),
  INST(Blendpd         , "blendpd"         , Enc(ExtRmi)            , O(660F3A,0D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 290, 1 , 13 ),
  INST(Blendps         , "blendps"         , Enc(ExtRmi)            , O(660F3A,0C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 290, 1 , 14 ),
  INST(Blendvpd        , "blendvpd"        , Enc(ExtRmXMM0)         , O(660F38,15,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 8 , 8 , 347, 1 , 15 ),
  INST(Blendvps        , "blendvps"        , Enc(ExtRmXMM0)         , O(660F38,14,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 4 , 4 , 347, 1 , 16 ),
  INST(Blsfill         , "blsfill"         , Enc(VexVm_Wx)          , V(XOP_M9,01,2,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , 0 , 0 , 153, 2 , 12 ),
  INST(Blsi            , "blsi"            , Enc(VexVm_Wx)          , V(000F38,F3,3,0,x,_,_,_  ), 0                         , F(RW)                                 , EF(WWWUUW__), 0 , 0 , 0 , 0 , 153, 2 , 17 ),
  INST(Blsic           , "blsic"           , Enc(VexVm_Wx)          , V(XOP_M9,01,6,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , 0 , 0 , 153, 2 , 12 ),
  INST(Blsmsk          , "blsmsk"          , Enc(VexVm_Wx)          , V(000F38,F3,2,0,x,_,_,_  ), 0                         , F(RW)                                 , EF(WWWUUW__), 0 , 0 , 0 , 0 , 153, 2 , 17 ),
  INST(Blsr            , "blsr"            , Enc(VexVm_Wx)          , V(000F38,F3,1,0,x,_,_,_  ), 0                         , F(RW)                                 , EF(WWWUUW__), 0 , 0 , 0 , 0 , 153, 2 , 17 ),
  INST(Bsf             , "bsf"             , Enc(X86Rm)             , O(000F00,BC,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(UUWUUU__), 0 , 0 , 0 , 0 , 20 , 3 , 18 ),
  INST(Bsr             , "bsr"             , Enc(X86Rm)             , O(000F00,BD,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(UUWUUU__), 0 , 0 , 0 , 0 , 20 , 3 , 18 ),
  INST(Bswap           , "bswap"           , Enc(X86Bswap)          , O(000F00,C8,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 0 , 0 , 348, 1 , 19 ),
  INST(Bt              , "bt"              , Enc(X86Bt)             , O(000F00,A3,_,_,x,_,_,_  ), O(000F00,BA,4,_,x,_,_,_  ), F(RO)                                 , EF(UU_UUW__), 0 , 0 , 0 , 0 , 143, 3 , 20 ),
  INST(Btc             , "btc"             , Enc(X86Bt)             , O(000F00,BB,_,_,x,_,_,_  ), O(000F00,BA,7,_,x,_,_,_  ), F(RW)|F(Lock)                         , EF(UU_UUW__), 0 , 0 , 0 , 0 , 146, 3 , 21 ),
  INST(Btr             , "btr"             , Enc(X86Bt)             , O(000F00,B3,_,_,x,_,_,_  ), O(000F00,BA,6,_,x,_,_,_  ), F(RW)|F(Lock)                         , EF(UU_UUW__), 0 , 0 , 0 , 0 , 146, 3 , 22 ),
  INST(Bts             , "bts"             , Enc(X86Bt)             , O(000F00,AB,_,_,x,_,_,_  ), O(000F00,BA,5,_,x,_,_,_  ), F(RW)|F(Lock)                         , EF(UU_UUW__), 0 , 0 , 0 , 0 , 146, 3 , 23 ),
  INST(Bzhi            , "bzhi"            , Enc(VexRmv_Wx)         , V(000F38,F5,_,0,x,_,_,_  ), 0                         , F(RW)                                 , EF(WWWUUW__), 0 , 0 , 0 , 0 , 247, 2 , 11 ),
  INST(Call            , "call"            , Enc(X86Call)           , O(000000,FF,2,_,_,_,_,_  ), O(000000,E8,_,_,_,_,_,_  ), F(RW)|F(Flow)|F(Volatile)             , EF(________), 0 , 0 , 0 , 0 , 249, 2 , 24 ),
  INST(Cbw             , "cbw"             , Enc(X86OpAx)           , O(660000,98,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 0 , 0 , 349, 1 , 25 ),
  INST(Cdq             , "cdq"             , Enc(X86OpDxAx)         , O(000000,99,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 0 , 0 , 350, 1 , 26 ),
  INST(Cdqe            , "cdqe"            , Enc(X86OpAx)           , O(000000,98,_,_,1,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 0 , 0 , 351, 1 , 25 ),
  INST(Clac            , "clac"            , Enc(X86Op)             , O(000F01,CA,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(___W____), 0 , 0 , 0 , 0 , 259, 1 , 27 ),
  INST(Clc             , "clc"             , Enc(X86Op)             , O(000000,F8,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(_____W__), 0 , 0 , 0 , 0 , 259, 1 , 28 ),
  INST(Cld             , "cld"             , Enc(X86Op)             , O(000000,FC,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(______W_), 0 , 0 , 0 , 0 , 259, 1 , 29 ),
  INST(Clflush         , "clflush"         , Enc(X86M_Only)         , O(000F00,AE,7,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , 0 , 0 , 352, 1 , 30 ),
  INST(Clflushopt      , "clflushopt"      , Enc(X86M_Only)         , O(660F00,AE,7,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , 0 , 0 , 352, 1 , 30 ),
  INST(Clwb            , "clwb"            , Enc(X86M_Only)         , O(660F00,AE,6,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , 0 , 0 , 352, 1 , 30 ),
  INST(Clzero          , "clzero"          , Enc(X86Op)             , O(000F01,FC,_,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 353, 1 , 31 ),
  INST(Cmc             , "cmc"             , Enc(X86Op)             , O(000000,F5,_,_,_,_,_,_  ), 0                         , 0                                     , EF(_____X__), 0 , 0 , 0 , 0 , 259, 1 , 32 ),
  INST(Cmova           , "cmova"           , Enc(X86Rm)             , O(000F00,47,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R__R__), 0 , 0 , 0 , 0 , 20 , 3 , 33 ),
  INST(Cmovae          , "cmovae"          , Enc(X86Rm)             , O(000F00,43,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____R__), 0 , 0 , 0 , 0 , 20 , 3 , 34 ),
  INST(Cmovb           , "cmovb"           , Enc(X86Rm)             , O(000F00,42,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____R__), 0 , 0 , 0 , 0 , 20 , 3 , 34 ),
  INST(Cmovbe          , "cmovbe"          , Enc(X86Rm)             , O(000F00,46,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R__R__), 0 , 0 , 0 , 0 , 20 , 3 , 33 ),
  INST(Cmovc           , "cmovc"           , Enc(X86Rm)             , O(000F00,42,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____R__), 0 , 0 , 0 , 0 , 20 , 3 , 34 ),
  INST(Cmove           , "cmove"           , Enc(X86Rm)             , O(000F00,44,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R_____), 0 , 0 , 0 , 0 , 20 , 3 , 35 ),
  INST(Cmovg           , "cmovg"           , Enc(X86Rm)             , O(000F00,4F,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RRR_____), 0 , 0 , 0 , 0 , 20 , 3 , 36 ),
  INST(Cmovge          , "cmovge"          , Enc(X86Rm)             , O(000F00,4D,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RR______), 0 , 0 , 0 , 0 , 20 , 3 , 37 ),
  INST(Cmovl           , "cmovl"           , Enc(X86Rm)             , O(000F00,4C,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RR______), 0 , 0 , 0 , 0 , 20 , 3 , 37 ),
  INST(Cmovle          , "cmovle"          , Enc(X86Rm)             , O(000F00,4E,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RRR_____), 0 , 0 , 0 , 0 , 20 , 3 , 36 ),
  INST(Cmovna          , "cmovna"          , Enc(X86Rm)             , O(000F00,46,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R__R__), 0 , 0 , 0 , 0 , 20 , 3 , 33 ),
  INST(Cmovnae         , "cmovnae"         , Enc(X86Rm)             , O(000F00,42,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____R__), 0 , 0 , 0 , 0 , 20 , 3 , 34 ),
  INST(Cmovnb          , "cmovnb"          , Enc(X86Rm)             , O(000F00,43,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____R__), 0 , 0 , 0 , 0 , 20 , 3 , 34 ),
  INST(Cmovnbe         , "cmovnbe"         , Enc(X86Rm)             , O(000F00,47,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R__R__), 0 , 0 , 0 , 0 , 20 , 3 , 33 ),
  INST(Cmovnc          , "cmovnc"          , Enc(X86Rm)             , O(000F00,43,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____R__), 0 , 0 , 0 , 0 , 20 , 3 , 34 ),
  INST(Cmovne          , "cmovne"          , Enc(X86Rm)             , O(000F00,45,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R_____), 0 , 0 , 0 , 0 , 20 , 3 , 35 ),
  INST(Cmovng          , "cmovng"          , Enc(X86Rm)             , O(000F00,4E,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RRR_____), 0 , 0 , 0 , 0 , 20 , 3 , 36 ),
  INST(Cmovnge         , "cmovnge"         , Enc(X86Rm)             , O(000F00,4C,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RR______), 0 , 0 , 0 , 0 , 20 , 3 , 37 ),
  INST(Cmovnl          , "cmovnl"          , Enc(X86Rm)             , O(000F00,4D,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RR______), 0 , 0 , 0 , 0 , 20 , 3 , 37 ),
  INST(Cmovnle         , "cmovnle"         , Enc(X86Rm)             , O(000F00,4F,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RRR_____), 0 , 0 , 0 , 0 , 20 , 3 , 36 ),
  INST(Cmovno          , "cmovno"          , Enc(X86Rm)             , O(000F00,41,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(R_______), 0 , 0 , 0 , 0 , 20 , 3 , 38 ),
  INST(Cmovnp          , "cmovnp"          , Enc(X86Rm)             , O(000F00,4B,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(____R___), 0 , 0 , 0 , 0 , 20 , 3 , 39 ),
  INST(Cmovns          , "cmovns"          , Enc(X86Rm)             , O(000F00,49,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_R______), 0 , 0 , 0 , 0 , 20 , 3 , 40 ),
  INST(Cmovnz          , "cmovnz"          , Enc(X86Rm)             , O(000F00,45,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R_____), 0 , 0 , 0 , 0 , 20 , 3 , 35 ),
  INST(Cmovo           , "cmovo"           , Enc(X86Rm)             , O(000F00,40,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(R_______), 0 , 0 , 0 , 0 , 20 , 3 , 38 ),
  INST(Cmovp           , "cmovp"           , Enc(X86Rm)             , O(000F00,4A,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(____R___), 0 , 0 , 0 , 0 , 20 , 3 , 39 ),
  INST(Cmovpe          , "cmovpe"          , Enc(X86Rm)             , O(000F00,4A,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(____R___), 0 , 0 , 0 , 0 , 20 , 3 , 39 ),
  INST(Cmovpo          , "cmovpo"          , Enc(X86Rm)             , O(000F00,4B,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(____R___), 0 , 0 , 0 , 0 , 20 , 3 , 39 ),
  INST(Cmovs           , "cmovs"           , Enc(X86Rm)             , O(000F00,48,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_R______), 0 , 0 , 0 , 0 , 20 , 3 , 40 ),
  INST(Cmovz           , "cmovz"           , Enc(X86Rm)             , O(000F00,44,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R_____), 0 , 0 , 0 , 0 , 20 , 3 , 35 ),
  INST(Cmp             , "cmp"             , Enc(X86Arith)          , O(000000,38,7,_,x,_,_,_  ), 0                         , F(RO)                                 , EF(WWWWWW__), 0 , 0 , 0 , 0 , 23 , 10, 41 ),
  INST(Cmppd           , "cmppd"           , Enc(ExtRmi)            , O(660F00,C2,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 290, 1 , 13 ),
  INST(Cmpps           , "cmpps"           , Enc(ExtRmi)            , O(000F00,C2,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 290, 1 , 14 ),
  INST(CmpsB           , "cmps_b"          , Enc(X86Op)             , O(000000,A6,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 42 ),
  INST(CmpsD           , "cmps_d"          , Enc(X86Op)             , O(000000,A7,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 42 ),
  INST(CmpsQ           , "cmps_q"          , Enc(X86Op)             , O(000000,A7,_,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 42 ),
  INST(CmpsW           , "cmps_w"          , Enc(X86Op)             , O(660000,A7,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 42 ),
  INST(Cmpsd           , "cmpsd"           , Enc(ExtRmi)            , O(F20F00,C2,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 251, 2 , 13 ),
  INST(Cmpss           , "cmpss"           , Enc(ExtRmi)            , O(F30F00,C2,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 354, 1 , 14 ),
  INST(Cmpxchg         , "cmpxchg"         , Enc(X86Cmpxchg)        , O(000F00,B0,_,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)|F(Special)              , EF(WWWWWW__), 0 , 0 , 0 , 0 , 107, 4 , 43 ),
  INST(Cmpxchg16b      , "cmpxchg16b"      , Enc(X86M_Only)         , O(000F00,C7,1,_,1,_,_,_  ), 0                         , F(RW)|F(Lock)|F(Special)              , EF(__W_____), 0 , 0 , 0 , 0 , 355, 1 , 44 ),
  INST(Cmpxchg8b       , "cmpxchg8b"       , Enc(X86M_Only)         , O(000F00,C7,1,_,_,_,_,_  ), 0                         , F(RW)|F(Lock)|F(Special)              , EF(__W_____), 0 , 0 , 0 , 0 , 356, 1 , 44 ),
  INST(Comisd          , "comisd"          , Enc(ExtRm)             , O(660F00,2F,_,_,_,_,_,_  ), 0                         , F(RO)                                 , EF(WWWWWW__), 0 , 0 , 8 , 8 , 357, 1 , 45 ),
  INST(Comiss          , "comiss"          , Enc(ExtRm)             , O(000F00,2F,_,_,_,_,_,_  ), 0                         , F(RO)                                 , EF(WWWWWW__), 0 , 0 , 4 , 4 , 358, 1 , 46 ),
  INST(Cpuid           , "cpuid"           , Enc(X86Op)             , O(000F00,A2,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 359, 1 , 47 ),
  INST(Cqo             , "cqo"             , Enc(X86OpDxAx)         , O(000000,99,_,_,1,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 0 , 0 , 360, 1 , 26 ),
  INST(Crc32           , "crc32"           , Enc(X86Crc)            , O(F20F38,F0,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 0 , 0 , 253, 2 , 48 ),
  INST(Cvtdq2pd        , "cvtdq2pd"        , Enc(ExtRm)             , O(F30F00,E6,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8 , 4 , 61 , 1 , 49 ),
  INST(Cvtdq2ps        , "cvtdq2ps"        , Enc(ExtRm)             , O(000F00,5B,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 4 , 4 , 63 , 1 , 50 ),
  INST(Cvtpd2dq        , "cvtpd2dq"        , Enc(ExtRm)             , O(F20F00,E6,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 4 , 8 , 63 , 1 , 51 ),
  INST(Cvtpd2pi        , "cvtpd2pi"        , Enc(ExtRm)             , O(660F00,2D,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 4 , 8 , 361, 1 , 52 ),
  INST(Cvtpd2ps        , "cvtpd2ps"        , Enc(ExtRm)             , O(660F00,5A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 4 , 8 , 63 , 1 , 51 ),
  INST(Cvtpi2pd        , "cvtpi2pd"        , Enc(ExtRm)             , O(660F00,2A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8 , 4 , 362, 1 , 49 ),
  INST(Cvtpi2ps        , "cvtpi2ps"        , Enc(ExtRm)             , O(000F00,2A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 4 , 4 , 362, 1 , 53 ),
  INST(Cvtps2dq        , "cvtps2dq"        , Enc(ExtRm)             , O(660F00,5B,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 4 , 4 , 63 , 1 , 50 ),
  INST(Cvtps2pd        , "cvtps2pd"        , Enc(ExtRm)             , O(000F00,5A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8 , 4 , 61 , 1 , 49 ),
  INST(Cvtps2pi        , "cvtps2pi"        , Enc(ExtRm)             , O(000F00,2D,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 4 , 4 , 363, 1 , 53 ),
  INST(Cvtsd2si        , "cvtsd2si"        , Enc(ExtRm_Wx)          , O(F20F00,2D,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 0 , 8 , 364, 1 , 54 ),
  INST(Cvtsd2ss        , "cvtsd2ss"        , Enc(ExtRm)             , O(F20F00,5A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 4 , 4 , 8 , 61 , 1 , 55 ),
  INST(Cvtsi2sd        , "cvtsi2sd"        , Enc(ExtRm_Wx)          , O(F20F00,2A,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 8 , 0 , 365, 1 , 56 ),
  INST(Cvtsi2ss        , "cvtsi2ss"        , Enc(ExtRm_Wx)          , O(F30F00,2A,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 4 , 4 , 0 , 365, 1 , 57 ),
  INST(Cvtss2sd        , "cvtss2sd"        , Enc(ExtRm)             , O(F30F00,5A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 8 , 4 , 227, 1 , 58 ),
  INST(Cvtss2si        , "cvtss2si"        , Enc(ExtRm_Wx)          , O(F30F00,2D,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 0 , 4 , 309, 1 , 59 ),
  INST(Cvttpd2dq       , "cvttpd2dq"       , Enc(ExtRm)             , O(660F00,E6,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 4 , 8 , 63 , 1 , 51 ),
  INST(Cvttpd2pi       , "cvttpd2pi"       , Enc(ExtRm)             , O(660F00,2C,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 4 , 8 , 361, 1 , 52 ),
  INST(Cvttps2dq       , "cvttps2dq"       , Enc(ExtRm)             , O(F30F00,5B,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 4 , 4 , 63 , 1 , 50 ),
  INST(Cvttps2pi       , "cvttps2pi"       , Enc(ExtRm)             , O(000F00,2C,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 4 , 4 , 363, 1 , 53 ),
  INST(Cvttsd2si       , "cvttsd2si"       , Enc(ExtRm_Wx)          , O(F20F00,2C,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 0 , 8 , 364, 1 , 54 ),
  INST(Cvttss2si       , "cvttss2si"       , Enc(ExtRm_Wx)          , O(F30F00,2C,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 0 , 4 , 309, 1 , 59 ),
  INST(Cwd             , "cwd"             , Enc(X86OpDxAx)         , O(660000,99,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 0 , 0 , 366, 1 , 26 ),
  INST(Cwde            , "cwde"            , Enc(X86OpAx)           , O(000000,98,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 0 , 0 , 367, 1 , 25 ),
  INST(Daa             , "daa"             , Enc(X86Op)             , O(000000,27,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(UWWXWX__), 0 , 0 , 0 , 0 , 368, 1 , 60 ),
  INST(Das             , "das"             , Enc(X86Op)             , O(000000,2F,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(UWWXWX__), 0 , 0 , 0 , 0 , 368, 1 , 60 ),
  INST(Dec             , "dec"             , Enc(X86IncDec)         , O(000000,FE,1,_,x,_,_,_  ), O(000000,48,_,_,x,_,_,_  ), F(RW)|F(Lock)                         , EF(WWWWW___), 0 , 0 , 0 , 0 , 255, 2 , 61 ),
  INST(Div             , "div"             , Enc(X86M_Bx_MulDiv)    , O(000000,F6,6,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(UUUUUU__), 0 , 0 , 0 , 0 , 111, 4 , 62 ),
  INST(Divpd           , "divpd"           , Enc(ExtRm)             , O(660F00,5E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 288, 1 , 4  ),
  INST(Divps           , "divps"           , Enc(ExtRm)             , O(000F00,5E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Divsd           , "divsd"           , Enc(ExtRm)             , O(F20F00,5E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 345, 1 , 4  ),
  INST(Divss           , "divss"           , Enc(ExtRm)             , O(F30F00,5E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 346, 1 , 5  ),
  INST(Dppd            , "dppd"            , Enc(ExtRmi)            , O(660F3A,41,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 290, 1 , 13 ),
  INST(Dpps            , "dpps"            , Enc(ExtRmi)            , O(660F3A,40,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 290, 1 , 14 ),
  INST(Emms            , "emms"            , Enc(X86Op)             , O(000F00,77,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 63 ),
  INST(Enter           , "enter"           , Enc(X86Enter)          , O(000000,C8,_,_,_,_,_,_  ), 0                         , F(Volatile)|F(Special)                , EF(________), 0 , 0 , 0 , 0 , 369, 1 , 64 ),
  INST(Extractps       , "extractps"       , Enc(ExtExtract)        , O(660F3A,17,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 4 , 4 , 370, 1 , 65 ),
  INST(Extrq           , "extrq"           , Enc(ExtExtrq)          , O(660F00,79,_,_,_,_,_,_  ), O(660F00,78,0,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 257, 2 , 66 ),
  INST(F2xm1           , "f2xm1"           , Enc(FpuOp)             , O_FPU(00,D9F0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fabs            , "fabs"            , Enc(FpuOp)             , O_FPU(00,D9E1,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fadd            , "fadd"            , Enc(FpuArith)          , O_FPU(00,C0C0,0)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , 0 , 0 , 149, 3 , 68 ),
  INST(Faddp           , "faddp"           , Enc(FpuRDef)           , O_FPU(00,DEC0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 2 , 69 ),
  INST(Fbld            , "fbld"            , Enc(X86M_Only)         , O_FPU(00,00DF,4)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 371, 1 , 70 ),
  INST(Fbstp           , "fbstp"           , Enc(X86M_Only)         , O_FPU(00,00DF,6)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 371, 1 , 70 ),
  INST(Fchs            , "fchs"            , Enc(FpuOp)             , O_FPU(00,D9E0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fclex           , "fclex"           , Enc(FpuOp)             , O_FPU(9B,DBE2,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fcmovb          , "fcmovb"          , Enc(FpuR)              , O_FPU(00,DAC0,_)          , 0                         , F(Fp)                                 , EF(_____R__), 0 , 0 , 0 , 0 , 260, 1 , 71 ),
  INST(Fcmovbe         , "fcmovbe"         , Enc(FpuR)              , O_FPU(00,DAD0,_)          , 0                         , F(Fp)                                 , EF(__R__R__), 0 , 0 , 0 , 0 , 260, 1 , 72 ),
  INST(Fcmove          , "fcmove"          , Enc(FpuR)              , O_FPU(00,DAC8,_)          , 0                         , F(Fp)                                 , EF(__R_____), 0 , 0 , 0 , 0 , 260, 1 , 73 ),
  INST(Fcmovnb         , "fcmovnb"         , Enc(FpuR)              , O_FPU(00,DBC0,_)          , 0                         , F(Fp)                                 , EF(_____R__), 0 , 0 , 0 , 0 , 260, 1 , 71 ),
  INST(Fcmovnbe        , "fcmovnbe"        , Enc(FpuR)              , O_FPU(00,DBD0,_)          , 0                         , F(Fp)                                 , EF(__R__R__), 0 , 0 , 0 , 0 , 260, 1 , 72 ),
  INST(Fcmovne         , "fcmovne"         , Enc(FpuR)              , O_FPU(00,DBC8,_)          , 0                         , F(Fp)                                 , EF(__R_____), 0 , 0 , 0 , 0 , 260, 1 , 73 ),
  INST(Fcmovnu         , "fcmovnu"         , Enc(FpuR)              , O_FPU(00,DBD8,_)          , 0                         , F(Fp)                                 , EF(____R___), 0 , 0 , 0 , 0 , 260, 1 , 74 ),
  INST(Fcmovu          , "fcmovu"          , Enc(FpuR)              , O_FPU(00,DAD8,_)          , 0                         , F(Fp)                                 , EF(____R___), 0 , 0 , 0 , 0 , 260, 1 , 74 ),
  INST(Fcom            , "fcom"            , Enc(FpuCom)            , O_FPU(00,D0D0,2)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 261, 2 , 75 ),
  INST(Fcomi           , "fcomi"           , Enc(FpuR)              , O_FPU(00,DBF0,_)          , 0                         , F(Fp)                                 , EF(WWWWWW__), 0 , 0 , 0 , 0 , 260, 1 , 76 ),
  INST(Fcomip          , "fcomip"          , Enc(FpuR)              , O_FPU(00,DFF0,_)          , 0                         , F(Fp)                                 , EF(WWWWWW__), 0 , 0 , 0 , 0 , 260, 1 , 76 ),
  INST(Fcomp           , "fcomp"           , Enc(FpuCom)            , O_FPU(00,D8D8,3)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 261, 2 , 75 ),
  INST(Fcompp          , "fcompp"          , Enc(FpuOp)             , O_FPU(00,DED9,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fcos            , "fcos"            , Enc(FpuOp)             , O_FPU(00,D9FF,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fdecstp         , "fdecstp"         , Enc(FpuOp)             , O_FPU(00,D9F6,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fdiv            , "fdiv"            , Enc(FpuArith)          , O_FPU(00,F0F8,6)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , 0 , 0 , 149, 3 , 68 ),
  INST(Fdivp           , "fdivp"           , Enc(FpuRDef)           , O_FPU(00,DEF8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 2 , 69 ),
  INST(Fdivr           , "fdivr"           , Enc(FpuArith)          , O_FPU(00,F8F0,7)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , 0 , 0 , 149, 3 , 68 ),
  INST(Fdivrp          , "fdivrp"          , Enc(FpuRDef)           , O_FPU(00,DEF0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 2 , 69 ),
  INST(Femms           , "femms"           , Enc(X86Op)             , O(000F00,0E,_,_,_,_,_,_  ), 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 77 ),
  INST(Ffree           , "ffree"           , Enc(FpuR)              , O_FPU(00,DDC0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 260, 1 , 78 ),
  INST(Fiadd           , "fiadd"           , Enc(FpuM)              , O_FPU(00,00DA,0)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , 0 , 0 , 372, 1 , 79 ),
  INST(Ficom           , "ficom"           , Enc(FpuM)              , O_FPU(00,00DA,2)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , 0 , 0 , 372, 1 , 79 ),
  INST(Ficomp          , "ficomp"          , Enc(FpuM)              , O_FPU(00,00DA,3)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , 0 , 0 , 372, 1 , 79 ),
  INST(Fidiv           , "fidiv"           , Enc(FpuM)              , O_FPU(00,00DA,6)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , 0 , 0 , 372, 1 , 79 ),
  INST(Fidivr          , "fidivr"          , Enc(FpuM)              , O_FPU(00,00DA,7)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , 0 , 0 , 372, 1 , 79 ),
  INST(Fild            , "fild"            , Enc(FpuM)              , O_FPU(00,00DB,0)          , O_FPU(00,00DF,5)          , F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , EF(________), 0 , 0 , 0 , 0 , 373, 1 , 80 ),
  INST(Fimul           , "fimul"           , Enc(FpuM)              , O_FPU(00,00DA,1)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , 0 , 0 , 372, 1 , 79 ),
  INST(Fincstp         , "fincstp"         , Enc(FpuOp)             , O_FPU(00,D9F7,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Finit           , "finit"           , Enc(FpuOp)             , O_FPU(9B,DBE3,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fist            , "fist"            , Enc(FpuM)              , O_FPU(00,00DB,2)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , 0 , 0 , 372, 1 , 79 ),
  INST(Fistp           , "fistp"           , Enc(FpuM)              , O_FPU(00,00DB,3)          , O_FPU(00,00DF,7)          , F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , EF(________), 0 , 0 , 0 , 0 , 373, 1 , 81 ),
  INST(Fisttp          , "fisttp"          , Enc(FpuM)              , O_FPU(00,00DB,1)          , O_FPU(00,00DD,1)          , F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , EF(________), 0 , 0 , 0 , 0 , 373, 1 , 82 ),
  INST(Fisub           , "fisub"           , Enc(FpuM)              , O_FPU(00,00DA,4)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , 0 , 0 , 372, 1 , 79 ),
  INST(Fisubr          , "fisubr"          , Enc(FpuM)              , O_FPU(00,00DA,5)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , 0 , 0 , 372, 1 , 79 ),
  INST(Fld             , "fld"             , Enc(FpuFldFst)         , O_FPU(00,00D9,0)          , O_FPU(00,00DB,5)          , F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , EF(________), 0 , 0 , 0 , 0 , 374, 1 , 83 ),
  INST(Fld1            , "fld1"            , Enc(FpuOp)             , O_FPU(00,D9E8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fldcw           , "fldcw"           , Enc(X86M_Only)         , O_FPU(00,00D9,5)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 375, 1 , 70 ),
  INST(Fldenv          , "fldenv"          , Enc(X86M_Only)         , O_FPU(00,00D9,4)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 376, 1 , 70 ),
  INST(Fldl2e          , "fldl2e"          , Enc(FpuOp)             , O_FPU(00,D9EA,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fldl2t          , "fldl2t"          , Enc(FpuOp)             , O_FPU(00,D9E9,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fldlg2          , "fldlg2"          , Enc(FpuOp)             , O_FPU(00,D9EC,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fldln2          , "fldln2"          , Enc(FpuOp)             , O_FPU(00,D9ED,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fldpi           , "fldpi"           , Enc(FpuOp)             , O_FPU(00,D9EB,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fldz            , "fldz"            , Enc(FpuOp)             , O_FPU(00,D9EE,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fmul            , "fmul"            , Enc(FpuArith)          , O_FPU(00,C8C8,1)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , 0 , 0 , 149, 3 , 68 ),
  INST(Fmulp           , "fmulp"           , Enc(FpuRDef)           , O_FPU(00,DEC8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 2 , 69 ),
  INST(Fnclex          , "fnclex"          , Enc(FpuOp)             , O_FPU(00,DBE2,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fninit          , "fninit"          , Enc(FpuOp)             , O_FPU(00,DBE3,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fnop            , "fnop"            , Enc(FpuOp)             , O_FPU(00,D9D0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fnsave          , "fnsave"          , Enc(X86M_Only)         , O_FPU(00,00DD,6)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 376, 1 , 70 ),
  INST(Fnstcw          , "fnstcw"          , Enc(X86M_Only)         , O_FPU(00,00D9,7)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 375, 1 , 70 ),
  INST(Fnstenv         , "fnstenv"         , Enc(X86M_Only)         , O_FPU(00,00D9,6)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 376, 1 , 70 ),
  INST(Fnstsw          , "fnstsw"          , Enc(FpuStsw)           , O_FPU(00,00DD,7)          , O_FPU(00,DFE0,_)          , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 377, 1 , 84 ),
  INST(Fpatan          , "fpatan"          , Enc(FpuOp)             , O_FPU(00,D9F3,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fprem           , "fprem"           , Enc(FpuOp)             , O_FPU(00,D9F8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fprem1          , "fprem1"          , Enc(FpuOp)             , O_FPU(00,D9F5,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fptan           , "fptan"           , Enc(FpuOp)             , O_FPU(00,D9F2,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Frndint         , "frndint"         , Enc(FpuOp)             , O_FPU(00,D9FC,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Frstor          , "frstor"          , Enc(X86M_Only)         , O_FPU(00,00DD,4)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 376, 1 , 70 ),
  INST(Fsave           , "fsave"           , Enc(X86M_Only)         , O_FPU(9B,00DD,6)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 376, 1 , 70 ),
  INST(Fscale          , "fscale"          , Enc(FpuOp)             , O_FPU(00,D9FD,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fsin            , "fsin"            , Enc(FpuOp)             , O_FPU(00,D9FE,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fsincos         , "fsincos"         , Enc(FpuOp)             , O_FPU(00,D9FB,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fsqrt           , "fsqrt"           , Enc(FpuOp)             , O_FPU(00,D9FA,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fst             , "fst"             , Enc(FpuFldFst)         , O_FPU(00,00D9,2)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , 0 , 0 , 262, 1 , 85 ),
  INST(Fstcw           , "fstcw"           , Enc(X86M_Only)         , O_FPU(9B,00D9,7)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 375, 1 , 70 ),
  INST(Fstenv          , "fstenv"          , Enc(X86M_Only)         , O_FPU(9B,00D9,6)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 376, 1 , 70 ),
  INST(Fstp            , "fstp"            , Enc(FpuFldFst)         , O_FPU(00,00D9,3)          , O(000000,DB,7,_,_,_,_,_  ), F(Fp)|F(FPU_M4)|F(FPU_M8)|F(FPU_M10)  , EF(________), 0 , 0 , 0 , 0 , 374, 1 , 86 ),
  INST(Fstsw           , "fstsw"           , Enc(FpuStsw)           , O_FPU(9B,00DD,7)          , O_FPU(9B,DFE0,_)          , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 377, 1 , 87 ),
  INST(Fsub            , "fsub"            , Enc(FpuArith)          , O_FPU(00,E0E8,4)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , 0 , 0 , 149, 3 , 68 ),
  INST(Fsubp           , "fsubp"           , Enc(FpuRDef)           , O_FPU(00,DEE8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 2 , 69 ),
  INST(Fsubr           , "fsubr"           , Enc(FpuArith)          , O_FPU(00,E8E0,5)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , 0 , 0 , 149, 3 , 68 ),
  INST(Fsubrp          , "fsubrp"          , Enc(FpuRDef)           , O_FPU(00,DEE0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 2 , 69 ),
  INST(Ftst            , "ftst"            , Enc(FpuOp)             , O_FPU(00,D9E4,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fucom           , "fucom"           , Enc(FpuRDef)           , O_FPU(00,DDE0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 2 , 69 ),
  INST(Fucomi          , "fucomi"          , Enc(FpuR)              , O_FPU(00,DBE8,_)          , 0                         , F(Fp)                                 , EF(WWWWWW__), 0 , 0 , 0 , 0 , 260, 1 , 76 ),
  INST(Fucomip         , "fucomip"         , Enc(FpuR)              , O_FPU(00,DFE8,_)          , 0                         , F(Fp)                                 , EF(WWWWWW__), 0 , 0 , 0 , 0 , 260, 1 , 76 ),
  INST(Fucomp          , "fucomp"          , Enc(FpuRDef)           , O_FPU(00,DDE8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 2 , 69 ),
  INST(Fucompp         , "fucompp"         , Enc(FpuOp)             , O_FPU(00,DAE9,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fwait           , "fwait"           , Enc(X86Op)             , O_FPU(00,00DB,_)          , 0                         , F(Fp)|F(Volatile)                     , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 88 ),
  INST(Fxam            , "fxam"            , Enc(FpuOp)             , O_FPU(00,D9E5,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fxch            , "fxch"            , Enc(FpuR)              , O_FPU(00,D9C8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 2 , 78 ),
  INST(Fxrstor         , "fxrstor"         , Enc(X86M_Only)         , O(000F00,AE,1,_,_,_,_,_  ), 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 376, 1 , 70 ),
  INST(Fxrstor64       , "fxrstor64"       , Enc(X86M_Only)         , O(000F00,AE,1,_,1,_,_,_  ), 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 378, 1 , 70 ),
  INST(Fxsave          , "fxsave"          , Enc(X86M_Only)         , O(000F00,AE,0,_,_,_,_,_  ), 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 376, 1 , 70 ),
  INST(Fxsave64        , "fxsave64"        , Enc(X86M_Only)         , O(000F00,AE,0,_,1,_,_,_  ), 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 378, 1 , 70 ),
  INST(Fxtract         , "fxtract"         , Enc(FpuOp)             , O_FPU(00,D9F4,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fyl2x           , "fyl2x"           , Enc(FpuOp)             , O_FPU(00,D9F1,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Fyl2xp1         , "fyl2xp1"         , Enc(FpuOp)             , O_FPU(00,D9F9,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 67 ),
  INST(Haddpd          , "haddpd"          , Enc(ExtRm)             , O(660F00,7C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 288, 1 , 4  ),
  INST(Haddps          , "haddps"          , Enc(ExtRm)             , O(F20F00,7C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Hsubpd          , "hsubpd"          , Enc(ExtRm)             , O(660F00,7D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 288, 1 , 4  ),
  INST(Hsubps          , "hsubps"          , Enc(ExtRm)             , O(F20F00,7D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Idiv            , "idiv"            , Enc(X86M_Bx_MulDiv)    , O(000000,F6,7,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(UUUUUU__), 0 , 0 , 0 , 0 , 115, 4 , 62 ),
  INST(Imul            , "imul"            , Enc(X86Imul)           , O(000000,F6,5,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(WUUUUW__), 0 , 0 , 0 , 0 , 33 , 10, 89 ),
  INST(Inc             , "inc"             , Enc(X86IncDec)         , O(000000,FE,0,_,x,_,_,_  ), O(000000,40,_,_,x,_,_,_  ), F(RW)|F(Lock)                         , EF(WWWWW___), 0 , 0 , 0 , 0 , 255, 2 , 90 ),
  INST(Insertps        , "insertps"        , Enc(ExtRmi)            , O(660F3A,21,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 354, 1 , 14 ),
  INST(Insertq         , "insertq"         , Enc(ExtInsertq)        , O(F20F00,79,_,_,_,_,_,_  ), O(F20F00,78,_,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 263, 2 , 91 ),
  INST(Int             , "int"             , Enc(X86Int)            , O(000000,CD,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(___W___W), 0 , 0 , 0 , 0 , 379, 1 , 92 ),
  INST(Int3            , "int3"            , Enc(X86Op)             , O(000000,CC,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(___W___W), 0 , 0 , 0 , 0 , 259, 1 , 93 ),
  INST(Into            , "into"            , Enc(X86Op)             , O(000000,CE,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(___W___W), 0 , 0 , 0 , 0 , 259, 1 , 93 ),
  INST(Ja              , "ja"              , Enc(X86Jcc)            , O(000000,77,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(__R__R__), 0 , 0 , 0 , 0 , 380, 1 , 94 ),
  INST(Jae             , "jae"             , Enc(X86Jcc)            , O(000000,73,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(_____R__), 0 , 0 , 0 , 0 , 380, 1 , 95 ),
  INST(Jb              , "jb"              , Enc(X86Jcc)            , O(000000,72,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(_____R__), 0 , 0 , 0 , 0 , 380, 1 , 95 ),
  INST(Jbe             , "jbe"             , Enc(X86Jcc)            , O(000000,76,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(__R__R__), 0 , 0 , 0 , 0 , 380, 1 , 94 ),
  INST(Jc              , "jc"              , Enc(X86Jcc)            , O(000000,72,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(_____R__), 0 , 0 , 0 , 0 , 381, 1 , 95 ),
  INST(Je              , "je"              , Enc(X86Jcc)            , O(000000,74,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(__R_____), 0 , 0 , 0 , 0 , 380, 1 , 96 ),
  INST(Jg              , "jg"              , Enc(X86Jcc)            , O(000000,7F,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(RRR_____), 0 , 0 , 0 , 0 , 380, 1 , 97 ),
  INST(Jge             , "jge"             , Enc(X86Jcc)            , O(000000,7D,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(RR______), 0 , 0 , 0 , 0 , 380, 1 , 98 ),
  INST(Jl              , "jl"              , Enc(X86Jcc)            , O(000000,7C,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(RR______), 0 , 0 , 0 , 0 , 380, 1 , 98 ),
  INST(Jle             , "jle"             , Enc(X86Jcc)            , O(000000,7E,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(RRR_____), 0 , 0 , 0 , 0 , 380, 1 , 97 ),
  INST(Jna             , "jna"             , Enc(X86Jcc)            , O(000000,76,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(__R__R__), 0 , 0 , 0 , 0 , 380, 1 , 94 ),
  INST(Jnae            , "jnae"            , Enc(X86Jcc)            , O(000000,72,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(_____R__), 0 , 0 , 0 , 0 , 380, 1 , 95 ),
  INST(Jnb             , "jnb"             , Enc(X86Jcc)            , O(000000,73,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(_____R__), 0 , 0 , 0 , 0 , 380, 1 , 95 ),
  INST(Jnbe            , "jnbe"            , Enc(X86Jcc)            , O(000000,77,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(__R__R__), 0 , 0 , 0 , 0 , 380, 1 , 94 ),
  INST(Jnc             , "jnc"             , Enc(X86Jcc)            , O(000000,73,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(_____R__), 0 , 0 , 0 , 0 , 381, 1 , 95 ),
  INST(Jne             , "jne"             , Enc(X86Jcc)            , O(000000,75,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(__R_____), 0 , 0 , 0 , 0 , 380, 1 , 96 ),
  INST(Jng             , "jng"             , Enc(X86Jcc)            , O(000000,7E,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(RRR_____), 0 , 0 , 0 , 0 , 380, 1 , 97 ),
  INST(Jnge            , "jnge"            , Enc(X86Jcc)            , O(000000,7C,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(RR______), 0 , 0 , 0 , 0 , 380, 1 , 98 ),
  INST(Jnl             , "jnl"             , Enc(X86Jcc)            , O(000000,7D,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(RR______), 0 , 0 , 0 , 0 , 380, 1 , 98 ),
  INST(Jnle            , "jnle"            , Enc(X86Jcc)            , O(000000,7F,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(RRR_____), 0 , 0 , 0 , 0 , 380, 1 , 97 ),
  INST(Jno             , "jno"             , Enc(X86Jcc)            , O(000000,71,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(R_______), 0 , 0 , 0 , 0 , 380, 1 , 99 ),
  INST(Jnp             , "jnp"             , Enc(X86Jcc)            , O(000000,7B,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(____R___), 0 , 0 , 0 , 0 , 380, 1 , 100),
  INST(Jns             , "jns"             , Enc(X86Jcc)            , O(000000,79,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(_R______), 0 , 0 , 0 , 0 , 380, 1 , 101),
  INST(Jnz             , "jnz"             , Enc(X86Jcc)            , O(000000,75,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(__R_____), 0 , 0 , 0 , 0 , 380, 1 , 96 ),
  INST(Jo              , "jo"              , Enc(X86Jcc)            , O(000000,70,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(R_______), 0 , 0 , 0 , 0 , 380, 1 , 99 ),
  INST(Jp              , "jp"              , Enc(X86Jcc)            , O(000000,7A,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(____R___), 0 , 0 , 0 , 0 , 380, 1 , 100),
  INST(Jpe             , "jpe"             , Enc(X86Jcc)            , O(000000,7A,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(____R___), 0 , 0 , 0 , 0 , 380, 1 , 100),
  INST(Jpo             , "jpo"             , Enc(X86Jcc)            , O(000000,7B,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(____R___), 0 , 0 , 0 , 0 , 380, 1 , 100),
  INST(Js              , "js"              , Enc(X86Jcc)            , O(000000,78,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(_R______), 0 , 0 , 0 , 0 , 380, 1 , 101),
  INST(Jz              , "jz"              , Enc(X86Jcc)            , O(000000,74,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(__R_____), 0 , 0 , 0 , 0 , 380, 1 , 96 ),
  INST(Jecxz           , "jecxz"           , Enc(X86Jecxz)          , O(000000,E3,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)|F(Special)        , EF(________), 0 , 0 , 0 , 0 , 265, 2 , 102),
  INST(Jmp             , "jmp"             , Enc(X86Jmp)            , O(000000,FF,4,_,_,_,_,_  ), O(000000,E9,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(________), 0 , 0 , 0 , 0 , 267, 2 , 103),
  INST(Kaddb           , "kaddb"           , Enc(VexRvm)            , V(660F00,4A,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1 , 1 , 382, 1 , 104),
  INST(Kaddd           , "kaddd"           , Enc(VexRvm)            , V(660F00,4A,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 382, 1 , 105),
  INST(Kaddq           , "kaddq"           , Enc(VexRvm)            , V(000F00,4A,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 382, 1 , 106),
  INST(Kaddw           , "kaddw"           , Enc(VexRvm)            , V(000F00,4A,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 382, 1 , 107),
  INST(Kandb           , "kandb"           , Enc(VexRvm)            , V(660F00,41,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1 , 1 , 382, 1 , 104),
  INST(Kandd           , "kandd"           , Enc(VexRvm)            , V(660F00,41,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 382, 1 , 105),
  INST(Kandnb          , "kandnb"          , Enc(VexRvm)            , V(660F00,42,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1 , 1 , 382, 1 , 104),
  INST(Kandnd          , "kandnd"          , Enc(VexRvm)            , V(660F00,42,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 382, 1 , 105),
  INST(Kandnq          , "kandnq"          , Enc(VexRvm)            , V(000F00,42,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 382, 1 , 106),
  INST(Kandnw          , "kandnw"          , Enc(VexRvm)            , V(000F00,42,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 382, 1 , 107),
  INST(Kandq           , "kandq"           , Enc(VexRvm)            , V(000F00,41,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 382, 1 , 106),
  INST(Kandw           , "kandw"           , Enc(VexRvm)            , V(000F00,41,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 382, 1 , 107),
  INST(Kmovb           , "kmovb"           , Enc(VexKmov)           , V(660F00,90,_,0,0,_,_,_  ), V(660F00,92,_,0,0,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1 , 1 , 269, 2 , 108),
  INST(Kmovd           , "kmovd"           , Enc(VexKmov)           , V(660F00,90,_,0,1,_,_,_  ), V(F20F00,92,_,0,0,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 271, 2 , 109),
  INST(Kmovq           , "kmovq"           , Enc(VexKmov)           , V(000F00,90,_,0,1,_,_,_  ), V(F20F00,92,_,0,1,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 273, 2 , 110),
  INST(Kmovw           , "kmovw"           , Enc(VexKmov)           , V(000F00,90,_,0,0,_,_,_  ), V(000F00,92,_,0,0,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 275, 2 , 111),
  INST(Knotb           , "knotb"           , Enc(VexRm)             , V(660F00,44,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1 , 1 , 383, 1 , 112),
  INST(Knotd           , "knotd"           , Enc(VexRm)             , V(660F00,44,_,0,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 383, 1 , 113),
  INST(Knotq           , "knotq"           , Enc(VexRm)             , V(000F00,44,_,0,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 383, 1 , 114),
  INST(Knotw           , "knotw"           , Enc(VexRm)             , V(000F00,44,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 383, 1 , 115),
  INST(Korb            , "korb"            , Enc(VexRvm)            , V(660F00,45,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1 , 1 , 382, 1 , 104),
  INST(Kord            , "kord"            , Enc(VexRvm)            , V(660F00,45,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 382, 1 , 105),
  INST(Korq            , "korq"            , Enc(VexRvm)            , V(000F00,45,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 382, 1 , 106),
  INST(Kortestb        , "kortestb"        , Enc(VexRm)             , V(660F00,98,_,0,0,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 1 , 1 , 384, 1 , 116),
  INST(Kortestd        , "kortestd"        , Enc(VexRm)             , V(660F00,98,_,0,1,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 4 , 4 , 384, 1 , 117),
  INST(Kortestq        , "kortestq"        , Enc(VexRm)             , V(000F00,98,_,0,1,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 8 , 8 , 384, 1 , 118),
  INST(Kortestw        , "kortestw"        , Enc(VexRm)             , V(000F00,98,_,0,0,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 2 , 2 , 384, 1 , 119),
  INST(Korw            , "korw"            , Enc(VexRvm)            , V(000F00,45,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 382, 1 , 107),
  INST(Kshiftlb        , "kshiftlb"        , Enc(VexRmi)            , V(660F3A,32,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1 , 1 , 385, 1 , 120),
  INST(Kshiftld        , "kshiftld"        , Enc(VexRmi)            , V(660F3A,33,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 385, 1 , 121),
  INST(Kshiftlq        , "kshiftlq"        , Enc(VexRmi)            , V(660F3A,33,_,0,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 385, 1 , 122),
  INST(Kshiftlw        , "kshiftlw"        , Enc(VexRmi)            , V(660F3A,32,_,0,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 385, 1 , 123),
  INST(Kshiftrb        , "kshiftrb"        , Enc(VexRmi)            , V(660F3A,30,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1 , 1 , 385, 1 , 120),
  INST(Kshiftrd        , "kshiftrd"        , Enc(VexRmi)            , V(660F3A,31,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 385, 1 , 121),
  INST(Kshiftrq        , "kshiftrq"        , Enc(VexRmi)            , V(660F3A,31,_,0,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 385, 1 , 122),
  INST(Kshiftrw        , "kshiftrw"        , Enc(VexRmi)            , V(660F3A,30,_,0,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 385, 1 , 123),
  INST(Ktestb          , "ktestb"          , Enc(VexRm)             , V(660F00,99,_,0,0,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 1 , 1 , 384, 1 , 116),
  INST(Ktestd          , "ktestd"          , Enc(VexRm)             , V(660F00,99,_,0,1,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 4 , 4 , 384, 1 , 117),
  INST(Ktestq          , "ktestq"          , Enc(VexRm)             , V(000F00,99,_,0,1,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 8 , 8 , 384, 1 , 118),
  INST(Ktestw          , "ktestw"          , Enc(VexRm)             , V(000F00,99,_,0,0,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 2 , 2 , 384, 1 , 119),
  INST(Kunpckbw        , "kunpckbw"        , Enc(VexRvm)            , V(660F00,4B,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 1 , 382, 1 , 124),
  INST(Kunpckdq        , "kunpckdq"        , Enc(VexRvm)            , V(000F00,4B,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 4 , 382, 1 , 125),
  INST(Kunpckwd        , "kunpckwd"        , Enc(VexRvm)            , V(000F00,4B,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 2 , 382, 1 , 126),
  INST(Kxnorb          , "kxnorb"          , Enc(VexRvm)            , V(660F00,46,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1 , 1 , 382, 1 , 104),
  INST(Kxnord          , "kxnord"          , Enc(VexRvm)            , V(660F00,46,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 382, 1 , 105),
  INST(Kxnorq          , "kxnorq"          , Enc(VexRvm)            , V(000F00,46,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 382, 1 , 106),
  INST(Kxnorw          , "kxnorw"          , Enc(VexRvm)            , V(000F00,46,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 382, 1 , 107),
  INST(Kxorb           , "kxorb"           , Enc(VexRvm)            , V(660F00,47,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1 , 1 , 382, 1 , 104),
  INST(Kxord           , "kxord"           , Enc(VexRvm)            , V(660F00,47,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 382, 1 , 105),
  INST(Kxorq           , "kxorq"           , Enc(VexRvm)            , V(000F00,47,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 382, 1 , 106),
  INST(Kxorw           , "kxorw"           , Enc(VexRvm)            , V(000F00,47,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 382, 1 , 107),
  INST(Lahf            , "lahf"            , Enc(X86Op)             , O(000000,9F,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(_RRRRR__), 0 , 0 , 0 , 0 , 386, 1 , 127),
  INST(Lddqu           , "lddqu"           , Enc(ExtRm)             , O(F20F00,F0,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 16 , 16 , 200, 1 , 128),
  INST(Ldmxcsr         , "ldmxcsr"         , Enc(X86M_Only)         , O(000F00,AE,2,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , 0 , 0 , 387, 1 , 30 ),
  INST(Lea             , "lea"             , Enc(X86Lea)            , O(000000,8D,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 0 , 0 , 388, 1 , 129),
  INST(Leave           , "leave"           , Enc(X86Op)             , O(000000,C9,_,_,_,_,_,_  ), 0                         , F(Volatile)|F(Special)                , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 130),
  INST(Lfence          , "lfence"          , Enc(X86Fence)          , O(000F00,AE,5,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 131),
  INST(LodsB           , "lods_b"          , Enc(X86Op)             , O(000000,AC,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(______R_), 0 , 1 , 0 , 0 , 0  , 0 , 132),
  INST(LodsD           , "lods_d"          , Enc(X86Op)             , O(000000,AD,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(______R_), 0 , 4 , 0 , 0 , 0  , 0 , 133),
  INST(LodsQ           , "lods_q"          , Enc(X86Op)             , O(000000,AD,_,_,1,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(______R_), 0 , 8 , 0 , 0 , 0  , 0 , 134),
  INST(LodsW           , "lods_w"          , Enc(X86Op)             , O(660000,AD,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(______R_), 0 , 2 , 0 , 0 , 0  , 0 , 135),
  INST(Lzcnt           , "lzcnt"           , Enc(X86Rm)             , O(F30F00,BD,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(UUWUUW__), 0 , 0 , 0 , 0 , 152, 3 , 18 ),
  INST(Maskmovdqu      , "maskmovdqu"      , Enc(ExtRmZDI)          , O(660F00,57,_,_,_,_,_,_  ), 0                         , F(RO)|F(Special)                      , EF(________), 0 , 0 , 16, 16, 389, 1 , 136),
  INST(Maskmovq        , "maskmovq"        , Enc(ExtRmZDI)          , O(000F00,F7,_,_,_,_,_,_  ), 0                         , F(RO)|F(Special)                      , EF(________), 0 , 0 , 8 , 8 , 390, 1 , 137),
  INST(Maxpd           , "maxpd"           , Enc(ExtRm)             , O(660F00,5F,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 288, 1 , 4  ),
  INST(Maxps           , "maxps"           , Enc(ExtRm)             , O(000F00,5F,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Maxsd           , "maxsd"           , Enc(ExtRm)             , O(F20F00,5F,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 345, 1 , 4  ),
  INST(Maxss           , "maxss"           , Enc(ExtRm)             , O(F30F00,5F,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 346, 1 , 5  ),
  INST(Mfence          , "mfence"          , Enc(X86Fence)          , O(000F00,AE,6,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)                     , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 138),
  INST(Minpd           , "minpd"           , Enc(ExtRm)             , O(660F00,5D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 288, 1 , 4  ),
  INST(Minps           , "minps"           , Enc(ExtRm)             , O(000F00,5D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Minsd           , "minsd"           , Enc(ExtRm)             , O(F20F00,5D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 345, 1 , 4  ),
  INST(Minss           , "minss"           , Enc(ExtRm)             , O(F30F00,5D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 346, 1 , 5  ),
  INST(Monitor         , "monitor"         , Enc(X86Op)             , O(000F01,C8,_,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 0  , 0 , 139),
  INST(Mov             , "mov"             , Enc(X86Mov)            , 0                         , 0                         , F(WO)                                 , EF(________), 0 , 0 , 0 , 0 , 0  , 13, 140),
  INST(Movapd          , "movapd"          , Enc(ExtMov)            , O(660F00,28,_,_,_,_,_,_  ), O(660F00,29,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 8 , 8 , 63 , 2 , 141),
  INST(Movaps          , "movaps"          , Enc(ExtMov)            , O(000F00,28,_,_,_,_,_,_  ), O(000F00,29,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 4 , 4 , 63 , 2 , 142),
  INST(Movbe           , "movbe"           , Enc(ExtMovbe)          , O(000F38,F0,_,_,x,_,_,_  ), O(000F38,F1,_,_,x,_,_,_  ), F(WO)                                 , EF(________), 0 , 0 , 0 , 0 , 51 , 6 , 143),
  INST(Movd            , "movd"            , Enc(ExtMovd)           , O(000F00,6E,_,_,_,_,_,_  ), O(000F00,7E,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 4 , 4 , 277, 2 , 144),
  INST(Movddup         , "movddup"         , Enc(ExtMov)            , O(F20F00,12,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8 , 8 , 61 , 1 , 145),
  INST(Movdq2q         , "movdq2q"         , Enc(ExtMov)            , O(F20F00,D6,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 8 , 8 , 391, 1 , 146),
  INST(Movdqa          , "movdqa"          , Enc(ExtMov)            , O(660F00,6F,_,_,_,_,_,_  ), O(660F00,7F,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 16, 16, 63 , 2 , 147),
  INST(Movdqu          , "movdqu"          , Enc(ExtMov)            , O(F30F00,6F,_,_,_,_,_,_  ), O(F30F00,7F,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 16, 16, 63 , 2 , 148),
  INST(Movhlps         , "movhlps"         , Enc(ExtMov)            , O(000F00,12,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 4 , 4 , 392, 1 , 149),
  INST(Movhpd          , "movhpd"          , Enc(ExtMov)            , O(660F00,16,_,_,_,_,_,_  ), O(660F00,17,_,_,_,_,_,_  ), F(RW)                                 , EF(________), 8 , 8 , 8 , 8 , 206, 2 , 150),
  INST(Movhps          , "movhps"          , Enc(ExtMov)            , O(000F00,16,_,_,_,_,_,_  ), O(000F00,17,_,_,_,_,_,_  ), F(RW)                                 , EF(________), 8 , 8 , 4 , 4 , 206, 2 , 151),
  INST(Movlhps         , "movlhps"         , Enc(ExtMov)            , O(000F00,16,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 8 , 8 , 4 , 4 , 392, 1 , 152),
  INST(Movlpd          , "movlpd"          , Enc(ExtMov)            , O(660F00,12,_,_,_,_,_,_  ), O(660F00,13,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 8 , 8 , 8 , 206, 2 , 153),
  INST(Movlps          , "movlps"          , Enc(ExtMov)            , O(000F00,12,_,_,_,_,_,_  ), O(000F00,13,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 8 , 4 , 4 , 206, 2 , 154),
  INST(Movmskpd        , "movmskpd"        , Enc(ExtMov)            , O(660F00,50,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 0 , 8 , 393, 1 , 155),
  INST(Movmskps        , "movmskps"        , Enc(ExtMov)            , O(000F00,50,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 0 , 4 , 393, 1 , 156),
  INST(Movntdq         , "movntdq"         , Enc(ExtMov)            , 0                         , O(660F00,E7,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 0 , 0 , 197, 1 , 157),
  INST(Movntdqa        , "movntdqa"        , Enc(ExtMov)            , O(660F38,2A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 0 , 0 , 200, 1 , 158),
  INST(Movnti          , "movnti"          , Enc(ExtMovnti)         , O(000F00,C3,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 0 , 0 , 55 , 2 , 159),
  INST(Movntpd         , "movntpd"         , Enc(ExtMov)            , 0                         , O(660F00,2B,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 8 , 8 , 197, 1 , 160),
  INST(Movntps         , "movntps"         , Enc(ExtMov)            , 0                         , O(000F00,2B,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 4 , 4 , 197, 1 , 161),
  INST(Movntq          , "movntq"          , Enc(ExtMov)            , 0                         , O(000F00,E7,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 8 , 0 , 0 , 394, 1 , 162),
  INST(Movntsd         , "movntsd"         , Enc(ExtMov)            , 0                         , O(F20F00,2B,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 8 , 8 , 8 , 157, 1 , 163),
  INST(Movntss         , "movntss"         , Enc(ExtMov)            , 0                         , O(F30F00,2B,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 4 , 4 , 4 , 280, 1 , 164),
  INST(Movq            , "movq"            , Enc(ExtMovq)           , O(000F00,6E,_,_,x,_,_,_  ), O(000F00,7E,_,_,x,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 8 , 8 , 57 , 6 , 165),
  INST(Movq2dq         , "movq2dq"         , Enc(ExtRm)             , O(F30F00,D6,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8 , 8 , 395, 1 , 166),
  INST(MovsB           , "movs_b"          , Enc(X86Op)             , O(000000,A4,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , 0 , 0 , 0  , 0 , 167),
  INST(MovsD           , "movs_d"          , Enc(X86Op)             , O(000000,A5,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , 0 , 0 , 0  , 0 , 167),
  INST(MovsQ           , "movs_q"          , Enc(X86Op)             , O(000000,A5,_,_,1,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , 0 , 0 , 0  , 0 , 167),
  INST(MovsW           , "movs_w"          , Enc(X86Op)             , O(660000,A5,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , 0 , 0 , 0  , 0 , 167),
  INST(Movsd           , "movsd"           , Enc(ExtMov)            , O(F20F00,10,_,_,_,_,_,_  ), O(F20F00,11,_,_,_,_,_,_  ), F(WO)|F(ZeroIfMem)                    , EF(________), 0 , 8 , 8 , 8 , 155, 3 , 168),
  INST(Movshdup        , "movshdup"        , Enc(ExtRm)             , O(F30F00,16,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 0 , 0 , 63 , 1 , 169),
  INST(Movsldup        , "movsldup"        , Enc(ExtRm)             , O(F30F00,12,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 0 , 0 , 63 , 1 , 169),
  INST(Movss           , "movss"           , Enc(ExtMov)            , O(F30F00,10,_,_,_,_,_,_  ), O(F30F00,11,_,_,_,_,_,_  ), F(WO)|F(ZeroIfMem)                    , EF(________), 0 , 4 , 4 , 4 , 279, 2 , 170),
  INST(Movsx           , "movsx"           , Enc(X86MovsxMovzx)     , O(000F00,BE,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 0 , 0 , 281, 2 , 171),
  INST(Movsxd          , "movsxd"          , Enc(X86Rm)             , O(000000,63,_,_,1,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 0 , 0 , 396, 1 , 172),
  INST(Movupd          , "movupd"          , Enc(ExtMov)            , O(660F00,10,_,_,_,_,_,_  ), O(660F00,11,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 8 , 8 , 63 , 2 , 173),
  INST(Movups          , "movups"          , Enc(ExtMov)            , O(000F00,10,_,_,_,_,_,_  ), O(000F00,11,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 4 , 4 , 63 , 2 , 174),
  INST(Movzx           , "movzx"           , Enc(X86MovsxMovzx)     , O(000F00,B6,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 0 , 0 , 281, 2 , 171),
  INST(Mpsadbw         , "mpsadbw"         , Enc(ExtRmi)            , O(660F3A,42,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 1 , 290, 1 , 175),
  INST(Mul             , "mul"             , Enc(X86M_Bx_MulDiv)    , O(000000,F6,4,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(WUUUUW__), 0 , 0 , 0 , 0 , 33 , 4 , 62 ),
  INST(Mulpd           , "mulpd"           , Enc(ExtRm)             , O(660F00,59,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 288, 1 , 4  ),
  INST(Mulps           , "mulps"           , Enc(ExtRm)             , O(000F00,59,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Mulsd           , "mulsd"           , Enc(ExtRm)             , O(F20F00,59,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 345, 1 , 4  ),
  INST(Mulss           , "mulss"           , Enc(ExtRm)             , O(F30F00,59,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 346, 1 , 5  ),
  INST(Mulx            , "mulx"            , Enc(VexRvmZDX_Wx)      , V(F20F38,F6,_,0,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 0 , 0 , 283, 2 , 176),
  INST(Mwait           , "mwait"           , Enc(X86Op)             , O(000F01,C9,_,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 0  , 0 , 139),
  INST(Neg             , "neg"             , Enc(X86M_Bx)           , O(000000,F6,3,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWWWW__), 0 , 0 , 0 , 0 , 256, 1 , 177),
  INST(Nop             , "nop"             , Enc(X86Op)             , O(000000,90,_,_,_,_,_,_  ), 0                         , 0                                     , EF(________), 0 , 0 , 0 , 0 , 285, 2 , 178),
  INST(Not             , "not"             , Enc(X86M_Bx)           , O(000000,F6,2,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(________), 0 , 0 , 0 , 0 , 256, 1 , 179),
  INST(Or              , "or"              , Enc(X86Arith)          , O(000000,08,1,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWUWW__), 0 , 0 , 0 , 0 , 13 , 10, 3  ),
  INST(Orpd            , "orpd"            , Enc(ExtRm)             , O(660F00,56,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 288, 1 , 4  ),
  INST(Orps            , "orps"            , Enc(ExtRm)             , O(000F00,56,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Pabsb           , "pabsb"           , Enc(ExtRm_P)           , O(000F38,1C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 1 , 287, 2 , 180),
  INST(Pabsd           , "pabsd"           , Enc(ExtRm_P)           , O(000F38,1E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 2 , 181),
  INST(Pabsw           , "pabsw"           , Enc(ExtRm_P)           , O(000F38,1D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Packssdw        , "packssdw"        , Enc(ExtRm_P)           , O(000F00,6B,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 4 , 287, 2 , 183),
  INST(Packsswb        , "packsswb"        , Enc(ExtRm_P)           , O(000F00,63,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 2 , 287, 2 , 184),
  INST(Packusdw        , "packusdw"        , Enc(ExtRm)             , O(660F38,2B,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 4 , 288, 1 , 185),
  INST(Packuswb        , "packuswb"        , Enc(ExtRm_P)           , O(000F00,67,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 2 , 287, 2 , 184),
  INST(Paddb           , "paddb"           , Enc(ExtRm_P)           , O(000F00,FC,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 1 , 287, 2 , 180),
  INST(Paddd           , "paddd"           , Enc(ExtRm_P)           , O(000F00,FE,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 2 , 181),
  INST(Paddq           , "paddq"           , Enc(ExtRm_P)           , O(000F00,D4,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Paddsb          , "paddsb"          , Enc(ExtRm_P)           , O(000F00,EC,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 1 , 287, 2 , 180),
  INST(Paddsw          , "paddsw"          , Enc(ExtRm_P)           , O(000F00,ED,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Paddusb         , "paddusb"         , Enc(ExtRm_P)           , O(000F00,DC,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 1 , 287, 2 , 180),
  INST(Paddusw         , "paddusw"         , Enc(ExtRm_P)           , O(000F00,DD,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Paddw           , "paddw"           , Enc(ExtRm_P)           , O(000F00,FD,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Palignr         , "palignr"         , Enc(ExtRmi_P)          , O(000F3A,0F,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 0 , 0 , 289, 2 , 186),
  INST(Pand            , "pand"            , Enc(ExtRm_P)           , O(000F00,DB,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 16, 16, 287, 2 , 187),
  INST(Pandn           , "pandn"           , Enc(ExtRm_P)           , O(000F00,DF,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 16, 16, 287, 2 , 187),
  INST(Pause           , "pause"           , Enc(X86Op)             , O(F30000,90,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 188),
  INST(Pavgb           , "pavgb"           , Enc(ExtRm_P)           , O(000F00,E0,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 1 , 287, 2 , 180),
  INST(Pavgusb         , "pavgusb"         , Enc(Ext3dNow)          , O(000F0F,BF,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 1 , 287, 1 , 189),
  INST(Pavgw           , "pavgw"           , Enc(ExtRm_P)           , O(000F00,E3,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Pblendvb        , "pblendvb"        , Enc(ExtRmXMM0)         , O(660F38,10,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 1 , 1 , 347, 1 , 190),
  INST(Pblendw         , "pblendw"         , Enc(ExtRmi)            , O(660F3A,0E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 290, 1 , 191),
  INST(Pclmulqdq       , "pclmulqdq"       , Enc(ExtRmi)            , O(660F3A,44,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 16, 8 , 290, 1 , 192),
  INST(Pcmpeqb         , "pcmpeqb"         , Enc(ExtRm_P)           , O(000F00,74,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 1 , 287, 2 , 180),
  INST(Pcmpeqd         , "pcmpeqd"         , Enc(ExtRm_P)           , O(000F00,76,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 2 , 181),
  INST(Pcmpeqq         , "pcmpeqq"         , Enc(ExtRm)             , O(660F38,29,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 288, 1 , 4  ),
  INST(Pcmpeqw         , "pcmpeqw"         , Enc(ExtRm_P)           , O(000F00,75,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Pcmpestri       , "pcmpestri"       , Enc(ExtRmi)            , O(660F3A,61,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , 0 , 0 , 397, 1 , 193),
  INST(Pcmpestrm       , "pcmpestrm"       , Enc(ExtRmi)            , O(660F3A,60,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , 0 , 0 , 398, 1 , 193),
  INST(Pcmpgtb         , "pcmpgtb"         , Enc(ExtRm_P)           , O(000F00,64,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 1 , 287, 2 , 180),
  INST(Pcmpgtd         , "pcmpgtd"         , Enc(ExtRm_P)           , O(000F00,66,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 2 , 181),
  INST(Pcmpgtq         , "pcmpgtq"         , Enc(ExtRm)             , O(660F38,37,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 288, 1 , 4  ),
  INST(Pcmpgtw         , "pcmpgtw"         , Enc(ExtRm_P)           , O(000F00,65,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Pcmpistri       , "pcmpistri"       , Enc(ExtRmi)            , O(660F3A,63,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , 0 , 0 , 399, 1 , 193),
  INST(Pcmpistrm       , "pcmpistrm"       , Enc(ExtRmi)            , O(660F3A,62,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , 0 , 0 , 400, 1 , 193),
  INST(Pcommit         , "pcommit"         , Enc(X86Op_O)           , O(660F00,AE,7,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 194),
  INST(Pdep            , "pdep"            , Enc(VexRvm_Wx)         , V(F20F38,F5,_,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 0 , 0 , 245, 2 , 195),
  INST(Pext            , "pext"            , Enc(VexRvm_Wx)         , V(F30F38,F5,_,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 0 , 0 , 245, 2 , 195),
  INST(Pextrb          , "pextrb"          , Enc(ExtExtract)        , O(000F3A,14,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 1 , 1 , 401, 1 , 196),
  INST(Pextrd          , "pextrd"          , Enc(ExtExtract)        , O(000F3A,16,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 4 , 4 , 370, 1 , 65 ),
  INST(Pextrq          , "pextrq"          , Enc(ExtExtract)        , O(000F3A,16,_,_,1,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 8 , 8 , 402, 1 , 197),
  INST(Pextrw          , "pextrw"          , Enc(ExtPextrw)         , O(000F00,C5,_,_,_,_,_,_  ), O(000F3A,15,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 8 , 2 , 2 , 291, 2 , 198),
  INST(Pf2id           , "pf2id"           , Enc(Ext3dNow)          , O(000F0F,1D,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 4 , 4 , 295, 1 , 199),
  INST(Pf2iw           , "pf2iw"           , Enc(Ext3dNow)          , O(000F0F,1C,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 2 , 4 , 295, 1 , 200),
  INST(Pfacc           , "pfacc"           , Enc(Ext3dNow)          , O(000F0F,AE,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 1 , 201),
  INST(Pfadd           , "pfadd"           , Enc(Ext3dNow)          , O(000F0F,9E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 1 , 201),
  INST(Pfcmpeq         , "pfcmpeq"         , Enc(Ext3dNow)          , O(000F0F,B0,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 1 , 201),
  INST(Pfcmpge         , "pfcmpge"         , Enc(Ext3dNow)          , O(000F0F,90,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 1 , 201),
  INST(Pfcmpgt         , "pfcmpgt"         , Enc(Ext3dNow)          , O(000F0F,A0,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 1 , 201),
  INST(Pfmax           , "pfmax"           , Enc(Ext3dNow)          , O(000F0F,A4,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 1 , 201),
  INST(Pfmin           , "pfmin"           , Enc(Ext3dNow)          , O(000F0F,94,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 1 , 201),
  INST(Pfmul           , "pfmul"           , Enc(Ext3dNow)          , O(000F0F,B4,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 1 , 201),
  INST(Pfnacc          , "pfnacc"          , Enc(Ext3dNow)          , O(000F0F,8A,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 1 , 201),
  INST(Pfpnacc         , "pfpnacc"         , Enc(Ext3dNow)          , O(000F0F,8E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 1 , 201),
  INST(Pfrcp           , "pfrcp"           , Enc(Ext3dNow)          , O(000F0F,96,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 4 , 4 , 295, 1 , 199),
  INST(Pfrcpit1        , "pfrcpit1"        , Enc(Ext3dNow)          , O(000F0F,A6,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 1 , 201),
  INST(Pfrcpit2        , "pfrcpit2"        , Enc(Ext3dNow)          , O(000F0F,B6,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 1 , 201),
  INST(Pfrcpv          , "pfrcpv"          , Enc(Ext3dNow)          , O(000F0F,86,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 0 , 0 , 287, 1 , 202),
  INST(Pfrsqit1        , "pfrsqit1"        , Enc(Ext3dNow)          , O(000F0F,A7,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 4 , 4 , 295, 1 , 203),
  INST(Pfrsqrt         , "pfrsqrt"         , Enc(Ext3dNow)          , O(000F0F,97,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 4 , 4 , 295, 1 , 203),
  INST(Pfrsqrtv        , "pfrsqrtv"        , Enc(Ext3dNow)          , O(000F0F,87,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 0 , 0 , 287, 1 , 202),
  INST(Pfsub           , "pfsub"           , Enc(Ext3dNow)          , O(000F0F,9A,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 1 , 201),
  INST(Pfsubr          , "pfsubr"          , Enc(Ext3dNow)          , O(000F0F,AA,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 1 , 201),
  INST(Phaddd          , "phaddd"          , Enc(ExtRm_P)           , O(000F38,02,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 2 , 181),
  INST(Phaddsw         , "phaddsw"         , Enc(ExtRm_P)           , O(000F38,03,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Phaddw          , "phaddw"          , Enc(ExtRm_P)           , O(000F38,01,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Phminposuw      , "phminposuw"      , Enc(ExtRm)             , O(660F38,41,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 0 , 2 , 288, 1 , 204),
  INST(Phsubd          , "phsubd"          , Enc(ExtRm_P)           , O(000F38,06,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 2 , 181),
  INST(Phsubsw         , "phsubsw"         , Enc(ExtRm_P)           , O(000F38,07,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Phsubw          , "phsubw"          , Enc(ExtRm_P)           , O(000F38,05,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Pi2fd           , "pi2fd"           , Enc(Ext3dNow)          , O(000F0F,0D,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 4 , 4 , 295, 1 , 199),
  INST(Pi2fw           , "pi2fw"           , Enc(Ext3dNow)          , O(000F0F,0C,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 4 , 2 , 295, 1 , 205),
  INST(Pinsrb          , "pinsrb"          , Enc(ExtRmi)            , O(660F3A,20,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 1 , 403, 1 , 206),
  INST(Pinsrd          , "pinsrd"          , Enc(ExtRmi)            , O(660F3A,22,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 404, 1 , 14 ),
  INST(Pinsrq          , "pinsrq"          , Enc(ExtRmi)            , O(660F3A,22,_,_,1,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 405, 1 , 13 ),
  INST(Pinsrw          , "pinsrw"          , Enc(ExtRmi_P)          , O(000F00,C4,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 406, 1 , 207),
  INST(Pmaddubsw       , "pmaddubsw"       , Enc(ExtRm_P)           , O(000F38,04,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 1 , 287, 2 , 208),
  INST(Pmaddwd         , "pmaddwd"         , Enc(ExtRm_P)           , O(000F00,F5,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 2 , 287, 2 , 209),
  INST(Pmaxsb          , "pmaxsb"          , Enc(ExtRm)             , O(660F38,3C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 1 , 288, 1 , 210),
  INST(Pmaxsd          , "pmaxsd"          , Enc(ExtRm)             , O(660F38,3D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Pmaxsw          , "pmaxsw"          , Enc(ExtRm_P)           , O(000F00,EE,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Pmaxub          , "pmaxub"          , Enc(ExtRm_P)           , O(000F00,DE,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 1 , 287, 2 , 180),
  INST(Pmaxud          , "pmaxud"          , Enc(ExtRm)             , O(660F38,3F,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Pmaxuw          , "pmaxuw"          , Enc(ExtRm)             , O(660F38,3E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 288, 1 , 211),
  INST(Pminsb          , "pminsb"          , Enc(ExtRm)             , O(660F38,38,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 1 , 288, 1 , 210),
  INST(Pminsd          , "pminsd"          , Enc(ExtRm)             , O(660F38,39,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Pminsw          , "pminsw"          , Enc(ExtRm_P)           , O(000F00,EA,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Pminub          , "pminub"          , Enc(ExtRm_P)           , O(000F00,DA,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 1 , 287, 2 , 180),
  INST(Pminud          , "pminud"          , Enc(ExtRm)             , O(660F38,3B,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Pminuw          , "pminuw"          , Enc(ExtRm)             , O(660F38,3A,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 288, 1 , 211),
  INST(Pmovmskb        , "pmovmskb"        , Enc(ExtRm_P)           , O(000F00,D7,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 0 , 1 , 407, 1 , 212),
  INST(Pmovsxbd        , "pmovsxbd"        , Enc(ExtRm)             , O(660F38,21,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 4 , 1 , 227, 1 , 213),
  INST(Pmovsxbq        , "pmovsxbq"        , Enc(ExtRm)             , O(660F38,22,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8 , 1 , 230, 1 , 214),
  INST(Pmovsxbw        , "pmovsxbw"        , Enc(ExtRm)             , O(660F38,20,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 2 , 1 , 61 , 1 , 215),
  INST(Pmovsxdq        , "pmovsxdq"        , Enc(ExtRm)             , O(660F38,25,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8 , 4 , 61 , 1 , 49 ),
  INST(Pmovsxwd        , "pmovsxwd"        , Enc(ExtRm)             , O(660F38,23,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 4 , 2 , 61 , 1 , 216),
  INST(Pmovsxwq        , "pmovsxwq"        , Enc(ExtRm)             , O(660F38,24,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8 , 2 , 227, 1 , 217),
  INST(Pmovzxbd        , "pmovzxbd"        , Enc(ExtRm)             , O(660F38,31,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 4 , 1 , 227, 1 , 213),
  INST(Pmovzxbq        , "pmovzxbq"        , Enc(ExtRm)             , O(660F38,32,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8 , 1 , 230, 1 , 214),
  INST(Pmovzxbw        , "pmovzxbw"        , Enc(ExtRm)             , O(660F38,30,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 2 , 1 , 61 , 1 , 215),
  INST(Pmovzxdq        , "pmovzxdq"        , Enc(ExtRm)             , O(660F38,35,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8 , 4 , 61 , 1 , 49 ),
  INST(Pmovzxwd        , "pmovzxwd"        , Enc(ExtRm)             , O(660F38,33,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 4 , 2 , 61 , 1 , 216),
  INST(Pmovzxwq        , "pmovzxwq"        , Enc(ExtRm)             , O(660F38,34,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8 , 2 , 227, 1 , 217),
  INST(Pmuldq          , "pmuldq"          , Enc(ExtRm)             , O(660F38,28,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 4 , 288, 1 , 218),
  INST(Pmulhrsw        , "pmulhrsw"        , Enc(ExtRm_P)           , O(000F38,0B,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Pmulhrw         , "pmulhrw"         , Enc(Ext3dNow)          , O(000F0F,B7,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 1 , 219),
  INST(Pmulhuw         , "pmulhuw"         , Enc(ExtRm_P)           , O(000F00,E4,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Pmulhw          , "pmulhw"          , Enc(ExtRm_P)           , O(000F00,E5,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Pmulld          , "pmulld"          , Enc(ExtRm)             , O(660F38,40,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Pmullw          , "pmullw"          , Enc(ExtRm_P)           , O(000F00,D5,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Pmuludq         , "pmuludq"         , Enc(ExtRm_P)           , O(000F00,F4,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 4 , 287, 2 , 220),
  INST(Pop             , "pop"             , Enc(X86Pop)            , O(000000,8F,0,_,_,_,_,_  ), O(000000,58,_,_,_,_,_,_  ), F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 293, 2 , 221),
  INST(Popa            , "popa"            , Enc(X86Op)             , O(000000,61,_,_,_,_,_,_  ), 0                         , F(Volatile)|F(Special)                , EF(________), 0 , 0 , 0 , 0 , 408, 1 , 130),
  INST(Popcnt          , "popcnt"          , Enc(X86Rm)             , O(F30F00,B8,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWWWW__), 0 , 0 , 0 , 0 , 152, 3 , 222),
  INST(Popf            , "popf"            , Enc(X86Op)             , O(000000,9D,_,_,_,_,_,_  ), 0                         , F(Volatile)|F(Special)                , EF(WWWWWWWW), 0 , 0 , 0 , 0 , 259, 1 , 223),
  INST(Por             , "por"             , Enc(ExtRm_P)           , O(000F00,EB,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 16, 16, 287, 2 , 187),
  INST(Prefetch        , "prefetch"        , Enc(X86Prefetch)       , O(000F00,18,_,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , 0 , 0 , 0  , 0 , 224),
  INST(Prefetch3dNow   , "prefetch3dnow"   , Enc(X86M_Only)         , O(000F00,0D,0,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , 0 , 0 , 352, 1 , 30 ),
  INST(Prefetchw       , "prefetchw"       , Enc(X86M_Only)         , O(000F00,0D,1,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(UUUUUU__), 0 , 0 , 0 , 0 , 352, 1 , 225),
  INST(Prefetchwt1     , "prefetchwt1"     , Enc(X86M_Only)         , O(000F00,0D,2,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(UUUUUU__), 0 , 0 , 0 , 0 , 352, 1 , 225),
  INST(Psadbw          , "psadbw"          , Enc(ExtRm_P)           , O(000F00,F6,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 1 , 287, 2 , 208),
  INST(Pshufb          , "pshufb"          , Enc(ExtRm_P)           , O(000F38,00,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 1 , 295, 2 , 180),
  INST(Pshufd          , "pshufd"          , Enc(ExtRmi)            , O(660F00,70,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 4 , 4 , 70 , 1 , 226),
  INST(Pshufhw         , "pshufhw"         , Enc(ExtRmi)            , O(F30F00,70,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 2 , 2 , 70 , 1 , 227),
  INST(Pshuflw         , "pshuflw"         , Enc(ExtRmi)            , O(F20F00,70,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 2 , 2 , 70 , 1 , 227),
  INST(Pshufw          , "pshufw"          , Enc(ExtRmi_P)          , O(000F00,70,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 2 , 2 , 409, 1 , 228),
  INST(Psignb          , "psignb"          , Enc(ExtRm_P)           , O(000F38,08,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 1 , 287, 2 , 180),
  INST(Psignd          , "psignd"          , Enc(ExtRm_P)           , O(000F38,0A,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 2 , 181),
  INST(Psignw          , "psignw"          , Enc(ExtRm_P)           , O(000F38,09,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Pslld           , "pslld"           , Enc(ExtRmRi_P)         , O(000F00,F2,_,_,_,_,_,_  ), O(000F00,72,6,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 297, 2 , 229),
  INST(Pslldq          , "pslldq"          , Enc(ExtRmRi)           , 0                         , O(660F00,73,7,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 16, 16, 410, 1 , 230),
  INST(Psllq           , "psllq"           , Enc(ExtRmRi_P)         , O(000F00,F3,_,_,_,_,_,_  ), O(000F00,73,6,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 297, 2 , 231),
  INST(Psllw           , "psllw"           , Enc(ExtRmRi_P)         , O(000F00,F1,_,_,_,_,_,_  ), O(000F00,71,6,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 297, 2 , 232),
  INST(Psrad           , "psrad"           , Enc(ExtRmRi_P)         , O(000F00,E2,_,_,_,_,_,_  ), O(000F00,72,4,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 297, 2 , 233),
  INST(Psraw           , "psraw"           , Enc(ExtRmRi_P)         , O(000F00,E1,_,_,_,_,_,_  ), O(000F00,71,4,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 297, 2 , 234),
  INST(Psrld           , "psrld"           , Enc(ExtRmRi_P)         , O(000F00,D2,_,_,_,_,_,_  ), O(000F00,72,2,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 297, 2 , 235),
  INST(Psrldq          , "psrldq"          , Enc(ExtRmRi)           , 0                         , O(660F00,73,3,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 16, 16, 410, 1 , 236),
  INST(Psrlq           , "psrlq"           , Enc(ExtRmRi_P)         , O(000F00,D3,_,_,_,_,_,_  ), O(000F00,73,2,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 297, 2 , 237),
  INST(Psrlw           , "psrlw"           , Enc(ExtRmRi_P)         , O(000F00,D1,_,_,_,_,_,_  ), O(000F00,71,2,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 297, 2 , 238),
  INST(Psubb           , "psubb"           , Enc(ExtRm_P)           , O(000F00,F8,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 1 , 287, 2 , 180),
  INST(Psubd           , "psubd"           , Enc(ExtRm_P)           , O(000F00,FA,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 287, 2 , 181),
  INST(Psubq           , "psubq"           , Enc(ExtRm_P)           , O(000F00,FB,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 287, 2 , 239),
  INST(Psubsb          , "psubsb"          , Enc(ExtRm_P)           , O(000F00,E8,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 1 , 287, 2 , 180),
  INST(Psubsw          , "psubsw"          , Enc(ExtRm_P)           , O(000F00,E9,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Psubusb         , "psubusb"         , Enc(ExtRm_P)           , O(000F00,D8,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1 , 1 , 287, 2 , 180),
  INST(Psubusw         , "psubusw"         , Enc(ExtRm_P)           , O(000F00,D9,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Psubw           , "psubw"           , Enc(ExtRm_P)           , O(000F00,F9,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 2 , 287, 2 , 182),
  INST(Pswapd          , "pswapd"          , Enc(Ext3dNow)          , O(000F0F,BB,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 4 , 4 , 295, 1 , 199),
  INST(Ptest           , "ptest"           , Enc(ExtRm)             , O(660F38,17,_,_,_,_,_,_  ), 0                         , F(RO)                                 , EF(WWWWWW__), 0 , 0 , 0 , 0 , 341, 1 , 240),
  INST(Punpckhbw       , "punpckhbw"       , Enc(ExtRm_P)           , O(000F00,68,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 1 , 287, 2 , 208),
  INST(Punpckhdq       , "punpckhdq"       , Enc(ExtRm_P)           , O(000F00,6A,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 4 , 287, 2 , 220),
  INST(Punpckhqdq      , "punpckhqdq"      , Enc(ExtRm)             , O(660F00,6D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 16, 8 , 288, 1 , 241),
  INST(Punpckhwd       , "punpckhwd"       , Enc(ExtRm_P)           , O(000F00,69,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 2 , 287, 2 , 209),
  INST(Punpcklbw       , "punpcklbw"       , Enc(ExtRm_P)           , O(000F00,60,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2 , 1 , 287, 2 , 208),
  INST(Punpckldq       , "punpckldq"       , Enc(ExtRm_P)           , O(000F00,62,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 4 , 287, 2 , 220),
  INST(Punpcklqdq      , "punpcklqdq"      , Enc(ExtRm)             , O(660F00,6C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 16, 8 , 288, 1 , 241),
  INST(Punpcklwd       , "punpcklwd"       , Enc(ExtRm_P)           , O(000F00,61,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 2 , 287, 2 , 209),
  INST(Push            , "push"            , Enc(X86Push)           , O(000000,FF,6,_,_,_,_,_  ), O(000000,50,_,_,_,_,_,_  ), F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 299, 2 , 242),
  INST(Pusha           , "pusha"           , Enc(X86Op)             , O(000000,60,_,_,_,_,_,_  ), 0                         , F(Volatile)|F(Special)                , EF(________), 0 , 0 , 0 , 0 , 408, 1 , 130),
  INST(Pushf           , "pushf"           , Enc(X86Op)             , O(000000,9C,_,_,_,_,_,_  ), 0                         , F(Volatile)|F(Special)                , EF(RRRRRRRR), 0 , 0 , 0 , 0 , 259, 1 , 243),
  INST(Pxor            , "pxor"            , Enc(ExtRm_P)           , O(000F00,EF,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 16, 16, 287, 2 , 187),
  INST(Rcl             , "rcl"             , Enc(X86Rot)            , O(000000,D0,2,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(W____X__), 0 , 0 , 0 , 0 , 411, 1 , 244),
  INST(Rcpps           , "rcpps"           , Enc(ExtRm)             , O(000F00,53,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 4 , 4 , 63 , 1 , 50 ),
  INST(Rcpss           , "rcpss"           , Enc(ExtRm)             , O(F30F00,53,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 4 , 4 , 4 , 227, 1 , 245),
  INST(Rcr             , "rcr"             , Enc(X86Rot)            , O(000000,D0,3,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(W____X__), 0 , 0 , 0 , 0 , 411, 1 , 244),
  INST(Rdfsbase        , "rdfsbase"        , Enc(X86M)              , O(F30F00,AE,0,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 0 , 0 , 412, 1 , 246),
  INST(Rdgsbase        , "rdgsbase"        , Enc(X86M)              , O(F30F00,AE,1,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 0 , 0 , 412, 1 , 246),
  INST(Rdrand          , "rdrand"          , Enc(X86M)              , O(000F00,C7,6,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWWWW__), 0 , 8 , 0 , 0 , 413, 1 , 247),
  INST(Rdseed          , "rdseed"          , Enc(X86M)              , O(000F00,C7,7,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWWWW__), 0 , 8 , 0 , 0 , 413, 1 , 247),
  INST(Rdtsc           , "rdtsc"           , Enc(X86Op)             , O(000F00,31,_,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 414, 1 , 31 ),
  INST(Rdtscp          , "rdtscp"          , Enc(X86Op)             , O(000F01,F9,_,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 415, 1 , 31 ),
  INST(RepLodsB        , "rep lods_b"      , Enc(X86Rep)            , O(000000,AC,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 0 , 0 , 0  , 0 , 248),
  INST(RepLodsD        , "rep lods_d"      , Enc(X86Rep)            , O(000000,AD,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 0 , 0 , 0  , 0 , 248),
  INST(RepLodsQ        , "rep lods_q"      , Enc(X86Rep)            , O(000000,AD,1,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 0 , 0 , 0  , 0 , 248),
  INST(RepLodsW        , "rep lods_w"      , Enc(X86Rep)            , O(660000,AD,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 0 , 0 , 0  , 0 , 248),
  INST(RepMovsB        , "rep movs_b"      , Enc(X86Rep)            , O(000000,A4,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 0 , 0 , 0  , 0 , 248),
  INST(RepMovsD        , "rep movs_d"      , Enc(X86Rep)            , O(000000,A5,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 0 , 0 , 0  , 0 , 248),
  INST(RepMovsQ        , "rep movs_q"      , Enc(X86Rep)            , O(000000,A5,1,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 0 , 0 , 0  , 0 , 248),
  INST(RepMovsW        , "rep movs_w"      , Enc(X86Rep)            , O(660000,A5,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 0 , 0 , 0  , 0 , 248),
  INST(RepStosB        , "rep stos_b"      , Enc(X86Rep)            , O(000000,AA,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 0 , 0 , 0  , 0 , 248),
  INST(RepStosD        , "rep stos_d"      , Enc(X86Rep)            , O(000000,AB,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 0 , 0 , 0  , 0 , 248),
  INST(RepStosQ        , "rep stos_q"      , Enc(X86Rep)            , O(000000,AB,1,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 0 , 0 , 0  , 0 , 248),
  INST(RepStosW        , "rep stos_w"      , Enc(X86Rep)            , O(660000,AB,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 0 , 0 , 0  , 0 , 248),
  INST(RepeCmpsB       , "repe cmps_b"     , Enc(X86Rep)            , O(000000,A6,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 249),
  INST(RepeCmpsD       , "repe cmps_d"     , Enc(X86Rep)            , O(000000,A7,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 249),
  INST(RepeCmpsQ       , "repe cmps_q"     , Enc(X86Rep)            , O(000000,A7,1,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 249),
  INST(RepeCmpsW       , "repe cmps_w"     , Enc(X86Rep)            , O(660000,A7,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 249),
  INST(RepeScasB       , "repe scas_b"     , Enc(X86Rep)            , O(000000,AE,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 249),
  INST(RepeScasD       , "repe scas_d"     , Enc(X86Rep)            , O(000000,AF,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 249),
  INST(RepeScasQ       , "repe scas_q"     , Enc(X86Rep)            , O(000000,AF,1,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 249),
  INST(RepeScasW       , "repe scas_w"     , Enc(X86Rep)            , O(660000,AF,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 249),
  INST(RepneCmpsB      , "repne cmps_b"    , Enc(X86Rep)            , O(000000,A6,0,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 249),
  INST(RepneCmpsD      , "repne cmps_d"    , Enc(X86Rep)            , O(000000,A7,0,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 249),
  INST(RepneCmpsQ      , "repne cmps_q"    , Enc(X86Rep)            , O(000000,A7,0,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 249),
  INST(RepneCmpsW      , "repne cmps_w"    , Enc(X86Rep)            , O(660000,A7,0,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 249),
  INST(RepneScasB      , "repne scas_b"    , Enc(X86Rep)            , O(000000,AE,0,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 249),
  INST(RepneScasD      , "repne scas_d"    , Enc(X86Rep)            , O(000000,AF,0,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 249),
  INST(RepneScasQ      , "repne scas_q"    , Enc(X86Rep)            , O(000000,AF,0,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 249),
  INST(RepneScasW      , "repne scas_w"    , Enc(X86Rep)            , O(660000,AF,0,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 249),
  INST(Ret             , "ret"             , Enc(X86Ret)            , O(000000,C2,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 301, 2 , 250),
  INST(Rol             , "rol"             , Enc(X86Rot)            , O(000000,D0,0,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(W____W__), 0 , 0 , 0 , 0 , 411, 1 , 251),
  INST(Ror             , "ror"             , Enc(X86Rot)            , O(000000,D0,1,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(W____W__), 0 , 0 , 0 , 0 , 411, 1 , 251),
  INST(Rorx            , "rorx"            , Enc(VexRmi_Wx)         , V(F20F3A,F0,_,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 0 , 0 , 303, 2 , 252),
  INST(Roundpd         , "roundpd"         , Enc(ExtRmi)            , O(660F3A,09,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8 , 8 , 70 , 1 , 253),
  INST(Roundps         , "roundps"         , Enc(ExtRmi)            , O(660F3A,08,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 4 , 4 , 70 , 1 , 226),
  INST(Roundsd         , "roundsd"         , Enc(ExtRmi)            , O(660F3A,0B,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 8 , 8 , 416, 1 , 254),
  INST(Roundss         , "roundss"         , Enc(ExtRmi)            , O(660F3A,0A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 4 , 4 , 4 , 417, 1 , 255),
  INST(Rsqrtps         , "rsqrtps"         , Enc(ExtRm)             , O(000F00,52,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 4 , 4 , 63 , 1 , 50 ),
  INST(Rsqrtss         , "rsqrtss"         , Enc(ExtRm)             , O(F30F00,52,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 4 , 4 , 4 , 227, 1 , 245),
  INST(Sahf            , "sahf"            , Enc(X86Op)             , O(000000,9E,_,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(_WWWWW__), 0 , 0 , 0 , 0 , 418, 1 , 256),
  INST(Sal             , "sal"             , Enc(X86Rot)            , O(000000,D0,4,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(WWWUWW__), 0 , 0 , 0 , 0 , 411, 1 , 257),
  INST(Sar             , "sar"             , Enc(X86Rot)            , O(000000,D0,7,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(WWWUWW__), 0 , 0 , 0 , 0 , 411, 1 , 257),
  INST(Sarx            , "sarx"            , Enc(VexRmv_Wx)         , V(F30F38,F7,_,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 0 , 0 , 247, 2 , 258),
  INST(Sbb             , "sbb"             , Enc(X86Arith)          , O(000000,18,3,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWWWX__), 0 , 0 , 0 , 0 , 13 , 10, 1  ),
  INST(ScasB           , "scas_b"          , Enc(X86Op)             , O(000000,AE,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 42 ),
  INST(ScasD           , "scas_d"          , Enc(X86Op)             , O(000000,AF,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 42 ),
  INST(ScasQ           , "scas_q"          , Enc(X86Op)             , O(000000,AF,_,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 42 ),
  INST(ScasW           , "scas_w"          , Enc(X86Op)             , O(660000,AF,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 0 , 0 , 0  , 0 , 42 ),
  INST(Seta            , "seta"            , Enc(X86Set)            , O(000F00,97,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R__R__), 0 , 1 , 0 , 0 , 419, 1 , 259),
  INST(Setae           , "setae"           , Enc(X86Set)            , O(000F00,93,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_____R__), 0 , 1 , 0 , 0 , 419, 1 , 260),
  INST(Setb            , "setb"            , Enc(X86Set)            , O(000F00,92,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_____R__), 0 , 1 , 0 , 0 , 419, 1 , 260),
  INST(Setbe           , "setbe"           , Enc(X86Set)            , O(000F00,96,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R__R__), 0 , 1 , 0 , 0 , 419, 1 , 259),
  INST(Setc            , "setc"            , Enc(X86Set)            , O(000F00,92,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_____R__), 0 , 1 , 0 , 0 , 419, 1 , 260),
  INST(Sete            , "sete"            , Enc(X86Set)            , O(000F00,94,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R_____), 0 , 1 , 0 , 0 , 419, 1 , 261),
  INST(Setg            , "setg"            , Enc(X86Set)            , O(000F00,9F,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RRR_____), 0 , 1 , 0 , 0 , 419, 1 , 262),
  INST(Setge           , "setge"           , Enc(X86Set)            , O(000F00,9D,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RR______), 0 , 1 , 0 , 0 , 419, 1 , 263),
  INST(Setl            , "setl"            , Enc(X86Set)            , O(000F00,9C,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RR______), 0 , 1 , 0 , 0 , 419, 1 , 263),
  INST(Setle           , "setle"           , Enc(X86Set)            , O(000F00,9E,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RRR_____), 0 , 1 , 0 , 0 , 419, 1 , 262),
  INST(Setna           , "setna"           , Enc(X86Set)            , O(000F00,96,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R__R__), 0 , 1 , 0 , 0 , 419, 1 , 259),
  INST(Setnae          , "setnae"          , Enc(X86Set)            , O(000F00,92,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_____R__), 0 , 1 , 0 , 0 , 419, 1 , 260),
  INST(Setnb           , "setnb"           , Enc(X86Set)            , O(000F00,93,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_____R__), 0 , 1 , 0 , 0 , 419, 1 , 260),
  INST(Setnbe          , "setnbe"          , Enc(X86Set)            , O(000F00,97,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R__R__), 0 , 1 , 0 , 0 , 419, 1 , 259),
  INST(Setnc           , "setnc"           , Enc(X86Set)            , O(000F00,93,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_____R__), 0 , 1 , 0 , 0 , 419, 1 , 260),
  INST(Setne           , "setne"           , Enc(X86Set)            , O(000F00,95,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R_____), 0 , 1 , 0 , 0 , 419, 1 , 261),
  INST(Setng           , "setng"           , Enc(X86Set)            , O(000F00,9E,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RRR_____), 0 , 1 , 0 , 0 , 419, 1 , 262),
  INST(Setnge          , "setnge"          , Enc(X86Set)            , O(000F00,9C,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RR______), 0 , 1 , 0 , 0 , 419, 1 , 263),
  INST(Setnl           , "setnl"           , Enc(X86Set)            , O(000F00,9D,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RR______), 0 , 1 , 0 , 0 , 419, 1 , 263),
  INST(Setnle          , "setnle"          , Enc(X86Set)            , O(000F00,9F,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RRR_____), 0 , 1 , 0 , 0 , 419, 1 , 262),
  INST(Setno           , "setno"           , Enc(X86Set)            , O(000F00,91,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(R_______), 0 , 1 , 0 , 0 , 419, 1 , 264),
  INST(Setnp           , "setnp"           , Enc(X86Set)            , O(000F00,9B,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(____R___), 0 , 1 , 0 , 0 , 419, 1 , 265),
  INST(Setns           , "setns"           , Enc(X86Set)            , O(000F00,99,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_R______), 0 , 1 , 0 , 0 , 419, 1 , 266),
  INST(Setnz           , "setnz"           , Enc(X86Set)            , O(000F00,95,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R_____), 0 , 1 , 0 , 0 , 419, 1 , 261),
  INST(Seto            , "seto"            , Enc(X86Set)            , O(000F00,90,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(R_______), 0 , 1 , 0 , 0 , 419, 1 , 264),
  INST(Setp            , "setp"            , Enc(X86Set)            , O(000F00,9A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(____R___), 0 , 1 , 0 , 0 , 419, 1 , 265),
  INST(Setpe           , "setpe"           , Enc(X86Set)            , O(000F00,9A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(____R___), 0 , 1 , 0 , 0 , 419, 1 , 265),
  INST(Setpo           , "setpo"           , Enc(X86Set)            , O(000F00,9B,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(____R___), 0 , 1 , 0 , 0 , 419, 1 , 265),
  INST(Sets            , "sets"            , Enc(X86Set)            , O(000F00,98,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_R______), 0 , 1 , 0 , 0 , 419, 1 , 266),
  INST(Setz            , "setz"            , Enc(X86Set)            , O(000F00,94,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R_____), 0 , 1 , 0 , 0 , 419, 1 , 261),
  INST(Sfence          , "sfence"          , Enc(X86Fence)          , O(000F00,AE,7,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 131),
  INST(Sha1msg1        , "sha1msg1"        , Enc(ExtRm)             , O(000F38,C9,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 0 , 0 , 288, 1 , 7  ),
  INST(Sha1msg2        , "sha1msg2"        , Enc(ExtRm)             , O(000F38,CA,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 0 , 0 , 288, 1 , 7  ),
  INST(Sha1nexte       , "sha1nexte"       , Enc(ExtRm)             , O(000F38,C8,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 0 , 0 , 288, 1 , 7  ),
  INST(Sha1rnds4       , "sha1rnds4"       , Enc(ExtRmi)            , O(000F3A,CC,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 0 , 0 , 290, 1 , 267),
  INST(Sha256msg1      , "sha256msg1"      , Enc(ExtRm)             , O(000F38,CC,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 0 , 0 , 288, 1 , 7  ),
  INST(Sha256msg2      , "sha256msg2"      , Enc(ExtRm)             , O(000F38,CD,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 0 , 0 , 288, 1 , 7  ),
  INST(Sha256rnds2     , "sha256rnds2"     , Enc(ExtRmXMM0)         , O(000F38,CB,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 0 , 0 , 347, 1 , 268),
  INST(Shl             , "shl"             , Enc(X86Rot)            , O(000000,D0,4,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(WWWUWW__), 0 , 0 , 0 , 0 , 411, 1 , 257),
  INST(Shld            , "shld"            , Enc(X86ShldShrd)       , O(000F00,A4,_,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(UWWUWW__), 0 , 0 , 0 , 0 , 158, 3 , 269),
  INST(Shlx            , "shlx"            , Enc(VexRmv_Wx)         , V(660F38,F7,_,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 0 , 0 , 247, 2 , 258),
  INST(Shr             , "shr"             , Enc(X86Rot)            , O(000000,D0,5,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(WWWUWW__), 0 , 0 , 0 , 0 , 411, 1 , 257),
  INST(Shrd            , "shrd"            , Enc(X86ShldShrd)       , O(000F00,AC,_,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(UWWUWW__), 0 , 0 , 0 , 0 , 158, 3 , 269),
  INST(Shrx            , "shrx"            , Enc(VexRmv_Wx)         , V(F20F38,F7,_,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 0 , 0 , 247, 2 , 258),
  INST(Shufpd          , "shufpd"          , Enc(ExtRmi)            , O(660F00,C6,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 290, 1 , 13 ),
  INST(Shufps          , "shufps"          , Enc(ExtRmi)            , O(000F00,C6,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 290, 1 , 14 ),
  INST(Sqrtpd          , "sqrtpd"          , Enc(ExtRm)             , O(660F00,51,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8 , 8 , 63 , 1 , 166),
  INST(Sqrtps          , "sqrtps"          , Enc(ExtRm)             , O(000F00,51,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 4 , 4 , 63 , 1 , 50 ),
  INST(Sqrtsd          , "sqrtsd"          , Enc(ExtRm)             , O(F20F00,51,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 8 , 8 , 61 , 1 , 270),
  INST(Sqrtss          , "sqrtss"          , Enc(ExtRm)             , O(F30F00,51,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 4 , 4 , 4 , 227, 1 , 245),
  INST(Stac            , "stac"            , Enc(X86Op)             , O(000F01,CB,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(___W____), 0 , 0 , 0 , 0 , 259, 1 , 27 ),
  INST(Stc             , "stc"             , Enc(X86Op)             , O(000000,F9,_,_,_,_,_,_  ), 0                         , 0                                     , EF(_____W__), 0 , 0 , 0 , 0 , 259, 1 , 271),
  INST(Std             , "std"             , Enc(X86Op)             , O(000000,FD,_,_,_,_,_,_  ), 0                         , 0                                     , EF(______W_), 0 , 0 , 0 , 0 , 259, 1 , 272),
  INST(Sti             , "sti"             , Enc(X86Op)             , O(000000,FB,_,_,_,_,_,_  ), 0                         , 0                                     , EF(_______W), 0 , 0 , 0 , 0 , 259, 1 , 273),
  INST(Stmxcsr         , "stmxcsr"         , Enc(X86M_Only)         , O(000F00,AE,3,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(________), 0 , 0 , 0 , 0 , 420, 1 , 274),
  INST(StosB           , "stos_b"          , Enc(X86Op)             , O(000000,AA,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 0 , 0 , 0  , 0 , 275),
  INST(StosD           , "stos_d"          , Enc(X86Op)             , O(000000,AB,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 0 , 0 , 0  , 0 , 275),
  INST(StosQ           , "stos_q"          , Enc(X86Op)             , O(000000,AB,_,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 0 , 0 , 0  , 0 , 275),
  INST(StosW           , "stos_w"          , Enc(X86Op)             , O(660000,AB,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 0 , 0 , 0  , 0 , 275),
  INST(Sub             , "sub"             , Enc(X86Arith)          , O(000000,28,5,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWWWW__), 0 , 0 , 0 , 0 , 13 , 10, 3  ),
  INST(Subpd           , "subpd"           , Enc(ExtRm)             , O(660F00,5C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 288, 1 , 4  ),
  INST(Subps           , "subps"           , Enc(ExtRm)             , O(000F00,5C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Subsd           , "subsd"           , Enc(ExtRm)             , O(F20F00,5C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 345, 1 , 4  ),
  INST(Subss           , "subss"           , Enc(ExtRm)             , O(F30F00,5C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 346, 1 , 5  ),
  INST(T1mskc          , "t1mskc"          , Enc(VexVm_Wx)          , V(XOP_M9,01,7,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , 0 , 0 , 153, 2 , 12 ),
  INST(Test            , "test"            , Enc(X86Test)           , O(000000,84,_,_,x,_,_,_  ), O(000000,F6,_,_,x,_,_,_  ), F(RO)                                 , EF(WWWUWW__), 0 , 0 , 0 , 0 , 87 , 5 , 276),
  INST(Tzcnt           , "tzcnt"           , Enc(X86Rm)             , O(F30F00,BC,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(UUWUUW__), 0 , 0 , 0 , 0 , 152, 3 , 222),
  INST(Tzmsk           , "tzmsk"           , Enc(VexVm_Wx)          , V(XOP_M9,01,4,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , 0 , 0 , 153, 2 , 12 ),
  INST(Ucomisd         , "ucomisd"         , Enc(ExtRm)             , O(660F00,2E,_,_,_,_,_,_  ), 0                         , F(RO)                                 , EF(WWWWWW__), 0 , 0 , 8 , 8 , 357, 1 , 45 ),
  INST(Ucomiss         , "ucomiss"         , Enc(ExtRm)             , O(000F00,2E,_,_,_,_,_,_  ), 0                         , F(RO)                                 , EF(WWWWWW__), 0 , 0 , 4 , 4 , 358, 1 , 46 ),
  INST(Ud2             , "ud2"             , Enc(X86Op)             , O(000F00,0B,_,_,_,_,_,_  ), 0                         , 0                                     , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 178),
  INST(Unpckhpd        , "unpckhpd"        , Enc(ExtRm)             , O(660F00,15,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 288, 1 , 4  ),
  INST(Unpckhps        , "unpckhps"        , Enc(ExtRm)             , O(000F00,15,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Unpcklpd        , "unpcklpd"        , Enc(ExtRm)             , O(660F00,14,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 288, 1 , 4  ),
  INST(Unpcklps        , "unpcklps"        , Enc(ExtRm)             , O(000F00,14,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Vaddpd          , "vaddpd"          , Enc(VexRvm_Lx)         , V(660F00,58,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 277),
  INST(Vaddps          , "vaddps"          , Enc(VexRvm_Lx)         , V(000F00,58,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 278),
  INST(Vaddsd          , "vaddsd"          , Enc(VexRvm)            , V(F20F00,58,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 421, 1 , 279),
  INST(Vaddss          , "vaddss"          , Enc(VexRvm)            , V(F30F00,58,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 422, 1 , 280),
  INST(Vaddsubpd       , "vaddsubpd"       , Enc(VexRvm_Lx)         , V(660F00,D0,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 161, 2 , 281),
  INST(Vaddsubps       , "vaddsubps"       , Enc(VexRvm_Lx)         , V(F20F00,D0,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 161, 2 , 282),
  INST(Vaesdec         , "vaesdec"         , Enc(VexRvm)            , V(660F38,DE,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 0 , 0 , 69 , 1 , 283),
  INST(Vaesdeclast     , "vaesdeclast"     , Enc(VexRvm)            , V(660F38,DF,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 0 , 0 , 69 , 1 , 283),
  INST(Vaesenc         , "vaesenc"         , Enc(VexRvm)            , V(660F38,DC,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 0 , 0 , 69 , 1 , 283),
  INST(Vaesenclast     , "vaesenclast"     , Enc(VexRvm)            , V(660F38,DD,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 0 , 0 , 69 , 1 , 283),
  INST(Vaesimc         , "vaesimc"         , Enc(VexRm)             , V(660F38,DB,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 0 , 0 , 63 , 1 , 284),
  INST(Vaeskeygenassist, "vaeskeygenassist", Enc(VexRmi)            , V(660F3A,DF,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 0 , 0 , 70 , 1 , 285),
  INST(Valignd         , "valignd"         , Enc(VexRvmi_Lx)        , V(660F3A,03,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 0 , 0 , 164, 3 , 286),
  INST(Valignq         , "valignq"         , Enc(VexRvmi_Lx)        , V(660F3A,03,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 0 , 0 , 164, 3 , 286),
  INST(Vandnpd         , "vandnpd"         , Enc(VexRvm_Lx)         , V(660F00,55,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 287),
  INST(Vandnps         , "vandnps"         , Enc(VexRvm_Lx)         , V(000F00,55,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 288),
  INST(Vandpd          , "vandpd"          , Enc(VexRvm_Lx)         , V(660F00,54,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 287),
  INST(Vandps          , "vandps"          , Enc(VexRvm_Lx)         , V(000F00,54,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 288),
  INST(Vblendmb        , "vblendmb"        , Enc(VexRvm_Lx)         , V(660F38,66,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 161, 3 , 289),
  INST(Vblendmd        , "vblendmd"        , Enc(VexRvm_Lx)         , V(660F38,64,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 290),
  INST(Vblendmpd       , "vblendmpd"       , Enc(VexRvm_Lx)         , V(660F38,65,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 291),
  INST(Vblendmps       , "vblendmps"       , Enc(VexRvm_Lx)         , V(660F38,65,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 290),
  INST(Vblendmq        , "vblendmq"        , Enc(VexRvm_Lx)         , V(660F38,64,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 291),
  INST(Vblendmw        , "vblendmw"        , Enc(VexRvm_Lx)         , V(660F38,66,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 292),
  INST(Vblendpd        , "vblendpd"        , Enc(VexRvmi_Lx)        , V(660F3A,0D,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 164, 2 , 293),
  INST(Vblendps        , "vblendps"        , Enc(VexRvmi_Lx)        , V(660F3A,0C,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 164, 2 , 294),
  INST(Vblendvpd       , "vblendvpd"       , Enc(VexRvmr_Lx)        , V(660F3A,4B,_,x,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 305, 2 , 295),
  INST(Vblendvps       , "vblendvps"       , Enc(VexRvmr_Lx)        , V(660F3A,4A,_,x,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 305, 2 , 296),
  INST(Vbroadcastf128  , "vbroadcastf128"  , Enc(VexRm)             , V(660F38,1A,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 16, 16, 423, 1 , 297),
  INST(Vbroadcastf32x2 , "vbroadcastf32x2" , Enc(VexRm_Lx)          , V(660F38,19,_,x,_,0,3,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 424, 1 , 298),
  INST(Vbroadcastf32x4 , "vbroadcastf32x4" , Enc(VexRm_Lx)          , V(660F38,1A,_,x,_,0,4,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 16, 16, 425, 1 , 299),
  INST(Vbroadcastf32x8 , "vbroadcastf32x8" , Enc(VexRm)             , V(660F38,1B,_,2,_,0,5,T8 ), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 32, 32, 426, 1 , 300),
  INST(Vbroadcastf64x2 , "vbroadcastf64x2" , Enc(VexRm_Lx)          , V(660F38,1A,_,x,_,1,4,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 16, 16, 425, 1 , 301),
  INST(Vbroadcastf64x4 , "vbroadcastf64x4" , Enc(VexRm)             , V(660F38,1B,_,2,_,1,5,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 32, 32, 426, 1 , 302),
  INST(Vbroadcasti128  , "vbroadcasti128"  , Enc(VexRm)             , V(660F38,5A,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 16, 16, 423, 1 , 297),
  INST(Vbroadcasti32x2 , "vbroadcasti32x2" , Enc(VexRm_Lx)          , V(660F38,59,_,x,_,0,3,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 427, 1 , 298),
  INST(Vbroadcasti32x4 , "vbroadcasti32x4" , Enc(VexRm_Lx)          , V(660F38,5A,_,x,_,0,4,T4 ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 16, 16, 424, 1 , 303),
  INST(Vbroadcasti32x8 , "vbroadcasti32x8" , Enc(VexRm)             , V(660F38,5B,_,2,_,0,5,T8 ), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 32, 32, 232, 1 , 300),
  INST(Vbroadcasti64x2 , "vbroadcasti64x2" , Enc(VexRm_Lx)          , V(660F38,5A,_,x,_,1,4,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 16, 16, 424, 1 , 301),
  INST(Vbroadcasti64x4 , "vbroadcasti64x4" , Enc(VexRm)             , V(660F38,5B,_,2,_,1,5,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 32, 32, 232, 1 , 302),
  INST(Vbroadcastsd    , "vbroadcastsd"    , Enc(VexRm_Lx)          , V(660F38,19,_,x,0,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 424, 1 , 304),
  INST(Vbroadcastss    , "vbroadcastss"    , Enc(VexRm_Lx)          , V(660F38,18,_,x,0,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 325, 1 , 305),
  INST(Vcmppd          , "vcmppd"          , Enc(VexRvmi_Lx)        , V(660F00,C2,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 167, 3 , 306),
  INST(Vcmpps          , "vcmpps"          , Enc(VexRvmi_Lx)        , V(000F00,C2,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 167, 3 , 307),
  INST(Vcmpsd          , "vcmpsd"          , Enc(VexRvmi)           , V(F20F00,C2,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 428, 1 , 308),
  INST(Vcmpss          , "vcmpss"          , Enc(VexRvmi)           , V(F30F00,C2,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 429, 1 , 309),
  INST(Vcomisd         , "vcomisd"         , Enc(VexRm)             , V(660F00,2F,_,I,I,1,3,T1S), 0                         , F(RO)|F(Vex)   |A512(F_  ,0,0 ,SAE,B) , EF(WWWWWW__), 0 , 0 , 8 , 8 , 357, 1 , 310),
  INST(Vcomiss         , "vcomiss"         , Enc(VexRm)             , V(000F00,2F,_,I,I,0,2,T1S), 0                         , F(RO)|F(Vex)   |A512(F_  ,0,0 ,SAE,B) , EF(WWWWWW__), 0 , 0 , 4 , 4 , 358, 1 , 311),
  INST(Vcompresspd     , "vcompresspd"     , Enc(VexMr_Lx)          , V(660F38,8A,_,x,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 170, 3 , 312),
  INST(Vcompressps     , "vcompressps"     , Enc(VexMr_Lx)          , V(660F38,8A,_,x,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 170, 3 , 313),
  INST(Vcvtdq2pd       , "vcvtdq2pd"       , Enc(VexRm_Lx)          , V(F30F00,E6,_,x,I,0,3,HV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 4 , 173, 3 , 314),
  INST(Vcvtdq2ps       , "vcvtdq2ps"       , Enc(VexRm_Lx)          , V(000F00,5B,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 176, 3 , 315),
  INST(Vcvtpd2dq       , "vcvtpd2dq"       , Enc(VexRm_Lx)          , V(F20F00,E6,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 8 , 307, 2 , 316),
  INST(Vcvtpd2ps       , "vcvtpd2ps"       , Enc(VexRm_Lx)          , V(660F00,5A,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 8 , 179, 3 , 316),
  INST(Vcvtpd2qq       , "vcvtpd2qq"       , Enc(VexRm_Lx)          , V(660F00,7B,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 176, 3 , 317),
  INST(Vcvtpd2udq      , "vcvtpd2udq"      , Enc(VexRm_Lx)          , V(000F00,79,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 8 , 307, 2 , 318),
  INST(Vcvtpd2uqq      , "vcvtpd2uqq"      , Enc(VexRm_Lx)          , V(660F00,79,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 176, 3 , 317),
  INST(Vcvtph2ps       , "vcvtph2ps"       , Enc(VexRm_Lx)          , V(660F38,13,_,x,0,0,3,HVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 2 , 173, 3 , 319),
  INST(Vcvtps2dq       , "vcvtps2dq"       , Enc(VexRm_Lx)          , V(660F00,5B,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 176, 3 , 315),
  INST(Vcvtps2pd       , "vcvtps2pd"       , Enc(VexRm_Lx)          , V(000F00,5A,_,x,I,0,4,HV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 4 , 173, 3 , 320),
  INST(Vcvtps2ph       , "vcvtps2ph"       , Enc(VexMri_Lx)         , V(660F3A,1D,_,x,0,0,3,HVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 2 , 4 , 182, 3 , 321),
  INST(Vcvtps2qq       , "vcvtps2qq"       , Enc(VexRm_Lx)          , V(660F00,7B,_,x,_,0,3,HV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 4 , 173, 3 , 322),
  INST(Vcvtps2udq      , "vcvtps2udq"      , Enc(VexRm_Lx)          , V(000F00,79,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 176, 3 , 323),
  INST(Vcvtps2uqq      , "vcvtps2uqq"      , Enc(VexRm_Lx)          , V(660F00,79,_,x,_,0,3,HV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 4 , 173, 3 , 322),
  INST(Vcvtqq2pd       , "vcvtqq2pd"       , Enc(VexRm_Lx)          , V(F30F00,E6,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 176, 3 , 317),
  INST(Vcvtqq2ps       , "vcvtqq2ps"       , Enc(VexRm_Lx)          , V(000F00,5B,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 8 , 307, 2 , 324),
  INST(Vcvtsd2si       , "vcvtsd2si"       , Enc(VexRm)             , V(F20F00,2D,_,I,x,x,3,T1F), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,RC ,B) , EF(________), 0 , 0 , 0 , 8 , 364, 1 , 325),
  INST(Vcvtsd2ss       , "vcvtsd2ss"       , Enc(VexRvm)            , V(F20F00,5A,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 8 , 421, 1 , 326),
  INST(Vcvtsd2usi      , "vcvtsd2usi"      , Enc(VexRm)             , V(F20F00,79,_,I,_,x,3,T1F), 0                         , F(WO)          |A512(F_  ,0,0 ,RC ,B) , EF(________), 0 , 0 , 0 , 0 , 364, 1 , 327),
  INST(Vcvtsi2sd       , "vcvtsi2sd"       , Enc(VexRvm)            , V(F20F00,2A,_,I,x,x,2,T1W), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,RC ,B) , EF(________), 0 , 0 , 8 , 0 , 430, 1 , 328),
  INST(Vcvtsi2ss       , "vcvtsi2ss"       , Enc(VexRvm)            , V(F30F00,2A,_,I,x,x,2,T1W), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,RC ,B) , EF(________), 0 , 0 , 4 , 0 , 430, 1 , 329),
  INST(Vcvtss2sd       , "vcvtss2sd"       , Enc(VexRvm)            , V(F30F00,5A,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 4 , 422, 1 , 330),
  INST(Vcvtss2si       , "vcvtss2si"       , Enc(VexRm)             , V(F20F00,2D,_,I,x,x,2,T1F), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,RC ,B) , EF(________), 0 , 0 , 0 , 4 , 309, 2 , 331),
  INST(Vcvtss2usi      , "vcvtss2usi"      , Enc(VexRm)             , V(F30F00,79,_,I,_,x,2,T1F), 0                         , F(WO)          |A512(F_  ,0,0 ,RC ,B) , EF(________), 0 , 0 , 0 , 4 , 311, 2 , 332),
  INST(Vcvttpd2dq      , "vcvttpd2dq"      , Enc(VexRm_Lx)          , V(660F00,E6,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 8 , 307, 2 , 333),
  INST(Vcvttpd2qq      , "vcvttpd2qq"      , Enc(VexRm_Lx)          , V(660F00,7A,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 176, 3 , 334),
  INST(Vcvttpd2udq     , "vcvttpd2udq"     , Enc(VexRm_Lx)          , V(000F00,78,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 8 , 307, 2 , 335),
  INST(Vcvttpd2uqq     , "vcvttpd2uqq"     , Enc(VexRm_Lx)          , V(660F00,78,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 176, 3 , 336),
  INST(Vcvttps2dq      , "vcvttps2dq"      , Enc(VexRm_Lx)          , V(F30F00,5B,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 176, 3 , 337),
  INST(Vcvttps2qq      , "vcvttps2qq"      , Enc(VexRm_Lx)          , V(660F00,7A,_,x,_,0,3,HV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 4 , 173, 3 , 338),
  INST(Vcvttps2udq     , "vcvttps2udq"     , Enc(VexRm_Lx)          , V(000F00,78,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 176, 3 , 339),
  INST(Vcvttps2uqq     , "vcvttps2uqq"     , Enc(VexRm_Lx)          , V(660F00,78,_,x,_,0,3,HV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 4 , 173, 3 , 338),
  INST(Vcvttsd2si      , "vcvttsd2si"      , Enc(VexRm)             , V(F20F00,2C,_,I,x,x,3,T1F), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,SAE,B) , EF(________), 0 , 0 , 0 , 8 , 364, 1 , 340),
  INST(Vcvttsd2usi     , "vcvttsd2usi"     , Enc(VexRm)             , V(F20F00,78,_,I,_,x,3,T1F), 0                         , F(WO)          |A512(F_  ,0,0 ,SAE,B) , EF(________), 0 , 0 , 0 , 8 , 364, 1 , 341),
  INST(Vcvttss2si      , "vcvttss2si"      , Enc(VexRm)             , V(F30F00,2C,_,I,x,x,2,T1F), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,SAE,B) , EF(________), 0 , 0 , 0 , 4 , 309, 2 , 342),
  INST(Vcvttss2usi     , "vcvttss2usi"     , Enc(VexRm)             , V(F30F00,78,_,I,_,x,2,T1F), 0                         , F(WO)          |A512(F_  ,0,0 ,SAE,B) , EF(________), 0 , 0 , 0 , 4 , 311, 2 , 343),
  INST(Vcvtudq2pd      , "vcvtudq2pd"      , Enc(VexRm_Lx)          , V(F30F00,7A,_,x,_,0,3,HV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 4 , 173, 3 , 344),
  INST(Vcvtudq2ps      , "vcvtudq2ps"      , Enc(VexRm_Lx)          , V(F20F00,7A,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 176, 3 , 323),
  INST(Vcvtuqq2pd      , "vcvtuqq2pd"      , Enc(VexRm_Lx)          , V(F30F00,7A,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 176, 3 , 317),
  INST(Vcvtuqq2ps      , "vcvtuqq2ps"      , Enc(VexRm_Lx)          , V(F20F00,7A,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 8 , 307, 2 , 324),
  INST(Vcvtusi2sd      , "vcvtusi2sd"      , Enc(VexRvm)            , V(F20F00,7B,_,I,_,x,2,T1W), 0                         , F(WO)          |A512(F_  ,0,0 ,RC ,B) , EF(________), 0 , 0 , 8 , 0 , 430, 1 , 345),
  INST(Vcvtusi2ss      , "vcvtusi2ss"      , Enc(VexRvm)            , V(F30F00,7B,_,I,_,x,2,T1W), 0                         , F(WO)          |A512(F_  ,0,0 ,RC ,B) , EF(________), 0 , 0 , 4 , 0 , 430, 1 , 346),
  INST(Vdbpsadbw       , "vdbpsadbw"       , Enc(VexRvmi_Lx)        , V(660F3A,42,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 1 , 164, 3 , 347),
  INST(Vdivpd          , "vdivpd"          , Enc(VexRvm_Lx)         , V(660F00,5E,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 277),
  INST(Vdivps          , "vdivps"          , Enc(VexRvm_Lx)         , V(000F00,5E,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 278),
  INST(Vdivsd          , "vdivsd"          , Enc(VexRvm)            , V(F20F00,5E,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 421, 1 , 279),
  INST(Vdivss          , "vdivss"          , Enc(VexRvm)            , V(F30F00,5E,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 422, 1 , 280),
  INST(Vdppd           , "vdppd"           , Enc(VexRvmi_Lx)        , V(660F3A,41,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 164, 2 , 293),
  INST(Vdpps           , "vdpps"           , Enc(VexRvmi_Lx)        , V(660F3A,40,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 164, 2 , 294),
  INST(Vexp2pd         , "vexp2pd"         , Enc(VexRm)             , V(660F38,C8,_,2,_,1,4,FV ), 0                         , F(WO)          |A512(ER  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 67 , 1 , 348),
  INST(Vexp2ps         , "vexp2ps"         , Enc(VexRm)             , V(660F38,C8,_,2,_,0,4,FV ), 0                         , F(WO)          |A512(ER  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 67 , 1 , 349),
  INST(Vexpandpd       , "vexpandpd"       , Enc(VexRm_Lx)          , V(660F38,88,_,x,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 176, 3 , 350),
  INST(Vexpandps       , "vexpandps"       , Enc(VexRm_Lx)          , V(660F38,88,_,x,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 176, 3 , 351),
  INST(Vextractf128    , "vextractf128"    , Enc(VexMri)            , V(660F3A,19,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 16, 16, 183, 1 , 352),
  INST(Vextractf32x4   , "vextractf32x4"   , Enc(VexMri_Lx)         , V(660F3A,19,_,x,_,0,4,T4 ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 16, 16, 431, 1 , 353),
  INST(Vextractf32x8   , "vextractf32x8"   , Enc(VexMri)            , V(660F3A,1B,_,2,_,0,5,T8 ), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 32, 32, 184, 1 , 354),
  INST(Vextractf64x2   , "vextractf64x2"   , Enc(VexMri_Lx)         , V(660F3A,19,_,x,_,1,4,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 16, 16, 431, 1 , 355),
  INST(Vextractf64x4   , "vextractf64x4"   , Enc(VexMri)            , V(660F3A,1B,_,2,_,1,5,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 32, 32, 184, 1 , 356),
  INST(Vextracti128    , "vextracti128"    , Enc(VexMri)            , V(660F3A,39,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 16, 16, 183, 1 , 352),
  INST(Vextracti32x4   , "vextracti32x4"   , Enc(VexMri_Lx)         , V(660F3A,39,_,x,_,0,4,T4 ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 16, 16, 431, 1 , 353),
  INST(Vextracti32x8   , "vextracti32x8"   , Enc(VexMri)            , V(660F3A,3B,_,2,_,0,5,T8 ), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 32, 32, 184, 1 , 354),
  INST(Vextracti64x2   , "vextracti64x2"   , Enc(VexMri_Lx)         , V(660F3A,39,_,x,_,1,4,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 16, 16, 431, 1 , 355),
  INST(Vextracti64x4   , "vextracti64x4"   , Enc(VexMri)            , V(660F3A,3B,_,2,_,1,5,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 32, 32, 184, 1 , 356),
  INST(Vextractps      , "vextractps"      , Enc(VexMri)            , V(660F3A,17,_,0,I,I,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 370, 1 , 357),
  INST(Vfixupimmpd     , "vfixupimmpd"     , Enc(VexRvmi_Lx)        , V(660F3A,54,_,x,_,1,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 185, 3 , 358),
  INST(Vfixupimmps     , "vfixupimmps"     , Enc(VexRvmi_Lx)        , V(660F3A,54,_,x,_,0,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 185, 3 , 359),
  INST(Vfixupimmsd     , "vfixupimmsd"     , Enc(VexRvmi)           , V(660F3A,55,_,I,_,1,3,T1S), 0                         , F(RW)          |A512(F_  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 432, 1 , 360),
  INST(Vfixupimmss     , "vfixupimmss"     , Enc(VexRvmi)           , V(660F3A,55,_,I,_,0,2,T1S), 0                         , F(RW)          |A512(F_  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 433, 1 , 361),
  INST(Vfmadd132pd     , "vfmadd132pd"     , Enc(VexRvm_Lx)         , V(660F38,98,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 362),
  INST(Vfmadd132ps     , "vfmadd132ps"     , Enc(VexRvm_Lx)         , V(660F38,98,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 363),
  INST(Vfmadd132sd     , "vfmadd132sd"     , Enc(VexRvm)            , V(660F38,99,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 434, 1 , 364),
  INST(Vfmadd132ss     , "vfmadd132ss"     , Enc(VexRvm)            , V(660F38,99,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 435, 1 , 365),
  INST(Vfmadd213pd     , "vfmadd213pd"     , Enc(VexRvm_Lx)         , V(660F38,A8,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 362),
  INST(Vfmadd213ps     , "vfmadd213ps"     , Enc(VexRvm_Lx)         , V(660F38,A8,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 363),
  INST(Vfmadd213sd     , "vfmadd213sd"     , Enc(VexRvm)            , V(660F38,A9,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 434, 1 , 364),
  INST(Vfmadd213ss     , "vfmadd213ss"     , Enc(VexRvm)            , V(660F38,A9,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 435, 1 , 365),
  INST(Vfmadd231pd     , "vfmadd231pd"     , Enc(VexRvm_Lx)         , V(660F38,B8,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 362),
  INST(Vfmadd231ps     , "vfmadd231ps"     , Enc(VexRvm_Lx)         , V(660F38,B8,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 363),
  INST(Vfmadd231sd     , "vfmadd231sd"     , Enc(VexRvm)            , V(660F38,B9,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 434, 1 , 364),
  INST(Vfmadd231ss     , "vfmadd231ss"     , Enc(VexRvm)            , V(660F38,B9,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 435, 1 , 365),
  INST(Vfmaddpd        , "vfmaddpd"        , Enc(Fma4_Lx)           , V(660F3A,69,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 119, 4 , 366),
  INST(Vfmaddps        , "vfmaddps"        , Enc(Fma4_Lx)           , V(660F3A,68,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 119, 4 , 367),
  INST(Vfmaddsd        , "vfmaddsd"        , Enc(Fma4)              , V(660F3A,6B,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 313, 2 , 368),
  INST(Vfmaddss        , "vfmaddss"        , Enc(Fma4)              , V(660F3A,6A,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 315, 2 , 369),
  INST(Vfmaddsub132pd  , "vfmaddsub132pd"  , Enc(VexRvm_Lx)         , V(660F38,96,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 362),
  INST(Vfmaddsub132ps  , "vfmaddsub132ps"  , Enc(VexRvm_Lx)         , V(660F38,96,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 363),
  INST(Vfmaddsub213pd  , "vfmaddsub213pd"  , Enc(VexRvm_Lx)         , V(660F38,A6,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 362),
  INST(Vfmaddsub213ps  , "vfmaddsub213ps"  , Enc(VexRvm_Lx)         , V(660F38,A6,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 363),
  INST(Vfmaddsub231pd  , "vfmaddsub231pd"  , Enc(VexRvm_Lx)         , V(660F38,B6,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 362),
  INST(Vfmaddsub231ps  , "vfmaddsub231ps"  , Enc(VexRvm_Lx)         , V(660F38,B6,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 363),
  INST(Vfmaddsubpd     , "vfmaddsubpd"     , Enc(Fma4_Lx)           , V(660F3A,5D,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 119, 4 , 366),
  INST(Vfmaddsubps     , "vfmaddsubps"     , Enc(Fma4_Lx)           , V(660F3A,5C,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 119, 4 , 367),
  INST(Vfmsub132pd     , "vfmsub132pd"     , Enc(VexRvm_Lx)         , V(660F38,9A,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 362),
  INST(Vfmsub132ps     , "vfmsub132ps"     , Enc(VexRvm_Lx)         , V(660F38,9A,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 363),
  INST(Vfmsub132sd     , "vfmsub132sd"     , Enc(VexRvm)            , V(660F38,9B,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 434, 1 , 364),
  INST(Vfmsub132ss     , "vfmsub132ss"     , Enc(VexRvm)            , V(660F38,9B,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 435, 1 , 365),
  INST(Vfmsub213pd     , "vfmsub213pd"     , Enc(VexRvm_Lx)         , V(660F38,AA,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 362),
  INST(Vfmsub213ps     , "vfmsub213ps"     , Enc(VexRvm_Lx)         , V(660F38,AA,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 363),
  INST(Vfmsub213sd     , "vfmsub213sd"     , Enc(VexRvm)            , V(660F38,AB,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 434, 1 , 364),
  INST(Vfmsub213ss     , "vfmsub213ss"     , Enc(VexRvm)            , V(660F38,AB,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 435, 1 , 365),
  INST(Vfmsub231pd     , "vfmsub231pd"     , Enc(VexRvm_Lx)         , V(660F38,BA,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 362),
  INST(Vfmsub231ps     , "vfmsub231ps"     , Enc(VexRvm_Lx)         , V(660F38,BA,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 363),
  INST(Vfmsub231sd     , "vfmsub231sd"     , Enc(VexRvm)            , V(660F38,BB,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 434, 1 , 364),
  INST(Vfmsub231ss     , "vfmsub231ss"     , Enc(VexRvm)            , V(660F38,BB,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 435, 1 , 365),
  INST(Vfmsubadd132pd  , "vfmsubadd132pd"  , Enc(VexRvm_Lx)         , V(660F38,97,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 362),
  INST(Vfmsubadd132ps  , "vfmsubadd132ps"  , Enc(VexRvm_Lx)         , V(660F38,97,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 363),
  INST(Vfmsubadd213pd  , "vfmsubadd213pd"  , Enc(VexRvm_Lx)         , V(660F38,A7,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 362),
  INST(Vfmsubadd213ps  , "vfmsubadd213ps"  , Enc(VexRvm_Lx)         , V(660F38,A7,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 363),
  INST(Vfmsubadd231pd  , "vfmsubadd231pd"  , Enc(VexRvm_Lx)         , V(660F38,B7,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 362),
  INST(Vfmsubadd231ps  , "vfmsubadd231ps"  , Enc(VexRvm_Lx)         , V(660F38,B7,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 363),
  INST(Vfmsubaddpd     , "vfmsubaddpd"     , Enc(Fma4_Lx)           , V(660F3A,5F,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 119, 4 , 366),
  INST(Vfmsubaddps     , "vfmsubaddps"     , Enc(Fma4_Lx)           , V(660F3A,5E,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 119, 4 , 367),
  INST(Vfmsubpd        , "vfmsubpd"        , Enc(Fma4_Lx)           , V(660F3A,6D,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 119, 4 , 366),
  INST(Vfmsubps        , "vfmsubps"        , Enc(Fma4_Lx)           , V(660F3A,6C,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 119, 4 , 367),
  INST(Vfmsubsd        , "vfmsubsd"        , Enc(Fma4)              , V(660F3A,6F,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 313, 2 , 368),
  INST(Vfmsubss        , "vfmsubss"        , Enc(Fma4)              , V(660F3A,6E,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 315, 2 , 369),
  INST(Vfnmadd132pd    , "vfnmadd132pd"    , Enc(VexRvm_Lx)         , V(660F38,9C,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 362),
  INST(Vfnmadd132ps    , "vfnmadd132ps"    , Enc(VexRvm_Lx)         , V(660F38,9C,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 363),
  INST(Vfnmadd132sd    , "vfnmadd132sd"    , Enc(VexRvm)            , V(660F38,9D,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 434, 1 , 364),
  INST(Vfnmadd132ss    , "vfnmadd132ss"    , Enc(VexRvm)            , V(660F38,9D,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 435, 1 , 365),
  INST(Vfnmadd213pd    , "vfnmadd213pd"    , Enc(VexRvm_Lx)         , V(660F38,AC,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 362),
  INST(Vfnmadd213ps    , "vfnmadd213ps"    , Enc(VexRvm_Lx)         , V(660F38,AC,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 363),
  INST(Vfnmadd213sd    , "vfnmadd213sd"    , Enc(VexRvm)            , V(660F38,AD,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 434, 1 , 364),
  INST(Vfnmadd213ss    , "vfnmadd213ss"    , Enc(VexRvm)            , V(660F38,AD,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 435, 1 , 365),
  INST(Vfnmadd231pd    , "vfnmadd231pd"    , Enc(VexRvm_Lx)         , V(660F38,BC,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 362),
  INST(Vfnmadd231ps    , "vfnmadd231ps"    , Enc(VexRvm_Lx)         , V(660F38,BC,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 363),
  INST(Vfnmadd231sd    , "vfnmadd231sd"    , Enc(VexRvm)            , V(660F38,BC,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 434, 1 , 364),
  INST(Vfnmadd231ss    , "vfnmadd231ss"    , Enc(VexRvm)            , V(660F38,BC,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 435, 1 , 365),
  INST(Vfnmaddpd       , "vfnmaddpd"       , Enc(Fma4_Lx)           , V(660F3A,79,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 119, 4 , 366),
  INST(Vfnmaddps       , "vfnmaddps"       , Enc(Fma4_Lx)           , V(660F3A,78,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 119, 4 , 367),
  INST(Vfnmaddsd       , "vfnmaddsd"       , Enc(Fma4)              , V(660F3A,7B,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 313, 2 , 368),
  INST(Vfnmaddss       , "vfnmaddss"       , Enc(Fma4)              , V(660F3A,7A,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 315, 2 , 369),
  INST(Vfnmsub132pd    , "vfnmsub132pd"    , Enc(VexRvm_Lx)         , V(660F38,9E,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 362),
  INST(Vfnmsub132ps    , "vfnmsub132ps"    , Enc(VexRvm_Lx)         , V(660F38,9E,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 363),
  INST(Vfnmsub132sd    , "vfnmsub132sd"    , Enc(VexRvm)            , V(660F38,9F,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 434, 1 , 364),
  INST(Vfnmsub132ss    , "vfnmsub132ss"    , Enc(VexRvm)            , V(660F38,9F,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 435, 1 , 365),
  INST(Vfnmsub213pd    , "vfnmsub213pd"    , Enc(VexRvm_Lx)         , V(660F38,AE,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 362),
  INST(Vfnmsub213ps    , "vfnmsub213ps"    , Enc(VexRvm_Lx)         , V(660F38,AE,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 363),
  INST(Vfnmsub213sd    , "vfnmsub213sd"    , Enc(VexRvm)            , V(660F38,AF,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 434, 1 , 364),
  INST(Vfnmsub213ss    , "vfnmsub213ss"    , Enc(VexRvm)            , V(660F38,AF,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 435, 1 , 365),
  INST(Vfnmsub231pd    , "vfnmsub231pd"    , Enc(VexRvm_Lx)         , V(660F38,BE,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 362),
  INST(Vfnmsub231ps    , "vfnmsub231ps"    , Enc(VexRvm_Lx)         , V(660F38,BE,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 363),
  INST(Vfnmsub231sd    , "vfnmsub231sd"    , Enc(VexRvm)            , V(660F38,BF,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 434, 1 , 364),
  INST(Vfnmsub231ss    , "vfnmsub231ss"    , Enc(VexRvm)            , V(660F38,BF,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 435, 1 , 365),
  INST(Vfnmsubpd       , "vfnmsubpd"       , Enc(Fma4_Lx)           , V(660F3A,7D,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 119, 4 , 366),
  INST(Vfnmsubps       , "vfnmsubps"       , Enc(Fma4_Lx)           , V(660F3A,7C,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 119, 4 , 367),
  INST(Vfnmsubsd       , "vfnmsubsd"       , Enc(Fma4)              , V(660F3A,7F,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 313, 2 , 368),
  INST(Vfnmsubss       , "vfnmsubss"       , Enc(Fma4)              , V(660F3A,7E,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 315, 2 , 369),
  INST(Vfpclasspd      , "vfpclasspd"      , Enc(VexRmi_Lx)         , V(660F3A,66,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,K_,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 436, 1 , 370),
  INST(Vfpclassps      , "vfpclassps"      , Enc(VexRmi_Lx)         , V(660F3A,66,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,K_,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 436, 1 , 371),
  INST(Vfpclasssd      , "vfpclasssd"      , Enc(VexRmi_Lx)         , V(660F3A,67,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(DQ  ,0,K_,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 437, 1 , 372),
  INST(Vfpclassss      , "vfpclassss"      , Enc(VexRmi_Lx)         , V(660F3A,67,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(DQ  ,0,K_,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 438, 1 , 373),
  INST(Vfrczpd         , "vfrczpd"         , Enc(VexRm_Lx)          , V(XOP_M9,81,_,x,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 176, 2 , 374),
  INST(Vfrczps         , "vfrczps"         , Enc(VexRm_Lx)          , V(XOP_M9,80,_,x,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 176, 2 , 375),
  INST(Vfrczsd         , "vfrczsd"         , Enc(VexRm)             , V(XOP_M9,83,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 61 , 1 , 114),
  INST(Vfrczss         , "vfrczss"         , Enc(VexRm)             , V(XOP_M9,82,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 227, 1 , 113),
  INST(Vgatherdpd      , "vgatherdpd"      , Enc(VexRmvRm_VM)       , V(660F38,92,_,x,1,_,_,_  ), V(660F38,92,_,x,_,1,3,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 92 , 5 , 376),
  INST(Vgatherdps      , "vgatherdps"      , Enc(VexRmvRm_VM)       , V(660F38,92,_,x,0,_,_,_  ), V(660F38,92,_,x,_,0,2,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 97 , 5 , 377),
  INST(Vgatherpf0dpd   , "vgatherpf0dpd"   , Enc(VexM_VM)           , V(660F38,C6,1,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PF  ,0,K_,0  ,B) , EF(________), 0 , 0 , 0 , 8 , 439, 1 , 378),
  INST(Vgatherpf0dps   , "vgatherpf0dps"   , Enc(VexM_VM)           , V(660F38,C6,1,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PF  ,0,K_,0  ,B) , EF(________), 0 , 0 , 0 , 4 , 440, 1 , 379),
  INST(Vgatherpf0qpd   , "vgatherpf0qpd"   , Enc(VexM_VM)           , V(660F38,C7,1,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PF  ,0,K_,0  ,B) , EF(________), 0 , 0 , 0 , 8 , 441, 1 , 378),
  INST(Vgatherpf0qps   , "vgatherpf0qps"   , Enc(VexM_VM)           , V(660F38,C7,1,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PF  ,0,K_,0  ,B) , EF(________), 0 , 0 , 0 , 4 , 441, 1 , 379),
  INST(Vgatherpf1dpd   , "vgatherpf1dpd"   , Enc(VexM_VM)           , V(660F38,C6,2,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PF  ,0,K_,0  ,B) , EF(________), 0 , 0 , 0 , 8 , 439, 1 , 378),
  INST(Vgatherpf1dps   , "vgatherpf1dps"   , Enc(VexM_VM)           , V(660F38,C6,2,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PF  ,0,K_,0  ,B) , EF(________), 0 , 0 , 0 , 4 , 440, 1 , 379),
  INST(Vgatherpf1qpd   , "vgatherpf1qpd"   , Enc(VexM_VM)           , V(660F38,C7,2,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PF  ,0,K_,0  ,B) , EF(________), 0 , 0 , 0 , 8 , 441, 1 , 378),
  INST(Vgatherpf1qps   , "vgatherpf1qps"   , Enc(VexM_VM)           , V(660F38,C7,2,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PF  ,0,K_,0  ,B) , EF(________), 0 , 0 , 0 , 4 , 441, 1 , 379),
  INST(Vgatherqpd      , "vgatherqpd"      , Enc(VexRmvRm_VM)       , V(660F38,93,_,x,1,_,_,_  ), V(660F38,93,_,x,_,1,3,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 102, 5 , 380),
  INST(Vgatherqps      , "vgatherqps"      , Enc(VexRmvRm_VM)       , V(660F38,93,_,x,0,_,_,_  ), V(660F38,93,_,x,_,0,2,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 123, 4 , 381),
  INST(Vgetexppd       , "vgetexppd"       , Enc(VexRm_Lx)          , V(660F38,42,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 176, 3 , 334),
  INST(Vgetexpps       , "vgetexpps"       , Enc(VexRm_Lx)          , V(660F38,42,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 176, 3 , 339),
  INST(Vgetexpsd       , "vgetexpsd"       , Enc(VexRm)             , V(660F38,43,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 61 , 1 , 382),
  INST(Vgetexpss       , "vgetexpss"       , Enc(VexRm)             , V(660F38,43,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 227, 1 , 383),
  INST(Vgetmantpd      , "vgetmantpd"      , Enc(VexRmi_Lx)         , V(660F3A,26,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 191, 3 , 384),
  INST(Vgetmantps      , "vgetmantps"      , Enc(VexRmi_Lx)         , V(660F3A,26,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 191, 3 , 385),
  INST(Vgetmantsd      , "vgetmantsd"      , Enc(VexRmi)            , V(660F3A,27,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 416, 1 , 386),
  INST(Vgetmantss      , "vgetmantss"      , Enc(VexRmi)            , V(660F3A,27,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 417, 1 , 387),
  INST(Vhaddpd         , "vhaddpd"         , Enc(VexRvm_Lx)         , V(660F00,7C,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 161, 2 , 281),
  INST(Vhaddps         , "vhaddps"         , Enc(VexRvm_Lx)         , V(F20F00,7C,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 161, 2 , 282),
  INST(Vhsubpd         , "vhsubpd"         , Enc(VexRvm_Lx)         , V(660F00,7D,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 161, 2 , 281),
  INST(Vhsubps         , "vhsubps"         , Enc(VexRvm_Lx)         , V(F20F00,7D,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 161, 2 , 282),
  INST(Vinsertf128     , "vinsertf128"     , Enc(VexRvmi)           , V(660F3A,18,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 16, 16, 317, 1 , 388),
  INST(Vinsertf32x4    , "vinsertf32x4"    , Enc(VexRvmi_Lx)        , V(660F3A,18,_,x,_,0,4,T4 ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 16, 16, 317, 2 , 389),
  INST(Vinsertf32x8    , "vinsertf32x8"    , Enc(VexRvmi)           , V(660F3A,1A,_,2,_,0,5,T8 ), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 32, 32, 442, 1 , 390),
  INST(Vinsertf64x2    , "vinsertf64x2"    , Enc(VexRvmi_Lx)        , V(660F3A,18,_,x,_,1,4,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 16, 16, 317, 2 , 391),
  INST(Vinsertf64x4    , "vinsertf64x4"    , Enc(VexRvmi)           , V(660F3A,1A,_,2,_,1,5,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 32, 32, 442, 1 , 392),
  INST(Vinserti128     , "vinserti128"     , Enc(VexRvmi)           , V(660F3A,38,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 16, 16, 317, 1 , 388),
  INST(Vinserti32x4    , "vinserti32x4"    , Enc(VexRvmi_Lx)        , V(660F3A,38,_,x,_,0,4,T4 ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 16, 16, 317, 2 , 389),
  INST(Vinserti32x8    , "vinserti32x8"    , Enc(VexRvmi)           , V(660F3A,3A,_,2,_,0,5,T8 ), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 32, 32, 442, 1 , 390),
  INST(Vinserti64x2    , "vinserti64x2"    , Enc(VexRvmi_Lx)        , V(660F3A,38,_,x,_,1,4,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 16, 16, 317, 2 , 391),
  INST(Vinserti64x4    , "vinserti64x4"    , Enc(VexRvmi)           , V(660F3A,3A,_,2,_,1,5,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 32, 32, 442, 1 , 392),
  INST(Vinsertps       , "vinsertps"       , Enc(VexRvmi)           , V(660F3A,21,_,0,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 443, 1 , 393),
  INST(Vlddqu          , "vlddqu"          , Enc(VexRm_Lx)          , V(F20F00,F0,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 16, 16, 200, 2 , 394),
  INST(Vldmxcsr        , "vldmxcsr"        , Enc(VexM)              , V(000F00,AE,2,0,I,_,_,_  ), 0                         , F(RO)|F(Vex)|F(Volatile)              , EF(________), 0 , 0 , 0 , 0 , 387, 1 , 395),
  INST(Vmaskmovdqu     , "vmaskmovdqu"     , Enc(VexRmZDI)          , V(660F00,F7,_,0,I,_,_,_  ), 0                         , F(RO)|F(Vex)|F(Special)               , EF(________), 0 , 0 , 16, 16, 444, 1 , 396),
  INST(Vmaskmovpd      , "vmaskmovpd"      , Enc(VexRvmMvr_Lx)      , V(660F38,2D,_,x,0,_,_,_  ), V(660F38,2F,_,x,0,_,_,_  ), F(RW)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 127, 4 , 397),
  INST(Vmaskmovps      , "vmaskmovps"      , Enc(VexRvmMvr_Lx)      , V(660F38,2C,_,x,0,_,_,_  ), V(660F38,2E,_,x,0,_,_,_  ), F(RW)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 127, 4 , 398),
  INST(Vmaxpd          , "vmaxpd"          , Enc(VexRvm_Lx)         , V(660F00,5F,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 399),
  INST(Vmaxps          , "vmaxps"          , Enc(VexRvm_Lx)         , V(000F00,5F,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 400),
  INST(Vmaxsd          , "vmaxsd"          , Enc(VexRvm)            , V(F20F00,5F,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 421, 1 , 401),
  INST(Vmaxss          , "vmaxss"          , Enc(VexRvm)            , V(F30F00,5F,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 422, 1 , 402),
  INST(Vminpd          , "vminpd"          , Enc(VexRvm_Lx)         , V(660F00,5D,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 399),
  INST(Vminps          , "vminps"          , Enc(VexRvm_Lx)         , V(000F00,5D,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 400),
  INST(Vminsd          , "vminsd"          , Enc(VexRvm)            , V(F20F00,5D,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 421, 1 , 401),
  INST(Vminss          , "vminss"          , Enc(VexRvm)            , V(F30F00,5D,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 422, 1 , 402),
  INST(Vmovapd         , "vmovapd"         , Enc(VexRmMr_Lx)        , V(660F00,28,_,x,I,1,4,FVM), V(660F00,29,_,x,I,1,4,FVM), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 63 , 6 , 403),
  INST(Vmovaps         , "vmovaps"         , Enc(VexRmMr_Lx)        , V(000F00,28,_,x,I,0,4,FVM), V(000F00,29,_,x,I,0,4,FVM), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 63 , 6 , 404),
  INST(Vmovd           , "vmovd"           , Enc(VexMovDQ)          , V(660F00,6E,_,0,0,0,2,T1S), V(660F00,7E,_,0,0,0,2,T1S), F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 319, 2 , 405),
  INST(Vmovddup        , "vmovddup"        , Enc(VexRm_Lx)          , V(F20F00,12,_,x,I,1,3,DUP), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 194, 3 , 304),
  INST(Vmovdqa         , "vmovdqa"         , Enc(VexRmMr_Lx)        , V(660F00,6F,_,x,I,_,_,_  ), V(660F00,7F,_,x,I,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 16, 16, 63 , 4 , 406),
  INST(Vmovdqa32       , "vmovdqa32"       , Enc(VexRmMr_Lx)        , V(660F00,6F,_,x,_,0,4,FVM), V(660F00,7F,_,x,_,0,4,FVM), F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 63 , 6 , 407),
  INST(Vmovdqa64       , "vmovdqa64"       , Enc(VexRmMr_Lx)        , V(660F00,6F,_,x,_,1,4,FVM), V(660F00,7F,_,x,_,1,4,FVM), F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 63 , 6 , 408),
  INST(Vmovdqu         , "vmovdqu"         , Enc(VexRmMr_Lx)        , V(F30F00,6F,_,x,I,_,_,_  ), V(F30F00,7F,_,x,I,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 16, 16, 63 , 4 , 409),
  INST(Vmovdqu16       , "vmovdqu16"       , Enc(VexRmMr_Lx)        , V(F20F00,6F,_,x,_,1,4,FVM), V(F20F00,7F,_,x,_,1,4,FVM), F(WO)          |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 63 , 6 , 410),
  INST(Vmovdqu32       , "vmovdqu32"       , Enc(VexRmMr_Lx)        , V(F30F00,6F,_,x,_,0,4,FVM), V(F30F00,7F,_,x,_,0,4,FVM), F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 63 , 6 , 411),
  INST(Vmovdqu64       , "vmovdqu64"       , Enc(VexRmMr_Lx)        , V(F30F00,6F,_,x,_,1,4,FVM), V(F30F00,7F,_,x,_,1,4,FVM), F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 63 , 6 , 412),
  INST(Vmovdqu8        , "vmovdqu8"        , Enc(VexRmMr_Lx)        , V(F20F00,6F,_,x,_,0,4,FVM), V(F20F00,7F,_,x,_,0,4,FVM), F(WO)          |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 63 , 6 , 413),
  INST(Vmovhlps        , "vmovhlps"        , Enc(VexRvm)            , V(000F00,12,_,0,I,0,_,_  ), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 208, 1 , 414),
  INST(Vmovhpd         , "vmovhpd"         , Enc(VexRvmMr)          , V(660F00,16,_,0,I,1,3,T1S), V(660F00,17,_,0,I,1,3,T1S), F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 321, 2 , 415),
  INST(Vmovhps         , "vmovhps"         , Enc(VexRvmMr)          , V(000F00,16,_,0,I,0,3,T2 ), V(000F00,17,_,0,I,0,3,T2 ), F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 321, 2 , 416),
  INST(Vmovlhps        , "vmovlhps"        , Enc(VexRvm)            , V(000F00,16,_,0,I,0,_,_  ), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 208, 1 , 414),
  INST(Vmovlpd         , "vmovlpd"         , Enc(VexRvmMr)          , V(660F00,12,_,0,I,1,3,T1S), V(660F00,13,_,0,I,1,3,T1S), F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 321, 2 , 417),
  INST(Vmovlps         , "vmovlps"         , Enc(VexRvmMr)          , V(000F00,12,_,0,I,0,3,T2 ), V(000F00,13,_,0,I,0,3,T2 ), F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 321, 2 , 418),
  INST(Vmovmskpd       , "vmovmskpd"       , Enc(VexRm_Lx)          , V(660F00,50,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 0 , 8 , 445, 1 , 419),
  INST(Vmovmskps       , "vmovmskps"       , Enc(VexRm_Lx)          , V(000F00,50,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 0 , 4 , 445, 1 , 420),
  INST(Vmovntdq        , "vmovntdq"        , Enc(VexMr_Lx)          , V(660F00,E7,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,B) , EF(________), 0 , 0 , 16, 16, 197, 3 , 421),
  INST(Vmovntdqa       , "vmovntdqa"       , Enc(VexRm_Lx)          , V(660F38,2A,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,B) , EF(________), 0 , 0 , 16, 16, 200, 3 , 422),
  INST(Vmovntpd        , "vmovntpd"        , Enc(VexMr_Lx)          , V(660F00,2B,_,x,I,1,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 197, 3 , 423),
  INST(Vmovntps        , "vmovntps"        , Enc(VexMr_Lx)          , V(000F00,2B,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 197, 3 , 424),
  INST(Vmovq           , "vmovq"           , Enc(VexMovDQ)          , V(660F00,6E,_,0,I,1,3,T1S), V(660F00,7E,_,0,I,1,3,T1S), F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 203, 3 , 425),
  INST(Vmovsd          , "vmovsd"          , Enc(VexMovSsSd)        , V(F20F00,10,_,I,I,1,3,T1S), V(F20F00,11,_,I,I,1,3,T1S), F(WO)|F(Vex)   |A512(F_  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 206, 3 , 426),
  INST(Vmovshdup       , "vmovshdup"       , Enc(VexRm_Lx)          , V(F30F00,16,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 176, 3 , 305),
  INST(Vmovsldup       , "vmovsldup"       , Enc(VexRm_Lx)          , V(F30F00,12,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 176, 3 , 305),
  INST(Vmovss          , "vmovss"          , Enc(VexMovSsSd)        , V(F30F00,10,_,I,I,0,2,T1S), V(F30F00,11,_,I,I,0,2,T1S), F(WO)|F(Vex)   |A512(F_  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 209, 3 , 427),
  INST(Vmovupd         , "vmovupd"         , Enc(VexRmMr_Lx)        , V(660F00,10,_,x,I,1,4,FVM), V(660F00,11,_,x,I,1,4,FVM), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 63 , 6 , 428),
  INST(Vmovups         , "vmovups"         , Enc(VexRmMr_Lx)        , V(000F00,10,_,x,I,0,4,FVM), V(000F00,11,_,x,I,0,4,FVM), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 63 , 6 , 429),
  INST(Vmpsadbw        , "vmpsadbw"        , Enc(VexRvmi_Lx)        , V(660F3A,42,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 1 , 164, 2 , 430),
  INST(Vmulpd          , "vmulpd"          , Enc(VexRvm_Lx)         , V(660F00,59,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 277),
  INST(Vmulps          , "vmulps"          , Enc(VexRvm_Lx)         , V(000F00,59,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 278),
  INST(Vmulsd          , "vmulsd"          , Enc(VexRvm_Lx)         , V(F20F00,59,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 421, 1 , 431),
  INST(Vmulss          , "vmulss"          , Enc(VexRvm_Lx)         , V(F30F00,59,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 422, 1 , 432),
  INST(Vorpd           , "vorpd"           , Enc(VexRvm_Lx)         , V(660F00,56,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 287),
  INST(Vorps           , "vorps"           , Enc(VexRvm_Lx)         , V(000F00,56,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 433),
  INST(Vpabsb          , "vpabsb"          , Enc(VexRm_Lx)          , V(660F38,1C,_,x,I,_,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 176, 3 , 434),
  INST(Vpabsd          , "vpabsd"          , Enc(VexRm_Lx)          , V(660F38,1E,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 176, 3 , 305),
  INST(Vpabsq          , "vpabsq"          , Enc(VexRm_Lx)          , V(660F38,1F,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 176, 3 , 350),
  INST(Vpabsw          , "vpabsw"          , Enc(VexRm_Lx)          , V(660F38,1D,_,x,I,_,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 176, 3 , 435),
  INST(Vpackssdw       , "vpackssdw"       , Enc(VexRvm_Lx)         , V(660F00,6B,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 4 , 161, 3 , 436),
  INST(Vpacksswb       , "vpacksswb"       , Enc(VexRvm_Lx)         , V(660F00,63,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 2 , 161, 3 , 437),
  INST(Vpackusdw       , "vpackusdw"       , Enc(VexRvm_Lx)         , V(660F38,2B,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 4 , 161, 3 , 436),
  INST(Vpackuswb       , "vpackuswb"       , Enc(VexRvm_Lx)         , V(660F00,67,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 2 , 161, 3 , 437),
  INST(Vpaddb          , "vpaddb"          , Enc(VexRvm_Lx)         , V(660F00,FC,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 161, 3 , 438),
  INST(Vpaddd          , "vpaddd"          , Enc(VexRvm_Lx)         , V(660F00,FE,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 433),
  INST(Vpaddq          , "vpaddq"          , Enc(VexRvm_Lx)         , V(660F00,D4,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 439),
  INST(Vpaddsb         , "vpaddsb"         , Enc(VexRvm_Lx)         , V(660F00,EC,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 161, 3 , 438),
  INST(Vpaddsw         , "vpaddsw"         , Enc(VexRvm_Lx)         , V(660F00,ED,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 440),
  INST(Vpaddusb        , "vpaddusb"        , Enc(VexRvm_Lx)         , V(660F00,DC,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 161, 3 , 438),
  INST(Vpaddusw        , "vpaddusw"        , Enc(VexRvm_Lx)         , V(660F00,DD,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 440),
  INST(Vpaddw          , "vpaddw"          , Enc(VexRvm_Lx)         , V(660F00,FD,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 440),
  INST(Vpalignr        , "vpalignr"        , Enc(VexRvmi_Lx)        , V(660F3A,0F,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 0 , 0 , 164, 3 , 441),
  INST(Vpand           , "vpand"           , Enc(VexRvm_Lx)         , V(660F00,DB,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 16, 16, 161, 2 , 442),
  INST(Vpandd          , "vpandd"          , Enc(VexRvm_Lx)         , V(660F00,DB,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 290),
  INST(Vpandn          , "vpandn"          , Enc(VexRvm_Lx)         , V(660F00,DF,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 16, 16, 161, 2 , 442),
  INST(Vpandnd         , "vpandnd"         , Enc(VexRvm_Lx)         , V(660F00,DF,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 290),
  INST(Vpandnq         , "vpandnq"         , Enc(VexRvm_Lx)         , V(660F00,DF,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 291),
  INST(Vpandq          , "vpandq"          , Enc(VexRvm_Lx)         , V(660F00,DB,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 291),
  INST(Vpavgb          , "vpavgb"          , Enc(VexRvm_Lx)         , V(660F00,E0,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 161, 3 , 438),
  INST(Vpavgw          , "vpavgw"          , Enc(VexRvm_Lx)         , V(660F00,E3,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 440),
  INST(Vpblendd        , "vpblendd"        , Enc(VexRvmi_Lx)        , V(660F3A,02,_,x,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 164, 2 , 294),
  INST(Vpblendvb       , "vpblendvb"       , Enc(VexRvmr)           , V(660F3A,4C,_,x,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1 , 1 , 305, 2 , 443),
  INST(Vpblendw        , "vpblendw"        , Enc(VexRvmi_Lx)        , V(660F3A,0E,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 164, 2 , 444),
  INST(Vpbroadcastb    , "vpbroadcastb"    , Enc(VexRm_Lx)          , V(660F38,78,_,x,0,0,0,T1S), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 323, 2 , 434),
  INST(Vpbroadcastd    , "vpbroadcastd"    , Enc(VexRm_Lx)          , V(660F38,58,_,x,0,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 325, 2 , 305),
  INST(Vpbroadcastmb2d , "vpbroadcastmb2d" , Enc(VexRm_Lx)          , V(F30F38,3A,_,x,_,0,_,_  ), 0                         , F(WO)          |A512(CD  ,1,0 ,0  ,B) , EF(________), 0 , 0 , 4 , 1 , 446, 1 , 445),
  INST(Vpbroadcastmb2q , "vpbroadcastmb2q" , Enc(VexRm_Lx)          , V(F30F38,2A,_,x,_,1,_,_  ), 0                         , F(WO)          |A512(CD  ,1,0 ,0  ,B) , EF(________), 0 , 0 , 8 , 1 , 446, 1 , 446),
  INST(Vpbroadcastq    , "vpbroadcastq"    , Enc(VexRm_Lx)          , V(660F38,59,_,x,0,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 447, 1 , 304),
  INST(Vpbroadcastw    , "vpbroadcastw"    , Enc(VexRm_Lx)          , V(660F38,79,_,x,0,0,1,T1S), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 327, 2 , 435),
  INST(Vpclmulqdq      , "vpclmulqdq"      , Enc(VexRvmi)           , V(660F3A,44,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 16, 8 , 164, 1 , 447),
  INST(Vpcmov          , "vpcmov"          , Enc(VexRvrmRvmr_Lx)    , V(XOP_M8,A2,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 0 , 0 , 119, 4 , 448),
  INST(Vpcmpb          , "vpcmpb"          , Enc(VexRvm_Lx)         , V(660F3A,3F,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 212, 3 , 449),
  INST(Vpcmpd          , "vpcmpd"          , Enc(VexRvm_Lx)         , V(660F3A,1F,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 212, 3 , 450),
  INST(Vpcmpeqb        , "vpcmpeqb"        , Enc(VexRvm_Lx)         , V(660F00,74,_,x,I,I,4,FV ), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,K_,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 215, 3 , 451),
  INST(Vpcmpeqd        , "vpcmpeqd"        , Enc(VexRvm_Lx)         , V(660F00,76,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 215, 3 , 452),
  INST(Vpcmpeqq        , "vpcmpeqq"        , Enc(VexRvm_Lx)         , V(660F38,29,_,x,I,1,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 215, 3 , 453),
  INST(Vpcmpeqw        , "vpcmpeqw"        , Enc(VexRvm_Lx)         , V(660F00,75,_,x,I,I,4,FV ), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,K_,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 215, 3 , 454),
  INST(Vpcmpestri      , "vpcmpestri"      , Enc(VexRmi)            , V(660F3A,61,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)|F(Special)               , EF(________), 0 , 0 , 0 , 0 , 397, 1 , 455),
  INST(Vpcmpestrm      , "vpcmpestrm"      , Enc(VexRmi)            , V(660F3A,60,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)|F(Special)               , EF(________), 0 , 0 , 0 , 0 , 398, 1 , 455),
  INST(Vpcmpgtb        , "vpcmpgtb"        , Enc(VexRvm_Lx)         , V(660F00,64,_,x,I,I,4,FV ), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,K_,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 215, 3 , 451),
  INST(Vpcmpgtd        , "vpcmpgtd"        , Enc(VexRvm_Lx)         , V(660F00,66,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 215, 3 , 452),
  INST(Vpcmpgtq        , "vpcmpgtq"        , Enc(VexRvm_Lx)         , V(660F38,37,_,x,I,1,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 215, 3 , 453),
  INST(Vpcmpgtw        , "vpcmpgtw"        , Enc(VexRvm_Lx)         , V(660F00,65,_,x,I,I,4,FV ), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,K_,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 215, 3 , 454),
  INST(Vpcmpistri      , "vpcmpistri"      , Enc(VexRmi)            , V(660F3A,63,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)|F(Special)               , EF(________), 0 , 0 , 0 , 0 , 399, 1 , 455),
  INST(Vpcmpistrm      , "vpcmpistrm"      , Enc(VexRmi)            , V(660F3A,62,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)|F(Special)               , EF(________), 0 , 0 , 0 , 0 , 400, 1 , 455),
  INST(Vpcmpq          , "vpcmpq"          , Enc(VexRvm_Lx)         , V(660F3A,1F,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 212, 3 , 456),
  INST(Vpcmpub         , "vpcmpub"         , Enc(VexRvm_Lx)         , V(660F3A,3E,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 212, 3 , 449),
  INST(Vpcmpud         , "vpcmpud"         , Enc(VexRvm_Lx)         , V(660F3A,1E,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 212, 3 , 450),
  INST(Vpcmpuq         , "vpcmpuq"         , Enc(VexRvm_Lx)         , V(660F3A,1E,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 212, 3 , 456),
  INST(Vpcmpuw         , "vpcmpuw"         , Enc(VexRvm_Lx)         , V(660F3A,3E,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 212, 3 , 457),
  INST(Vpcmpw          , "vpcmpw"          , Enc(VexRvm_Lx)         , V(660F3A,3F,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 212, 3 , 457),
  INST(Vpcomb          , "vpcomb"          , Enc(VexRvmi)           , V(XOP_M8,CC,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1 , 1 , 164, 1 , 458),
  INST(Vpcomd          , "vpcomd"          , Enc(VexRvmi)           , V(XOP_M8,CE,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 164, 1 , 459),
  INST(Vpcompressd     , "vpcompressd"     , Enc(VexMr_Lx)          , V(660F38,8B,_,x,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 170, 3 , 313),
  INST(Vpcompressq     , "vpcompressq"     , Enc(VexMr_Lx)          , V(660F38,8B,_,x,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 170, 3 , 312),
  INST(Vpcomq          , "vpcomq"          , Enc(VexRvmi)           , V(XOP_M8,CF,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 164, 1 , 460),
  INST(Vpcomub         , "vpcomub"         , Enc(VexRvmi)           , V(XOP_M8,EC,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1 , 1 , 164, 1 , 458),
  INST(Vpcomud         , "vpcomud"         , Enc(VexRvmi)           , V(XOP_M8,EE,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 164, 1 , 459),
  INST(Vpcomuq         , "vpcomuq"         , Enc(VexRvmi)           , V(XOP_M8,EF,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 164, 1 , 460),
  INST(Vpcomuw         , "vpcomuw"         , Enc(VexRvmi)           , V(XOP_M8,ED,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 164, 1 , 461),
  INST(Vpcomw          , "vpcomw"          , Enc(VexRvmi)           , V(XOP_M8,CD,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 164, 1 , 461),
  INST(Vpconflictd     , "vpconflictd"     , Enc(VexRm_Lx)          , V(660F38,C4,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(CD  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 176, 3 , 462),
  INST(Vpconflictq     , "vpconflictq"     , Enc(VexRm_Lx)          , V(660F38,C4,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(CD  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 176, 3 , 463),
  INST(Vperm2f128      , "vperm2f128"      , Enc(VexRvmi)           , V(660F3A,06,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 16, 16, 165, 1 , 388),
  INST(Vperm2i128      , "vperm2i128"      , Enc(VexRvmi)           , V(660F3A,46,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 16, 16, 165, 1 , 388),
  INST(Vpermb          , "vpermb"          , Enc(VexRvm_Lx)         , V(660F38,8D,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(VBMI,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 161, 3 , 464),
  INST(Vpermd          , "vpermd"          , Enc(VexRvm_Lx)         , V(660F38,36,_,x,0,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 136, 2 , 433),
  INST(Vpermi2b        , "vpermi2b"        , Enc(VexRvm_Lx)         , V(660F38,75,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(VBMI,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 161, 3 , 464),
  INST(Vpermi2d        , "vpermi2d"        , Enc(VexRvm_Lx)         , V(660F38,76,_,x,_,0,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 465),
  INST(Vpermi2pd       , "vpermi2pd"       , Enc(VexRvm_Lx)         , V(660F38,77,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 291),
  INST(Vpermi2ps       , "vpermi2ps"       , Enc(VexRvm_Lx)         , V(660F38,77,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 290),
  INST(Vpermi2q        , "vpermi2q"        , Enc(VexRvm_Lx)         , V(660F38,76,_,x,_,1,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 466),
  INST(Vpermi2w        , "vpermi2w"        , Enc(VexRvm_Lx)         , V(660F38,75,_,x,_,1,4,FVM), 0                         , F(RW)          |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 188, 3 , 467),
  INST(Vpermil2pd      , "vpermil2pd"      , Enc(VexRvrmiRvmri_Lx)  , V(660F3A,49,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 131, 4 , 468),
  INST(Vpermil2ps      , "vpermil2ps"      , Enc(VexRvrmiRvmri_Lx)  , V(660F3A,48,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 131, 4 , 469),
  INST(Vpermilpd       , "vpermilpd"       , Enc(VexRvmRmi_Lx)      , V(660F38,0D,_,x,0,1,4,FV ), V(660F3A,05,_,x,0,1,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 69 , 6 , 470),
  INST(Vpermilps       , "vpermilps"       , Enc(VexRvmRmi_Lx)      , V(660F38,0C,_,x,0,0,4,FV ), V(660F3A,04,_,x,0,0,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 69 , 6 , 471),
  INST(Vpermpd         , "vpermpd"         , Enc(VexRmi)            , V(660F3A,01,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 72 , 1 , 122),
  INST(Vpermps         , "vpermps"         , Enc(VexRvm)            , V(660F38,16,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 71 , 1 , 105),
  INST(Vpermq          , "vpermq"          , Enc(VexRvmRmi_Lx)      , V(660F38,36,_,x,_,1,4,FV ), V(660F3A,00,_,x,1,1,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 135, 4 , 472),
  INST(Vpermt2b        , "vpermt2b"        , Enc(VexRvm_Lx)         , V(660F38,7D,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(VBMI,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 161, 3 , 464),
  INST(Vpermt2d        , "vpermt2d"        , Enc(VexRvm_Lx)         , V(660F38,7E,_,x,_,0,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 465),
  INST(Vpermt2pd       , "vpermt2pd"       , Enc(VexRvm_Lx)         , V(660F38,7F,_,x,_,1,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 466),
  INST(Vpermt2ps       , "vpermt2ps"       , Enc(VexRvm_Lx)         , V(660F38,7F,_,x,_,0,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 188, 3 , 465),
  INST(Vpermt2q        , "vpermt2q"        , Enc(VexRvm_Lx)         , V(660F38,7E,_,x,_,1,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 188, 3 , 466),
  INST(Vpermt2w        , "vpermt2w"        , Enc(VexRvm_Lx)         , V(660F38,7D,_,x,_,1,4,FVM), 0                         , F(RW)          |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 188, 3 , 467),
  INST(Vpermw          , "vpermw"          , Enc(VexRvm_Lx)         , V(660F38,8D,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 292),
  INST(Vpexpandd       , "vpexpandd"       , Enc(VexRm_Lx)          , V(660F38,89,_,x,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 176, 3 , 351),
  INST(Vpexpandq       , "vpexpandq"       , Enc(VexRm_Lx)          , V(660F38,89,_,x,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 176, 3 , 350),
  INST(Vpextrb         , "vpextrb"         , Enc(VexMri)            , V(660F3A,14,_,0,0,I,0,T1S), 0                         , F(WO)|F(Vex)   |A512(BW  ,0,0 ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 401, 1 , 473),
  INST(Vpextrd         , "vpextrd"         , Enc(VexMri)            , V(660F3A,16,_,0,0,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(DQ  ,0,0 ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 370, 1 , 474),
  INST(Vpextrq         , "vpextrq"         , Enc(VexMri)            , V(660F3A,16,_,0,1,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(DQ  ,0,0 ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 402, 1 , 475),
  INST(Vpextrw         , "vpextrw"         , Enc(VexMri)            , V(660F3A,15,_,0,0,I,1,T1S), 0                         , F(WO)|F(Vex)   |A512(BW  ,0,0 ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 292, 1 , 476),
  INST(Vpgatherdd      , "vpgatherdd"      , Enc(VexRmvRm_VM)       , V(660F38,90,_,x,0,_,_,_  ), V(660F38,90,_,x,_,0,2,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 97 , 5 , 477),
  INST(Vpgatherdq      , "vpgatherdq"      , Enc(VexRmvRm_VM)       , V(660F38,90,_,x,1,_,_,_  ), V(660F38,90,_,x,_,1,3,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 92 , 5 , 478),
  INST(Vpgatherqd      , "vpgatherqd"      , Enc(VexRmvRm_VM)       , V(660F38,91,_,x,0,_,_,_  ), V(660F38,91,_,x,_,0,2,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 123, 4 , 479),
  INST(Vpgatherqq      , "vpgatherqq"      , Enc(VexRmvRm_VM)       , V(660F38,91,_,x,1,_,_,_  ), V(660F38,91,_,x,_,1,3,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 102, 5 , 480),
  INST(Vphaddbd        , "vphaddbd"        , Enc(VexRm)             , V(XOP_M9,C2,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 1 , 63 , 1 , 481),
  INST(Vphaddbq        , "vphaddbq"        , Enc(VexRm)             , V(XOP_M9,C3,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 1 , 63 , 1 , 482),
  INST(Vphaddbw        , "vphaddbw"        , Enc(VexRm)             , V(XOP_M9,C1,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 1 , 63 , 1 , 483),
  INST(Vphaddd         , "vphaddd"         , Enc(VexRvm_Lx)         , V(660F38,02,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 161, 2 , 282),
  INST(Vphadddq        , "vphadddq"        , Enc(VexRm)             , V(XOP_M9,CB,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 4 , 63 , 1 , 484),
  INST(Vphaddsw        , "vphaddsw"        , Enc(VexRvm_Lx)         , V(660F38,03,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 161, 2 , 485),
  INST(Vphaddubd       , "vphaddubd"       , Enc(VexRm)             , V(XOP_M9,D2,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 1 , 63 , 1 , 481),
  INST(Vphaddubq       , "vphaddubq"       , Enc(VexRm)             , V(XOP_M9,D3,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 1 , 63 , 1 , 482),
  INST(Vphaddubw       , "vphaddubw"       , Enc(VexRm)             , V(XOP_M9,D1,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 1 , 63 , 1 , 483),
  INST(Vphaddudq       , "vphaddudq"       , Enc(VexRm)             , V(XOP_M9,DB,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 4 , 63 , 1 , 484),
  INST(Vphadduwd       , "vphadduwd"       , Enc(VexRm)             , V(XOP_M9,D6,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 2 , 63 , 1 , 486),
  INST(Vphadduwq       , "vphadduwq"       , Enc(VexRm)             , V(XOP_M9,D7,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 2 , 63 , 1 , 487),
  INST(Vphaddw         , "vphaddw"         , Enc(VexRvm_Lx)         , V(660F38,01,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 161, 2 , 485),
  INST(Vphaddwd        , "vphaddwd"        , Enc(VexRm)             , V(XOP_M9,C6,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 2 , 63 , 1 , 486),
  INST(Vphaddwq        , "vphaddwq"        , Enc(VexRm)             , V(XOP_M9,C7,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 2 , 63 , 1 , 487),
  INST(Vphminposuw     , "vphminposuw"     , Enc(VexRm)             , V(660F38,41,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 0 , 2 , 63 , 1 , 488),
  INST(Vphsubbw        , "vphsubbw"        , Enc(VexRm)             , V(XOP_M9,E1,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 1 , 63 , 1 , 483),
  INST(Vphsubd         , "vphsubd"         , Enc(VexRvm_Lx)         , V(660F38,06,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 161, 2 , 282),
  INST(Vphsubdq        , "vphsubdq"        , Enc(VexRm)             , V(XOP_M9,E3,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 4 , 63 , 1 , 484),
  INST(Vphsubsw        , "vphsubsw"        , Enc(VexRvm_Lx)         , V(660F38,07,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 161, 2 , 485),
  INST(Vphsubw         , "vphsubw"         , Enc(VexRvm_Lx)         , V(660F38,05,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 161, 2 , 485),
  INST(Vphsubwd        , "vphsubwd"        , Enc(VexRm)             , V(XOP_M9,E2,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 2 , 63 , 1 , 486),
  INST(Vpinsrb         , "vpinsrb"         , Enc(VexRvmi)           , V(660F3A,20,_,0,0,I,0,T1S), 0                         , F(WO)|F(Vex)   |A512(BW  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 329, 2 , 489),
  INST(Vpinsrd         , "vpinsrd"         , Enc(VexRvmi)           , V(660F3A,22,_,0,0,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(DQ  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 331, 2 , 490),
  INST(Vpinsrq         , "vpinsrq"         , Enc(VexRvmi)           , V(660F3A,22,_,0,1,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(DQ  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 333, 2 , 491),
  INST(Vpinsrw         , "vpinsrw"         , Enc(VexRvmi)           , V(660F00,C4,_,0,0,I,1,T1S), 0                         , F(WO)|F(Vex)   |A512(BW  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 448, 1 , 492),
  INST(Vplzcntd        , "vplzcntd"        , Enc(VexRm_Lx)          , V(660F38,44,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(CD  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 176, 3 , 462),
  INST(Vplzcntq        , "vplzcntq"        , Enc(VexRm_Lx)          , V(660F38,44,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(CD  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 176, 3 , 463),
  INST(Vpmacsdd        , "vpmacsdd"        , Enc(VexRvmr)           , V(XOP_M8,9E,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 120, 1 , 493),
  INST(Vpmacsdqh       , "vpmacsdqh"       , Enc(VexRvmr)           , V(XOP_M8,9F,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 4 , 120, 1 , 494),
  INST(Vpmacsdql       , "vpmacsdql"       , Enc(VexRvmr)           , V(XOP_M8,97,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 4 , 120, 1 , 494),
  INST(Vpmacssdd       , "vpmacssdd"       , Enc(VexRvmr)           , V(XOP_M8,8E,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 120, 1 , 493),
  INST(Vpmacssdqh      , "vpmacssdqh"      , Enc(VexRvmr)           , V(XOP_M8,8F,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 4 , 120, 1 , 494),
  INST(Vpmacssdql      , "vpmacssdql"      , Enc(VexRvmr)           , V(XOP_M8,87,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 4 , 120, 1 , 494),
  INST(Vpmacsswd       , "vpmacsswd"       , Enc(VexRvmr)           , V(XOP_M8,86,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 2 , 120, 1 , 495),
  INST(Vpmacssww       , "vpmacssww"       , Enc(VexRvmr)           , V(XOP_M8,85,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 120, 1 , 496),
  INST(Vpmacswd        , "vpmacswd"        , Enc(VexRvmr)           , V(XOP_M8,96,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 2 , 120, 1 , 495),
  INST(Vpmacsww        , "vpmacsww"        , Enc(VexRvmr)           , V(XOP_M8,95,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 120, 1 , 496),
  INST(Vpmadcsswd      , "vpmadcsswd"      , Enc(VexRvmr)           , V(XOP_M8,A6,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 2 , 120, 1 , 495),
  INST(Vpmadcswd       , "vpmadcswd"       , Enc(VexRvmr)           , V(XOP_M8,B6,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 2 , 120, 1 , 495),
  INST(Vpmadd52huq     , "vpmadd52huq"     , Enc(VexRvm_Lx)         , V(660F38,B5,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(IFMA,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 497),
  INST(Vpmadd52luq     , "vpmadd52luq"     , Enc(VexRvm_Lx)         , V(660F38,B4,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(IFMA,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 497),
  INST(Vpmaddubsw      , "vpmaddubsw"      , Enc(VexRvm_Lx)         , V(660F38,04,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 1 , 161, 3 , 498),
  INST(Vpmaddwd        , "vpmaddwd"        , Enc(VexRvm_Lx)         , V(660F00,F5,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 2 , 161, 3 , 499),
  INST(Vpmaskmovd      , "vpmaskmovd"      , Enc(VexRvmMvr_Lx)      , V(660F38,8C,_,x,0,_,_,_  ), V(660F38,8E,_,x,0,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 0 , 4 , 127, 4 , 500),
  INST(Vpmaskmovq      , "vpmaskmovq"      , Enc(VexRvmMvr_Lx)      , V(660F38,8C,_,x,1,_,_,_  ), V(660F38,8E,_,x,1,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 0 , 8 , 127, 4 , 501),
  INST(Vpmaxsb         , "vpmaxsb"         , Enc(VexRvm_Lx)         , V(660F38,3C,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 161, 3 , 438),
  INST(Vpmaxsd         , "vpmaxsd"         , Enc(VexRvm_Lx)         , V(660F38,3D,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 433),
  INST(Vpmaxsq         , "vpmaxsq"         , Enc(VexRvm_Lx)         , V(660F38,3D,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 291),
  INST(Vpmaxsw         , "vpmaxsw"         , Enc(VexRvm_Lx)         , V(660F00,EE,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 440),
  INST(Vpmaxub         , "vpmaxub"         , Enc(VexRvm_Lx)         , V(660F00,DE,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 161, 3 , 438),
  INST(Vpmaxud         , "vpmaxud"         , Enc(VexRvm_Lx)         , V(660F38,3F,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 433),
  INST(Vpmaxuq         , "vpmaxuq"         , Enc(VexRvm_Lx)         , V(660F38,3F,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 291),
  INST(Vpmaxuw         , "vpmaxuw"         , Enc(VexRvm_Lx)         , V(660F38,3E,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 440),
  INST(Vpminsb         , "vpminsb"         , Enc(VexRvm_Lx)         , V(660F38,38,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 161, 3 , 438),
  INST(Vpminsd         , "vpminsd"         , Enc(VexRvm_Lx)         , V(660F38,39,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 433),
  INST(Vpminsq         , "vpminsq"         , Enc(VexRvm_Lx)         , V(660F38,39,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 291),
  INST(Vpminsw         , "vpminsw"         , Enc(VexRvm_Lx)         , V(660F00,EA,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 440),
  INST(Vpminub         , "vpminub"         , Enc(VexRvm_Lx)         , V(660F00,DA,_,x,I,_,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 161, 3 , 438),
  INST(Vpminud         , "vpminud"         , Enc(VexRvm_Lx)         , V(660F38,3B,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 433),
  INST(Vpminuq         , "vpminuq"         , Enc(VexRvm_Lx)         , V(660F38,3B,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 291),
  INST(Vpminuw         , "vpminuw"         , Enc(VexRvm_Lx)         , V(660F38,3A,_,x,I,_,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 440),
  INST(Vpmovb2m        , "vpmovb2m"        , Enc(VexRm_Lx)          , V(F30F38,29,_,x,_,0,_,_  ), 0                         , F(WO)          |A512(BW  ,1,0 ,0  ,B) , EF(________), 0 , 0 , 0 , 1 , 449, 1 , 502),
  INST(Vpmovd2m        , "vpmovd2m"        , Enc(VexRm_Lx)          , V(F30F38,39,_,x,_,0,_,_  ), 0                         , F(WO)          |A512(DQ  ,1,0 ,0  ,B) , EF(________), 0 , 0 , 0 , 4 , 449, 1 , 503),
  INST(Vpmovdb         , "vpmovdb"         , Enc(VexMr_Lx)          , V(F30F38,31,_,x,_,0,2,QVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 4 , 218, 3 , 504),
  INST(Vpmovdw         , "vpmovdw"         , Enc(VexMr_Lx)          , V(F30F38,33,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 4 , 221, 3 , 505),
  INST(Vpmovm2b        , "vpmovm2b"        , Enc(VexRm_Lx)          , V(F30F38,28,_,x,_,0,_,_  ), 0                         , F(WO)          |A512(BW  ,1,0 ,0  ,B) , EF(________), 0 , 0 , 0 , 2 , 446, 1 , 506),
  INST(Vpmovm2d        , "vpmovm2d"        , Enc(VexRm_Lx)          , V(F30F38,38,_,x,_,0,_,_  ), 0                         , F(WO)          |A512(DQ  ,1,0 ,0  ,B) , EF(________), 0 , 0 , 0 , 4 , 446, 1 , 503),
  INST(Vpmovm2q        , "vpmovm2q"        , Enc(VexRm_Lx)          , V(F30F38,38,_,x,_,1,_,_  ), 0                         , F(WO)          |A512(DQ  ,1,0 ,0  ,B) , EF(________), 0 , 0 , 0 , 8 , 446, 1 , 507),
  INST(Vpmovm2w        , "vpmovm2w"        , Enc(VexRm_Lx)          , V(F30F38,28,_,x,_,1,_,_  ), 0                         , F(WO)          |A512(BW  ,1,0 ,0  ,B) , EF(________), 0 , 0 , 0 , 2 , 446, 1 , 506),
  INST(Vpmovmskb       , "vpmovmskb"       , Enc(VexRm_Lx)          , V(660F00,D7,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 0 , 1 , 445, 1 , 508),
  INST(Vpmovq2m        , "vpmovq2m"        , Enc(VexRm_Lx)          , V(F30F38,39,_,x,_,1,_,_  ), 0                         , F(WO)          |A512(DQ  ,1,0 ,0  ,B) , EF(________), 0 , 0 , 0 , 8 , 449, 1 , 507),
  INST(Vpmovqb         , "vpmovqb"         , Enc(VexMr_Lx)          , V(F30F38,32,_,x,_,0,1,OVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 8 , 224, 3 , 509),
  INST(Vpmovqd         , "vpmovqd"         , Enc(VexMr_Lx)          , V(F30F38,35,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 8 , 221, 3 , 510),
  INST(Vpmovqw         , "vpmovqw"         , Enc(VexMr_Lx)          , V(F30F38,34,_,x,_,0,2,QVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 8 , 218, 3 , 511),
  INST(Vpmovsdb        , "vpmovsdb"        , Enc(VexMr_Lx)          , V(F30F38,21,_,x,_,0,2,QVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 4 , 218, 3 , 504),
  INST(Vpmovsdw        , "vpmovsdw"        , Enc(VexMr_Lx)          , V(F30F38,23,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 4 , 221, 3 , 505),
  INST(Vpmovsqb        , "vpmovsqb"        , Enc(VexMr_Lx)          , V(F30F38,22,_,x,_,0,1,OVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 8 , 224, 3 , 509),
  INST(Vpmovsqd        , "vpmovsqd"        , Enc(VexMr_Lx)          , V(F30F38,25,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 8 , 221, 3 , 510),
  INST(Vpmovsqw        , "vpmovsqw"        , Enc(VexMr_Lx)          , V(F30F38,24,_,x,_,0,2,QVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 8 , 218, 3 , 511),
  INST(Vpmovswb        , "vpmovswb"        , Enc(VexMr_Lx)          , V(F30F38,20,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 2 , 221, 3 , 512),
  INST(Vpmovsxbd       , "vpmovsxbd"       , Enc(VexRm_Lx)          , V(660F38,21,_,x,I,I,2,QVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 1 , 227, 3 , 513),
  INST(Vpmovsxbq       , "vpmovsxbq"       , Enc(VexRm_Lx)          , V(660F38,22,_,x,I,I,1,OVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 1 , 230, 3 , 514),
  INST(Vpmovsxbw       , "vpmovsxbw"       , Enc(VexRm_Lx)          , V(660F38,20,_,x,I,I,3,HVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 1 , 173, 3 , 515),
  INST(Vpmovsxdq       , "vpmovsxdq"       , Enc(VexRm_Lx)          , V(660F38,25,_,x,I,0,3,HVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 4 , 233, 3 , 314),
  INST(Vpmovsxwd       , "vpmovsxwd"       , Enc(VexRm_Lx)          , V(660F38,23,_,x,I,I,3,HVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 2 , 173, 3 , 516),
  INST(Vpmovsxwq       , "vpmovsxwq"       , Enc(VexRm_Lx)          , V(660F38,24,_,x,I,I,2,QVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 2 , 227, 3 , 517),
  INST(Vpmovusdb       , "vpmovusdb"       , Enc(VexMr_Lx)          , V(F30F38,11,_,x,_,0,2,QVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 4 , 218, 3 , 504),
  INST(Vpmovusdw       , "vpmovusdw"       , Enc(VexMr_Lx)          , V(F30F38,13,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 4 , 221, 3 , 505),
  INST(Vpmovusqb       , "vpmovusqb"       , Enc(VexMr_Lx)          , V(F30F38,12,_,x,_,0,1,OVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 8 , 224, 3 , 509),
  INST(Vpmovusqd       , "vpmovusqd"       , Enc(VexMr_Lx)          , V(F30F38,15,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 8 , 221, 3 , 510),
  INST(Vpmovusqw       , "vpmovusqw"       , Enc(VexMr_Lx)          , V(F30F38,14,_,x,_,0,2,QVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 8 , 218, 3 , 511),
  INST(Vpmovuswb       , "vpmovuswb"       , Enc(VexMr_Lx)          , V(F30F38,10,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 2 , 221, 3 , 512),
  INST(Vpmovw2m        , "vpmovw2m"        , Enc(VexRm_Lx)          , V(F30F38,29,_,x,_,1,_,_  ), 0                         , F(WO)          |A512(BW  ,1,0 ,0  ,B) , EF(________), 0 , 0 , 0 , 2 , 449, 1 , 506),
  INST(Vpmovwb         , "vpmovwb"         , Enc(VexMr_Lx)          , V(F30F38,30,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 2 , 221, 3 , 512),
  INST(Vpmovzxbd       , "vpmovzxbd"       , Enc(VexRm_Lx)          , V(660F38,31,_,x,I,I,2,QVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 1 , 227, 3 , 513),
  INST(Vpmovzxbq       , "vpmovzxbq"       , Enc(VexRm_Lx)          , V(660F38,32,_,x,I,I,1,OVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 1 , 230, 3 , 514),
  INST(Vpmovzxbw       , "vpmovzxbw"       , Enc(VexRm_Lx)          , V(660F38,30,_,x,I,I,3,HVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 1 , 173, 3 , 515),
  INST(Vpmovzxdq       , "vpmovzxdq"       , Enc(VexRm_Lx)          , V(660F38,35,_,x,I,0,3,HVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 4 , 233, 3 , 314),
  INST(Vpmovzxwd       , "vpmovzxwd"       , Enc(VexRm_Lx)          , V(660F38,33,_,x,I,I,3,HVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 2 , 173, 3 , 516),
  INST(Vpmovzxwq       , "vpmovzxwq"       , Enc(VexRm_Lx)          , V(660F38,34,_,x,I,I,2,QVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 2 , 227, 3 , 517),
  INST(Vpmuldq         , "vpmuldq"         , Enc(VexRvm_Lx)         , V(660F38,28,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 4 , 161, 3 , 518),
  INST(Vpmulhrsw       , "vpmulhrsw"       , Enc(VexRvm_Lx)         , V(660F38,0B,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 440),
  INST(Vpmulhuw        , "vpmulhuw"        , Enc(VexRvm_Lx)         , V(660F00,E4,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 440),
  INST(Vpmulhw         , "vpmulhw"         , Enc(VexRvm_Lx)         , V(660F00,E5,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 440),
  INST(Vpmulld         , "vpmulld"         , Enc(VexRvm_Lx)         , V(660F38,40,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 433),
  INST(Vpmullq         , "vpmullq"         , Enc(VexRvm_Lx)         , V(660F38,40,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 519),
  INST(Vpmullw         , "vpmullw"         , Enc(VexRvm_Lx)         , V(660F00,D5,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 440),
  INST(Vpmultishiftqb  , "vpmultishiftqb"  , Enc(VexRvm_Lx)         , V(660F38,83,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(VBMI,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 8 , 161, 3 , 520),
  INST(Vpmuludq        , "vpmuludq"        , Enc(VexRvm_Lx)         , V(660F00,F4,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 4 , 161, 3 , 518),
  INST(Vpor            , "vpor"            , Enc(VexRvm_Lx)         , V(660F00,EB,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 16, 16, 161, 2 , 442),
  INST(Vpord           , "vpord"           , Enc(VexRvm_Lx)         , V(660F00,EB,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 290),
  INST(Vporq           , "vporq"           , Enc(VexRvm_Lx)         , V(660F00,EB,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 291),
  INST(Vpperm          , "vpperm"          , Enc(VexRvrmRvmr)       , V(XOP_M8,A3,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1 , 1 , 119, 2 , 521),
  INST(Vprold          , "vprold"          , Enc(VexVmi_Lx)         , V(660F00,72,1,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 191, 3 , 522),
  INST(Vprolq          , "vprolq"          , Enc(VexVmi_Lx)         , V(660F00,72,1,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 191, 3 , 523),
  INST(Vprolvd         , "vprolvd"         , Enc(VexRvm_Lx)         , V(660F38,15,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 290),
  INST(Vprolvq         , "vprolvq"         , Enc(VexRvm_Lx)         , V(660F38,15,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 291),
  INST(Vprord          , "vprord"          , Enc(VexVmi_Lx)         , V(660F00,72,0,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 191, 3 , 522),
  INST(Vprorq          , "vprorq"          , Enc(VexVmi_Lx)         , V(660F00,72,0,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 191, 3 , 523),
  INST(Vprorvd         , "vprorvd"         , Enc(VexRvm_Lx)         , V(660F38,14,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 290),
  INST(Vprorvq         , "vprorvq"         , Enc(VexRvm_Lx)         , V(660F38,14,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 291),
  INST(Vprotb          , "vprotb"          , Enc(VexRvmRmvRmi)      , V(XOP_M9,90,_,0,x,_,_,_  ), V(XOP_M8,C0,_,0,x,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1 , 1 , 335, 2 , 524),
  INST(Vprotd          , "vprotd"          , Enc(VexRvmRmvRmi)      , V(XOP_M9,92,_,0,x,_,_,_  ), V(XOP_M8,C2,_,0,x,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 335, 2 , 525),
  INST(Vprotq          , "vprotq"          , Enc(VexRvmRmvRmi)      , V(XOP_M9,93,_,0,x,_,_,_  ), V(XOP_M8,C3,_,0,x,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 335, 2 , 526),
  INST(Vprotw          , "vprotw"          , Enc(VexRvmRmvRmi)      , V(XOP_M9,91,_,0,x,_,_,_  ), V(XOP_M8,C1,_,0,x,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 335, 2 , 527),
  INST(Vpsadbw         , "vpsadbw"         , Enc(VexRvm_Lx)         , V(660F00,F6,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,0 ,0  ,B) , EF(________), 0 , 0 , 2 , 1 , 161, 3 , 528),
  INST(Vpscatterdd     , "vpscatterdd"     , Enc(VexMr_VM)          , V(660F38,A0,_,x,_,0,2,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 236, 3 , 529),
  INST(Vpscatterdq     , "vpscatterdq"     , Enc(VexMr_VM)          , V(660F38,A0,_,x,_,1,3,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 236, 3 , 530),
  INST(Vpscatterqd     , "vpscatterqd"     , Enc(VexMr_VM)          , V(660F38,A1,_,x,_,0,2,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 337, 2 , 529),
  INST(Vpscatterqq     , "vpscatterqq"     , Enc(VexMr_VM)          , V(660F38,A1,_,x,_,1,3,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 239, 3 , 530),
  INST(Vpshab          , "vpshab"          , Enc(VexRvmRmv)         , V(XOP_M9,98,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1 , 1 , 339, 2 , 531),
  INST(Vpshad          , "vpshad"          , Enc(VexRvmRmv)         , V(XOP_M9,9A,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 339, 2 , 532),
  INST(Vpshaq          , "vpshaq"          , Enc(VexRvmRmv)         , V(XOP_M9,9B,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 339, 2 , 533),
  INST(Vpshaw          , "vpshaw"          , Enc(VexRvmRmv)         , V(XOP_M9,99,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 339, 2 , 534),
  INST(Vpshlb          , "vpshlb"          , Enc(VexRvmRmv)         , V(XOP_M9,94,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1 , 1 , 339, 2 , 531),
  INST(Vpshld          , "vpshld"          , Enc(VexRvmRmv)         , V(XOP_M9,96,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 339, 2 , 532),
  INST(Vpshlq          , "vpshlq"          , Enc(VexRvmRmv)         , V(XOP_M9,97,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 339, 2 , 533),
  INST(Vpshlw          , "vpshlw"          , Enc(VexRvmRmv)         , V(XOP_M9,95,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 339, 2 , 534),
  INST(Vpshufb         , "vpshufb"         , Enc(VexRvm_Lx)         , V(660F38,00,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 161, 3 , 438),
  INST(Vpshufd         , "vpshufd"         , Enc(VexRmi_Lx)         , V(660F00,70,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 191, 3 , 535),
  INST(Vpshufhw        , "vpshufhw"        , Enc(VexRmi_Lx)         , V(F30F00,70,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 191, 3 , 536),
  INST(Vpshuflw        , "vpshuflw"        , Enc(VexRmi_Lx)         , V(F20F00,70,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 191, 3 , 536),
  INST(Vpsignb         , "vpsignb"         , Enc(VexRvm_Lx)         , V(660F38,08,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1 , 1 , 161, 2 , 537),
  INST(Vpsignd         , "vpsignd"         , Enc(VexRvm_Lx)         , V(660F38,0A,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 161, 2 , 282),
  INST(Vpsignw         , "vpsignw"         , Enc(VexRvm_Lx)         , V(660F38,09,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2 , 2 , 161, 2 , 485),
  INST(Vpslld          , "vpslld"          , Enc(VexRvmVmi_Lx)      , V(660F00,F2,_,x,I,0,4,128), V(660F00,72,6,x,I,0,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 75 , 6 , 538),
  INST(Vpslldq         , "vpslldq"         , Enc(VexVmi_VexEvex_Lx) , V(660F00,73,7,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,0 ,0  ,B) , EF(________), 0 , 0 , 16, 16, 191, 3 , 539),
  INST(Vpsllq          , "vpsllq"          , Enc(VexRvmVmi_Lx)      , V(660F00,F3,_,x,I,1,4,128), V(660F00,73,6,x,I,1,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 75 , 6 , 540),
  INST(Vpsllvd         , "vpsllvd"         , Enc(VexRvm_Lx)         , V(660F38,47,_,x,0,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 433),
  INST(Vpsllvq         , "vpsllvq"         , Enc(VexRvm_Lx)         , V(660F38,47,_,x,1,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 439),
  INST(Vpsllvw         , "vpsllvw"         , Enc(VexRvm_Lx)         , V(660F38,12,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 0 , 0 , 161, 3 , 541),
  INST(Vpsllw          , "vpsllw"          , Enc(VexRvmVmi_Lx)      , V(660F00,F1,_,x,I,I,4,FVM), V(660F00,71,6,x,I,I,4,FVM), F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 75 , 6 , 542),
  INST(Vpsrad          , "vpsrad"          , Enc(VexRvmVmi_Lx)      , V(660F00,E2,_,x,I,0,4,128), V(660F00,72,4,x,I,0,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 75 , 6 , 543),
  INST(Vpsraq          , "vpsraq"          , Enc(VexRvmVmi_Lx)      , V(660F00,E2,_,x,_,1,4,128), V(660F00,72,4,x,_,1,4,FV ), F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 81 , 6 , 544),
  INST(Vpsravd         , "vpsravd"         , Enc(VexRvm_Lx)         , V(660F38,46,_,x,0,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 433),
  INST(Vpsravq         , "vpsravq"         , Enc(VexRvm_Lx)         , V(660F38,46,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 291),
  INST(Vpsravw         , "vpsravw"         , Enc(VexRvm_Lx)         , V(660F38,11,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 292),
  INST(Vpsraw          , "vpsraw"          , Enc(VexRvmVmi_Lx)      , V(660F00,E1,_,x,I,I,4,128), V(660F00,71,4,x,I,I,4,FVM), F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 75 , 6 , 545),
  INST(Vpsrld          , "vpsrld"          , Enc(VexRvmVmi_Lx)      , V(660F00,D2,_,x,I,0,4,128), V(660F00,72,2,x,I,0,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 75 , 6 , 546),
  INST(Vpsrldq         , "vpsrldq"         , Enc(VexVmi_VexEvex_Lx) , V(660F00,73,3,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,0 ,0  ,B) , EF(________), 0 , 0 , 16, 16, 191, 3 , 539),
  INST(Vpsrlq          , "vpsrlq"          , Enc(VexRvmVmi_Lx)      , V(660F00,D3,_,x,I,1,4,128), V(660F00,73,2,x,I,1,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 75 , 6 , 547),
  INST(Vpsrlvd         , "vpsrlvd"         , Enc(VexRvm_Lx)         , V(660F38,45,_,x,0,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 433),
  INST(Vpsrlvq         , "vpsrlvq"         , Enc(VexRvm_Lx)         , V(660F38,45,_,x,1,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 439),
  INST(Vpsrlvw         , "vpsrlvw"         , Enc(VexRvm_Lx)         , V(660F38,10,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 292),
  INST(Vpsrlw          , "vpsrlw"          , Enc(VexRvmVmi_Lx)      , V(660F00,D1,_,x,I,I,4,128), V(660F00,71,2,x,I,I,4,FVM), F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 75 , 6 , 548),
  INST(Vpsubb          , "vpsubb"          , Enc(VexRvm_Lx)         , V(660F00,F8,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 161, 3 , 438),
  INST(Vpsubd          , "vpsubd"          , Enc(VexRvm_Lx)         , V(660F00,FA,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 433),
  INST(Vpsubq          , "vpsubq"          , Enc(VexRvm_Lx)         , V(660F00,FB,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 439),
  INST(Vpsubsb         , "vpsubsb"         , Enc(VexRvm_Lx)         , V(660F00,E8,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 161, 3 , 438),
  INST(Vpsubsw         , "vpsubsw"         , Enc(VexRvm_Lx)         , V(660F00,E9,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 440),
  INST(Vpsubusb        , "vpsubusb"        , Enc(VexRvm_Lx)         , V(660F00,D8,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 1 , 1 , 161, 3 , 438),
  INST(Vpsubusw        , "vpsubusw"        , Enc(VexRvm_Lx)         , V(660F00,D9,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 440),
  INST(Vpsubw          , "vpsubw"          , Enc(VexRvm_Lx)         , V(660F00,F9,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 2 , 161, 3 , 440),
  INST(Vpternlogd      , "vpternlogd"      , Enc(VexRvmi_Lx)        , V(660F3A,25,_,x,_,0,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 185, 3 , 549),
  INST(Vpternlogq      , "vpternlogq"      , Enc(VexRvmi_Lx)        , V(660F3A,25,_,x,_,1,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 185, 3 , 550),
  INST(Vptest          , "vptest"          , Enc(VexRm_Lx)          , V(660F38,17,_,x,I,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 0 , 0 , 341, 2 , 551),
  INST(Vptestmb        , "vptestmb"        , Enc(VexRvm_Lx)         , V(660F38,26,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,B) , EF(________), 0 , 0 , 0 , 1 , 242, 3 , 552),
  INST(Vptestmd        , "vptestmd"        , Enc(VexRvm_Lx)         , V(660F38,27,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 0 , 4 , 242, 3 , 553),
  INST(Vptestmq        , "vptestmq"        , Enc(VexRvm_Lx)         , V(660F38,27,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 0 , 8 , 242, 3 , 554),
  INST(Vptestmw        , "vptestmw"        , Enc(VexRvm_Lx)         , V(660F38,26,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,B) , EF(________), 0 , 0 , 0 , 2 , 242, 3 , 555),
  INST(Vptestnmb       , "vptestnmb"       , Enc(VexRvm_Lx)         , V(F30F38,26,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,B) , EF(________), 0 , 0 , 0 , 1 , 242, 3 , 552),
  INST(Vptestnmd       , "vptestnmd"       , Enc(VexRvm_Lx)         , V(F30F38,27,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 0 , 4 , 242, 3 , 553),
  INST(Vptestnmq       , "vptestnmq"       , Enc(VexRvm_Lx)         , V(F30F38,27,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 0 , 8 , 242, 3 , 554),
  INST(Vptestnmw       , "vptestnmw"       , Enc(VexRvm_Lx)         , V(F30F38,26,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,B) , EF(________), 0 , 0 , 0 , 2 , 242, 3 , 555),
  INST(Vpunpckhbw      , "vpunpckhbw"      , Enc(VexRvm_Lx)         , V(660F00,68,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 1 , 161, 3 , 498),
  INST(Vpunpckhdq      , "vpunpckhdq"      , Enc(VexRvm_Lx)         , V(660F00,6A,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 4 , 161, 3 , 518),
  INST(Vpunpckhqdq     , "vpunpckhqdq"     , Enc(VexRvm_Lx)         , V(660F00,6D,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 16, 8 , 161, 3 , 556),
  INST(Vpunpckhwd      , "vpunpckhwd"      , Enc(VexRvm_Lx)         , V(660F00,69,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 2 , 161, 3 , 499),
  INST(Vpunpcklbw      , "vpunpcklbw"      , Enc(VexRvm_Lx)         , V(660F00,60,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 2 , 1 , 161, 3 , 498),
  INST(Vpunpckldq      , "vpunpckldq"      , Enc(VexRvm_Lx)         , V(660F00,62,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 4 , 161, 3 , 518),
  INST(Vpunpcklqdq     , "vpunpcklqdq"     , Enc(VexRvm_Lx)         , V(660F00,6C,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 16, 8 , 161, 3 , 556),
  INST(Vpunpcklwd      , "vpunpcklwd"      , Enc(VexRvm_Lx)         , V(660F00,61,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 2 , 161, 3 , 499),
  INST(Vpxor           , "vpxor"           , Enc(VexRvm_Lx)         , V(660F00,EF,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 16, 16, 161, 2 , 442),
  INST(Vpxord          , "vpxord"          , Enc(VexRvm_Lx)         , V(660F00,EF,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 290),
  INST(Vpxorq          , "vpxorq"          , Enc(VexRvm_Lx)         , V(660F00,EF,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 291),
  INST(Vrangepd        , "vrangepd"        , Enc(VexRvmi_Lx)        , V(660F3A,50,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 164, 3 , 557),
  INST(Vrangeps        , "vrangeps"        , Enc(VexRvmi_Lx)        , V(660F3A,50,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 164, 3 , 558),
  INST(Vrangesd        , "vrangesd"        , Enc(VexRvmi)           , V(660F3A,51,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(DQ  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 450, 1 , 559),
  INST(Vrangess        , "vrangess"        , Enc(VexRvmi)           , V(660F3A,51,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(DQ  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 443, 1 , 560),
  INST(Vrcp14pd        , "vrcp14pd"        , Enc(VexRm_Lx)          , V(660F38,4C,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 176, 3 , 350),
  INST(Vrcp14ps        , "vrcp14ps"        , Enc(VexRm_Lx)          , V(660F38,4C,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 176, 3 , 351),
  INST(Vrcp14sd        , "vrcp14sd"        , Enc(VexRvm)            , V(660F38,4D,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 421, 1 , 561),
  INST(Vrcp14ss        , "vrcp14ss"        , Enc(VexRvm)            , V(660F38,4D,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 422, 1 , 562),
  INST(Vrcp28pd        , "vrcp28pd"        , Enc(VexRm)             , V(660F38,CA,_,2,_,1,4,FV ), 0                         , F(WO)          |A512(ER  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 67 , 1 , 348),
  INST(Vrcp28ps        , "vrcp28ps"        , Enc(VexRm)             , V(660F38,CA,_,2,_,0,4,FV ), 0                         , F(WO)          |A512(ER  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 67 , 1 , 349),
  INST(Vrcp28sd        , "vrcp28sd"        , Enc(VexRvm)            , V(660F38,CB,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(ER  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 421, 1 , 563),
  INST(Vrcp28ss        , "vrcp28ss"        , Enc(VexRvm)            , V(660F38,CB,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(ER  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 422, 1 , 564),
  INST(Vrcpps          , "vrcpps"          , Enc(VexRm_Lx)          , V(000F00,53,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 176, 2 , 375),
  INST(Vrcpss          , "vrcpss"          , Enc(VexRvm)            , V(F30F00,53,_,I,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 422, 1 , 105),
  INST(Vreducepd       , "vreducepd"       , Enc(VexRmi_Lx)         , V(660F3A,56,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 191, 3 , 565),
  INST(Vreduceps       , "vreduceps"       , Enc(VexRmi_Lx)         , V(660F3A,56,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 191, 3 , 566),
  INST(Vreducesd       , "vreducesd"       , Enc(VexRvmi)           , V(660F3A,57,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 450, 1 , 567),
  INST(Vreducess       , "vreducess"       , Enc(VexRvmi)           , V(660F3A,57,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 443, 1 , 568),
  INST(Vrndscalepd     , "vrndscalepd"     , Enc(VexRmi_Lx)         , V(660F3A,09,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 191, 3 , 384),
  INST(Vrndscaleps     , "vrndscaleps"     , Enc(VexRmi_Lx)         , V(660F3A,08,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 191, 3 , 385),
  INST(Vrndscalesd     , "vrndscalesd"     , Enc(VexRvmi)           , V(660F3A,0B,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 450, 1 , 569),
  INST(Vrndscaless     , "vrndscaless"     , Enc(VexRvmi)           , V(660F3A,0A,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 443, 1 , 570),
  INST(Vroundpd        , "vroundpd"        , Enc(VexRmi_Lx)         , V(660F3A,09,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 77 , 2 , 571),
  INST(Vroundps        , "vroundps"        , Enc(VexRmi_Lx)         , V(660F3A,08,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 77 , 2 , 572),
  INST(Vroundsd        , "vroundsd"        , Enc(VexRvmi)           , V(660F3A,0B,_,I,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8 , 8 , 450, 1 , 460),
  INST(Vroundss        , "vroundss"        , Enc(VexRvmi)           , V(660F3A,0A,_,I,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 443, 1 , 459),
  INST(Vrsqrt14pd      , "vrsqrt14pd"      , Enc(VexRm_Lx)          , V(660F38,4E,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 176, 3 , 350),
  INST(Vrsqrt14ps      , "vrsqrt14ps"      , Enc(VexRm_Lx)          , V(660F38,4E,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 176, 3 , 351),
  INST(Vrsqrt14sd      , "vrsqrt14sd"      , Enc(VexRvm)            , V(660F38,4F,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 421, 1 , 561),
  INST(Vrsqrt14ss      , "vrsqrt14ss"      , Enc(VexRvm)            , V(660F38,4F,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 422, 1 , 562),
  INST(Vrsqrt28pd      , "vrsqrt28pd"      , Enc(VexRm)             , V(660F38,CC,_,2,_,1,4,FV ), 0                         , F(WO)          |A512(ER  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 67 , 1 , 348),
  INST(Vrsqrt28ps      , "vrsqrt28ps"      , Enc(VexRm)             , V(660F38,CC,_,2,_,0,4,FV ), 0                         , F(WO)          |A512(ER  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 67 , 1 , 349),
  INST(Vrsqrt28sd      , "vrsqrt28sd"      , Enc(VexRvm)            , V(660F38,CD,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(ER  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 8 , 8 , 421, 1 , 563),
  INST(Vrsqrt28ss      , "vrsqrt28ss"      , Enc(VexRvm)            , V(660F38,CD,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(ER  ,0,KZ,SAE,B) , EF(________), 0 , 0 , 4 , 4 , 422, 1 , 564),
  INST(Vrsqrtps        , "vrsqrtps"        , Enc(VexRm_Lx)          , V(000F00,52,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 176, 2 , 375),
  INST(Vrsqrtss        , "vrsqrtss"        , Enc(VexRvm)            , V(F30F00,52,_,I,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4 , 4 , 422, 1 , 105),
  INST(Vscalefpd       , "vscalefpd"       , Enc(VexRvm_Lx)         , V(660F38,2C,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 573),
  INST(Vscalefps       , "vscalefps"       , Enc(VexRvm_Lx)         , V(660F38,2C,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 574),
  INST(Vscalefsd       , "vscalefsd"       , Enc(VexRvm)            , V(660F38,2D,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 421, 1 , 575),
  INST(Vscalefss       , "vscalefss"       , Enc(VexRvm)            , V(660F38,2D,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 422, 1 , 576),
  INST(Vscatterdpd     , "vscatterdpd"     , Enc(VexMr_Lx)          , V(660F38,A2,_,x,_,1,3,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 343, 2 , 577),
  INST(Vscatterdps     , "vscatterdps"     , Enc(VexMr_Lx)          , V(660F38,A2,_,x,_,0,2,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 236, 3 , 578),
  INST(Vscatterpf0dpd  , "vscatterpf0dpd"  , Enc(VexM_VM)           , V(660F38,C6,5,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PF  ,0,K_,0  ,B) , EF(________), 0 , 0 , 0 , 8 , 439, 1 , 378),
  INST(Vscatterpf0dps  , "vscatterpf0dps"  , Enc(VexM_VM)           , V(660F38,C6,5,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PF  ,0,K_,0  ,B) , EF(________), 0 , 0 , 0 , 4 , 440, 1 , 379),
  INST(Vscatterpf0qpd  , "vscatterpf0qpd"  , Enc(VexM_VM)           , V(660F38,C7,5,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PF  ,0,K_,0  ,B) , EF(________), 0 , 0 , 0 , 8 , 441, 1 , 378),
  INST(Vscatterpf0qps  , "vscatterpf0qps"  , Enc(VexM_VM)           , V(660F38,C7,5,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PF  ,0,K_,0  ,B) , EF(________), 0 , 0 , 0 , 4 , 441, 1 , 379),
  INST(Vscatterpf1dpd  , "vscatterpf1dpd"  , Enc(VexM_VM)           , V(660F38,C6,6,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PF  ,0,K_,0  ,B) , EF(________), 0 , 0 , 0 , 8 , 439, 1 , 378),
  INST(Vscatterpf1dps  , "vscatterpf1dps"  , Enc(VexM_VM)           , V(660F38,C6,6,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PF  ,0,K_,0  ,B) , EF(________), 0 , 0 , 0 , 4 , 440, 1 , 379),
  INST(Vscatterpf1qpd  , "vscatterpf1qpd"  , Enc(VexM_VM)           , V(660F38,C7,6,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PF  ,0,K_,0  ,B) , EF(________), 0 , 0 , 0 , 8 , 441, 1 , 378),
  INST(Vscatterpf1qps  , "vscatterpf1qps"  , Enc(VexM_VM)           , V(660F38,C7,6,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PF  ,0,K_,0  ,B) , EF(________), 0 , 0 , 0 , 4 , 441, 1 , 379),
  INST(Vscatterqpd     , "vscatterqpd"     , Enc(VexMr_Lx)          , V(660F38,A3,_,x,_,1,3,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 239, 3 , 577),
  INST(Vscatterqps     , "vscatterqps"     , Enc(VexMr_Lx)          , V(660F38,A3,_,x,_,0,2,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 337, 2 , 578),
  INST(Vshuff32x4      , "vshuff32x4"      , Enc(VexRvmi_Lx)        , V(660F3A,23,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 16, 16, 165, 2 , 389),
  INST(Vshuff64x2      , "vshuff64x2"      , Enc(VexRvmi_Lx)        , V(660F3A,23,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 16, 16, 165, 2 , 389),
  INST(Vshufi32x4      , "vshufi32x4"      , Enc(VexRvmi_Lx)        , V(660F3A,43,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 16, 16, 165, 2 , 389),
  INST(Vshufi64x2      , "vshufi64x2"      , Enc(VexRvmi_Lx)        , V(660F3A,43,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 16, 16, 165, 2 , 389),
  INST(Vshufpd         , "vshufpd"         , Enc(VexRvmi_Lx)        , V(660F00,C6,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 164, 3 , 579),
  INST(Vshufps         , "vshufps"         , Enc(VexRvmi_Lx)        , V(000F00,C6,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 164, 3 , 580),
  INST(Vsqrtpd         , "vsqrtpd"         , Enc(VexRm_Lx)          , V(660F00,51,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 176, 3 , 581),
  INST(Vsqrtps         , "vsqrtps"         , Enc(VexRm_Lx)          , V(000F00,51,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 176, 3 , 315),
  INST(Vsqrtsd         , "vsqrtsd"         , Enc(VexRvm)            , V(F20F00,51,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 421, 1 , 279),
  INST(Vsqrtss         , "vsqrtss"         , Enc(VexRvm)            , V(F30F00,51,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 422, 1 , 280),
  INST(Vstmxcsr        , "vstmxcsr"        , Enc(VexM)              , V(000F00,AE,3,0,I,_,_,_  ), 0                         , F(Vex)|F(Volatile)                    , EF(________), 0 , 0 , 0 , 0 , 420, 1 , 582),
  INST(Vsubpd          , "vsubpd"          , Enc(VexRvm_Lx)         , V(660F00,5C,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 277),
  INST(Vsubps          , "vsubps"          , Enc(VexRvm_Lx)         , V(000F00,5C,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 278),
  INST(Vsubsd          , "vsubsd"          , Enc(VexRvm)            , V(F20F00,5C,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 8 , 8 , 421, 1 , 279),
  INST(Vsubss          , "vsubss"          , Enc(VexRvm)            , V(F30F00,5C,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , EF(________), 0 , 0 , 4 , 4 , 422, 1 , 280),
  INST(Vtestpd         , "vtestpd"         , Enc(VexRm_Lx)          , V(660F38,0F,_,x,0,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 8 , 8 , 341, 2 , 583),
  INST(Vtestps         , "vtestps"         , Enc(VexRm_Lx)          , V(660F38,0E,_,x,0,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 4 , 4 , 341, 2 , 584),
  INST(Vucomisd        , "vucomisd"        , Enc(VexRm)             , V(660F00,2E,_,I,I,1,3,T1S), 0                         , F(RO)|F(Vex)   |A512(F_  ,0,0 ,SAE,B) , EF(WWWWWW__), 0 , 0 , 8 , 8 , 357, 1 , 310),
  INST(Vucomiss        , "vucomiss"        , Enc(VexRm)             , V(000F00,2E,_,I,I,0,2,T1S), 0                         , F(RO)|F(Vex)   |A512(F_  ,0,0 ,SAE,B) , EF(WWWWWW__), 0 , 0 , 4 , 4 , 358, 1 , 311),
  INST(Vunpckhpd       , "vunpckhpd"       , Enc(VexRvm_Lx)         , V(660F00,15,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 439),
  INST(Vunpckhps       , "vunpckhps"       , Enc(VexRvm_Lx)         , V(000F00,15,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 433),
  INST(Vunpcklpd       , "vunpcklpd"       , Enc(VexRvm_Lx)         , V(660F00,14,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 439),
  INST(Vunpcklps       , "vunpcklps"       , Enc(VexRvm_Lx)         , V(000F00,14,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 433),
  INST(Vxorpd          , "vxorpd"          , Enc(VexRvm_Lx)         , V(660F00,57,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 8 , 8 , 161, 3 , 287),
  INST(Vxorps          , "vxorps"          , Enc(VexRvm_Lx)         , V(000F00,57,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,B) , EF(________), 0 , 0 , 4 , 4 , 161, 3 , 288),
  INST(Vzeroall        , "vzeroall"        , Enc(VexOp)             , V(000F00,77,_,1,I,_,_,_  ), 0                         , F(Vex)|F(Volatile)                    , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 585),
  INST(Vzeroupper      , "vzeroupper"      , Enc(VexOp)             , V(000F00,77,_,0,I,_,_,_  ), 0                         , F(Vex)|F(Volatile)                    , EF(________), 0 , 0 , 0 , 0 , 259, 1 , 585),
  INST(Wrfsbase        , "wrfsbase"        , Enc(X86M)              , O(F30F00,AE,2,_,x,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , 0 , 0 , 451, 1 , 586),
  INST(Wrgsbase        , "wrgsbase"        , Enc(X86M)              , O(F30F00,AE,3,_,x,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , 0 , 0 , 451, 1 , 586),
  INST(Xadd            , "xadd"            , Enc(X86Xadd)           , O(000F00,C0,_,_,x,_,_,_  ), 0                         , F(RW)|F(Xchg)|F(Lock)                 , EF(WWWWWW__), 0 , 0 , 0 , 0 , 139, 4 , 587),
  INST(Xchg            , "xchg"            , Enc(X86Xchg)           , O(000000,86,_,_,x,_,_,_  ), 0                         , F(RW)|F(Xchg)|F(Lock)                 , EF(________), 0 , 0 , 0 , 0 , 43 , 8 , 588),
  INST(Xgetbv          , "xgetbv"          , Enc(X86Op)             , O(000F01,D0,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , 0 , 0 , 452, 1 , 167),
  INST(Xor             , "xor"             , Enc(X86Arith)          , O(000000,30,6,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWUWW__), 0 , 0 , 0 , 0 , 13 , 10, 3  ),
  INST(Xorpd           , "xorpd"           , Enc(ExtRm)             , O(660F00,57,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8 , 8 , 288, 1 , 4  ),
  INST(Xorps           , "xorps"           , Enc(ExtRm)             , O(000F00,57,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4 , 4 , 288, 1 , 5  ),
  INST(Xrstor          , "xrstor"          , Enc(X86M_Only)         , O(000F00,AE,5,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 453, 1 , 589),
  INST(Xrstor64        , "xrstor64"        , Enc(X86M_Only)         , O(000F00,AE,5,_,1,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 454, 1 , 589),
  INST(Xrstors         , "xrstors"         , Enc(X86M_Only)         , O(000F00,C7,3,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 453, 1 , 589),
  INST(Xrstors64       , "xrstors64"       , Enc(X86M_Only)         , O(000F00,C7,3,_,1,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 454, 1 , 589),
  INST(Xsave           , "xsave"           , Enc(X86M_Only)         , O(000F00,AE,4,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 453, 1 , 590),
  INST(Xsave64         , "xsave64"         , Enc(X86M_Only)         , O(000F00,AE,4,_,1,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 454, 1 , 590),
  INST(Xsavec          , "xsavec"          , Enc(X86M_Only)         , O(000F00,C7,4,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 453, 1 , 590),
  INST(Xsavec64        , "xsavec64"        , Enc(X86M_Only)         , O(000F00,C7,4,_,1,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 454, 1 , 590),
  INST(Xsaveopt        , "xsaveopt"        , Enc(X86M_Only)         , O(000F00,AE,6,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 453, 1 , 590),
  INST(Xsaveopt64      , "xsaveopt64"      , Enc(X86M_Only)         , O(000F00,AE,6,_,1,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 454, 1 , 590),
  INST(Xsaves          , "xsaves"          , Enc(X86M_Only)         , O(000F00,C7,5,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 453, 1 , 590),
  INST(Xsaves64        , "xsaves64"        , Enc(X86M_Only)         , O(000F00,C7,5,_,1,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 454, 1 , 590),
  INST(Xsetbv          , "xsetbv"          , Enc(X86Op)             , O(000F01,D1,_,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 0 , 0 , 455, 1 , 139)
// ${X86InstData:End}
};

// ${X86InstExtendedData:Begin}
// ------------------- Automatically generated, do not edit -------------------
const X86Inst::ExtendedData _x86InstExtendedData[] = {
  { Enc(None)               , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, 0                                     , 0                          },
  { Enc(X86Arith)           , 0  , 0  , 0  , 0  , 0x20, 0x3F, 0, F(RW)|F(Lock)                         , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0  , 0  , 0x20, 0x20, 0, F(RW)                                 , 0                          },
  { Enc(X86Arith)           , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(RW)|F(Lock)                         , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0  , 0  , 0x01, 0x01, 0, F(RW)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(VexRvm_Wx)          , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(RW)                                 , 0                          },
  { Enc(VexRmv_Wx)          , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(RW)                                 , 0                          },
  { Enc(VexVm_Wx)           , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(WO)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRmXMM0)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)|F(Special)                      , 0                          },
  { Enc(ExtRmXMM0)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)|F(Special)                      , 0                          },
  { Enc(VexVm_Wx)           , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(RW)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(RW)                                 , 0                          },
  { Enc(X86Bswap)           , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(X86Bt)              , 0  , 0  , 0  , 0  , 0x00, 0x3B, 0, F(RO)                                 , O(000F00,BA,4,_,x,_,_,_  ) },
  { Enc(X86Bt)              , 0  , 0  , 0  , 0  , 0x00, 0x3B, 0, F(RW)|F(Lock)                         , O(000F00,BA,7,_,x,_,_,_  ) },
  { Enc(X86Bt)              , 0  , 0  , 0  , 0  , 0x00, 0x3B, 0, F(RW)|F(Lock)                         , O(000F00,BA,6,_,x,_,_,_  ) },
  { Enc(X86Bt)              , 0  , 0  , 0  , 0  , 0x00, 0x3B, 0, F(RW)|F(Lock)                         , O(000F00,BA,5,_,x,_,_,_  ) },
  { Enc(X86Call)            , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RW)|F(Flow)|F(Volatile)             , O(000000,E8,_,_,_,_,_,_  ) },
  { Enc(X86OpAx)            , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RW)|F(Special)                      , 0                          },
  { Enc(X86OpDxAx)          , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RW)|F(Special)                      , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0x08, 0, F(Volatile)                           , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0x20, 0, F(Volatile)                           , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0x40, 0, F(Volatile)                           , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RO)|F(Volatile)                     , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x20, 0x20, 0, 0                                     , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0  , 0  , 0x24, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0  , 0  , 0x20, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0  , 0  , 0x04, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0  , 0  , 0x07, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0  , 0  , 0x03, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0  , 0  , 0x01, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0  , 0  , 0x10, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0  , 0  , 0x02, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(X86Arith)           , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(RO)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x40, 0x3F, 0, F(RW)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Cmpxchg)         , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(RW)|F(Lock)|F(Special)              , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0  , 0  , 0x00, 0x04, 0, F(RW)|F(Lock)|F(Special)              , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 8  , 8  , 0x00, 0x3F, 0, F(RO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 4  , 4  , 0x00, 0x3F, 0, F(RO)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RW)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Crc)             , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 16 , 8  , 4  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 16 , 4  , 4  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 16 , 4  , 8  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 8  , 4  , 8  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 8  , 4  , 4  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRm_Wx)           , 0  , 8  , 0  , 8  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 4  , 4  , 8  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRm_Wx)           , 0  , 8  , 8  , 0  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRm_Wx)           , 0  , 4  , 4  , 0  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 8  , 8  , 4  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRm_Wx)           , 0  , 8  , 0  , 4  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x28, 0x3F, 0, F(RW)|F(Special)                      , 0                          },
  { Enc(X86IncDec)          , 0  , 0  , 0  , 0  , 0x00, 0x1F, 0, F(RW)|F(Lock)                         , O(000000,48,_,_,x,_,_,_  ) },
  { Enc(X86M_Bx_MulDiv)     , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(RW)|F(Special)                      , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Volatile)                           , 0                          },
  { Enc(X86Enter)           , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Volatile)|F(Special)                , 0                          },
  { Enc(ExtExtract)         , 0  , 8  , 4  , 4  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtExtrq)           , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)                                 , O(660F00,78,0,_,_,_,_,_  ) },
  { Enc(FpuOp)              , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Fp)                                 , 0                          },
  { Enc(FpuArith)           , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Fp)|F(FPU_M4)|F(FPU_M8)             , 0                          },
  { Enc(FpuRDef)            , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Fp)                                 , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Fp)                                 , 0                          },
  { Enc(FpuR)               , 0  , 0  , 0  , 0  , 0x20, 0x00, 0, F(Fp)                                 , 0                          },
  { Enc(FpuR)               , 0  , 0  , 0  , 0  , 0x24, 0x00, 0, F(Fp)                                 , 0                          },
  { Enc(FpuR)               , 0  , 0  , 0  , 0  , 0x04, 0x00, 0, F(Fp)                                 , 0                          },
  { Enc(FpuR)               , 0  , 0  , 0  , 0  , 0x10, 0x00, 0, F(Fp)                                 , 0                          },
  { Enc(FpuCom)             , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Fp)                                 , 0                          },
  { Enc(FpuR)               , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(Fp)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Fp)                                 , 0                          },
  { Enc(FpuR)               , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Fp)                                 , 0                          },
  { Enc(FpuM)               , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Fp)|F(FPU_M2)|F(FPU_M4)             , 0                          },
  { Enc(FpuM)               , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , O_FPU(00,00DF,5)           },
  { Enc(FpuM)               , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , O_FPU(00,00DF,7)           },
  { Enc(FpuM)               , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , O_FPU(00,00DD,1)           },
  { Enc(FpuFldFst)          , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , O_FPU(00,00DB,5)           },
  { Enc(FpuStsw)            , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Fp)                                 , O_FPU(00,DFE0,_)           },
  { Enc(FpuFldFst)          , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Fp)|F(FPU_M4)|F(FPU_M8)             , 0                          },
  { Enc(FpuFldFst)          , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Fp)|F(FPU_M4)|F(FPU_M8)|F(FPU_M10)  , O(000000,DB,7,_,_,_,_,_  ) },
  { Enc(FpuStsw)            , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Fp)                                 , O_FPU(9B,DFE0,_)           },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Fp)|F(Volatile)                     , 0                          },
  { Enc(X86Imul)            , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(RW)|F(Special)                      , 0                          },
  { Enc(X86IncDec)          , 0  , 0  , 0  , 0  , 0x00, 0x1F, 0, F(RW)|F(Lock)                         , O(000000,40,_,_,x,_,_,_  ) },
  { Enc(ExtInsertq)         , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)                                 , O(F20F00,78,_,_,_,_,_,_  ) },
  { Enc(X86Int)             , 0  , 0  , 0  , 0  , 0x00, 0x88, 0, F(Volatile)                           , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0x88, 0, F(Volatile)                           , 0                          },
  { Enc(X86Jcc)             , 0  , 0  , 0  , 0  , 0x24, 0x00, 0, F(Flow)|F(Volatile)                   , 0                          },
  { Enc(X86Jcc)             , 0  , 0  , 0  , 0  , 0x20, 0x00, 0, F(Flow)|F(Volatile)                   , 0                          },
  { Enc(X86Jcc)             , 0  , 0  , 0  , 0  , 0x04, 0x00, 0, F(Flow)|F(Volatile)                   , 0                          },
  { Enc(X86Jcc)             , 0  , 0  , 0  , 0  , 0x07, 0x00, 0, F(Flow)|F(Volatile)                   , 0                          },
  { Enc(X86Jcc)             , 0  , 0  , 0  , 0  , 0x03, 0x00, 0, F(Flow)|F(Volatile)                   , 0                          },
  { Enc(X86Jcc)             , 0  , 0  , 0  , 0  , 0x01, 0x00, 0, F(Flow)|F(Volatile)                   , 0                          },
  { Enc(X86Jcc)             , 0  , 0  , 0  , 0  , 0x10, 0x00, 0, F(Flow)|F(Volatile)                   , 0                          },
  { Enc(X86Jcc)             , 0  , 0  , 0  , 0  , 0x02, 0x00, 0, F(Flow)|F(Volatile)                   , 0                          },
  { Enc(X86Jecxz)           , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Flow)|F(Volatile)|F(Special)        , 0                          },
  { Enc(X86Jmp)             , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Flow)|F(Volatile)                   , O(000000,E9,_,_,_,_,_,_  ) },
  { Enc(VexRvm)             , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexKmov)            , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , V(660F00,92,_,0,0,_,_,_  ) },
  { Enc(VexKmov)            , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , V(F20F00,92,_,0,0,_,_,_  ) },
  { Enc(VexKmov)            , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , V(F20F00,92,_,0,1,_,_,_  ) },
  { Enc(VexKmov)            , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , V(000F00,92,_,0,0,_,_,_  ) },
  { Enc(VexRm)              , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 1  , 1  , 0x00, 0x3F, 0, F(RO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 4  , 4  , 0x00, 0x3F, 0, F(RO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 8  , 8  , 0x00, 0x3F, 0, F(RO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 2  , 2  , 0x00, 0x3F, 0, F(RO)|F(Vex)                          , 0                          },
  { Enc(VexRmi)             , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRmi)             , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRmi)             , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRmi)             , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 2  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 8  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 4  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x3E, 0x00, 0, F(RW)|F(Volatile)|F(Special)          , 0                          },
  { Enc(ExtRm)              , 0  , 16 , 16 , 16 , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(X86Lea)             , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Volatile)|F(Special)                , 0                          },
  { Enc(X86Fence)           , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Volatile)                           , 0                          },
  { Enc(X86Op)              , 0  , 1  , 0  , 0  , 0x40, 0x00, 0, F(WO)|F(Special)                      , 0                          },
  { Enc(X86Op)              , 0  , 4  , 0  , 0  , 0x40, 0x00, 0, F(WO)|F(Special)                      , 0                          },
  { Enc(X86Op)              , 0  , 8  , 0  , 0  , 0x40, 0x00, 0, F(WO)|F(Special)                      , 0                          },
  { Enc(X86Op)              , 0  , 2  , 0  , 0  , 0x40, 0x00, 0, F(WO)|F(Special)                      , 0                          },
  { Enc(ExtRmZDI)           , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(RO)|F(Special)                      , 0                          },
  { Enc(ExtRmZDI)           , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RO)|F(Special)                      , 0                          },
  { Enc(X86Fence)           , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RW)|F(Volatile)                     , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RO)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Mov)             , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtMov)             , 0  , 16 , 8  , 8  , 0x00, 0x00, 0, F(WO)                                 , O(660F00,29,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 16 , 4  , 4  , 0x00, 0x00, 0, F(WO)                                 , O(000F00,29,_,_,_,_,_,_  ) },
  { Enc(ExtMovbe)           , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)                                 , O(000F38,F1,_,_,x,_,_,_  ) },
  { Enc(ExtMovd)            , 0  , 16 , 4  , 4  , 0x00, 0x00, 0, F(WO)                                 , O(000F00,7E,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 16 , 8  , 8  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtMov)             , 0  , 8  , 8  , 8  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtMov)             , 0  , 16 , 16 , 16 , 0x00, 0x00, 0, F(WO)                                 , O(660F00,7F,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 16 , 16 , 16 , 0x00, 0x00, 0, F(WO)                                 , O(F30F00,7F,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 8  , 4  , 4  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtMov)             , 8  , 8  , 8  , 8  , 0x00, 0x00, 0, F(RW)                                 , O(660F00,17,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 8  , 8  , 4  , 4  , 0x00, 0x00, 0, F(RW)                                 , O(000F00,17,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 8  , 8  , 4  , 4  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtMov)             , 0  , 8  , 8  , 8  , 0x00, 0x00, 0, F(WO)                                 , O(660F00,13,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 8  , 4  , 4  , 0x00, 0x00, 0, F(WO)                                 , O(000F00,13,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 8  , 0  , 8  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtMov)             , 0  , 8  , 0  , 4  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtMov)             , 0  , 16 , 0  , 0  , 0x00, 0x00, 0, F(WO)                                 , O(660F00,E7,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 16 , 0  , 0  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtMovnti)          , 0  , 8  , 0  , 0  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtMov)             , 0  , 16 , 8  , 8  , 0x00, 0x00, 0, F(WO)                                 , O(660F00,2B,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 16 , 4  , 4  , 0x00, 0x00, 0, F(WO)                                 , O(000F00,2B,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 8  , 0  , 0  , 0x00, 0x00, 0, F(WO)                                 , O(000F00,E7,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 8  , 8  , 8  , 0x00, 0x00, 0, F(WO)                                 , O(F20F00,2B,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 4  , 4  , 4  , 0x00, 0x00, 0, F(WO)                                 , O(F30F00,2B,_,_,_,_,_,_  ) },
  { Enc(ExtMovq)            , 0  , 16 , 8  , 8  , 0x00, 0x00, 0, F(WO)                                 , O(000F00,7E,_,_,x,_,_,_  ) },
  { Enc(ExtRm)              , 0  , 16 , 8  , 8  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)|F(Special)                      , 0                          },
  { Enc(ExtMov)             , 0  , 8  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(ZeroIfMem)                    , O(F20F00,11,_,_,_,_,_,_  ) },
  { Enc(ExtRm)              , 0  , 16 , 0  , 0  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtMov)             , 0  , 4  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(ZeroIfMem)                    , O(F30F00,11,_,_,_,_,_,_  ) },
  { Enc(X86MovsxMovzx)      , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtMov)             , 0  , 16 , 8  , 8  , 0x00, 0x00, 0, F(WO)                                 , O(660F00,11,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 16 , 4  , 4  , 0x00, 0x00, 0, F(WO)                                 , O(000F00,11,_,_,_,_,_,_  ) },
  { Enc(ExtRmi)             , 0  , 0  , 2  , 1  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(VexRvmZDX_Wx)       , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RW)|F(Special)                      , 0                          },
  { Enc(X86M_Bx)            , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(RW)|F(Lock)                         , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, 0                                     , 0                          },
  { Enc(X86M_Bx)            , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RW)|F(Lock)                         , 0                          },
  { Enc(ExtRm_P)            , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRm_P)            , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRm_P)            , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRm_P)            , 0  , 0  , 2  , 4  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRm_P)            , 0  , 0  , 1  , 2  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 2  , 4  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRmi_P)           , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRm_P)            , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(Ext3dNow)           , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRmXMM0)          , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(RW)|F(Special)                      , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 16 , 8  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)|F(Special)                      , 0                          },
  { Enc(X86Op_O)            , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Volatile)                           , 0                          },
  { Enc(VexRvm_Wx)          , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtExtract)         , 0  , 8  , 1  , 1  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtExtract)         , 0  , 8  , 8  , 8  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtPextrw)          , 0  , 8  , 2  , 2  , 0x00, 0x00, 0, F(WO)                                 , O(000F3A,15,_,_,_,_,_,_  ) },
  { Enc(Ext3dNow)           , 0  , 8  , 4  , 4  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(Ext3dNow)           , 0  , 8  , 2  , 4  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(Ext3dNow)           , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(Ext3dNow)           , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(Ext3dNow)           , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 0  , 2  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(Ext3dNow)           , 0  , 8  , 4  , 2  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRmi_P)           , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRm_P)            , 0  , 0  , 2  , 1  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRm_P)            , 0  , 0  , 4  , 2  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRm_P)            , 0  , 8  , 0  , 1  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 16 , 4  , 1  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 16 , 8  , 1  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 16 , 2  , 1  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 16 , 4  , 2  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 16 , 8  , 2  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 8  , 4  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(Ext3dNow)           , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRm_P)            , 0  , 0  , 8  , 4  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(X86Pop)             , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)|F(Volatile)|F(Special)          , O(000000,58,_,_,_,_,_,_  ) },
  { Enc(X86Rm)              , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(WO)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0xFF, 0, F(Volatile)|F(Special)                , 0                          },
  { Enc(X86Prefetch)        , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RO)|F(Volatile)                     , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(RO)|F(Volatile)                     , 0                          },
  { Enc(ExtRmi)             , 0  , 16 , 4  , 4  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 16 , 2  , 2  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRmi_P)           , 0  , 8  , 2  , 2  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRmRi_P)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)                                 , O(000F00,72,6,_,_,_,_,_  ) },
  { Enc(ExtRmRi)            , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(RW)                                 , O(660F00,73,7,_,_,_,_,_  ) },
  { Enc(ExtRmRi_P)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)                                 , O(000F00,73,6,_,_,_,_,_  ) },
  { Enc(ExtRmRi_P)          , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(RW)                                 , O(000F00,71,6,_,_,_,_,_  ) },
  { Enc(ExtRmRi_P)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)                                 , O(000F00,72,4,_,_,_,_,_  ) },
  { Enc(ExtRmRi_P)          , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(RW)                                 , O(000F00,71,4,_,_,_,_,_  ) },
  { Enc(ExtRmRi_P)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)                                 , O(000F00,72,2,_,_,_,_,_  ) },
  { Enc(ExtRmRi)            , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(RW)                                 , O(660F00,73,3,_,_,_,_,_  ) },
  { Enc(ExtRmRi_P)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)                                 , O(000F00,73,2,_,_,_,_,_  ) },
  { Enc(ExtRmRi_P)          , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(RW)                                 , O(000F00,71,2,_,_,_,_,_  ) },
  { Enc(ExtRm_P)            , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(RO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 16 , 8  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(X86Push)            , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RO)|F(Volatile)|F(Special)          , O(000000,50,_,_,_,_,_,_  ) },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0xFF, 0x00, 0, F(Volatile)|F(Special)                , 0                          },
  { Enc(X86Rot)             , 0  , 0  , 0  , 0  , 0x20, 0x21, 0, F(RW)|F(Special)                      , 0                          },
  { Enc(ExtRm)              , 0  , 4  , 4  , 4  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(X86M)               , 0  , 8  , 0  , 0  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(X86M)               , 0  , 8  , 0  , 0  , 0x00, 0x3F, 0, F(WO)                                 , 0                          },
  { Enc(X86Rep)             , 0  , 0  , 0  , 0  , 0x40, 0x00, 0, F(RW)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Rep)             , 0  , 0  , 0  , 0  , 0x40, 0x3F, 0, F(RW)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Ret)             , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RW)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Rot)             , 0  , 0  , 0  , 0  , 0x00, 0x21, 0, F(RW)|F(Special)                      , 0                          },
  { Enc(VexRmi_Wx)          , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 16 , 8  , 8  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 8  , 8  , 8  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 4  , 4  , 4  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0x3E, 0, F(RO)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Rot)             , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(RW)|F(Special)                      , 0                          },
  { Enc(VexRmv_Wx)          , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(X86Set)             , 0  , 1  , 0  , 0  , 0x24, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(X86Set)             , 0  , 1  , 0  , 0  , 0x20, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(X86Set)             , 0  , 1  , 0  , 0  , 0x04, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(X86Set)             , 0  , 1  , 0  , 0  , 0x07, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(X86Set)             , 0  , 1  , 0  , 0  , 0x03, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(X86Set)             , 0  , 1  , 0  , 0  , 0x01, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(X86Set)             , 0  , 1  , 0  , 0  , 0x10, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(X86Set)             , 0  , 1  , 0  , 0  , 0x02, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RW)                                 , 0                          },
  { Enc(ExtRmXMM0)          , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RW)|F(Special)                      , 0                          },
  { Enc(X86ShldShrd)        , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(RW)|F(Special)                      , 0                          },
  { Enc(ExtRm)              , 0  , 8  , 8  , 8  , 0x00, 0x00, 0, F(WO)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0x20, 0, 0                                     , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0x40, 0, 0                                     , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x00, 0x80, 0, 0                                     , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Volatile)                           , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0  , 0  , 0x40, 0x00, 0, F(RW)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Test)            , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(RO)                                 , O(000000,F6,_,_,x,_,_,_  ) },
  { Enc(VexRvm_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRmi)             , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)          |A512(BW  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)          |A512(BW  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmr_Lx)         , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmr_Lx)         , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,KZ,0  ,B) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 32 , 32 , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,0,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 32 , 32 , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,SAE,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,SAE,B) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 8  , 8  , 0x00, 0x3F, 0, F(RO)|F(Vex)   |A512(F_  ,0,0 ,SAE,B) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 4  , 4  , 0x00, 0x3F, 0, F(RO)|F(Vex)   |A512(F_  ,0,0 ,SAE,B) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 8  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 4  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,1,KZ,RC ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 4  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,RC ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 4  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 8  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , 0                          },
  { Enc(VexMri_Lx)          , 0  , 0  , 2  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 8  , 4  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,1,KZ,RC ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,RC ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 4  , 8  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,1,KZ,RC ,B) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,RC ,B) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 4  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,0 ,RC ,B) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 8  , 0  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,RC ,B) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 4  , 0  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,RC ,B) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 8  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,SAE,B) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,RC ,B) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,0 ,RC ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 4  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 4  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 8  , 4  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,SAE,B) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,0 ,SAE,B) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,SAE,B) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,0 ,SAE,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 8  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 8  , 0  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,0 ,RC ,B) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 4  , 0  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,0 ,RC ,B) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 2  , 1  , 0x00, 0x00, 0, F(WO)          |A512(BW  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(ER  ,0,KZ,SAE,B) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(ER  ,0,KZ,SAE,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexMri)             , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexMri_Lx)          , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexMri)             , 0  , 0  , 32 , 32 , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,0,KZ,0  ,B) , 0                          },
  { Enc(VexMri_Lx)          , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexMri)             , 0  , 0  , 32 , 32 , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,KZ,0  ,B) , 0                          },
  { Enc(VexMri)             , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)          |A512(F_  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)          |A512(F_  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)          |A512(F_  ,0,KZ,SAE,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)          |A512(F_  ,0,KZ,SAE,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , 0                          },
  { Enc(Fma4_Lx)            , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(Fma4_Lx)            , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(Fma4)               , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(Fma4)               , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,1,K_,0  ,B) , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,1,K_,0  ,B) , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,0,K_,0  ,B) , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,0,K_,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRmvRm_VM)        , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,B) , V(660F38,92,_,x,_,1,3,T1S) },
  { Enc(VexRmvRm_VM)        , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,B) , V(660F38,92,_,x,_,0,2,T1S) },
  { Enc(VexM_VM)            , 0  , 0  , 0  , 8  , 0x00, 0x00, 0, F(RO)|F(VM)    |A512(PF  ,0,K_,0  ,B) , 0                          },
  { Enc(VexM_VM)            , 0  , 0  , 0  , 4  , 0x00, 0x00, 0, F(RO)|F(VM)    |A512(PF  ,0,K_,0  ,B) , 0                          },
  { Enc(VexRmvRm_VM)        , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,B) , V(660F38,93,_,x,_,1,3,T1S) },
  { Enc(VexRmvRm_VM)        , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,B) , V(660F38,93,_,x,_,0,2,T1S) },
  { Enc(VexRm)              , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,KZ,SAE,B) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,KZ,SAE,B) , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRmi)             , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,KZ,SAE,B) , 0                          },
  { Enc(VexRmi)             , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,KZ,SAE,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 32 , 32 , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,0,KZ,0  ,B) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 32 , 32 , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,KZ,0  ,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexM)               , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RO)|F(Vex)|F(Volatile)              , 0                          },
  { Enc(VexRmZDI)           , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(RO)|F(Vex)|F(Special)               , 0                          },
  { Enc(VexRvmMvr_Lx)       , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)|F(Vex)                          , V(660F38,2F,_,x,0,_,_,_  ) },
  { Enc(VexRvmMvr_Lx)       , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)|F(Vex)                          , V(660F38,2E,_,x,0,_,_,_  ) },
  { Enc(VexRvm_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , V(660F00,29,_,x,I,1,4,FVM) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , V(000F00,29,_,x,I,0,4,FVM) },
  { Enc(VexMovDQ)           , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , V(660F00,7E,_,0,0,0,2,T1S) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(WO)|F(Vex)                          , V(660F00,7F,_,x,I,_,_,_  ) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , V(660F00,7F,_,x,_,0,4,FVM) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , V(660F00,7F,_,x,_,1,4,FVM) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(WO)|F(Vex)                          , V(F30F00,7F,_,x,I,_,_,_  ) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)          |A512(BW  ,1,KZ,0  ,B) , V(F20F00,7F,_,x,_,1,4,FVM) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , V(F30F00,7F,_,x,_,0,4,FVM) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , V(F30F00,7F,_,x,_,1,4,FVM) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)          |A512(BW  ,1,KZ,0  ,B) , V(F20F00,7F,_,x,_,0,4,FVM) },
  { Enc(VexRvm)             , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , 0                          },
  { Enc(VexRvmMr)           , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , V(660F00,17,_,0,I,1,3,T1S) },
  { Enc(VexRvmMr)           , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , V(000F00,17,_,0,I,0,3,T2 ) },
  { Enc(VexRvmMr)           , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , V(660F00,13,_,0,I,1,3,T1S) },
  { Enc(VexRvmMr)           , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , V(000F00,13,_,0,I,0,3,T2 ) },
  { Enc(VexRm_Lx)           , 0  , 0  , 0  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,B) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,B) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,B) , 0                          },
  { Enc(VexMovDQ)           , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,B) , V(660F00,7E,_,0,I,1,3,T1S) },
  { Enc(VexMovSsSd)         , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,0  ,B) , V(F20F00,11,_,I,I,1,3,T1S) },
  { Enc(VexMovSsSd)         , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,0  ,B) , V(F30F00,11,_,I,I,0,2,T1S) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , V(660F00,11,_,x,I,1,4,FVM) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , V(000F00,11,_,x,I,0,4,FVM) },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 2  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,RC ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 2  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 1  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmr)            , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 4  , 1  , 0x00, 0x00, 0, F(WO)          |A512(CD  ,1,0 ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 8  , 1  , 0x00, 0x00, 0, F(WO)          |A512(CD  ,1,0 ,0  ,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 16 , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvrmRvmr_Lx)     , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)          |A512(BW  ,1,K_,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,K_,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,1,K_,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,K_,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,K_,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,1,K_,0  ,B) , 0                          },
  { Enc(VexRmi)             , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)|F(Vex)|F(Special)               , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,K_,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)          |A512(BW  ,1,K_,0  ,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(CD  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(CD  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)          |A512(VBMI,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(RW)          |A512(BW  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvrmiRvmri_Lx)   , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvrmiRvmri_Lx)   , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmRmi_Lx)       , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , V(660F3A,05,_,x,0,1,4,FV ) },
  { Enc(VexRvmRmi_Lx)       , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , V(660F3A,04,_,x,0,0,4,FV ) },
  { Enc(VexRvmRmi_Lx)       , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , V(660F3A,00,_,x,1,1,4,FV ) },
  { Enc(VexMri)             , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,0,0 ,0  ,B) , 0                          },
  { Enc(VexMri)             , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(DQ  ,0,0 ,0  ,B) , 0                          },
  { Enc(VexMri)             , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(DQ  ,0,0 ,0  ,B) , 0                          },
  { Enc(VexMri)             , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,0,0 ,0  ,B) , 0                          },
  { Enc(VexRmvRm_VM)        , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,B) , V(660F38,90,_,x,_,0,2,T1S) },
  { Enc(VexRmvRm_VM)        , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,B) , V(660F38,90,_,x,_,1,3,T1S) },
  { Enc(VexRmvRm_VM)        , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,B) , V(660F38,91,_,x,_,0,2,T1S) },
  { Enc(VexRmvRm_VM)        , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,B) , V(660F38,91,_,x,_,1,3,T1S) },
  { Enc(VexRm)              , 0  , 0  , 4  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 8  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 2  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 8  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 4  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 8  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,0,KZ,0  ,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(DQ  ,0,KZ,0  ,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(DQ  ,0,KZ,0  ,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,0,KZ,0  ,B) , 0                          },
  { Enc(VexRvmr)            , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmr)            , 0  , 0  , 8  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmr)            , 0  , 0  , 4  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmr)            , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(IFMA,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 2  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 4  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvmMvr_Lx)       , 0  , 0  , 0  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , V(660F38,8E,_,x,0,_,_,_  ) },
  { Enc(VexRvmMvr_Lx)       , 0  , 0  , 0  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , V(660F38,8E,_,x,1,_,_,_  ) },
  { Enc(VexRm_Lx)           , 0  , 0  , 0  , 1  , 0x00, 0x00, 0, F(WO)          |A512(BW  ,1,0 ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0  , 4  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,1,0 ,0  ,B) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 1  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 2  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0  , 2  , 0x00, 0x00, 0, F(WO)          |A512(BW  ,1,0 ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0  , 8  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,1,0 ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 1  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 4  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 2  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 1  , 2  , 0x00, 0x00, 0, F(WO)          |A512(BW  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 4  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 8  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 2  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 4  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 8  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 8  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 1  , 8  , 0x00, 0x00, 0, F(WO)          |A512(VBMI,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvrmRvmr)        , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexVmi_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexVmi_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvmRmvRmi)       , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , V(XOP_M8,C0,_,0,x,_,_,_  ) },
  { Enc(VexRvmRmvRmi)       , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , V(XOP_M8,C2,_,0,x,_,_,_  ) },
  { Enc(VexRvmRmvRmi)       , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , V(XOP_M8,C3,_,0,x,_,_,_  ) },
  { Enc(VexRvmRmvRmi)       , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , V(XOP_M8,C1,_,0,x,_,_,_  ) },
  { Enc(VexRvm_Lx)          , 0  , 0  , 2  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,1,0 ,0  ,B) , 0                          },
  { Enc(VexMr_VM)           , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,B) , 0                          },
  { Enc(VexMr_VM)           , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,B) , 0                          },
  { Enc(VexRvmRmv)          , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmRmv)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmRmv)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmRmv)          , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 1  , 1  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmVmi_Lx)       , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , V(660F00,72,6,x,I,0,4,FV ) },
  { Enc(VexVmi_VexEvex_Lx)  , 0  , 0  , 16 , 16 , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,1,0 ,0  ,B) , 0                          },
  { Enc(VexRvmVmi_Lx)       , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , V(660F00,73,6,x,I,1,4,FV ) },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)          |A512(BW  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvmVmi_Lx)       , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , V(660F00,71,6,x,I,I,4,FVM) },
  { Enc(VexRvmVmi_Lx)       , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , V(660F00,72,4,x,I,0,4,FV ) },
  { Enc(VexRvmVmi_Lx)       , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,0  ,B) , V(660F00,72,4,x,_,1,4,FV ) },
  { Enc(VexRvmVmi_Lx)       , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , V(660F00,71,4,x,I,I,4,FVM) },
  { Enc(VexRvmVmi_Lx)       , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , V(660F00,72,2,x,I,0,4,FV ) },
  { Enc(VexRvmVmi_Lx)       , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , V(660F00,73,2,x,I,1,4,FV ) },
  { Enc(VexRvmVmi_Lx)       , 0  , 0  , 2  , 2  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,B) , V(660F00,71,2,x,I,I,4,FVM) },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(RW)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(RW)          |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(RO)|F(Vex)                          , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0  , 1  , 0x00, 0x00, 0, F(WO)          |A512(BW  ,1,K_,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,K_,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,K_,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0  , 2  , 0x00, 0x00, 0, F(WO)          |A512(BW  ,1,K_,0  ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 16 , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,1,KZ,SAE,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,0,KZ,SAE,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,0,KZ,SAE,B) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,KZ,0  ,B) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,KZ,0  ,B) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(ER  ,0,KZ,SAE,B) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(ER  ,0,KZ,SAE,B) , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,0,KZ,0  ,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(DQ  ,0,KZ,0  ,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,KZ,SAE,B) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,KZ,SAE,B) , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,RC ,B) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,1,KZ,RC ,B) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,KZ,RC ,B) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)          |A512(F_  ,0,KZ,RC ,B) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,B) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,B) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 4  , 4  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,B) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 8  , 8  , 0x00, 0x00, 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,RC ,B) , 0                          },
  { Enc(VexM)               , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Vex)|F(Volatile)                    , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 8  , 8  , 0x00, 0x3F, 0, F(RO)|F(Vex)                          , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 4  , 4  , 0x00, 0x3F, 0, F(RO)|F(Vex)                          , 0                          },
  { Enc(VexOp)              , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(Vex)|F(Volatile)                    , 0                          },
  { Enc(X86M)               , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RO)|F(Volatile)                     , 0                          },
  { Enc(X86Xadd)            , 0  , 0  , 0  , 0  , 0x00, 0x3F, 0, F(RW)|F(Xchg)|F(Lock)                 , 0                          },
  { Enc(X86Xchg)            , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RW)|F(Xchg)|F(Lock)                 , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(RO)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0  , 0  , 0x00, 0x00, 0, F(WO)|F(Volatile)|F(Special)          , 0                          }
};
// ----------------------------------------------------------------------------
// ${X86InstExtendedData:End}

#undef INST
#undef A512

#undef O_FPU
#undef O
#undef V

#undef Enc
#undef EF
#undef F

// ============================================================================
// [asmjit::X86Util - Id <-> Name]
// ============================================================================

#if !defined(ASMJIT_DISABLE_TEXT)
// ${X86InstNameData:Begin}
// ------------------- Automatically generated, do not edit -------------------
static const char _x86InstNameData[] =
  "\0" "adc\0" "adcx\0" "adox\0" "bextr\0" "blcfill\0" "blci\0" "blcic\0"
  "blcmsk\0" "blcs\0" "blsfill\0" "blsi\0" "blsic\0" "blsmsk\0" "blsr\0"
  "bsf\0" "bsr\0" "bswap\0" "bt\0" "btc\0" "btr\0" "bts\0" "bzhi\0" "call\0"
  "cbw\0" "cdq\0" "cdqe\0" "clac\0" "clc\0" "cld\0" "clflush\0" "clflushopt\0"
  "clwb\0" "clzero\0" "cmc\0" "cmova\0" "cmovae\0" "cmovc\0" "cmovg\0"
  "cmovge\0" "cmovl\0" "cmovle\0" "cmovna\0" "cmovnae\0" "cmovnc\0" "cmovng\0"
  "cmovnge\0" "cmovnl\0" "cmovnle\0" "cmovno\0" "cmovnp\0" "cmovns\0"
  "cmovnz\0" "cmovo\0" "cmovp\0" "cmovpe\0" "cmovpo\0" "cmovs\0" "cmovz\0"
  "cmp\0" "cmpxchg\0" "cmpxchg16b\0" "cmpxchg8b\0" "cpuid\0" "cqo\0" "crc32\0"
  "cvtpd2pi\0" "cvtpi2pd\0" "cvtpi2ps\0" "cvtps2pi\0" "cvttpd2pi\0"
  "cvttps2pi\0" "cwd\0" "cwde\0" "daa\0" "das\0" "enter\0" "f2xm1\0" "fabs\0"
  "faddp\0" "fbld\0" "fbstp\0" "fchs\0" "fclex\0" "fcmovb\0" "fcmovbe\0"
  "fcmove\0" "fcmovnb\0" "fcmovnbe\0" "fcmovne\0" "fcmovnu\0" "fcmovu\0"
  "fcom\0" "fcomi\0" "fcomip\0" "fcomp\0" "fcompp\0" "fcos\0" "fdecstp\0"
  "fdiv\0" "fdivp\0" "fdivr\0" "fdivrp\0" "femms\0" "ffree\0" "fiadd\0"
  "ficom\0" "ficomp\0" "fidiv\0" "fidivr\0" "fild\0" "fimul\0" "fincstp\0"
  "finit\0" "fist\0" "fistp\0" "fisttp\0" "fisub\0" "fisubr\0" "fld\0" "fld1\0"
  "fldcw\0" "fldenv\0" "fldl2e\0" "fldl2t\0" "fldlg2\0" "fldln2\0" "fldpi\0"
  "fldz\0" "fmulp\0" "fnclex\0" "fninit\0" "fnop\0" "fnsave\0" "fnstcw\0"
  "fnstenv\0" "fnstsw\0" "fpatan\0" "fprem\0" "fprem1\0" "fptan\0" "frndint\0"
  "frstor\0" "fsave\0" "fscale\0" "fsin\0" "fsincos\0" "fsqrt\0" "fst\0"
  "fstcw\0" "fstenv\0" "fstp\0" "fstsw\0" "fsubp\0" "fsubrp\0" "ftst\0"
  "fucom\0" "fucomi\0" "fucomip\0" "fucomp\0" "fucompp\0" "fwait\0" "fxam\0"
  "fxch\0" "fxrstor\0" "fxrstor64\0" "fxsave\0" "fxsave64\0" "fxtract\0"
  "fyl2x\0" "fyl2xp1\0" "inc\0" "insertq\0" "int3\0" "into\0" "ja\0" "jae\0"
  "jb\0" "jbe\0" "jc\0" "je\0" "jecxz\0" "jg\0" "jge\0" "jl\0" "jle\0" "jmp\0"
  "jna\0" "jnae\0" "jnb\0" "jnbe\0" "jnc\0" "jne\0" "jng\0" "jnge\0" "jnl\0"
  "jnle\0" "jno\0" "jnp\0" "jns\0" "jnz\0" "jo\0" "jp\0" "jpe\0" "jpo\0" "js\0"
  "jz\0" "kaddb\0" "kaddd\0" "kaddq\0" "kaddw\0" "kandb\0" "kandd\0" "kandnb\0"
  "kandnd\0" "kandnq\0" "kandnw\0" "kandq\0" "kandw\0" "kmovb\0" "kmovw\0"
  "knotb\0" "knotd\0" "knotq\0" "knotw\0" "korb\0" "kord\0" "korq\0"
  "kortestb\0" "kortestd\0" "kortestq\0" "kortestw\0" "korw\0" "kshiftlb\0"
  "kshiftld\0" "kshiftlq\0" "kshiftlw\0" "kshiftrb\0" "kshiftrd\0" "kshiftrq\0"
  "kshiftrw\0" "ktestb\0" "ktestd\0" "ktestq\0" "ktestw\0" "kunpckbw\0"
  "kunpckdq\0" "kunpckwd\0" "kxnorb\0" "kxnord\0" "kxnorq\0" "kxnorw\0"
  "kxorb\0" "kxord\0" "kxorq\0" "kxorw\0" "lahf\0" "lea\0" "leave\0" "lfence\0"
  "lzcnt\0" "mfence\0" "monitor\0" "movdq2q\0" "movnti\0" "movntq\0"
  "movntsd\0" "movntss\0" "movq2dq\0" "movsx\0" "movsxd\0" "movzx\0" "mulx\0"
  "mwait\0" "neg\0" "not\0" "pause\0" "pavgusb\0" "pcommit\0" "pdep\0" "pext\0"
  "pf2id\0" "pf2iw\0" "pfacc\0" "pfadd\0" "pfcmpeq\0" "pfcmpge\0" "pfcmpgt\0"
  "pfmax\0" "pfmin\0" "pfmul\0" "pfnacc\0" "pfpnacc\0" "pfrcp\0" "pfrcpit1\0"
  "pfrcpit2\0" "pfrcpv\0" "pfrsqit1\0" "pfrsqrt\0" "pfrsqrtv\0" "pfsub\0"
  "pfsubr\0" "pi2fd\0" "pi2fw\0" "pmulhrw\0" "pop\0" "popa\0" "popcnt\0"
  "popf\0" "prefetch\0" "prefetch3dnow\0" "prefetchw\0" "prefetchwt1\0"
  "pshufw\0" "pswapd\0" "push\0" "pusha\0" "pushf\0" "rcl\0" "rcr\0"
  "rdfsbase\0" "rdgsbase\0" "rdrand\0" "rdseed\0" "rdtsc\0" "rdtscp\0"
  "rep lods_b\0" "rep lods_d\0" "rep lods_q\0" "rep lods_w\0" "rep movs_b\0"
  "rep movs_d\0" "rep movs_q\0" "rep movs_w\0" "rep stos_b\0" "rep stos_d\0"
  "rep stos_q\0" "rep stos_w\0" "repe cmps_b\0" "repe cmps_d\0" "repe cmps_q\0"
  "repe cmps_w\0" "repe scas_b\0" "repe scas_d\0" "repe scas_q\0"
  "repe scas_w\0" "repne cmps_b\0" "repne cmps_d\0" "repne cmps_q\0"
  "repne cmps_w\0" "repne scas_b\0" "repne scas_d\0" "repne scas_q\0"
  "repne scas_w\0" "ret\0" "rol\0" "ror\0" "rorx\0" "sahf\0" "sal\0" "sar\0"
  "sarx\0" "sbb\0" "seta\0" "setae\0" "setb\0" "setbe\0" "setc\0" "sete\0"
  "setg\0" "setge\0" "setl\0" "setle\0" "setna\0" "setnae\0" "setnb\0"
  "setnbe\0" "setnc\0" "setne\0" "setng\0" "setnge\0" "setnl\0" "setnle\0"
  "setno\0" "setnp\0" "setns\0" "setnz\0" "seto\0" "setp\0" "setpe\0" "setpo\0"
  "sets\0" "setz\0" "sfence\0" "sha1msg1\0" "sha1msg2\0" "sha1nexte\0"
  "sha1rnds4\0" "sha256msg1\0" "sha256msg2\0" "sha256rnds2\0" "shl\0" "shlx\0"
  "shr\0" "shrd\0" "shrx\0" "stac\0" "stc\0" "sti\0" "t1mskc\0" "tzcnt\0"
  "tzmsk\0" "ud2\0" "vaddpd\0" "vaddps\0" "vaddsd\0" "vaddss\0" "vaddsubpd\0"
  "vaddsubps\0" "vaesdec\0" "vaesdeclast\0" "vaesenc\0" "vaesenclast\0"
  "vaesimc\0" "vaeskeygenassist\0" "valignd\0" "valignq\0" "vandnpd\0"
  "vandnps\0" "vandpd\0" "vandps\0" "vblendmb\0" "vblendmd\0" "vblendmpd\0"
  "vblendmps\0" "vblendmq\0" "vblendmw\0" "vblendpd\0" "vblendps\0"
  "vblendvpd\0" "vblendvps\0" "vbroadcastf128\0" "vbroadcastf32x2\0"
  "vbroadcastf32x4\0" "vbroadcastf32x8\0" "vbroadcastf64x2\0"
  "vbroadcastf64x4\0" "vbroadcasti128\0" "vbroadcasti32x2\0"
  "vbroadcasti32x4\0" "vbroadcasti32x8\0" "vbroadcasti64x2\0"
  "vbroadcasti64x4\0" "vbroadcastsd\0" "vbroadcastss\0" "vcmppd\0" "vcmpps\0"
  "vcmpsd\0" "vcmpss\0" "vcomisd\0" "vcomiss\0" "vcompresspd\0" "vcompressps\0"
  "vcvtdq2pd\0" "vcvtdq2ps\0" "vcvtpd2dq\0" "vcvtpd2ps\0" "vcvtpd2qq\0"
  "vcvtpd2udq\0" "vcvtpd2uqq\0" "vcvtph2ps\0" "vcvtps2dq\0" "vcvtps2pd\0"
  "vcvtps2ph\0" "vcvtps2qq\0" "vcvtps2udq\0" "vcvtps2uqq\0" "vcvtqq2pd\0"
  "vcvtqq2ps\0" "vcvtsd2si\0" "vcvtsd2ss\0" "vcvtsd2usi\0" "vcvtsi2sd\0"
  "vcvtsi2ss\0" "vcvtss2sd\0" "vcvtss2si\0" "vcvtss2usi\0" "vcvttpd2dq\0"
  "vcvttpd2qq\0" "vcvttpd2udq\0" "vcvttpd2uqq\0" "vcvttps2dq\0" "vcvttps2qq\0"
  "vcvttps2udq\0" "vcvttps2uqq\0" "vcvttsd2si\0" "vcvttsd2usi\0" "vcvttss2si\0"
  "vcvttss2usi\0" "vcvtudq2pd\0" "vcvtudq2ps\0" "vcvtuqq2pd\0" "vcvtuqq2ps\0"
  "vcvtusi2sd\0" "vcvtusi2ss\0" "vdbpsadbw\0" "vdivpd\0" "vdivps\0" "vdivsd\0"
  "vdivss\0" "vdppd\0" "vdpps\0" "vexp2pd\0" "vexp2ps\0" "vexpandpd\0"
  "vexpandps\0" "vextractf128\0" "vextractf32x4\0" "vextractf32x8\0"
  "vextractf64x2\0" "vextractf64x4\0" "vextracti128\0" "vextracti32x4\0"
  "vextracti32x8\0" "vextracti64x2\0" "vextracti64x4\0" "vextractps\0"
  "vfixupimmpd\0" "vfixupimmps\0" "vfixupimmsd\0" "vfixupimmss\0"
  "vfmadd132pd\0" "vfmadd132ps\0" "vfmadd132sd\0" "vfmadd132ss\0"
  "vfmadd213pd\0" "vfmadd213ps\0" "vfmadd213sd\0" "vfmadd213ss\0"
  "vfmadd231pd\0" "vfmadd231ps\0" "vfmadd231sd\0" "vfmadd231ss\0" "vfmaddpd\0"
  "vfmaddps\0" "vfmaddsd\0" "vfmaddss\0" "vfmaddsub132pd\0" "vfmaddsub132ps\0"
  "vfmaddsub213pd\0" "vfmaddsub213ps\0" "vfmaddsub231pd\0" "vfmaddsub231ps\0"
  "vfmaddsubpd\0" "vfmaddsubps\0" "vfmsub132pd\0" "vfmsub132ps\0"
  "vfmsub132sd\0" "vfmsub132ss\0" "vfmsub213pd\0" "vfmsub213ps\0"
  "vfmsub213sd\0" "vfmsub213ss\0" "vfmsub231pd\0" "vfmsub231ps\0"
  "vfmsub231sd\0" "vfmsub231ss\0" "vfmsubadd132pd\0" "vfmsubadd132ps\0"
  "vfmsubadd213pd\0" "vfmsubadd213ps\0" "vfmsubadd231pd\0" "vfmsubadd231ps\0"
  "vfmsubaddpd\0" "vfmsubaddps\0" "vfmsubpd\0" "vfmsubps\0" "vfmsubsd\0"
  "vfmsubss\0" "vfnmadd132pd\0" "vfnmadd132ps\0" "vfnmadd132sd\0"
  "vfnmadd132ss\0" "vfnmadd213pd\0" "vfnmadd213ps\0" "vfnmadd213sd\0"
  "vfnmadd213ss\0" "vfnmadd231pd\0" "vfnmadd231ps\0" "vfnmadd231sd\0"
  "vfnmadd231ss\0" "vfnmaddpd\0" "vfnmaddps\0" "vfnmaddsd\0" "vfnmaddss\0"
  "vfnmsub132pd\0" "vfnmsub132ps\0" "vfnmsub132sd\0" "vfnmsub132ss\0"
  "vfnmsub213pd\0" "vfnmsub213ps\0" "vfnmsub213sd\0" "vfnmsub213ss\0"
  "vfnmsub231pd\0" "vfnmsub231ps\0" "vfnmsub231sd\0" "vfnmsub231ss\0"
  "vfnmsubpd\0" "vfnmsubps\0" "vfnmsubsd\0" "vfnmsubss\0" "vfpclasspd\0"
  "vfpclassps\0" "vfpclasssd\0" "vfpclassss\0" "vfrczpd\0" "vfrczps\0"
  "vfrczsd\0" "vfrczss\0" "vgatherdpd\0" "vgatherdps\0" "vgatherpf0dpd\0"
  "vgatherpf0dps\0" "vgatherpf0qpd\0" "vgatherpf0qps\0" "vgatherpf1dpd\0"
  "vgatherpf1dps\0" "vgatherpf1qpd\0" "vgatherpf1qps\0" "vgatherqpd\0"
  "vgatherqps\0" "vgetexppd\0" "vgetexpps\0" "vgetexpsd\0" "vgetexpss\0"
  "vgetmantpd\0" "vgetmantps\0" "vgetmantsd\0" "vgetmantss\0" "vhaddpd\0"
  "vhaddps\0" "vhsubpd\0" "vhsubps\0" "vinsertf128\0" "vinsertf32x4\0"
  "vinsertf32x8\0" "vinsertf64x2\0" "vinsertf64x4\0" "vinserti128\0"
  "vinserti32x4\0" "vinserti32x8\0" "vinserti64x2\0" "vinserti64x4\0"
  "vinsertps\0" "vlddqu\0" "vldmxcsr\0" "vmaskmovdqu\0" "vmaskmovpd\0"
  "vmaskmovps\0" "vmaxpd\0" "vmaxps\0" "vmaxsd\0" "vmaxss\0" "vminpd\0"
  "vminps\0" "vminsd\0" "vminss\0" "vmovapd\0" "vmovaps\0" "vmovd\0"
  "vmovddup\0" "vmovdqa\0" "vmovdqa32\0" "vmovdqa64\0" "vmovdqu\0"
  "vmovdqu16\0" "vmovdqu32\0" "vmovdqu64\0" "vmovdqu8\0" "vmovhlps\0"
  "vmovhpd\0" "vmovhps\0" "vmovlhps\0" "vmovlpd\0" "vmovlps\0" "vmovmskpd\0"
  "vmovmskps\0" "vmovntdq\0" "vmovntdqa\0" "vmovntpd\0" "vmovntps\0" "vmovq\0"
  "vmovsd\0" "vmovshdup\0" "vmovsldup\0" "vmovss\0" "vmovupd\0" "vmovups\0"
  "vmpsadbw\0" "vmulpd\0" "vmulps\0" "vmulsd\0" "vmulss\0" "vorpd\0" "vorps\0"
  "vpabsb\0" "vpabsd\0" "vpabsq\0" "vpabsw\0" "vpackssdw\0" "vpacksswb\0"
  "vpackusdw\0" "vpackuswb\0" "vpaddb\0" "vpaddd\0" "vpaddq\0" "vpaddsb\0"
  "vpaddsw\0" "vpaddusb\0" "vpaddusw\0" "vpaddw\0" "vpalignr\0" "vpand\0"
  "vpandd\0" "vpandn\0" "vpandnd\0" "vpandnq\0" "vpandq\0" "vpavgb\0"
  "vpavgw\0" "vpblendd\0" "vpblendvb\0" "vpblendw\0" "vpbroadcastb\0"
  "vpbroadcastd\0" "vpbroadcastmb2d\0" "vpbroadcastmb2q\0" "vpbroadcastq\0"
  "vpbroadcastw\0" "vpclmulqdq\0" "vpcmov\0" "vpcmpb\0" "vpcmpd\0" "vpcmpeqb\0"
  "vpcmpeqd\0" "vpcmpeqq\0" "vpcmpeqw\0" "vpcmpestri\0" "vpcmpestrm\0"
  "vpcmpgtb\0" "vpcmpgtd\0" "vpcmpgtq\0" "vpcmpgtw\0" "vpcmpistri\0"
  "vpcmpistrm\0" "vpcmpq\0" "vpcmpub\0" "vpcmpud\0" "vpcmpuq\0" "vpcmpuw\0"
  "vpcmpw\0" "vpcomb\0" "vpcomd\0" "vpcompressd\0" "vpcompressq\0" "vpcomq\0"
  "vpcomub\0" "vpcomud\0" "vpcomuq\0" "vpcomuw\0" "vpcomw\0" "vpconflictd\0"
  "vpconflictq\0" "vperm2f128\0" "vperm2i128\0" "vpermb\0" "vpermd\0"
  "vpermi2b\0" "vpermi2d\0" "vpermi2pd\0" "vpermi2ps\0" "vpermi2q\0"
  "vpermi2w\0" "vpermil2pd\0" "vpermil2ps\0" "vpermilpd\0" "vpermilps\0"
  "vpermpd\0" "vpermps\0" "vpermq\0" "vpermt2b\0" "vpermt2d\0" "vpermt2pd\0"
  "vpermt2ps\0" "vpermt2q\0" "vpermt2w\0" "vpermw\0" "vpexpandd\0"
  "vpexpandq\0" "vpextrb\0" "vpextrd\0" "vpextrq\0" "vpextrw\0" "vpgatherdd\0"
  "vpgatherdq\0" "vpgatherqd\0" "vpgatherqq\0" "vphaddbd\0" "vphaddbq\0"
  "vphaddbw\0" "vphaddd\0" "vphadddq\0" "vphaddsw\0" "vphaddubd\0"
  "vphaddubq\0" "vphaddubw\0" "vphaddudq\0" "vphadduwd\0" "vphadduwq\0"
  "vphaddw\0" "vphaddwd\0" "vphaddwq\0" "vphminposuw\0" "vphsubbw\0"
  "vphsubd\0" "vphsubdq\0" "vphsubsw\0" "vphsubw\0" "vphsubwd\0" "vpinsrb\0"
  "vpinsrd\0" "vpinsrq\0" "vpinsrw\0" "vplzcntd\0" "vplzcntq\0" "vpmacsdd\0"
  "vpmacsdqh\0" "vpmacsdql\0" "vpmacssdd\0" "vpmacssdqh\0" "vpmacssdql\0"
  "vpmacsswd\0" "vpmacssww\0" "vpmacswd\0" "vpmacsww\0" "vpmadcsswd\0"
  "vpmadcswd\0" "vpmadd52huq\0" "vpmadd52luq\0" "vpmaddubsw\0" "vpmaddwd\0"
  "vpmaskmovd\0" "vpmaskmovq\0" "vpmaxsb\0" "vpmaxsd\0" "vpmaxsq\0" "vpmaxsw\0"
  "vpmaxub\0" "vpmaxud\0" "vpmaxuq\0" "vpmaxuw\0" "vpminsb\0" "vpminsd\0"
  "vpminsq\0" "vpminsw\0" "vpminub\0" "vpminud\0" "vpminuq\0" "vpminuw\0"
  "vpmovb2m\0" "vpmovd2m\0" "vpmovdb\0" "vpmovdw\0" "vpmovm2b\0" "vpmovm2d\0"
  "vpmovm2q\0" "vpmovm2w\0" "vpmovmskb\0" "vpmovq2m\0" "vpmovqb\0" "vpmovqd\0"
  "vpmovqw\0" "vpmovsdb\0" "vpmovsdw\0" "vpmovsqb\0" "vpmovsqd\0" "vpmovsqw\0"
  "vpmovswb\0" "vpmovsxbd\0" "vpmovsxbq\0" "vpmovsxbw\0" "vpmovsxdq\0"
  "vpmovsxwd\0" "vpmovsxwq\0" "vpmovusdb\0" "vpmovusdw\0" "vpmovusqb\0"
  "vpmovusqd\0" "vpmovusqw\0" "vpmovuswb\0" "vpmovw2m\0" "vpmovwb\0"
  "vpmovzxbd\0" "vpmovzxbq\0" "vpmovzxbw\0" "vpmovzxdq\0" "vpmovzxwd\0"
  "vpmovzxwq\0" "vpmuldq\0" "vpmulhrsw\0" "vpmulhuw\0" "vpmulhw\0" "vpmulld\0"
  "vpmullq\0" "vpmullw\0" "vpmultishiftqb\0" "vpmuludq\0" "vpor\0" "vpord\0"
  "vporq\0" "vpperm\0" "vprold\0" "vprolq\0" "vprolvd\0" "vprolvq\0" "vprord\0"
  "vprorq\0" "vprorvd\0" "vprorvq\0" "vprotb\0" "vprotd\0" "vprotq\0"
  "vprotw\0" "vpsadbw\0" "vpscatterdd\0" "vpscatterdq\0" "vpscatterqd\0"
  "vpscatterqq\0" "vpshab\0" "vpshad\0" "vpshaq\0" "vpshaw\0" "vpshlb\0"
  "vpshld\0" "vpshlq\0" "vpshlw\0" "vpshufb\0" "vpshufd\0" "vpshufhw\0"
  "vpshuflw\0" "vpsignb\0" "vpsignd\0" "vpsignw\0" "vpslld\0" "vpslldq\0"
  "vpsllq\0" "vpsllvd\0" "vpsllvq\0" "vpsllvw\0" "vpsllw\0" "vpsrad\0"
  "vpsraq\0" "vpsravd\0" "vpsravq\0" "vpsravw\0" "vpsraw\0" "vpsrld\0"
  "vpsrldq\0" "vpsrlq\0" "vpsrlvd\0" "vpsrlvq\0" "vpsrlvw\0" "vpsrlw\0"
  "vpsubb\0" "vpsubd\0" "vpsubq\0" "vpsubsb\0" "vpsubsw\0" "vpsubusb\0"
  "vpsubusw\0" "vpsubw\0" "vpternlogd\0" "vpternlogq\0" "vptest\0" "vptestmb\0"
  "vptestmd\0" "vptestmq\0" "vptestmw\0" "vptestnmb\0" "vptestnmd\0"
  "vptestnmq\0" "vptestnmw\0" "vpunpckhbw\0" "vpunpckhdq\0" "vpunpckhqdq\0"
  "vpunpckhwd\0" "vpunpcklbw\0" "vpunpckldq\0" "vpunpcklqdq\0" "vpunpcklwd\0"
  "vpxor\0" "vpxord\0" "vpxorq\0" "vrangepd\0" "vrangeps\0" "vrangesd\0"
  "vrangess\0" "vrcp14pd\0" "vrcp14ps\0" "vrcp14sd\0" "vrcp14ss\0" "vrcp28pd\0"
  "vrcp28ps\0" "vrcp28sd\0" "vrcp28ss\0" "vrcpps\0" "vrcpss\0" "vreducepd\0"
  "vreduceps\0" "vreducesd\0" "vreducess\0" "vrndscalepd\0" "vrndscaleps\0"
  "vrndscalesd\0" "vrndscaless\0" "vroundpd\0" "vroundps\0" "vroundsd\0"
  "vroundss\0" "vrsqrt14pd\0" "vrsqrt14ps\0" "vrsqrt14sd\0" "vrsqrt14ss\0"
  "vrsqrt28pd\0" "vrsqrt28ps\0" "vrsqrt28sd\0" "vrsqrt28ss\0" "vrsqrtps\0"
  "vrsqrtss\0" "vscalefpd\0" "vscalefps\0" "vscalefsd\0" "vscalefss\0"
  "vscatterdpd\0" "vscatterdps\0" "vscatterpf0dpd\0" "vscatterpf0dps\0"
  "vscatterpf0qpd\0" "vscatterpf0qps\0" "vscatterpf1dpd\0" "vscatterpf1dps\0"
  "vscatterpf1qpd\0" "vscatterpf1qps\0" "vscatterqpd\0" "vscatterqps\0"
  "vshuff32x4\0" "vshuff64x2\0" "vshufi32x4\0" "vshufi64x2\0" "vshufpd\0"
  "vshufps\0" "vsqrtpd\0" "vsqrtps\0" "vsqrtsd\0" "vsqrtss\0" "vstmxcsr\0"
  "vsubpd\0" "vsubps\0" "vsubsd\0" "vsubss\0" "vtestpd\0" "vtestps\0"
  "vucomisd\0" "vucomiss\0" "vunpckhpd\0" "vunpckhps\0" "vunpcklpd\0"
  "vunpcklps\0" "vxorpd\0" "vxorps\0" "vzeroall\0" "vzeroupper\0" "wrfsbase\0"
  "wrgsbase\0" "xadd\0" "xgetbv\0" "xrstors\0" "xrstors64\0" "xsavec\0"
  "xsavec64\0" "xsaveopt\0" "xsaveopt64\0" "xsaves\0" "xsaves64\0" "xsetbv";

static const uint16_t _x86InstNameIndex[] = {
  0, 1, 5, 657, 4545, 4557, 4779, 4789, 4284, 4296, 10, 2790, 2798, 2810, 2818,
  2830, 2838, 2034, 6055, 2871, 2879, 3798, 3808, 15, 21, 29, 34, 40, 47, 2957,
  2966, 2975, 2985, 52, 60, 65, 71, 78, 83, 87, 91, 97, 100, 104, 108, 112,
  117, 122, 126, 130, 135, 140, 144, 148, 156, 167, 172, 179, 183, 189, 514,
  521, 196, 529, 202, 208, 215, 221, 228, 235, 536, 544, 243, 553, 250, 257,
  265, 272, 280, 287, 294, 301, 308, 314, 320, 327, 334, 340, 346, 3211, 3218,
  2292, 2305, 2318, 2331, 3225, 3232, 350, 358, 369, 9258, 9267, 379, 385, 389,
  3279, 3289, 3299, 395, 3309, 404, 413, 3361, 3371, 422, 3443, 3453, 3474,
  3484, 3494, 3504, 3525, 431, 3571, 441, 3617, 3640, 451, 455, 460, 464, 2793,
  676, 3739, 3746, 3753, 3760, 3767, 3773, 644, 468, 3953, 6769, 474, 480,
  1756, 485, 491, 496, 502, 507, 513, 520, 528, 535, 543, 552, 560, 568, 575,
  580, 586, 593, 599, 606, 611, 619, 624, 630, 636, 643, 649, 655, 661, 667,
  674, 680, 687, 692, 698, 706, 712, 717, 723, 730, 736, 743, 747, 752, 758,
  765, 772, 779, 786, 793, 799, 1798, 804, 810, 817, 824, 829, 836, 843, 851,
  858, 865, 871, 878, 884, 892, 899, 905, 912, 917, 925, 931, 935, 941, 948,
  953, 1876, 959, 1882, 965, 972, 977, 983, 990, 998, 1005, 1013, 1019, 1024,
  1029, 1037, 1047, 1054, 1063, 1071, 1077, 5308, 5316, 5324, 5332, 675, 693,
  1085, 5468, 1089, 888, 1097, 1102, 1107, 1110, 1114, 1117, 1121, 1124, 1133,
  1136, 1140, 1143, 1151, 1155, 1160, 1164, 1169, 1173, 1177, 1181, 1186, 1190,
  1195, 1199, 1203, 1207, 1211, 1214, 1217, 1221, 1225, 1228, 1127, 1147, 1231,
  1237, 1243, 1249, 1255, 1261, 1267, 1274, 1281, 1288, 1295, 1301, 1307, 7249,
  7260, 1313, 1319, 1325, 1331, 1337, 1343, 1348, 1353, 1358, 1367, 1376, 1385,
  1394, 1399, 1408, 1417, 1426, 1435, 1444, 1453, 1462, 1471, 1478, 1485, 1492,
  1499, 1508, 1517, 1526, 1533, 1540, 1547, 1554, 1560, 1566, 1572, 1578, 5478,
  5485, 1583, 1587, 1593, 2062, 2073, 2084, 2095, 1600, 5494, 7257, 5528, 5535,
  7276, 5549, 1606, 5556, 5563, 7340, 5577, 1613, 6223, 5584, 5592, 522, 7250,
  5606, 1621, 5615, 5498, 5690, 5699, 5707, 5715, 5724, 5732, 5740, 5750, 5760,
  5769, 1629, 5779, 5788, 1636, 1643, 1651, 7261, 1659, 2106, 2117, 2128, 2139,
  5803, 5810, 5820, 5830, 1667, 1673, 5837, 5845, 1680, 5853, 694, 5862, 5869,
  5876, 5883, 1686, 1691, 1697, 825, 1701, 1034, 9316, 9323, 5902, 5909, 5923,
  5930, 5940, 5950, 5960, 5970, 5977, 5984, 5991, 5999, 6007, 6016, 6025, 6032,
  6041, 6054, 1705, 6084, 1711, 6091, 6107, 6117, 6210, 6242, 6251, 6260, 6269,
  6278, 6289, 6300, 6309, 6318, 6327, 6336, 6347, 1719, 1727, 1732, 6752, 6760,
  6768, 6776, 1737, 1743, 1749, 1755, 1761, 1769, 1777, 1785, 1791, 1797, 1803,
  1810, 1818, 1824, 1833, 1842, 1849, 1858, 1866, 1875, 1881, 6855, 6872, 6941,
  6967, 6988, 7005, 7014, 1888, 1894, 7031, 7039, 7047, 7055, 7225, 7236, 7267,
  7275, 7291, 7299, 7307, 7323, 7331, 7339, 7355, 7363, 7371, 7387, 7465, 7562,
  7572, 7582, 7592, 7602, 7612, 7699, 7709, 7719, 7729, 7739, 7749, 7759, 7767,
  1900, 7777, 7786, 7794, 7810, 7833, 1908, 1912, 1917, 1924, 7842, 1929, 1938,
  1952, 1962, 3731, 8066, 8074, 8082, 8091, 1974, 8100, 8108, 8116, 8124, 8131,
  8139, 8170, 8177, 8215, 8222, 8229, 8237, 8268, 8275, 8282, 8289, 8296, 8304,
  8312, 8321, 8330, 1981, 8359, 8442, 8453, 8464, 8476, 8487, 8498, 8509, 8521,
  1988, 1993, 1999, 8532, 2005, 8660, 8667, 2009, 2013, 2022, 2031, 2038, 2045,
  2051, 2058, 2069, 2080, 2091, 2102, 2113, 2124, 2135, 2146, 2157, 2168, 2179,
  2190, 2202, 2214, 2226, 2238, 2250, 2262, 2274, 2286, 2299, 2312, 2325, 2338,
  2351, 2364, 2377, 2390, 2394, 2398, 2402, 8762, 8771, 8780, 8789, 8886, 8895,
  2407, 2412, 2416, 2420, 2425, 2344, 2357, 2370, 2383, 2429, 2434, 2440, 2445,
  2451, 2456, 2461, 2466, 2472, 2477, 2483, 2489, 2496, 2502, 2509, 2515, 2521,
  2527, 2534, 2540, 2547, 2553, 2559, 2565, 2571, 2576, 2581, 2587, 2593, 2598,
  2603, 2610, 2619, 2628, 2638, 2648, 2659, 2670, 2682, 8046, 2686, 2691, 2695,
  2700, 9156, 9164, 9172, 8887, 9188, 8896, 2705, 2710, 6147, 2714, 9204, 2150,
  2161, 2172, 2183, 732, 4287, 4299, 4975, 4985, 2718, 8360, 2725, 2731, 9257,
  9266, 2737, 9275, 9285, 9295, 9305, 2741, 2748, 2755, 2762, 2769, 2779, 2789,
  2797, 2809, 2817, 2829, 2837, 2854, 2862, 2870, 2878, 2886, 2893, 2900, 2909,
  2918, 2928, 2938, 2947, 2956, 2965, 2974, 2984, 2994, 3009, 3025, 3041, 3057,
  3073, 3089, 3104, 3120, 3136, 3152, 3168, 3184, 3197, 3210, 3217, 3224, 3231,
  3238, 3246, 3254, 3266, 3278, 3288, 3298, 3308, 3318, 3328, 3339, 3350, 3360,
  3370, 3380, 3390, 3400, 3411, 3422, 3432, 3442, 3452, 3462, 3473, 3483, 3493,
  3503, 3513, 3524, 3535, 3546, 3558, 3570, 3581, 3592, 3604, 3616, 3627, 3639,
  3650, 3662, 3673, 3684, 3695, 3706, 3717, 3728, 3738, 3745, 3752, 3759, 3766,
  3772, 3778, 3786, 3794, 3804, 3814, 3827, 3841, 3855, 3869, 3883, 3896, 3910,
  3924, 3938, 3952, 3963, 3975, 3987, 3999, 4011, 4023, 4035, 4047, 4059, 4071,
  4083, 4095, 4107, 4119, 4131, 4143, 4155, 4164, 4173, 4182, 4191, 4206, 4221,
  4236, 4251, 4266, 4281, 4293, 4305, 4317, 4329, 4341, 4353, 4365, 4377, 4389,
  4401, 4413, 4425, 4437, 4449, 4464, 4479, 4494, 4509, 4524, 4539, 4551, 4563,
  4572, 4581, 4590, 4599, 4612, 4625, 4638, 4651, 4664, 4677, 4690, 4703, 4716,
  4729, 4742, 4755, 4765, 4775, 4785, 4795, 4808, 4821, 4834, 4847, 4860, 4873,
  4886, 4899, 4912, 4925, 4938, 4951, 4961, 4971, 4981, 4991, 5002, 5013, 5024,
  5035, 5043, 5051, 5059, 5067, 5078, 5089, 5103, 5117, 5131, 5145, 5159, 5173,
  5187, 5201, 5212, 5223, 5233, 5243, 5253, 5263, 5274, 5285, 5296, 5307, 5315,
  5323, 5331, 5339, 5351, 5364, 5377, 5390, 5403, 5415, 5428, 5441, 5454, 5467,
  5477, 5484, 5493, 5505, 5516, 5527, 5534, 5541, 5548, 5555, 5562, 5569, 5576,
  5583, 5591, 5599, 5605, 5614, 5622, 5632, 5642, 5650, 5660, 5670, 5680, 5689,
  5698, 5706, 5714, 5723, 5731, 5739, 5749, 5759, 5768, 5778, 5787, 5796, 5802,
  5809, 5819, 5829, 5836, 5844, 5852, 5861, 5868, 5875, 5882, 5889, 5895, 5901,
  5908, 5915, 5922, 5929, 5939, 5949, 5959, 5969, 5976, 5983, 5990, 5998, 6006,
  6015, 6024, 6031, 6040, 6046, 6053, 6060, 6068, 6076, 6083, 6090, 6097, 6106,
  6116, 6125, 6138, 6151, 6167, 6183, 6196, 6209, 6220, 6227, 6234, 6241, 6250,
  6259, 6268, 6277, 6288, 6299, 6308, 6317, 6326, 6335, 6346, 6357, 6364, 6372,
  6380, 6388, 6396, 6403, 6410, 6417, 6429, 6441, 6448, 6456, 6464, 6472, 6480,
  6487, 6499, 6511, 6522, 6533, 6540, 6547, 6556, 6565, 6575, 6585, 6594, 6603,
  6614, 6625, 6635, 6645, 6653, 6661, 6668, 6677, 6686, 6696, 6706, 6715, 6724,
  6731, 6741, 6751, 6759, 6767, 6775, 6783, 6794, 6805, 6816, 6827, 6836, 6845,
  6854, 6862, 6871, 6880, 6890, 6900, 6910, 6920, 6930, 6940, 6948, 6957, 6966,
  6978, 6987, 6995, 7004, 7013, 7021, 7030, 7038, 7046, 7054, 7062, 7071, 7080,
  7089, 7099, 7109, 7119, 7130, 7141, 7151, 7161, 7170, 7179, 7190, 7200, 7212,
  7224, 7235, 7244, 7255, 7266, 7274, 7282, 7290, 7298, 7306, 7314, 7322, 7330,
  7338, 7346, 7354, 7362, 7370, 7378, 7386, 7394, 7403, 7412, 7420, 7428, 7437,
  7446, 7455, 7464, 7474, 7483, 7491, 7499, 7507, 7516, 7525, 7534, 7543, 7552,
  7561, 7571, 7581, 7591, 7601, 7611, 7621, 7631, 7641, 7651, 7661, 7671, 7681,
  7690, 7698, 7708, 7718, 7728, 7738, 7748, 7758, 7766, 7776, 7785, 7793, 7801,
  7809, 7817, 7832, 7841, 7846, 7852, 7858, 7865, 7872, 7879, 7887, 7895, 7902,
  7909, 7917, 7925, 7932, 7939, 7946, 7953, 7961, 7973, 7985, 7997, 8009, 8016,
  8023, 8030, 8037, 8044, 8051, 8058, 8065, 8073, 8081, 8090, 8099, 8107, 8115,
  8123, 8130, 8138, 8145, 8153, 8161, 8169, 8176, 8183, 8190, 8198, 8206, 8214,
  8221, 8228, 8236, 8243, 8251, 8259, 8267, 8274, 8281, 8288, 8295, 8303, 8311,
  8320, 8329, 8336, 8347, 8358, 8365, 8374, 8383, 8392, 8401, 8411, 8421, 8431,
  8441, 8452, 8463, 8475, 8486, 8497, 8508, 8520, 8531, 8537, 8544, 8551, 8560,
  8569, 8578, 8587, 8596, 8605, 8614, 8623, 8632, 8641, 8650, 8659, 8666, 8673,
  8683, 8693, 8703, 8713, 8725, 8737, 8749, 8761, 8770, 8779, 8788, 8797, 8808,
  8819, 8830, 8841, 8852, 8863, 8874, 8885, 8894, 8903, 8913, 8923, 8933, 8943,
  8955, 8967, 8982, 8997, 9012, 9027, 9042, 9057, 9072, 9087, 9099, 9111, 9122,
  9133, 9144, 9155, 9163, 9171, 9179, 9187, 9195, 9203, 9212, 9219, 9226, 9233,
  9240, 9248, 9256, 9265, 9274, 9284, 9294, 9304, 9314, 9321, 9328, 9337, 9348,
  9357, 9366, 353, 9371, 8533, 9315, 9322, 1030, 1038, 9378, 9386, 1048, 1055,
  9396, 9403, 9412, 9421, 9432, 9439, 9448
};

enum X86InstAlphaIndex {
  kX86InstAlphaIndexFirst   = 'a',
  kX86InstAlphaIndexLast    = 'z',
  kX86InstAlphaIndexInvalid = 0xFFFF
};

static const uint16_t _x86InstAlphaIndex[26] = {
  X86Inst::kIdAdc,
  X86Inst::kIdBextr,
  X86Inst::kIdCall,
  X86Inst::kIdDaa,
  X86Inst::kIdEmms,
  X86Inst::kIdF2xm1,
  0xFFFF,
  X86Inst::kIdHaddpd,
  X86Inst::kIdIdiv,
  X86Inst::kIdJa,
  X86Inst::kIdKaddb,
  X86Inst::kIdLahf,
  X86Inst::kIdMaskmovdqu,
  X86Inst::kIdNeg,
  X86Inst::kIdOr,
  X86Inst::kIdPabsb,
  0xFFFF,
  X86Inst::kIdRcl,
  X86Inst::kIdSahf,
  X86Inst::kIdT1mskc,
  X86Inst::kIdUcomisd,
  X86Inst::kIdVaddpd,
  X86Inst::kIdWrfsbase,
  X86Inst::kIdXadd,
  0xFFFF,
  0xFFFF
};
// ----------------------------------------------------------------------------
// ${X86InstNameData:End}

//! \internal
//!
//! Compare two instruction names.
//!
//! `a` is null terminated instruction name from `_x86InstNameData[]` table.
//! `b` is non-null terminated instruction name passed to `X86Inst::getIdByName()`.
static ASMJIT_INLINE int X86Inst_compareName(const char* a, const char* b, size_t len) noexcept {
  for (size_t i = 0; i < len; i++) {
    int c = static_cast<int>(static_cast<uint8_t>(a[i])) -
            static_cast<int>(static_cast<uint8_t>(b[i])) ;
    if (c != 0) return c;
  }

  return static_cast<int>(a[len]);
}

uint32_t X86Inst::getIdByName(const char* name, size_t len) noexcept {
  if (!name) return DebugUtils::errored(kInvalidInst);
  if (len == kInvalidIndex) len = ::strlen(name);
  if (len == 0) return DebugUtils::errored(kInvalidInst);

  uint32_t prefix = name[0] - kX86InstAlphaIndexFirst;
  if (prefix > kX86InstAlphaIndexLast - kX86InstAlphaIndexFirst)
    return DebugUtils::errored(kInvalidInst);

  uint32_t index = _x86InstAlphaIndex[prefix];
  if (index == kX86InstAlphaIndexInvalid)
    return DebugUtils::errored(kInvalidInst);

  const uint16_t* base = _x86InstNameIndex + index;
  const uint16_t* end = _x86InstNameIndex + X86Inst::_kIdCount;

  // Special handling of instructions starting with 'j' because `jcc` instruction(s)
  // are not sorted alphabetically due to suffixes that are considered part of the
  // instruction. This results in `jecxz` and `jmp` stored after all `jcc` instructions.
  bool useLinearSearch = prefix == ('j' - kX86InstAlphaIndexFirst);

  while (++prefix <= kX86InstAlphaIndexLast - kX86InstAlphaIndexFirst) {
    index = _x86InstAlphaIndex[prefix];
    if (index == kX86InstAlphaIndexInvalid)
      continue;
    end = _x86InstNameIndex + index;
    break;
  }

  if (useLinearSearch) {
    while (base != end) {
      if (X86Inst_compareName(_x86InstNameData + base[0], name, len) == 0)
        return static_cast<uint32_t>((size_t)(base - _x86InstNameIndex));
      base++;
    }
  }
  else {
    for (size_t lim = (size_t)(end - base); lim != 0; lim >>= 1) {
      const uint16_t* cur = base + (lim >> 1);
      int result = X86Inst_compareName(_x86InstNameData + cur[0], name, len);

      if (result < 0) {
        base = cur + 1;
        lim--;
        continue;
      }

      if (result > 0)
        continue;

      return static_cast<uint32_t>((size_t)(cur - _x86InstNameIndex));
    }
  }

  return DebugUtils::errored(kInvalidInst);
}

const char* X86Inst::getNameById(uint32_t id) noexcept {
  if (id >= X86Inst::_kIdCount) return nullptr;
  return _x86InstNameData + _x86InstNameIndex[id];
}
#endif // !ASMJIT_DISABLE_TEXT

// ============================================================================
// [asmjit::X86Util - Validation]
// ============================================================================

#if !defined(ASMJIT_DISABLE_VALIDATION)
// ${X86InstSignatureData:Begin}
// ------------------- Automatically generated, do not edit -------------------
#define ISIGNATURE(count, x86, x64, implicit, o0, o1, o2, o3, o4, o5) \
  { count, (x86 ? uint8_t(X86Inst::kArchMaskX86) : uint8_t(0)) | \
           (x64 ? uint8_t(X86Inst::kArchMaskX64) : uint8_t(0)) , \
    implicit, \
    0, \
    o0, o1, o2, o3, o4, o5 \
  }
static const X86Inst::ISignature _x86InstISignatureData[] = {
  ISIGNATURE(2, 1, 1, 0, 1  , 2  , 0  , 0  , 0  , 0  ), // #0   {W:r8lo|r8hi|m8, R:r8lo|r8hi|i8}
  ISIGNATURE(2, 1, 1, 0, 3  , 4  , 0  , 0  , 0  , 0  ), //      {W:r16|m16, R:r16|sreg|i16}
  ISIGNATURE(2, 1, 1, 0, 5  , 6  , 0  , 0  , 0  , 0  ), //      {W:r32|m32, R:r32|i32}
  ISIGNATURE(2, 0, 1, 0, 7  , 8  , 0  , 0  , 0  , 0  ), //      {W:r64|m64, R:r64|sreg|i32}
  ISIGNATURE(2, 1, 1, 0, 9  , 10 , 0  , 0  , 0  , 0  ), //      {W:r8lo|r8hi, R:r8lo|r8hi|m8|i8}
  ISIGNATURE(2, 1, 1, 0, 11 , 12 , 0  , 0  , 0  , 0  ), //      {W:r16|sreg, R:r16|m16}
  ISIGNATURE(2, 1, 1, 0, 13 , 14 , 0  , 0  , 0  , 0  ), //      {W:r32, R:r32|m32|i32}
  ISIGNATURE(2, 0, 1, 0, 15 , 16 , 0  , 0  , 0  , 0  ), //      {W:r64|sreg, R:r64|m64}
  ISIGNATURE(2, 1, 1, 0, 17 , 18 , 0  , 0  , 0  , 0  ), //      {W:r16, R:i16}
  ISIGNATURE(2, 0, 1, 0, 19 , 20 , 0  , 0  , 0  , 0  ), //      {W:r64, R:i64|creg|dreg}
  ISIGNATURE(2, 1, 0, 0, 13 , 21 , 0  , 0  , 0  , 0  ), //      {W:r32, R:creg|dreg}
  ISIGNATURE(2, 1, 0, 0, 22 , 23 , 0  , 0  , 0  , 0  ), //      {W:creg|dreg, R:r32}
  ISIGNATURE(2, 0, 1, 0, 22 , 24 , 0  , 0  , 0  , 0  ), //      {W:creg|dreg, R:r64}
  ISIGNATURE(2, 1, 1, 0, 25 , 26 , 0  , 0  , 0  , 0  ), // #13  {X:r8lo|r8hi|m8|r16|m16|r32|m32|r64|m64, R:i8}
  ISIGNATURE(2, 1, 1, 0, 27 , 28 , 0  , 0  , 0  , 0  ), //      {X:r16|m16, R:i16|r16}
  ISIGNATURE(2, 1, 1, 0, 29 , 30 , 0  , 0  , 0  , 0  ), //      {X:r32|m32|r64|m64, R:i32}
  ISIGNATURE(2, 1, 1, 0, 31 , 32 , 0  , 0  , 0  , 0  ), //      {X:r8lo|r8hi|m8, R:r8lo|r8hi}
  ISIGNATURE(2, 1, 1, 0, 33 , 23 , 0  , 0  , 0  , 0  ), //      {X:r32|m32, R:r32}
  ISIGNATURE(2, 0, 1, 0, 34 , 24 , 0  , 0  , 0  , 0  ), //      {X:r64|m64, R:r64}
  ISIGNATURE(2, 1, 1, 0, 35 , 36 , 0  , 0  , 0  , 0  ), //      {X:r8lo|r8hi, R:r8lo|r8hi|m8}
  ISIGNATURE(2, 1, 1, 0, 37 , 12 , 0  , 0  , 0  , 0  ), // #20  {X:r16, R:r16|m16}
  ISIGNATURE(2, 1, 1, 0, 38 , 39 , 0  , 0  , 0  , 0  ), // #21  {X:r32, R:r32|m32}
  ISIGNATURE(2, 0, 1, 0, 40 , 16 , 0  , 0  , 0  , 0  ), //      {X:r64, R:r64|m64}
  ISIGNATURE(2, 1, 1, 0, 41 , 26 , 0  , 0  , 0  , 0  ), // #23  {R:r8lo|r8hi|m8|r16|m16|r32|m32|r64|m64, R:i8}
  ISIGNATURE(2, 1, 1, 0, 12 , 28 , 0  , 0  , 0  , 0  ), //      {R:r16|m16, R:i16|r16}
  ISIGNATURE(2, 1, 1, 0, 42 , 30 , 0  , 0  , 0  , 0  ), //      {R:r32|m32|r64|m64, R:i32}
  ISIGNATURE(2, 1, 1, 0, 36 , 32 , 0  , 0  , 0  , 0  ), //      {R:r8lo|r8hi|m8, R:r8lo|r8hi}
  ISIGNATURE(2, 1, 1, 0, 39 , 23 , 0  , 0  , 0  , 0  ), //      {R:r32|m32, R:r32}
  ISIGNATURE(2, 0, 1, 0, 16 , 24 , 0  , 0  , 0  , 0  ), //      {R:r64|m64, R:r64}
  ISIGNATURE(2, 1, 1, 0, 32 , 36 , 0  , 0  , 0  , 0  ), //      {R:r8lo|r8hi, R:r8lo|r8hi|m8}
  ISIGNATURE(2, 1, 1, 0, 43 , 12 , 0  , 0  , 0  , 0  ), //      {R:r16, R:r16|m16}
  ISIGNATURE(2, 1, 1, 0, 23 , 39 , 0  , 0  , 0  , 0  ), //      {R:r32, R:r32|m32}
  ISIGNATURE(2, 0, 1, 0, 24 , 16 , 0  , 0  , 0  , 0  ), //      {R:r64, R:r64|m64}
  ISIGNATURE(2, 1, 1, 1, 44 , 36 , 0  , 0  , 0  , 0  ), // #33  {X:<ax>, R:r8lo|r8hi|m8}
  ISIGNATURE(3, 1, 1, 2, 45 , 44 , 12 , 0  , 0  , 0  ), //      {W:<dx>, X:<ax>, R:r16|m16}
  ISIGNATURE(3, 1, 1, 2, 46 , 47 , 39 , 0  , 0  , 0  ), //      {W:<edx>, X:<eax>, R:r32|m32}
  ISIGNATURE(3, 0, 1, 2, 48 , 49 , 16 , 0  , 0  , 0  ), //      {W:<rdx>, X:<rax>, R:r64|m64}
  ISIGNATURE(2, 1, 1, 0, 37 , 50 , 0  , 0  , 0  , 0  ), //      {X:r16, R:r16|m16|i8|i16}
  ISIGNATURE(2, 1, 1, 0, 38 , 51 , 0  , 0  , 0  , 0  ), //      {X:r32, R:r32|m32|i8|i32}
  ISIGNATURE(2, 0, 1, 0, 40 , 52 , 0  , 0  , 0  , 0  ), //      {X:r64, R:r64|m64|i8|i32}
  ISIGNATURE(3, 1, 1, 0, 17 , 12 , 53 , 0  , 0  , 0  ), //      {W:r16, R:r16|m16, R:i8|i16}
  ISIGNATURE(3, 1, 1, 0, 13 , 39 , 54 , 0  , 0  , 0  ), //      {W:r32, R:r32|m32, R:i8|i32}
  ISIGNATURE(3, 0, 1, 0, 19 , 16 , 54 , 0  , 0  , 0  ), //      {W:r64, R:r64|m64, R:i8|i32}
  ISIGNATURE(2, 1, 1, 0, 27 , 37 , 0  , 0  , 0  , 0  ), // #43  {X:r16|m16, X:r16}
  ISIGNATURE(2, 1, 1, 0, 33 , 38 , 0  , 0  , 0  , 0  ), //      {X:r32|m32, X:r32}
  ISIGNATURE(2, 0, 1, 0, 34 , 40 , 0  , 0  , 0  , 0  ), //      {X:r64|m64, X:r64}
  ISIGNATURE(2, 1, 1, 0, 37 , 27 , 0  , 0  , 0  , 0  ), //      {X:r16, X:r16|m16}
  ISIGNATURE(2, 1, 1, 0, 38 , 33 , 0  , 0  , 0  , 0  ), //      {X:r32, X:r32|m32}
  ISIGNATURE(2, 0, 1, 0, 40 , 34 , 0  , 0  , 0  , 0  ), //      {X:r64, X:r64|m64}
  ISIGNATURE(2, 1, 1, 0, 31 , 35 , 0  , 0  , 0  , 0  ), //      {X:r8lo|r8hi|m8, X:r8lo|r8hi}
  ISIGNATURE(2, 1, 1, 0, 35 , 31 , 0  , 0  , 0  , 0  ), //      {X:r8lo|r8hi, X:r8lo|r8hi|m8}
  ISIGNATURE(2, 1, 1, 0, 17 , 55 , 0  , 0  , 0  , 0  ), // #51  {W:r16, R:m16}
  ISIGNATURE(2, 1, 1, 0, 13 , 56 , 0  , 0  , 0  , 0  ), //      {W:r32, R:m32}
  ISIGNATURE(2, 0, 1, 0, 19 , 57 , 0  , 0  , 0  , 0  ), //      {W:r64, R:m64}
  ISIGNATURE(2, 1, 1, 0, 58 , 43 , 0  , 0  , 0  , 0  ), //      {W:m16, R:r16}
  ISIGNATURE(2, 1, 1, 0, 59 , 23 , 0  , 0  , 0  , 0  ), // #55  {W:m32, R:r32}
  ISIGNATURE(2, 0, 1, 0, 60 , 24 , 0  , 0  , 0  , 0  ), //      {W:m64, R:r64}
  ISIGNATURE(2, 1, 1, 0, 61 , 62 , 0  , 0  , 0  , 0  ), // #57  {W:mm, R:mm|m64|r64|xmm}
  ISIGNATURE(2, 1, 1, 0, 63 , 64 , 0  , 0  , 0  , 0  ), //      {W:mm|m64|r64|xmm, R:mm}
  ISIGNATURE(2, 0, 1, 0, 7  , 65 , 0  , 0  , 0  , 0  ), //      {W:r64|m64, R:xmm}
  ISIGNATURE(2, 0, 1, 0, 66 , 16 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:r64|m64}
  ISIGNATURE(2, 1, 1, 0, 66 , 67 , 0  , 0  , 0  , 0  ), // #61  {W:xmm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 68 , 65 , 0  , 0  , 0  , 0  ), //      {W:xmm|m64, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 69 , 0  , 0  , 0  , 0  ), // #63  {W:xmm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 70 , 65 , 0  , 0  , 0  , 0  ), //      {W:xmm|m128, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 71 , 72 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:ymm|m256}
  ISIGNATURE(2, 1, 1, 0, 73 , 74 , 0  , 0  , 0  , 0  ), //      {W:ymm|m256, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 75 , 76 , 0  , 0  , 0  , 0  ), // #67  {W:zmm, R:zmm|m512}
  ISIGNATURE(2, 1, 1, 0, 77 , 78 , 0  , 0  , 0  , 0  ), //      {W:zmm|m512, R:zmm}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 69 , 0  , 0  , 0  ), // #69  {W:xmm, R:xmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 66 , 69 , 26 , 0  , 0  , 0  ), // #70  {W:xmm, R:xmm|m128, R:i8}
  ISIGNATURE(3, 1, 1, 0, 71 , 74 , 72 , 0  , 0  , 0  ), // #71  {W:ymm, R:ymm, R:ymm|m256}
  ISIGNATURE(3, 1, 1, 0, 71 , 72 , 26 , 0  , 0  , 0  ), // #72  {W:ymm, R:ymm|m256, R:i8}
  ISIGNATURE(3, 1, 1, 0, 75 , 78 , 76 , 0  , 0  , 0  ), //      {W:zmm, R:zmm, R:zmm|m512}
  ISIGNATURE(3, 1, 1, 0, 75 , 76 , 26 , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512, R:i8}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 79 , 0  , 0  , 0  ), // #75  {W:xmm, R:xmm, R:i8|xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 71 , 74 , 79 , 0  , 0  , 0  ), //      {W:ymm, R:ymm, R:i8|xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 66 , 69 , 26 , 0  , 0  , 0  ), // #77  {W:xmm, R:xmm|m128, R:i8}
  ISIGNATURE(3, 1, 1, 0, 71 , 72 , 26 , 0  , 0  , 0  ), //      {W:ymm, R:ymm|m256, R:i8}
  ISIGNATURE(3, 1, 1, 0, 75 , 78 , 69 , 0  , 0  , 0  ), //      {W:zmm, R:zmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 75 , 76 , 26 , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512, R:i8}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 69 , 0  , 0  , 0  ), // #81  {W:xmm, R:xmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 66 , 69 , 26 , 0  , 0  , 0  ), //      {W:xmm, R:xmm|m128, R:i8}
  ISIGNATURE(3, 1, 1, 0, 71 , 74 , 69 , 0  , 0  , 0  ), //      {W:ymm, R:ymm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 71 , 72 , 26 , 0  , 0  , 0  ), //      {W:ymm, R:ymm|m256, R:i8}
  ISIGNATURE(3, 1, 1, 0, 75 , 78 , 69 , 0  , 0  , 0  ), //      {W:zmm, R:zmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 75 , 76 , 26 , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512, R:i8}
  ISIGNATURE(2, 1, 1, 0, 36 , 2  , 0  , 0  , 0  , 0  ), // #87  {R:r8lo|r8hi|m8, R:i8|r8lo|r8hi}
  ISIGNATURE(2, 1, 1, 0, 12 , 28 , 0  , 0  , 0  , 0  ), //      {R:r16|m16, R:i16|r16}
  ISIGNATURE(2, 1, 1, 0, 42 , 30 , 0  , 0  , 0  , 0  ), //      {R:r32|m32|r64|m64, R:i32}
  ISIGNATURE(2, 1, 1, 0, 39 , 23 , 0  , 0  , 0  , 0  ), //      {R:r32|m32, R:r32}
  ISIGNATURE(2, 0, 1, 0, 16 , 24 , 0  , 0  , 0  , 0  ), //      {R:r64|m64, R:r64}
  ISIGNATURE(3, 1, 1, 0, 66 , 80 , 65 , 0  , 0  , 0  ), // #92  {W:xmm, R:vm32x, R:xmm}
  ISIGNATURE(3, 1, 1, 0, 71 , 80 , 74 , 0  , 0  , 0  ), //      {W:ymm, R:vm32x, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 66 , 80 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:vm32x}
  ISIGNATURE(2, 1, 1, 0, 71 , 81 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:vm32y}
  ISIGNATURE(2, 1, 1, 0, 75 , 82 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:vm32z}
  ISIGNATURE(3, 1, 1, 0, 66 , 80 , 65 , 0  , 0  , 0  ), // #97  {W:xmm, R:vm32x, R:xmm}
  ISIGNATURE(3, 1, 1, 0, 71 , 81 , 74 , 0  , 0  , 0  ), //      {W:ymm, R:vm32y, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 66 , 80 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:vm32x}
  ISIGNATURE(2, 1, 1, 0, 71 , 81 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:vm32y}
  ISIGNATURE(2, 1, 1, 0, 75 , 82 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:vm32z}
  ISIGNATURE(3, 1, 1, 0, 66 , 83 , 65 , 0  , 0  , 0  ), // #102 {W:xmm, R:vm64x, R:xmm}
  ISIGNATURE(3, 1, 1, 0, 71 , 84 , 74 , 0  , 0  , 0  ), //      {W:ymm, R:vm64y, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 66 , 83 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:vm64x}
  ISIGNATURE(2, 1, 1, 0, 71 , 84 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:vm64y}
  ISIGNATURE(2, 1, 1, 0, 75 , 85 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:vm64z}
  ISIGNATURE(3, 1, 1, 1, 31 , 32 , 86 , 0  , 0  , 0  ), // #107 {X:r8lo|r8hi|m8, R:r8lo|r8hi, R:<al>}
  ISIGNATURE(3, 1, 1, 1, 27 , 43 , 87 , 0  , 0  , 0  ), //      {X:r16|m16, R:r16, R:<ax>}
  ISIGNATURE(3, 1, 1, 1, 33 , 23 , 88 , 0  , 0  , 0  ), //      {X:r32|m32, R:r32, R:<eax>}
  ISIGNATURE(3, 0, 1, 1, 34 , 24 , 89 , 0  , 0  , 0  ), //      {X:r64|m64, R:r64, R:<rax>}
  ISIGNATURE(2, 1, 1, 1, 44 , 36 , 0  , 0  , 0  , 0  ), // #111 {X:<ax>, R:r8lo|r8hi|m8}
  ISIGNATURE(3, 1, 1, 2, 44 , 90 , 12 , 0  , 0  , 0  ), //      {X:<ax>, X:<dx>, R:r16|m16}
  ISIGNATURE(3, 1, 1, 2, 47 , 91 , 39 , 0  , 0  , 0  ), //      {X:<eax>, X:<edx>, R:r32|m32}
  ISIGNATURE(3, 0, 1, 2, 49 , 92 , 16 , 0  , 0  , 0  ), //      {X:<rax>, X:<rdx>, R:r64|m64}
  ISIGNATURE(2, 1, 1, 1, 44 , 36 , 0  , 0  , 0  , 0  ), // #115 {X:<ax>, R:r8lo|r8hi|m8}
  ISIGNATURE(3, 1, 1, 2, 90 , 44 , 12 , 0  , 0  , 0  ), //      {X:<dx>, X:<ax>, R:r16|m16}
  ISIGNATURE(3, 1, 1, 2, 91 , 47 , 39 , 0  , 0  , 0  ), //      {X:<edx>, X:<eax>, R:r32|m32}
  ISIGNATURE(3, 0, 1, 2, 92 , 49 , 16 , 0  , 0  , 0  ), //      {X:<rdx>, X:<rax>, R:r64|m64}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 65 , 69 , 0  , 0  ), // #119 {W:xmm, R:xmm, R:xmm, R:xmm|m128}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 69 , 65 , 0  , 0  ), // #120 {W:xmm, R:xmm, R:xmm|m128, R:xmm}
  ISIGNATURE(4, 1, 1, 0, 71 , 74 , 74 , 72 , 0  , 0  ), //      {W:ymm, R:ymm, R:ymm, R:ymm|m256}
  ISIGNATURE(4, 1, 1, 0, 71 , 74 , 72 , 74 , 0  , 0  ), //      {W:ymm, R:ymm, R:ymm|m256, R:ymm}
  ISIGNATURE(3, 1, 1, 0, 66 , 93 , 65 , 0  , 0  , 0  ), // #123 {W:xmm, R:vm64x|vm64y, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 83 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:vm64x}
  ISIGNATURE(2, 1, 1, 0, 71 , 84 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:vm64y}
  ISIGNATURE(2, 1, 1, 0, 75 , 85 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:vm64z}
  ISIGNATURE(3, 1, 1, 0, 94 , 65 , 65 , 0  , 0  , 0  ), // #127 {W:m128, R:xmm, R:xmm}
  ISIGNATURE(3, 1, 1, 0, 95 , 74 , 74 , 0  , 0  , 0  ), //      {W:m256, R:ymm, R:ymm}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 96 , 0  , 0  , 0  ), //      {W:xmm, R:xmm, R:m128}
  ISIGNATURE(3, 1, 1, 0, 71 , 74 , 97 , 0  , 0  , 0  ), //      {W:ymm, R:ymm, R:m256}
  ISIGNATURE(5, 1, 1, 0, 66 , 65 , 69 , 65 , 98 , 0  ), // #131 {W:xmm, R:xmm, R:xmm|m128, R:xmm, R:i4}
  ISIGNATURE(5, 1, 1, 0, 66 , 65 , 65 , 69 , 98 , 0  ), //      {W:xmm, R:xmm, R:xmm, R:xmm|m128, R:i4}
  ISIGNATURE(5, 1, 1, 0, 71 , 74 , 72 , 74 , 98 , 0  ), //      {W:ymm, R:ymm, R:ymm|m256, R:ymm, R:i4}
  ISIGNATURE(5, 1, 1, 0, 71 , 74 , 74 , 72 , 98 , 0  ), //      {W:ymm, R:ymm, R:ymm, R:ymm|m256, R:i4}
  ISIGNATURE(3, 1, 1, 0, 71 , 72 , 26 , 0  , 0  , 0  ), // #135 {W:ymm, R:ymm|m256, R:i8}
  ISIGNATURE(3, 1, 1, 0, 71 , 74 , 72 , 0  , 0  , 0  ), // #136 {W:ymm, R:ymm, R:ymm|m256}
  ISIGNATURE(3, 1, 1, 0, 75 , 78 , 76 , 0  , 0  , 0  ), //      {W:zmm, R:zmm, R:zmm|m512}
  ISIGNATURE(3, 1, 1, 0, 75 , 76 , 26 , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512, R:i8}
  ISIGNATURE(2, 1, 1, 0, 31 , 35 , 0  , 0  , 0  , 0  ), // #139 {X:r8lo|r8hi|m8, X:r8lo|r8hi}
  ISIGNATURE(2, 1, 1, 0, 27 , 37 , 0  , 0  , 0  , 0  ), //      {X:r16|m16, X:r16}
  ISIGNATURE(2, 1, 1, 0, 33 , 38 , 0  , 0  , 0  , 0  ), //      {X:r32|m32, X:r32}
  ISIGNATURE(2, 0, 1, 0, 34 , 40 , 0  , 0  , 0  , 0  ), //      {X:r64|m64, X:r64}
  ISIGNATURE(2, 1, 1, 0, 12 , 99 , 0  , 0  , 0  , 0  ), // #143 {R:r16|m16, R:r16|i8}
  ISIGNATURE(2, 1, 1, 0, 39 , 100, 0  , 0  , 0  , 0  ), //      {R:r32|m32, R:r32|i8}
  ISIGNATURE(2, 0, 1, 0, 16 , 101, 0  , 0  , 0  , 0  ), //      {R:r64|m64, R:r64|i8}
  ISIGNATURE(2, 1, 1, 0, 27 , 99 , 0  , 0  , 0  , 0  ), // #146 {X:r16|m16, R:r16|i8}
  ISIGNATURE(2, 1, 1, 0, 33 , 100, 0  , 0  , 0  , 0  ), //      {X:r32|m32, R:r32|i8}
  ISIGNATURE(2, 0, 1, 0, 34 , 101, 0  , 0  , 0  , 0  ), //      {X:r64|m64, R:r64|i8}
  ISIGNATURE(1, 1, 1, 0, 102, 0  , 0  , 0  , 0  , 0  ), // #149 {X:m32|m64}
  ISIGNATURE(2, 1, 1, 0, 103, 104, 0  , 0  , 0  , 0  ), //      {X:fp0, R:fp}
  ISIGNATURE(2, 1, 1, 0, 105, 106, 0  , 0  , 0  , 0  ), //      {X:fp, R:fp0}
  ISIGNATURE(2, 1, 1, 0, 17 , 12 , 0  , 0  , 0  , 0  ), // #152 {W:r16, R:r16|m16}
  ISIGNATURE(2, 1, 1, 0, 13 , 39 , 0  , 0  , 0  , 0  ), // #153 {W:r32, R:r32|m32}
  ISIGNATURE(2, 0, 1, 0, 19 , 16 , 0  , 0  , 0  , 0  ), //      {W:r64, R:r64|m64}
  ISIGNATURE(2, 1, 1, 2, 107, 107, 0  , 0  , 0  , 0  ), // #155 {X:<zdi>, X:<zsi>}
  ISIGNATURE(2, 1, 1, 0, 66 , 67 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 60 , 65 , 0  , 0  , 0  , 0  ), // #157 {W:m64, R:xmm}
  ISIGNATURE(3, 1, 1, 0, 27 , 43 , 108, 0  , 0  , 0  ), // #158 {X:r16|m16, R:r16, R:i8|cl}
  ISIGNATURE(3, 1, 1, 0, 33 , 23 , 108, 0  , 0  , 0  ), //      {X:r32|m32, R:r32, R:i8|cl}
  ISIGNATURE(3, 0, 1, 0, 34 , 24 , 108, 0  , 0  , 0  ), //      {X:r64|m64, R:r64, R:i8|cl}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 69 , 0  , 0  , 0  ), // #161 {W:xmm, R:xmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 71 , 74 , 72 , 0  , 0  , 0  ), //      {W:ymm, R:ymm, R:ymm|m256}
  ISIGNATURE(3, 1, 1, 0, 75 , 78 , 76 , 0  , 0  , 0  ), //      {W:zmm, R:zmm, R:zmm|m512}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 69 , 26 , 0  , 0  ), // #164 {W:xmm, R:xmm, R:xmm|m128, R:i8}
  ISIGNATURE(4, 1, 1, 0, 71 , 74 , 72 , 26 , 0  , 0  ), // #165 {W:ymm, R:ymm, R:ymm|m256, R:i8}
  ISIGNATURE(4, 1, 1, 0, 75 , 78 , 76 , 26 , 0  , 0  ), //      {W:zmm, R:zmm, R:zmm|m512, R:i8}
  ISIGNATURE(4, 1, 1, 0, 109, 65 , 69 , 26 , 0  , 0  ), // #167 {W:xmm|k, R:xmm, R:xmm|m128, R:i8}
  ISIGNATURE(4, 1, 1, 0, 110, 74 , 72 , 26 , 0  , 0  ), //      {W:ymm|k, R:ymm, R:ymm|m256, R:i8}
  ISIGNATURE(4, 1, 1, 0, 111, 78 , 76 , 26 , 0  , 0  ), //      {W:k, R:zmm, R:zmm|m512, R:i8}
  ISIGNATURE(2, 1, 1, 0, 70 , 65 , 0  , 0  , 0  , 0  ), // #170 {W:xmm|m128, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 73 , 74 , 0  , 0  , 0  , 0  ), //      {W:ymm|m256, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 77 , 78 , 0  , 0  , 0  , 0  ), //      {W:zmm|m512, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 67 , 0  , 0  , 0  , 0  ), // #173 {W:xmm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 71 , 69 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 75 , 72 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:ymm|m256}
  ISIGNATURE(2, 1, 1, 0, 66 , 69 , 0  , 0  , 0  , 0  ), // #176 {W:xmm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 71 , 72 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:ymm|m256}
  ISIGNATURE(2, 1, 1, 0, 75 , 76 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512}
  ISIGNATURE(2, 1, 1, 0, 66 , 112, 0  , 0  , 0  , 0  ), // #179 {W:xmm, R:xmm|m128|ymm|m256|m64}
  ISIGNATURE(2, 1, 1, 0, 71 , 69 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 75 , 72 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:ymm|m256}
  ISIGNATURE(3, 1, 1, 0, 68 , 65 , 26 , 0  , 0  , 0  ), // #182 {W:xmm|m64, R:xmm, R:i8}
  ISIGNATURE(3, 1, 1, 0, 70 , 74 , 26 , 0  , 0  , 0  ), // #183 {W:xmm|m128, R:ymm, R:i8}
  ISIGNATURE(3, 1, 1, 0, 73 , 78 , 26 , 0  , 0  , 0  ), // #184 {W:ymm|m256, R:zmm, R:i8}
  ISIGNATURE(4, 1, 1, 0, 113, 65 , 69 , 26 , 0  , 0  ), // #185 {X:xmm, R:xmm, R:xmm|m128, R:i8}
  ISIGNATURE(4, 1, 1, 0, 114, 74 , 72 , 26 , 0  , 0  ), //      {X:ymm, R:ymm, R:ymm|m256, R:i8}
  ISIGNATURE(4, 1, 1, 0, 115, 78 , 76 , 26 , 0  , 0  ), //      {X:zmm, R:zmm, R:zmm|m512, R:i8}
  ISIGNATURE(3, 1, 1, 0, 113, 65 , 69 , 0  , 0  , 0  ), // #188 {X:xmm, R:xmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 114, 74 , 72 , 0  , 0  , 0  ), //      {X:ymm, R:ymm, R:ymm|m256}
  ISIGNATURE(3, 1, 1, 0, 115, 78 , 76 , 0  , 0  , 0  ), //      {X:zmm, R:zmm, R:zmm|m512}
  ISIGNATURE(3, 1, 1, 0, 66 , 69 , 26 , 0  , 0  , 0  ), // #191 {W:xmm, R:xmm|m128, R:i8}
  ISIGNATURE(3, 1, 1, 0, 71 , 72 , 26 , 0  , 0  , 0  ), //      {W:ymm, R:ymm|m256, R:i8}
  ISIGNATURE(3, 1, 1, 0, 75 , 76 , 26 , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512, R:i8}
  ISIGNATURE(2, 1, 1, 0, 66 , 67 , 0  , 0  , 0  , 0  ), // #194 {W:xmm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 71 , 72 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:ymm|m256}
  ISIGNATURE(2, 1, 1, 0, 75 , 76 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512}
  ISIGNATURE(2, 1, 1, 0, 94 , 65 , 0  , 0  , 0  , 0  ), // #197 {W:m128, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 95 , 74 , 0  , 0  , 0  , 0  ), //      {W:m256, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 116, 78 , 0  , 0  , 0  , 0  ), //      {W:m512, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 96 , 0  , 0  , 0  , 0  ), // #200 {W:xmm, R:m128}
  ISIGNATURE(2, 1, 1, 0, 71 , 97 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:m256}
  ISIGNATURE(2, 1, 1, 0, 75 , 117, 0  , 0  , 0  , 0  ), //      {W:zmm, R:m512}
  ISIGNATURE(2, 0, 1, 0, 7  , 65 , 0  , 0  , 0  , 0  ), // #203 {W:r64|m64, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 118, 0  , 0  , 0  , 0  ), //      {W:xmm, R:m64|r64|xmm}
  ISIGNATURE(2, 1, 1, 0, 68 , 65 , 0  , 0  , 0  , 0  ), //      {W:xmm|m64, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 60 , 65 , 0  , 0  , 0  , 0  ), // #206 {W:m64, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 57 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:m64}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 65 , 0  , 0  , 0  ), // #208 {W:xmm, R:xmm, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 119, 65 , 0  , 0  , 0  , 0  ), // #209 {W:m32|m64, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 120, 0  , 0  , 0  , 0  ), //      {W:xmm, R:m32|m64}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 65 , 0  , 0  , 0  ), //      {W:xmm, R:xmm, R:xmm}
  ISIGNATURE(4, 1, 1, 0, 111, 65 , 69 , 26 , 0  , 0  ), // #212 {W:k, R:xmm, R:xmm|m128, R:i8}
  ISIGNATURE(4, 1, 1, 0, 111, 74 , 72 , 26 , 0  , 0  ), //      {W:k, R:ymm, R:ymm|m256, R:i8}
  ISIGNATURE(4, 1, 1, 0, 111, 78 , 76 , 26 , 0  , 0  ), //      {W:k, R:zmm, R:zmm|m512, R:i8}
  ISIGNATURE(3, 1, 1, 0, 109, 65 , 69 , 0  , 0  , 0  ), // #215 {W:xmm|k, R:xmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 110, 74 , 72 , 0  , 0  , 0  ), //      {W:ymm|k, R:ymm, R:ymm|m256}
  ISIGNATURE(3, 1, 1, 0, 111, 78 , 76 , 0  , 0  , 0  ), //      {W:k, R:zmm, R:zmm|m512}
  ISIGNATURE(2, 1, 1, 0, 121, 65 , 0  , 0  , 0  , 0  ), // #218 {W:xmm|m32, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 68 , 74 , 0  , 0  , 0  , 0  ), //      {W:xmm|m64, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 70 , 78 , 0  , 0  , 0  , 0  ), //      {W:xmm|m128, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 68 , 65 , 0  , 0  , 0  , 0  ), // #221 {W:xmm|m64, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 70 , 74 , 0  , 0  , 0  , 0  ), //      {W:xmm|m128, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 73 , 78 , 0  , 0  , 0  , 0  ), //      {W:ymm|m256, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 122, 65 , 0  , 0  , 0  , 0  ), // #224 {W:xmm|m16, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 121, 74 , 0  , 0  , 0  , 0  ), //      {W:xmm|m32, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 68 , 78 , 0  , 0  , 0  , 0  ), //      {W:xmm|m64, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 123, 0  , 0  , 0  , 0  ), // #227 {W:xmm, R:xmm|m32}
  ISIGNATURE(2, 1, 1, 0, 71 , 67 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 75 , 69 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 66 , 124, 0  , 0  , 0  , 0  ), // #230 {W:xmm, R:xmm|m16}
  ISIGNATURE(2, 1, 1, 0, 71 , 123, 0  , 0  , 0  , 0  ), //      {W:ymm, R:xmm|m32}
  ISIGNATURE(2, 1, 1, 0, 75 , 67 , 0  , 0  , 0  , 0  ), // #232 {W:zmm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 66 , 125, 0  , 0  , 0  , 0  ), // #233 {W:xmm, R:xmm|m64|m32}
  ISIGNATURE(2, 1, 1, 0, 71 , 126, 0  , 0  , 0  , 0  ), //      {W:ymm, R:xmm|m128|m64}
  ISIGNATURE(2, 1, 1, 0, 75 , 69 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 127, 65 , 0  , 0  , 0  , 0  ), // #236 {W:vm32x, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 128, 74 , 0  , 0  , 0  , 0  ), //      {W:vm32y, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 129, 78 , 0  , 0  , 0  , 0  ), //      {W:vm32z, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 130, 65 , 0  , 0  , 0  , 0  ), // #239 {W:vm64x, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 131, 74 , 0  , 0  , 0  , 0  ), //      {W:vm64y, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 132, 78 , 0  , 0  , 0  , 0  ), //      {W:vm64z, R:zmm}
  ISIGNATURE(3, 1, 1, 0, 111, 65 , 69 , 0  , 0  , 0  ), // #242 {W:k, R:xmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 111, 74 , 72 , 0  , 0  , 0  ), //      {W:k, R:ymm, R:ymm|m256}
  ISIGNATURE(3, 1, 1, 0, 111, 78 , 76 , 0  , 0  , 0  ), //      {W:k, R:zmm, R:zmm|m512}
  ISIGNATURE(3, 1, 1, 0, 13 , 23 , 39 , 0  , 0  , 0  ), // #245 {W:r32, R:r32, R:r32|m32}
  ISIGNATURE(3, 0, 1, 0, 19 , 24 , 16 , 0  , 0  , 0  ), //      {W:r64, R:r64, R:r64|m64}
  ISIGNATURE(3, 1, 1, 0, 13 , 39 , 23 , 0  , 0  , 0  ), // #247 {W:r32, R:r32|m32, R:r32}
  ISIGNATURE(3, 0, 1, 0, 19 , 16 , 24 , 0  , 0  , 0  ), //      {W:r64, R:r64|m64, R:r64}
  ISIGNATURE(1, 1, 1, 0, 133, 0  , 0  , 0  , 0  , 0  ), // #249 {X:rel32|r64|m64|i32|i64}
  ISIGNATURE(1, 1, 0, 0, 39 , 0  , 0  , 0  , 0  , 0  ), //      {R:r32|m32}
  ISIGNATURE(2, 1, 1, 2, 107, 107, 0  , 0  , 0  , 0  ), // #251 {X:<zsi>, X:<zdi>}
  ISIGNATURE(3, 1, 1, 0, 113, 67 , 26 , 0  , 0  , 0  ), //      {X:xmm, R:xmm|m64, R:i8}
  ISIGNATURE(2, 1, 1, 0, 38 , 134, 0  , 0  , 0  , 0  ), // #253 {X:r32, R:r8lo|r8hi|m8|r16|m16|r32|m32}
  ISIGNATURE(2, 0, 1, 0, 40 , 135, 0  , 0  , 0  , 0  ), //      {X:r64, R:r8lo|r8hi|m8|r64|m64}
  ISIGNATURE(1, 1, 0, 0, 136, 0  , 0  , 0  , 0  , 0  ), // #255 {X:r16|r32}
  ISIGNATURE(1, 1, 1, 0, 25 , 0  , 0  , 0  , 0  , 0  ), // #256 {X:r8lo|r8hi|m8|r16|m16|r32|m32|r64|m64}
  ISIGNATURE(3, 1, 1, 0, 113, 26 , 26 , 0  , 0  , 0  ), // #257 {X:xmm, R:i8, R:i8}
  ISIGNATURE(2, 1, 1, 0, 113, 65 , 0  , 0  , 0  , 0  ), //      {X:xmm, R:xmm}
  ISIGNATURE(0, 1, 1, 0, 0  , 0  , 0  , 0  , 0  , 0  ), // #259 {}
  ISIGNATURE(1, 1, 1, 0, 105, 0  , 0  , 0  , 0  , 0  ), // #260 {X:fp}
  ISIGNATURE(0, 1, 1, 0, 0  , 0  , 0  , 0  , 0  , 0  ), // #261 {}
  ISIGNATURE(1, 1, 1, 0, 137, 0  , 0  , 0  , 0  , 0  ), // #262 {X:m32|m64|fp}
  ISIGNATURE(2, 1, 1, 0, 113, 65 , 0  , 0  , 0  , 0  ), // #263 {X:xmm, R:xmm}
  ISIGNATURE(4, 1, 1, 0, 113, 65 , 26 , 26 , 0  , 0  ), //      {X:xmm, R:xmm, R:i8, R:i8}
  ISIGNATURE(2, 1, 0, 0, 138, 139, 0  , 0  , 0  , 0  ), // #265 {R:cx|ecx, R:rel8}
  ISIGNATURE(2, 0, 1, 0, 140, 139, 0  , 0  , 0  , 0  ), //      {R:ecx|rcx, R:rel8}
  ISIGNATURE(1, 1, 1, 0, 141, 0  , 0  , 0  , 0  , 0  ), // #267 {X:rel8|rel32|r64|m64|i32|i64}
  ISIGNATURE(1, 1, 0, 0, 39 , 0  , 0  , 0  , 0  , 0  ), //      {R:r32|m32}
  ISIGNATURE(2, 1, 1, 0, 111, 142, 0  , 0  , 0  , 0  ), // #269 {W:k, R:k|m8|r32|r64|r8lo|r8hi|r16}
  ISIGNATURE(2, 1, 1, 0, 143, 144, 0  , 0  , 0  , 0  ), //      {W:m8|r32|r64|r8lo|r8hi|r16, R:k}
  ISIGNATURE(2, 1, 1, 0, 111, 145, 0  , 0  , 0  , 0  ), // #271 {W:k, R:k|m32|r32|r64}
  ISIGNATURE(2, 1, 1, 0, 146, 144, 0  , 0  , 0  , 0  ), //      {W:m32|r32|r64, R:k}
  ISIGNATURE(2, 1, 1, 0, 111, 147, 0  , 0  , 0  , 0  ), // #273 {W:k, R:k|m64|r64}
  ISIGNATURE(2, 1, 1, 0, 7  , 144, 0  , 0  , 0  , 0  ), //      {W:m64|r64, R:k}
  ISIGNATURE(2, 1, 1, 0, 111, 148, 0  , 0  , 0  , 0  ), // #275 {W:k, R:k|m16|r32|r64|r16}
  ISIGNATURE(2, 1, 1, 0, 149, 144, 0  , 0  , 0  , 0  ), //      {W:m16|r32|r64|r16, R:k}
  ISIGNATURE(2, 1, 1, 0, 150, 151, 0  , 0  , 0  , 0  ), // #277 {W:mm|xmm, R:r32|m32|r64}
  ISIGNATURE(2, 1, 1, 0, 146, 152, 0  , 0  , 0  , 0  ), //      {W:r32|m32|r64, R:mm|xmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 123, 0  , 0  , 0  , 0  ), // #279 {W:xmm, R:xmm|m32}
  ISIGNATURE(2, 1, 1, 0, 59 , 65 , 0  , 0  , 0  , 0  ), // #280 {W:m32, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 153, 36 , 0  , 0  , 0  , 0  ), // #281 {W:r16|r32|r64, R:r8lo|r8hi|m8}
  ISIGNATURE(2, 1, 1, 0, 154, 12 , 0  , 0  , 0  , 0  ), //      {W:r32|r64, R:r16|m16}
  ISIGNATURE(4, 1, 1, 1, 13 , 13 , 39 , 155, 0  , 0  ), // #283 {W:r32, W:r32, R:r32|m32, R:<edx>}
  ISIGNATURE(4, 0, 1, 1, 19 , 19 , 16 , 156, 0  , 0  ), //      {W:r64, W:r64, R:r64|m64, R:<rdx>}
  ISIGNATURE(0, 1, 1, 0, 0  , 0  , 0  , 0  , 0  , 0  ), // #285 {}
  ISIGNATURE(1, 1, 1, 0, 157, 0  , 0  , 0  , 0  , 0  ), //      {R:r16|m16|r32|m32}
  ISIGNATURE(2, 1, 1, 0, 158, 159, 0  , 0  , 0  , 0  ), // #287 {X:mm, R:mm|m64}
  ISIGNATURE(2, 1, 1, 0, 113, 69 , 0  , 0  , 0  , 0  ), // #288 {X:xmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 158, 159, 26 , 0  , 0  , 0  ), // #289 {X:mm, R:mm|m64, R:i8}
  ISIGNATURE(3, 1, 1, 0, 113, 69 , 26 , 0  , 0  , 0  ), // #290 {X:xmm, R:xmm|m128, R:i8}
  ISIGNATURE(3, 1, 1, 0, 154, 64 , 26 , 0  , 0  , 0  ), // #291 {W:r32|r64, R:mm, R:i8}
  ISIGNATURE(3, 1, 1, 0, 149, 65 , 26 , 0  , 0  , 0  ), // #292 {W:r32|r64|m16|r16, R:xmm, R:i8}
  ISIGNATURE(1, 1, 1, 0, 160, 0  , 0  , 0  , 0  , 0  ), // #293 {W:r16|m16|r64|m64|fs|gs}
  ISIGNATURE(1, 1, 0, 0, 5  , 0  , 0  , 0  , 0  , 0  ), //      {W:r32|m32|ds|es|ss}
  ISIGNATURE(2, 1, 1, 0, 61 , 159, 0  , 0  , 0  , 0  ), // #295 {W:mm, R:mm|m64}
  ISIGNATURE(2, 1, 1, 0, 66 , 69 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 158, 161, 0  , 0  , 0  , 0  ), // #297 {X:mm, R:i8|mm|m64}
  ISIGNATURE(2, 1, 1, 0, 113, 79 , 0  , 0  , 0  , 0  ), //      {X:xmm, R:i8|xmm|m128}
  ISIGNATURE(1, 1, 1, 0, 162, 0  , 0  , 0  , 0  , 0  ), // #299 {X:r16|m16|r64|m64|i8|i16|i32|fs|gs}
  ISIGNATURE(1, 1, 0, 0, 39 , 0  , 0  , 0  , 0  , 0  ), //      {R:r32|m32|cs|ss|ds|es}
  ISIGNATURE(0, 1, 1, 0, 0  , 0  , 0  , 0  , 0  , 0  ), // #301 {}
  ISIGNATURE(1, 1, 1, 0, 163, 0  , 0  , 0  , 0  , 0  ), //      {X:i16}
  ISIGNATURE(3, 1, 1, 0, 13 , 39 , 26 , 0  , 0  , 0  ), // #303 {W:r32, R:r32|m32, R:i8}
  ISIGNATURE(3, 0, 1, 0, 19 , 16 , 26 , 0  , 0  , 0  ), //      {W:r64, R:r64|m64, R:i8}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 69 , 65 , 0  , 0  ), // #305 {W:xmm, R:xmm, R:xmm|m128, R:xmm}
  ISIGNATURE(4, 1, 1, 0, 71 , 74 , 72 , 74 , 0  , 0  ), //      {W:ymm, R:ymm, R:ymm|m256, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 66 , 164, 0  , 0  , 0  , 0  ), // #307 {W:xmm, R:xmm|m128|ymm|m256}
  ISIGNATURE(2, 1, 1, 0, 71 , 76 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:zmm|m512}
  ISIGNATURE(2, 1, 1, 0, 154, 123, 0  , 0  , 0  , 0  ), // #309 {W:r32|r64, R:xmm|m32}
  ISIGNATURE(2, 0, 1, 0, 19 , 67 , 0  , 0  , 0  , 0  ), //      {W:r64, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 13 , 123, 0  , 0  , 0  , 0  ), // #311 {W:r32, R:xmm|m32}
  ISIGNATURE(2, 0, 1, 0, 19 , 67 , 0  , 0  , 0  , 0  ), //      {W:r64, R:xmm|m64}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 65 , 67 , 0  , 0  ), // #313 {W:xmm, R:xmm, R:xmm, R:xmm|m64}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 67 , 65 , 0  , 0  ), //      {W:xmm, R:xmm, R:xmm|m64, R:xmm}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 65 , 123, 0  , 0  ), // #315 {W:xmm, R:xmm, R:xmm, R:xmm|m32}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 123, 65 , 0  , 0  ), //      {W:xmm, R:xmm, R:xmm|m32, R:xmm}
  ISIGNATURE(4, 1, 1, 0, 71 , 74 , 69 , 26 , 0  , 0  ), // #317 {W:ymm, R:ymm, R:xmm|m128, R:i8}
  ISIGNATURE(4, 1, 1, 0, 75 , 78 , 69 , 26 , 0  , 0  ), //      {W:zmm, R:zmm, R:xmm|m128, R:i8}
  ISIGNATURE(2, 1, 1, 0, 146, 65 , 0  , 0  , 0  , 0  ), // #319 {W:r32|m32|r64, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 151, 0  , 0  , 0  , 0  ), //      {W:xmm, R:r32|m32|r64}
  ISIGNATURE(2, 1, 1, 0, 60 , 65 , 0  , 0  , 0  , 0  ), // #321 {W:m64, R:xmm}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 57 , 0  , 0  , 0  ), //      {W:xmm, R:xmm, R:m64}
  ISIGNATURE(2, 1, 1, 0, 165, 166, 0  , 0  , 0  , 0  ), // #323 {W:xmm|ymm|zmm, R:xmm|m8}
  ISIGNATURE(2, 1, 1, 0, 165, 167, 0  , 0  , 0  , 0  ), //      {W:xmm|ymm|zmm, R:r32|r64}
  ISIGNATURE(2, 1, 1, 0, 165, 123, 0  , 0  , 0  , 0  ), // #325 {W:xmm|ymm|zmm, R:xmm|m32}
  ISIGNATURE(2, 1, 1, 0, 165, 167, 0  , 0  , 0  , 0  ), //      {W:xmm|ymm|zmm, R:r32|r64}
  ISIGNATURE(2, 1, 1, 0, 165, 124, 0  , 0  , 0  , 0  ), // #327 {W:xmm|ymm|zmm, R:xmm|m16}
  ISIGNATURE(2, 1, 1, 0, 165, 167, 0  , 0  , 0  , 0  ), //      {W:xmm|ymm|zmm, R:r32|r64}
  ISIGNATURE(3, 1, 1, 0, 66 , 168, 26 , 0  , 0  , 0  ), // #329 {W:xmm, R:r32|m8|r64|r8lo|r8hi|r16, R:i8}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 168, 26 , 0  , 0  ), //      {W:xmm, R:xmm, R:r32|m8|r64|r8lo|r8hi|r16, R:i8}
  ISIGNATURE(3, 1, 1, 0, 66 , 151, 26 , 0  , 0  , 0  ), // #331 {W:xmm, R:r32|m32|r64, R:i8}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 151, 26 , 0  , 0  ), //      {W:xmm, R:xmm, R:r32|m32|r64, R:i8}
  ISIGNATURE(3, 0, 1, 0, 66 , 16 , 26 , 0  , 0  , 0  ), // #333 {W:xmm, R:r64|m64, R:i8}
  ISIGNATURE(4, 0, 1, 0, 66 , 65 , 16 , 26 , 0  , 0  ), //      {W:xmm, R:xmm, R:r64|m64, R:i8}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 69 , 0  , 0  , 0  ), // #335 {W:xmm, R:xmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 66 , 69 , 169, 0  , 0  , 0  ), //      {W:xmm, R:xmm|m128, R:i8|xmm}
  ISIGNATURE(2, 1, 1, 0, 170, 65 , 0  , 0  , 0  , 0  ), // #337 {W:vm64x|vm64y, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 132, 74 , 0  , 0  , 0  , 0  ), //      {W:vm64z, R:ymm}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 69 , 0  , 0  , 0  ), // #339 {W:xmm, R:xmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 66 , 69 , 65 , 0  , 0  , 0  ), //      {W:xmm, R:xmm|m128, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 65 , 69 , 0  , 0  , 0  , 0  ), // #341 {R:xmm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 74 , 72 , 0  , 0  , 0  , 0  ), //      {R:ymm, R:ymm|m256}
  ISIGNATURE(2, 1, 1, 0, 127, 171, 0  , 0  , 0  , 0  ), // #343 {W:vm32x, R:xmm|ymm}
  ISIGNATURE(2, 1, 1, 0, 128, 78 , 0  , 0  , 0  , 0  ), //      {W:vm32y, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 113, 67 , 0  , 0  , 0  , 0  ), // #345 {X:xmm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 113, 123, 0  , 0  , 0  , 0  ), // #346 {X:xmm, R:xmm|m32}
  ISIGNATURE(3, 1, 1, 1, 113, 69 , 172, 0  , 0  , 0  ), // #347 {X:xmm, R:xmm|m128, R:<xmm0>}
  ISIGNATURE(1, 1, 1, 0, 173, 0  , 0  , 0  , 0  , 0  ), // #348 {X:r32|r64}
  ISIGNATURE(1, 1, 1, 1, 44 , 0  , 0  , 0  , 0  , 0  ), // #349 {X:<ax>}
  ISIGNATURE(2, 1, 1, 2, 46 , 88 , 0  , 0  , 0  , 0  ), // #350 {W:<edx>, R:<eax>}
  ISIGNATURE(1, 0, 1, 1, 49 , 0  , 0  , 0  , 0  , 0  ), // #351 {X:<rax>}
  ISIGNATURE(1, 1, 1, 0, 174, 0  , 0  , 0  , 0  , 0  ), // #352 {R:mem}
  ISIGNATURE(1, 1, 1, 1, 175, 0  , 0  , 0  , 0  , 0  ), // #353 {R:<zax>}
  ISIGNATURE(3, 1, 1, 0, 113, 123, 26 , 0  , 0  , 0  ), // #354 {X:xmm, R:xmm|m32, R:i8}
  ISIGNATURE(5, 0, 1, 4, 176, 92 , 49 , 177, 178, 0  ), // #355 {X:m128, X:<rdx>, X:<rax>, R:<rcx>, R:<rbx>}
  ISIGNATURE(5, 1, 1, 4, 179, 91 , 47 , 180, 181, 0  ), // #356 {X:m64, X:<edx>, X:<eax>, R:<ecx>, R:<ebx>}
  ISIGNATURE(2, 1, 1, 0, 65 , 67 , 0  , 0  , 0  , 0  ), // #357 {R:xmm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 65 , 123, 0  , 0  , 0  , 0  ), // #358 {R:xmm, R:xmm|m32}
  ISIGNATURE(4, 1, 1, 4, 47 , 182, 183, 46 , 0  , 0  ), // #359 {X:<eax>, W:<ebx>, X:<ecx>, W:<edx>}
  ISIGNATURE(2, 0, 1, 2, 48 , 89 , 0  , 0  , 0  , 0  ), // #360 {W:<rdx>, R:<rax>}
  ISIGNATURE(2, 1, 1, 0, 61 , 69 , 0  , 0  , 0  , 0  ), // #361 {W:mm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 66 , 159, 0  , 0  , 0  , 0  ), // #362 {W:xmm, R:mm|m64}
  ISIGNATURE(2, 1, 1, 0, 61 , 67 , 0  , 0  , 0  , 0  ), // #363 {W:mm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 154, 67 , 0  , 0  , 0  , 0  ), // #364 {W:r32|r64, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 66 , 42 , 0  , 0  , 0  , 0  ), // #365 {W:xmm, R:r32|m32|r64|m64}
  ISIGNATURE(2, 1, 1, 2, 45 , 87 , 0  , 0  , 0  , 0  ), // #366 {W:<dx>, R:<ax>}
  ISIGNATURE(1, 1, 1, 1, 47 , 0  , 0  , 0  , 0  , 0  ), // #367 {X:<eax>}
  ISIGNATURE(1, 1, 0, 1, 44 , 0  , 0  , 0  , 0  , 0  ), // #368 {X:<ax>}
  ISIGNATURE(2, 1, 1, 0, 163, 26 , 0  , 0  , 0  , 0  ), // #369 {X:i16, R:i8}
  ISIGNATURE(3, 1, 1, 0, 146, 65 , 26 , 0  , 0  , 0  ), // #370 {W:r32|m32|r64, R:xmm, R:i8}
  ISIGNATURE(1, 1, 1, 0, 184, 0  , 0  , 0  , 0  , 0  ), // #371 {X:m80}
  ISIGNATURE(1, 1, 1, 0, 185, 0  , 0  , 0  , 0  , 0  ), // #372 {X:m16|m32}
  ISIGNATURE(1, 1, 1, 0, 186, 0  , 0  , 0  , 0  , 0  ), // #373 {X:m16|m32|m64}
  ISIGNATURE(1, 1, 1, 0, 187, 0  , 0  , 0  , 0  , 0  ), // #374 {X:m32|m64|m80|fp}
  ISIGNATURE(1, 1, 1, 0, 188, 0  , 0  , 0  , 0  , 0  ), // #375 {X:m16}
  ISIGNATURE(1, 1, 1, 0, 189, 0  , 0  , 0  , 0  , 0  ), // #376 {X:mem}
  ISIGNATURE(1, 1, 1, 0, 190, 0  , 0  , 0  , 0  , 0  ), // #377 {X:ax|m16}
  ISIGNATURE(1, 0, 1, 0, 189, 0  , 0  , 0  , 0  , 0  ), // #378 {X:mem}
  ISIGNATURE(1, 1, 1, 0, 191, 0  , 0  , 0  , 0  , 0  ), // #379 {X:i8}
  ISIGNATURE(1, 1, 1, 0, 192, 0  , 0  , 0  , 0  , 0  ), // #380 {X:rel8|rel32}
  ISIGNATURE(1, 1, 1, 0, 193, 0  , 0  , 0  , 0  , 0  ), // #381 {X:rel8}
  ISIGNATURE(3, 1, 1, 0, 111, 144, 144, 0  , 0  , 0  ), // #382 {W:k, R:k, R:k}
  ISIGNATURE(2, 1, 1, 0, 111, 144, 0  , 0  , 0  , 0  ), // #383 {W:k, R:k}
  ISIGNATURE(2, 1, 1, 0, 144, 144, 0  , 0  , 0  , 0  ), // #384 {R:k, R:k}
  ISIGNATURE(3, 1, 1, 0, 111, 144, 26 , 0  , 0  , 0  ), // #385 {W:k, R:k, R:i8}
  ISIGNATURE(1, 1, 1, 1, 194, 0  , 0  , 0  , 0  , 0  ), // #386 {W:<ah>}
  ISIGNATURE(1, 1, 1, 0, 56 , 0  , 0  , 0  , 0  , 0  ), // #387 {R:m32}
  ISIGNATURE(2, 1, 1, 0, 153, 174, 0  , 0  , 0  , 0  ), // #388 {W:r16|r32|r64, R:mem}
  ISIGNATURE(3, 1, 1, 1, 113, 65 , 175, 0  , 0  , 0  ), // #389 {X:xmm, R:xmm, R:<zdi>}
  ISIGNATURE(3, 1, 1, 1, 158, 64 , 175, 0  , 0  , 0  ), // #390 {X:mm, R:mm, R:<zdi>}
  ISIGNATURE(2, 1, 1, 0, 61 , 65 , 0  , 0  , 0  , 0  ), // #391 {W:mm, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 65 , 0  , 0  , 0  , 0  ), // #392 {W:xmm, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 154, 65 , 0  , 0  , 0  , 0  ), // #393 {W:r32|r64, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 60 , 64 , 0  , 0  , 0  , 0  ), // #394 {W:m64, R:mm}
  ISIGNATURE(2, 1, 1, 0, 66 , 64 , 0  , 0  , 0  , 0  ), // #395 {W:xmm, R:mm}
  ISIGNATURE(2, 0, 1, 0, 19 , 39 , 0  , 0  , 0  , 0  ), // #396 {W:r64, R:r32|m32}
  ISIGNATURE(6, 1, 1, 3, 65 , 69 , 26 , 195, 88 , 155), // #397 {R:xmm, R:xmm|m128, R:i8, W:<ecx>, R:<eax>, R:<edx>}
  ISIGNATURE(6, 1, 1, 3, 65 , 69 , 26 , 196, 88 , 155), // #398 {R:xmm, R:xmm|m128, R:i8, W:<xmm0>, R:<eax>, R:<edx>}
  ISIGNATURE(4, 1, 1, 1, 65 , 69 , 26 , 195, 0  , 0  ), // #399 {R:xmm, R:xmm|m128, R:i8, W:<ecx>}
  ISIGNATURE(4, 1, 1, 1, 65 , 69 , 26 , 196, 0  , 0  ), // #400 {R:xmm, R:xmm|m128, R:i8, W:<xmm0>}
  ISIGNATURE(3, 1, 1, 0, 143, 65 , 26 , 0  , 0  , 0  ), // #401 {W:r32|m8|r64|r8lo|r8hi|r16, R:xmm, R:i8}
  ISIGNATURE(3, 0, 1, 0, 7  , 65 , 26 , 0  , 0  , 0  ), // #402 {W:r64|m64, R:xmm, R:i8}
  ISIGNATURE(3, 1, 1, 0, 113, 168, 26 , 0  , 0  , 0  ), // #403 {X:xmm, R:r32|m8|r64|r8lo|r8hi|r16, R:i8}
  ISIGNATURE(3, 1, 1, 0, 113, 151, 26 , 0  , 0  , 0  ), // #404 {X:xmm, R:r32|m32|r64, R:i8}
  ISIGNATURE(3, 0, 1, 0, 113, 16 , 26 , 0  , 0  , 0  ), // #405 {X:xmm, R:r64|m64, R:i8}
  ISIGNATURE(3, 1, 1, 0, 197, 198, 26 , 0  , 0  , 0  ), // #406 {X:mm|xmm, R:r32|m16|r64|r16, R:i8}
  ISIGNATURE(2, 1, 1, 0, 154, 152, 0  , 0  , 0  , 0  ), // #407 {W:r32|r64, R:mm|xmm}
  ISIGNATURE(0, 1, 0, 0, 0  , 0  , 0  , 0  , 0  , 0  ), // #408 {}
  ISIGNATURE(3, 1, 1, 0, 61 , 159, 26 , 0  , 0  , 0  ), // #409 {W:mm, R:mm|m64, R:i8}
  ISIGNATURE(2, 1, 1, 0, 113, 26 , 0  , 0  , 0  , 0  ), // #410 {X:xmm, R:i8}
  ISIGNATURE(2, 1, 1, 0, 25 , 108, 0  , 0  , 0  , 0  ), // #411 {X:r8lo|r8hi|m8|r16|m16|r32|m32|r64|m64, R:cl|i8}
  ISIGNATURE(1, 0, 1, 0, 154, 0  , 0  , 0  , 0  , 0  ), // #412 {W:r32|r64}
  ISIGNATURE(1, 1, 1, 0, 153, 0  , 0  , 0  , 0  , 0  ), // #413 {W:r16|r32|r64}
  ISIGNATURE(2, 1, 1, 2, 46 , 199, 0  , 0  , 0  , 0  ), // #414 {W:<edx>, W:<eax>}
  ISIGNATURE(3, 1, 1, 3, 46 , 199, 195, 0  , 0  , 0  ), // #415 {W:<edx>, W:<eax>, W:<ecx>}
  ISIGNATURE(3, 1, 1, 0, 66 , 67 , 26 , 0  , 0  , 0  ), // #416 {W:xmm, R:xmm|m64, R:i8}
  ISIGNATURE(3, 1, 1, 0, 66 , 123, 26 , 0  , 0  , 0  ), // #417 {W:xmm, R:xmm|m32, R:i8}
  ISIGNATURE(1, 1, 1, 1, 200, 0  , 0  , 0  , 0  , 0  ), // #418 {R:<ah>}
  ISIGNATURE(1, 1, 1, 0, 1  , 0  , 0  , 0  , 0  , 0  ), // #419 {W:r8lo|r8hi|m8}
  ISIGNATURE(1, 1, 1, 0, 59 , 0  , 0  , 0  , 0  , 0  ), // #420 {W:m32}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 67 , 0  , 0  , 0  ), // #421 {W:xmm, R:xmm, R:xmm|m64}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 123, 0  , 0  , 0  ), // #422 {W:xmm, R:xmm, R:xmm|m32}
  ISIGNATURE(2, 1, 1, 0, 71 , 96 , 0  , 0  , 0  , 0  ), // #423 {W:ymm, R:m128}
  ISIGNATURE(2, 1, 1, 0, 201, 67 , 0  , 0  , 0  , 0  ), // #424 {W:ymm|zmm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 201, 96 , 0  , 0  , 0  , 0  ), // #425 {W:ymm|zmm, R:m128}
  ISIGNATURE(2, 1, 1, 0, 75 , 97 , 0  , 0  , 0  , 0  ), // #426 {W:zmm, R:m256}
  ISIGNATURE(2, 1, 1, 0, 165, 67 , 0  , 0  , 0  , 0  ), // #427 {W:xmm|ymm|zmm, R:xmm|m64}
  ISIGNATURE(4, 1, 1, 0, 109, 65 , 67 , 26 , 0  , 0  ), // #428 {W:xmm|k, R:xmm, R:xmm|m64, R:i8}
  ISIGNATURE(4, 1, 1, 0, 109, 65 , 123, 26 , 0  , 0  ), // #429 {W:xmm|k, R:xmm, R:xmm|m32, R:i8}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 42 , 0  , 0  , 0  ), // #430 {W:xmm, R:xmm, R:r32|m32|r64|m64}
  ISIGNATURE(3, 1, 1, 0, 70 , 202, 26 , 0  , 0  , 0  ), // #431 {W:xmm|m128, R:ymm|zmm, R:i8}
  ISIGNATURE(4, 1, 1, 0, 113, 65 , 67 , 26 , 0  , 0  ), // #432 {X:xmm, R:xmm, R:xmm|m64, R:i8}
  ISIGNATURE(4, 1, 1, 0, 113, 65 , 123, 26 , 0  , 0  ), // #433 {X:xmm, R:xmm, R:xmm|m32, R:i8}
  ISIGNATURE(3, 1, 1, 0, 113, 65 , 67 , 0  , 0  , 0  ), // #434 {X:xmm, R:xmm, R:xmm|m64}
  ISIGNATURE(3, 1, 1, 0, 113, 65 , 123, 0  , 0  , 0  ), // #435 {X:xmm, R:xmm, R:xmm|m32}
  ISIGNATURE(3, 1, 1, 0, 111, 203, 26 , 0  , 0  , 0  ), // #436 {W:k, R:xmm|m128|ymm|m256|zmm|m512, R:i8}
  ISIGNATURE(3, 1, 1, 0, 111, 67 , 26 , 0  , 0  , 0  ), // #437 {W:k, R:xmm|m64, R:i8}
  ISIGNATURE(3, 1, 1, 0, 111, 123, 26 , 0  , 0  , 0  ), // #438 {W:k, R:xmm|m32, R:i8}
  ISIGNATURE(1, 1, 1, 0, 81 , 0  , 0  , 0  , 0  , 0  ), // #439 {R:vm32y}
  ISIGNATURE(1, 1, 1, 0, 82 , 0  , 0  , 0  , 0  , 0  ), // #440 {R:vm32z}
  ISIGNATURE(1, 1, 1, 0, 85 , 0  , 0  , 0  , 0  , 0  ), // #441 {R:vm64z}
  ISIGNATURE(4, 1, 1, 0, 75 , 78 , 72 , 26 , 0  , 0  ), // #442 {W:zmm, R:zmm, R:ymm|m256, R:i8}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 123, 26 , 0  , 0  ), // #443 {W:xmm, R:xmm, R:xmm|m32, R:i8}
  ISIGNATURE(3, 1, 1, 1, 65 , 65 , 175, 0  , 0  , 0  ), // #444 {R:xmm, R:xmm, R:<zdi>}
  ISIGNATURE(2, 1, 1, 0, 154, 171, 0  , 0  , 0  , 0  ), // #445 {W:r32|r64, R:xmm|ymm}
  ISIGNATURE(2, 1, 1, 0, 165, 144, 0  , 0  , 0  , 0  ), // #446 {W:xmm|ymm|zmm, R:k}
  ISIGNATURE(2, 1, 1, 0, 165, 118, 0  , 0  , 0  , 0  ), // #447 {W:xmm|ymm|zmm, R:xmm|m64|r64}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 198, 26 , 0  , 0  ), // #448 {W:xmm, R:xmm, R:r32|m16|r64|r16, R:i8}
  ISIGNATURE(2, 1, 1, 0, 111, 204, 0  , 0  , 0  , 0  ), // #449 {W:k, R:xmm|ymm|zmm}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 67 , 26 , 0  , 0  ), // #450 {W:xmm, R:xmm, R:xmm|m64, R:i8}
  ISIGNATURE(1, 0, 1, 0, 167, 0  , 0  , 0  , 0  , 0  ), // #451 {R:r32|r64}
  ISIGNATURE(3, 1, 1, 3, 180, 46 , 199, 0  , 0  , 0  ), // #452 {R:<ecx>, W:<edx>, W:<eax>}
  ISIGNATURE(3, 1, 1, 2, 189, 155, 88 , 0  , 0  , 0  ), // #453 {X:mem, R:<edx>, R:<eax>}
  ISIGNATURE(3, 0, 1, 2, 189, 155, 88 , 0  , 0  , 0  ), // #454 {X:mem, R:<edx>, R:<eax>}
  ISIGNATURE(3, 1, 1, 3, 180, 155, 88 , 0  , 0  , 0  )  // #455 {R:<ecx>, R:<edx>, R:<eax>}
};
#undef ISIGNATURE

#define FLAG(flag) X86Inst::kOp##flag
#define MEM(mem) X86Inst::kMemOp##mem
#define OSIGNATURE(flags, memFlags, extFlags, regId) \
  { uint32_t(flags), uint16_t(memFlags), uint8_t(extFlags), uint8_t(regId) }
static const X86Inst::OSignature _x86InstOSignatureData[] = {
  OSIGNATURE(0, 0, 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Mem), MEM(M8), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(I8), 0, 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Gpw) | FLAG(Mem), MEM(M16), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Seg) | FLAG(I16), 0, 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Gpd) | FLAG(Mem), MEM(M32), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(I32), 0, 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Gpq) | FLAG(Mem), MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(Seg) | FLAG(I32), 0, 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(GpbLo) | FLAG(GpbHi), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Mem) | FLAG(I8), MEM(M8), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Gpw) | FLAG(Seg), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Mem), MEM(M16), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Gpd), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Mem) | FLAG(I32), MEM(M32), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Gpq) | FLAG(Seg), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(Mem), MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Gpw), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(I16), 0, 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Gpq), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Cr) | FLAG(Dr) | FLAG(I64), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Cr) | FLAG(Dr), 0, 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Cr) | FLAG(Dr), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpd), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpq), 0, 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M8) | MEM(M16) | MEM(M32) | MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(I8), 0, 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Gpw) | FLAG(Mem), MEM(M16), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(I16), 0, 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M32) | MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(I32), 0, 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Mem), MEM(M8), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi), 0, 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Gpd) | FLAG(Mem), MEM(M32), 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Gpq) | FLAG(Mem), MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(GpbLo) | FLAG(GpbHi), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Mem), MEM(M8), 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Gpw), 0, 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Gpd), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Mem), MEM(M32), 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Gpq), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M8) | MEM(M16) | MEM(M32) | MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M32) | MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpw), 0, 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpw), 0, 0, 0),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Gpw), 0, 0, 2),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 2),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 2),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 0),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Mem) | FLAG(I8) | FLAG(I16), MEM(M16), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Mem) | FLAG(I8) | FLAG(I32), MEM(M32), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(Mem) | FLAG(I8) | FLAG(I32), MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(I8) | FLAG(I16), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(I8) | FLAG(I32), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(M16), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(M32), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(M16), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(M32), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Mm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(Mm) | FLAG(Xmm) | FLAG(Mem), MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Gpq) | FLAG(Mm) | FLAG(Xmm) | FLAG(Mem), MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Mm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Xmm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Xmm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Xmm) | FLAG(Mem), MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(M128), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Xmm) | FLAG(Mem), MEM(M128), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Ymm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Ymm) | FLAG(Mem), MEM(M256), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Ymm) | FLAG(Mem), MEM(M256), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Ymm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Zmm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Zmm) | FLAG(Mem), MEM(M512), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Zmm) | FLAG(Mem), MEM(M512), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Zmm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem) | FLAG(I8), MEM(M128), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm32x), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm32y), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm32z), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm64x), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm64y), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm64z), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(GpbLo), 0, 0, 0),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpw), 0, 0, 0),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 0),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpw), 0, 0, 2),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 2),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 2),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm64x) | MEM(Vm64y), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(M128), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(M256), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(M128), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(M256), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(I4), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(I8), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(I8), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(I8), 0, 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(M32) | MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Fp), 0, 0, 0),
  OSIGNATURE(FLAG(R) | FLAG(Fp), 0, 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Fp), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Fp), 0, 0, 0),
  OSIGNATURE(FLAG(X) | FLAG(Implicit), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(I8), 0, 0, 1),
  OSIGNATURE(FLAG(W) | FLAG(K) | FLAG(Xmm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(K) | FLAG(Ymm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(K), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Ymm) | FLAG(Mem), MEM(M64) | MEM(M128) | MEM(M256), 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Xmm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Ymm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Zmm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(M512), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(M512), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(Xmm) | FLAG(Mem), MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(M32) | MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(M32) | MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Xmm) | FLAG(Mem), MEM(M32), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Xmm) | FLAG(Mem), MEM(M16), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(M32), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(M16), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(M32) | MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(M64) | MEM(M128), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm32x), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm32y), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm32z), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm64x), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm64y), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm64z), 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Gpq) | FLAG(Mem) | FLAG(I32) | FLAG(I64) | FLAG(Rel32), MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Mem), MEM(M8) | MEM(M16) | MEM(M32), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpq) | FLAG(Mem), MEM(M8) | MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Gpw) | FLAG(Gpd), 0, 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Fp) | FLAG(Mem), MEM(M32) | MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Gpd), 0, 0, 1),
  OSIGNATURE(FLAG(R) | FLAG(Rel8), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Gpq), 0, 0, 1),
  OSIGNATURE(FLAG(X) | FLAG(Gpq) | FLAG(Mem) | FLAG(I32) | FLAG(I64) | FLAG(Rel8) | FLAG(Rel32), MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(K) | FLAG(Mem), MEM(M8), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M8), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(K), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Gpq) | FLAG(K) | FLAG(Mem), MEM(M32), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M32), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(K) | FLAG(Mem), MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(K) | FLAG(Mem), MEM(M16), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M16), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Mm) | FLAG(Xmm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M32), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Mm) | FLAG(Xmm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq), 0, 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Gpd) | FLAG(Gpq), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 2),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 2),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Mem), MEM(M16) | MEM(M32), 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Mm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Mm) | FLAG(Mem), MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Gpw) | FLAG(Gpq) | FLAG(Mem), MEM(M16) | MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Mm) | FLAG(Mem) | FLAG(I8), MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Gpw) | FLAG(Gpq) | FLAG(Mem) | FLAG(I8) | FLAG(I16) | FLAG(I32), MEM(M16) | MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(I16), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Ymm) | FLAG(Mem), MEM(M128) | MEM(M256), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Xmm) | FLAG(Ymm) | FLAG(Zmm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(M8), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Gpq), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M8), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(I8), 0, 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm64x) | MEM(Vm64y), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Ymm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Xmm), 0, 0, 0),
  OSIGNATURE(FLAG(X) | FLAG(Gpd) | FLAG(Gpq), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(Any), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Implicit), 0, 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(M128), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 1),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 3),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 1),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 3),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 3),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 1),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(M80), 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(M16) | MEM(M32), 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(M16) | MEM(M32) | MEM(M64), 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Fp) | FLAG(Mem), MEM(M32) | MEM(M64) | MEM(M80), 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(M16), 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(Any), 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Gpw) | FLAG(Mem), MEM(M16), 0, 0),
  OSIGNATURE(FLAG(X) | FLAG(I8), 0, 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Rel8) | FLAG(Rel32), 0, 0, 0xFF),
  OSIGNATURE(FLAG(X) | FLAG(Rel8), 0, 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(GpbHi), 0, 0, 0),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 1),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Xmm), 0, 0, 0),
  OSIGNATURE(FLAG(X) | FLAG(Mm) | FLAG(Xmm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M16), 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(GpbHi), 0, 0, 0),
  OSIGNATURE(FLAG(W) | FLAG(Ymm) | FLAG(Zmm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Ymm) | FLAG(Zmm), 0, 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Ymm) | FLAG(Zmm) | FLAG(Mem), MEM(M128) | MEM(M256) | MEM(M512), 0, 0xFF),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Ymm) | FLAG(Zmm), 0, 0, 0xFF)
};
#undef OSIGNATURE
#undef MEM
#undef FLAG
// ----------------------------------------------------------------------------
// ${X86InstSignatureData:End}

static const uint32_t _x86OpFlagFromRegType[X86Reg::kRegCount] = {
  X86Inst::kOpNone,  // #00 None.
  X86Inst::kOpNone,  // #01 Reserved.
  X86Inst::kOpNone,  // #02 RIP.
  X86Inst::kOpSeg,   // #03 SEG.
  X86Inst::kOpGpbLo, // #04 GPB-LO.
  X86Inst::kOpGpbHi, // #05 GPB-HI.
  X86Inst::kOpGpw,   // #06 GPW.
  X86Inst::kOpGpd,   // #07 GPD.
  X86Inst::kOpGpq,   // #08 GPQ.
  X86Inst::kOpFp,    // #09 FP.
  X86Inst::kOpMm,    // #10 MM.
  X86Inst::kOpK,     // #11 K.
  X86Inst::kOpXmm,   // #12 XMM.
  X86Inst::kOpYmm,   // #13 YMM.
  X86Inst::kOpZmm,   // #14 ZMM.
  X86Inst::kOpNone,  // #15 FUTURE.
  X86Inst::kOpBnd,   // #16 BND.
  X86Inst::kOpCr,    // #17 CR.
  X86Inst::kOpDr     // #18 DR.
};

struct X86ValidationData {
  //! Allowed registers by reg-type (X86::kReg...).
  uint32_t allowedRegMask[X86Reg::kRegCount];
  uint32_t allowedMemBaseRegs;
  uint32_t allowedMemIndexRegs;
};

static const X86ValidationData _x86ValidationData = {
  {
    0x00000000U,       // #00 None.
    0x00000000U,       // #01 Reserved.
    0x00000001U,       // #02 RIP.
    0x0000007EU,       // #03 SEG (ES|CS|SS|DS|FS|GS).
    0x0000000FU,       // #04 GPB-LO.
    0x0000000FU,       // #05 GPB-HI.
    0x000000FFU,       // #06 GPW.
    0x000000FFU,       // #07 GPD.
    0x000000FFU,       // #08 GPQ.
    0x000000FFU,       // #09 FP.
    0x000000FFU,       // #10 MM.
    0x000000FFU,       // #11 K.
    0x000000FFU,       // #12 XMM.
    0x000000FFU,       // #13 YMM.
    0x000000FFU,       // #14 ZMM.
    0x00000000U,       // #15 FUTURE.
    0x0000000FU,       // #16 BND
    0x000000FFU,       // #17 CR.
    0x000000FFU        // #18 DR
  },

  // AllowedMemBaseRegs:
  (1U << Label::kLabelTag) |
  (1U << X86Reg::kRegGpw) | (1U << X86Reg::kRegGpd) | (1U << X86Reg::kRegRip),

  // AllowedMemIndexRegs:
  (1U << X86Reg::kRegGpw) | (1U << X86Reg::kRegGpd) |
  (1U << X86Reg::kRegXmm) | (1U << X86Reg::kRegYmm) | (1U << X86Reg::kRegZmm)
};

static const X86ValidationData _x64ValidationData = {
  {
    0x00000000U,       // #00 None.
    0x00000000U,       // #01 Reserved.
    0x00000001U,       // #02 RIP.
    0x00000060U,       // #03 SEG (FS|GS).
    0x0000FFFFU,       // #04 GPB-LO.
    0x0000000FU,       // #05 GPB-HI.
    0x0000FFFFU,       // #06 GPW.
    0x0000FFFFU,       // #07 GPD.
    0x0000FFFFU,       // #08 GPQ.
    0x000000FFU,       // #09 FP.
    0x000000FFU,       // #10 MM.
    0x000000FFU,       // #11 K.
    0xFFFFFFFFU,       // #12 XMM (16 base regs, 32 regs only with EVEX encoding).
    0xFFFFFFFFU,       // #13 YMM (16 base regs, 32 regs only with EVEX encoding).
    0xFFFFFFFFU,       // #14 ZMM (16 base regs, 32 regs only with EVEX encoding).
    0x00000000U,       // #15 FUTURE.
    0x0000000FU,       // #16 BND.
    0x000001FFU,       // #17 CR.
    0x000000FFU        // #18 DR.
  },

  // AllowedMemBaseRegs:
  (1U << Label::kLabelTag) |
  (1U << X86Reg::kRegGpd) | (1U << X86Reg::kRegGpq) | (1U << X86Reg::kRegRip),

  // AllowedMemIndexRegs:
  (1U << X86Reg::kRegGpd) | (1U << X86Reg::kRegGpq) |
  (1U << X86Reg::kRegXmm) | (1U << X86Reg::kRegYmm) | (1U << X86Reg::kRegZmm)
};

Error X86Inst::validate(
  uint32_t archId,
  uint32_t instId, uint32_t options,
  const Operand_& opExtra, const Operand_* opArray, uint32_t opCount) noexcept {

  uint32_t i;
  uint32_t archMask;
  const X86ValidationData* vd;

  if (archId == Arch::kTypeX86) {
    vd = &_x86ValidationData;
    archMask = X86Inst::kArchMaskX86;
  }
  else if (archId == Arch::kTypeX64) {
    vd = &_x64ValidationData;
    archMask = X86Inst::kArchMaskX64;
  }
  else {
    return DebugUtils::errored(kErrorInvalidArch);
  }

  if (ASMJIT_UNLIKELY(instId >= X86Inst::_kIdCount))
    return DebugUtils::errored(kErrorInvalidInstruction);
  const X86Inst* instData = _x86InstData + instId;

  // Translate the given operands to `X86Inst::OSignature`.
  X86Inst::OSignature oSigTranslated[6];
  uint32_t combinedOpFlags = 0;
  uint32_t combinedRegMask = 0;

  for (i = 0; i < opCount; i++) {
    const Operand_& op = opArray[i];
    if (op.getOp() == Operand::kOpNone) break;

    uint32_t opFlags = 0;
    uint32_t memFlags = 0;
    uint32_t regId = kInvalidReg;

    switch (op.getOp()) {
      case Operand::kOpReg: {
        uint32_t regType = static_cast<const Reg&>(op).getRegType();
        if (regType >= X86Reg::kRegCount)
          return DebugUtils::errored(kErrorIllegalRegType);

        opFlags = _x86OpFlagFromRegType[regType];
        if (opFlags == 0)
          return DebugUtils::errored(kErrorIllegalRegType);

        // If `regId` is equal or greater than Operand::kPackedIdMin it means
        // that the register is virtual and its index will be assigned later
        // by the register allocator. We must pass unless asked to disallow
        // virtual registers.
        // TODO: We need an option to refuse virtual regs here.
        regId = op.getId();
        if (regId < Operand::kPackedIdMin) {
          if (regId >= 32)
            return DebugUtils::errored(kErrorInvalidPhysId);

          uint32_t regMask = Utils::mask(regId);
          if ((vd->allowedRegMask[regType] & regMask) == 0)
            return DebugUtils::errored(kErrorInvalidPhysId);

          combinedRegMask |= regMask;
        }
        break;
      }

      case Operand::kOpMem: {
        const X86Mem& m = static_cast<const X86Mem&>(op);
        uint32_t memSize = m.getSize();

        uint32_t baseType = m.getBaseType();
        uint32_t indexType = m.getIndexType();

        // TODO: Validate base and index and combine with `combinedRegMask`.
        if (baseType) {
          if ((vd->allowedMemBaseRegs & (1U << baseType)) == 0)
            return DebugUtils::errored(kErrorIllegalAddressing);
        }

        if (indexType) {
          if ((vd->allowedMemIndexRegs & (1U << indexType)) == 0)
            return DebugUtils::errored(kErrorIllegalAddressing);

          if (indexType == X86Reg::kRegXmm) {
            opFlags |= X86Inst::kOpVm;
            memFlags |= X86Inst::kMemOpVm32x | X86Inst::kMemOpVm64x;
          }
          else if (indexType == X86Reg::kRegYmm) {
            opFlags |= X86Inst::kOpVm;
            memFlags |= X86Inst::kMemOpVm32y | X86Inst::kMemOpVm64y;
          }
          else if (indexType == X86Reg::kRegZmm) {
            opFlags |= X86Inst::kOpVm;
            memFlags |= X86Inst::kMemOpVm32z | X86Inst::kMemOpVm64z;
          }
          else {
            opFlags |= X86Inst::kOpMem;
          }
        }
        else {
          // TODO: We need 'any-size' information, otherwise we can't validate properly.
          opFlags |= X86Inst::kOpMem;
          memFlags |= X86Inst::kMemOpM8    |
                      X86Inst::kMemOpM16   |
                      X86Inst::kMemOpM32   |
                      X86Inst::kMemOpM64   |
                      X86Inst::kMemOpM80   |
                      X86Inst::kMemOpM128  |
                      X86Inst::kMemOpM256  |
                      X86Inst::kMemOpM512  |
                      X86Inst::kMemOpM1024 |
                      X86Inst::kMemOpAny   ;
        }
        break;
      }

      case Operand::kOpImm: {
        // TODO: We need signed vs. zero extension, otherwise we can't validate properly.
        opFlags |= X86Inst::kOpI4 | X86Inst::kOpI8 | X86Inst::kOpI16 | X86Inst::kOpI32 | X86Inst::kOpI64;
        break;
      }

      case Operand::kOpLabel: {
        opFlags |= X86Inst::kOpRel8 | X86Inst::kOpRel32;
        break;
      }

      default:
        return DebugUtils::errored(kErrorInvalidState);
    }

    X86Inst::OSignature& tod = oSigTranslated[i];
    tod.flags = opFlags;
    tod.memFlags = static_cast<uint16_t>(memFlags);
    tod.regId = static_cast<uint8_t>(regId);
    combinedOpFlags |= opFlags;
  }

  // Decrease the number of operands of those that are none. This is important
  // as Assembler and CodeCompiler may just pass more operands where some of
  // them are none (it means that no operand is given at that index). However,
  // validate that there are no gaps (like [reg, none, reg] or [none, reg]).
  if (i < opCount) {
    while (--opCount > i)
      if (!opArray[opCount].isNone())
        return DebugUtils::errored(kErrorInvalidState);
  }

  // Validate X86 and X64 specific cases.
  if (archMask == kArchMaskX86) {
    // Illegal use of 64-bit register in 32-bit mode.
    if ((combinedOpFlags & X86Inst::kOpGpq) != 0)
      return DebugUtils::errored(kErrorIllegalUseOfGpq);
  }
  else {
    // Illegal use of a high 8-bit register together with register(s) that require REX.W prefix.
    if ((combinedOpFlags & X86Inst::kOpGpbHi) != 0 && (combinedRegMask & 0xFFFFFF00U) != 0)
      return DebugUtils::errored(kErrorIllegalUseOfGpbHi);
  }

  // Validate instruction operands.
  const X86Inst::ISignature* iSig = _x86InstISignatureData + instData->_signatureTableIndex;
  const X86Inst::ISignature* iEnd = iSig                   + instData->_signatureTableCount;

  if (iSig != iEnd) {
    const X86Inst::OSignature* oSigData = _x86InstOSignatureData;
    do {
      // Check if the architecture is compatible.
      if ((iSig->archMask & archMask) == 0) continue;

      // Compare the operands table with reference operands.
      uint32_t iCount = iSig->opCount;
      if (iCount == opCount) {
        uint32_t j;
        for (j = 0; j < opCount; j++) {
          const X86Inst::OSignature* oChk = oSigTranslated + j;
          const X86Inst::OSignature* oRef = oSigData + iSig->operands[j];

          // Base flags.
          uint32_t opFlags = oChk->flags;
          if ((oRef->flags & opFlags) == 0)
            break;

          // Memory specific flags and sizes.
          uint32_t memFlags = oChk->memFlags;
          if ((oRef->memFlags & memFlags) == 0 && memFlags != 0)
            break;

          // Specific register index.
          uint32_t regId = oChk->regId;
          if (oRef->regId != kInvalidReg && regId != kInvalidReg && oRef->regId != regId)
            break;
        }

        if (j == opCount)
          break;
      }
      else if (iCount - iSig->implicit == opCount) {
        uint32_t j;
        uint32_t r = 0;

        for (j = 0; j < opCount && r < iCount; j++, r++) {
          const X86Inst::OSignature* oChk = oSigTranslated + j;
          const X86Inst::OSignature* oRef;

Next:
          oRef = oSigData + iSig->operands[r];
          // Skip implicit.
          if ((oRef->flags & X86Inst::kOpImplicit) != 0) {
            if (++r >= iCount)
              break;
            else
              goto Next;
          }

          // Base flags.
          uint32_t opFlags = oChk->flags;
          if ((oRef->flags & opFlags) == 0)
            break;

          // Memory specific flags and sizes.
          uint32_t memFlags = oChk->memFlags;
          if ((oRef->memFlags & memFlags) == 0 && memFlags != 0)
            break;

          // Specific register index.
          uint32_t regId = oChk->regId;
          if (oRef->regId != kInvalidReg && regId != kInvalidReg && oRef->regId != regId)
            break;
        }

        if (j == opCount)
          break;
      }
    } while (++iSig != iEnd);

    if (iSig == iEnd)
      return DebugUtils::errored(kErrorIllegalInstruction);
  }

  return kErrorOk;
}
#endif // !ASMJIT_DISABLE_VALIDATION

// ============================================================================
// [asmjit::X86Util - Condition Codes]
// ============================================================================

#define CC_TO_INST(inst) { \
  inst##o     , inst##no    , inst##b     , inst##ae   , \
  inst##e     , inst##ne    , inst##be    , inst##a    , \
  inst##s     , inst##ns    , inst##pe    , inst##po   , \
  inst##l     , inst##ge    , inst##le    , inst##g    , \
  kInvalidInst, kInvalidInst, kInvalidInst, kInvalidInst \
}

const uint32_t _x86ReverseCond[20] = {
  X86Inst::kCondO, X86Inst::kCondNO, X86Inst::kCondA , X86Inst::kCondBE, // O|NO|B |AE
  X86Inst::kCondE, X86Inst::kCondNE, X86Inst::kCondAE, X86Inst::kCondB , // E|NE|BE|A
  X86Inst::kCondS, X86Inst::kCondNS, X86Inst::kCondPE, X86Inst::kCondPO, // S|NS|PE|PO
  X86Inst::kCondG, X86Inst::kCondLE, X86Inst::kCondGE, X86Inst::kCondL , // L|GE|LE|G
  X86Inst::kCondFpuUnordered, X86Inst::kCondFpuNotUnordered, 0x12, 0x13
};
const uint32_t _x86CondToCmovcc[20] = CC_TO_INST(X86Inst::kIdCmov);
const uint32_t _x86CondToJcc   [20] = CC_TO_INST(X86Inst::kIdJ);
const uint32_t _x86CondToSetcc [20] = CC_TO_INST(X86Inst::kIdSet);

#undef CC_TO_INST

// ============================================================================
// [asmjit::X86Util - Test]
// ============================================================================

#if defined(ASMJIT_TEST)
UNIT(x86_inst_bits) {
  INFO("Checking validity of X86Inst enums.");

  // Cross-validate prefixes.
  EXPECT(X86Inst::kOptionRex  == 0x80000000U, "EVEX prefix must be at 0x80000000");
  EXPECT(X86Inst::kOptionVex3 == 0x00000004U, "VEX3 prefix must be at 0x00000004");
  EXPECT(X86Inst::kOptionEvex == 0x00000008U, "EVEX prefix must be at 0x00000008");

  // These could be combined together to form a valid REX prefix, they must match.
  EXPECT(int(X86Inst::kOptionOpCodeB) == int(X86Inst::kOpCode_B));
  EXPECT(int(X86Inst::kOptionOpCodeX) == int(X86Inst::kOpCode_X));
  EXPECT(int(X86Inst::kOptionOpCodeR) == int(X86Inst::kOpCode_R));
  EXPECT(int(X86Inst::kOptionOpCodeW) == int(X86Inst::kOpCode_W));

  uint32_t rex_rb = (X86Inst::kOpCode_R >> X86Inst::kOpCode_REX_Shift) |
                    (X86Inst::kOpCode_B >> X86Inst::kOpCode_REX_Shift) | 0x40;
  uint32_t rex_rw = (X86Inst::kOpCode_R >> X86Inst::kOpCode_REX_Shift) |
                    (X86Inst::kOpCode_W >> X86Inst::kOpCode_REX_Shift) | 0x40;
  EXPECT(rex_rb == 0x45, "kOpCode_R|B must form a valid REX prefix 0x45 if combined with 0x40.");
  EXPECT(rex_rw == 0x4C, "kOpCode_R|W must form a valid REX prefix 0x4C if combined with 0x40.");
}
#endif // ASMJIT_TEST

#if defined(ASMJIT_TEST) && !defined(ASMJIT_DISABLE_TEXT)
UNIT(x86_inst_names) {
  // All known instructions should be matched.
  INFO("Matching all X86/X64 instructions.");
  for (uint32_t a = 0; a < X86Inst::_kIdCount; a++) {
    uint32_t b = X86Inst::getIdByName(_x86InstNameData + _x86InstNameIndex[a]);
    EXPECT(a == b,
      "Should match existing instruction \"%s\" {id:%u} != \"%s\" {id:%u}.",
        _x86InstNameData + _x86InstNameIndex[a], a,
        _x86InstNameData + _x86InstNameIndex[b], b);
  }

  // Everything else should return `kInvalidInst`.
  INFO("Trying to look-up instructions that don't exist.");
  EXPECT(X86Inst::getIdByName(nullptr) == kInvalidInst,
    "Should return kInvalidInst for null input.");

  EXPECT(X86Inst::getIdByName("") == kInvalidInst,
    "Should return kInvalidInst for empty string.");

  EXPECT(X86Inst::getIdByName("_") == kInvalidInst,
    "Should return kInvalidInst for unknown instruction.");

  EXPECT(X86Inst::getIdByName("123xyz") == kInvalidInst,
    "Should return kInvalidInst for unknown instruction.");
}
#endif // ASMJIT_TEST && !ASMJIT_DISABLE_TEXT

#if defined(ASMJIT_TEST) && !defined(ASMJIT_DISABLE_VALIDATION)
static Error x86_validate(uint32_t id, const Operand& o0 = Operand(), const Operand& o1 = Operand(), const Operand& o2 = Operand()) {
  Operand opArray[] = { o0, o1, o2 };
  return X86Inst::validate(Arch::kTypeX86, id, 0, Operand(), opArray, 3);
}

static Error x64_validate(uint32_t id, const Operand& o0 = Operand(), const Operand& o1 = Operand(), const Operand& o2 = Operand()) {
  Operand opArray[] = { o0, o1, o2 };
  return X86Inst::validate(Arch::kTypeX64, id, 0, Operand(), opArray, 3);
}

UNIT(x86_inst_validation) {
  INFO("Validating instructions that use GP registers.");
  EXPECT(x86_validate(X86Inst::kIdCmp   , x86::eax , x86::edx ) == kErrorOk);
  EXPECT(x64_validate(X86Inst::kIdCmp   , x86::rax , x86::rdx ) == kErrorOk);

  EXPECT(x86_validate(X86Inst::kIdCmp   , x86::eax            ) != kErrorOk);
  EXPECT(x86_validate(X86Inst::kIdCmp   , x86::rax , x86::rdx ) != kErrorOk);
  EXPECT(x64_validate(X86Inst::kIdCmp   , x86::rax , x86::al  ) != kErrorOk);

  INFO("Validating instructions that use FP registers.");
  EXPECT(x86_validate(X86Inst::kIdFadd  , x86::fp0 , x86::fp7 ) == kErrorOk);
  EXPECT(x86_validate(X86Inst::kIdFadd  , x86::fp7 , x86::fp0 ) == kErrorOk);
  EXPECT(x86_validate(X86Inst::kIdFadd  , x86::fp0 , x86::eax ) != kErrorOk);
  EXPECT(x86_validate(X86Inst::kIdFadd  , x86::fp4 , x86::fp3 ) != kErrorOk);

  INFO("Validating instructions that use MM registers.");
  EXPECT(x86_validate(X86Inst::kIdPand  , x86::mm0 , x86::mm1 ) == kErrorOk);
  EXPECT(x86_validate(X86Inst::kIdPand  , x86::mm0 , x86::eax ) != kErrorOk);

  INFO("Validating instructions that use XMM registers.");
  EXPECT(x86_validate(X86Inst::kIdPand  , x86::xmm0, x86::xmm1) == kErrorOk);
  EXPECT(x64_validate(X86Inst::kIdPand  , x86::xmm8, x86::xmm9) == kErrorOk);
  EXPECT(x86_validate(X86Inst::kIdPand  , x86::xmm0, x86::eax ) != kErrorOk);
  EXPECT(x64_validate(X86Inst::kIdPand  , x86::xmm0, x86::rax ) != kErrorOk);

  INFO("Validating instructions that use YMM registers.");
  EXPECT(x86_validate(X86Inst::kIdVpand , x86::ymm0, x86::ymm1, x86::ymm2) == kErrorOk);
  EXPECT(x86_validate(X86Inst::kIdVpand , x86::ymm0, x86::ymm1, x86::eax ) != kErrorOk);

  INFO("Validating instructions that use ZMM registers.");
  EXPECT(x86_validate(X86Inst::kIdVpaddw, x86::zmm0, x86::zmm1, x86::zmm2) == kErrorOk);
  EXPECT(x86_validate(X86Inst::kIdVpaddw, x86::zmm0, x86::zmm1, x86::eax ) != kErrorOk);

  INFO("Validating instructions that use CR registers.");
  EXPECT(x86_validate(X86Inst::kIdMov   , x86::eax , x86::cr0 ) == kErrorOk);
  EXPECT(x86_validate(X86Inst::kIdMov   , x86::eax , x86::cr8 ) != kErrorOk);
  EXPECT(x64_validate(X86Inst::kIdMov   , x86::rax , x86::cr8 ) == kErrorOk);
  EXPECT(x64_validate(X86Inst::kIdMov   , x86::eax , x86::cr0 ) != kErrorOk);

  INFO("Validating instructions that use DR registers.");
  EXPECT(x86_validate(X86Inst::kIdMov   , x86::eax , x86::dr0 ) == kErrorOk);
  EXPECT(x64_validate(X86Inst::kIdMov   , x86::rax , x86::dr7 ) == kErrorOk);
  EXPECT(x86_validate(X86Inst::kIdMov   , x86::ax  , x86::dr0 ) != kErrorOk);
  EXPECT(x64_validate(X86Inst::kIdMov   , x86::eax , x86::dr7 ) != kErrorOk);

  INFO("Validating instructions that use segment registers.");
  EXPECT(x86_validate(X86Inst::kIdMov   , x86::ax  , x86::fs  ) == kErrorOk);
  EXPECT(x64_validate(X86Inst::kIdMov   , x86::ax  , x86::cs  ) != kErrorOk);

  INFO("Validating instructions that use memory operands.");
  EXPECT(x86_validate(X86Inst::kIdMov   , x86::eax , x86::ptr(x86::ebx)) == kErrorOk);
  EXPECT(x64_validate(X86Inst::kIdMov   , x86::rax , x86::ptr(x86::rbx)) == kErrorOk);

  INFO("Validating instructions that use immediate values.");
  EXPECT(x86_validate(X86Inst::kIdMov   , x86::eax , imm(1)) == kErrorOk);
  EXPECT(x64_validate(X86Inst::kIdMov   , x86::rax , imm(1)) == kErrorOk);
}
#endif // ASMJIT_TEST && !ASMJIT_DISABLE_VALIDATION

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86
