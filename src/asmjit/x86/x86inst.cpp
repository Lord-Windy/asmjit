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
// information (including registers and flags), and all indexes to all tables.
// ----------------------------------------------------------------------------

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../asmjit_build.h"
#if defined(ASMJIT_BUILD_X86)

// [Dependencies]
#include "../base/utils.h"
#include "../x86/x86inst.h"
#include "../x86/x86operand.h"

// [Api-Begin]
#include "../asmjit_apibegin.h"

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

// Don't store `_nameDataIndex` if instruction names are disabled. Since
// some  APIs can use `_nameDataIndex` it's much safer if it's zero.
#if defined(ASMJIT_DISABLE_TEXT)
# define NAME_DATA_INDEX(X) 0
#else
# define NAME_DATA_INDEX(X) X
#endif

#define A512(CPU, VL, MASKING, SAE_ER, BROADCAST) ( \
  X86Inst::kInstFlagEvex                  | \
  X86Inst::kInstFlagEvexSet_##CPU         | \
  (VL ? X86Inst::kInstFlagEvexVL : 0)     | \
  (X86Inst::kInstFlagEvex##MASKING ? (X86Inst::kInstFlagEvex##MASKING | X86Inst::kInstFlagEvexK_) : 0) | \
  X86Inst::kInstFlagEvex##SAE_ER          | \
  X86Inst::kInstFlagEvexB##BROADCAST        \
)

// Defines an X86/X64 instruction.
#define INST(id, name, encoding, opcode0, opcode1, instFlags, eflags, writeIndex, writeSize, familyType, familyIndex, nameDataIndex, commonIndex) { \
  uint32_t(encoding),                       \
  uint32_t(NAME_DATA_INDEX(nameDataIndex)), \
  uint32_t(commonIndex),                    \
  uint32_t(X86Inst::familyType),            \
  uint32_t(familyIndex),                    \
  0,                                        \
  opcode0                                   \
}

const X86Inst X86InstDB::instData[] = {
  // <-----------------+-------------------+------------------------+------------------+--------+------------------+--------+---------------------------------------+-------------+-------+-----------------+-----+----+
  //                   |                   |                        |    Main OpCode   |#0 EVEX |Alternative OpCode|#1 EVEX |         Instruction Flags             |   E-FLAGS   | Write |    Family       |     |    |
  //  Instruction Id   | Instruction Name  |  Instruction Encoding  |                  +--------+                  +--------+----------------+----------------------+-------------+---+---+------------+----+NameX|ComX|
  //                   |                   |                        |#0:PP-MMM OP/O L|W|W|N|TT. |#1:PP-MMM OP/O L|W|W|N|TT. | Global Flags   |A512(CPU_|V|kz|rnd|b) | EF:OSZAPCDX |Idx|Cnt| Type       |Idx.|     |    |
  // <-----------------+-------------------+------------------------+------------------+--------+------------------+--------+----------------+----------------------+-------------+---+---+------------+----+-----+----+
  // ${instData:Begin}
  INST(None            , ""                , Enc(None)              , 0                         , 0                         , 0                                     , EF(________), 0 , 0 , kFamilyNone, 0  , 0   , 0  ),
  INST(Adc             , "adc"             , Enc(X86Arith)          , O(000000,10,2,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWWWX__), 0 , 0 , kFamilyNone, 0  , 1   , 1  ),
  INST(Adcx            , "adcx"            , Enc(X86Rm)             , O(660F38,F6,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____X__), 0 , 0 , kFamilyNone, 0  , 5   , 2  ),
  INST(Add             , "add"             , Enc(X86Arith)          , O(000000,00,0,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 657 , 3  ),
  INST(Addpd           , "addpd"           , Enc(ExtRm)             , O(660F00,58,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 0  , 4545, 4  ),
  INST(Addps           , "addps"           , Enc(ExtRm)             , O(000F00,58,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 0  , 4557, 4  ),
  INST(Addsd           , "addsd"           , Enc(ExtRm)             , O(F20F00,58,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 0  , 4779, 5  ),
  INST(Addss           , "addss"           , Enc(ExtRm)             , O(F30F00,58,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 0  , 4789, 6  ),
  INST(Addsubpd        , "addsubpd"        , Enc(ExtRm)             , O(660F00,D0,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 0  , 4284, 4  ),
  INST(Addsubps        , "addsubps"        , Enc(ExtRm)             , O(F20F00,D0,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 0  , 4296, 4  ),
  INST(Adox            , "adox"            , Enc(X86Rm)             , O(F30F38,F6,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(X_______), 0 , 0 , kFamilyNone, 0  , 10  , 7  ),
  INST(Aesdec          , "aesdec"          , Enc(ExtRm)             , O(660F38,DE,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 1  , 2790, 4  ),
  INST(Aesdeclast      , "aesdeclast"      , Enc(ExtRm)             , O(660F38,DF,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 1  , 2798, 4  ),
  INST(Aesenc          , "aesenc"          , Enc(ExtRm)             , O(660F38,DC,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 1  , 2810, 4  ),
  INST(Aesenclast      , "aesenclast"      , Enc(ExtRm)             , O(660F38,DD,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 1  , 2818, 4  ),
  INST(Aesimc          , "aesimc"          , Enc(ExtRm)             , O(660F38,DB,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , kFamilySse , 2  , 2830, 8  ),
  INST(Aeskeygenassist , "aeskeygenassist" , Enc(ExtRmi)            , O(660F3A,DF,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , kFamilySse , 2  , 2838, 9  ),
  INST(And             , "and"             , Enc(X86Arith)          , O(000000,20,4,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWUWW__), 0 , 0 , kFamilyNone, 0  , 2034, 3  ),
  INST(Andn            , "andn"            , Enc(VexRvm_Wx)         , V(000F38,F2,_,0,x,_,_,_  ), 0                         , F(RW)                                 , EF(WWWUUW__), 0 , 0 , kFamilyNone, 0  , 6055, 10 ),
  INST(Andnpd          , "andnpd"          , Enc(ExtRm)             , O(660F00,55,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 1  , 2871, 4  ),
  INST(Andnps          , "andnps"          , Enc(ExtRm)             , O(000F00,55,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 1  , 2879, 4  ),
  INST(Andpd           , "andpd"           , Enc(ExtRm)             , O(660F00,54,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 1  , 3798, 4  ),
  INST(Andps           , "andps"           , Enc(ExtRm)             , O(000F00,54,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 1  , 3808, 4  ),
  INST(Bextr           , "bextr"           , Enc(VexRmv_Wx)         , V(000F38,F7,_,0,x,_,_,_  ), 0                         , F(RW)                                 , EF(WUWUUW__), 0 , 0 , kFamilyNone, 0  , 15  , 11 ),
  INST(Blcfill         , "blcfill"         , Enc(VexVm_Wx)          , V(XOP_M9,01,1,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , kFamilyNone, 0  , 21  , 12 ),
  INST(Blci            , "blci"            , Enc(VexVm_Wx)          , V(XOP_M9,02,6,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , kFamilyNone, 0  , 29  , 12 ),
  INST(Blcic           , "blcic"           , Enc(VexVm_Wx)          , V(XOP_M9,01,5,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , kFamilyNone, 0  , 34  , 12 ),
  INST(Blcmsk          , "blcmsk"          , Enc(VexVm_Wx)          , V(XOP_M9,02,1,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , kFamilyNone, 0  , 40  , 12 ),
  INST(Blcs            , "blcs"            , Enc(VexVm_Wx)          , V(XOP_M9,01,3,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , kFamilyNone, 0  , 47  , 12 ),
  INST(Blendpd         , "blendpd"         , Enc(ExtRmi)            , O(660F3A,0D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 1  , 2957, 13 ),
  INST(Blendps         , "blendps"         , Enc(ExtRmi)            , O(660F3A,0C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 1  , 2966, 13 ),
  INST(Blendvpd        , "blendvpd"        , Enc(ExtRmXMM0)         , O(660F38,15,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , kFamilySse , 3  , 2975, 14 ),
  INST(Blendvps        , "blendvps"        , Enc(ExtRmXMM0)         , O(660F38,14,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , kFamilySse , 3  , 2985, 14 ),
  INST(Blsfill         , "blsfill"         , Enc(VexVm_Wx)          , V(XOP_M9,01,2,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , kFamilyNone, 0  , 52  , 12 ),
  INST(Blsi            , "blsi"            , Enc(VexVm_Wx)          , V(000F38,F3,3,0,x,_,_,_  ), 0                         , F(RW)                                 , EF(WWWUUW__), 0 , 0 , kFamilyNone, 0  , 60  , 15 ),
  INST(Blsic           , "blsic"           , Enc(VexVm_Wx)          , V(XOP_M9,01,6,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , kFamilyNone, 0  , 65  , 12 ),
  INST(Blsmsk          , "blsmsk"          , Enc(VexVm_Wx)          , V(000F38,F3,2,0,x,_,_,_  ), 0                         , F(RW)                                 , EF(WWWUUW__), 0 , 0 , kFamilyNone, 0  , 71  , 15 ),
  INST(Blsr            , "blsr"            , Enc(VexVm_Wx)          , V(000F38,F3,1,0,x,_,_,_  ), 0                         , F(RW)                                 , EF(WWWUUW__), 0 , 0 , kFamilyNone, 0  , 78  , 15 ),
  INST(Bsf             , "bsf"             , Enc(X86Rm)             , O(000F00,BC,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(UUWUUU__), 0 , 0 , kFamilyNone, 0  , 83  , 16 ),
  INST(Bsr             , "bsr"             , Enc(X86Rm)             , O(000F00,BD,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(UUWUUU__), 0 , 0 , kFamilyNone, 0  , 87  , 16 ),
  INST(Bswap           , "bswap"           , Enc(X86Bswap)          , O(000F00,C8,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 91  , 17 ),
  INST(Bt              , "bt"              , Enc(X86Bt)             , O(000F00,A3,_,_,x,_,_,_  ), O(000F00,BA,4,_,x,_,_,_  ), F(RO)                                 , EF(UU_UUW__), 0 , 0 , kFamilyNone, 0  , 97  , 18 ),
  INST(Btc             , "btc"             , Enc(X86Bt)             , O(000F00,BB,_,_,x,_,_,_  ), O(000F00,BA,7,_,x,_,_,_  ), F(RW)|F(Lock)                         , EF(UU_UUW__), 0 , 0 , kFamilyNone, 0  , 100 , 19 ),
  INST(Btr             , "btr"             , Enc(X86Bt)             , O(000F00,B3,_,_,x,_,_,_  ), O(000F00,BA,6,_,x,_,_,_  ), F(RW)|F(Lock)                         , EF(UU_UUW__), 0 , 0 , kFamilyNone, 0  , 104 , 20 ),
  INST(Bts             , "bts"             , Enc(X86Bt)             , O(000F00,AB,_,_,x,_,_,_  ), O(000F00,BA,5,_,x,_,_,_  ), F(RW)|F(Lock)                         , EF(UU_UUW__), 0 , 0 , kFamilyNone, 0  , 108 , 21 ),
  INST(Bzhi            , "bzhi"            , Enc(VexRmv_Wx)         , V(000F38,F5,_,0,x,_,_,_  ), 0                         , F(RW)                                 , EF(WWWUUW__), 0 , 0 , kFamilyNone, 0  , 112 , 11 ),
  INST(Call            , "call"            , Enc(X86Call)           , O(000000,FF,2,_,_,_,_,_  ), 0                         , F(RW)|F(Flow)|F(Volatile)             , EF(________), 0 , 0 , kFamilyNone, 0  , 117 , 22 ),
  INST(Cbw             , "cbw"             , Enc(X86OpAx)           , O(660000,98,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , kFamilyNone, 0  , 122 , 23 ),
  INST(Cdq             , "cdq"             , Enc(X86OpDxAx)         , O(000000,99,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , kFamilyNone, 0  , 126 , 24 ),
  INST(Cdqe            , "cdqe"            , Enc(X86OpAx)           , O(000000,98,_,_,1,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , kFamilyNone, 0  , 130 , 25 ),
  INST(Clac            , "clac"            , Enc(X86Op)             , O(000F01,CA,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(___W____), 0 , 0 , kFamilyNone, 0  , 135 , 26 ),
  INST(Clc             , "clc"             , Enc(X86Op)             , O(000000,F8,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(_____W__), 0 , 0 , kFamilyNone, 0  , 140 , 27 ),
  INST(Cld             , "cld"             , Enc(X86Op)             , O(000000,FC,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(______W_), 0 , 0 , kFamilyNone, 0  , 144 , 28 ),
  INST(Clflush         , "clflush"         , Enc(X86M_Only)         , O(000F00,AE,7,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , kFamilyNone, 0  , 148 , 29 ),
  INST(Clflushopt      , "clflushopt"      , Enc(X86M_Only)         , O(660F00,AE,7,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , kFamilyNone, 0  , 156 , 29 ),
  INST(Clwb            , "clwb"            , Enc(X86M_Only)         , O(660F00,AE,6,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , kFamilyNone, 0  , 167 , 29 ),
  INST(Clzero          , "clzero"          , Enc(X86Op)             , O(000F01,FC,_,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 172 , 30 ),
  INST(Cmc             , "cmc"             , Enc(X86Op)             , O(000000,F5,_,_,_,_,_,_  ), 0                         , 0                                     , EF(_____X__), 0 , 0 , kFamilyNone, 0  , 179 , 31 ),
  INST(Cmova           , "cmova"           , Enc(X86Rm)             , O(000F00,47,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R__R__), 0 , 0 , kFamilyNone, 0  , 183 , 32 ),
  INST(Cmovae          , "cmovae"          , Enc(X86Rm)             , O(000F00,43,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____R__), 0 , 0 , kFamilyNone, 0  , 189 , 33 ),
  INST(Cmovb           , "cmovb"           , Enc(X86Rm)             , O(000F00,42,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____R__), 0 , 0 , kFamilyNone, 0  , 514 , 33 ),
  INST(Cmovbe          , "cmovbe"          , Enc(X86Rm)             , O(000F00,46,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R__R__), 0 , 0 , kFamilyNone, 0  , 521 , 32 ),
  INST(Cmovc           , "cmovc"           , Enc(X86Rm)             , O(000F00,42,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____R__), 0 , 0 , kFamilyNone, 0  , 196 , 33 ),
  INST(Cmove           , "cmove"           , Enc(X86Rm)             , O(000F00,44,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R_____), 0 , 0 , kFamilyNone, 0  , 529 , 34 ),
  INST(Cmovg           , "cmovg"           , Enc(X86Rm)             , O(000F00,4F,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RRR_____), 0 , 0 , kFamilyNone, 0  , 202 , 35 ),
  INST(Cmovge          , "cmovge"          , Enc(X86Rm)             , O(000F00,4D,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RR______), 0 , 0 , kFamilyNone, 0  , 208 , 36 ),
  INST(Cmovl           , "cmovl"           , Enc(X86Rm)             , O(000F00,4C,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RR______), 0 , 0 , kFamilyNone, 0  , 215 , 36 ),
  INST(Cmovle          , "cmovle"          , Enc(X86Rm)             , O(000F00,4E,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RRR_____), 0 , 0 , kFamilyNone, 0  , 221 , 35 ),
  INST(Cmovna          , "cmovna"          , Enc(X86Rm)             , O(000F00,46,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R__R__), 0 , 0 , kFamilyNone, 0  , 228 , 32 ),
  INST(Cmovnae         , "cmovnae"         , Enc(X86Rm)             , O(000F00,42,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____R__), 0 , 0 , kFamilyNone, 0  , 235 , 33 ),
  INST(Cmovnb          , "cmovnb"          , Enc(X86Rm)             , O(000F00,43,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____R__), 0 , 0 , kFamilyNone, 0  , 536 , 33 ),
  INST(Cmovnbe         , "cmovnbe"         , Enc(X86Rm)             , O(000F00,47,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R__R__), 0 , 0 , kFamilyNone, 0  , 544 , 32 ),
  INST(Cmovnc          , "cmovnc"          , Enc(X86Rm)             , O(000F00,43,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____R__), 0 , 0 , kFamilyNone, 0  , 243 , 33 ),
  INST(Cmovne          , "cmovne"          , Enc(X86Rm)             , O(000F00,45,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R_____), 0 , 0 , kFamilyNone, 0  , 553 , 34 ),
  INST(Cmovng          , "cmovng"          , Enc(X86Rm)             , O(000F00,4E,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RRR_____), 0 , 0 , kFamilyNone, 0  , 250 , 35 ),
  INST(Cmovnge         , "cmovnge"         , Enc(X86Rm)             , O(000F00,4C,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RR______), 0 , 0 , kFamilyNone, 0  , 257 , 36 ),
  INST(Cmovnl          , "cmovnl"          , Enc(X86Rm)             , O(000F00,4D,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RR______), 0 , 0 , kFamilyNone, 0  , 265 , 36 ),
  INST(Cmovnle         , "cmovnle"         , Enc(X86Rm)             , O(000F00,4F,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RRR_____), 0 , 0 , kFamilyNone, 0  , 272 , 35 ),
  INST(Cmovno          , "cmovno"          , Enc(X86Rm)             , O(000F00,41,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(R_______), 0 , 0 , kFamilyNone, 0  , 280 , 37 ),
  INST(Cmovnp          , "cmovnp"          , Enc(X86Rm)             , O(000F00,4B,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(____R___), 0 , 0 , kFamilyNone, 0  , 287 , 38 ),
  INST(Cmovns          , "cmovns"          , Enc(X86Rm)             , O(000F00,49,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_R______), 0 , 0 , kFamilyNone, 0  , 294 , 39 ),
  INST(Cmovnz          , "cmovnz"          , Enc(X86Rm)             , O(000F00,45,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R_____), 0 , 0 , kFamilyNone, 0  , 301 , 34 ),
  INST(Cmovo           , "cmovo"           , Enc(X86Rm)             , O(000F00,40,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(R_______), 0 , 0 , kFamilyNone, 0  , 308 , 37 ),
  INST(Cmovp           , "cmovp"           , Enc(X86Rm)             , O(000F00,4A,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(____R___), 0 , 0 , kFamilyNone, 0  , 314 , 38 ),
  INST(Cmovpe          , "cmovpe"          , Enc(X86Rm)             , O(000F00,4A,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(____R___), 0 , 0 , kFamilyNone, 0  , 320 , 38 ),
  INST(Cmovpo          , "cmovpo"          , Enc(X86Rm)             , O(000F00,4B,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(____R___), 0 , 0 , kFamilyNone, 0  , 327 , 38 ),
  INST(Cmovs           , "cmovs"           , Enc(X86Rm)             , O(000F00,48,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_R______), 0 , 0 , kFamilyNone, 0  , 334 , 39 ),
  INST(Cmovz           , "cmovz"           , Enc(X86Rm)             , O(000F00,44,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R_____), 0 , 0 , kFamilyNone, 0  , 340 , 34 ),
  INST(Cmp             , "cmp"             , Enc(X86Arith)          , O(000000,38,7,_,x,_,_,_  ), 0                         , F(RO)                                 , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 346 , 40 ),
  INST(Cmppd           , "cmppd"           , Enc(ExtRmi)            , O(660F00,C2,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 4  , 3211, 13 ),
  INST(Cmpps           , "cmpps"           , Enc(ExtRmi)            , O(000F00,C2,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 4  , 3218, 13 ),
  INST(CmpsB           , "cmps_b"          , Enc(X86Op)             , O(000000,A6,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2292, 41 ),
  INST(CmpsD           , "cmps_d"          , Enc(X86Op)             , O(000000,A7,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2305, 41 ),
  INST(CmpsQ           , "cmps_q"          , Enc(X86Op)             , O(000000,A7,_,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2318, 41 ),
  INST(CmpsW           , "cmps_w"          , Enc(X86Op)             , O(660000,A7,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2331, 41 ),
  INST(Cmpsd           , "cmpsd"           , Enc(ExtRmi)            , O(F20F00,C2,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 3225, 42 ),
  INST(Cmpss           , "cmpss"           , Enc(ExtRmi)            , O(F30F00,C2,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 5  , 3232, 43 ),
  INST(Cmpxchg         , "cmpxchg"         , Enc(X86Cmpxchg)        , O(000F00,B0,_,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)|F(Special)              , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 350 , 44 ),
  INST(Cmpxchg16b      , "cmpxchg16b"      , Enc(X86M_Only)         , O(000F00,C7,1,_,1,_,_,_  ), 0                         , F(RW)|F(Lock)|F(Special)              , EF(__W_____), 0 , 0 , kFamilyNone, 0  , 358 , 45 ),
  INST(Cmpxchg8b       , "cmpxchg8b"       , Enc(X86M_Only)         , O(000F00,C7,1,_,_,_,_,_  ), 0                         , F(RW)|F(Lock)|F(Special)              , EF(__W_____), 0 , 0 , kFamilyNone, 0  , 369 , 46 ),
  INST(Comisd          , "comisd"          , Enc(ExtRm)             , O(660F00,2F,_,_,_,_,_,_  ), 0                         , F(RO)                                 , EF(WWWWWW__), 0 , 0 , kFamilySse , 6  , 9258, 47 ),
  INST(Comiss          , "comiss"          , Enc(ExtRm)             , O(000F00,2F,_,_,_,_,_,_  ), 0                         , F(RO)                                 , EF(WWWWWW__), 0 , 0 , kFamilySse , 6  , 9267, 48 ),
  INST(Cpuid           , "cpuid"           , Enc(X86Op)             , O(000F00,A2,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 379 , 49 ),
  INST(Cqo             , "cqo"             , Enc(X86OpDxAx)         , O(000000,99,_,_,1,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , kFamilyNone, 0  , 385 , 50 ),
  INST(Crc32           , "crc32"           , Enc(X86Crc)            , O(F20F38,F0,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 389 , 51 ),
  INST(Cvtdq2pd        , "cvtdq2pd"        , Enc(ExtRm)             , O(F30F00,E6,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 7  , 3279, 52 ),
  INST(Cvtdq2ps        , "cvtdq2ps"        , Enc(ExtRm)             , O(000F00,5B,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 7  , 3289, 53 ),
  INST(Cvtpd2dq        , "cvtpd2dq"        , Enc(ExtRm)             , O(F20F00,E6,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 7  , 3299, 53 ),
  INST(Cvtpd2pi        , "cvtpd2pi"        , Enc(ExtRm)             , O(660F00,2D,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 8  , 395 , 54 ),
  INST(Cvtpd2ps        , "cvtpd2ps"        , Enc(ExtRm)             , O(660F00,5A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 9  , 3309, 53 ),
  INST(Cvtpi2pd        , "cvtpi2pd"        , Enc(ExtRm)             , O(660F00,2A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 8  , 404 , 55 ),
  INST(Cvtpi2ps        , "cvtpi2ps"        , Enc(ExtRm)             , O(000F00,2A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 8  , 413 , 56 ),
  INST(Cvtps2dq        , "cvtps2dq"        , Enc(ExtRm)             , O(660F00,5B,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 6  , 3361, 53 ),
  INST(Cvtps2pd        , "cvtps2pd"        , Enc(ExtRm)             , O(000F00,5A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 6  , 3371, 52 ),
  INST(Cvtps2pi        , "cvtps2pi"        , Enc(ExtRm)             , O(000F00,2D,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 8  , 422 , 57 ),
  INST(Cvtsd2si        , "cvtsd2si"        , Enc(ExtRm_Wx)          , O(F20F00,2D,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 10 , 3443, 58 ),
  INST(Cvtsd2ss        , "cvtsd2ss"        , Enc(ExtRm)             , O(F20F00,5A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 4 , kFamilySse , 11 , 3453, 59 ),
  INST(Cvtsi2sd        , "cvtsi2sd"        , Enc(ExtRm_Wx)          , O(F20F00,2A,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 12 , 3474, 60 ),
  INST(Cvtsi2ss        , "cvtsi2ss"        , Enc(ExtRm_Wx)          , O(F30F00,2A,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 4 , kFamilySse , 12 , 3484, 61 ),
  INST(Cvtss2sd        , "cvtss2sd"        , Enc(ExtRm)             , O(F30F00,5A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 12 , 3494, 62 ),
  INST(Cvtss2si        , "cvtss2si"        , Enc(ExtRm_Wx)          , O(F30F00,2D,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 13 , 3504, 63 ),
  INST(Cvttpd2dq       , "cvttpd2dq"       , Enc(ExtRm)             , O(660F00,E6,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 14 , 3525, 53 ),
  INST(Cvttpd2pi       , "cvttpd2pi"       , Enc(ExtRm)             , O(660F00,2C,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 8  , 431 , 54 ),
  INST(Cvttps2dq       , "cvttps2dq"       , Enc(ExtRm)             , O(F30F00,5B,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 15 , 3571, 53 ),
  INST(Cvttps2pi       , "cvttps2pi"       , Enc(ExtRm)             , O(000F00,2C,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 8  , 441 , 57 ),
  INST(Cvttsd2si       , "cvttsd2si"       , Enc(ExtRm_Wx)          , O(F20F00,2C,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 16 , 3617, 58 ),
  INST(Cvttss2si       , "cvttss2si"       , Enc(ExtRm_Wx)          , O(F30F00,2C,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 17 , 3640, 63 ),
  INST(Cwd             , "cwd"             , Enc(X86OpDxAx)         , O(660000,99,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , kFamilyNone, 0  , 451 , 64 ),
  INST(Cwde            , "cwde"            , Enc(X86OpAx)           , O(000000,98,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , kFamilyNone, 0  , 455 , 65 ),
  INST(Daa             , "daa"             , Enc(X86Op)             , O(000000,27,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(UWWXWX__), 0 , 0 , kFamilyNone, 0  , 460 , 66 ),
  INST(Das             , "das"             , Enc(X86Op)             , O(000000,2F,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(UWWXWX__), 0 , 0 , kFamilyNone, 0  , 464 , 66 ),
  INST(Dec             , "dec"             , Enc(X86IncDec)         , O(000000,FE,1,_,x,_,_,_  ), O(000000,48,_,_,x,_,_,_  ), F(RW)|F(Lock)                         , EF(WWWWW___), 0 , 0 , kFamilyNone, 0  , 2793, 67 ),
  INST(Div             , "div"             , Enc(X86M_Bx_MulDiv)    , O(000000,F6,6,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(UUUUUU__), 0 , 0 , kFamilyNone, 0  , 676 , 68 ),
  INST(Divpd           , "divpd"           , Enc(ExtRm)             , O(660F00,5E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 18 , 3739, 4  ),
  INST(Divps           , "divps"           , Enc(ExtRm)             , O(000F00,5E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 18 , 3746, 4  ),
  INST(Divsd           , "divsd"           , Enc(ExtRm)             , O(F20F00,5E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 18 , 3753, 5  ),
  INST(Divss           , "divss"           , Enc(ExtRm)             , O(F30F00,5E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 18 , 3760, 6  ),
  INST(Dppd            , "dppd"            , Enc(ExtRmi)            , O(660F3A,41,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 18 , 3767, 13 ),
  INST(Dpps            , "dpps"            , Enc(ExtRmi)            , O(660F3A,40,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 18 , 3773, 13 ),
  INST(Emms            , "emms"            , Enc(X86Op)             , O(000F00,77,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(________), 0 , 0 , kFamilyNone, 0  , 644 , 69 ),
  INST(Enter           , "enter"           , Enc(X86Enter)          , O(000000,C8,_,_,_,_,_,_  ), 0                         , F(Volatile)|F(Special)                , EF(________), 0 , 0 , kFamilyNone, 0  , 468 , 70 ),
  INST(Extractps       , "extractps"       , Enc(ExtExtract)        , O(660F3A,17,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 19 , 3953, 71 ),
  INST(Extrq           , "extrq"           , Enc(ExtExtrq)          , O(660F00,79,_,_,_,_,_,_  ), O(660F00,78,0,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 6769, 72 ),
  INST(F2xm1           , "f2xm1"           , Enc(FpuOp)             , O_FPU(00,D9F0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 474 , 73 ),
  INST(Fabs            , "fabs"            , Enc(FpuOp)             , O_FPU(00,D9E1,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 480 , 73 ),
  INST(Fadd            , "fadd"            , Enc(FpuArith)          , O_FPU(00,C0C0,0)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , kFamilyNone, 0  , 1756, 74 ),
  INST(Faddp           , "faddp"           , Enc(FpuRDef)           , O_FPU(00,DEC0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 485 , 75 ),
  INST(Fbld            , "fbld"            , Enc(X86M_Only)         , O_FPU(00,00DF,4)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 491 , 76 ),
  INST(Fbstp           , "fbstp"           , Enc(X86M_Only)         , O_FPU(00,00DF,6)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 496 , 76 ),
  INST(Fchs            , "fchs"            , Enc(FpuOp)             , O_FPU(00,D9E0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 502 , 73 ),
  INST(Fclex           , "fclex"           , Enc(FpuOp)             , O_FPU(9B,DBE2,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 507 , 73 ),
  INST(Fcmovb          , "fcmovb"          , Enc(FpuR)              , O_FPU(00,DAC0,_)          , 0                         , F(Fp)                                 , EF(_____R__), 0 , 0 , kFamilyNone, 0  , 513 , 77 ),
  INST(Fcmovbe         , "fcmovbe"         , Enc(FpuR)              , O_FPU(00,DAD0,_)          , 0                         , F(Fp)                                 , EF(__R__R__), 0 , 0 , kFamilyNone, 0  , 520 , 78 ),
  INST(Fcmove          , "fcmove"          , Enc(FpuR)              , O_FPU(00,DAC8,_)          , 0                         , F(Fp)                                 , EF(__R_____), 0 , 0 , kFamilyNone, 0  , 528 , 79 ),
  INST(Fcmovnb         , "fcmovnb"         , Enc(FpuR)              , O_FPU(00,DBC0,_)          , 0                         , F(Fp)                                 , EF(_____R__), 0 , 0 , kFamilyNone, 0  , 535 , 77 ),
  INST(Fcmovnbe        , "fcmovnbe"        , Enc(FpuR)              , O_FPU(00,DBD0,_)          , 0                         , F(Fp)                                 , EF(__R__R__), 0 , 0 , kFamilyNone, 0  , 543 , 78 ),
  INST(Fcmovne         , "fcmovne"         , Enc(FpuR)              , O_FPU(00,DBC8,_)          , 0                         , F(Fp)                                 , EF(__R_____), 0 , 0 , kFamilyNone, 0  , 552 , 79 ),
  INST(Fcmovnu         , "fcmovnu"         , Enc(FpuR)              , O_FPU(00,DBD8,_)          , 0                         , F(Fp)                                 , EF(____R___), 0 , 0 , kFamilyNone, 0  , 560 , 80 ),
  INST(Fcmovu          , "fcmovu"          , Enc(FpuR)              , O_FPU(00,DAD8,_)          , 0                         , F(Fp)                                 , EF(____R___), 0 , 0 , kFamilyNone, 0  , 568 , 80 ),
  INST(Fcom            , "fcom"            , Enc(FpuCom)            , O_FPU(00,D0D0,2)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 575 , 81 ),
  INST(Fcomi           , "fcomi"           , Enc(FpuR)              , O_FPU(00,DBF0,_)          , 0                         , F(Fp)                                 , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 580 , 82 ),
  INST(Fcomip          , "fcomip"          , Enc(FpuR)              , O_FPU(00,DFF0,_)          , 0                         , F(Fp)                                 , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 586 , 82 ),
  INST(Fcomp           , "fcomp"           , Enc(FpuCom)            , O_FPU(00,D8D8,3)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 593 , 81 ),
  INST(Fcompp          , "fcompp"          , Enc(FpuOp)             , O_FPU(00,DED9,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 599 , 73 ),
  INST(Fcos            , "fcos"            , Enc(FpuOp)             , O_FPU(00,D9FF,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 606 , 73 ),
  INST(Fdecstp         , "fdecstp"         , Enc(FpuOp)             , O_FPU(00,D9F6,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 611 , 73 ),
  INST(Fdiv            , "fdiv"            , Enc(FpuArith)          , O_FPU(00,F0F8,6)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , kFamilyNone, 0  , 619 , 74 ),
  INST(Fdivp           , "fdivp"           , Enc(FpuRDef)           , O_FPU(00,DEF8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 624 , 75 ),
  INST(Fdivr           , "fdivr"           , Enc(FpuArith)          , O_FPU(00,F8F0,7)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , kFamilyNone, 0  , 630 , 74 ),
  INST(Fdivrp          , "fdivrp"          , Enc(FpuRDef)           , O_FPU(00,DEF0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 636 , 75 ),
  INST(Femms           , "femms"           , Enc(X86Op)             , O(000F00,0E,_,_,_,_,_,_  ), 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 643 , 73 ),
  INST(Ffree           , "ffree"           , Enc(FpuR)              , O_FPU(00,DDC0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 649 , 83 ),
  INST(Fiadd           , "fiadd"           , Enc(FpuM)              , O_FPU(00,00DA,0)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , kFamilyNone, 0  , 655 , 84 ),
  INST(Ficom           , "ficom"           , Enc(FpuM)              , O_FPU(00,00DA,2)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , kFamilyNone, 0  , 661 , 84 ),
  INST(Ficomp          , "ficomp"          , Enc(FpuM)              , O_FPU(00,00DA,3)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , kFamilyNone, 0  , 667 , 84 ),
  INST(Fidiv           , "fidiv"           , Enc(FpuM)              , O_FPU(00,00DA,6)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , kFamilyNone, 0  , 674 , 84 ),
  INST(Fidivr          , "fidivr"          , Enc(FpuM)              , O_FPU(00,00DA,7)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , kFamilyNone, 0  , 680 , 84 ),
  INST(Fild            , "fild"            , Enc(FpuM)              , O_FPU(00,00DB,0)          , O_FPU(00,00DF,5)          , F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , EF(________), 0 , 0 , kFamilyNone, 0  , 687 , 85 ),
  INST(Fimul           , "fimul"           , Enc(FpuM)              , O_FPU(00,00DA,1)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , kFamilyNone, 0  , 692 , 84 ),
  INST(Fincstp         , "fincstp"         , Enc(FpuOp)             , O_FPU(00,D9F7,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 698 , 73 ),
  INST(Finit           , "finit"           , Enc(FpuOp)             , O_FPU(9B,DBE3,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 706 , 73 ),
  INST(Fist            , "fist"            , Enc(FpuM)              , O_FPU(00,00DB,2)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , kFamilyNone, 0  , 712 , 84 ),
  INST(Fistp           , "fistp"           , Enc(FpuM)              , O_FPU(00,00DB,3)          , O_FPU(00,00DF,7)          , F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , EF(________), 0 , 0 , kFamilyNone, 0  , 717 , 86 ),
  INST(Fisttp          , "fisttp"          , Enc(FpuM)              , O_FPU(00,00DB,1)          , O_FPU(00,00DD,1)          , F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , EF(________), 0 , 0 , kFamilyNone, 0  , 723 , 87 ),
  INST(Fisub           , "fisub"           , Enc(FpuM)              , O_FPU(00,00DA,4)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , kFamilyNone, 0  , 730 , 84 ),
  INST(Fisubr          , "fisubr"          , Enc(FpuM)              , O_FPU(00,00DA,5)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , kFamilyNone, 0  , 736 , 84 ),
  INST(Fld             , "fld"             , Enc(FpuFldFst)         , O_FPU(00,00D9,0)          , O_FPU(00,00DB,5)          , F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , EF(________), 0 , 0 , kFamilyNone, 0  , 743 , 88 ),
  INST(Fld1            , "fld1"            , Enc(FpuOp)             , O_FPU(00,D9E8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 747 , 73 ),
  INST(Fldcw           , "fldcw"           , Enc(X86M_Only)         , O_FPU(00,00D9,5)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 752 , 89 ),
  INST(Fldenv          , "fldenv"          , Enc(X86M_Only)         , O_FPU(00,00D9,4)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 758 , 90 ),
  INST(Fldl2e          , "fldl2e"          , Enc(FpuOp)             , O_FPU(00,D9EA,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 765 , 73 ),
  INST(Fldl2t          , "fldl2t"          , Enc(FpuOp)             , O_FPU(00,D9E9,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 772 , 73 ),
  INST(Fldlg2          , "fldlg2"          , Enc(FpuOp)             , O_FPU(00,D9EC,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 779 , 73 ),
  INST(Fldln2          , "fldln2"          , Enc(FpuOp)             , O_FPU(00,D9ED,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 786 , 73 ),
  INST(Fldpi           , "fldpi"           , Enc(FpuOp)             , O_FPU(00,D9EB,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 793 , 73 ),
  INST(Fldz            , "fldz"            , Enc(FpuOp)             , O_FPU(00,D9EE,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 799 , 73 ),
  INST(Fmul            , "fmul"            , Enc(FpuArith)          , O_FPU(00,C8C8,1)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , kFamilyNone, 0  , 1798, 74 ),
  INST(Fmulp           , "fmulp"           , Enc(FpuRDef)           , O_FPU(00,DEC8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 804 , 75 ),
  INST(Fnclex          , "fnclex"          , Enc(FpuOp)             , O_FPU(00,DBE2,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 810 , 73 ),
  INST(Fninit          , "fninit"          , Enc(FpuOp)             , O_FPU(00,DBE3,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 817 , 73 ),
  INST(Fnop            , "fnop"            , Enc(FpuOp)             , O_FPU(00,D9D0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 824 , 73 ),
  INST(Fnsave          , "fnsave"          , Enc(X86M_Only)         , O_FPU(00,00DD,6)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 829 , 90 ),
  INST(Fnstcw          , "fnstcw"          , Enc(X86M_Only)         , O_FPU(00,00D9,7)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 836 , 89 ),
  INST(Fnstenv         , "fnstenv"         , Enc(X86M_Only)         , O_FPU(00,00D9,6)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 843 , 90 ),
  INST(Fnstsw          , "fnstsw"          , Enc(FpuStsw)           , O_FPU(00,00DD,7)          , O_FPU(00,DFE0,_)          , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 851 , 91 ),
  INST(Fpatan          , "fpatan"          , Enc(FpuOp)             , O_FPU(00,D9F3,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 858 , 73 ),
  INST(Fprem           , "fprem"           , Enc(FpuOp)             , O_FPU(00,D9F8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 865 , 73 ),
  INST(Fprem1          , "fprem1"          , Enc(FpuOp)             , O_FPU(00,D9F5,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 871 , 73 ),
  INST(Fptan           , "fptan"           , Enc(FpuOp)             , O_FPU(00,D9F2,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 878 , 73 ),
  INST(Frndint         , "frndint"         , Enc(FpuOp)             , O_FPU(00,D9FC,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 884 , 73 ),
  INST(Frstor          , "frstor"          , Enc(X86M_Only)         , O_FPU(00,00DD,4)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 892 , 90 ),
  INST(Fsave           , "fsave"           , Enc(X86M_Only)         , O_FPU(9B,00DD,6)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 899 , 90 ),
  INST(Fscale          , "fscale"          , Enc(FpuOp)             , O_FPU(00,D9FD,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 905 , 73 ),
  INST(Fsin            , "fsin"            , Enc(FpuOp)             , O_FPU(00,D9FE,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 912 , 73 ),
  INST(Fsincos         , "fsincos"         , Enc(FpuOp)             , O_FPU(00,D9FB,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 917 , 73 ),
  INST(Fsqrt           , "fsqrt"           , Enc(FpuOp)             , O_FPU(00,D9FA,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 925 , 73 ),
  INST(Fst             , "fst"             , Enc(FpuFldFst)         , O_FPU(00,00D9,2)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , kFamilyNone, 0  , 931 , 92 ),
  INST(Fstcw           , "fstcw"           , Enc(X86M_Only)         , O_FPU(9B,00D9,7)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 935 , 89 ),
  INST(Fstenv          , "fstenv"          , Enc(X86M_Only)         , O_FPU(9B,00D9,6)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 941 , 90 ),
  INST(Fstp            , "fstp"            , Enc(FpuFldFst)         , O_FPU(00,00D9,3)          , O(000000,DB,7,_,_,_,_,_  ), F(Fp)|F(FPU_M4)|F(FPU_M8)|F(FPU_M10)  , EF(________), 0 , 0 , kFamilyNone, 0  , 948 , 93 ),
  INST(Fstsw           , "fstsw"           , Enc(FpuStsw)           , O_FPU(9B,00DD,7)          , O_FPU(9B,DFE0,_)          , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 953 , 94 ),
  INST(Fsub            , "fsub"            , Enc(FpuArith)          , O_FPU(00,E0E8,4)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , kFamilyNone, 0  , 1876, 74 ),
  INST(Fsubp           , "fsubp"           , Enc(FpuRDef)           , O_FPU(00,DEE8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 959 , 75 ),
  INST(Fsubr           , "fsubr"           , Enc(FpuArith)          , O_FPU(00,E8E0,5)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , kFamilyNone, 0  , 1882, 74 ),
  INST(Fsubrp          , "fsubrp"          , Enc(FpuRDef)           , O_FPU(00,DEE0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 965 , 75 ),
  INST(Ftst            , "ftst"            , Enc(FpuOp)             , O_FPU(00,D9E4,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 972 , 73 ),
  INST(Fucom           , "fucom"           , Enc(FpuRDef)           , O_FPU(00,DDE0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 977 , 75 ),
  INST(Fucomi          , "fucomi"          , Enc(FpuR)              , O_FPU(00,DBE8,_)          , 0                         , F(Fp)                                 , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 983 , 82 ),
  INST(Fucomip         , "fucomip"         , Enc(FpuR)              , O_FPU(00,DFE8,_)          , 0                         , F(Fp)                                 , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 990 , 82 ),
  INST(Fucomp          , "fucomp"          , Enc(FpuRDef)           , O_FPU(00,DDE8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 998 , 75 ),
  INST(Fucompp         , "fucompp"         , Enc(FpuOp)             , O_FPU(00,DAE9,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 1005, 73 ),
  INST(Fwait           , "fwait"           , Enc(X86Op)             , O_FPU(00,00DB,_)          , 0                         , F(Fp)|F(Volatile)                     , EF(________), 0 , 0 , kFamilyNone, 0  , 1013, 95 ),
  INST(Fxam            , "fxam"            , Enc(FpuOp)             , O_FPU(00,D9E5,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 1019, 73 ),
  INST(Fxch            , "fxch"            , Enc(FpuR)              , O_FPU(00,D9C8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 1024, 75 ),
  INST(Fxrstor         , "fxrstor"         , Enc(X86M_Only)         , O(000F00,AE,1,_,_,_,_,_  ), 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 1029, 90 ),
  INST(Fxrstor64       , "fxrstor64"       , Enc(X86M_Only)         , O(000F00,AE,1,_,1,_,_,_  ), 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 1037, 96 ),
  INST(Fxsave          , "fxsave"          , Enc(X86M_Only)         , O(000F00,AE,0,_,_,_,_,_  ), 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 1047, 90 ),
  INST(Fxsave64        , "fxsave64"        , Enc(X86M_Only)         , O(000F00,AE,0,_,1,_,_,_  ), 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 1054, 96 ),
  INST(Fxtract         , "fxtract"         , Enc(FpuOp)             , O_FPU(00,D9F4,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 1063, 73 ),
  INST(Fyl2x           , "fyl2x"           , Enc(FpuOp)             , O_FPU(00,D9F1,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 1071, 73 ),
  INST(Fyl2xp1         , "fyl2xp1"         , Enc(FpuOp)             , O_FPU(00,D9F9,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 1077, 73 ),
  INST(Haddpd          , "haddpd"          , Enc(ExtRm)             , O(660F00,7C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 20 , 5308, 4  ),
  INST(Haddps          , "haddps"          , Enc(ExtRm)             , O(F20F00,7C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 20 , 5316, 4  ),
  INST(Hsubpd          , "hsubpd"          , Enc(ExtRm)             , O(660F00,7D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 20 , 5324, 4  ),
  INST(Hsubps          , "hsubps"          , Enc(ExtRm)             , O(F20F00,7D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 20 , 5332, 4  ),
  INST(Idiv            , "idiv"            , Enc(X86M_Bx_MulDiv)    , O(000000,F6,7,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(UUUUUU__), 0 , 0 , kFamilyNone, 0  , 675 , 97 ),
  INST(Imul            , "imul"            , Enc(X86Imul)           , O(000000,F6,5,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(WUUUUW__), 0 , 0 , kFamilyNone, 0  , 693 , 98 ),
  INST(Inc             , "inc"             , Enc(X86IncDec)         , O(000000,FE,0,_,x,_,_,_  ), O(000000,40,_,_,x,_,_,_  ), F(RW)|F(Lock)                         , EF(WWWWW___), 0 , 0 , kFamilyNone, 0  , 1085, 99 ),
  INST(Insertps        , "insertps"        , Enc(ExtRmi)            , O(660F3A,21,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 21 , 5468, 43 ),
  INST(Insertq         , "insertq"         , Enc(ExtInsertq)        , O(F20F00,79,_,_,_,_,_,_  ), O(F20F00,78,_,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1089, 100),
  INST(Int             , "int"             , Enc(X86Int)            , O(000000,CD,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(___W___W), 0 , 0 , kFamilyNone, 0  , 888 , 101),
  INST(Int3            , "int3"            , Enc(X86Op)             , O(000000,CC,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(___W___W), 0 , 0 , kFamilyNone, 0  , 1097, 102),
  INST(Into            , "into"            , Enc(X86Op)             , O(000000,CE,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(___W___W), 0 , 0 , kFamilyNone, 0  , 1102, 102),
  INST(Ja              , "ja"              , Enc(X86Jcc)            , O(000F00,87,_,_,_,_,_,_  ), O(000000,77,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(__R__R__), 0 , 0 , kFamilyNone, 0  , 1107, 103),
  INST(Jae             , "jae"             , Enc(X86Jcc)            , O(000F00,83,_,_,_,_,_,_  ), O(000000,73,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(_____R__), 0 , 0 , kFamilyNone, 0  , 1110, 104),
  INST(Jb              , "jb"              , Enc(X86Jcc)            , O(000F00,82,_,_,_,_,_,_  ), O(000000,72,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(_____R__), 0 , 0 , kFamilyNone, 0  , 1114, 105),
  INST(Jbe             , "jbe"             , Enc(X86Jcc)            , O(000F00,86,_,_,_,_,_,_  ), O(000000,76,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(__R__R__), 0 , 0 , kFamilyNone, 0  , 1117, 106),
  INST(Jc              , "jc"              , Enc(X86Jcc)            , O(000F00,82,_,_,_,_,_,_  ), O(000000,72,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(_____R__), 0 , 0 , kFamilyNone, 0  , 1121, 107),
  INST(Je              , "je"              , Enc(X86Jcc)            , O(000F00,84,_,_,_,_,_,_  ), O(000000,74,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(__R_____), 0 , 0 , kFamilyNone, 0  , 1124, 108),
  INST(Jg              , "jg"              , Enc(X86Jcc)            , O(000F00,8F,_,_,_,_,_,_  ), O(000000,7F,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(RRR_____), 0 , 0 , kFamilyNone, 0  , 1133, 109),
  INST(Jge             , "jge"             , Enc(X86Jcc)            , O(000F00,8D,_,_,_,_,_,_  ), O(000000,7D,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(RR______), 0 , 0 , kFamilyNone, 0  , 1136, 110),
  INST(Jl              , "jl"              , Enc(X86Jcc)            , O(000F00,8C,_,_,_,_,_,_  ), O(000000,7C,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(RR______), 0 , 0 , kFamilyNone, 0  , 1140, 111),
  INST(Jle             , "jle"             , Enc(X86Jcc)            , O(000F00,8E,_,_,_,_,_,_  ), O(000000,7E,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(RRR_____), 0 , 0 , kFamilyNone, 0  , 1143, 112),
  INST(Jna             , "jna"             , Enc(X86Jcc)            , O(000F00,86,_,_,_,_,_,_  ), O(000000,76,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(__R__R__), 0 , 0 , kFamilyNone, 0  , 1151, 106),
  INST(Jnae            , "jnae"            , Enc(X86Jcc)            , O(000F00,82,_,_,_,_,_,_  ), O(000000,72,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(_____R__), 0 , 0 , kFamilyNone, 0  , 1155, 105),
  INST(Jnb             , "jnb"             , Enc(X86Jcc)            , O(000F00,83,_,_,_,_,_,_  ), O(000000,73,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(_____R__), 0 , 0 , kFamilyNone, 0  , 1160, 104),
  INST(Jnbe            , "jnbe"            , Enc(X86Jcc)            , O(000F00,87,_,_,_,_,_,_  ), O(000000,77,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(__R__R__), 0 , 0 , kFamilyNone, 0  , 1164, 103),
  INST(Jnc             , "jnc"             , Enc(X86Jcc)            , O(000F00,83,_,_,_,_,_,_  ), O(000000,73,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(_____R__), 0 , 0 , kFamilyNone, 0  , 1169, 113),
  INST(Jne             , "jne"             , Enc(X86Jcc)            , O(000F00,85,_,_,_,_,_,_  ), O(000000,75,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(__R_____), 0 , 0 , kFamilyNone, 0  , 1173, 114),
  INST(Jng             , "jng"             , Enc(X86Jcc)            , O(000F00,8E,_,_,_,_,_,_  ), O(000000,7E,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(RRR_____), 0 , 0 , kFamilyNone, 0  , 1177, 112),
  INST(Jnge            , "jnge"            , Enc(X86Jcc)            , O(000F00,8C,_,_,_,_,_,_  ), O(000000,7C,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(RR______), 0 , 0 , kFamilyNone, 0  , 1181, 111),
  INST(Jnl             , "jnl"             , Enc(X86Jcc)            , O(000F00,8D,_,_,_,_,_,_  ), O(000000,7D,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(RR______), 0 , 0 , kFamilyNone, 0  , 1186, 110),
  INST(Jnle            , "jnle"            , Enc(X86Jcc)            , O(000F00,8F,_,_,_,_,_,_  ), O(000000,7F,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(RRR_____), 0 , 0 , kFamilyNone, 0  , 1190, 109),
  INST(Jno             , "jno"             , Enc(X86Jcc)            , O(000F00,81,_,_,_,_,_,_  ), O(000000,71,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(R_______), 0 , 0 , kFamilyNone, 0  , 1195, 115),
  INST(Jnp             , "jnp"             , Enc(X86Jcc)            , O(000F00,8B,_,_,_,_,_,_  ), O(000000,7B,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(____R___), 0 , 0 , kFamilyNone, 0  , 1199, 116),
  INST(Jns             , "jns"             , Enc(X86Jcc)            , O(000F00,89,_,_,_,_,_,_  ), O(000000,79,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(_R______), 0 , 0 , kFamilyNone, 0  , 1203, 117),
  INST(Jnz             , "jnz"             , Enc(X86Jcc)            , O(000F00,85,_,_,_,_,_,_  ), O(000000,75,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(__R_____), 0 , 0 , kFamilyNone, 0  , 1207, 114),
  INST(Jo              , "jo"              , Enc(X86Jcc)            , O(000F00,80,_,_,_,_,_,_  ), O(000000,70,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(R_______), 0 , 0 , kFamilyNone, 0  , 1211, 118),
  INST(Jp              , "jp"              , Enc(X86Jcc)            , O(000F00,8A,_,_,_,_,_,_  ), O(000000,7A,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(____R___), 0 , 0 , kFamilyNone, 0  , 1214, 119),
  INST(Jpe             , "jpe"             , Enc(X86Jcc)            , O(000F00,8A,_,_,_,_,_,_  ), O(000000,7A,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(____R___), 0 , 0 , kFamilyNone, 0  , 1217, 119),
  INST(Jpo             , "jpo"             , Enc(X86Jcc)            , O(000F00,8B,_,_,_,_,_,_  ), O(000000,7B,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(____R___), 0 , 0 , kFamilyNone, 0  , 1221, 116),
  INST(Js              , "js"              , Enc(X86Jcc)            , O(000F00,88,_,_,_,_,_,_  ), O(000000,78,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(_R______), 0 , 0 , kFamilyNone, 0  , 1225, 120),
  INST(Jz              , "jz"              , Enc(X86Jcc)            , O(000F00,84,_,_,_,_,_,_  ), O(000000,74,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(__R_____), 0 , 0 , kFamilyNone, 0  , 1228, 108),
  INST(Jecxz           , "jecxz"           , Enc(X86Jecxz)          , 0                         , O(000000,E3,_,_,_,_,_,_  ), F(Flow)|F(Volatile)|F(Special)        , EF(________), 0 , 0 , kFamilyNone, 0  , 1127, 121),
  INST(Jmp             , "jmp"             , Enc(X86Jmp)            , O(000000,FF,4,_,_,_,_,_  ), O(000000,EB,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(________), 0 , 0 , kFamilyNone, 0  , 1147, 122),
  INST(Kaddb           , "kaddb"           , Enc(VexRvm)            , V(660F00,4A,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1231, 123),
  INST(Kaddd           , "kaddd"           , Enc(VexRvm)            , V(660F00,4A,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1237, 123),
  INST(Kaddq           , "kaddq"           , Enc(VexRvm)            , V(000F00,4A,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1243, 123),
  INST(Kaddw           , "kaddw"           , Enc(VexRvm)            , V(000F00,4A,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1249, 123),
  INST(Kandb           , "kandb"           , Enc(VexRvm)            , V(660F00,41,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1255, 123),
  INST(Kandd           , "kandd"           , Enc(VexRvm)            , V(660F00,41,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1261, 123),
  INST(Kandnb          , "kandnb"          , Enc(VexRvm)            , V(660F00,42,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1267, 123),
  INST(Kandnd          , "kandnd"          , Enc(VexRvm)            , V(660F00,42,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1274, 123),
  INST(Kandnq          , "kandnq"          , Enc(VexRvm)            , V(000F00,42,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1281, 123),
  INST(Kandnw          , "kandnw"          , Enc(VexRvm)            , V(000F00,42,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1288, 123),
  INST(Kandq           , "kandq"           , Enc(VexRvm)            , V(000F00,41,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1295, 123),
  INST(Kandw           , "kandw"           , Enc(VexRvm)            , V(000F00,41,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1301, 123),
  INST(Kmovb           , "kmovb"           , Enc(VexKmov)           , V(660F00,90,_,0,0,_,_,_  ), V(660F00,92,_,0,0,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1307, 124),
  INST(Kmovd           , "kmovd"           , Enc(VexKmov)           , V(660F00,90,_,0,1,_,_,_  ), V(F20F00,92,_,0,0,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7249, 125),
  INST(Kmovq           , "kmovq"           , Enc(VexKmov)           , V(000F00,90,_,0,1,_,_,_  ), V(F20F00,92,_,0,1,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7260, 126),
  INST(Kmovw           , "kmovw"           , Enc(VexKmov)           , V(000F00,90,_,0,0,_,_,_  ), V(000F00,92,_,0,0,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1313, 127),
  INST(Knotb           , "knotb"           , Enc(VexRm)             , V(660F00,44,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1319, 128),
  INST(Knotd           , "knotd"           , Enc(VexRm)             , V(660F00,44,_,0,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1325, 128),
  INST(Knotq           , "knotq"           , Enc(VexRm)             , V(000F00,44,_,0,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1331, 128),
  INST(Knotw           , "knotw"           , Enc(VexRm)             , V(000F00,44,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1337, 128),
  INST(Korb            , "korb"            , Enc(VexRvm)            , V(660F00,45,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1343, 123),
  INST(Kord            , "kord"            , Enc(VexRvm)            , V(660F00,45,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1348, 123),
  INST(Korq            , "korq"            , Enc(VexRvm)            , V(000F00,45,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1353, 123),
  INST(Kortestb        , "kortestb"        , Enc(VexRm)             , V(660F00,98,_,0,0,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 1358, 129),
  INST(Kortestd        , "kortestd"        , Enc(VexRm)             , V(660F00,98,_,0,1,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 1367, 129),
  INST(Kortestq        , "kortestq"        , Enc(VexRm)             , V(000F00,98,_,0,1,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 1376, 129),
  INST(Kortestw        , "kortestw"        , Enc(VexRm)             , V(000F00,98,_,0,0,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 1385, 129),
  INST(Korw            , "korw"            , Enc(VexRvm)            , V(000F00,45,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1394, 123),
  INST(Kshiftlb        , "kshiftlb"        , Enc(VexRmi)            , V(660F3A,32,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1399, 130),
  INST(Kshiftld        , "kshiftld"        , Enc(VexRmi)            , V(660F3A,33,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1408, 130),
  INST(Kshiftlq        , "kshiftlq"        , Enc(VexRmi)            , V(660F3A,33,_,0,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1417, 130),
  INST(Kshiftlw        , "kshiftlw"        , Enc(VexRmi)            , V(660F3A,32,_,0,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1426, 130),
  INST(Kshiftrb        , "kshiftrb"        , Enc(VexRmi)            , V(660F3A,30,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1435, 130),
  INST(Kshiftrd        , "kshiftrd"        , Enc(VexRmi)            , V(660F3A,31,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1444, 130),
  INST(Kshiftrq        , "kshiftrq"        , Enc(VexRmi)            , V(660F3A,31,_,0,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1453, 130),
  INST(Kshiftrw        , "kshiftrw"        , Enc(VexRmi)            , V(660F3A,30,_,0,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1462, 130),
  INST(Ktestb          , "ktestb"          , Enc(VexRm)             , V(660F00,99,_,0,0,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 1471, 129),
  INST(Ktestd          , "ktestd"          , Enc(VexRm)             , V(660F00,99,_,0,1,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 1478, 129),
  INST(Ktestq          , "ktestq"          , Enc(VexRm)             , V(000F00,99,_,0,1,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 1485, 129),
  INST(Ktestw          , "ktestw"          , Enc(VexRm)             , V(000F00,99,_,0,0,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 1492, 129),
  INST(Kunpckbw        , "kunpckbw"        , Enc(VexRvm)            , V(660F00,4B,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1499, 123),
  INST(Kunpckdq        , "kunpckdq"        , Enc(VexRvm)            , V(000F00,4B,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1508, 123),
  INST(Kunpckwd        , "kunpckwd"        , Enc(VexRvm)            , V(000F00,4B,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1517, 123),
  INST(Kxnorb          , "kxnorb"          , Enc(VexRvm)            , V(660F00,46,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1526, 123),
  INST(Kxnord          , "kxnord"          , Enc(VexRvm)            , V(660F00,46,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1533, 123),
  INST(Kxnorq          , "kxnorq"          , Enc(VexRvm)            , V(000F00,46,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1540, 123),
  INST(Kxnorw          , "kxnorw"          , Enc(VexRvm)            , V(000F00,46,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1547, 123),
  INST(Kxorb           , "kxorb"           , Enc(VexRvm)            , V(660F00,47,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1554, 123),
  INST(Kxord           , "kxord"           , Enc(VexRvm)            , V(660F00,47,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1560, 123),
  INST(Kxorq           , "kxorq"           , Enc(VexRvm)            , V(000F00,47,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1566, 123),
  INST(Kxorw           , "kxorw"           , Enc(VexRvm)            , V(000F00,47,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 1572, 123),
  INST(Lahf            , "lahf"            , Enc(X86Op)             , O(000000,9F,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(_RRRRR__), 0 , 0 , kFamilyNone, 0  , 1578, 131),
  INST(Lddqu           , "lddqu"           , Enc(ExtRm)             , O(F20F00,F0,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 22 , 5478, 132),
  INST(Ldmxcsr         , "ldmxcsr"         , Enc(X86M_Only)         , O(000F00,AE,2,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , kFamilyNone, 0  , 5485, 133),
  INST(Lea             , "lea"             , Enc(X86Lea)            , O(000000,8D,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 1583, 134),
  INST(Leave           , "leave"           , Enc(X86Op)             , O(000000,C9,_,_,_,_,_,_  ), 0                         , F(Volatile)|F(Special)                , EF(________), 0 , 0 , kFamilyNone, 0  , 1587, 135),
  INST(Lfence          , "lfence"          , Enc(X86Fence)          , O(000F00,AE,5,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(________), 0 , 0 , kFamilyNone, 0  , 1593, 69 ),
  INST(LodsB           , "lods_b"          , Enc(X86Op)             , O(000000,AC,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(______R_), 0 , 1 , kFamilyNone, 0  , 2062, 136),
  INST(LodsD           , "lods_d"          , Enc(X86Op)             , O(000000,AD,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(______R_), 0 , 4 , kFamilyNone, 0  , 2073, 137),
  INST(LodsQ           , "lods_q"          , Enc(X86Op)             , O(000000,AD,_,_,1,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(______R_), 0 , 8 , kFamilyNone, 0  , 2084, 138),
  INST(LodsW           , "lods_w"          , Enc(X86Op)             , O(660000,AD,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(______R_), 0 , 2 , kFamilyNone, 0  , 2095, 139),
  INST(Lzcnt           , "lzcnt"           , Enc(X86Rm)             , O(F30F00,BD,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(UUWUUW__), 0 , 0 , kFamilyNone, 0  , 1600, 140),
  INST(Maskmovdqu      , "maskmovdqu"      , Enc(ExtRmZDI)          , O(660F00,57,_,_,_,_,_,_  ), 0                         , F(RO)|F(Special)                      , EF(________), 0 , 0 , kFamilySse , 23 , 5494, 141),
  INST(Maskmovq        , "maskmovq"        , Enc(ExtRmZDI)          , O(000F00,F7,_,_,_,_,_,_  ), 0                         , F(RO)|F(Special)                      , EF(________), 0 , 0 , kFamilySse , 8  , 7257, 142),
  INST(Maxpd           , "maxpd"           , Enc(ExtRm)             , O(660F00,5F,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 24 , 5528, 4  ),
  INST(Maxps           , "maxps"           , Enc(ExtRm)             , O(000F00,5F,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 24 , 5535, 4  ),
  INST(Maxsd           , "maxsd"           , Enc(ExtRm)             , O(F20F00,5F,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 24 , 7276, 5  ),
  INST(Maxss           , "maxss"           , Enc(ExtRm)             , O(F30F00,5F,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 24 , 5549, 6  ),
  INST(Mfence          , "mfence"          , Enc(X86Fence)          , O(000F00,AE,6,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)                     , EF(________), 0 , 0 , kFamilyNone, 0  , 1606, 143),
  INST(Minpd           , "minpd"           , Enc(ExtRm)             , O(660F00,5D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 25 , 5556, 4  ),
  INST(Minps           , "minps"           , Enc(ExtRm)             , O(000F00,5D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 25 , 5563, 4  ),
  INST(Minsd           , "minsd"           , Enc(ExtRm)             , O(F20F00,5D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 25 , 7340, 5  ),
  INST(Minss           , "minss"           , Enc(ExtRm)             , O(F30F00,5D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 25 , 5577, 6  ),
  INST(Monitor         , "monitor"         , Enc(X86Op)             , O(000F01,C8,_,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 1613, 144),
  INST(Mov             , "mov"             , Enc(X86Mov)            , 0                         , 0                         , F(WO)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 6223, 145),
  INST(Movapd          , "movapd"          , Enc(ExtMov)            , O(660F00,28,_,_,_,_,_,_  ), O(660F00,29,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, kFamilySse , 26 , 5584, 146),
  INST(Movaps          , "movaps"          , Enc(ExtMov)            , O(000F00,28,_,_,_,_,_,_  ), O(000F00,29,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, kFamilySse , 26 , 5592, 147),
  INST(Movbe           , "movbe"           , Enc(ExtMovbe)          , O(000F38,F0,_,_,x,_,_,_  ), O(000F38,F1,_,_,x,_,_,_  ), F(WO)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 522 , 148),
  INST(Movd            , "movd"            , Enc(ExtMovd)           , O(000F00,6E,_,_,_,_,_,_  ), O(000F00,7E,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, kFamilySse , 27 , 7250, 149),
  INST(Movddup         , "movddup"         , Enc(ExtMov)            , O(F20F00,12,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 27 , 5606, 52 ),
  INST(Movdq2q         , "movdq2q"         , Enc(ExtMov)            , O(F20F00,D6,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 8  , 1621, 150),
  INST(Movdqa          , "movdqa"          , Enc(ExtMov)            , O(660F00,6F,_,_,_,_,_,_  ), O(660F00,7F,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, kFamilySse , 28 , 5615, 151),
  INST(Movdqu          , "movdqu"          , Enc(ExtMov)            , O(F30F00,6F,_,_,_,_,_,_  ), O(F30F00,7F,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, kFamilySse , 26 , 5498, 152),
  INST(Movhlps         , "movhlps"         , Enc(ExtMov)            , O(000F00,12,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 29 , 5690, 153),
  INST(Movhpd          , "movhpd"          , Enc(ExtMov)            , O(660F00,16,_,_,_,_,_,_  ), O(660F00,17,_,_,_,_,_,_  ), F(RW)                                 , EF(________), 8 , 8 , kFamilySse , 30 , 5699, 154),
  INST(Movhps          , "movhps"          , Enc(ExtMov)            , O(000F00,16,_,_,_,_,_,_  ), O(000F00,17,_,_,_,_,_,_  ), F(RW)                                 , EF(________), 8 , 8 , kFamilySse , 30 , 5707, 155),
  INST(Movlhps         , "movlhps"         , Enc(ExtMov)            , O(000F00,16,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 8 , 8 , kFamilySse , 29 , 5715, 156),
  INST(Movlpd          , "movlpd"          , Enc(ExtMov)            , O(660F00,12,_,_,_,_,_,_  ), O(660F00,13,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 30 , 5724, 157),
  INST(Movlps          , "movlps"          , Enc(ExtMov)            , O(000F00,12,_,_,_,_,_,_  ), O(000F00,13,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 30 , 5732, 158),
  INST(Movmskpd        , "movmskpd"        , Enc(ExtMov)            , O(660F00,50,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 31 , 5740, 159),
  INST(Movmskps        , "movmskps"        , Enc(ExtMov)            , O(000F00,50,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 31 , 5750, 159),
  INST(Movntdq         , "movntdq"         , Enc(ExtMov)            , 0                         , O(660F00,E7,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, kFamilySse , 31 , 5760, 160),
  INST(Movntdqa        , "movntdqa"        , Enc(ExtMov)            , O(660F38,2A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 31 , 5769, 132),
  INST(Movnti          , "movnti"          , Enc(ExtMovnti)         , O(000F00,C3,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilyNone, 0  , 1629, 161),
  INST(Movntpd         , "movntpd"         , Enc(ExtMov)            , 0                         , O(660F00,2B,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, kFamilySse , 32 , 5779, 162),
  INST(Movntps         , "movntps"         , Enc(ExtMov)            , 0                         , O(000F00,2B,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, kFamilySse , 32 , 5788, 163),
  INST(Movntq          , "movntq"          , Enc(ExtMov)            , 0                         , O(000F00,E7,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 8  , 1636, 164),
  INST(Movntsd         , "movntsd"         , Enc(ExtMov)            , 0                         , O(F20F00,2B,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 8  , 1643, 165),
  INST(Movntss         , "movntss"         , Enc(ExtMov)            , 0                         , O(F30F00,2B,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 4 , kFamilySse , 8  , 1651, 166),
  INST(Movq            , "movq"            , Enc(ExtMovq)           , O(000F00,6E,_,_,x,_,_,_  ), O(000F00,7E,_,_,x,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, kFamilySse , 26 , 7261, 167),
  INST(Movq2dq         , "movq2dq"         , Enc(ExtRm)             , O(F30F00,D6,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 8  , 1659, 168),
  INST(MovsB           , "movs_b"          , Enc(X86Op)             , O(000000,A4,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , kFamilyNone, 0  , 2106, 169),
  INST(MovsD           , "movs_d"          , Enc(X86Op)             , O(000000,A5,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , kFamilyNone, 0  , 2117, 169),
  INST(MovsQ           , "movs_q"          , Enc(X86Op)             , O(000000,A5,_,_,1,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , kFamilyNone, 0  , 2128, 169),
  INST(MovsW           , "movs_w"          , Enc(X86Op)             , O(660000,A5,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , kFamilyNone, 0  , 2139, 169),
  INST(Movsd           , "movsd"           , Enc(ExtMov)            , O(F20F00,10,_,_,_,_,_,_  ), O(F20F00,11,_,_,_,_,_,_  ), F(WO)|F(ZeroIfMem)                    , EF(________), 0 , 8 , kFamilyNone, 0  , 5803, 170),
  INST(Movshdup        , "movshdup"        , Enc(ExtRm)             , O(F30F00,16,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 33 , 5810, 53 ),
  INST(Movsldup        , "movsldup"        , Enc(ExtRm)             , O(F30F00,12,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 33 , 5820, 53 ),
  INST(Movss           , "movss"           , Enc(ExtMov)            , O(F30F00,10,_,_,_,_,_,_  ), O(F30F00,11,_,_,_,_,_,_  ), F(WO)|F(ZeroIfMem)                    , EF(________), 0 , 4 , kFamilySse , 34 , 5830, 171),
  INST(Movsx           , "movsx"           , Enc(X86MovsxMovzx)     , O(000F00,BE,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 1667, 172),
  INST(Movsxd          , "movsxd"          , Enc(X86Rm)             , O(000000,63,_,_,1,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 1673, 173),
  INST(Movupd          , "movupd"          , Enc(ExtMov)            , O(660F00,10,_,_,_,_,_,_  ), O(660F00,11,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, kFamilySse , 35 , 5837, 174),
  INST(Movups          , "movups"          , Enc(ExtMov)            , O(000F00,10,_,_,_,_,_,_  ), O(000F00,11,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, kFamilySse , 35 , 5845, 175),
  INST(Movzx           , "movzx"           , Enc(X86MovsxMovzx)     , O(000F00,B6,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 1680, 172),
  INST(Mpsadbw         , "mpsadbw"         , Enc(ExtRmi)            , O(660F3A,42,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 36 , 5853, 13 ),
  INST(Mul             , "mul"             , Enc(X86M_Bx_MulDiv)    , O(000000,F6,4,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(WUUUUW__), 0 , 0 , kFamilyNone, 0  , 694 , 176),
  INST(Mulpd           , "mulpd"           , Enc(ExtRm)             , O(660F00,59,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 37 , 5862, 4  ),
  INST(Mulps           , "mulps"           , Enc(ExtRm)             , O(000F00,59,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 37 , 5869, 4  ),
  INST(Mulsd           , "mulsd"           , Enc(ExtRm)             , O(F20F00,59,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 37 , 5876, 5  ),
  INST(Mulss           , "mulss"           , Enc(ExtRm)             , O(F30F00,59,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 37 , 5883, 6  ),
  INST(Mulx            , "mulx"            , Enc(VexRvmZDX_Wx)      , V(F20F38,F6,_,0,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , kFamilyNone, 0  , 1686, 177),
  INST(Mwait           , "mwait"           , Enc(X86Op)             , O(000F01,C9,_,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 1691, 144),
  INST(Neg             , "neg"             , Enc(X86M_Bx)           , O(000000,F6,3,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 1697, 178),
  INST(Nop             , "nop"             , Enc(X86Op)             , O(000000,90,_,_,_,_,_,_  ), 0                         , 0                                     , EF(________), 0 , 0 , kFamilyNone, 0  , 825 , 179),
  INST(Not             , "not"             , Enc(X86M_Bx)           , O(000000,F6,2,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(________), 0 , 0 , kFamilyNone, 0  , 1701, 180),
  INST(Or              , "or"              , Enc(X86Arith)          , O(000000,08,1,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWUWW__), 0 , 0 , kFamilyNone, 0  , 1034, 3  ),
  INST(Orpd            , "orpd"            , Enc(ExtRm)             , O(660F00,56,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 38 , 9316, 4  ),
  INST(Orps            , "orps"            , Enc(ExtRm)             , O(000F00,56,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 38 , 9323, 4  ),
  INST(Pabsb           , "pabsb"           , Enc(ExtRm_P)           , O(000F38,1C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 39 , 5902, 181),
  INST(Pabsd           , "pabsd"           , Enc(ExtRm_P)           , O(000F38,1E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 39 , 5909, 181),
  INST(Pabsw           , "pabsw"           , Enc(ExtRm_P)           , O(000F38,1D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 40 , 5923, 181),
  INST(Packssdw        , "packssdw"        , Enc(ExtRm_P)           , O(000F00,6B,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 41 , 5930, 181),
  INST(Packsswb        , "packsswb"        , Enc(ExtRm_P)           , O(000F00,63,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 41 , 5940, 181),
  INST(Packusdw        , "packusdw"        , Enc(ExtRm)             , O(660F38,2B,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 41 , 5950, 4  ),
  INST(Packuswb        , "packuswb"        , Enc(ExtRm_P)           , O(000F00,67,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 41 , 5960, 181),
  INST(Paddb           , "paddb"           , Enc(ExtRm_P)           , O(000F00,FC,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 41 , 5970, 181),
  INST(Paddd           , "paddd"           , Enc(ExtRm_P)           , O(000F00,FE,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 41 , 5977, 181),
  INST(Paddq           , "paddq"           , Enc(ExtRm_P)           , O(000F00,D4,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 41 , 5984, 181),
  INST(Paddsb          , "paddsb"          , Enc(ExtRm_P)           , O(000F00,EC,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 41 , 5991, 181),
  INST(Paddsw          , "paddsw"          , Enc(ExtRm_P)           , O(000F00,ED,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 41 , 5999, 181),
  INST(Paddusb         , "paddusb"         , Enc(ExtRm_P)           , O(000F00,DC,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 41 , 6007, 181),
  INST(Paddusw         , "paddusw"         , Enc(ExtRm_P)           , O(000F00,DD,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 41 , 6016, 181),
  INST(Paddw           , "paddw"           , Enc(ExtRm_P)           , O(000F00,FD,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 41 , 6025, 181),
  INST(Palignr         , "palignr"         , Enc(ExtRmi_P)          , O(000F3A,0F,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 41 , 6032, 182),
  INST(Pand            , "pand"            , Enc(ExtRm_P)           , O(000F00,DB,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 41 , 6041, 181),
  INST(Pandn           , "pandn"           , Enc(ExtRm_P)           , O(000F00,DF,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 42 , 6054, 181),
  INST(Pause           , "pause"           , Enc(X86Op)             , O(F30000,90,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 1705, 183),
  INST(Pavgb           , "pavgb"           , Enc(ExtRm_P)           , O(000F00,E0,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 43 , 6084, 181),
  INST(Pavgusb         , "pavgusb"         , Enc(Ext3dNow)          , O(000F0F,BF,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1711, 184),
  INST(Pavgw           , "pavgw"           , Enc(ExtRm_P)           , O(000F00,E3,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 44 , 6091, 181),
  INST(Pblendvb        , "pblendvb"        , Enc(ExtRmXMM0)         , O(660F38,10,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , kFamilySse , 45 , 6107, 14 ),
  INST(Pblendw         , "pblendw"         , Enc(ExtRmi)            , O(660F3A,0E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 43 , 6117, 13 ),
  INST(Pclmulqdq       , "pclmulqdq"       , Enc(ExtRmi)            , O(660F3A,44,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 46 , 6210, 13 ),
  INST(Pcmpeqb         , "pcmpeqb"         , Enc(ExtRm_P)           , O(000F00,74,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 47 , 6242, 181),
  INST(Pcmpeqd         , "pcmpeqd"         , Enc(ExtRm_P)           , O(000F00,76,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 47 , 6251, 181),
  INST(Pcmpeqq         , "pcmpeqq"         , Enc(ExtRm)             , O(660F38,29,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 47 , 6260, 4  ),
  INST(Pcmpeqw         , "pcmpeqw"         , Enc(ExtRm_P)           , O(000F00,75,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 47 , 6269, 181),
  INST(Pcmpestri       , "pcmpestri"       , Enc(ExtRmi)            , O(660F3A,61,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , kFamilySse , 28 , 6278, 185),
  INST(Pcmpestrm       , "pcmpestrm"       , Enc(ExtRmi)            , O(660F3A,60,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , kFamilySse , 28 , 6289, 186),
  INST(Pcmpgtb         , "pcmpgtb"         , Enc(ExtRm_P)           , O(000F00,64,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 47 , 6300, 181),
  INST(Pcmpgtd         , "pcmpgtd"         , Enc(ExtRm_P)           , O(000F00,66,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 47 , 6309, 181),
  INST(Pcmpgtq         , "pcmpgtq"         , Enc(ExtRm)             , O(660F38,37,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 47 , 6318, 4  ),
  INST(Pcmpgtw         , "pcmpgtw"         , Enc(ExtRm_P)           , O(000F00,65,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 47 , 6327, 181),
  INST(Pcmpistri       , "pcmpistri"       , Enc(ExtRmi)            , O(660F3A,63,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , kFamilySse , 28 , 6336, 187),
  INST(Pcmpistrm       , "pcmpistrm"       , Enc(ExtRmi)            , O(660F3A,62,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , kFamilySse , 28 , 6347, 188),
  INST(Pcommit         , "pcommit"         , Enc(X86Op_O)           , O(660F00,AE,7,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(________), 0 , 0 , kFamilyNone, 0  , 1719, 69 ),
  INST(Pdep            , "pdep"            , Enc(VexRvm_Wx)         , V(F20F38,F5,_,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 1727, 189),
  INST(Pext            , "pext"            , Enc(VexRvm_Wx)         , V(F30F38,F5,_,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 1732, 189),
  INST(Pextrb          , "pextrb"          , Enc(ExtExtract)        , O(000F3A,14,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 48 , 6752, 190),
  INST(Pextrd          , "pextrd"          , Enc(ExtExtract)        , O(000F3A,16,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 48 , 6760, 71 ),
  INST(Pextrq          , "pextrq"          , Enc(ExtExtract)        , O(000F3A,16,_,_,1,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 48 , 6768, 191),
  INST(Pextrw          , "pextrw"          , Enc(ExtPextrw)         , O(000F00,C5,_,_,_,_,_,_  ), O(000F3A,15,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 48 , 6776, 192),
  INST(Pf2id           , "pf2id"           , Enc(Ext3dNow)          , O(000F0F,1D,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 8  , 1737, 193),
  INST(Pf2iw           , "pf2iw"           , Enc(Ext3dNow)          , O(000F0F,1C,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 8  , 1743, 193),
  INST(Pfacc           , "pfacc"           , Enc(Ext3dNow)          , O(000F0F,AE,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1749, 184),
  INST(Pfadd           , "pfadd"           , Enc(Ext3dNow)          , O(000F0F,9E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1755, 184),
  INST(Pfcmpeq         , "pfcmpeq"         , Enc(Ext3dNow)          , O(000F0F,B0,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1761, 184),
  INST(Pfcmpge         , "pfcmpge"         , Enc(Ext3dNow)          , O(000F0F,90,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1769, 184),
  INST(Pfcmpgt         , "pfcmpgt"         , Enc(Ext3dNow)          , O(000F0F,A0,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1777, 184),
  INST(Pfmax           , "pfmax"           , Enc(Ext3dNow)          , O(000F0F,A4,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1785, 184),
  INST(Pfmin           , "pfmin"           , Enc(Ext3dNow)          , O(000F0F,94,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1791, 184),
  INST(Pfmul           , "pfmul"           , Enc(Ext3dNow)          , O(000F0F,B4,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1797, 184),
  INST(Pfnacc          , "pfnacc"          , Enc(Ext3dNow)          , O(000F0F,8A,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1803, 184),
  INST(Pfpnacc         , "pfpnacc"         , Enc(Ext3dNow)          , O(000F0F,8E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1810, 184),
  INST(Pfrcp           , "pfrcp"           , Enc(Ext3dNow)          , O(000F0F,96,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 8  , 1818, 193),
  INST(Pfrcpit1        , "pfrcpit1"        , Enc(Ext3dNow)          , O(000F0F,A6,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1824, 184),
  INST(Pfrcpit2        , "pfrcpit2"        , Enc(Ext3dNow)          , O(000F0F,B6,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1833, 184),
  INST(Pfrcpv          , "pfrcpv"          , Enc(Ext3dNow)          , O(000F0F,86,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1842, 184),
  INST(Pfrsqit1        , "pfrsqit1"        , Enc(Ext3dNow)          , O(000F0F,A7,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1849, 194),
  INST(Pfrsqrt         , "pfrsqrt"         , Enc(Ext3dNow)          , O(000F0F,97,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1858, 194),
  INST(Pfrsqrtv        , "pfrsqrtv"        , Enc(Ext3dNow)          , O(000F0F,87,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1866, 184),
  INST(Pfsub           , "pfsub"           , Enc(Ext3dNow)          , O(000F0F,9A,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1875, 184),
  INST(Pfsubr          , "pfsubr"          , Enc(Ext3dNow)          , O(000F0F,AA,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1881, 184),
  INST(Phaddd          , "phaddd"          , Enc(ExtRm_P)           , O(000F38,02,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 49 , 6855, 181),
  INST(Phaddsw         , "phaddsw"         , Enc(ExtRm_P)           , O(000F38,03,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 50 , 6872, 181),
  INST(Phaddw          , "phaddw"          , Enc(ExtRm_P)           , O(000F38,01,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 51 , 6941, 181),
  INST(Phminposuw      , "phminposuw"      , Enc(ExtRm)             , O(660F38,41,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 52 , 6967, 4  ),
  INST(Phsubd          , "phsubd"          , Enc(ExtRm_P)           , O(000F38,06,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 53 , 6988, 181),
  INST(Phsubsw         , "phsubsw"         , Enc(ExtRm_P)           , O(000F38,07,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 54 , 7005, 181),
  INST(Phsubw          , "phsubw"          , Enc(ExtRm_P)           , O(000F38,05,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 54 , 7014, 181),
  INST(Pi2fd           , "pi2fd"           , Enc(Ext3dNow)          , O(000F0F,0D,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 8  , 1888, 193),
  INST(Pi2fw           , "pi2fw"           , Enc(Ext3dNow)          , O(000F0F,0C,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 8  , 1894, 193),
  INST(Pinsrb          , "pinsrb"          , Enc(ExtRmi)            , O(660F3A,20,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 55 , 7031, 195),
  INST(Pinsrd          , "pinsrd"          , Enc(ExtRmi)            , O(660F3A,22,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 55 , 7039, 196),
  INST(Pinsrq          , "pinsrq"          , Enc(ExtRmi)            , O(660F3A,22,_,_,1,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 55 , 7047, 197),
  INST(Pinsrw          , "pinsrw"          , Enc(ExtRmi_P)          , O(000F00,C4,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 53 , 7055, 198),
  INST(Pmaddubsw       , "pmaddubsw"       , Enc(ExtRm_P)           , O(000F38,04,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 56 , 7225, 181),
  INST(Pmaddwd         , "pmaddwd"         , Enc(ExtRm_P)           , O(000F00,F5,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 56 , 7236, 181),
  INST(Pmaxsb          , "pmaxsb"          , Enc(ExtRm)             , O(660F38,3C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 57 , 7267, 4  ),
  INST(Pmaxsd          , "pmaxsd"          , Enc(ExtRm)             , O(660F38,3D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 57 , 7275, 4  ),
  INST(Pmaxsw          , "pmaxsw"          , Enc(ExtRm_P)           , O(000F00,EE,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 58 , 7291, 181),
  INST(Pmaxub          , "pmaxub"          , Enc(ExtRm_P)           , O(000F00,DE,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 58 , 7299, 181),
  INST(Pmaxud          , "pmaxud"          , Enc(ExtRm)             , O(660F38,3F,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 58 , 7307, 4  ),
  INST(Pmaxuw          , "pmaxuw"          , Enc(ExtRm)             , O(660F38,3E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 5  , 7323, 4  ),
  INST(Pminsb          , "pminsb"          , Enc(ExtRm)             , O(660F38,38,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 5  , 7331, 4  ),
  INST(Pminsd          , "pminsd"          , Enc(ExtRm)             , O(660F38,39,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 5  , 7339, 4  ),
  INST(Pminsw          , "pminsw"          , Enc(ExtRm_P)           , O(000F00,EA,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 59 , 7355, 181),
  INST(Pminub          , "pminub"          , Enc(ExtRm_P)           , O(000F00,DA,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 59 , 7363, 181),
  INST(Pminud          , "pminud"          , Enc(ExtRm)             , O(660F38,3B,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 59 , 7371, 4  ),
  INST(Pminuw          , "pminuw"          , Enc(ExtRm)             , O(660F38,3A,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 11 , 7387, 4  ),
  INST(Pmovmskb        , "pmovmskb"        , Enc(ExtRm_P)           , O(000F00,D7,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 60 , 7465, 199),
  INST(Pmovsxbd        , "pmovsxbd"        , Enc(ExtRm)             , O(660F38,21,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 61 , 7562, 200),
  INST(Pmovsxbq        , "pmovsxbq"        , Enc(ExtRm)             , O(660F38,22,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 61 , 7572, 201),
  INST(Pmovsxbw        , "pmovsxbw"        , Enc(ExtRm)             , O(660F38,20,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 61 , 7582, 52 ),
  INST(Pmovsxdq        , "pmovsxdq"        , Enc(ExtRm)             , O(660F38,25,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 61 , 7592, 52 ),
  INST(Pmovsxwd        , "pmovsxwd"        , Enc(ExtRm)             , O(660F38,23,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 61 , 7602, 52 ),
  INST(Pmovsxwq        , "pmovsxwq"        , Enc(ExtRm)             , O(660F38,24,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 61 , 7612, 200),
  INST(Pmovzxbd        , "pmovzxbd"        , Enc(ExtRm)             , O(660F38,31,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 62 , 7699, 200),
  INST(Pmovzxbq        , "pmovzxbq"        , Enc(ExtRm)             , O(660F38,32,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 62 , 7709, 201),
  INST(Pmovzxbw        , "pmovzxbw"        , Enc(ExtRm)             , O(660F38,30,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 62 , 7719, 52 ),
  INST(Pmovzxdq        , "pmovzxdq"        , Enc(ExtRm)             , O(660F38,35,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 62 , 7729, 52 ),
  INST(Pmovzxwd        , "pmovzxwd"        , Enc(ExtRm)             , O(660F38,33,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 62 , 7739, 52 ),
  INST(Pmovzxwq        , "pmovzxwq"        , Enc(ExtRm)             , O(660F38,34,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 62 , 7749, 200),
  INST(Pmuldq          , "pmuldq"          , Enc(ExtRm)             , O(660F38,28,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 63 , 7759, 4  ),
  INST(Pmulhrsw        , "pmulhrsw"        , Enc(ExtRm_P)           , O(000F38,0B,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 63 , 7767, 181),
  INST(Pmulhrw         , "pmulhrw"         , Enc(Ext3dNow)          , O(000F0F,B7,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 1900, 184),
  INST(Pmulhuw         , "pmulhuw"         , Enc(ExtRm_P)           , O(000F00,E4,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 64 , 7777, 181),
  INST(Pmulhw          , "pmulhw"          , Enc(ExtRm_P)           , O(000F00,E5,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 64 , 7786, 181),
  INST(Pmulld          , "pmulld"          , Enc(ExtRm)             , O(660F38,40,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 64 , 7794, 4  ),
  INST(Pmullw          , "pmullw"          , Enc(ExtRm_P)           , O(000F00,D5,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 63 , 7810, 181),
  INST(Pmuludq         , "pmuludq"         , Enc(ExtRm_P)           , O(000F00,F4,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 65 , 7833, 181),
  INST(Pop             , "pop"             , Enc(X86Pop)            , O(000000,8F,0,_,_,_,_,_  ), O(000000,58,_,_,_,_,_,_  ), F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 1908, 202),
  INST(Popa            , "popa"            , Enc(X86Op)             , O(000000,61,_,_,_,_,_,_  ), 0                         , F(Volatile)|F(Special)                , EF(________), 0 , 0 , kFamilyNone, 0  , 1912, 203),
  INST(Popcnt          , "popcnt"          , Enc(X86Rm)             , O(F30F00,B8,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 1917, 204),
  INST(Popf            , "popf"            , Enc(X86Op)             , O(000000,9D,_,_,_,_,_,_  ), 0                         , F(Volatile)|F(Special)                , EF(WWWWWWWW), 0 , 0 , kFamilyNone, 0  , 1924, 205),
  INST(Por             , "por"             , Enc(ExtRm_P)           , O(000F00,EB,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 66 , 7842, 181),
  INST(Prefetch        , "prefetch"        , Enc(X86Prefetch)       , O(000F00,18,_,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , kFamilyNone, 0  , 1929, 206),
  INST(Prefetch3dNow   , "prefetch3dnow"   , Enc(X86M_Only)         , O(000F00,0D,0,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , kFamilyNone, 0  , 1938, 29 ),
  INST(Prefetchw       , "prefetchw"       , Enc(X86M_Only)         , O(000F00,0D,1,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(UUUUUU__), 0 , 0 , kFamilyNone, 0  , 1952, 207),
  INST(Prefetchwt1     , "prefetchwt1"     , Enc(X86M_Only)         , O(000F00,0D,2,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(UUUUUU__), 0 , 0 , kFamilyNone, 0  , 1962, 207),
  INST(Psadbw          , "psadbw"          , Enc(ExtRm_P)           , O(000F00,F6,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 20 , 3731, 181),
  INST(Pshufb          , "pshufb"          , Enc(ExtRm_P)           , O(000F38,00,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 67 , 8066, 208),
  INST(Pshufd          , "pshufd"          , Enc(ExtRmi)            , O(660F00,70,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 68 , 8074, 209),
  INST(Pshufhw         , "pshufhw"         , Enc(ExtRmi)            , O(F30F00,70,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 68 , 8082, 209),
  INST(Pshuflw         , "pshuflw"         , Enc(ExtRmi)            , O(F20F00,70,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 68 , 8091, 209),
  INST(Pshufw          , "pshufw"          , Enc(ExtRmi_P)          , O(000F00,70,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 8  , 1974, 210),
  INST(Psignb          , "psignb"          , Enc(ExtRm_P)           , O(000F38,08,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 0  , 8100, 181),
  INST(Psignd          , "psignd"          , Enc(ExtRm_P)           , O(000F38,0A,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 0  , 8108, 181),
  INST(Psignw          , "psignw"          , Enc(ExtRm_P)           , O(000F38,09,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 0  , 8116, 181),
  INST(Pslld           , "pslld"           , Enc(ExtRmRi_P)         , O(000F00,F2,_,_,_,_,_,_  ), O(000F00,72,6,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 0  , 8124, 211),
  INST(Pslldq          , "pslldq"          , Enc(ExtRmRi)           , 0                         , O(660F00,73,7,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 0  , 8131, 212),
  INST(Psllq           , "psllq"           , Enc(ExtRmRi_P)         , O(000F00,F3,_,_,_,_,_,_  ), O(000F00,73,6,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 0  , 8139, 213),
  INST(Psllw           , "psllw"           , Enc(ExtRmRi_P)         , O(000F00,F1,_,_,_,_,_,_  ), O(000F00,71,6,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 69 , 8170, 214),
  INST(Psrad           , "psrad"           , Enc(ExtRmRi_P)         , O(000F00,E2,_,_,_,_,_,_  ), O(000F00,72,4,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 69 , 8177, 215),
  INST(Psraw           , "psraw"           , Enc(ExtRmRi_P)         , O(000F00,E1,_,_,_,_,_,_  ), O(000F00,71,4,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 70 , 8215, 216),
  INST(Psrld           , "psrld"           , Enc(ExtRmRi_P)         , O(000F00,D2,_,_,_,_,_,_  ), O(000F00,72,2,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 70 , 8222, 217),
  INST(Psrldq          , "psrldq"          , Enc(ExtRmRi)           , 0                         , O(660F00,73,3,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 70 , 8229, 218),
  INST(Psrlq           , "psrlq"           , Enc(ExtRmRi_P)         , O(000F00,D3,_,_,_,_,_,_  ), O(000F00,73,2,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 70 , 8237, 219),
  INST(Psrlw           , "psrlw"           , Enc(ExtRmRi_P)         , O(000F00,D1,_,_,_,_,_,_  ), O(000F00,71,2,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 71 , 8268, 220),
  INST(Psubb           , "psubb"           , Enc(ExtRm_P)           , O(000F00,F8,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 71 , 8275, 181),
  INST(Psubd           , "psubd"           , Enc(ExtRm_P)           , O(000F00,FA,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 71 , 8282, 181),
  INST(Psubq           , "psubq"           , Enc(ExtRm_P)           , O(000F00,FB,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 71 , 8289, 181),
  INST(Psubsb          , "psubsb"          , Enc(ExtRm_P)           , O(000F00,E8,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 71 , 8296, 181),
  INST(Psubsw          , "psubsw"          , Enc(ExtRm_P)           , O(000F00,E9,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 71 , 8304, 181),
  INST(Psubusb         , "psubusb"         , Enc(ExtRm_P)           , O(000F00,D8,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 71 , 8312, 181),
  INST(Psubusw         , "psubusw"         , Enc(ExtRm_P)           , O(000F00,D9,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 71 , 8321, 181),
  INST(Psubw           , "psubw"           , Enc(ExtRm_P)           , O(000F00,F9,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 71 , 8330, 181),
  INST(Pswapd          , "pswapd"          , Enc(Ext3dNow)          , O(000F0F,BB,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 8  , 1981, 193),
  INST(Ptest           , "ptest"           , Enc(ExtRm)             , O(660F38,17,_,_,_,_,_,_  ), 0                         , F(RO)                                 , EF(WWWWWW__), 0 , 0 , kFamilySse , 72 , 8359, 221),
  INST(Punpckhbw       , "punpckhbw"       , Enc(ExtRm_P)           , O(000F00,68,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 73 , 8442, 181),
  INST(Punpckhdq       , "punpckhdq"       , Enc(ExtRm_P)           , O(000F00,6A,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 73 , 8453, 181),
  INST(Punpckhqdq      , "punpckhqdq"      , Enc(ExtRm)             , O(660F00,6D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 73 , 8464, 4  ),
  INST(Punpckhwd       , "punpckhwd"       , Enc(ExtRm_P)           , O(000F00,69,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 73 , 8476, 181),
  INST(Punpcklbw       , "punpcklbw"       , Enc(ExtRm_P)           , O(000F00,60,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 73 , 8487, 181),
  INST(Punpckldq       , "punpckldq"       , Enc(ExtRm_P)           , O(000F00,62,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 73 , 8498, 181),
  INST(Punpcklqdq      , "punpcklqdq"      , Enc(ExtRm)             , O(660F00,6C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 73 , 8509, 4  ),
  INST(Punpcklwd       , "punpcklwd"       , Enc(ExtRm_P)           , O(000F00,61,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 73 , 8521, 181),
  INST(Push            , "push"            , Enc(X86Push)           , O(000000,FF,6,_,_,_,_,_  ), O(000000,50,_,_,_,_,_,_  ), F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 1988, 222),
  INST(Pusha           , "pusha"           , Enc(X86Op)             , O(000000,60,_,_,_,_,_,_  ), 0                         , F(Volatile)|F(Special)                , EF(________), 0 , 0 , kFamilyNone, 0  , 1993, 203),
  INST(Pushf           , "pushf"           , Enc(X86Op)             , O(000000,9C,_,_,_,_,_,_  ), 0                         , F(Volatile)|F(Special)                , EF(RRRRRRRR), 0 , 0 , kFamilyNone, 0  , 1999, 223),
  INST(Pxor            , "pxor"            , Enc(ExtRm_P)           , O(000F00,EF,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 74 , 8532, 181),
  INST(Rcl             , "rcl"             , Enc(X86Rot)            , O(000000,D0,2,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(W____X__), 0 , 0 , kFamilyNone, 0  , 2005, 224),
  INST(Rcpps           , "rcpps"           , Enc(ExtRm)             , O(000F00,53,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 75 , 8660, 53 ),
  INST(Rcpss           , "rcpss"           , Enc(ExtRm)             , O(F30F00,53,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 4 , kFamilySse , 76 , 8667, 225),
  INST(Rcr             , "rcr"             , Enc(X86Rot)            , O(000000,D0,3,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(W____X__), 0 , 0 , kFamilyNone, 0  , 2009, 224),
  INST(Rdfsbase        , "rdfsbase"        , Enc(X86M)              , O(F30F00,AE,0,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilyNone, 0  , 2013, 226),
  INST(Rdgsbase        , "rdgsbase"        , Enc(X86M)              , O(F30F00,AE,1,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilyNone, 0  , 2022, 226),
  INST(Rdrand          , "rdrand"          , Enc(X86M)              , O(000F00,C7,6,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWWWW__), 0 , 8 , kFamilyNone, 0  , 2031, 227),
  INST(Rdseed          , "rdseed"          , Enc(X86M)              , O(000F00,C7,7,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWWWW__), 0 , 8 , kFamilyNone, 0  , 2038, 227),
  INST(Rdtsc           , "rdtsc"           , Enc(X86Op)             , O(000F00,31,_,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 2045, 228),
  INST(Rdtscp          , "rdtscp"          , Enc(X86Op)             , O(000F01,F9,_,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 2051, 229),
  INST(RepLodsB        , "rep lods_b"      , Enc(X86Rep)            , O(000000,AC,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , kFamilyNone, 0  , 2058, 230),
  INST(RepLodsD        , "rep lods_d"      , Enc(X86Rep)            , O(000000,AD,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , kFamilyNone, 0  , 2069, 230),
  INST(RepLodsQ        , "rep lods_q"      , Enc(X86Rep)            , O(000000,AD,1,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , kFamilyNone, 0  , 2080, 230),
  INST(RepLodsW        , "rep lods_w"      , Enc(X86Rep)            , O(660000,AD,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , kFamilyNone, 0  , 2091, 230),
  INST(RepMovsB        , "rep movs_b"      , Enc(X86Rep)            , O(000000,A4,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , kFamilyNone, 0  , 2102, 230),
  INST(RepMovsD        , "rep movs_d"      , Enc(X86Rep)            , O(000000,A5,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , kFamilyNone, 0  , 2113, 230),
  INST(RepMovsQ        , "rep movs_q"      , Enc(X86Rep)            , O(000000,A5,1,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , kFamilyNone, 0  , 2124, 230),
  INST(RepMovsW        , "rep movs_w"      , Enc(X86Rep)            , O(660000,A5,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , kFamilyNone, 0  , 2135, 230),
  INST(RepStosB        , "rep stos_b"      , Enc(X86Rep)            , O(000000,AA,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , kFamilyNone, 0  , 2146, 230),
  INST(RepStosD        , "rep stos_d"      , Enc(X86Rep)            , O(000000,AB,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , kFamilyNone, 0  , 2157, 230),
  INST(RepStosQ        , "rep stos_q"      , Enc(X86Rep)            , O(000000,AB,1,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , kFamilyNone, 0  , 2168, 230),
  INST(RepStosW        , "rep stos_w"      , Enc(X86Rep)            , O(660000,AB,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , kFamilyNone, 0  , 2179, 230),
  INST(RepeCmpsB       , "repe cmps_b"     , Enc(X86Rep)            , O(000000,A6,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2190, 41 ),
  INST(RepeCmpsD       , "repe cmps_d"     , Enc(X86Rep)            , O(000000,A7,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2202, 41 ),
  INST(RepeCmpsQ       , "repe cmps_q"     , Enc(X86Rep)            , O(000000,A7,1,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2214, 41 ),
  INST(RepeCmpsW       , "repe cmps_w"     , Enc(X86Rep)            , O(660000,A7,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2226, 41 ),
  INST(RepeScasB       , "repe scas_b"     , Enc(X86Rep)            , O(000000,AE,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2238, 41 ),
  INST(RepeScasD       , "repe scas_d"     , Enc(X86Rep)            , O(000000,AF,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2250, 41 ),
  INST(RepeScasQ       , "repe scas_q"     , Enc(X86Rep)            , O(000000,AF,1,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2262, 41 ),
  INST(RepeScasW       , "repe scas_w"     , Enc(X86Rep)            , O(660000,AF,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2274, 41 ),
  INST(RepneCmpsB      , "repne cmps_b"    , Enc(X86Rep)            , O(000000,A6,0,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2286, 41 ),
  INST(RepneCmpsD      , "repne cmps_d"    , Enc(X86Rep)            , O(000000,A7,0,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2299, 41 ),
  INST(RepneCmpsQ      , "repne cmps_q"    , Enc(X86Rep)            , O(000000,A7,0,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2312, 41 ),
  INST(RepneCmpsW      , "repne cmps_w"    , Enc(X86Rep)            , O(660000,A7,0,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2325, 41 ),
  INST(RepneScasB      , "repne scas_b"    , Enc(X86Rep)            , O(000000,AE,0,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2338, 41 ),
  INST(RepneScasD      , "repne scas_d"    , Enc(X86Rep)            , O(000000,AF,0,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2351, 41 ),
  INST(RepneScasQ      , "repne scas_q"    , Enc(X86Rep)            , O(000000,AF,0,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2364, 41 ),
  INST(RepneScasW      , "repne scas_w"    , Enc(X86Rep)            , O(660000,AF,0,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2377, 41 ),
  INST(Ret             , "ret"             , Enc(X86Ret)            , O(000000,C2,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 2390, 231),
  INST(Rol             , "rol"             , Enc(X86Rot)            , O(000000,D0,0,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(W____W__), 0 , 0 , kFamilyNone, 0  , 2394, 232),
  INST(Ror             , "ror"             , Enc(X86Rot)            , O(000000,D0,1,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(W____W__), 0 , 0 , kFamilyNone, 0  , 2398, 232),
  INST(Rorx            , "rorx"            , Enc(VexRmi_Wx)         , V(F20F3A,F0,_,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 2402, 233),
  INST(Roundpd         , "roundpd"         , Enc(ExtRmi)            , O(660F3A,09,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 77 , 8762, 209),
  INST(Roundps         , "roundps"         , Enc(ExtRmi)            , O(660F3A,08,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 77 , 8771, 209),
  INST(Roundsd         , "roundsd"         , Enc(ExtRmi)            , O(660F3A,0B,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 78 , 8780, 234),
  INST(Roundss         , "roundss"         , Enc(ExtRmi)            , O(660F3A,0A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 4 , kFamilySse , 78 , 8789, 235),
  INST(Rsqrtps         , "rsqrtps"         , Enc(ExtRm)             , O(000F00,52,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 79 , 8886, 53 ),
  INST(Rsqrtss         , "rsqrtss"         , Enc(ExtRm)             , O(F30F00,52,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 4 , kFamilySse , 80 , 8895, 225),
  INST(Sahf            , "sahf"            , Enc(X86Op)             , O(000000,9E,_,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(_WWWWW__), 0 , 0 , kFamilyNone, 0  , 2407, 236),
  INST(Sal             , "sal"             , Enc(X86Rot)            , O(000000,D0,4,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(WWWUWW__), 0 , 0 , kFamilyNone, 0  , 2412, 237),
  INST(Sar             , "sar"             , Enc(X86Rot)            , O(000000,D0,7,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(WWWUWW__), 0 , 0 , kFamilyNone, 0  , 2416, 237),
  INST(Sarx            , "sarx"            , Enc(VexRmv_Wx)         , V(F30F38,F7,_,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 2420, 238),
  INST(Sbb             , "sbb"             , Enc(X86Arith)          , O(000000,18,3,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWWWX__), 0 , 0 , kFamilyNone, 0  , 2425, 1  ),
  INST(ScasB           , "scas_b"          , Enc(X86Op)             , O(000000,AE,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2344, 41 ),
  INST(ScasD           , "scas_d"          , Enc(X86Op)             , O(000000,AF,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2357, 41 ),
  INST(ScasQ           , "scas_q"          , Enc(X86Op)             , O(000000,AF,_,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2370, 41 ),
  INST(ScasW           , "scas_w"          , Enc(X86Op)             , O(660000,AF,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , kFamilyNone, 0  , 2383, 41 ),
  INST(Seta            , "seta"            , Enc(X86Set)            , O(000F00,97,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R__R__), 0 , 1 , kFamilyNone, 0  , 2429, 239),
  INST(Setae           , "setae"           , Enc(X86Set)            , O(000F00,93,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_____R__), 0 , 1 , kFamilyNone, 0  , 2434, 240),
  INST(Setb            , "setb"            , Enc(X86Set)            , O(000F00,92,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_____R__), 0 , 1 , kFamilyNone, 0  , 2440, 240),
  INST(Setbe           , "setbe"           , Enc(X86Set)            , O(000F00,96,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R__R__), 0 , 1 , kFamilyNone, 0  , 2445, 239),
  INST(Setc            , "setc"            , Enc(X86Set)            , O(000F00,92,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_____R__), 0 , 1 , kFamilyNone, 0  , 2451, 240),
  INST(Sete            , "sete"            , Enc(X86Set)            , O(000F00,94,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R_____), 0 , 1 , kFamilyNone, 0  , 2456, 241),
  INST(Setg            , "setg"            , Enc(X86Set)            , O(000F00,9F,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RRR_____), 0 , 1 , kFamilyNone, 0  , 2461, 242),
  INST(Setge           , "setge"           , Enc(X86Set)            , O(000F00,9D,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RR______), 0 , 1 , kFamilyNone, 0  , 2466, 243),
  INST(Setl            , "setl"            , Enc(X86Set)            , O(000F00,9C,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RR______), 0 , 1 , kFamilyNone, 0  , 2472, 243),
  INST(Setle           , "setle"           , Enc(X86Set)            , O(000F00,9E,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RRR_____), 0 , 1 , kFamilyNone, 0  , 2477, 242),
  INST(Setna           , "setna"           , Enc(X86Set)            , O(000F00,96,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R__R__), 0 , 1 , kFamilyNone, 0  , 2483, 239),
  INST(Setnae          , "setnae"          , Enc(X86Set)            , O(000F00,92,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_____R__), 0 , 1 , kFamilyNone, 0  , 2489, 240),
  INST(Setnb           , "setnb"           , Enc(X86Set)            , O(000F00,93,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_____R__), 0 , 1 , kFamilyNone, 0  , 2496, 240),
  INST(Setnbe          , "setnbe"          , Enc(X86Set)            , O(000F00,97,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R__R__), 0 , 1 , kFamilyNone, 0  , 2502, 239),
  INST(Setnc           , "setnc"           , Enc(X86Set)            , O(000F00,93,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_____R__), 0 , 1 , kFamilyNone, 0  , 2509, 240),
  INST(Setne           , "setne"           , Enc(X86Set)            , O(000F00,95,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R_____), 0 , 1 , kFamilyNone, 0  , 2515, 241),
  INST(Setng           , "setng"           , Enc(X86Set)            , O(000F00,9E,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RRR_____), 0 , 1 , kFamilyNone, 0  , 2521, 242),
  INST(Setnge          , "setnge"          , Enc(X86Set)            , O(000F00,9C,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RR______), 0 , 1 , kFamilyNone, 0  , 2527, 243),
  INST(Setnl           , "setnl"           , Enc(X86Set)            , O(000F00,9D,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RR______), 0 , 1 , kFamilyNone, 0  , 2534, 243),
  INST(Setnle          , "setnle"          , Enc(X86Set)            , O(000F00,9F,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RRR_____), 0 , 1 , kFamilyNone, 0  , 2540, 242),
  INST(Setno           , "setno"           , Enc(X86Set)            , O(000F00,91,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(R_______), 0 , 1 , kFamilyNone, 0  , 2547, 244),
  INST(Setnp           , "setnp"           , Enc(X86Set)            , O(000F00,9B,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(____R___), 0 , 1 , kFamilyNone, 0  , 2553, 245),
  INST(Setns           , "setns"           , Enc(X86Set)            , O(000F00,99,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_R______), 0 , 1 , kFamilyNone, 0  , 2559, 246),
  INST(Setnz           , "setnz"           , Enc(X86Set)            , O(000F00,95,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R_____), 0 , 1 , kFamilyNone, 0  , 2565, 241),
  INST(Seto            , "seto"            , Enc(X86Set)            , O(000F00,90,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(R_______), 0 , 1 , kFamilyNone, 0  , 2571, 244),
  INST(Setp            , "setp"            , Enc(X86Set)            , O(000F00,9A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(____R___), 0 , 1 , kFamilyNone, 0  , 2576, 245),
  INST(Setpe           , "setpe"           , Enc(X86Set)            , O(000F00,9A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(____R___), 0 , 1 , kFamilyNone, 0  , 2581, 245),
  INST(Setpo           , "setpo"           , Enc(X86Set)            , O(000F00,9B,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(____R___), 0 , 1 , kFamilyNone, 0  , 2587, 245),
  INST(Sets            , "sets"            , Enc(X86Set)            , O(000F00,98,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_R______), 0 , 1 , kFamilyNone, 0  , 2593, 246),
  INST(Setz            , "setz"            , Enc(X86Set)            , O(000F00,94,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R_____), 0 , 1 , kFamilyNone, 0  , 2598, 241),
  INST(Sfence          , "sfence"          , Enc(X86Fence)          , O(000F00,AE,7,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(________), 0 , 0 , kFamilyNone, 0  , 2603, 69 ),
  INST(Sha1msg1        , "sha1msg1"        , Enc(ExtRm)             , O(000F38,C9,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 2610, 4  ),
  INST(Sha1msg2        , "sha1msg2"        , Enc(ExtRm)             , O(000F38,CA,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 2619, 4  ),
  INST(Sha1nexte       , "sha1nexte"       , Enc(ExtRm)             , O(000F38,C8,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 2628, 4  ),
  INST(Sha1rnds4       , "sha1rnds4"       , Enc(ExtRmi)            , O(000F3A,CC,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 2638, 13 ),
  INST(Sha256msg1      , "sha256msg1"      , Enc(ExtRm)             , O(000F38,CC,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 2648, 4  ),
  INST(Sha256msg2      , "sha256msg2"      , Enc(ExtRm)             , O(000F38,CD,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 8  , 2659, 4  ),
  INST(Sha256rnds2     , "sha256rnds2"     , Enc(ExtRmXMM0)         , O(000F38,CB,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , kFamilySse , 8  , 2670, 14 ),
  INST(Shl             , "shl"             , Enc(X86Rot)            , O(000000,D0,4,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(WWWUWW__), 0 , 0 , kFamilyNone, 0  , 2682, 237),
  INST(Shld            , "shld"            , Enc(X86ShldShrd)       , O(000F00,A4,_,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(UWWUWW__), 0 , 0 , kFamilyNone, 0  , 8046, 247),
  INST(Shlx            , "shlx"            , Enc(VexRmv_Wx)         , V(660F38,F7,_,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 2686, 238),
  INST(Shr             , "shr"             , Enc(X86Rot)            , O(000000,D0,5,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(WWWUWW__), 0 , 0 , kFamilyNone, 0  , 2691, 237),
  INST(Shrd            , "shrd"            , Enc(X86ShldShrd)       , O(000F00,AC,_,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(UWWUWW__), 0 , 0 , kFamilyNone, 0  , 2695, 247),
  INST(Shrx            , "shrx"            , Enc(VexRmv_Wx)         , V(F20F38,F7,_,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , kFamilyNone, 0  , 2700, 238),
  INST(Shufpd          , "shufpd"          , Enc(ExtRmi)            , O(660F00,C6,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 81 , 9156, 13 ),
  INST(Shufps          , "shufps"          , Enc(ExtRmi)            , O(000F00,C6,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 81 , 9164, 13 ),
  INST(Sqrtpd          , "sqrtpd"          , Enc(ExtRm)             , O(660F00,51,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 61 , 9172, 53 ),
  INST(Sqrtps          , "sqrtps"          , Enc(ExtRm)             , O(000F00,51,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, kFamilySse , 61 , 8887, 53 ),
  INST(Sqrtsd          , "sqrtsd"          , Enc(ExtRm)             , O(F20F00,51,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , kFamilySse , 81 , 9188, 248),
  INST(Sqrtss          , "sqrtss"          , Enc(ExtRm)             , O(F30F00,51,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 4 , kFamilySse , 81 , 8896, 225),
  INST(Stac            , "stac"            , Enc(X86Op)             , O(000F01,CB,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(___W____), 0 , 0 , kFamilyNone, 0  , 2705, 26 ),
  INST(Stc             , "stc"             , Enc(X86Op)             , O(000000,F9,_,_,_,_,_,_  ), 0                         , 0                                     , EF(_____W__), 0 , 0 , kFamilyNone, 0  , 2710, 249),
  INST(Std             , "std"             , Enc(X86Op)             , O(000000,FD,_,_,_,_,_,_  ), 0                         , 0                                     , EF(______W_), 0 , 0 , kFamilyNone, 0  , 6147, 250),
  INST(Sti             , "sti"             , Enc(X86Op)             , O(000000,FB,_,_,_,_,_,_  ), 0                         , 0                                     , EF(_______W), 0 , 0 , kFamilyNone, 0  , 2714, 251),
  INST(Stmxcsr         , "stmxcsr"         , Enc(X86M_Only)         , O(000F00,AE,3,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(________), 0 , 0 , kFamilyNone, 0  , 9204, 252),
  INST(StosB           , "stos_b"          , Enc(X86Op)             , O(000000,AA,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , kFamilyNone, 0  , 2150, 230),
  INST(StosD           , "stos_d"          , Enc(X86Op)             , O(000000,AB,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , kFamilyNone, 0  , 2161, 230),
  INST(StosQ           , "stos_q"          , Enc(X86Op)             , O(000000,AB,_,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , kFamilyNone, 0  , 2172, 230),
  INST(StosW           , "stos_w"          , Enc(X86Op)             , O(660000,AB,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , kFamilyNone, 0  , 2183, 230),
  INST(Sub             , "sub"             , Enc(X86Arith)          , O(000000,28,5,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 732 , 3  ),
  INST(Subpd           , "subpd"           , Enc(ExtRm)             , O(660F00,5C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 18 , 4287, 4  ),
  INST(Subps           , "subps"           , Enc(ExtRm)             , O(000F00,5C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 18 , 4299, 4  ),
  INST(Subsd           , "subsd"           , Enc(ExtRm)             , O(F20F00,5C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 18 , 4975, 5  ),
  INST(Subss           , "subss"           , Enc(ExtRm)             , O(F30F00,5C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 18 , 4985, 6  ),
  INST(T1mskc          , "t1mskc"          , Enc(VexVm_Wx)          , V(XOP_M9,01,7,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , kFamilyNone, 0  , 2718, 12 ),
  INST(Test            , "test"            , Enc(X86Test)           , O(000000,84,_,_,x,_,_,_  ), O(000000,F6,_,_,x,_,_,_  ), F(RO)                                 , EF(WWWUWW__), 0 , 0 , kFamilyNone, 0  , 8360, 253),
  INST(Tzcnt           , "tzcnt"           , Enc(X86Rm)             , O(F30F00,BC,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(UUWUUW__), 0 , 0 , kFamilyNone, 0  , 2725, 204),
  INST(Tzmsk           , "tzmsk"           , Enc(VexVm_Wx)          , V(XOP_M9,01,4,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , kFamilyNone, 0  , 2731, 12 ),
  INST(Ucomisd         , "ucomisd"         , Enc(ExtRm)             , O(660F00,2E,_,_,_,_,_,_  ), 0                         , F(RO)                                 , EF(WWWWWW__), 0 , 0 , kFamilySse , 17 , 9257, 47 ),
  INST(Ucomiss         , "ucomiss"         , Enc(ExtRm)             , O(000F00,2E,_,_,_,_,_,_  ), 0                         , F(RO)                                 , EF(WWWWWW__), 0 , 0 , kFamilySse , 17 , 9266, 48 ),
  INST(Ud2             , "ud2"             , Enc(X86Op)             , O(000F00,0B,_,_,_,_,_,_  ), 0                         , 0                                     , EF(________), 0 , 0 , kFamilyNone, 0  , 2737, 254),
  INST(Unpckhpd        , "unpckhpd"        , Enc(ExtRm)             , O(660F00,15,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 82 , 9275, 4  ),
  INST(Unpckhps        , "unpckhps"        , Enc(ExtRm)             , O(000F00,15,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 82 , 9285, 4  ),
  INST(Unpcklpd        , "unpcklpd"        , Enc(ExtRm)             , O(660F00,14,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 82 , 9295, 4  ),
  INST(Unpcklps        , "unpcklps"        , Enc(ExtRm)             , O(000F00,14,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 82 , 9305, 4  ),
  INST(Vaddpd          , "vaddpd"          , Enc(VexRvm_Lx)         , V(660F00,58,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 2741, 255),
  INST(Vaddps          , "vaddps"          , Enc(VexRvm_Lx)         , V(000F00,58,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 2748, 256),
  INST(Vaddsd          , "vaddsd"          , Enc(VexRvm)            , V(F20F00,58,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 2755, 257),
  INST(Vaddss          , "vaddss"          , Enc(VexRvm)            , V(F30F00,58,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 2762, 258),
  INST(Vaddsubpd       , "vaddsubpd"       , Enc(VexRvm_Lx)         , V(660F00,D0,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 2769, 259),
  INST(Vaddsubps       , "vaddsubps"       , Enc(VexRvm_Lx)         , V(F20F00,D0,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 2779, 259),
  INST(Vaesdec         , "vaesdec"         , Enc(VexRvm)            , V(660F38,DE,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 2789, 260),
  INST(Vaesdeclast     , "vaesdeclast"     , Enc(VexRvm)            , V(660F38,DF,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 2797, 260),
  INST(Vaesenc         , "vaesenc"         , Enc(VexRvm)            , V(660F38,DC,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 2809, 260),
  INST(Vaesenclast     , "vaesenclast"     , Enc(VexRvm)            , V(660F38,DD,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 2817, 260),
  INST(Vaesimc         , "vaesimc"         , Enc(VexRm)             , V(660F38,DB,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 2829, 261),
  INST(Vaeskeygenassist, "vaeskeygenassist", Enc(VexRmi)            , V(660F3A,DF,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 2837, 262),
  INST(Valignd         , "valignd"         , Enc(VexRvmi_Lx)        , V(660F3A,03,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 2854, 263),
  INST(Valignq         , "valignq"         , Enc(VexRvmi_Lx)        , V(660F3A,03,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 2862, 264),
  INST(Vandnpd         , "vandnpd"         , Enc(VexRvm_Lx)         , V(660F00,55,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 2870, 265),
  INST(Vandnps         , "vandnps"         , Enc(VexRvm_Lx)         , V(000F00,55,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 2878, 266),
  INST(Vandpd          , "vandpd"          , Enc(VexRvm_Lx)         , V(660F00,54,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 2886, 265),
  INST(Vandps          , "vandps"          , Enc(VexRvm_Lx)         , V(000F00,54,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 2893, 266),
  INST(Vblendmb        , "vblendmb"        , Enc(VexRvm_Lx)         , V(660F38,66,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 2900, 267),
  INST(Vblendmd        , "vblendmd"        , Enc(VexRvm_Lx)         , V(660F38,64,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 2909, 268),
  INST(Vblendmpd       , "vblendmpd"       , Enc(VexRvm_Lx)         , V(660F38,65,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 2918, 269),
  INST(Vblendmps       , "vblendmps"       , Enc(VexRvm_Lx)         , V(660F38,65,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 2928, 268),
  INST(Vblendmq        , "vblendmq"        , Enc(VexRvm_Lx)         , V(660F38,64,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 2938, 269),
  INST(Vblendmw        , "vblendmw"        , Enc(VexRvm_Lx)         , V(660F38,66,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 2947, 267),
  INST(Vblendpd        , "vblendpd"        , Enc(VexRvmi_Lx)        , V(660F3A,0D,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 2956, 270),
  INST(Vblendps        , "vblendps"        , Enc(VexRvmi_Lx)        , V(660F3A,0C,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 2965, 270),
  INST(Vblendvpd       , "vblendvpd"       , Enc(VexRvmr_Lx)        , V(660F3A,4B,_,x,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 2974, 271),
  INST(Vblendvps       , "vblendvps"       , Enc(VexRvmr_Lx)        , V(660F3A,4A,_,x,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 2984, 271),
  INST(Vbroadcastf128  , "vbroadcastf128"  , Enc(VexRm)             , V(660F38,1A,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 2994, 272),
  INST(Vbroadcastf32x2 , "vbroadcastf32x2" , Enc(VexRm_Lx)          , V(660F38,19,_,x,_,0,3,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3009, 273),
  INST(Vbroadcastf32x4 , "vbroadcastf32x4" , Enc(VexRm_Lx)          , V(660F38,1A,_,x,_,0,4,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3025, 274),
  INST(Vbroadcastf32x8 , "vbroadcastf32x8" , Enc(VexRm)             , V(660F38,1B,_,2,_,0,5,T8 ), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3041, 275),
  INST(Vbroadcastf64x2 , "vbroadcastf64x2" , Enc(VexRm_Lx)          , V(660F38,1A,_,x,_,1,4,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3057, 276),
  INST(Vbroadcastf64x4 , "vbroadcastf64x4" , Enc(VexRm)             , V(660F38,1B,_,2,_,1,5,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3073, 277),
  INST(Vbroadcasti128  , "vbroadcasti128"  , Enc(VexRm)             , V(660F38,5A,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 3089, 272),
  INST(Vbroadcasti32x2 , "vbroadcasti32x2" , Enc(VexRm_Lx)          , V(660F38,59,_,x,_,0,3,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3104, 278),
  INST(Vbroadcasti32x4 , "vbroadcasti32x4" , Enc(VexRm_Lx)          , V(660F38,5A,_,x,_,0,4,T4 ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3120, 279),
  INST(Vbroadcasti32x8 , "vbroadcasti32x8" , Enc(VexRm)             , V(660F38,5B,_,2,_,0,5,T8 ), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3136, 280),
  INST(Vbroadcasti64x2 , "vbroadcasti64x2" , Enc(VexRm_Lx)          , V(660F38,5A,_,x,_,1,4,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3152, 273),
  INST(Vbroadcasti64x4 , "vbroadcasti64x4" , Enc(VexRm)             , V(660F38,5B,_,2,_,1,5,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3168, 281),
  INST(Vbroadcastsd    , "vbroadcastsd"    , Enc(VexRm_Lx)          , V(660F38,19,_,x,0,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3184, 282),
  INST(Vbroadcastss    , "vbroadcastss"    , Enc(VexRm_Lx)          , V(660F38,18,_,x,0,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3197, 283),
  INST(Vcmppd          , "vcmppd"          , Enc(VexRvmi_Lx)        , V(660F00,C2,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 3210, 284),
  INST(Vcmpps          , "vcmpps"          , Enc(VexRvmi_Lx)        , V(000F00,C2,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 3217, 285),
  INST(Vcmpsd          , "vcmpsd"          , Enc(VexRvmi)           , V(F20F00,C2,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3224, 286),
  INST(Vcmpss          , "vcmpss"          , Enc(VexRvmi)           , V(F30F00,C2,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3231, 287),
  INST(Vcomisd         , "vcomisd"         , Enc(VexRm)             , V(660F00,2F,_,I,I,1,3,T1S), 0                         , F(RO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 3238, 288),
  INST(Vcomiss         , "vcomiss"         , Enc(VexRm)             , V(000F00,2F,_,I,I,0,2,T1S), 0                         , F(RO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 3246, 289),
  INST(Vcompresspd     , "vcompresspd"     , Enc(VexMr_Lx)          , V(660F38,8A,_,x,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3254, 290),
  INST(Vcompressps     , "vcompressps"     , Enc(VexMr_Lx)          , V(660F38,8A,_,x,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3266, 290),
  INST(Vcvtdq2pd       , "vcvtdq2pd"       , Enc(VexRm_Lx)          , V(F30F00,E6,_,x,I,0,3,HV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 3278, 291),
  INST(Vcvtdq2ps       , "vcvtdq2ps"       , Enc(VexRm_Lx)          , V(000F00,5B,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 3288, 292),
  INST(Vcvtpd2dq       , "vcvtpd2dq"       , Enc(VexRm_Lx)          , V(F20F00,E6,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 3298, 293),
  INST(Vcvtpd2ps       , "vcvtpd2ps"       , Enc(VexRm_Lx)          , V(660F00,5A,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 3308, 294),
  INST(Vcvtpd2qq       , "vcvtpd2qq"       , Enc(VexRm_Lx)          , V(660F00,7B,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 3318, 295),
  INST(Vcvtpd2udq      , "vcvtpd2udq"      , Enc(VexRm_Lx)          , V(000F00,79,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 3328, 296),
  INST(Vcvtpd2uqq      , "vcvtpd2uqq"      , Enc(VexRm_Lx)          , V(660F00,79,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 3339, 295),
  INST(Vcvtph2ps       , "vcvtph2ps"       , Enc(VexRm_Lx)          , V(660F38,13,_,x,0,0,3,HVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3350, 297),
  INST(Vcvtps2dq       , "vcvtps2dq"       , Enc(VexRm_Lx)          , V(660F00,5B,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 3360, 292),
  INST(Vcvtps2pd       , "vcvtps2pd"       , Enc(VexRm_Lx)          , V(000F00,5A,_,x,I,0,4,HV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 3370, 298),
  INST(Vcvtps2ph       , "vcvtps2ph"       , Enc(VexMri_Lx)         , V(660F3A,1D,_,x,0,0,3,HVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3380, 299),
  INST(Vcvtps2qq       , "vcvtps2qq"       , Enc(VexRm_Lx)          , V(660F00,7B,_,x,_,0,3,HV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 3390, 300),
  INST(Vcvtps2udq      , "vcvtps2udq"      , Enc(VexRm_Lx)          , V(000F00,79,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 3400, 301),
  INST(Vcvtps2uqq      , "vcvtps2uqq"      , Enc(VexRm_Lx)          , V(660F00,79,_,x,_,0,3,HV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 3411, 300),
  INST(Vcvtqq2pd       , "vcvtqq2pd"       , Enc(VexRm_Lx)          , V(F30F00,E6,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 3422, 295),
  INST(Vcvtqq2ps       , "vcvtqq2ps"       , Enc(VexRm_Lx)          , V(000F00,5B,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 3432, 302),
  INST(Vcvtsd2si       , "vcvtsd2si"       , Enc(VexRm)             , V(F20F00,2D,_,I,x,x,3,T1F), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3442, 303),
  INST(Vcvtsd2ss       , "vcvtsd2ss"       , Enc(VexRvm)            , V(F20F00,5A,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3452, 257),
  INST(Vcvtsd2usi      , "vcvtsd2usi"      , Enc(VexRm)             , V(F20F00,79,_,I,_,x,3,T1F), 0                         , F(WO)          |A512(F_  ,0,0 ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3462, 304),
  INST(Vcvtsi2sd       , "vcvtsi2sd"       , Enc(VexRvm)            , V(F20F00,2A,_,I,x,x,2,T1W), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3473, 305),
  INST(Vcvtsi2ss       , "vcvtsi2ss"       , Enc(VexRvm)            , V(F30F00,2A,_,I,x,x,2,T1W), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3483, 305),
  INST(Vcvtss2sd       , "vcvtss2sd"       , Enc(VexRvm)            , V(F30F00,5A,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3493, 306),
  INST(Vcvtss2si       , "vcvtss2si"       , Enc(VexRm)             , V(F20F00,2D,_,I,x,x,2,T1F), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3503, 307),
  INST(Vcvtss2usi      , "vcvtss2usi"      , Enc(VexRm)             , V(F30F00,79,_,I,_,x,2,T1F), 0                         , F(WO)          |A512(F_  ,0,0 ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3513, 308),
  INST(Vcvttpd2dq      , "vcvttpd2dq"      , Enc(VexRm_Lx)          , V(660F00,E6,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 3524, 309),
  INST(Vcvttpd2qq      , "vcvttpd2qq"      , Enc(VexRm_Lx)          , V(660F00,7A,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 3535, 310),
  INST(Vcvttpd2udq     , "vcvttpd2udq"     , Enc(VexRm_Lx)          , V(000F00,78,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 3546, 311),
  INST(Vcvttpd2uqq     , "vcvttpd2uqq"     , Enc(VexRm_Lx)          , V(660F00,78,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,SAE,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 3558, 312),
  INST(Vcvttps2dq      , "vcvttps2dq"      , Enc(VexRm_Lx)          , V(F30F00,5B,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 3570, 313),
  INST(Vcvttps2qq      , "vcvttps2qq"      , Enc(VexRm_Lx)          , V(660F00,7A,_,x,_,0,3,HV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,SAE,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 3581, 314),
  INST(Vcvttps2udq     , "vcvttps2udq"     , Enc(VexRm_Lx)          , V(000F00,78,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 3592, 315),
  INST(Vcvttps2uqq     , "vcvttps2uqq"     , Enc(VexRm_Lx)          , V(660F00,78,_,x,_,0,3,HV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,SAE,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 3604, 314),
  INST(Vcvttsd2si      , "vcvttsd2si"      , Enc(VexRm)             , V(F20F00,2C,_,I,x,x,3,T1F), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3616, 316),
  INST(Vcvttsd2usi     , "vcvttsd2usi"     , Enc(VexRm)             , V(F20F00,78,_,I,_,x,3,T1F), 0                         , F(WO)          |A512(F_  ,0,0 ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3627, 317),
  INST(Vcvttss2si      , "vcvttss2si"      , Enc(VexRm)             , V(F30F00,2C,_,I,x,x,2,T1F), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3639, 318),
  INST(Vcvttss2usi     , "vcvttss2usi"     , Enc(VexRm)             , V(F30F00,78,_,I,_,x,2,T1F), 0                         , F(WO)          |A512(F_  ,0,0 ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3650, 319),
  INST(Vcvtudq2pd      , "vcvtudq2pd"      , Enc(VexRm_Lx)          , V(F30F00,7A,_,x,_,0,3,HV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 3662, 320),
  INST(Vcvtudq2ps      , "vcvtudq2ps"      , Enc(VexRm_Lx)          , V(F20F00,7A,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 3673, 301),
  INST(Vcvtuqq2pd      , "vcvtuqq2pd"      , Enc(VexRm_Lx)          , V(F30F00,7A,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 3684, 295),
  INST(Vcvtuqq2ps      , "vcvtuqq2ps"      , Enc(VexRm_Lx)          , V(F20F00,7A,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 3695, 302),
  INST(Vcvtusi2sd      , "vcvtusi2sd"      , Enc(VexRvm)            , V(F20F00,7B,_,I,_,x,2,T1W), 0                         , F(WO)          |A512(F_  ,0,0 ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3706, 321),
  INST(Vcvtusi2ss      , "vcvtusi2ss"      , Enc(VexRvm)            , V(F30F00,7B,_,I,_,x,2,T1W), 0                         , F(WO)          |A512(F_  ,0,0 ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3717, 321),
  INST(Vdbpsadbw       , "vdbpsadbw"       , Enc(VexRvmi_Lx)        , V(660F3A,42,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3728, 322),
  INST(Vdivpd          , "vdivpd"          , Enc(VexRvm_Lx)         , V(660F00,5E,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 3738, 255),
  INST(Vdivps          , "vdivps"          , Enc(VexRvm_Lx)         , V(000F00,5E,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 3745, 256),
  INST(Vdivsd          , "vdivsd"          , Enc(VexRvm)            , V(F20F00,5E,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3752, 257),
  INST(Vdivss          , "vdivss"          , Enc(VexRvm)            , V(F30F00,5E,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3759, 258),
  INST(Vdppd           , "vdppd"           , Enc(VexRvmi_Lx)        , V(660F3A,41,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 3766, 270),
  INST(Vdpps           , "vdpps"           , Enc(VexRvmi_Lx)        , V(660F3A,40,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 3772, 270),
  INST(Vexp2pd         , "vexp2pd"         , Enc(VexRm)             , V(660F38,C8,_,2,_,1,4,FV ), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 3778, 323),
  INST(Vexp2ps         , "vexp2ps"         , Enc(VexRm)             , V(660F38,C8,_,2,_,0,4,FV ), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 3786, 324),
  INST(Vexpandpd       , "vexpandpd"       , Enc(VexRm_Lx)          , V(660F38,88,_,x,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3794, 325),
  INST(Vexpandps       , "vexpandps"       , Enc(VexRm_Lx)          , V(660F38,88,_,x,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3804, 325),
  INST(Vextractf128    , "vextractf128"    , Enc(VexMri)            , V(660F3A,19,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 3814, 326),
  INST(Vextractf32x4   , "vextractf32x4"   , Enc(VexMri_Lx)         , V(660F3A,19,_,x,_,0,4,T4 ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3827, 327),
  INST(Vextractf32x8   , "vextractf32x8"   , Enc(VexMri)            , V(660F3A,1B,_,2,_,0,5,T8 ), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3841, 328),
  INST(Vextractf64x2   , "vextractf64x2"   , Enc(VexMri_Lx)         , V(660F3A,19,_,x,_,1,4,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3855, 329),
  INST(Vextractf64x4   , "vextractf64x4"   , Enc(VexMri)            , V(660F3A,1B,_,2,_,1,5,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3869, 330),
  INST(Vextracti128    , "vextracti128"    , Enc(VexMri)            , V(660F3A,39,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 3883, 326),
  INST(Vextracti32x4   , "vextracti32x4"   , Enc(VexMri_Lx)         , V(660F3A,39,_,x,_,0,4,T4 ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3896, 327),
  INST(Vextracti32x8   , "vextracti32x8"   , Enc(VexMri)            , V(660F3A,3B,_,2,_,0,5,T8 ), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3910, 328),
  INST(Vextracti64x2   , "vextracti64x2"   , Enc(VexMri_Lx)         , V(660F3A,39,_,x,_,1,4,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3924, 329),
  INST(Vextracti64x4   , "vextracti64x4"   , Enc(VexMri)            , V(660F3A,3B,_,2,_,1,5,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3938, 330),
  INST(Vextractps      , "vextractps"      , Enc(VexMri)            , V(660F3A,17,_,0,I,I,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3952, 331),
  INST(Vfixupimmpd     , "vfixupimmpd"     , Enc(VexRvmi_Lx)        , V(660F3A,54,_,x,_,1,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 3963, 332),
  INST(Vfixupimmps     , "vfixupimmps"     , Enc(VexRvmi_Lx)        , V(660F3A,54,_,x,_,0,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,SAE,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 3975, 333),
  INST(Vfixupimmsd     , "vfixupimmsd"     , Enc(VexRvmi)           , V(660F3A,55,_,I,_,1,3,T1S), 0                         , F(RW)          |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3987, 334),
  INST(Vfixupimmss     , "vfixupimmss"     , Enc(VexRvmi)           , V(660F3A,55,_,I,_,0,2,T1S), 0                         , F(RW)          |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 3999, 335),
  INST(Vfmadd132pd     , "vfmadd132pd"     , Enc(VexRvm_Lx)         , V(660F38,98,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4011, 336),
  INST(Vfmadd132ps     , "vfmadd132ps"     , Enc(VexRvm_Lx)         , V(660F38,98,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 4023, 337),
  INST(Vfmadd132sd     , "vfmadd132sd"     , Enc(VexRvm)            , V(660F38,99,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4035, 338),
  INST(Vfmadd132ss     , "vfmadd132ss"     , Enc(VexRvm)            , V(660F38,99,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4047, 339),
  INST(Vfmadd213pd     , "vfmadd213pd"     , Enc(VexRvm_Lx)         , V(660F38,A8,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4059, 336),
  INST(Vfmadd213ps     , "vfmadd213ps"     , Enc(VexRvm_Lx)         , V(660F38,A8,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 4071, 337),
  INST(Vfmadd213sd     , "vfmadd213sd"     , Enc(VexRvm)            , V(660F38,A9,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4083, 338),
  INST(Vfmadd213ss     , "vfmadd213ss"     , Enc(VexRvm)            , V(660F38,A9,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4095, 339),
  INST(Vfmadd231pd     , "vfmadd231pd"     , Enc(VexRvm_Lx)         , V(660F38,B8,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4107, 336),
  INST(Vfmadd231ps     , "vfmadd231ps"     , Enc(VexRvm_Lx)         , V(660F38,B8,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 4119, 337),
  INST(Vfmadd231sd     , "vfmadd231sd"     , Enc(VexRvm)            , V(660F38,B9,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4131, 338),
  INST(Vfmadd231ss     , "vfmadd231ss"     , Enc(VexRvm)            , V(660F38,B9,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4143, 339),
  INST(Vfmaddpd        , "vfmaddpd"        , Enc(Fma4_Lx)           , V(660F3A,69,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4155, 340),
  INST(Vfmaddps        , "vfmaddps"        , Enc(Fma4_Lx)           , V(660F3A,68,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4164, 340),
  INST(Vfmaddsd        , "vfmaddsd"        , Enc(Fma4)              , V(660F3A,6B,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4173, 341),
  INST(Vfmaddss        , "vfmaddss"        , Enc(Fma4)              , V(660F3A,6A,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4182, 342),
  INST(Vfmaddsub132pd  , "vfmaddsub132pd"  , Enc(VexRvm_Lx)         , V(660F38,96,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4191, 336),
  INST(Vfmaddsub132ps  , "vfmaddsub132ps"  , Enc(VexRvm_Lx)         , V(660F38,96,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 4206, 337),
  INST(Vfmaddsub213pd  , "vfmaddsub213pd"  , Enc(VexRvm_Lx)         , V(660F38,A6,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4221, 336),
  INST(Vfmaddsub213ps  , "vfmaddsub213ps"  , Enc(VexRvm_Lx)         , V(660F38,A6,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 4236, 337),
  INST(Vfmaddsub231pd  , "vfmaddsub231pd"  , Enc(VexRvm_Lx)         , V(660F38,B6,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4251, 336),
  INST(Vfmaddsub231ps  , "vfmaddsub231ps"  , Enc(VexRvm_Lx)         , V(660F38,B6,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 4266, 337),
  INST(Vfmaddsubpd     , "vfmaddsubpd"     , Enc(Fma4_Lx)           , V(660F3A,5D,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4281, 340),
  INST(Vfmaddsubps     , "vfmaddsubps"     , Enc(Fma4_Lx)           , V(660F3A,5C,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4293, 340),
  INST(Vfmsub132pd     , "vfmsub132pd"     , Enc(VexRvm_Lx)         , V(660F38,9A,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4305, 336),
  INST(Vfmsub132ps     , "vfmsub132ps"     , Enc(VexRvm_Lx)         , V(660F38,9A,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 4317, 337),
  INST(Vfmsub132sd     , "vfmsub132sd"     , Enc(VexRvm)            , V(660F38,9B,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4329, 338),
  INST(Vfmsub132ss     , "vfmsub132ss"     , Enc(VexRvm)            , V(660F38,9B,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4341, 339),
  INST(Vfmsub213pd     , "vfmsub213pd"     , Enc(VexRvm_Lx)         , V(660F38,AA,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4353, 336),
  INST(Vfmsub213ps     , "vfmsub213ps"     , Enc(VexRvm_Lx)         , V(660F38,AA,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 4365, 337),
  INST(Vfmsub213sd     , "vfmsub213sd"     , Enc(VexRvm)            , V(660F38,AB,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4377, 338),
  INST(Vfmsub213ss     , "vfmsub213ss"     , Enc(VexRvm)            , V(660F38,AB,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4389, 339),
  INST(Vfmsub231pd     , "vfmsub231pd"     , Enc(VexRvm_Lx)         , V(660F38,BA,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4401, 336),
  INST(Vfmsub231ps     , "vfmsub231ps"     , Enc(VexRvm_Lx)         , V(660F38,BA,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 4413, 337),
  INST(Vfmsub231sd     , "vfmsub231sd"     , Enc(VexRvm)            , V(660F38,BB,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4425, 338),
  INST(Vfmsub231ss     , "vfmsub231ss"     , Enc(VexRvm)            , V(660F38,BB,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4437, 339),
  INST(Vfmsubadd132pd  , "vfmsubadd132pd"  , Enc(VexRvm_Lx)         , V(660F38,97,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4449, 336),
  INST(Vfmsubadd132ps  , "vfmsubadd132ps"  , Enc(VexRvm_Lx)         , V(660F38,97,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 4464, 337),
  INST(Vfmsubadd213pd  , "vfmsubadd213pd"  , Enc(VexRvm_Lx)         , V(660F38,A7,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4479, 336),
  INST(Vfmsubadd213ps  , "vfmsubadd213ps"  , Enc(VexRvm_Lx)         , V(660F38,A7,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 4494, 337),
  INST(Vfmsubadd231pd  , "vfmsubadd231pd"  , Enc(VexRvm_Lx)         , V(660F38,B7,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4509, 336),
  INST(Vfmsubadd231ps  , "vfmsubadd231ps"  , Enc(VexRvm_Lx)         , V(660F38,B7,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 4524, 337),
  INST(Vfmsubaddpd     , "vfmsubaddpd"     , Enc(Fma4_Lx)           , V(660F3A,5F,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4539, 340),
  INST(Vfmsubaddps     , "vfmsubaddps"     , Enc(Fma4_Lx)           , V(660F3A,5E,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4551, 340),
  INST(Vfmsubpd        , "vfmsubpd"        , Enc(Fma4_Lx)           , V(660F3A,6D,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4563, 340),
  INST(Vfmsubps        , "vfmsubps"        , Enc(Fma4_Lx)           , V(660F3A,6C,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4572, 340),
  INST(Vfmsubsd        , "vfmsubsd"        , Enc(Fma4)              , V(660F3A,6F,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4581, 341),
  INST(Vfmsubss        , "vfmsubss"        , Enc(Fma4)              , V(660F3A,6E,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4590, 342),
  INST(Vfnmadd132pd    , "vfnmadd132pd"    , Enc(VexRvm_Lx)         , V(660F38,9C,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4599, 336),
  INST(Vfnmadd132ps    , "vfnmadd132ps"    , Enc(VexRvm_Lx)         , V(660F38,9C,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 4612, 337),
  INST(Vfnmadd132sd    , "vfnmadd132sd"    , Enc(VexRvm)            , V(660F38,9D,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4625, 338),
  INST(Vfnmadd132ss    , "vfnmadd132ss"    , Enc(VexRvm)            , V(660F38,9D,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4638, 339),
  INST(Vfnmadd213pd    , "vfnmadd213pd"    , Enc(VexRvm_Lx)         , V(660F38,AC,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4651, 336),
  INST(Vfnmadd213ps    , "vfnmadd213ps"    , Enc(VexRvm_Lx)         , V(660F38,AC,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 4664, 337),
  INST(Vfnmadd213sd    , "vfnmadd213sd"    , Enc(VexRvm)            , V(660F38,AD,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4677, 338),
  INST(Vfnmadd213ss    , "vfnmadd213ss"    , Enc(VexRvm)            , V(660F38,AD,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4690, 339),
  INST(Vfnmadd231pd    , "vfnmadd231pd"    , Enc(VexRvm_Lx)         , V(660F38,BC,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4703, 336),
  INST(Vfnmadd231ps    , "vfnmadd231ps"    , Enc(VexRvm_Lx)         , V(660F38,BC,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 4716, 337),
  INST(Vfnmadd231sd    , "vfnmadd231sd"    , Enc(VexRvm)            , V(660F38,BC,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4729, 338),
  INST(Vfnmadd231ss    , "vfnmadd231ss"    , Enc(VexRvm)            , V(660F38,BC,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4742, 339),
  INST(Vfnmaddpd       , "vfnmaddpd"       , Enc(Fma4_Lx)           , V(660F3A,79,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4755, 340),
  INST(Vfnmaddps       , "vfnmaddps"       , Enc(Fma4_Lx)           , V(660F3A,78,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4765, 340),
  INST(Vfnmaddsd       , "vfnmaddsd"       , Enc(Fma4)              , V(660F3A,7B,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4775, 341),
  INST(Vfnmaddss       , "vfnmaddss"       , Enc(Fma4)              , V(660F3A,7A,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4785, 342),
  INST(Vfnmsub132pd    , "vfnmsub132pd"    , Enc(VexRvm_Lx)         , V(660F38,9E,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4795, 336),
  INST(Vfnmsub132ps    , "vfnmsub132ps"    , Enc(VexRvm_Lx)         , V(660F38,9E,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 4808, 337),
  INST(Vfnmsub132sd    , "vfnmsub132sd"    , Enc(VexRvm)            , V(660F38,9F,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4821, 338),
  INST(Vfnmsub132ss    , "vfnmsub132ss"    , Enc(VexRvm)            , V(660F38,9F,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4834, 339),
  INST(Vfnmsub213pd    , "vfnmsub213pd"    , Enc(VexRvm_Lx)         , V(660F38,AE,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4847, 336),
  INST(Vfnmsub213ps    , "vfnmsub213ps"    , Enc(VexRvm_Lx)         , V(660F38,AE,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 4860, 337),
  INST(Vfnmsub213sd    , "vfnmsub213sd"    , Enc(VexRvm)            , V(660F38,AF,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4873, 338),
  INST(Vfnmsub213ss    , "vfnmsub213ss"    , Enc(VexRvm)            , V(660F38,AF,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4886, 339),
  INST(Vfnmsub231pd    , "vfnmsub231pd"    , Enc(VexRvm_Lx)         , V(660F38,BE,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4899, 336),
  INST(Vfnmsub231ps    , "vfnmsub231ps"    , Enc(VexRvm_Lx)         , V(660F38,BE,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 4912, 337),
  INST(Vfnmsub231sd    , "vfnmsub231sd"    , Enc(VexRvm)            , V(660F38,BF,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4925, 338),
  INST(Vfnmsub231ss    , "vfnmsub231ss"    , Enc(VexRvm)            , V(660F38,BF,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 4938, 339),
  INST(Vfnmsubpd       , "vfnmsubpd"       , Enc(Fma4_Lx)           , V(660F3A,7D,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4951, 340),
  INST(Vfnmsubps       , "vfnmsubps"       , Enc(Fma4_Lx)           , V(660F3A,7C,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4961, 340),
  INST(Vfnmsubsd       , "vfnmsubsd"       , Enc(Fma4)              , V(660F3A,7F,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4971, 341),
  INST(Vfnmsubss       , "vfnmsubss"       , Enc(Fma4)              , V(660F3A,7E,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 4981, 342),
  INST(Vfpclasspd      , "vfpclasspd"      , Enc(VexRmi_Lx)         , V(660F3A,66,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,K_,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 4991, 343),
  INST(Vfpclassps      , "vfpclassps"      , Enc(VexRmi_Lx)         , V(660F3A,66,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,K_,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 5002, 344),
  INST(Vfpclasssd      , "vfpclasssd"      , Enc(VexRmi_Lx)         , V(660F3A,67,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(DQ  ,0,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5013, 345),
  INST(Vfpclassss      , "vfpclassss"      , Enc(VexRmi_Lx)         , V(660F3A,67,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(DQ  ,0,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5024, 346),
  INST(Vfrczpd         , "vfrczpd"         , Enc(VexRm_Lx)          , V(XOP_M9,81,_,x,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 5035, 347),
  INST(Vfrczps         , "vfrczps"         , Enc(VexRm_Lx)          , V(XOP_M9,80,_,x,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 5043, 347),
  INST(Vfrczsd         , "vfrczsd"         , Enc(VexRm)             , V(XOP_M9,83,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 5051, 348),
  INST(Vfrczss         , "vfrczss"         , Enc(VexRm)             , V(XOP_M9,82,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 5059, 349),
  INST(Vgatherdpd      , "vgatherdpd"      , Enc(VexRmvRm_VM)       , V(660F38,92,_,x,1,_,_,_  ), V(660F38,92,_,x,_,1,3,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5067, 350),
  INST(Vgatherdps      , "vgatherdps"      , Enc(VexRmvRm_VM)       , V(660F38,92,_,x,0,_,_,_  ), V(660F38,92,_,x,_,0,2,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5078, 351),
  INST(Vgatherpf0dpd   , "vgatherpf0dpd"   , Enc(VexM_VM)           , V(660F38,C6,1,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5089, 352),
  INST(Vgatherpf0dps   , "vgatherpf0dps"   , Enc(VexM_VM)           , V(660F38,C6,1,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5103, 353),
  INST(Vgatherpf0qpd   , "vgatherpf0qpd"   , Enc(VexM_VM)           , V(660F38,C7,1,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5117, 354),
  INST(Vgatherpf0qps   , "vgatherpf0qps"   , Enc(VexM_VM)           , V(660F38,C7,1,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5131, 354),
  INST(Vgatherpf1dpd   , "vgatherpf1dpd"   , Enc(VexM_VM)           , V(660F38,C6,2,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5145, 352),
  INST(Vgatherpf1dps   , "vgatherpf1dps"   , Enc(VexM_VM)           , V(660F38,C6,2,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5159, 353),
  INST(Vgatherpf1qpd   , "vgatherpf1qpd"   , Enc(VexM_VM)           , V(660F38,C7,2,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5173, 354),
  INST(Vgatherpf1qps   , "vgatherpf1qps"   , Enc(VexM_VM)           , V(660F38,C7,2,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5187, 354),
  INST(Vgatherqpd      , "vgatherqpd"      , Enc(VexRmvRm_VM)       , V(660F38,93,_,x,1,_,_,_  ), V(660F38,93,_,x,_,1,3,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5201, 355),
  INST(Vgatherqps      , "vgatherqps"      , Enc(VexRmvRm_VM)       , V(660F38,93,_,x,0,_,_,_  ), V(660F38,93,_,x,_,0,2,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5212, 356),
  INST(Vgetexppd       , "vgetexppd"       , Enc(VexRm_Lx)          , V(660F38,42,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 5223, 310),
  INST(Vgetexpps       , "vgetexpps"       , Enc(VexRm_Lx)          , V(660F38,42,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 5233, 315),
  INST(Vgetexpsd       , "vgetexpsd"       , Enc(VexRm)             , V(660F38,43,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5243, 357),
  INST(Vgetexpss       , "vgetexpss"       , Enc(VexRm)             , V(660F38,43,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5253, 358),
  INST(Vgetmantpd      , "vgetmantpd"      , Enc(VexRmi_Lx)         , V(660F3A,26,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 5263, 359),
  INST(Vgetmantps      , "vgetmantps"      , Enc(VexRmi_Lx)         , V(660F3A,26,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 5274, 360),
  INST(Vgetmantsd      , "vgetmantsd"      , Enc(VexRmi)            , V(660F3A,27,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5285, 361),
  INST(Vgetmantss      , "vgetmantss"      , Enc(VexRmi)            , V(660F3A,27,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5296, 362),
  INST(Vhaddpd         , "vhaddpd"         , Enc(VexRvm_Lx)         , V(660F00,7C,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 5307, 259),
  INST(Vhaddps         , "vhaddps"         , Enc(VexRvm_Lx)         , V(F20F00,7C,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 5315, 259),
  INST(Vhsubpd         , "vhsubpd"         , Enc(VexRvm_Lx)         , V(660F00,7D,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 5323, 259),
  INST(Vhsubps         , "vhsubps"         , Enc(VexRvm_Lx)         , V(F20F00,7D,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 5331, 259),
  INST(Vinsertf128     , "vinsertf128"     , Enc(VexRvmi)           , V(660F3A,18,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 5339, 363),
  INST(Vinsertf32x4    , "vinsertf32x4"    , Enc(VexRvmi_Lx)        , V(660F3A,18,_,x,_,0,4,T4 ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5351, 364),
  INST(Vinsertf32x8    , "vinsertf32x8"    , Enc(VexRvmi)           , V(660F3A,1A,_,2,_,0,5,T8 ), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5364, 365),
  INST(Vinsertf64x2    , "vinsertf64x2"    , Enc(VexRvmi_Lx)        , V(660F3A,18,_,x,_,1,4,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5377, 366),
  INST(Vinsertf64x4    , "vinsertf64x4"    , Enc(VexRvmi)           , V(660F3A,1A,_,2,_,1,5,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5390, 367),
  INST(Vinserti128     , "vinserti128"     , Enc(VexRvmi)           , V(660F3A,38,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 5403, 363),
  INST(Vinserti32x4    , "vinserti32x4"    , Enc(VexRvmi_Lx)        , V(660F3A,38,_,x,_,0,4,T4 ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5415, 364),
  INST(Vinserti32x8    , "vinserti32x8"    , Enc(VexRvmi)           , V(660F3A,3A,_,2,_,0,5,T8 ), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5428, 365),
  INST(Vinserti64x2    , "vinserti64x2"    , Enc(VexRvmi_Lx)        , V(660F3A,38,_,x,_,1,4,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5441, 366),
  INST(Vinserti64x4    , "vinserti64x4"    , Enc(VexRvmi)           , V(660F3A,3A,_,2,_,1,5,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5454, 367),
  INST(Vinsertps       , "vinsertps"       , Enc(VexRvmi)           , V(660F3A,21,_,0,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5467, 368),
  INST(Vlddqu          , "vlddqu"          , Enc(VexRm_Lx)          , V(F20F00,F0,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 5477, 369),
  INST(Vldmxcsr        , "vldmxcsr"        , Enc(VexM)              , V(000F00,AE,2,0,I,_,_,_  ), 0                         , F(RO)|F(Vex)|F(Volatile)              , EF(________), 0 , 0 , kFamilyNone, 0  , 5484, 370),
  INST(Vmaskmovdqu     , "vmaskmovdqu"     , Enc(VexRmZDI)          , V(660F00,F7,_,0,I,_,_,_  ), 0                         , F(RO)|F(Vex)|F(Special)               , EF(________), 0 , 0 , kFamilyNone, 0  , 5493, 371),
  INST(Vmaskmovpd      , "vmaskmovpd"      , Enc(VexRvmMvr_Lx)      , V(660F38,2D,_,x,0,_,_,_  ), V(660F38,2F,_,x,0,_,_,_  ), F(RW)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 5505, 372),
  INST(Vmaskmovps      , "vmaskmovps"      , Enc(VexRvmMvr_Lx)      , V(660F38,2C,_,x,0,_,_,_  ), V(660F38,2E,_,x,0,_,_,_  ), F(RW)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 5516, 373),
  INST(Vmaxpd          , "vmaxpd"          , Enc(VexRvm_Lx)         , V(660F00,5F,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 5527, 374),
  INST(Vmaxps          , "vmaxps"          , Enc(VexRvm_Lx)         , V(000F00,5F,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 5534, 375),
  INST(Vmaxsd          , "vmaxsd"          , Enc(VexRvm)            , V(F20F00,5F,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5541, 376),
  INST(Vmaxss          , "vmaxss"          , Enc(VexRvm)            , V(F30F00,5F,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5548, 377),
  INST(Vminpd          , "vminpd"          , Enc(VexRvm_Lx)         , V(660F00,5D,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 5555, 374),
  INST(Vminps          , "vminps"          , Enc(VexRvm_Lx)         , V(000F00,5D,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 5562, 375),
  INST(Vminsd          , "vminsd"          , Enc(VexRvm)            , V(F20F00,5D,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5569, 376),
  INST(Vminss          , "vminss"          , Enc(VexRvm)            , V(F30F00,5D,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5576, 377),
  INST(Vmovapd         , "vmovapd"         , Enc(VexRmMr_Lx)        , V(660F00,28,_,x,I,1,4,FVM), V(660F00,29,_,x,I,1,4,FVM), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5583, 378),
  INST(Vmovaps         , "vmovaps"         , Enc(VexRmMr_Lx)        , V(000F00,28,_,x,I,0,4,FVM), V(000F00,29,_,x,I,0,4,FVM), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5591, 379),
  INST(Vmovd           , "vmovd"           , Enc(VexMovDQ)          , V(660F00,6E,_,0,0,0,2,T1S), V(660F00,7E,_,0,0,0,2,T1S), F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5599, 380),
  INST(Vmovddup        , "vmovddup"        , Enc(VexRm_Lx)          , V(F20F00,12,_,x,I,1,3,DUP), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5605, 381),
  INST(Vmovdqa         , "vmovdqa"         , Enc(VexRmMr_Lx)        , V(660F00,6F,_,x,I,_,_,_  ), V(660F00,7F,_,x,I,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 5614, 382),
  INST(Vmovdqa32       , "vmovdqa32"       , Enc(VexRmMr_Lx)        , V(660F00,6F,_,x,_,0,4,FVM), V(660F00,7F,_,x,_,0,4,FVM), F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5622, 383),
  INST(Vmovdqa64       , "vmovdqa64"       , Enc(VexRmMr_Lx)        , V(660F00,6F,_,x,_,1,4,FVM), V(660F00,7F,_,x,_,1,4,FVM), F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5632, 384),
  INST(Vmovdqu         , "vmovdqu"         , Enc(VexRmMr_Lx)        , V(F30F00,6F,_,x,I,_,_,_  ), V(F30F00,7F,_,x,I,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 5642, 385),
  INST(Vmovdqu16       , "vmovdqu16"       , Enc(VexRmMr_Lx)        , V(F20F00,6F,_,x,_,1,4,FVM), V(F20F00,7F,_,x,_,1,4,FVM), F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5650, 386),
  INST(Vmovdqu32       , "vmovdqu32"       , Enc(VexRmMr_Lx)        , V(F30F00,6F,_,x,_,0,4,FVM), V(F30F00,7F,_,x,_,0,4,FVM), F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5660, 387),
  INST(Vmovdqu64       , "vmovdqu64"       , Enc(VexRmMr_Lx)        , V(F30F00,6F,_,x,_,1,4,FVM), V(F30F00,7F,_,x,_,1,4,FVM), F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5670, 388),
  INST(Vmovdqu8        , "vmovdqu8"        , Enc(VexRmMr_Lx)        , V(F20F00,6F,_,x,_,0,4,FVM), V(F20F00,7F,_,x,_,0,4,FVM), F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5680, 389),
  INST(Vmovhlps        , "vmovhlps"        , Enc(VexRvm)            , V(000F00,12,_,0,I,0,_,_  ), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5689, 390),
  INST(Vmovhpd         , "vmovhpd"         , Enc(VexRvmMr)          , V(660F00,16,_,0,I,1,3,T1S), V(660F00,17,_,0,I,1,3,T1S), F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5698, 391),
  INST(Vmovhps         , "vmovhps"         , Enc(VexRvmMr)          , V(000F00,16,_,0,I,0,3,T2 ), V(000F00,17,_,0,I,0,3,T2 ), F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5706, 392),
  INST(Vmovlhps        , "vmovlhps"        , Enc(VexRvm)            , V(000F00,16,_,0,I,0,_,_  ), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5714, 390),
  INST(Vmovlpd         , "vmovlpd"         , Enc(VexRvmMr)          , V(660F00,12,_,0,I,1,3,T1S), V(660F00,13,_,0,I,1,3,T1S), F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5723, 393),
  INST(Vmovlps         , "vmovlps"         , Enc(VexRvmMr)          , V(000F00,12,_,0,I,0,3,T2 ), V(000F00,13,_,0,I,0,3,T2 ), F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5731, 394),
  INST(Vmovmskpd       , "vmovmskpd"       , Enc(VexRm_Lx)          , V(660F00,50,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 5739, 395),
  INST(Vmovmskps       , "vmovmskps"       , Enc(VexRm_Lx)          , V(000F00,50,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 5749, 395),
  INST(Vmovntdq        , "vmovntdq"        , Enc(VexMr_Lx)          , V(660F00,E7,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5759, 396),
  INST(Vmovntdqa       , "vmovntdqa"       , Enc(VexRm_Lx)          , V(660F38,2A,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5768, 397),
  INST(Vmovntpd        , "vmovntpd"        , Enc(VexMr_Lx)          , V(660F00,2B,_,x,I,1,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5778, 396),
  INST(Vmovntps        , "vmovntps"        , Enc(VexMr_Lx)          , V(000F00,2B,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5787, 396),
  INST(Vmovq           , "vmovq"           , Enc(VexMovDQ)          , V(660F00,6E,_,0,I,1,3,T1S), V(660F00,7E,_,0,I,1,3,T1S), F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5796, 398),
  INST(Vmovsd          , "vmovsd"          , Enc(VexMovSsSd)        , V(F20F00,10,_,I,I,1,3,T1S), V(F20F00,11,_,I,I,1,3,T1S), F(WO)|F(Vex)   |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5802, 399),
  INST(Vmovshdup       , "vmovshdup"       , Enc(VexRm_Lx)          , V(F30F00,16,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5809, 400),
  INST(Vmovsldup       , "vmovsldup"       , Enc(VexRm_Lx)          , V(F30F00,12,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5819, 400),
  INST(Vmovss          , "vmovss"          , Enc(VexMovSsSd)        , V(F30F00,10,_,I,I,0,2,T1S), V(F30F00,11,_,I,I,0,2,T1S), F(WO)|F(Vex)   |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5829, 401),
  INST(Vmovupd         , "vmovupd"         , Enc(VexRmMr_Lx)        , V(660F00,10,_,x,I,1,4,FVM), V(660F00,11,_,x,I,1,4,FVM), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5836, 402),
  INST(Vmovups         , "vmovups"         , Enc(VexRmMr_Lx)        , V(000F00,10,_,x,I,0,4,FVM), V(000F00,11,_,x,I,0,4,FVM), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5844, 403),
  INST(Vmpsadbw        , "vmpsadbw"        , Enc(VexRvmi_Lx)        , V(660F3A,42,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 5852, 270),
  INST(Vmulpd          , "vmulpd"          , Enc(VexRvm_Lx)         , V(660F00,59,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 5861, 255),
  INST(Vmulps          , "vmulps"          , Enc(VexRvm_Lx)         , V(000F00,59,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 5868, 256),
  INST(Vmulsd          , "vmulsd"          , Enc(VexRvm_Lx)         , V(F20F00,59,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5875, 257),
  INST(Vmulss          , "vmulss"          , Enc(VexRvm_Lx)         , V(F30F00,59,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5882, 258),
  INST(Vorpd           , "vorpd"           , Enc(VexRvm_Lx)         , V(660F00,56,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 5889, 265),
  INST(Vorps           , "vorps"           , Enc(VexRvm_Lx)         , V(000F00,56,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 5895, 404),
  INST(Vpabsb          , "vpabsb"          , Enc(VexRm_Lx)          , V(660F38,1C,_,x,I,_,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5901, 405),
  INST(Vpabsd          , "vpabsd"          , Enc(VexRm_Lx)          , V(660F38,1E,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5908, 400),
  INST(Vpabsq          , "vpabsq"          , Enc(VexRm_Lx)          , V(660F38,1F,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5915, 325),
  INST(Vpabsw          , "vpabsw"          , Enc(VexRm_Lx)          , V(660F38,1D,_,x,I,_,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5922, 405),
  INST(Vpackssdw       , "vpackssdw"       , Enc(VexRvm_Lx)         , V(660F00,6B,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 5929, 406),
  INST(Vpacksswb       , "vpacksswb"       , Enc(VexRvm_Lx)         , V(660F00,63,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5939, 407),
  INST(Vpackusdw       , "vpackusdw"       , Enc(VexRvm_Lx)         , V(660F38,2B,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 5949, 406),
  INST(Vpackuswb       , "vpackuswb"       , Enc(VexRvm_Lx)         , V(660F00,67,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5959, 407),
  INST(Vpaddb          , "vpaddb"          , Enc(VexRvm_Lx)         , V(660F00,FC,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5969, 407),
  INST(Vpaddd          , "vpaddd"          , Enc(VexRvm_Lx)         , V(660F00,FE,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 5976, 404),
  INST(Vpaddq          , "vpaddq"          , Enc(VexRvm_Lx)         , V(660F00,D4,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 5983, 408),
  INST(Vpaddsb         , "vpaddsb"         , Enc(VexRvm_Lx)         , V(660F00,EC,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5990, 407),
  INST(Vpaddsw         , "vpaddsw"         , Enc(VexRvm_Lx)         , V(660F00,ED,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 5998, 407),
  INST(Vpaddusb        , "vpaddusb"        , Enc(VexRvm_Lx)         , V(660F00,DC,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6006, 407),
  INST(Vpaddusw        , "vpaddusw"        , Enc(VexRvm_Lx)         , V(660F00,DD,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6015, 407),
  INST(Vpaddw          , "vpaddw"          , Enc(VexRvm_Lx)         , V(660F00,FD,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6024, 407),
  INST(Vpalignr        , "vpalignr"        , Enc(VexRvmi_Lx)        , V(660F3A,0F,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6031, 409),
  INST(Vpand           , "vpand"           , Enc(VexRvm_Lx)         , V(660F00,DB,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6040, 259),
  INST(Vpandd          , "vpandd"          , Enc(VexRvm_Lx)         , V(660F00,DB,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 6046, 268),
  INST(Vpandn          , "vpandn"          , Enc(VexRvm_Lx)         , V(660F00,DF,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6053, 259),
  INST(Vpandnd         , "vpandnd"         , Enc(VexRvm_Lx)         , V(660F00,DF,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 6060, 268),
  INST(Vpandnq         , "vpandnq"         , Enc(VexRvm_Lx)         , V(660F00,DF,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 6068, 269),
  INST(Vpandq          , "vpandq"          , Enc(VexRvm_Lx)         , V(660F00,DB,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 6076, 269),
  INST(Vpavgb          , "vpavgb"          , Enc(VexRvm_Lx)         , V(660F00,E0,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6083, 407),
  INST(Vpavgw          , "vpavgw"          , Enc(VexRvm_Lx)         , V(660F00,E3,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6090, 407),
  INST(Vpblendd        , "vpblendd"        , Enc(VexRvmi_Lx)        , V(660F3A,02,_,x,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6097, 270),
  INST(Vpblendvb       , "vpblendvb"       , Enc(VexRvmr)           , V(660F3A,4C,_,x,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6106, 271),
  INST(Vpblendw        , "vpblendw"        , Enc(VexRvmi_Lx)        , V(660F3A,0E,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6116, 270),
  INST(Vpbroadcastb    , "vpbroadcastb"    , Enc(VexRm_Lx)          , V(660F38,78,_,x,0,0,0,T1S), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6125, 410),
  INST(Vpbroadcastd    , "vpbroadcastd"    , Enc(VexRm_Lx)          , V(660F38,58,_,x,0,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6138, 411),
  INST(Vpbroadcastmb2d , "vpbroadcastmb2d" , Enc(VexRm_Lx)          , V(F30F38,3A,_,x,_,0,_,_  ), 0                         , F(WO)          |A512(CDI ,1,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6151, 412),
  INST(Vpbroadcastmb2q , "vpbroadcastmb2q" , Enc(VexRm_Lx)          , V(F30F38,2A,_,x,_,1,_,_  ), 0                         , F(WO)          |A512(CDI ,1,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6167, 412),
  INST(Vpbroadcastq    , "vpbroadcastq"    , Enc(VexRm_Lx)          , V(660F38,59,_,x,0,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6183, 413),
  INST(Vpbroadcastw    , "vpbroadcastw"    , Enc(VexRm_Lx)          , V(660F38,79,_,x,0,0,1,T1S), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6196, 414),
  INST(Vpclmulqdq      , "vpclmulqdq"      , Enc(VexRvmi)           , V(660F3A,44,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6209, 415),
  INST(Vpcmov          , "vpcmov"          , Enc(VexRvrmRvmr_Lx)    , V(XOP_M8,A2,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6220, 340),
  INST(Vpcmpb          , "vpcmpb"          , Enc(VexRvm_Lx)         , V(660F3A,3F,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6227, 416),
  INST(Vpcmpd          , "vpcmpd"          , Enc(VexRvm_Lx)         , V(660F3A,1F,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 6234, 417),
  INST(Vpcmpeqb        , "vpcmpeqb"        , Enc(VexRvm_Lx)         , V(660F00,74,_,x,I,I,4,FV ), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6241, 418),
  INST(Vpcmpeqd        , "vpcmpeqd"        , Enc(VexRvm_Lx)         , V(660F00,76,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,K_,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 6250, 419),
  INST(Vpcmpeqq        , "vpcmpeqq"        , Enc(VexRvm_Lx)         , V(660F38,29,_,x,I,1,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,K_,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 6259, 420),
  INST(Vpcmpeqw        , "vpcmpeqw"        , Enc(VexRvm_Lx)         , V(660F00,75,_,x,I,I,4,FV ), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6268, 418),
  INST(Vpcmpestri      , "vpcmpestri"      , Enc(VexRmi)            , V(660F3A,61,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)|F(Special)               , EF(________), 0 , 0 , kFamilyNone, 0  , 6277, 421),
  INST(Vpcmpestrm      , "vpcmpestrm"      , Enc(VexRmi)            , V(660F3A,60,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)|F(Special)               , EF(________), 0 , 0 , kFamilyNone, 0  , 6288, 422),
  INST(Vpcmpgtb        , "vpcmpgtb"        , Enc(VexRvm_Lx)         , V(660F00,64,_,x,I,I,4,FV ), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6299, 418),
  INST(Vpcmpgtd        , "vpcmpgtd"        , Enc(VexRvm_Lx)         , V(660F00,66,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,K_,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 6308, 419),
  INST(Vpcmpgtq        , "vpcmpgtq"        , Enc(VexRvm_Lx)         , V(660F38,37,_,x,I,1,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,K_,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 6317, 420),
  INST(Vpcmpgtw        , "vpcmpgtw"        , Enc(VexRvm_Lx)         , V(660F00,65,_,x,I,I,4,FV ), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6326, 418),
  INST(Vpcmpistri      , "vpcmpistri"      , Enc(VexRmi)            , V(660F3A,63,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)|F(Special)               , EF(________), 0 , 0 , kFamilyNone, 0  , 6335, 423),
  INST(Vpcmpistrm      , "vpcmpistrm"      , Enc(VexRmi)            , V(660F3A,62,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)|F(Special)               , EF(________), 0 , 0 , kFamilyNone, 0  , 6346, 424),
  INST(Vpcmpq          , "vpcmpq"          , Enc(VexRvm_Lx)         , V(660F3A,1F,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 6357, 425),
  INST(Vpcmpub         , "vpcmpub"         , Enc(VexRvm_Lx)         , V(660F3A,3E,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6364, 416),
  INST(Vpcmpud         , "vpcmpud"         , Enc(VexRvm_Lx)         , V(660F3A,1E,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 6372, 417),
  INST(Vpcmpuq         , "vpcmpuq"         , Enc(VexRvm_Lx)         , V(660F3A,1E,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 6380, 425),
  INST(Vpcmpuw         , "vpcmpuw"         , Enc(VexRvm_Lx)         , V(660F3A,3E,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 6388, 426),
  INST(Vpcmpw          , "vpcmpw"          , Enc(VexRvm_Lx)         , V(660F3A,3F,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 6396, 426),
  INST(Vpcomb          , "vpcomb"          , Enc(VexRvmi)           , V(XOP_M8,CC,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6403, 415),
  INST(Vpcomd          , "vpcomd"          , Enc(VexRvmi)           , V(XOP_M8,CE,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6410, 415),
  INST(Vpcompressd     , "vpcompressd"     , Enc(VexMr_Lx)          , V(660F38,8B,_,x,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6417, 290),
  INST(Vpcompressq     , "vpcompressq"     , Enc(VexMr_Lx)          , V(660F38,8B,_,x,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6429, 290),
  INST(Vpcomq          , "vpcomq"          , Enc(VexRvmi)           , V(XOP_M8,CF,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6441, 415),
  INST(Vpcomub         , "vpcomub"         , Enc(VexRvmi)           , V(XOP_M8,EC,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6448, 415),
  INST(Vpcomud         , "vpcomud"         , Enc(VexRvmi)           , V(XOP_M8,EE,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6456, 415),
  INST(Vpcomuq         , "vpcomuq"         , Enc(VexRvmi)           , V(XOP_M8,EF,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6464, 415),
  INST(Vpcomuw         , "vpcomuw"         , Enc(VexRvmi)           , V(XOP_M8,ED,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6472, 415),
  INST(Vpcomw          , "vpcomw"          , Enc(VexRvmi)           , V(XOP_M8,CD,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6480, 415),
  INST(Vpconflictd     , "vpconflictd"     , Enc(VexRm_Lx)          , V(660F38,C4,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(CDI ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 6487, 427),
  INST(Vpconflictq     , "vpconflictq"     , Enc(VexRm_Lx)          , V(660F38,C4,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(CDI ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 6499, 427),
  INST(Vperm2f128      , "vperm2f128"      , Enc(VexRvmi)           , V(660F3A,06,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6511, 428),
  INST(Vperm2i128      , "vperm2i128"      , Enc(VexRvmi)           , V(660F3A,46,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6522, 428),
  INST(Vpermb          , "vpermb"          , Enc(VexRvm_Lx)         , V(660F38,8D,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(VBMI,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6533, 429),
  INST(Vpermd          , "vpermd"          , Enc(VexRvm_Lx)         , V(660F38,36,_,x,0,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 6540, 430),
  INST(Vpermi2b        , "vpermi2b"        , Enc(VexRvm_Lx)         , V(660F38,75,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(VBMI,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6547, 429),
  INST(Vpermi2d        , "vpermi2d"        , Enc(VexRvm_Lx)         , V(660F38,76,_,x,_,0,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 6556, 431),
  INST(Vpermi2pd       , "vpermi2pd"       , Enc(VexRvm_Lx)         , V(660F38,77,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 6565, 269),
  INST(Vpermi2ps       , "vpermi2ps"       , Enc(VexRvm_Lx)         , V(660F38,77,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 6575, 268),
  INST(Vpermi2q        , "vpermi2q"        , Enc(VexRvm_Lx)         , V(660F38,76,_,x,_,1,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 6585, 432),
  INST(Vpermi2w        , "vpermi2w"        , Enc(VexRvm_Lx)         , V(660F38,75,_,x,_,1,4,FVM), 0                         , F(RW)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6594, 433),
  INST(Vpermil2pd      , "vpermil2pd"      , Enc(VexRvrmiRvmri_Lx)  , V(660F3A,49,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6603, 434),
  INST(Vpermil2ps      , "vpermil2ps"      , Enc(VexRvrmiRvmri_Lx)  , V(660F3A,48,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6614, 434),
  INST(Vpermilpd       , "vpermilpd"       , Enc(VexRvmRmi_Lx)      , V(660F38,0D,_,x,0,1,4,FV ), V(660F3A,05,_,x,0,1,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 6625, 435),
  INST(Vpermilps       , "vpermilps"       , Enc(VexRvmRmi_Lx)      , V(660F38,0C,_,x,0,0,4,FV ), V(660F3A,04,_,x,0,0,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 6635, 436),
  INST(Vpermpd         , "vpermpd"         , Enc(VexRmi)            , V(660F3A,01,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6645, 437),
  INST(Vpermps         , "vpermps"         , Enc(VexRvm)            , V(660F38,16,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6653, 438),
  INST(Vpermq          , "vpermq"          , Enc(VexRvmRmi_Lx)      , V(660F38,36,_,x,_,1,4,FV ), V(660F3A,00,_,x,1,1,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 6661, 439),
  INST(Vpermt2b        , "vpermt2b"        , Enc(VexRvm_Lx)         , V(660F38,7D,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(VBMI,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6668, 429),
  INST(Vpermt2d        , "vpermt2d"        , Enc(VexRvm_Lx)         , V(660F38,7E,_,x,_,0,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 6677, 431),
  INST(Vpermt2pd       , "vpermt2pd"       , Enc(VexRvm_Lx)         , V(660F38,7F,_,x,_,1,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 6686, 432),
  INST(Vpermt2ps       , "vpermt2ps"       , Enc(VexRvm_Lx)         , V(660F38,7F,_,x,_,0,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 6696, 431),
  INST(Vpermt2q        , "vpermt2q"        , Enc(VexRvm_Lx)         , V(660F38,7E,_,x,_,1,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 6706, 432),
  INST(Vpermt2w        , "vpermt2w"        , Enc(VexRvm_Lx)         , V(660F38,7D,_,x,_,1,4,FVM), 0                         , F(RW)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6715, 433),
  INST(Vpermw          , "vpermw"          , Enc(VexRvm_Lx)         , V(660F38,8D,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6724, 267),
  INST(Vpexpandd       , "vpexpandd"       , Enc(VexRm_Lx)          , V(660F38,89,_,x,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6731, 325),
  INST(Vpexpandq       , "vpexpandq"       , Enc(VexRm_Lx)          , V(660F38,89,_,x,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6741, 325),
  INST(Vpextrb         , "vpextrb"         , Enc(VexMri)            , V(660F3A,14,_,0,0,I,0,T1S), 0                         , F(WO)|F(Vex)   |A512(BW  ,0,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6751, 440),
  INST(Vpextrd         , "vpextrd"         , Enc(VexMri)            , V(660F3A,16,_,0,0,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(DQ  ,0,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6759, 441),
  INST(Vpextrq         , "vpextrq"         , Enc(VexMri)            , V(660F3A,16,_,0,1,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(DQ  ,0,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6767, 442),
  INST(Vpextrw         , "vpextrw"         , Enc(VexMri)            , V(660F3A,15,_,0,0,I,1,T1S), 0                         , F(WO)|F(Vex)   |A512(BW  ,0,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6775, 443),
  INST(Vpgatherdd      , "vpgatherdd"      , Enc(VexRmvRm_VM)       , V(660F38,90,_,x,0,_,_,_  ), V(660F38,90,_,x,_,0,2,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6783, 444),
  INST(Vpgatherdq      , "vpgatherdq"      , Enc(VexRmvRm_VM)       , V(660F38,90,_,x,1,_,_,_  ), V(660F38,90,_,x,_,1,3,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6794, 445),
  INST(Vpgatherqd      , "vpgatherqd"      , Enc(VexRmvRm_VM)       , V(660F38,91,_,x,0,_,_,_  ), V(660F38,91,_,x,_,0,2,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6805, 446),
  INST(Vpgatherqq      , "vpgatherqq"      , Enc(VexRmvRm_VM)       , V(660F38,91,_,x,1,_,_,_  ), V(660F38,91,_,x,_,1,3,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 6816, 447),
  INST(Vphaddbd        , "vphaddbd"        , Enc(VexRm)             , V(XOP_M9,C2,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6827, 261),
  INST(Vphaddbq        , "vphaddbq"        , Enc(VexRm)             , V(XOP_M9,C3,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6836, 261),
  INST(Vphaddbw        , "vphaddbw"        , Enc(VexRm)             , V(XOP_M9,C1,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6845, 261),
  INST(Vphaddd         , "vphaddd"         , Enc(VexRvm_Lx)         , V(660F38,02,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6854, 259),
  INST(Vphadddq        , "vphadddq"        , Enc(VexRm)             , V(XOP_M9,CB,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6862, 261),
  INST(Vphaddsw        , "vphaddsw"        , Enc(VexRvm_Lx)         , V(660F38,03,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6871, 259),
  INST(Vphaddubd       , "vphaddubd"       , Enc(VexRm)             , V(XOP_M9,D2,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6880, 261),
  INST(Vphaddubq       , "vphaddubq"       , Enc(VexRm)             , V(XOP_M9,D3,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6890, 261),
  INST(Vphaddubw       , "vphaddubw"       , Enc(VexRm)             , V(XOP_M9,D1,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6900, 261),
  INST(Vphaddudq       , "vphaddudq"       , Enc(VexRm)             , V(XOP_M9,DB,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6910, 261),
  INST(Vphadduwd       , "vphadduwd"       , Enc(VexRm)             , V(XOP_M9,D6,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6920, 261),
  INST(Vphadduwq       , "vphadduwq"       , Enc(VexRm)             , V(XOP_M9,D7,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6930, 261),
  INST(Vphaddw         , "vphaddw"         , Enc(VexRvm_Lx)         , V(660F38,01,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6940, 259),
  INST(Vphaddwd        , "vphaddwd"        , Enc(VexRm)             , V(XOP_M9,C6,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6948, 261),
  INST(Vphaddwq        , "vphaddwq"        , Enc(VexRm)             , V(XOP_M9,C7,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6957, 261),
  INST(Vphminposuw     , "vphminposuw"     , Enc(VexRm)             , V(660F38,41,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6966, 261),
  INST(Vphsubbw        , "vphsubbw"        , Enc(VexRm)             , V(XOP_M9,E1,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6978, 261),
  INST(Vphsubd         , "vphsubd"         , Enc(VexRvm_Lx)         , V(660F38,06,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6987, 259),
  INST(Vphsubdq        , "vphsubdq"        , Enc(VexRm)             , V(XOP_M9,E3,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 6995, 261),
  INST(Vphsubsw        , "vphsubsw"        , Enc(VexRvm_Lx)         , V(660F38,07,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7004, 259),
  INST(Vphsubw         , "vphsubw"         , Enc(VexRvm_Lx)         , V(660F38,05,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7013, 259),
  INST(Vphsubwd        , "vphsubwd"        , Enc(VexRm)             , V(XOP_M9,E2,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7021, 261),
  INST(Vpinsrb         , "vpinsrb"         , Enc(VexRvmi)           , V(660F3A,20,_,0,0,I,0,T1S), 0                         , F(WO)|F(Vex)   |A512(BW  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7030, 448),
  INST(Vpinsrd         , "vpinsrd"         , Enc(VexRvmi)           , V(660F3A,22,_,0,0,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7038, 449),
  INST(Vpinsrq         , "vpinsrq"         , Enc(VexRvmi)           , V(660F3A,22,_,0,1,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7046, 450),
  INST(Vpinsrw         , "vpinsrw"         , Enc(VexRvmi)           , V(660F00,C4,_,0,0,I,1,T1S), 0                         , F(WO)|F(Vex)   |A512(BW  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7054, 451),
  INST(Vplzcntd        , "vplzcntd"        , Enc(VexRm_Lx)          , V(660F38,44,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(CDI ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 7062, 427),
  INST(Vplzcntq        , "vplzcntq"        , Enc(VexRm_Lx)          , V(660F38,44,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(CDI ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 7071, 452),
  INST(Vpmacsdd        , "vpmacsdd"        , Enc(VexRvmr)           , V(XOP_M8,9E,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7080, 453),
  INST(Vpmacsdqh       , "vpmacsdqh"       , Enc(VexRvmr)           , V(XOP_M8,9F,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7089, 453),
  INST(Vpmacsdql       , "vpmacsdql"       , Enc(VexRvmr)           , V(XOP_M8,97,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7099, 453),
  INST(Vpmacssdd       , "vpmacssdd"       , Enc(VexRvmr)           , V(XOP_M8,8E,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7109, 453),
  INST(Vpmacssdqh      , "vpmacssdqh"      , Enc(VexRvmr)           , V(XOP_M8,8F,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7119, 453),
  INST(Vpmacssdql      , "vpmacssdql"      , Enc(VexRvmr)           , V(XOP_M8,87,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7130, 453),
  INST(Vpmacsswd       , "vpmacsswd"       , Enc(VexRvmr)           , V(XOP_M8,86,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7141, 453),
  INST(Vpmacssww       , "vpmacssww"       , Enc(VexRvmr)           , V(XOP_M8,85,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7151, 453),
  INST(Vpmacswd        , "vpmacswd"        , Enc(VexRvmr)           , V(XOP_M8,96,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7161, 453),
  INST(Vpmacsww        , "vpmacsww"        , Enc(VexRvmr)           , V(XOP_M8,95,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7170, 453),
  INST(Vpmadcsswd      , "vpmadcsswd"      , Enc(VexRvmr)           , V(XOP_M8,A6,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7179, 453),
  INST(Vpmadcswd       , "vpmadcswd"       , Enc(VexRvmr)           , V(XOP_M8,B6,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7190, 453),
  INST(Vpmadd52huq     , "vpmadd52huq"     , Enc(VexRvm_Lx)         , V(660F38,B5,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(IFMA,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 7200, 454),
  INST(Vpmadd52luq     , "vpmadd52luq"     , Enc(VexRvm_Lx)         , V(660F38,B4,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(IFMA,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 7212, 454),
  INST(Vpmaddubsw      , "vpmaddubsw"      , Enc(VexRvm_Lx)         , V(660F38,04,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7224, 407),
  INST(Vpmaddwd        , "vpmaddwd"        , Enc(VexRvm_Lx)         , V(660F00,F5,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7235, 407),
  INST(Vpmaskmovd      , "vpmaskmovd"      , Enc(VexRvmMvr_Lx)      , V(660F38,8C,_,x,0,_,_,_  ), V(660F38,8E,_,x,0,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7244, 455),
  INST(Vpmaskmovq      , "vpmaskmovq"      , Enc(VexRvmMvr_Lx)      , V(660F38,8C,_,x,1,_,_,_  ), V(660F38,8E,_,x,1,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7255, 456),
  INST(Vpmaxsb         , "vpmaxsb"         , Enc(VexRvm_Lx)         , V(660F38,3C,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7266, 407),
  INST(Vpmaxsd         , "vpmaxsd"         , Enc(VexRvm_Lx)         , V(660F38,3D,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 7274, 404),
  INST(Vpmaxsq         , "vpmaxsq"         , Enc(VexRvm_Lx)         , V(660F38,3D,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 7282, 269),
  INST(Vpmaxsw         , "vpmaxsw"         , Enc(VexRvm_Lx)         , V(660F00,EE,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7290, 407),
  INST(Vpmaxub         , "vpmaxub"         , Enc(VexRvm_Lx)         , V(660F00,DE,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7298, 407),
  INST(Vpmaxud         , "vpmaxud"         , Enc(VexRvm_Lx)         , V(660F38,3F,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 7306, 404),
  INST(Vpmaxuq         , "vpmaxuq"         , Enc(VexRvm_Lx)         , V(660F38,3F,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 7314, 269),
  INST(Vpmaxuw         , "vpmaxuw"         , Enc(VexRvm_Lx)         , V(660F38,3E,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7322, 407),
  INST(Vpminsb         , "vpminsb"         , Enc(VexRvm_Lx)         , V(660F38,38,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7330, 407),
  INST(Vpminsd         , "vpminsd"         , Enc(VexRvm_Lx)         , V(660F38,39,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 7338, 404),
  INST(Vpminsq         , "vpminsq"         , Enc(VexRvm_Lx)         , V(660F38,39,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 7346, 269),
  INST(Vpminsw         , "vpminsw"         , Enc(VexRvm_Lx)         , V(660F00,EA,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7354, 407),
  INST(Vpminub         , "vpminub"         , Enc(VexRvm_Lx)         , V(660F00,DA,_,x,I,_,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7362, 407),
  INST(Vpminud         , "vpminud"         , Enc(VexRvm_Lx)         , V(660F38,3B,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 7370, 404),
  INST(Vpminuq         , "vpminuq"         , Enc(VexRvm_Lx)         , V(660F38,3B,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 7378, 269),
  INST(Vpminuw         , "vpminuw"         , Enc(VexRvm_Lx)         , V(660F38,3A,_,x,I,_,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7386, 407),
  INST(Vpmovb2m        , "vpmovb2m"        , Enc(VexRm_Lx)          , V(F30F38,29,_,x,_,0,_,_  ), 0                         , F(WO)          |A512(BW  ,1,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7394, 457),
  INST(Vpmovd2m        , "vpmovd2m"        , Enc(VexRm_Lx)          , V(F30F38,39,_,x,_,0,_,_  ), 0                         , F(WO)          |A512(DQ  ,1,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7403, 458),
  INST(Vpmovdb         , "vpmovdb"         , Enc(VexMr_Lx)          , V(F30F38,31,_,x,_,0,2,QVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7412, 459),
  INST(Vpmovdw         , "vpmovdw"         , Enc(VexMr_Lx)          , V(F30F38,33,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7420, 460),
  INST(Vpmovm2b        , "vpmovm2b"        , Enc(VexRm_Lx)          , V(F30F38,28,_,x,_,0,_,_  ), 0                         , F(WO)          |A512(BW  ,1,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7428, 461),
  INST(Vpmovm2d        , "vpmovm2d"        , Enc(VexRm_Lx)          , V(F30F38,38,_,x,_,0,_,_  ), 0                         , F(WO)          |A512(DQ  ,1,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7437, 462),
  INST(Vpmovm2q        , "vpmovm2q"        , Enc(VexRm_Lx)          , V(F30F38,38,_,x,_,1,_,_  ), 0                         , F(WO)          |A512(DQ  ,1,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7446, 462),
  INST(Vpmovm2w        , "vpmovm2w"        , Enc(VexRm_Lx)          , V(F30F38,28,_,x,_,1,_,_  ), 0                         , F(WO)          |A512(BW  ,1,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7455, 461),
  INST(Vpmovmskb       , "vpmovmskb"       , Enc(VexRm_Lx)          , V(660F00,D7,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7464, 395),
  INST(Vpmovq2m        , "vpmovq2m"        , Enc(VexRm_Lx)          , V(F30F38,39,_,x,_,1,_,_  ), 0                         , F(WO)          |A512(DQ  ,1,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7474, 458),
  INST(Vpmovqb         , "vpmovqb"         , Enc(VexMr_Lx)          , V(F30F38,32,_,x,_,0,1,OVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7483, 463),
  INST(Vpmovqd         , "vpmovqd"         , Enc(VexMr_Lx)          , V(F30F38,35,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7491, 460),
  INST(Vpmovqw         , "vpmovqw"         , Enc(VexMr_Lx)          , V(F30F38,34,_,x,_,0,2,QVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7499, 459),
  INST(Vpmovsdb        , "vpmovsdb"        , Enc(VexMr_Lx)          , V(F30F38,21,_,x,_,0,2,QVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7507, 459),
  INST(Vpmovsdw        , "vpmovsdw"        , Enc(VexMr_Lx)          , V(F30F38,23,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7516, 460),
  INST(Vpmovsqb        , "vpmovsqb"        , Enc(VexMr_Lx)          , V(F30F38,22,_,x,_,0,1,OVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7525, 463),
  INST(Vpmovsqd        , "vpmovsqd"        , Enc(VexMr_Lx)          , V(F30F38,25,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7534, 460),
  INST(Vpmovsqw        , "vpmovsqw"        , Enc(VexMr_Lx)          , V(F30F38,24,_,x,_,0,2,QVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7543, 459),
  INST(Vpmovswb        , "vpmovswb"        , Enc(VexMr_Lx)          , V(F30F38,20,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7552, 464),
  INST(Vpmovsxbd       , "vpmovsxbd"       , Enc(VexRm_Lx)          , V(660F38,21,_,x,I,I,2,QVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7561, 465),
  INST(Vpmovsxbq       , "vpmovsxbq"       , Enc(VexRm_Lx)          , V(660F38,22,_,x,I,I,1,OVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7571, 466),
  INST(Vpmovsxbw       , "vpmovsxbw"       , Enc(VexRm_Lx)          , V(660F38,20,_,x,I,I,3,HVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7581, 467),
  INST(Vpmovsxdq       , "vpmovsxdq"       , Enc(VexRm_Lx)          , V(660F38,25,_,x,I,0,3,HVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7591, 468),
  INST(Vpmovsxwd       , "vpmovsxwd"       , Enc(VexRm_Lx)          , V(660F38,23,_,x,I,I,3,HVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7601, 469),
  INST(Vpmovsxwq       , "vpmovsxwq"       , Enc(VexRm_Lx)          , V(660F38,24,_,x,I,I,2,QVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7611, 465),
  INST(Vpmovusdb       , "vpmovusdb"       , Enc(VexMr_Lx)          , V(F30F38,11,_,x,_,0,2,QVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7621, 459),
  INST(Vpmovusdw       , "vpmovusdw"       , Enc(VexMr_Lx)          , V(F30F38,13,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7631, 460),
  INST(Vpmovusqb       , "vpmovusqb"       , Enc(VexMr_Lx)          , V(F30F38,12,_,x,_,0,1,OVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7641, 463),
  INST(Vpmovusqd       , "vpmovusqd"       , Enc(VexMr_Lx)          , V(F30F38,15,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7651, 460),
  INST(Vpmovusqw       , "vpmovusqw"       , Enc(VexMr_Lx)          , V(F30F38,14,_,x,_,0,2,QVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7661, 459),
  INST(Vpmovuswb       , "vpmovuswb"       , Enc(VexMr_Lx)          , V(F30F38,10,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7671, 464),
  INST(Vpmovw2m        , "vpmovw2m"        , Enc(VexRm_Lx)          , V(F30F38,29,_,x,_,1,_,_  ), 0                         , F(WO)          |A512(BW  ,1,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7681, 457),
  INST(Vpmovwb         , "vpmovwb"         , Enc(VexMr_Lx)          , V(F30F38,30,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7690, 464),
  INST(Vpmovzxbd       , "vpmovzxbd"       , Enc(VexRm_Lx)          , V(660F38,31,_,x,I,I,2,QVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7698, 465),
  INST(Vpmovzxbq       , "vpmovzxbq"       , Enc(VexRm_Lx)          , V(660F38,32,_,x,I,I,1,OVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7708, 466),
  INST(Vpmovzxbw       , "vpmovzxbw"       , Enc(VexRm_Lx)          , V(660F38,30,_,x,I,I,3,HVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7718, 467),
  INST(Vpmovzxdq       , "vpmovzxdq"       , Enc(VexRm_Lx)          , V(660F38,35,_,x,I,0,3,HVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7728, 468),
  INST(Vpmovzxwd       , "vpmovzxwd"       , Enc(VexRm_Lx)          , V(660F38,33,_,x,I,I,3,HVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7738, 469),
  INST(Vpmovzxwq       , "vpmovzxwq"       , Enc(VexRm_Lx)          , V(660F38,34,_,x,I,I,2,QVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7748, 465),
  INST(Vpmuldq         , "vpmuldq"         , Enc(VexRvm_Lx)         , V(660F38,28,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 7758, 408),
  INST(Vpmulhrsw       , "vpmulhrsw"       , Enc(VexRvm_Lx)         , V(660F38,0B,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7766, 407),
  INST(Vpmulhuw        , "vpmulhuw"        , Enc(VexRvm_Lx)         , V(660F00,E4,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7776, 407),
  INST(Vpmulhw         , "vpmulhw"         , Enc(VexRvm_Lx)         , V(660F00,E5,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7785, 407),
  INST(Vpmulld         , "vpmulld"         , Enc(VexRvm_Lx)         , V(660F38,40,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 7793, 404),
  INST(Vpmullq         , "vpmullq"         , Enc(VexRvm_Lx)         , V(660F38,40,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 7801, 470),
  INST(Vpmullw         , "vpmullw"         , Enc(VexRvm_Lx)         , V(660F00,D5,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7809, 407),
  INST(Vpmultishiftqb  , "vpmultishiftqb"  , Enc(VexRvm_Lx)         , V(660F38,83,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(VBMI,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 7817, 471),
  INST(Vpmuludq        , "vpmuludq"        , Enc(VexRvm_Lx)         , V(660F00,F4,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 7832, 408),
  INST(Vpor            , "vpor"            , Enc(VexRvm_Lx)         , V(660F00,EB,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7841, 259),
  INST(Vpord           , "vpord"           , Enc(VexRvm_Lx)         , V(660F00,EB,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 7846, 268),
  INST(Vporq           , "vporq"           , Enc(VexRvm_Lx)         , V(660F00,EB,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 7852, 269),
  INST(Vpperm          , "vpperm"          , Enc(VexRvrmRvmr)       , V(XOP_M8,A3,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7858, 472),
  INST(Vprold          , "vprold"          , Enc(VexVmi_Lx)         , V(660F00,72,1,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 7865, 473),
  INST(Vprolq          , "vprolq"          , Enc(VexVmi_Lx)         , V(660F00,72,1,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 7872, 474),
  INST(Vprolvd         , "vprolvd"         , Enc(VexRvm_Lx)         , V(660F38,15,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 7879, 268),
  INST(Vprolvq         , "vprolvq"         , Enc(VexRvm_Lx)         , V(660F38,15,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 7887, 269),
  INST(Vprord          , "vprord"          , Enc(VexVmi_Lx)         , V(660F00,72,0,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 7895, 473),
  INST(Vprorq          , "vprorq"          , Enc(VexVmi_Lx)         , V(660F00,72,0,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 7902, 474),
  INST(Vprorvd         , "vprorvd"         , Enc(VexRvm_Lx)         , V(660F38,14,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 7909, 268),
  INST(Vprorvq         , "vprorvq"         , Enc(VexRvm_Lx)         , V(660F38,14,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 7917, 269),
  INST(Vprotb          , "vprotb"          , Enc(VexRvmRmvRmi)      , V(XOP_M9,90,_,0,x,_,_,_  ), V(XOP_M8,C0,_,0,x,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7925, 475),
  INST(Vprotd          , "vprotd"          , Enc(VexRvmRmvRmi)      , V(XOP_M9,92,_,0,x,_,_,_  ), V(XOP_M8,C2,_,0,x,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7932, 476),
  INST(Vprotq          , "vprotq"          , Enc(VexRvmRmvRmi)      , V(XOP_M9,93,_,0,x,_,_,_  ), V(XOP_M8,C3,_,0,x,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7939, 477),
  INST(Vprotw          , "vprotw"          , Enc(VexRvmRmvRmi)      , V(XOP_M9,91,_,0,x,_,_,_  ), V(XOP_M8,C1,_,0,x,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 7946, 478),
  INST(Vpsadbw         , "vpsadbw"         , Enc(VexRvm_Lx)         , V(660F00,F6,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7953, 479),
  INST(Vpscatterdd     , "vpscatterdd"     , Enc(VexMr_VM)          , V(660F38,A0,_,x,_,0,2,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7961, 480),
  INST(Vpscatterdq     , "vpscatterdq"     , Enc(VexMr_VM)          , V(660F38,A0,_,x,_,1,3,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7973, 480),
  INST(Vpscatterqd     , "vpscatterqd"     , Enc(VexMr_VM)          , V(660F38,A1,_,x,_,0,2,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7985, 481),
  INST(Vpscatterqq     , "vpscatterqq"     , Enc(VexMr_VM)          , V(660F38,A1,_,x,_,1,3,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 7997, 482),
  INST(Vpshab          , "vpshab"          , Enc(VexRvmRmv)         , V(XOP_M9,98,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8009, 483),
  INST(Vpshad          , "vpshad"          , Enc(VexRvmRmv)         , V(XOP_M9,9A,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8016, 483),
  INST(Vpshaq          , "vpshaq"          , Enc(VexRvmRmv)         , V(XOP_M9,9B,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8023, 483),
  INST(Vpshaw          , "vpshaw"          , Enc(VexRvmRmv)         , V(XOP_M9,99,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8030, 483),
  INST(Vpshlb          , "vpshlb"          , Enc(VexRvmRmv)         , V(XOP_M9,94,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8037, 483),
  INST(Vpshld          , "vpshld"          , Enc(VexRvmRmv)         , V(XOP_M9,96,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8044, 483),
  INST(Vpshlq          , "vpshlq"          , Enc(VexRvmRmv)         , V(XOP_M9,97,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8051, 483),
  INST(Vpshlw          , "vpshlw"          , Enc(VexRvmRmv)         , V(XOP_M9,95,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8058, 483),
  INST(Vpshufb         , "vpshufb"         , Enc(VexRvm_Lx)         , V(660F38,00,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8065, 407),
  INST(Vpshufd         , "vpshufd"         , Enc(VexRmi_Lx)         , V(660F00,70,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8073, 484),
  INST(Vpshufhw        , "vpshufhw"        , Enc(VexRmi_Lx)         , V(F30F00,70,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8081, 485),
  INST(Vpshuflw        , "vpshuflw"        , Enc(VexRmi_Lx)         , V(F20F00,70,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8090, 485),
  INST(Vpsignb         , "vpsignb"         , Enc(VexRvm_Lx)         , V(660F38,08,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8099, 259),
  INST(Vpsignd         , "vpsignd"         , Enc(VexRvm_Lx)         , V(660F38,0A,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8107, 259),
  INST(Vpsignw         , "vpsignw"         , Enc(VexRvm_Lx)         , V(660F38,09,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8115, 259),
  INST(Vpslld          , "vpslld"          , Enc(VexRvmVmi_Lx)      , V(660F00,F2,_,x,I,0,4,128), V(660F00,72,6,x,I,0,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8123, 486),
  INST(Vpslldq         , "vpslldq"         , Enc(VexEvexVmi_Lx)     , V(660F00,73,7,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8130, 487),
  INST(Vpsllq          , "vpsllq"          , Enc(VexRvmVmi_Lx)      , V(660F00,F3,_,x,I,1,4,128), V(660F00,73,6,x,I,1,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8138, 488),
  INST(Vpsllvd         , "vpsllvd"         , Enc(VexRvm_Lx)         , V(660F38,47,_,x,0,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8145, 404),
  INST(Vpsllvq         , "vpsllvq"         , Enc(VexRvm_Lx)         , V(660F38,47,_,x,1,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8153, 408),
  INST(Vpsllvw         , "vpsllvw"         , Enc(VexRvm_Lx)         , V(660F38,12,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8161, 267),
  INST(Vpsllw          , "vpsllw"          , Enc(VexRvmVmi_Lx)      , V(660F00,F1,_,x,I,I,4,FVM), V(660F00,71,6,x,I,I,4,FVM), F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8169, 489),
  INST(Vpsrad          , "vpsrad"          , Enc(VexRvmVmi_Lx)      , V(660F00,E2,_,x,I,0,4,128), V(660F00,72,4,x,I,0,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8176, 490),
  INST(Vpsraq          , "vpsraq"          , Enc(VexRvmVmi_Lx)      , V(660F00,E2,_,x,_,1,4,128), V(660F00,72,4,x,_,1,4,FV ), F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8183, 491),
  INST(Vpsravd         , "vpsravd"         , Enc(VexRvm_Lx)         , V(660F38,46,_,x,0,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8190, 404),
  INST(Vpsravq         , "vpsravq"         , Enc(VexRvm_Lx)         , V(660F38,46,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8198, 269),
  INST(Vpsravw         , "vpsravw"         , Enc(VexRvm_Lx)         , V(660F38,11,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8206, 267),
  INST(Vpsraw          , "vpsraw"          , Enc(VexRvmVmi_Lx)      , V(660F00,E1,_,x,I,I,4,128), V(660F00,71,4,x,I,I,4,FVM), F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8214, 492),
  INST(Vpsrld          , "vpsrld"          , Enc(VexRvmVmi_Lx)      , V(660F00,D2,_,x,I,0,4,128), V(660F00,72,2,x,I,0,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8221, 493),
  INST(Vpsrldq         , "vpsrldq"         , Enc(VexEvexVmi_Lx)     , V(660F00,73,3,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,0 ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8228, 487),
  INST(Vpsrlq          , "vpsrlq"          , Enc(VexRvmVmi_Lx)      , V(660F00,D3,_,x,I,1,4,128), V(660F00,73,2,x,I,1,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8236, 494),
  INST(Vpsrlvd         , "vpsrlvd"         , Enc(VexRvm_Lx)         , V(660F38,45,_,x,0,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8243, 404),
  INST(Vpsrlvq         , "vpsrlvq"         , Enc(VexRvm_Lx)         , V(660F38,45,_,x,1,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8251, 408),
  INST(Vpsrlvw         , "vpsrlvw"         , Enc(VexRvm_Lx)         , V(660F38,10,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8259, 267),
  INST(Vpsrlw          , "vpsrlw"          , Enc(VexRvmVmi_Lx)      , V(660F00,D1,_,x,I,I,4,128), V(660F00,71,2,x,I,I,4,FVM), F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8267, 495),
  INST(Vpsubb          , "vpsubb"          , Enc(VexRvm_Lx)         , V(660F00,F8,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8274, 407),
  INST(Vpsubd          , "vpsubd"          , Enc(VexRvm_Lx)         , V(660F00,FA,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8281, 404),
  INST(Vpsubq          , "vpsubq"          , Enc(VexRvm_Lx)         , V(660F00,FB,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8288, 408),
  INST(Vpsubsb         , "vpsubsb"         , Enc(VexRvm_Lx)         , V(660F00,E8,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8295, 407),
  INST(Vpsubsw         , "vpsubsw"         , Enc(VexRvm_Lx)         , V(660F00,E9,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8303, 407),
  INST(Vpsubusb        , "vpsubusb"        , Enc(VexRvm_Lx)         , V(660F00,D8,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8311, 407),
  INST(Vpsubusw        , "vpsubusw"        , Enc(VexRvm_Lx)         , V(660F00,D9,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8320, 407),
  INST(Vpsubw          , "vpsubw"          , Enc(VexRvm_Lx)         , V(660F00,F9,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8329, 407),
  INST(Vpternlogd      , "vpternlogd"      , Enc(VexRvmi_Lx)        , V(660F3A,25,_,x,_,0,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8336, 496),
  INST(Vpternlogq      , "vpternlogq"      , Enc(VexRvmi_Lx)        , V(660F3A,25,_,x,_,1,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8347, 497),
  INST(Vptest          , "vptest"          , Enc(VexRm_Lx)          , V(660F38,17,_,x,I,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 8358, 498),
  INST(Vptestmb        , "vptestmb"        , Enc(VexRvm_Lx)         , V(660F38,26,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8365, 499),
  INST(Vptestmd        , "vptestmd"        , Enc(VexRvm_Lx)         , V(660F38,27,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8374, 500),
  INST(Vptestmq        , "vptestmq"        , Enc(VexRvm_Lx)         , V(660F38,27,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8383, 501),
  INST(Vptestmw        , "vptestmw"        , Enc(VexRvm_Lx)         , V(660F38,26,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8392, 499),
  INST(Vptestnmb       , "vptestnmb"       , Enc(VexRvm_Lx)         , V(F30F38,26,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8401, 499),
  INST(Vptestnmd       , "vptestnmd"       , Enc(VexRvm_Lx)         , V(F30F38,27,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8411, 500),
  INST(Vptestnmq       , "vptestnmq"       , Enc(VexRvm_Lx)         , V(F30F38,27,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8421, 501),
  INST(Vptestnmw       , "vptestnmw"       , Enc(VexRvm_Lx)         , V(F30F38,26,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8431, 499),
  INST(Vpunpckhbw      , "vpunpckhbw"      , Enc(VexRvm_Lx)         , V(660F00,68,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8441, 407),
  INST(Vpunpckhdq      , "vpunpckhdq"      , Enc(VexRvm_Lx)         , V(660F00,6A,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8452, 404),
  INST(Vpunpckhqdq     , "vpunpckhqdq"     , Enc(VexRvm_Lx)         , V(660F00,6D,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8463, 408),
  INST(Vpunpckhwd      , "vpunpckhwd"      , Enc(VexRvm_Lx)         , V(660F00,69,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8475, 407),
  INST(Vpunpcklbw      , "vpunpcklbw"      , Enc(VexRvm_Lx)         , V(660F00,60,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8486, 407),
  INST(Vpunpckldq      , "vpunpckldq"      , Enc(VexRvm_Lx)         , V(660F00,62,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8497, 404),
  INST(Vpunpcklqdq     , "vpunpcklqdq"     , Enc(VexRvm_Lx)         , V(660F00,6C,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8508, 408),
  INST(Vpunpcklwd      , "vpunpcklwd"      , Enc(VexRvm_Lx)         , V(660F00,61,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8520, 407),
  INST(Vpxor           , "vpxor"           , Enc(VexRvm_Lx)         , V(660F00,EF,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8531, 259),
  INST(Vpxord          , "vpxord"          , Enc(VexRvm_Lx)         , V(660F00,EF,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8537, 268),
  INST(Vpxorq          , "vpxorq"          , Enc(VexRvm_Lx)         , V(660F00,EF,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8544, 269),
  INST(Vrangepd        , "vrangepd"        , Enc(VexRvmi_Lx)        , V(660F3A,50,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,SAE,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8551, 502),
  INST(Vrangeps        , "vrangeps"        , Enc(VexRvmi_Lx)        , V(660F3A,50,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,SAE,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8560, 503),
  INST(Vrangesd        , "vrangesd"        , Enc(VexRvmi)           , V(660F3A,51,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(DQ  ,0,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8569, 504),
  INST(Vrangess        , "vrangess"        , Enc(VexRvmi)           , V(660F3A,51,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(DQ  ,0,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8578, 505),
  INST(Vrcp14pd        , "vrcp14pd"        , Enc(VexRm_Lx)          , V(660F38,4C,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8587, 506),
  INST(Vrcp14ps        , "vrcp14ps"        , Enc(VexRm_Lx)          , V(660F38,4C,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8596, 507),
  INST(Vrcp14sd        , "vrcp14sd"        , Enc(VexRvm)            , V(660F38,4D,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8605, 508),
  INST(Vrcp14ss        , "vrcp14ss"        , Enc(VexRvm)            , V(660F38,4D,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8614, 509),
  INST(Vrcp28pd        , "vrcp28pd"        , Enc(VexRm)             , V(660F38,CA,_,2,_,1,4,FV ), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8623, 323),
  INST(Vrcp28ps        , "vrcp28ps"        , Enc(VexRm)             , V(660F38,CA,_,2,_,0,4,FV ), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8632, 324),
  INST(Vrcp28sd        , "vrcp28sd"        , Enc(VexRvm)            , V(660F38,CB,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8641, 510),
  INST(Vrcp28ss        , "vrcp28ss"        , Enc(VexRvm)            , V(660F38,CB,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8650, 511),
  INST(Vrcpps          , "vrcpps"          , Enc(VexRm_Lx)          , V(000F00,53,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8659, 347),
  INST(Vrcpss          , "vrcpss"          , Enc(VexRvm)            , V(F30F00,53,_,I,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8666, 512),
  INST(Vreducepd       , "vreducepd"       , Enc(VexRmi_Lx)         , V(660F3A,56,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8673, 513),
  INST(Vreduceps       , "vreduceps"       , Enc(VexRmi_Lx)         , V(660F3A,56,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8683, 514),
  INST(Vreducesd       , "vreducesd"       , Enc(VexRvmi)           , V(660F3A,57,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8693, 515),
  INST(Vreducess       , "vreducess"       , Enc(VexRvmi)           , V(660F3A,57,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8703, 516),
  INST(Vrndscalepd     , "vrndscalepd"     , Enc(VexRmi_Lx)         , V(660F3A,09,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8713, 359),
  INST(Vrndscaleps     , "vrndscaleps"     , Enc(VexRmi_Lx)         , V(660F3A,08,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8725, 360),
  INST(Vrndscalesd     , "vrndscalesd"     , Enc(VexRvmi)           , V(660F3A,0B,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8737, 517),
  INST(Vrndscaless     , "vrndscaless"     , Enc(VexRvmi)           , V(660F3A,0A,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8749, 518),
  INST(Vroundpd        , "vroundpd"        , Enc(VexRmi_Lx)         , V(660F3A,09,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8761, 519),
  INST(Vroundps        , "vroundps"        , Enc(VexRmi_Lx)         , V(660F3A,08,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8770, 519),
  INST(Vroundsd        , "vroundsd"        , Enc(VexRvmi)           , V(660F3A,0B,_,I,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8779, 520),
  INST(Vroundss        , "vroundss"        , Enc(VexRvmi)           , V(660F3A,0A,_,I,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8788, 521),
  INST(Vrsqrt14pd      , "vrsqrt14pd"      , Enc(VexRm_Lx)          , V(660F38,4E,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8797, 506),
  INST(Vrsqrt14ps      , "vrsqrt14ps"      , Enc(VexRm_Lx)          , V(660F38,4E,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8808, 507),
  INST(Vrsqrt14sd      , "vrsqrt14sd"      , Enc(VexRvm)            , V(660F38,4F,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8819, 508),
  INST(Vrsqrt14ss      , "vrsqrt14ss"      , Enc(VexRvm)            , V(660F38,4F,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8830, 509),
  INST(Vrsqrt28pd      , "vrsqrt28pd"      , Enc(VexRm)             , V(660F38,CC,_,2,_,1,4,FV ), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8841, 323),
  INST(Vrsqrt28ps      , "vrsqrt28ps"      , Enc(VexRm)             , V(660F38,CC,_,2,_,0,4,FV ), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8852, 324),
  INST(Vrsqrt28sd      , "vrsqrt28sd"      , Enc(VexRvm)            , V(660F38,CD,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8863, 510),
  INST(Vrsqrt28ss      , "vrsqrt28ss"      , Enc(VexRvm)            , V(660F38,CD,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8874, 511),
  INST(Vrsqrtps        , "vrsqrtps"        , Enc(VexRm_Lx)          , V(000F00,52,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8885, 347),
  INST(Vrsqrtss        , "vrsqrtss"        , Enc(VexRvm)            , V(F30F00,52,_,I,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , kFamilyNone, 0  , 8894, 512),
  INST(Vscalefpd       , "vscalefpd"       , Enc(VexRvm_Lx)         , V(660F38,2C,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 8903, 522),
  INST(Vscalefps       , "vscalefps"       , Enc(VexRvm_Lx)         , V(660F38,2C,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 8913, 523),
  INST(Vscalefsd       , "vscalefsd"       , Enc(VexRvm)            , V(660F38,2D,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8923, 524),
  INST(Vscalefss       , "vscalefss"       , Enc(VexRvm)            , V(660F38,2D,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8933, 525),
  INST(Vscatterdpd     , "vscatterdpd"     , Enc(VexMr_Lx)          , V(660F38,A2,_,x,_,1,3,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8943, 526),
  INST(Vscatterdps     , "vscatterdps"     , Enc(VexMr_Lx)          , V(660F38,A2,_,x,_,0,2,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8955, 480),
  INST(Vscatterpf0dpd  , "vscatterpf0dpd"  , Enc(VexM_VM)           , V(660F38,C6,5,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8967, 352),
  INST(Vscatterpf0dps  , "vscatterpf0dps"  , Enc(VexM_VM)           , V(660F38,C6,5,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8982, 353),
  INST(Vscatterpf0qpd  , "vscatterpf0qpd"  , Enc(VexM_VM)           , V(660F38,C7,5,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 8997, 354),
  INST(Vscatterpf0qps  , "vscatterpf0qps"  , Enc(VexM_VM)           , V(660F38,C7,5,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 9012, 354),
  INST(Vscatterpf1dpd  , "vscatterpf1dpd"  , Enc(VexM_VM)           , V(660F38,C6,6,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 9027, 352),
  INST(Vscatterpf1dps  , "vscatterpf1dps"  , Enc(VexM_VM)           , V(660F38,C6,6,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 9042, 353),
  INST(Vscatterpf1qpd  , "vscatterpf1qpd"  , Enc(VexM_VM)           , V(660F38,C7,6,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 9057, 354),
  INST(Vscatterpf1qps  , "vscatterpf1qps"  , Enc(VexM_VM)           , V(660F38,C7,6,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 9072, 354),
  INST(Vscatterqpd     , "vscatterqpd"     , Enc(VexMr_Lx)          , V(660F38,A3,_,x,_,1,3,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 9087, 482),
  INST(Vscatterqps     , "vscatterqps"     , Enc(VexMr_Lx)          , V(660F38,A3,_,x,_,0,2,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 9099, 481),
  INST(Vshuff32x4      , "vshuff32x4"      , Enc(VexRvmi_Lx)        , V(660F3A,23,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 9111, 527),
  INST(Vshuff64x2      , "vshuff64x2"      , Enc(VexRvmi_Lx)        , V(660F3A,23,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 9122, 528),
  INST(Vshufi32x4      , "vshufi32x4"      , Enc(VexRvmi_Lx)        , V(660F3A,43,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 9133, 527),
  INST(Vshufi64x2      , "vshufi64x2"      , Enc(VexRvmi_Lx)        , V(660F3A,43,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 9144, 528),
  INST(Vshufpd         , "vshufpd"         , Enc(VexRvmi_Lx)        , V(660F00,C6,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 9155, 529),
  INST(Vshufps         , "vshufps"         , Enc(VexRvmi_Lx)        , V(000F00,C6,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 9163, 530),
  INST(Vsqrtpd         , "vsqrtpd"         , Enc(VexRm_Lx)          , V(660F00,51,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 9171, 531),
  INST(Vsqrtps         , "vsqrtps"         , Enc(VexRm_Lx)          , V(000F00,51,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 9179, 292),
  INST(Vsqrtsd         , "vsqrtsd"         , Enc(VexRvm)            , V(F20F00,51,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 9187, 257),
  INST(Vsqrtss         , "vsqrtss"         , Enc(VexRvm)            , V(F30F00,51,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 9195, 258),
  INST(Vstmxcsr        , "vstmxcsr"        , Enc(VexM)              , V(000F00,AE,3,0,I,_,_,_  ), 0                         , F(Vex)|F(Volatile)                    , EF(________), 0 , 0 , kFamilyNone, 0  , 9203, 532),
  INST(Vsubpd          , "vsubpd"          , Enc(VexRvm_Lx)         , V(660F00,5C,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 9212, 255),
  INST(Vsubps          , "vsubps"          , Enc(VexRvm_Lx)         , V(000F00,5C,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 9219, 256),
  INST(Vsubsd          , "vsubsd"          , Enc(VexRvm)            , V(F20F00,5C,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 9226, 257),
  INST(Vsubss          , "vsubss"          , Enc(VexRvm)            , V(F30F00,5C,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , kFamilyNone, 0  , 9233, 258),
  INST(Vtestpd         , "vtestpd"         , Enc(VexRm_Lx)          , V(660F38,0F,_,x,0,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 9240, 498),
  INST(Vtestps         , "vtestps"         , Enc(VexRm_Lx)          , V(660F38,0E,_,x,0,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 9248, 498),
  INST(Vucomisd        , "vucomisd"        , Enc(VexRm)             , V(660F00,2E,_,I,I,1,3,T1S), 0                         , F(RO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 9256, 288),
  INST(Vucomiss        , "vucomiss"        , Enc(VexRm)             , V(000F00,2E,_,I,I,0,2,T1S), 0                         , F(RO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 9265, 289),
  INST(Vunpckhpd       , "vunpckhpd"       , Enc(VexRvm_Lx)         , V(660F00,15,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 9274, 408),
  INST(Vunpckhps       , "vunpckhps"       , Enc(VexRvm_Lx)         , V(000F00,15,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 9284, 404),
  INST(Vunpcklpd       , "vunpcklpd"       , Enc(VexRvm_Lx)         , V(660F00,14,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 9294, 408),
  INST(Vunpcklps       , "vunpcklps"       , Enc(VexRvm_Lx)         , V(000F00,14,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 9304, 404),
  INST(Vxorpd          , "vxorpd"          , Enc(VexRvm_Lx)         , V(660F00,57,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,8) , EF(________), 0 , 0 , kFamilyNone, 0  , 9314, 265),
  INST(Vxorps          , "vxorps"          , Enc(VexRvm_Lx)         , V(000F00,57,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,4) , EF(________), 0 , 0 , kFamilyNone, 0  , 9321, 266),
  INST(Vzeroall        , "vzeroall"        , Enc(VexOp)             , V(000F00,77,_,1,I,_,_,_  ), 0                         , F(Vex)|F(Volatile)                    , EF(________), 0 , 0 , kFamilyNone, 0  , 9328, 533),
  INST(Vzeroupper      , "vzeroupper"      , Enc(VexOp)             , V(000F00,77,_,0,I,_,_,_  ), 0                         , F(Vex)|F(Volatile)                    , EF(________), 0 , 0 , kFamilyNone, 0  , 9337, 533),
  INST(Wrfsbase        , "wrfsbase"        , Enc(X86M)              , O(F30F00,AE,2,_,x,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , kFamilyNone, 0  , 9348, 534),
  INST(Wrgsbase        , "wrgsbase"        , Enc(X86M)              , O(F30F00,AE,3,_,x,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , kFamilyNone, 0  , 9357, 534),
  INST(Xadd            , "xadd"            , Enc(X86Xadd)           , O(000F00,C0,_,_,x,_,_,_  ), 0                         , F(RW)|F(Xchg)|F(Lock)                 , EF(WWWWWW__), 0 , 0 , kFamilyNone, 0  , 9366, 535),
  INST(Xchg            , "xchg"            , Enc(X86Xchg)           , O(000000,86,_,_,x,_,_,_  ), 0                         , F(RW)|F(Xchg)|F(Lock)                 , EF(________), 0 , 0 , kFamilyNone, 0  , 353 , 536),
  INST(Xgetbv          , "xgetbv"          , Enc(X86Op)             , O(000F01,D0,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , kFamilyNone, 0  , 9371, 537),
  INST(Xor             , "xor"             , Enc(X86Arith)          , O(000000,30,6,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWUWW__), 0 , 0 , kFamilyNone, 0  , 8533, 3  ),
  INST(Xorpd           , "xorpd"           , Enc(ExtRm)             , O(660F00,57,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 83 , 9315, 4  ),
  INST(Xorps           , "xorps"           , Enc(ExtRm)             , O(000F00,57,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , kFamilySse , 83 , 9322, 4  ),
  INST(Xrstor          , "xrstor"          , Enc(X86M_Only)         , O(000F00,AE,5,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 1030, 538),
  INST(Xrstor64        , "xrstor64"        , Enc(X86M_Only)         , O(000F00,AE,5,_,1,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 1038, 539),
  INST(Xrstors         , "xrstors"         , Enc(X86M_Only)         , O(000F00,C7,3,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 9378, 538),
  INST(Xrstors64       , "xrstors64"       , Enc(X86M_Only)         , O(000F00,C7,3,_,1,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 9386, 539),
  INST(Xsave           , "xsave"           , Enc(X86M_Only)         , O(000F00,AE,4,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 1048, 540),
  INST(Xsave64         , "xsave64"         , Enc(X86M_Only)         , O(000F00,AE,4,_,1,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 1055, 541),
  INST(Xsavec          , "xsavec"          , Enc(X86M_Only)         , O(000F00,C7,4,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 9396, 540),
  INST(Xsavec64        , "xsavec64"        , Enc(X86M_Only)         , O(000F00,C7,4,_,1,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 9403, 541),
  INST(Xsaveopt        , "xsaveopt"        , Enc(X86M_Only)         , O(000F00,AE,6,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 9412, 540),
  INST(Xsaveopt64      , "xsaveopt64"      , Enc(X86M_Only)         , O(000F00,AE,6,_,1,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 9421, 541),
  INST(Xsaves          , "xsaves"          , Enc(X86M_Only)         , O(000F00,C7,5,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 9432, 540),
  INST(Xsaves64        , "xsaves64"        , Enc(X86M_Only)         , O(000F00,C7,5,_,1,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 9439, 541),
  INST(Xsetbv          , "xsetbv"          , Enc(X86Op)             , O(000F01,D1,_,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , kFamilyNone, 0  , 9448, 542)
  // ${instData:End}
};

// ${altOpCodeData:Begin}
// ------------------- Automatically generated, do not edit -------------------
const uint32_t X86InstDB::altOpCodeData[] = {
  0                         , // #0
  O(000F00,BA,4,_,x,_,_,_  ), // #1
  O(000F00,BA,7,_,x,_,_,_  ), // #2
  O(000F00,BA,6,_,x,_,_,_  ), // #3
  O(000F00,BA,5,_,x,_,_,_  ), // #4
  O(000000,48,_,_,x,_,_,_  ), // #5
  O(660F00,78,0,_,_,_,_,_  ), // #6
  O_FPU(00,00DF,5)          , // #7
  O_FPU(00,00DF,7)          , // #8
  O_FPU(00,00DD,1)          , // #9
  O_FPU(00,00DB,5)          , // #10
  O_FPU(00,DFE0,_)          , // #11
  O(000000,DB,7,_,_,_,_,_  ), // #12
  O_FPU(9B,DFE0,_)          , // #13
  O(000000,40,_,_,x,_,_,_  ), // #14
  O(F20F00,78,_,_,_,_,_,_  ), // #15
  O(000000,77,_,_,_,_,_,_  ), // #16
  O(000000,73,_,_,_,_,_,_  ), // #17
  O(000000,72,_,_,_,_,_,_  ), // #18
  O(000000,76,_,_,_,_,_,_  ), // #19
  O(000000,74,_,_,_,_,_,_  ), // #20
  O(000000,7F,_,_,_,_,_,_  ), // #21
  O(000000,7D,_,_,_,_,_,_  ), // #22
  O(000000,7C,_,_,_,_,_,_  ), // #23
  O(000000,7E,_,_,_,_,_,_  ), // #24
  O(000000,75,_,_,_,_,_,_  ), // #25
  O(000000,71,_,_,_,_,_,_  ), // #26
  O(000000,7B,_,_,_,_,_,_  ), // #27
  O(000000,79,_,_,_,_,_,_  ), // #28
  O(000000,70,_,_,_,_,_,_  ), // #29
  O(000000,7A,_,_,_,_,_,_  ), // #30
  O(000000,78,_,_,_,_,_,_  ), // #31
  O(000000,E3,_,_,_,_,_,_  ), // #32
  O(000000,EB,_,_,_,_,_,_  ), // #33
  V(660F00,92,_,0,0,_,_,_  ), // #34
  V(F20F00,92,_,0,0,_,_,_  ), // #35
  V(F20F00,92,_,0,1,_,_,_  ), // #36
  V(000F00,92,_,0,0,_,_,_  ), // #37
  O(660F00,29,_,_,_,_,_,_  ), // #38
  O(000F00,29,_,_,_,_,_,_  ), // #39
  O(000F38,F1,_,_,x,_,_,_  ), // #40
  O(000F00,7E,_,_,_,_,_,_  ), // #41
  O(660F00,7F,_,_,_,_,_,_  ), // #42
  O(F30F00,7F,_,_,_,_,_,_  ), // #43
  O(660F00,17,_,_,_,_,_,_  ), // #44
  O(000F00,17,_,_,_,_,_,_  ), // #45
  O(660F00,13,_,_,_,_,_,_  ), // #46
  O(000F00,13,_,_,_,_,_,_  ), // #47
  O(660F00,E7,_,_,_,_,_,_  ), // #48
  O(660F00,2B,_,_,_,_,_,_  ), // #49
  O(000F00,2B,_,_,_,_,_,_  ), // #50
  O(000F00,E7,_,_,_,_,_,_  ), // #51
  O(F20F00,2B,_,_,_,_,_,_  ), // #52
  O(F30F00,2B,_,_,_,_,_,_  ), // #53
  O(000F00,7E,_,_,x,_,_,_  ), // #54
  O(F20F00,11,_,_,_,_,_,_  ), // #55
  O(F30F00,11,_,_,_,_,_,_  ), // #56
  O(660F00,11,_,_,_,_,_,_  ), // #57
  O(000F00,11,_,_,_,_,_,_  ), // #58
  O(000F3A,15,_,_,_,_,_,_  ), // #59
  O(000000,58,_,_,_,_,_,_  ), // #60
  O(000F00,72,6,_,_,_,_,_  ), // #61
  O(660F00,73,7,_,_,_,_,_  ), // #62
  O(000F00,73,6,_,_,_,_,_  ), // #63
  O(000F00,71,6,_,_,_,_,_  ), // #64
  O(000F00,72,4,_,_,_,_,_  ), // #65
  O(000F00,71,4,_,_,_,_,_  ), // #66
  O(000F00,72,2,_,_,_,_,_  ), // #67
  O(660F00,73,3,_,_,_,_,_  ), // #68
  O(000F00,73,2,_,_,_,_,_  ), // #69
  O(000F00,71,2,_,_,_,_,_  ), // #70
  O(000000,50,_,_,_,_,_,_  ), // #71
  O(000000,F6,_,_,x,_,_,_  ), // #72
  V(660F38,92,_,x,_,1,3,T1S), // #73
  V(660F38,92,_,x,_,0,2,T1S), // #74
  V(660F38,93,_,x,_,1,3,T1S), // #75
  V(660F38,93,_,x,_,0,2,T1S), // #76
  V(660F38,2F,_,x,0,_,_,_  ), // #77
  V(660F38,2E,_,x,0,_,_,_  ), // #78
  V(660F00,29,_,x,I,1,4,FVM), // #79
  V(000F00,29,_,x,I,0,4,FVM), // #80
  V(660F00,7E,_,0,0,0,2,T1S), // #81
  V(660F00,7F,_,x,I,_,_,_  ), // #82
  V(660F00,7F,_,x,_,0,4,FVM), // #83
  V(660F00,7F,_,x,_,1,4,FVM), // #84
  V(F30F00,7F,_,x,I,_,_,_  ), // #85
  V(F20F00,7F,_,x,_,1,4,FVM), // #86
  V(F30F00,7F,_,x,_,0,4,FVM), // #87
  V(F30F00,7F,_,x,_,1,4,FVM), // #88
  V(F20F00,7F,_,x,_,0,4,FVM), // #89
  V(660F00,17,_,0,I,1,3,T1S), // #90
  V(000F00,17,_,0,I,0,3,T2 ), // #91
  V(660F00,13,_,0,I,1,3,T1S), // #92
  V(000F00,13,_,0,I,0,3,T2 ), // #93
  V(660F00,7E,_,0,I,1,3,T1S), // #94
  V(F20F00,11,_,I,I,1,3,T1S), // #95
  V(F30F00,11,_,I,I,0,2,T1S), // #96
  V(660F00,11,_,x,I,1,4,FVM), // #97
  V(000F00,11,_,x,I,0,4,FVM), // #98
  V(660F3A,05,_,x,0,1,4,FV ), // #99
  V(660F3A,04,_,x,0,0,4,FV ), // #100
  V(660F3A,00,_,x,1,1,4,FV ), // #101
  V(660F38,90,_,x,_,0,2,T1S), // #102
  V(660F38,90,_,x,_,1,3,T1S), // #103
  V(660F38,91,_,x,_,0,2,T1S), // #104
  V(660F38,91,_,x,_,1,3,T1S), // #105
  V(660F38,8E,_,x,0,_,_,_  ), // #106
  V(660F38,8E,_,x,1,_,_,_  ), // #107
  V(XOP_M8,C0,_,0,x,_,_,_  ), // #108
  V(XOP_M8,C2,_,0,x,_,_,_  ), // #109
  V(XOP_M8,C3,_,0,x,_,_,_  ), // #110
  V(XOP_M8,C1,_,0,x,_,_,_  ), // #111
  V(660F00,72,6,x,I,0,4,FV ), // #112
  V(660F00,73,6,x,I,1,4,FV ), // #113
  V(660F00,71,6,x,I,I,4,FVM), // #114
  V(660F00,72,4,x,I,0,4,FV ), // #115
  V(660F00,72,4,x,_,1,4,FV ), // #116
  V(660F00,71,4,x,I,I,4,FVM), // #117
  V(660F00,72,2,x,I,0,4,FV ), // #118
  V(660F00,73,2,x,I,1,4,FV ), // #119
  V(660F00,71,2,x,I,I,4,FVM)  // #120
};
// ----------------------------------------------------------------------------
// ${altOpCodeData:End}

// ${sseData:Begin}
// ------------------- Automatically generated, do not edit -------------------
const X86Inst::SseData X86InstDB::sseData[] = {
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 704  }, // #0
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 703  }, // #1
  { 0, X86Inst::SseData::kAvxConvMove          , 703  }, // #2
  { 0, X86Inst::SseData::kAvxConvBlend         , 703  }, // #3
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 661  }, // #4
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 657  }, // #5
  { 0, X86Inst::SseData::kAvxConvMove          , 654  }, // #6
  { 0, X86Inst::SseData::kAvxConvMove          , 653  }, // #7
  { 0, X86Inst::SseData::kAvxConvNone          , 0    }, // #8
  { 0, X86Inst::SseData::kAvxConvMove          , 652  }, // #9
  { 0, X86Inst::SseData::kAvxConvMove          , 659  }, // #10
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 659  }, // #11
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 660  }, // #12
  { 0, X86Inst::SseData::kAvxConvMove          , 660  }, // #13
  { 0, X86Inst::SseData::kAvxConvMove          , 661  }, // #14
  { 0, X86Inst::SseData::kAvxConvMove          , 663  }, // #15
  { 0, X86Inst::SseData::kAvxConvMove          , 665  }, // #16
  { 0, X86Inst::SseData::kAvxConvMove          , 666  }, // #17
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 668  }, // #18
  { 0, X86Inst::SseData::kAvxConvMove          , 680  }, // #19
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 693  }, // #20
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 700  }, // #21
  { 0, X86Inst::SseData::kAvxConvMove          , 612  }, // #22
  { 0, X86Inst::SseData::kAvxConvMove          , 604  }, // #23
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 605  }, // #24
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 604  }, // #25
  { 0, X86Inst::SseData::kAvxConvMove          , 602  }, // #26
  { 0, X86Inst::SseData::kAvxConvMove          , 601  }, // #27
  { 0, X86Inst::SseData::kAvxConvMove          , 600  }, // #28
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 606  }, // #29
  { 0, X86Inst::SseData::kAvxConvMoveIfMem     , 606  }, // #30
  { 0, X86Inst::SseData::kAvxConvMove          , 606  }, // #31
  { 0, X86Inst::SseData::kAvxConvMove          , 605  }, // #32
  { 0, X86Inst::SseData::kAvxConvMove          , 597  }, // #33
  { 0, X86Inst::SseData::kAvxConvMoveIfMem     , 597  }, // #34
  { 0, X86Inst::SseData::kAvxConvMove          , 595  }, // #35
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 594  }, // #36
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 593  }, // #37
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 587  }, // #38
  { 0, X86Inst::SseData::kAvxConvMove          , 587  }, // #39
  { 0, X86Inst::SseData::kAvxConvMove          , 588  }, // #40
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 588  }, // #41
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 589  }, // #42
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 591  }, // #43
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 590  }, // #44
  { 0, X86Inst::SseData::kAvxConvBlend         , 591  }, // #45
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 597  }, // #46
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 600  }, // #47
  { 0, X86Inst::SseData::kAvxConvMove          , 641  }, // #48
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 627  }, // #49
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 628  }, // #50
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 634  }, // #51
  { 0, X86Inst::SseData::kAvxConvMove          , 636  }, // #52
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 637  }, // #53
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 638  }, // #54
  { 0, X86Inst::SseData::kAvxConvMove          , 637  }, // #55
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 653  }, // #56
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 655  }, // #57
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 656  }, // #58
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 658  }, // #59
  { 0, X86Inst::SseData::kAvxConvMove          , 667  }, // #60
  { 0, X86Inst::SseData::kAvxConvMove          , 677  }, // #61
  { 0, X86Inst::SseData::kAvxConvMove          , 685  }, // #62
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 685  }, // #63
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 684  }, // #64
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 686  }, // #65
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 682  }, // #66
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 705  }, // #67
  { 0, X86Inst::SseData::kAvxConvMove          , 705  }, // #68
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 707  }, // #69
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 711  }, // #70
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 714  }, // #71
  { 0, X86Inst::SseData::kAvxConvMove          , 715  }, // #72
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 723  }, // #73
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 720  }, // #74
  { 0, X86Inst::SseData::kAvxConvMove          , 733  }, // #75
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 733  }, // #76
  { 0, X86Inst::SseData::kAvxConvMove          , 702  }, // #77
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 702  }, // #78
  { 0, X86Inst::SseData::kAvxConvMove          , 710  }, // #79
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 710  }, // #80
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 677  }, // #81
  { 0, X86Inst::SseData::kAvxConvNonDestructive, 665  }, // #82
  { 0, X86Inst::SseData::kAvxConvNonDestructive, -10  }  // #83
};
// ----------------------------------------------------------------------------
// ${sseData:End}

// ${commonData:Begin}
// ------------------- Automatically generated, do not edit -------------------
const X86Inst::CommonData X86InstDB::commonData[] = {
  { 0                                     , 0  , 0  , 0x00, 0x00, 0  , 0  , 0 , 0 }, // #0
  { F(RW)|F(Lock)                         , 0  , 0  , 0x20, 0x3F, 0  , 14 , 10, 0 }, // #1
  { F(RW)                                 , 0  , 0  , 0x20, 0x20, 0  , 22 , 2 , 0 }, // #2
  { F(RW)|F(Lock)                         , 0  , 0  , 0x00, 0x3F, 0  , 14 , 10, 0 }, // #3
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 0  , 289, 1 , 0 }, // #4
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 0  , 346, 1 , 0 }, // #5
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 0  , 347, 1 , 0 }, // #6
  { F(RW)                                 , 0  , 0  , 0x01, 0x01, 0  , 22 , 2 , 0 }, // #7
  { F(WO)                                 , 0  , 0  , 0x00, 0x00, 0  , 64 , 1 , 0 }, // #8
  { F(WO)                                 , 0  , 0  , 0x00, 0x00, 0  , 71 , 1 , 0 }, // #9
  { F(RW)                                 , 0  , 0  , 0x00, 0x3F, 0  , 246, 2 , 0 }, // #10
  { F(RW)                                 , 0  , 0  , 0x00, 0x3F, 0  , 248, 2 , 0 }, // #11
  { F(WO)                                 , 0  , 0  , 0x00, 0x3F, 0  , 154, 2 , 0 }, // #12
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 0  , 291, 1 , 0 }, // #13
  { F(RW)|F(Special)                      , 0  , 0  , 0x00, 0x00, 0  , 348, 1 , 0 }, // #14
  { F(RW)                                 , 0  , 0  , 0x00, 0x3F, 0  , 154, 2 , 0 }, // #15
  { F(RW)                                 , 0  , 0  , 0x00, 0x3F, 0  , 21 , 3 , 0 }, // #16
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 0  , 349, 1 , 0 }, // #17
  { F(RO)                                 , 0  , 0  , 0x00, 0x3B, 1  , 144, 3 , 0 }, // #18
  { F(RW)|F(Lock)                         , 0  , 0  , 0x00, 0x3B, 2  , 147, 3 , 0 }, // #19
  { F(RW)|F(Lock)                         , 0  , 0  , 0x00, 0x3B, 3  , 147, 3 , 0 }, // #20
  { F(RW)|F(Lock)                         , 0  , 0  , 0x00, 0x3B, 4  , 147, 3 , 0 }, // #21
  { F(RW)|F(Flow)|F(Volatile)             , 0  , 0  , 0x00, 0x00, 0  , 250, 2 , 0 }, // #22
  { F(RW)|F(Special)                      , 0  , 0  , 0x00, 0x00, 0  , 350, 1 , 0 }, // #23
  { F(RW)|F(Special)                      , 0  , 0  , 0x00, 0x00, 0  , 351, 1 , 0 }, // #24
  { F(RW)|F(Special)                      , 0  , 0  , 0x00, 0x00, 0  , 352, 1 , 0 }, // #25
  { F(Volatile)                           , 0  , 0  , 0x00, 0x08, 0  , 260, 1 , 0 }, // #26
  { F(Volatile)                           , 0  , 0  , 0x00, 0x20, 0  , 260, 1 , 0 }, // #27
  { F(Volatile)                           , 0  , 0  , 0x00, 0x40, 0  , 260, 1 , 0 }, // #28
  { F(RO)|F(Volatile)                     , 0  , 0  , 0x00, 0x00, 0  , 353, 1 , 0 }, // #29
  { F(WO)|F(Volatile)|F(Special)          , 0  , 0  , 0x00, 0x00, 0  , 354, 1 , 0 }, // #30
  { 0                                     , 0  , 0  , 0x20, 0x20, 0  , 260, 1 , 0 }, // #31
  { F(RW)                                 , 0  , 0  , 0x24, 0x00, 0  , 21 , 3 , 0 }, // #32
  { F(RW)                                 , 0  , 0  , 0x20, 0x00, 0  , 21 , 3 , 0 }, // #33
  { F(RW)                                 , 0  , 0  , 0x04, 0x00, 0  , 21 , 3 , 0 }, // #34
  { F(RW)                                 , 0  , 0  , 0x07, 0x00, 0  , 21 , 3 , 0 }, // #35
  { F(RW)                                 , 0  , 0  , 0x03, 0x00, 0  , 21 , 3 , 0 }, // #36
  { F(RW)                                 , 0  , 0  , 0x01, 0x00, 0  , 21 , 3 , 0 }, // #37
  { F(RW)                                 , 0  , 0  , 0x10, 0x00, 0  , 21 , 3 , 0 }, // #38
  { F(RW)                                 , 0  , 0  , 0x02, 0x00, 0  , 21 , 3 , 0 }, // #39
  { F(RO)                                 , 0  , 0  , 0x00, 0x3F, 0  , 24 , 10, 0 }, // #40
  { F(RW)|F(Volatile)|F(Special)          , 0  , 0  , 0x40, 0x3F, 0  , 0  , 0 , 0 }, // #41
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 0  , 252, 2 , 0 }, // #42
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 0  , 355, 1 , 0 }, // #43
  { F(RW)|F(Lock)|F(Special)              , 0  , 0  , 0x00, 0x3F, 0  , 108, 4 , 0 }, // #44
  { F(RW)|F(Lock)|F(Special)              , 0  , 0  , 0x00, 0x04, 0  , 356, 1 , 0 }, // #45
  { F(RW)|F(Lock)|F(Special)              , 0  , 0  , 0x00, 0x04, 0  , 357, 1 , 0 }, // #46
  { F(RO)                                 , 0  , 0  , 0x00, 0x3F, 0  , 358, 1 , 0 }, // #47
  { F(RO)                                 , 0  , 0  , 0x00, 0x3F, 0  , 359, 1 , 0 }, // #48
  { F(RW)|F(Volatile)|F(Special)          , 0  , 0  , 0x00, 0x00, 0  , 360, 1 , 0 }, // #49
  { F(RW)|F(Special)                      , 0  , 0  , 0x00, 0x00, 0  , 361, 1 , 0 }, // #50
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 0  , 254, 2 , 0 }, // #51
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 0  , 62 , 1 , 0 }, // #52
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 0  , 64 , 1 , 0 }, // #53
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 362, 1 , 0 }, // #54
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 0  , 363, 1 , 0 }, // #55
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 363, 1 , 0 }, // #56
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 364, 1 , 0 }, // #57
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 365, 1 , 0 }, // #58
  { F(WO)                                 , 0  , 4  , 0x00, 0x00, 0  , 62 , 1 , 0 }, // #59
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 366, 1 , 0 }, // #60
  { F(WO)                                 , 0  , 4  , 0x00, 0x00, 0  , 366, 1 , 0 }, // #61
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 228, 1 , 0 }, // #62
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 310, 1 , 0 }, // #63
  { F(RW)|F(Special)                      , 0  , 0  , 0x00, 0x00, 0  , 367, 1 , 0 }, // #64
  { F(RW)|F(Special)                      , 0  , 0  , 0x00, 0x00, 0  , 368, 1 , 0 }, // #65
  { F(RW)|F(Special)                      , 0  , 0  , 0x28, 0x3F, 0  , 369, 1 , 0 }, // #66
  { F(RW)|F(Lock)                         , 0  , 0  , 0x00, 0x1F, 5  , 256, 2 , 0 }, // #67
  { F(RW)|F(Special)                      , 0  , 0  , 0x00, 0x3F, 0  , 112, 4 , 0 }, // #68
  { F(Volatile)                           , 0  , 0  , 0x00, 0x00, 0  , 260, 1 , 0 }, // #69
  { F(Volatile)|F(Special)                , 0  , 0  , 0x00, 0x00, 0  , 370, 1 , 0 }, // #70
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 371, 1 , 0 }, // #71
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 6  , 258, 2 , 0 }, // #72
  { F(Fp)                                 , 0  , 0  , 0x00, 0x00, 0  , 260, 1 , 0 }, // #73
  { F(Fp)|F(FPU_M4)|F(FPU_M8)             , 0  , 0  , 0x00, 0x00, 0  , 150, 3 , 0 }, // #74
  { F(Fp)                                 , 0  , 0  , 0x00, 0x00, 0  , 260, 2 , 0 }, // #75
  { F(Fp)                                 , 0  , 0  , 0x00, 0x00, 0  , 372, 1 , 0 }, // #76
  { F(Fp)                                 , 0  , 0  , 0x20, 0x00, 0  , 261, 1 , 0 }, // #77
  { F(Fp)                                 , 0  , 0  , 0x24, 0x00, 0  , 261, 1 , 0 }, // #78
  { F(Fp)                                 , 0  , 0  , 0x04, 0x00, 0  , 261, 1 , 0 }, // #79
  { F(Fp)                                 , 0  , 0  , 0x10, 0x00, 0  , 261, 1 , 0 }, // #80
  { F(Fp)                                 , 0  , 0  , 0x00, 0x00, 0  , 262, 2 , 0 }, // #81
  { F(Fp)                                 , 0  , 0  , 0x00, 0x3F, 0  , 261, 1 , 0 }, // #82
  { F(Fp)                                 , 0  , 0  , 0x00, 0x00, 0  , 261, 1 , 0 }, // #83
  { F(Fp)|F(FPU_M2)|F(FPU_M4)             , 0  , 0  , 0x00, 0x00, 0  , 373, 1 , 0 }, // #84
  { F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , 0  , 0  , 0x00, 0x00, 7  , 374, 1 , 0 }, // #85
  { F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , 0  , 0  , 0x00, 0x00, 8  , 374, 1 , 0 }, // #86
  { F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , 0  , 0  , 0x00, 0x00, 9  , 374, 1 , 0 }, // #87
  { F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , 0  , 0  , 0x00, 0x00, 10 , 375, 1 , 0 }, // #88
  { F(Fp)                                 , 0  , 0  , 0x00, 0x00, 0  , 376, 1 , 0 }, // #89
  { F(Fp)                                 , 0  , 0  , 0x00, 0x00, 0  , 377, 1 , 0 }, // #90
  { F(Fp)                                 , 0  , 0  , 0x00, 0x00, 11 , 378, 1 , 0 }, // #91
  { F(Fp)|F(FPU_M4)|F(FPU_M8)             , 0  , 0  , 0x00, 0x00, 0  , 263, 1 , 0 }, // #92
  { F(Fp)|F(FPU_M4)|F(FPU_M8)|F(FPU_M10)  , 0  , 0  , 0x00, 0x00, 12 , 375, 1 , 0 }, // #93
  { F(Fp)                                 , 0  , 0  , 0x00, 0x00, 13 , 378, 1 , 0 }, // #94
  { F(Fp)|F(Volatile)                     , 0  , 0  , 0x00, 0x00, 0  , 260, 1 , 0 }, // #95
  { F(Fp)                                 , 0  , 0  , 0x00, 0x00, 0  , 379, 1 , 0 }, // #96
  { F(RW)|F(Special)                      , 0  , 0  , 0x00, 0x3F, 0  , 116, 4 , 0 }, // #97
  { F(RW)|F(Special)                      , 0  , 0  , 0x00, 0x3F, 0  , 34 , 10, 0 }, // #98
  { F(RW)|F(Lock)                         , 0  , 0  , 0x00, 0x1F, 14 , 256, 2 , 0 }, // #99
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 15 , 264, 2 , 0 }, // #100
  { F(Volatile)                           , 0  , 0  , 0x00, 0x88, 0  , 380, 1 , 0 }, // #101
  { F(Volatile)                           , 0  , 0  , 0x00, 0x88, 0  , 260, 1 , 0 }, // #102
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x24, 0x00, 16 , 381, 1 , 0 }, // #103
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x20, 0x00, 17 , 381, 1 , 0 }, // #104
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x20, 0x00, 18 , 381, 1 , 0 }, // #105
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x24, 0x00, 19 , 381, 1 , 0 }, // #106
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x20, 0x00, 18 , 382, 1 , 0 }, // #107
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x04, 0x00, 20 , 381, 1 , 0 }, // #108
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x07, 0x00, 21 , 381, 1 , 0 }, // #109
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x03, 0x00, 22 , 381, 1 , 0 }, // #110
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x03, 0x00, 23 , 381, 1 , 0 }, // #111
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x07, 0x00, 24 , 381, 1 , 0 }, // #112
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x20, 0x00, 17 , 382, 1 , 0 }, // #113
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x04, 0x00, 25 , 381, 1 , 0 }, // #114
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x01, 0x00, 26 , 381, 1 , 0 }, // #115
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x10, 0x00, 27 , 381, 1 , 0 }, // #116
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x02, 0x00, 28 , 381, 1 , 0 }, // #117
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x01, 0x00, 29 , 381, 1 , 0 }, // #118
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x10, 0x00, 30 , 381, 1 , 0 }, // #119
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x02, 0x00, 31 , 381, 1 , 0 }, // #120
  { F(Flow)|F(Volatile)|F(Special)        , 0  , 0  , 0x00, 0x00, 32 , 266, 2 , 0 }, // #121
  { F(Flow)|F(Volatile)                   , 0  , 0  , 0x00, 0x00, 33 , 268, 2 , 0 }, // #122
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 383, 1 , 0 }, // #123
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 34 , 270, 2 , 0 }, // #124
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 35 , 272, 2 , 0 }, // #125
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 36 , 274, 2 , 0 }, // #126
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 37 , 276, 2 , 0 }, // #127
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 384, 1 , 0 }, // #128
  { F(RO)|F(Vex)                          , 0  , 0  , 0x00, 0x3F, 0  , 385, 1 , 0 }, // #129
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 386, 1 , 0 }, // #130
  { F(RW)|F(Volatile)|F(Special)          , 0  , 0  , 0x3E, 0x00, 0  , 387, 1 , 0 }, // #131
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 0  , 201, 1 , 0 }, // #132
  { F(RO)|F(Volatile)                     , 0  , 0  , 0x00, 0x00, 0  , 388, 1 , 0 }, // #133
  { F(WO)                                 , 0  , 0  , 0x00, 0x00, 0  , 389, 1 , 0 }, // #134
  { F(Volatile)|F(Special)                , 0  , 0  , 0x00, 0x00, 0  , 260, 1 , 0 }, // #135
  { F(WO)|F(Special)                      , 0  , 1  , 0x40, 0x00, 0  , 0  , 0 , 0 }, // #136
  { F(WO)|F(Special)                      , 0  , 4  , 0x40, 0x00, 0  , 0  , 0 , 0 }, // #137
  { F(WO)|F(Special)                      , 0  , 8  , 0x40, 0x00, 0  , 0  , 0 , 0 }, // #138
  { F(WO)|F(Special)                      , 0  , 2  , 0x40, 0x00, 0  , 0  , 0 , 0 }, // #139
  { F(RW)                                 , 0  , 0  , 0x00, 0x3F, 0  , 153, 3 , 0 }, // #140
  { F(RO)|F(Special)                      , 0  , 0  , 0x00, 0x00, 0  , 390, 1 , 0 }, // #141
  { F(RO)|F(Special)                      , 0  , 0  , 0x00, 0x00, 0  , 391, 1 , 0 }, // #142
  { F(RW)|F(Volatile)                     , 0  , 0  , 0x00, 0x00, 0  , 260, 1 , 0 }, // #143
  { F(RO)|F(Volatile)|F(Special)          , 0  , 0  , 0x00, 0x00, 0  , 0  , 0 , 0 }, // #144
  { F(WO)                                 , 0  , 0  , 0x00, 0x00, 0  , 0  , 14, 0 }, // #145
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 38 , 64 , 2 , 0 }, // #146
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 39 , 64 , 2 , 0 }, // #147
  { F(WO)                                 , 0  , 0  , 0x00, 0x00, 40 , 52 , 6 , 0 }, // #148
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 41 , 278, 2 , 0 }, // #149
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 392, 1 , 0 }, // #150
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 42 , 64 , 2 , 0 }, // #151
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 43 , 64 , 2 , 0 }, // #152
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 393, 1 , 0 }, // #153
  { F(RW)                                 , 8  , 8  , 0x00, 0x00, 44 , 207, 2 , 0 }, // #154
  { F(RW)                                 , 8  , 8  , 0x00, 0x00, 45 , 207, 2 , 0 }, // #155
  { F(RW)                                 , 8  , 8  , 0x00, 0x00, 0  , 393, 1 , 0 }, // #156
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 46 , 207, 2 , 0 }, // #157
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 47 , 207, 2 , 0 }, // #158
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 394, 1 , 0 }, // #159
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 48 , 198, 1 , 0 }, // #160
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 56 , 2 , 0 }, // #161
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 49 , 198, 1 , 0 }, // #162
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 50 , 198, 1 , 0 }, // #163
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 51 , 395, 1 , 0 }, // #164
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 52 , 158, 1 , 0 }, // #165
  { F(WO)                                 , 0  , 4  , 0x00, 0x00, 53 , 281, 1 , 0 }, // #166
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 54 , 58 , 6 , 0 }, // #167
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 0  , 396, 1 , 0 }, // #168
  { F(WO)|F(Special)                      , 0  , 0  , 0x00, 0x00, 0  , 0  , 0 , 0 }, // #169
  { F(WO)|F(ZeroIfMem)                    , 0  , 8  , 0x00, 0x00, 55 , 156, 3 , 0 }, // #170
  { F(WO)|F(ZeroIfMem)                    , 0  , 4  , 0x00, 0x00, 56 , 280, 2 , 0 }, // #171
  { F(WO)                                 , 0  , 0  , 0x00, 0x00, 0  , 282, 2 , 0 }, // #172
  { F(WO)                                 , 0  , 0  , 0x00, 0x00, 0  , 397, 1 , 0 }, // #173
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 57 , 64 , 2 , 0 }, // #174
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 58 , 64 , 2 , 0 }, // #175
  { F(RW)|F(Special)                      , 0  , 0  , 0x00, 0x3F, 0  , 34 , 4 , 0 }, // #176
  { F(RW)|F(Special)                      , 0  , 0  , 0x00, 0x00, 0  , 284, 2 , 0 }, // #177
  { F(RW)|F(Lock)                         , 0  , 0  , 0x00, 0x3F, 0  , 257, 1 , 0 }, // #178
  { 0                                     , 0  , 0  , 0x00, 0x00, 0  , 286, 2 , 0 }, // #179
  { F(RW)|F(Lock)                         , 0  , 0  , 0x00, 0x00, 0  , 257, 1 , 0 }, // #180
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 0  , 288, 2 , 0 }, // #181
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 0  , 290, 2 , 0 }, // #182
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 0  , 260, 1 , 0 }, // #183
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 0  , 288, 1 , 0 }, // #184
  { F(WO)|F(Special)                      , 0  , 0  , 0x00, 0x00, 0  , 398, 1 , 0 }, // #185
  { F(WO)|F(Special)                      , 0  , 0  , 0x00, 0x00, 0  , 399, 1 , 0 }, // #186
  { F(WO)|F(Special)                      , 0  , 0  , 0x00, 0x00, 0  , 400, 1 , 0 }, // #187
  { F(WO)|F(Special)                      , 0  , 0  , 0x00, 0x00, 0  , 401, 1 , 0 }, // #188
  { F(WO)                                 , 0  , 0  , 0x00, 0x00, 0  , 246, 2 , 0 }, // #189
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 402, 1 , 0 }, // #190
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 403, 1 , 0 }, // #191
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 59 , 292, 2 , 0 }, // #192
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 296, 1 , 0 }, // #193
  { F(WO)                                 , 0  , 0  , 0x00, 0x00, 0  , 296, 1 , 0 }, // #194
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 0  , 404, 1 , 0 }, // #195
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 0  , 405, 1 , 0 }, // #196
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 0  , 406, 1 , 0 }, // #197
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 0  , 407, 1 , 0 }, // #198
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 408, 1 , 0 }, // #199
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 0  , 228, 1 , 0 }, // #200
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 0  , 231, 1 , 0 }, // #201
  { F(WO)|F(Volatile)|F(Special)          , 0  , 0  , 0x00, 0x00, 60 , 294, 2 , 0 }, // #202
  { F(Volatile)|F(Special)                , 0  , 0  , 0x00, 0x00, 0  , 409, 1 , 0 }, // #203
  { F(WO)                                 , 0  , 0  , 0x00, 0x3F, 0  , 153, 3 , 0 }, // #204
  { F(Volatile)|F(Special)                , 0  , 0  , 0x00, 0xFF, 0  , 260, 1 , 0 }, // #205
  { F(RO)|F(Volatile)                     , 0  , 0  , 0x00, 0x00, 0  , 0  , 0 , 0 }, // #206
  { F(RO)|F(Volatile)                     , 0  , 0  , 0x00, 0x3F, 0  , 353, 1 , 0 }, // #207
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 0  , 296, 2 , 0 }, // #208
  { F(WO)                                 , 0  , 16 , 0x00, 0x00, 0  , 71 , 1 , 0 }, // #209
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 410, 1 , 0 }, // #210
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 61 , 298, 2 , 0 }, // #211
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 62 , 411, 1 , 0 }, // #212
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 63 , 298, 2 , 0 }, // #213
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 64 , 298, 2 , 0 }, // #214
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 65 , 298, 2 , 0 }, // #215
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 66 , 298, 2 , 0 }, // #216
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 67 , 298, 2 , 0 }, // #217
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 68 , 411, 1 , 0 }, // #218
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 69 , 298, 2 , 0 }, // #219
  { F(RW)                                 , 0  , 0  , 0x00, 0x00, 70 , 298, 2 , 0 }, // #220
  { F(RO)                                 , 0  , 0  , 0x00, 0x3F, 0  , 342, 1 , 0 }, // #221
  { F(RO)|F(Volatile)|F(Special)          , 0  , 0  , 0x00, 0x00, 71 , 300, 2 , 0 }, // #222
  { F(Volatile)|F(Special)                , 0  , 0  , 0xFF, 0x00, 0  , 260, 1 , 0 }, // #223
  { F(RW)|F(Special)                      , 0  , 0  , 0x20, 0x21, 0  , 412, 1 , 0 }, // #224
  { F(WO)                                 , 0  , 4  , 0x00, 0x00, 0  , 228, 1 , 0 }, // #225
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 413, 1 , 0 }, // #226
  { F(WO)                                 , 0  , 8  , 0x00, 0x3F, 0  , 414, 1 , 0 }, // #227
  { F(WO)|F(Volatile)|F(Special)          , 0  , 0  , 0x00, 0x00, 0  , 415, 1 , 0 }, // #228
  { F(WO)|F(Volatile)|F(Special)          , 0  , 0  , 0x00, 0x00, 0  , 416, 1 , 0 }, // #229
  { F(RW)|F(Volatile)|F(Special)          , 0  , 0  , 0x40, 0x00, 0  , 0  , 0 , 0 }, // #230
  { F(RW)|F(Volatile)|F(Special)          , 0  , 0  , 0x00, 0x00, 0  , 302, 2 , 0 }, // #231
  { F(RW)|F(Special)                      , 0  , 0  , 0x00, 0x21, 0  , 412, 1 , 0 }, // #232
  { F(WO)                                 , 0  , 0  , 0x00, 0x00, 0  , 304, 2 , 0 }, // #233
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 417, 1 , 0 }, // #234
  { F(WO)                                 , 0  , 4  , 0x00, 0x00, 0  , 418, 1 , 0 }, // #235
  { F(RO)|F(Volatile)|F(Special)          , 0  , 0  , 0x00, 0x3E, 0  , 419, 1 , 0 }, // #236
  { F(RW)|F(Special)                      , 0  , 0  , 0x00, 0x3F, 0  , 412, 1 , 0 }, // #237
  { F(WO)                                 , 0  , 0  , 0x00, 0x00, 0  , 248, 2 , 0 }, // #238
  { F(WO)                                 , 0  , 1  , 0x24, 0x00, 0  , 420, 1 , 0 }, // #239
  { F(WO)                                 , 0  , 1  , 0x20, 0x00, 0  , 420, 1 , 0 }, // #240
  { F(WO)                                 , 0  , 1  , 0x04, 0x00, 0  , 420, 1 , 0 }, // #241
  { F(WO)                                 , 0  , 1  , 0x07, 0x00, 0  , 420, 1 , 0 }, // #242
  { F(WO)                                 , 0  , 1  , 0x03, 0x00, 0  , 420, 1 , 0 }, // #243
  { F(WO)                                 , 0  , 1  , 0x01, 0x00, 0  , 420, 1 , 0 }, // #244
  { F(WO)                                 , 0  , 1  , 0x10, 0x00, 0  , 420, 1 , 0 }, // #245
  { F(WO)                                 , 0  , 1  , 0x02, 0x00, 0  , 420, 1 , 0 }, // #246
  { F(RW)|F(Special)                      , 0  , 0  , 0x00, 0x3F, 0  , 159, 3 , 0 }, // #247
  { F(WO)                                 , 0  , 8  , 0x00, 0x00, 0  , 62 , 1 , 0 }, // #248
  { 0                                     , 0  , 0  , 0x00, 0x20, 0  , 260, 1 , 0 }, // #249
  { 0                                     , 0  , 0  , 0x00, 0x40, 0  , 260, 1 , 0 }, // #250
  { 0                                     , 0  , 0  , 0x00, 0x80, 0  , 260, 1 , 0 }, // #251
  { F(Volatile)                           , 0  , 0  , 0x00, 0x00, 0  , 421, 1 , 0 }, // #252
  { F(RO)                                 , 0  , 0  , 0x00, 0x3F, 72 , 88 , 5 , 0 }, // #253
  { 0                                     , 0  , 0  , 0x00, 0x00, 0  , 260, 1 , 0 }, // #254
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #255
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #256
  { F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , 0  , 0  , 0x00, 0x00, 0  , 422, 1 , 0 }, // #257
  { F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , 0  , 0  , 0x00, 0x00, 0  , 423, 1 , 0 }, // #258
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 162, 2 , 0 }, // #259
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 70 , 1 , 0 }, // #260
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 64 , 1 , 0 }, // #261
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 71 , 1 , 0 }, // #262
  { F(WO)          |A512(F_  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 165, 3 , 0 }, // #263
  { F(WO)          |A512(F_  ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 165, 3 , 0 }, // #264
  { F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #265
  { F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #266
  { F(WO)          |A512(BW  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #267
  { F(WO)          |A512(F_  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #268
  { F(WO)          |A512(F_  ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #269
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 165, 2 , 0 }, // #270
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 306, 2 , 0 }, // #271
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 424, 1 , 0 }, // #272
  { F(WO)          |A512(DQ  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 425, 1 , 0 }, // #273
  { F(WO)          |A512(F_  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 426, 1 , 0 }, // #274
  { F(WO)          |A512(DQ  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 427, 1 , 0 }, // #275
  { F(WO)          |A512(DQ  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 426, 1 , 0 }, // #276
  { F(WO)          |A512(F_  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 427, 1 , 0 }, // #277
  { F(WO)          |A512(DQ  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 428, 1 , 0 }, // #278
  { F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 425, 1 , 0 }, // #279
  { F(WO)          |A512(DQ  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 233, 1 , 0 }, // #280
  { F(WO)          |A512(F_  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 233, 1 , 0 }, // #281
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 425, 1 , 0 }, // #282
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 326, 1 , 0 }, // #283
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,8) , 0  , 0  , 0x00, 0x00, 0  , 168, 3 , 0 }, // #284
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,4) , 0  , 0  , 0x00, 0x00, 0  , 168, 3 , 0 }, // #285
  { F(WO)|F(Vex)   |A512(F_  ,0,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 429, 1 , 0 }, // #286
  { F(WO)|F(Vex)   |A512(F_  ,0,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 430, 1 , 0 }, // #287
  { F(RO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , 0  , 0  , 0x00, 0x3F, 0  , 358, 1 , 0 }, // #288
  { F(RO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , 0  , 0  , 0x00, 0x3F, 0  , 359, 1 , 0 }, // #289
  { F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 171, 3 , 0 }, // #290
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 174, 3 , 0 }, // #291
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , 0  , 0  , 0x00, 0x00, 0  , 177, 3 , 0 }, // #292
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , 0  , 0  , 0x00, 0x00, 0  , 308, 2 , 0 }, // #293
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , 0  , 0  , 0x00, 0x00, 0  , 180, 3 , 0 }, // #294
  { F(WO)          |A512(DQ  ,1,KZ,ER ,8) , 0  , 0  , 0x00, 0x00, 0  , 177, 3 , 0 }, // #295
  { F(WO)          |A512(F_  ,1,KZ,ER ,8) , 0  , 0  , 0x00, 0x00, 0  , 308, 2 , 0 }, // #296
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 174, 3 , 0 }, // #297
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , 0  , 0  , 0x00, 0x00, 0  , 174, 3 , 0 }, // #298
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 183, 3 , 0 }, // #299
  { F(WO)          |A512(DQ  ,1,KZ,ER ,4) , 0  , 0  , 0x00, 0x00, 0  , 174, 3 , 0 }, // #300
  { F(WO)          |A512(F_  ,1,KZ,ER ,4) , 0  , 0  , 0x00, 0x00, 0  , 177, 3 , 0 }, // #301
  { F(WO)          |A512(DQ  ,1,KZ,ER ,8) , 0  , 0  , 0x00, 0x00, 0  , 308, 2 , 0 }, // #302
  { F(WO)|F(Vex)   |A512(F_  ,0,0 ,ER ,0) , 0  , 0  , 0x00, 0x00, 0  , 365, 1 , 0 }, // #303
  { F(WO)          |A512(F_  ,0,0 ,ER ,0) , 0  , 0  , 0x00, 0x00, 0  , 365, 1 , 0 }, // #304
  { F(WO)|F(Vex)   |A512(F_  ,0,0 ,ER ,0) , 0  , 0  , 0x00, 0x00, 0  , 431, 1 , 0 }, // #305
  { F(WO)|F(Vex)   |A512(F_  ,0,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 423, 1 , 0 }, // #306
  { F(WO)|F(Vex)   |A512(F_  ,0,0 ,ER ,0) , 0  , 0  , 0x00, 0x00, 0  , 310, 2 , 0 }, // #307
  { F(WO)          |A512(F_  ,0,0 ,ER ,0) , 0  , 0  , 0x00, 0x00, 0  , 312, 2 , 0 }, // #308
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,8) , 0  , 0  , 0x00, 0x00, 0  , 308, 2 , 0 }, // #309
  { F(WO)          |A512(F_  ,1,KZ,SAE,8) , 0  , 0  , 0x00, 0x00, 0  , 177, 3 , 0 }, // #310
  { F(WO)          |A512(F_  ,1,KZ,SAE,8) , 0  , 0  , 0x00, 0x00, 0  , 308, 2 , 0 }, // #311
  { F(WO)          |A512(DQ  ,1,KZ,SAE,8) , 0  , 0  , 0x00, 0x00, 0  , 177, 3 , 0 }, // #312
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,4) , 0  , 0  , 0x00, 0x00, 0  , 177, 3 , 0 }, // #313
  { F(WO)          |A512(DQ  ,1,KZ,SAE,4) , 0  , 0  , 0x00, 0x00, 0  , 174, 3 , 0 }, // #314
  { F(WO)          |A512(F_  ,1,KZ,SAE,4) , 0  , 0  , 0x00, 0x00, 0  , 177, 3 , 0 }, // #315
  { F(WO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 365, 1 , 0 }, // #316
  { F(WO)          |A512(F_  ,0,0 ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 365, 1 , 0 }, // #317
  { F(WO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 310, 2 , 0 }, // #318
  { F(WO)          |A512(F_  ,0,0 ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 312, 2 , 0 }, // #319
  { F(WO)          |A512(F_  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 174, 3 , 0 }, // #320
  { F(WO)          |A512(F_  ,0,0 ,ER ,0) , 0  , 0  , 0x00, 0x00, 0  , 431, 1 , 0 }, // #321
  { F(WO)          |A512(BW  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 165, 3 , 0 }, // #322
  { F(WO)          |A512(ERI ,0,KZ,SAE,8) , 0  , 0  , 0x00, 0x00, 0  , 68 , 1 , 0 }, // #323
  { F(WO)          |A512(ERI ,0,KZ,SAE,4) , 0  , 0  , 0x00, 0x00, 0  , 68 , 1 , 0 }, // #324
  { F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 177, 3 , 0 }, // #325
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 184, 1 , 0 }, // #326
  { F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 432, 1 , 0 }, // #327
  { F(WO)          |A512(DQ  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 185, 1 , 0 }, // #328
  { F(WO)          |A512(DQ  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 432, 1 , 0 }, // #329
  { F(WO)          |A512(F_  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 185, 1 , 0 }, // #330
  { F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 371, 1 , 0 }, // #331
  { F(RW)          |A512(F_  ,1,KZ,SAE,8) , 0  , 0  , 0x00, 0x00, 0  , 186, 3 , 0 }, // #332
  { F(RW)          |A512(F_  ,1,KZ,SAE,4) , 0  , 0  , 0x00, 0x00, 0  , 186, 3 , 0 }, // #333
  { F(RW)          |A512(F_  ,0,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 433, 1 , 0 }, // #334
  { F(RW)          |A512(F_  ,0,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 434, 1 , 0 }, // #335
  { F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , 0  , 0  , 0x00, 0x00, 0  , 189, 3 , 0 }, // #336
  { F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , 0  , 0  , 0x00, 0x00, 0  , 189, 3 , 0 }, // #337
  { F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , 0  , 0  , 0x00, 0x00, 0  , 435, 1 , 0 }, // #338
  { F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , 0  , 0  , 0x00, 0x00, 0  , 436, 1 , 0 }, // #339
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 120, 4 , 0 }, // #340
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 314, 2 , 0 }, // #341
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 316, 2 , 0 }, // #342
  { F(WO)          |A512(DQ  ,1,K_,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 437, 1 , 0 }, // #343
  { F(WO)          |A512(DQ  ,1,K_,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 437, 1 , 0 }, // #344
  { F(WO)          |A512(DQ  ,0,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 438, 1 , 0 }, // #345
  { F(WO)          |A512(DQ  ,0,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 439, 1 , 0 }, // #346
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 177, 2 , 0 }, // #347
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 62 , 1 , 0 }, // #348
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 228, 1 , 0 }, // #349
  { F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 73 , 93 , 5 , 0 }, // #350
  { F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 74 , 98 , 5 , 0 }, // #351
  { F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 440, 1 , 0 }, // #352
  { F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 441, 1 , 0 }, // #353
  { F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 442, 1 , 0 }, // #354
  { F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 75 , 103, 5 , 0 }, // #355
  { F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 76 , 124, 4 , 0 }, // #356
  { F(WO)          |A512(F_  ,0,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 62 , 1 , 0 }, // #357
  { F(WO)          |A512(F_  ,0,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 228, 1 , 0 }, // #358
  { F(WO)          |A512(F_  ,1,KZ,SAE,8) , 0  , 0  , 0x00, 0x00, 0  , 192, 3 , 0 }, // #359
  { F(WO)          |A512(F_  ,1,KZ,SAE,4) , 0  , 0  , 0x00, 0x00, 0  , 192, 3 , 0 }, // #360
  { F(WO)          |A512(F_  ,0,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 417, 1 , 0 }, // #361
  { F(WO)          |A512(F_  ,0,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 418, 1 , 0 }, // #362
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 318, 1 , 0 }, // #363
  { F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 318, 2 , 0 }, // #364
  { F(WO)          |A512(DQ  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 443, 1 , 0 }, // #365
  { F(WO)          |A512(DQ  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 318, 2 , 0 }, // #366
  { F(WO)          |A512(F_  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 443, 1 , 0 }, // #367
  { F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 444, 1 , 0 }, // #368
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 201, 2 , 0 }, // #369
  { F(RO)|F(Vex)|F(Volatile)              , 0  , 0  , 0x00, 0x00, 0  , 388, 1 , 0 }, // #370
  { F(RO)|F(Vex)|F(Special)               , 0  , 0  , 0x00, 0x00, 0  , 445, 1 , 0 }, // #371
  { F(RW)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 77 , 128, 4 , 0 }, // #372
  { F(RW)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 78 , 128, 4 , 0 }, // #373
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,8) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #374
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,4) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #375
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 422, 1 , 0 }, // #376
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 423, 1 , 0 }, // #377
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 79 , 64 , 6 , 0 }, // #378
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 80 , 64 , 6 , 0 }, // #379
  { F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 81 , 320, 2 , 0 }, // #380
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 195, 3 , 0 }, // #381
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 82 , 64 , 4 , 0 }, // #382
  { F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 83 , 64 , 6 , 0 }, // #383
  { F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 84 , 64 , 6 , 0 }, // #384
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 85 , 64 , 4 , 0 }, // #385
  { F(WO)          |A512(BW  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 86 , 64 , 6 , 0 }, // #386
  { F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 87 , 64 , 6 , 0 }, // #387
  { F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 88 , 64 , 6 , 0 }, // #388
  { F(WO)          |A512(BW  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 89 , 64 , 6 , 0 }, // #389
  { F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 209, 1 , 0 }, // #390
  { F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 90 , 322, 2 , 0 }, // #391
  { F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 91 , 322, 2 , 0 }, // #392
  { F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 92 , 322, 2 , 0 }, // #393
  { F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 93 , 322, 2 , 0 }, // #394
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 446, 1 , 0 }, // #395
  { F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 198, 3 , 0 }, // #396
  { F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 201, 3 , 0 }, // #397
  { F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 94 , 204, 3 , 0 }, // #398
  { F(WO)|F(Vex)   |A512(F_  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 95 , 207, 3 , 0 }, // #399
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 177, 3 , 0 }, // #400
  { F(WO)|F(Vex)   |A512(F_  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 96 , 210, 3 , 0 }, // #401
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 97 , 64 , 6 , 0 }, // #402
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 98 , 64 , 6 , 0 }, // #403
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #404
  { F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 177, 3 , 0 }, // #405
  { F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #406
  { F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #407
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #408
  { F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 165, 3 , 0 }, // #409
  { F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 324, 2 , 0 }, // #410
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 326, 2 , 0 }, // #411
  { F(WO)          |A512(CDI ,1,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 447, 1 , 0 }, // #412
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 448, 1 , 0 }, // #413
  { F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 328, 2 , 0 }, // #414
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 165, 1 , 0 }, // #415
  { F(WO)          |A512(BW  ,1,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 213, 3 , 0 }, // #416
  { F(WO)          |A512(F_  ,1,K_,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 213, 3 , 0 }, // #417
  { F(WO)|F(Vex)   |A512(BW  ,1,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 216, 3 , 0 }, // #418
  { F(WO)|F(Vex)   |A512(F_  ,1,K_,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 216, 3 , 0 }, // #419
  { F(WO)|F(Vex)   |A512(F_  ,1,K_,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 216, 3 , 0 }, // #420
  { F(WO)|F(Vex)|F(Special)               , 0  , 0  , 0x00, 0x00, 0  , 398, 1 , 0 }, // #421
  { F(WO)|F(Vex)|F(Special)               , 0  , 0  , 0x00, 0x00, 0  , 399, 1 , 0 }, // #422
  { F(WO)|F(Vex)|F(Special)               , 0  , 0  , 0x00, 0x00, 0  , 400, 1 , 0 }, // #423
  { F(WO)|F(Vex)|F(Special)               , 0  , 0  , 0x00, 0x00, 0  , 401, 1 , 0 }, // #424
  { F(WO)          |A512(F_  ,1,K_,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 213, 3 , 0 }, // #425
  { F(WO)          |A512(BW  ,1,K_,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 213, 3 , 0 }, // #426
  { F(WO)          |A512(CDI ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 177, 3 , 0 }, // #427
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 166, 1 , 0 }, // #428
  { F(WO)          |A512(VBMI,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #429
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 137, 2 , 0 }, // #430
  { F(RW)          |A512(F_  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 189, 3 , 0 }, // #431
  { F(RW)          |A512(F_  ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 189, 3 , 0 }, // #432
  { F(RW)          |A512(BW  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 189, 3 , 0 }, // #433
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 132, 4 , 0 }, // #434
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 99 , 70 , 6 , 0 }, // #435
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 100, 70 , 6 , 0 }, // #436
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 73 , 1 , 0 }, // #437
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 72 , 1 , 0 }, // #438
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 101, 136, 4 , 0 }, // #439
  { F(WO)|F(Vex)   |A512(BW  ,0,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 402, 1 , 0 }, // #440
  { F(WO)|F(Vex)   |A512(DQ  ,0,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 371, 1 , 0 }, // #441
  { F(WO)|F(Vex)   |A512(DQ  ,0,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 403, 1 , 0 }, // #442
  { F(WO)|F(Vex)   |A512(BW  ,0,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 293, 1 , 0 }, // #443
  { F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 102, 98 , 5 , 0 }, // #444
  { F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 103, 93 , 5 , 0 }, // #445
  { F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 104, 124, 4 , 0 }, // #446
  { F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 105, 103, 5 , 0 }, // #447
  { F(WO)|F(Vex)   |A512(BW  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 330, 2 , 0 }, // #448
  { F(WO)|F(Vex)   |A512(DQ  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 332, 2 , 0 }, // #449
  { F(WO)|F(Vex)   |A512(DQ  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 334, 2 , 0 }, // #450
  { F(WO)|F(Vex)   |A512(BW  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 449, 1 , 0 }, // #451
  { F(WO)          |A512(CDI ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 177, 3 , 0 }, // #452
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 121, 1 , 0 }, // #453
  { F(WO)          |A512(IFMA,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #454
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 106, 128, 4 , 0 }, // #455
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 107, 128, 4 , 0 }, // #456
  { F(WO)          |A512(BW  ,1,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 450, 1 , 0 }, // #457
  { F(WO)          |A512(DQ  ,1,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 450, 1 , 0 }, // #458
  { F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 219, 3 , 0 }, // #459
  { F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 222, 3 , 0 }, // #460
  { F(WO)          |A512(BW  ,1,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 447, 1 , 0 }, // #461
  { F(WO)          |A512(DQ  ,1,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 447, 1 , 0 }, // #462
  { F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 225, 3 , 0 }, // #463
  { F(WO)          |A512(BW  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 222, 3 , 0 }, // #464
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 228, 3 , 0 }, // #465
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 231, 3 , 0 }, // #466
  { F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 174, 3 , 0 }, // #467
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 234, 3 , 0 }, // #468
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 174, 3 , 0 }, // #469
  { F(WO)          |A512(DQ  ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #470
  { F(WO)          |A512(VBMI,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #471
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 120, 2 , 0 }, // #472
  { F(WO)          |A512(F_  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 192, 3 , 0 }, // #473
  { F(WO)          |A512(F_  ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 192, 3 , 0 }, // #474
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 108, 336, 2 , 0 }, // #475
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 109, 336, 2 , 0 }, // #476
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 110, 336, 2 , 0 }, // #477
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 111, 336, 2 , 0 }, // #478
  { F(WO)|F(Vex)   |A512(BW  ,1,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #479
  { F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 237, 3 , 0 }, // #480
  { F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 338, 2 , 0 }, // #481
  { F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 240, 3 , 0 }, // #482
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 340, 2 , 0 }, // #483
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 192, 3 , 0 }, // #484
  { F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 192, 3 , 0 }, // #485
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 112, 76 , 6 , 0 }, // #486
  { F(WO)|F(Vex)   |A512(BW  ,1,0 ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 192, 3 , 0 }, // #487
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 113, 76 , 6 , 0 }, // #488
  { F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 114, 76 , 6 , 0 }, // #489
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 115, 76 , 6 , 0 }, // #490
  { F(WO)          |A512(F_  ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 116, 82 , 6 , 0 }, // #491
  { F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 117, 76 , 6 , 0 }, // #492
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 118, 76 , 6 , 0 }, // #493
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 119, 76 , 6 , 0 }, // #494
  { F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 120, 76 , 6 , 0 }, // #495
  { F(RW)          |A512(F_  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 186, 3 , 0 }, // #496
  { F(RW)          |A512(F_  ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 186, 3 , 0 }, // #497
  { F(RO)|F(Vex)                          , 0  , 0  , 0x00, 0x3F, 0  , 342, 2 , 0 }, // #498
  { F(WO)          |A512(BW  ,1,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 243, 3 , 0 }, // #499
  { F(WO)          |A512(F_  ,1,K_,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 243, 3 , 0 }, // #500
  { F(WO)          |A512(F_  ,1,K_,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 243, 3 , 0 }, // #501
  { F(WO)          |A512(DQ  ,1,KZ,SAE,8) , 0  , 0  , 0x00, 0x00, 0  , 165, 3 , 0 }, // #502
  { F(WO)          |A512(DQ  ,1,KZ,SAE,4) , 0  , 0  , 0x00, 0x00, 0  , 165, 3 , 0 }, // #503
  { F(WO)          |A512(DQ  ,0,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 451, 1 , 0 }, // #504
  { F(WO)          |A512(DQ  ,0,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 444, 1 , 0 }, // #505
  { F(WO)          |A512(F_  ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 177, 3 , 0 }, // #506
  { F(WO)          |A512(F_  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 177, 3 , 0 }, // #507
  { F(WO)          |A512(F_  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 422, 1 , 0 }, // #508
  { F(WO)          |A512(F_  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 423, 1 , 0 }, // #509
  { F(WO)          |A512(ERI ,0,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 422, 1 , 0 }, // #510
  { F(WO)          |A512(ERI ,0,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 423, 1 , 0 }, // #511
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 423, 1 , 0 }, // #512
  { F(WO)          |A512(DQ  ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 192, 3 , 0 }, // #513
  { F(WO)          |A512(DQ  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 192, 3 , 0 }, // #514
  { F(WO)          |A512(DQ  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 451, 1 , 0 }, // #515
  { F(WO)          |A512(DQ  ,0,KZ,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 444, 1 , 0 }, // #516
  { F(WO)          |A512(F_  ,0,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 451, 1 , 0 }, // #517
  { F(WO)          |A512(F_  ,0,KZ,SAE,0) , 0  , 0  , 0x00, 0x00, 0  , 444, 1 , 0 }, // #518
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 78 , 2 , 0 }, // #519
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 451, 1 , 0 }, // #520
  { F(WO)|F(Vex)                          , 0  , 0  , 0x00, 0x00, 0  , 444, 1 , 0 }, // #521
  { F(WO)          |A512(F_  ,1,KZ,ER ,8) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #522
  { F(WO)          |A512(F_  ,1,KZ,ER ,4) , 0  , 0  , 0x00, 0x00, 0  , 162, 3 , 0 }, // #523
  { F(WO)          |A512(F_  ,0,KZ,ER ,0) , 0  , 0  , 0x00, 0x00, 0  , 422, 1 , 0 }, // #524
  { F(WO)          |A512(F_  ,0,KZ,ER ,0) , 0  , 0  , 0x00, 0x00, 0  , 423, 1 , 0 }, // #525
  { F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , 0  , 0  , 0x00, 0x00, 0  , 344, 2 , 0 }, // #526
  { F(WO)          |A512(F_  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 166, 2 , 0 }, // #527
  { F(WO)          |A512(F_  ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 166, 2 , 0 }, // #528
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , 0  , 0  , 0x00, 0x00, 0  , 165, 3 , 0 }, // #529
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , 0  , 0  , 0x00, 0x00, 0  , 165, 3 , 0 }, // #530
  { F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , 0  , 0  , 0x00, 0x00, 0  , 177, 3 , 0 }, // #531
  { F(Vex)|F(Volatile)                    , 0  , 0  , 0x00, 0x00, 0  , 421, 1 , 0 }, // #532
  { F(Vex)|F(Volatile)                    , 0  , 0  , 0x00, 0x00, 0  , 260, 1 , 0 }, // #533
  { F(RO)|F(Volatile)                     , 0  , 0  , 0x00, 0x00, 0  , 452, 1 , 0 }, // #534
  { F(RW)|F(Xchg)|F(Lock)                 , 0  , 0  , 0x00, 0x3F, 0  , 140, 4 , 0 }, // #535
  { F(RW)|F(Xchg)|F(Lock)                 , 0  , 0  , 0x00, 0x00, 0  , 44 , 8 , 0 }, // #536
  { F(WO)|F(Special)                      , 0  , 0  , 0x00, 0x00, 0  , 453, 1 , 0 }, // #537
  { F(RO)|F(Volatile)|F(Special)          , 0  , 0  , 0x00, 0x00, 0  , 454, 1 , 0 }, // #538
  { F(RO)|F(Volatile)|F(Special)          , 0  , 0  , 0x00, 0x00, 0  , 455, 1 , 0 }, // #539
  { F(WO)|F(Volatile)|F(Special)          , 0  , 0  , 0x00, 0x00, 0  , 454, 1 , 0 }, // #540
  { F(WO)|F(Volatile)|F(Special)          , 0  , 0  , 0x00, 0x00, 0  , 455, 1 , 0 }, // #541
  { F(RO)|F(Volatile)|F(Special)          , 0  , 0  , 0x00, 0x00, 0  , 456, 1 , 0 }  // #542
};
// ----------------------------------------------------------------------------
// ${commonData:End}

#undef INST
#undef A512
#undef NAME_DATA_INDEX

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
// ${nameData:Begin}
// ------------------- Automatically generated, do not edit -------------------
const char X86InstDB::nameData[] =
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

enum {
  kX86InstMaxLength = 16
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
// ${nameData:End}

//! \internal
//!
//! Compare two instruction names.
//!
//! `a` is a null terminated instruction name from `X86InstDB::nameData[]` table.
//! `b` is a non-null terminated instruction name passed to `X86Inst::getIdByName()`.
static ASMJIT_INLINE int X86Inst_compareName(const char* a, const char* b, size_t len) noexcept {
  for (size_t i = 0; i < len; i++) {
    int c = static_cast<int>(static_cast<uint8_t>(a[i])) -
            static_cast<int>(static_cast<uint8_t>(b[i])) ;
    if (c != 0) return c;
  }

  return static_cast<int>(a[len]);
}

uint32_t X86Inst::getIdByName(const char* name, size_t len) noexcept {
  if (ASMJIT_UNLIKELY(!name))
    return kInvalidInst;

  char nameLower[kX86InstMaxLength];

  if (len == kInvalidIndex) {
    size_t i = 0;
    for (;;) {
      uint32_t c = Utils::toLower<uint32_t>(name[i]);
      if (!c) break;

      if (ASMJIT_UNLIKELY(i > kX86InstMaxLength))
        return kInvalidInst;

      nameLower[i++] = c;
    }
    len = i;
  }
  else {
    size_t i;
    if (ASMJIT_UNLIKELY(len > kX86InstMaxLength))
      return kInvalidInst;
    for (i = 0; i < len; i++)
      nameLower[i] = Utils::toLower<uint32_t>(name[i]);
  }

  if (ASMJIT_UNLIKELY(len == 0))
    return kInvalidInst;

  uint32_t prefix = static_cast<uint32_t>(nameLower[0]) - kX86InstAlphaIndexFirst;
  if (ASMJIT_UNLIKELY(prefix > kX86InstAlphaIndexLast - kX86InstAlphaIndexFirst))
    return kInvalidInst;

  uint32_t index = _x86InstAlphaIndex[prefix];
  if (ASMJIT_UNLIKELY(index == kX86InstAlphaIndexInvalid))
    return kInvalidInst;

  const char* nameData = X86InstDB::nameData;
  const X86Inst* instData = X86InstDB::instData;

  const X86Inst* base = instData + index;
  const X86Inst* end  = instData + X86Inst::_kIdCount;

  // Special handling of instructions starting with 'j' because `jcc` instruction(s)
  // are not sorted alphabetically due to suffixes that are considered part of the
  // instruction. This results in `jecxz` and `jmp` stored after all `jcc` instructions.
  bool useLinearSearch = prefix == ('j' - kX86InstAlphaIndexFirst);

  // Limit the search only to instructions starting with `prefix`.
  while (++prefix <= kX86InstAlphaIndexLast - kX86InstAlphaIndexFirst) {
    index = _x86InstAlphaIndex[prefix];
    if (index == kX86InstAlphaIndexInvalid)
      continue;
    end = instData + index;
    break;
  }

  if (useLinearSearch) {
    while (base != end) {
      if (X86Inst_compareName(nameData + base[0].getNameDataIndex(), nameLower, len) == 0)
        return static_cast<uint32_t>((size_t)(base - instData));
      base++;
    }
  }
  else {
    for (size_t lim = (size_t)(end - base); lim != 0; lim >>= 1) {
      const X86Inst* cur = base + (lim >> 1);
      int result = X86Inst_compareName(nameData + cur[0].getNameDataIndex(), nameLower, len);

      if (result < 0) {
        base = cur + 1;
        lim--;
        continue;
      }

      if (result > 0)
        continue;

      return static_cast<uint32_t>((size_t)(cur - instData));
    }
  }

  return kInvalidInst;
}

const char* X86Inst::getNameById(uint32_t id) noexcept {
  if (ASMJIT_UNLIKELY(id >= X86Inst::_kIdCount))
    return nullptr;
  return X86Inst::getInst(id).getName();
}
#else
const char X86InstDB::nameData[] = "";
#endif // !ASMJIT_DISABLE_TEXT

// ============================================================================
// [asmjit::X86Util - Validation]
// ============================================================================

#if !defined(ASMJIT_DISABLE_VALIDATION)
// ${signatureData:Begin}
// ------------------- Automatically generated, do not edit -------------------
#define ISIGNATURE(count, x86, x64, implicit, o0, o1, o2, o3, o4, o5) \
  { count, (x86 ? uint8_t(X86Inst::kArchMaskX86) : uint8_t(0)) |      \
           (x64 ? uint8_t(X86Inst::kArchMaskX64) : uint8_t(0)) ,      \
    implicit,                                                         \
    0,                                                                \
    o0, o1, o2, o3, o4, o5                                            \
  }
static const X86Inst::ISignature _x86InstISignatureData[] = {
  ISIGNATURE(2, 1, 1, 0, 1  , 2  , 0  , 0  , 0  , 0  ), // #0   {W:r8lo|r8hi|m8, R:r8lo|r8hi|i8}
  ISIGNATURE(2, 1, 1, 0, 3  , 4  , 0  , 0  , 0  , 0  ), //      {W:r16|m16, R:r16|sreg|i16}
  ISIGNATURE(2, 1, 1, 0, 5  , 6  , 0  , 0  , 0  , 0  ), //      {W:r32|m32|sreg, R:r32}
  ISIGNATURE(2, 0, 1, 0, 7  , 8  , 0  , 0  , 0  , 0  ), //      {W:r64|m64, R:r64|sreg|i32}
  ISIGNATURE(2, 1, 1, 0, 9  , 10 , 0  , 0  , 0  , 0  ), //      {W:r8lo|r8hi, R:r8lo|r8hi|m8|i8}
  ISIGNATURE(2, 1, 1, 0, 11 , 12 , 0  , 0  , 0  , 0  ), //      {W:r16|sreg, R:r16|m16}
  ISIGNATURE(2, 1, 1, 0, 13 , 14 , 0  , 0  , 0  , 0  ), //      {W:r32, R:r32|m32|sreg|i32}
  ISIGNATURE(2, 0, 1, 0, 15 , 16 , 0  , 0  , 0  , 0  ), //      {W:r64|sreg, R:r64|m64}
  ISIGNATURE(2, 1, 1, 0, 17 , 18 , 0  , 0  , 0  , 0  ), //      {W:r16, R:i16}
  ISIGNATURE(2, 0, 1, 0, 19 , 20 , 0  , 0  , 0  , 0  ), //      {W:r64, R:i64|creg|dreg}
  ISIGNATURE(2, 1, 1, 0, 21 , 22 , 0  , 0  , 0  , 0  ), //      {W:r32|m32, R:i32}
  ISIGNATURE(2, 1, 0, 0, 13 , 23 , 0  , 0  , 0  , 0  ), //      {W:r32, R:creg|dreg}
  ISIGNATURE(2, 1, 0, 0, 24 , 6  , 0  , 0  , 0  , 0  ), //      {W:creg|dreg, R:r32}
  ISIGNATURE(2, 0, 1, 0, 24 , 25 , 0  , 0  , 0  , 0  ), //      {W:creg|dreg, R:r64}
  ISIGNATURE(2, 1, 1, 0, 26 , 27 , 0  , 0  , 0  , 0  ), // #14  {X:r8lo|r8hi|m8|r16|m16|r32|m32|r64|m64, R:i8}
  ISIGNATURE(2, 1, 1, 0, 28 , 29 , 0  , 0  , 0  , 0  ), //      {X:r16|m16, R:i16|r16}
  ISIGNATURE(2, 1, 1, 0, 30 , 22 , 0  , 0  , 0  , 0  ), //      {X:r32|m32|r64|m64, R:i32}
  ISIGNATURE(2, 1, 1, 0, 31 , 32 , 0  , 0  , 0  , 0  ), //      {X:r8lo|r8hi|m8, R:r8lo|r8hi}
  ISIGNATURE(2, 1, 1, 0, 33 , 6  , 0  , 0  , 0  , 0  ), //      {X:r32|m32, R:r32}
  ISIGNATURE(2, 0, 1, 0, 34 , 25 , 0  , 0  , 0  , 0  ), //      {X:r64|m64, R:r64}
  ISIGNATURE(2, 1, 1, 0, 35 , 36 , 0  , 0  , 0  , 0  ), //      {X:r8lo|r8hi, R:r8lo|r8hi|m8}
  ISIGNATURE(2, 1, 1, 0, 37 , 12 , 0  , 0  , 0  , 0  ), // #21  {X:r16, R:r16|m16}
  ISIGNATURE(2, 1, 1, 0, 38 , 39 , 0  , 0  , 0  , 0  ), // #22  {X:r32, R:r32|m32}
  ISIGNATURE(2, 0, 1, 0, 40 , 16 , 0  , 0  , 0  , 0  ), //      {X:r64, R:r64|m64}
  ISIGNATURE(2, 1, 1, 0, 41 , 27 , 0  , 0  , 0  , 0  ), // #24  {R:r8lo|r8hi|m8|r16|m16|r32|m32|r64|m64, R:i8}
  ISIGNATURE(2, 1, 1, 0, 12 , 29 , 0  , 0  , 0  , 0  ), //      {R:r16|m16, R:i16|r16}
  ISIGNATURE(2, 1, 1, 0, 42 , 22 , 0  , 0  , 0  , 0  ), //      {R:r32|m32|r64|m64, R:i32}
  ISIGNATURE(2, 1, 1, 0, 36 , 32 , 0  , 0  , 0  , 0  ), //      {R:r8lo|r8hi|m8, R:r8lo|r8hi}
  ISIGNATURE(2, 1, 1, 0, 39 , 6  , 0  , 0  , 0  , 0  ), //      {R:r32|m32, R:r32}
  ISIGNATURE(2, 0, 1, 0, 16 , 25 , 0  , 0  , 0  , 0  ), //      {R:r64|m64, R:r64}
  ISIGNATURE(2, 1, 1, 0, 32 , 36 , 0  , 0  , 0  , 0  ), //      {R:r8lo|r8hi, R:r8lo|r8hi|m8}
  ISIGNATURE(2, 1, 1, 0, 43 , 12 , 0  , 0  , 0  , 0  ), //      {R:r16, R:r16|m16}
  ISIGNATURE(2, 1, 1, 0, 6  , 39 , 0  , 0  , 0  , 0  ), //      {R:r32, R:r32|m32}
  ISIGNATURE(2, 0, 1, 0, 25 , 16 , 0  , 0  , 0  , 0  ), //      {R:r64, R:r64|m64}
  ISIGNATURE(2, 1, 1, 1, 44 , 36 , 0  , 0  , 0  , 0  ), // #34  {X:<ax>, R:r8lo|r8hi|m8}
  ISIGNATURE(3, 1, 1, 2, 45 , 44 , 12 , 0  , 0  , 0  ), //      {W:<dx>, X:<ax>, R:r16|m16}
  ISIGNATURE(3, 1, 1, 2, 46 , 47 , 39 , 0  , 0  , 0  ), //      {W:<edx>, X:<eax>, R:r32|m32}
  ISIGNATURE(3, 0, 1, 2, 48 , 49 , 16 , 0  , 0  , 0  ), //      {W:<rdx>, X:<rax>, R:r64|m64}
  ISIGNATURE(2, 1, 1, 0, 37 , 50 , 0  , 0  , 0  , 0  ), //      {X:r16, R:r16|m16|i8|i16}
  ISIGNATURE(2, 1, 1, 0, 38 , 51 , 0  , 0  , 0  , 0  ), //      {X:r32, R:r32|m32|i8|i32}
  ISIGNATURE(2, 0, 1, 0, 40 , 52 , 0  , 0  , 0  , 0  ), //      {X:r64, R:r64|m64|i8|i32}
  ISIGNATURE(3, 1, 1, 0, 17 , 12 , 53 , 0  , 0  , 0  ), //      {W:r16, R:r16|m16, R:i8|i16}
  ISIGNATURE(3, 1, 1, 0, 13 , 39 , 54 , 0  , 0  , 0  ), //      {W:r32, R:r32|m32, R:i8|i32}
  ISIGNATURE(3, 0, 1, 0, 19 , 16 , 54 , 0  , 0  , 0  ), //      {W:r64, R:r64|m64, R:i8|i32}
  ISIGNATURE(2, 1, 1, 0, 28 , 37 , 0  , 0  , 0  , 0  ), // #44  {X:r16|m16, X:r16}
  ISIGNATURE(2, 1, 1, 0, 33 , 38 , 0  , 0  , 0  , 0  ), //      {X:r32|m32, X:r32}
  ISIGNATURE(2, 0, 1, 0, 34 , 40 , 0  , 0  , 0  , 0  ), //      {X:r64|m64, X:r64}
  ISIGNATURE(2, 1, 1, 0, 37 , 28 , 0  , 0  , 0  , 0  ), //      {X:r16, X:r16|m16}
  ISIGNATURE(2, 1, 1, 0, 38 , 33 , 0  , 0  , 0  , 0  ), //      {X:r32, X:r32|m32}
  ISIGNATURE(2, 0, 1, 0, 40 , 34 , 0  , 0  , 0  , 0  ), //      {X:r64, X:r64|m64}
  ISIGNATURE(2, 1, 1, 0, 31 , 35 , 0  , 0  , 0  , 0  ), //      {X:r8lo|r8hi|m8, X:r8lo|r8hi}
  ISIGNATURE(2, 1, 1, 0, 35 , 31 , 0  , 0  , 0  , 0  ), //      {X:r8lo|r8hi, X:r8lo|r8hi|m8}
  ISIGNATURE(2, 1, 1, 0, 17 , 55 , 0  , 0  , 0  , 0  ), // #52  {W:r16, R:m16}
  ISIGNATURE(2, 1, 1, 0, 13 , 56 , 0  , 0  , 0  , 0  ), //      {W:r32, R:m32}
  ISIGNATURE(2, 0, 1, 0, 19 , 57 , 0  , 0  , 0  , 0  ), //      {W:r64, R:m64}
  ISIGNATURE(2, 1, 1, 0, 58 , 43 , 0  , 0  , 0  , 0  ), //      {W:m16, R:r16}
  ISIGNATURE(2, 1, 1, 0, 59 , 6  , 0  , 0  , 0  , 0  ), // #56  {W:m32, R:r32}
  ISIGNATURE(2, 0, 1, 0, 60 , 25 , 0  , 0  , 0  , 0  ), //      {W:m64, R:r64}
  ISIGNATURE(2, 1, 1, 0, 61 , 62 , 0  , 0  , 0  , 0  ), // #58  {W:mm, R:mm|m64|r64|xmm}
  ISIGNATURE(2, 1, 1, 0, 63 , 64 , 0  , 0  , 0  , 0  ), //      {W:mm|m64|r64|xmm, R:mm}
  ISIGNATURE(2, 0, 1, 0, 7  , 65 , 0  , 0  , 0  , 0  ), //      {W:r64|m64, R:xmm}
  ISIGNATURE(2, 0, 1, 0, 66 , 16 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:r64|m64}
  ISIGNATURE(2, 1, 1, 0, 66 , 67 , 0  , 0  , 0  , 0  ), // #62  {W:xmm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 68 , 65 , 0  , 0  , 0  , 0  ), //      {W:xmm|m64, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 69 , 0  , 0  , 0  , 0  ), // #64  {W:xmm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 70 , 65 , 0  , 0  , 0  , 0  ), //      {W:xmm|m128, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 71 , 72 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:ymm|m256}
  ISIGNATURE(2, 1, 1, 0, 73 , 74 , 0  , 0  , 0  , 0  ), //      {W:ymm|m256, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 75 , 76 , 0  , 0  , 0  , 0  ), // #68  {W:zmm, R:zmm|m512}
  ISIGNATURE(2, 1, 1, 0, 77 , 78 , 0  , 0  , 0  , 0  ), //      {W:zmm|m512, R:zmm}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 69 , 0  , 0  , 0  ), // #70  {W:xmm, R:xmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 66 , 69 , 27 , 0  , 0  , 0  ), // #71  {W:xmm, R:xmm|m128, R:i8}
  ISIGNATURE(3, 1, 1, 0, 71 , 74 , 72 , 0  , 0  , 0  ), // #72  {W:ymm, R:ymm, R:ymm|m256}
  ISIGNATURE(3, 1, 1, 0, 71 , 72 , 27 , 0  , 0  , 0  ), // #73  {W:ymm, R:ymm|m256, R:i8}
  ISIGNATURE(3, 1, 1, 0, 75 , 78 , 76 , 0  , 0  , 0  ), //      {W:zmm, R:zmm, R:zmm|m512}
  ISIGNATURE(3, 1, 1, 0, 75 , 76 , 27 , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512, R:i8}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 79 , 0  , 0  , 0  ), // #76  {W:xmm, R:xmm, R:i8|xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 71 , 74 , 79 , 0  , 0  , 0  ), //      {W:ymm, R:ymm, R:i8|xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 66 , 69 , 27 , 0  , 0  , 0  ), // #78  {W:xmm, R:xmm|m128, R:i8}
  ISIGNATURE(3, 1, 1, 0, 71 , 72 , 27 , 0  , 0  , 0  ), //      {W:ymm, R:ymm|m256, R:i8}
  ISIGNATURE(3, 1, 1, 0, 75 , 78 , 69 , 0  , 0  , 0  ), //      {W:zmm, R:zmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 75 , 76 , 27 , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512, R:i8}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 69 , 0  , 0  , 0  ), // #82  {W:xmm, R:xmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 66 , 69 , 27 , 0  , 0  , 0  ), //      {W:xmm, R:xmm|m128, R:i8}
  ISIGNATURE(3, 1, 1, 0, 71 , 74 , 69 , 0  , 0  , 0  ), //      {W:ymm, R:ymm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 71 , 72 , 27 , 0  , 0  , 0  ), //      {W:ymm, R:ymm|m256, R:i8}
  ISIGNATURE(3, 1, 1, 0, 75 , 78 , 69 , 0  , 0  , 0  ), //      {W:zmm, R:zmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 75 , 76 , 27 , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512, R:i8}
  ISIGNATURE(2, 1, 1, 0, 36 , 2  , 0  , 0  , 0  , 0  ), // #88  {R:r8lo|r8hi|m8, R:i8|r8lo|r8hi}
  ISIGNATURE(2, 1, 1, 0, 12 , 29 , 0  , 0  , 0  , 0  ), //      {R:r16|m16, R:i16|r16}
  ISIGNATURE(2, 1, 1, 0, 42 , 22 , 0  , 0  , 0  , 0  ), //      {R:r32|m32|r64|m64, R:i32}
  ISIGNATURE(2, 1, 1, 0, 39 , 6  , 0  , 0  , 0  , 0  ), //      {R:r32|m32, R:r32}
  ISIGNATURE(2, 0, 1, 0, 16 , 25 , 0  , 0  , 0  , 0  ), //      {R:r64|m64, R:r64}
  ISIGNATURE(3, 1, 1, 0, 66 , 80 , 65 , 0  , 0  , 0  ), // #93  {W:xmm, R:vm32x, R:xmm}
  ISIGNATURE(3, 1, 1, 0, 71 , 80 , 74 , 0  , 0  , 0  ), //      {W:ymm, R:vm32x, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 66 , 80 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:vm32x}
  ISIGNATURE(2, 1, 1, 0, 71 , 81 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:vm32y}
  ISIGNATURE(2, 1, 1, 0, 75 , 82 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:vm32z}
  ISIGNATURE(3, 1, 1, 0, 66 , 80 , 65 , 0  , 0  , 0  ), // #98  {W:xmm, R:vm32x, R:xmm}
  ISIGNATURE(3, 1, 1, 0, 71 , 81 , 74 , 0  , 0  , 0  ), //      {W:ymm, R:vm32y, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 66 , 80 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:vm32x}
  ISIGNATURE(2, 1, 1, 0, 71 , 81 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:vm32y}
  ISIGNATURE(2, 1, 1, 0, 75 , 82 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:vm32z}
  ISIGNATURE(3, 1, 1, 0, 66 , 83 , 65 , 0  , 0  , 0  ), // #103 {W:xmm, R:vm64x, R:xmm}
  ISIGNATURE(3, 1, 1, 0, 71 , 84 , 74 , 0  , 0  , 0  ), //      {W:ymm, R:vm64y, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 66 , 83 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:vm64x}
  ISIGNATURE(2, 1, 1, 0, 71 , 84 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:vm64y}
  ISIGNATURE(2, 1, 1, 0, 75 , 85 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:vm64z}
  ISIGNATURE(3, 1, 1, 1, 31 , 32 , 86 , 0  , 0  , 0  ), // #108 {X:r8lo|r8hi|m8, R:r8lo|r8hi, R:<al>}
  ISIGNATURE(3, 1, 1, 1, 28 , 43 , 87 , 0  , 0  , 0  ), //      {X:r16|m16, R:r16, R:<ax>}
  ISIGNATURE(3, 1, 1, 1, 33 , 6  , 88 , 0  , 0  , 0  ), //      {X:r32|m32, R:r32, R:<eax>}
  ISIGNATURE(3, 0, 1, 1, 34 , 25 , 89 , 0  , 0  , 0  ), //      {X:r64|m64, R:r64, R:<rax>}
  ISIGNATURE(2, 1, 1, 1, 44 , 36 , 0  , 0  , 0  , 0  ), // #112 {X:<ax>, R:r8lo|r8hi|m8}
  ISIGNATURE(3, 1, 1, 2, 44 , 90 , 12 , 0  , 0  , 0  ), //      {X:<ax>, X:<dx>, R:r16|m16}
  ISIGNATURE(3, 1, 1, 2, 47 , 91 , 39 , 0  , 0  , 0  ), //      {X:<eax>, X:<edx>, R:r32|m32}
  ISIGNATURE(3, 0, 1, 2, 49 , 92 , 16 , 0  , 0  , 0  ), //      {X:<rax>, X:<rdx>, R:r64|m64}
  ISIGNATURE(2, 1, 1, 1, 44 , 36 , 0  , 0  , 0  , 0  ), // #116 {X:<ax>, R:r8lo|r8hi|m8}
  ISIGNATURE(3, 1, 1, 2, 90 , 44 , 12 , 0  , 0  , 0  ), //      {X:<dx>, X:<ax>, R:r16|m16}
  ISIGNATURE(3, 1, 1, 2, 91 , 47 , 39 , 0  , 0  , 0  ), //      {X:<edx>, X:<eax>, R:r32|m32}
  ISIGNATURE(3, 0, 1, 2, 92 , 49 , 16 , 0  , 0  , 0  ), //      {X:<rdx>, X:<rax>, R:r64|m64}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 65 , 69 , 0  , 0  ), // #120 {W:xmm, R:xmm, R:xmm, R:xmm|m128}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 69 , 65 , 0  , 0  ), // #121 {W:xmm, R:xmm, R:xmm|m128, R:xmm}
  ISIGNATURE(4, 1, 1, 0, 71 , 74 , 74 , 72 , 0  , 0  ), //      {W:ymm, R:ymm, R:ymm, R:ymm|m256}
  ISIGNATURE(4, 1, 1, 0, 71 , 74 , 72 , 74 , 0  , 0  ), //      {W:ymm, R:ymm, R:ymm|m256, R:ymm}
  ISIGNATURE(3, 1, 1, 0, 66 , 93 , 65 , 0  , 0  , 0  ), // #124 {W:xmm, R:vm64x|vm64y, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 83 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:vm64x}
  ISIGNATURE(2, 1, 1, 0, 71 , 84 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:vm64y}
  ISIGNATURE(2, 1, 1, 0, 75 , 85 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:vm64z}
  ISIGNATURE(3, 1, 1, 0, 94 , 65 , 65 , 0  , 0  , 0  ), // #128 {W:m128, R:xmm, R:xmm}
  ISIGNATURE(3, 1, 1, 0, 95 , 74 , 74 , 0  , 0  , 0  ), //      {W:m256, R:ymm, R:ymm}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 96 , 0  , 0  , 0  ), //      {W:xmm, R:xmm, R:m128}
  ISIGNATURE(3, 1, 1, 0, 71 , 74 , 97 , 0  , 0  , 0  ), //      {W:ymm, R:ymm, R:m256}
  ISIGNATURE(5, 1, 1, 0, 66 , 65 , 69 , 65 , 98 , 0  ), // #132 {W:xmm, R:xmm, R:xmm|m128, R:xmm, R:i4}
  ISIGNATURE(5, 1, 1, 0, 66 , 65 , 65 , 69 , 98 , 0  ), //      {W:xmm, R:xmm, R:xmm, R:xmm|m128, R:i4}
  ISIGNATURE(5, 1, 1, 0, 71 , 74 , 72 , 74 , 98 , 0  ), //      {W:ymm, R:ymm, R:ymm|m256, R:ymm, R:i4}
  ISIGNATURE(5, 1, 1, 0, 71 , 74 , 74 , 72 , 98 , 0  ), //      {W:ymm, R:ymm, R:ymm, R:ymm|m256, R:i4}
  ISIGNATURE(3, 1, 1, 0, 71 , 72 , 27 , 0  , 0  , 0  ), // #136 {W:ymm, R:ymm|m256, R:i8}
  ISIGNATURE(3, 1, 1, 0, 71 , 74 , 72 , 0  , 0  , 0  ), // #137 {W:ymm, R:ymm, R:ymm|m256}
  ISIGNATURE(3, 1, 1, 0, 75 , 78 , 76 , 0  , 0  , 0  ), //      {W:zmm, R:zmm, R:zmm|m512}
  ISIGNATURE(3, 1, 1, 0, 75 , 76 , 27 , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512, R:i8}
  ISIGNATURE(2, 1, 1, 0, 31 , 35 , 0  , 0  , 0  , 0  ), // #140 {X:r8lo|r8hi|m8, X:r8lo|r8hi}
  ISIGNATURE(2, 1, 1, 0, 28 , 37 , 0  , 0  , 0  , 0  ), //      {X:r16|m16, X:r16}
  ISIGNATURE(2, 1, 1, 0, 33 , 38 , 0  , 0  , 0  , 0  ), //      {X:r32|m32, X:r32}
  ISIGNATURE(2, 0, 1, 0, 34 , 40 , 0  , 0  , 0  , 0  ), //      {X:r64|m64, X:r64}
  ISIGNATURE(2, 1, 1, 0, 12 , 99 , 0  , 0  , 0  , 0  ), // #144 {R:r16|m16, R:r16|i8}
  ISIGNATURE(2, 1, 1, 0, 39 , 100, 0  , 0  , 0  , 0  ), //      {R:r32|m32, R:r32|i8}
  ISIGNATURE(2, 0, 1, 0, 16 , 101, 0  , 0  , 0  , 0  ), //      {R:r64|m64, R:r64|i8}
  ISIGNATURE(2, 1, 1, 0, 28 , 99 , 0  , 0  , 0  , 0  ), // #147 {X:r16|m16, R:r16|i8}
  ISIGNATURE(2, 1, 1, 0, 33 , 100, 0  , 0  , 0  , 0  ), //      {X:r32|m32, R:r32|i8}
  ISIGNATURE(2, 0, 1, 0, 34 , 101, 0  , 0  , 0  , 0  ), //      {X:r64|m64, R:r64|i8}
  ISIGNATURE(1, 1, 1, 0, 102, 0  , 0  , 0  , 0  , 0  ), // #150 {X:m32|m64}
  ISIGNATURE(2, 1, 1, 0, 103, 104, 0  , 0  , 0  , 0  ), //      {X:fp0, R:fp}
  ISIGNATURE(2, 1, 1, 0, 105, 106, 0  , 0  , 0  , 0  ), //      {X:fp, R:fp0}
  ISIGNATURE(2, 1, 1, 0, 17 , 12 , 0  , 0  , 0  , 0  ), // #153 {W:r16, R:r16|m16}
  ISIGNATURE(2, 1, 1, 0, 13 , 39 , 0  , 0  , 0  , 0  ), // #154 {W:r32, R:r32|m32}
  ISIGNATURE(2, 0, 1, 0, 19 , 16 , 0  , 0  , 0  , 0  ), //      {W:r64, R:r64|m64}
  ISIGNATURE(2, 1, 1, 2, 107, 107, 0  , 0  , 0  , 0  ), // #156 {X:<zdi>, X:<zsi>}
  ISIGNATURE(2, 1, 1, 0, 66 , 67 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 60 , 65 , 0  , 0  , 0  , 0  ), // #158 {W:m64, R:xmm}
  ISIGNATURE(3, 1, 1, 0, 28 , 43 , 108, 0  , 0  , 0  ), // #159 {X:r16|m16, R:r16, R:i8|cl}
  ISIGNATURE(3, 1, 1, 0, 33 , 6  , 108, 0  , 0  , 0  ), //      {X:r32|m32, R:r32, R:i8|cl}
  ISIGNATURE(3, 0, 1, 0, 34 , 25 , 108, 0  , 0  , 0  ), //      {X:r64|m64, R:r64, R:i8|cl}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 69 , 0  , 0  , 0  ), // #162 {W:xmm, R:xmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 71 , 74 , 72 , 0  , 0  , 0  ), //      {W:ymm, R:ymm, R:ymm|m256}
  ISIGNATURE(3, 1, 1, 0, 75 , 78 , 76 , 0  , 0  , 0  ), //      {W:zmm, R:zmm, R:zmm|m512}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 69 , 27 , 0  , 0  ), // #165 {W:xmm, R:xmm, R:xmm|m128, R:i8}
  ISIGNATURE(4, 1, 1, 0, 71 , 74 , 72 , 27 , 0  , 0  ), // #166 {W:ymm, R:ymm, R:ymm|m256, R:i8}
  ISIGNATURE(4, 1, 1, 0, 75 , 78 , 76 , 27 , 0  , 0  ), //      {W:zmm, R:zmm, R:zmm|m512, R:i8}
  ISIGNATURE(4, 1, 1, 0, 109, 65 , 69 , 27 , 0  , 0  ), // #168 {W:xmm|k, R:xmm, R:xmm|m128, R:i8}
  ISIGNATURE(4, 1, 1, 0, 110, 74 , 72 , 27 , 0  , 0  ), //      {W:ymm|k, R:ymm, R:ymm|m256, R:i8}
  ISIGNATURE(4, 1, 1, 0, 111, 78 , 76 , 27 , 0  , 0  ), //      {W:k, R:zmm, R:zmm|m512, R:i8}
  ISIGNATURE(2, 1, 1, 0, 70 , 65 , 0  , 0  , 0  , 0  ), // #171 {W:xmm|m128, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 73 , 74 , 0  , 0  , 0  , 0  ), //      {W:ymm|m256, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 77 , 78 , 0  , 0  , 0  , 0  ), //      {W:zmm|m512, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 67 , 0  , 0  , 0  , 0  ), // #174 {W:xmm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 71 , 69 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 75 , 72 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:ymm|m256}
  ISIGNATURE(2, 1, 1, 0, 66 , 69 , 0  , 0  , 0  , 0  ), // #177 {W:xmm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 71 , 72 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:ymm|m256}
  ISIGNATURE(2, 1, 1, 0, 75 , 76 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512}
  ISIGNATURE(2, 1, 1, 0, 66 , 112, 0  , 0  , 0  , 0  ), // #180 {W:xmm, R:xmm|m128|ymm|m256|m64}
  ISIGNATURE(2, 1, 1, 0, 71 , 69 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 75 , 72 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:ymm|m256}
  ISIGNATURE(3, 1, 1, 0, 68 , 65 , 27 , 0  , 0  , 0  ), // #183 {W:xmm|m64, R:xmm, R:i8}
  ISIGNATURE(3, 1, 1, 0, 70 , 74 , 27 , 0  , 0  , 0  ), // #184 {W:xmm|m128, R:ymm, R:i8}
  ISIGNATURE(3, 1, 1, 0, 73 , 78 , 27 , 0  , 0  , 0  ), // #185 {W:ymm|m256, R:zmm, R:i8}
  ISIGNATURE(4, 1, 1, 0, 113, 65 , 69 , 27 , 0  , 0  ), // #186 {X:xmm, R:xmm, R:xmm|m128, R:i8}
  ISIGNATURE(4, 1, 1, 0, 114, 74 , 72 , 27 , 0  , 0  ), //      {X:ymm, R:ymm, R:ymm|m256, R:i8}
  ISIGNATURE(4, 1, 1, 0, 115, 78 , 76 , 27 , 0  , 0  ), //      {X:zmm, R:zmm, R:zmm|m512, R:i8}
  ISIGNATURE(3, 1, 1, 0, 113, 65 , 69 , 0  , 0  , 0  ), // #189 {X:xmm, R:xmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 114, 74 , 72 , 0  , 0  , 0  ), //      {X:ymm, R:ymm, R:ymm|m256}
  ISIGNATURE(3, 1, 1, 0, 115, 78 , 76 , 0  , 0  , 0  ), //      {X:zmm, R:zmm, R:zmm|m512}
  ISIGNATURE(3, 1, 1, 0, 66 , 69 , 27 , 0  , 0  , 0  ), // #192 {W:xmm, R:xmm|m128, R:i8}
  ISIGNATURE(3, 1, 1, 0, 71 , 72 , 27 , 0  , 0  , 0  ), //      {W:ymm, R:ymm|m256, R:i8}
  ISIGNATURE(3, 1, 1, 0, 75 , 76 , 27 , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512, R:i8}
  ISIGNATURE(2, 1, 1, 0, 66 , 67 , 0  , 0  , 0  , 0  ), // #195 {W:xmm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 71 , 72 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:ymm|m256}
  ISIGNATURE(2, 1, 1, 0, 75 , 76 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:zmm|m512}
  ISIGNATURE(2, 1, 1, 0, 94 , 65 , 0  , 0  , 0  , 0  ), // #198 {W:m128, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 95 , 74 , 0  , 0  , 0  , 0  ), //      {W:m256, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 116, 78 , 0  , 0  , 0  , 0  ), //      {W:m512, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 96 , 0  , 0  , 0  , 0  ), // #201 {W:xmm, R:m128}
  ISIGNATURE(2, 1, 1, 0, 71 , 97 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:m256}
  ISIGNATURE(2, 1, 1, 0, 75 , 117, 0  , 0  , 0  , 0  ), //      {W:zmm, R:m512}
  ISIGNATURE(2, 0, 1, 0, 7  , 65 , 0  , 0  , 0  , 0  ), // #204 {W:r64|m64, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 118, 0  , 0  , 0  , 0  ), //      {W:xmm, R:xmm|m64|r64}
  ISIGNATURE(2, 1, 1, 0, 68 , 65 , 0  , 0  , 0  , 0  ), //      {W:xmm|m64, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 60 , 65 , 0  , 0  , 0  , 0  ), // #207 {W:m64, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 57 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:m64}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 65 , 0  , 0  , 0  ), // #209 {W:xmm, R:xmm, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 119, 65 , 0  , 0  , 0  , 0  ), // #210 {W:m32|m64, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 120, 0  , 0  , 0  , 0  ), //      {W:xmm, R:m32|m64}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 65 , 0  , 0  , 0  ), //      {W:xmm, R:xmm, R:xmm}
  ISIGNATURE(4, 1, 1, 0, 111, 65 , 69 , 27 , 0  , 0  ), // #213 {W:k, R:xmm, R:xmm|m128, R:i8}
  ISIGNATURE(4, 1, 1, 0, 111, 74 , 72 , 27 , 0  , 0  ), //      {W:k, R:ymm, R:ymm|m256, R:i8}
  ISIGNATURE(4, 1, 1, 0, 111, 78 , 76 , 27 , 0  , 0  ), //      {W:k, R:zmm, R:zmm|m512, R:i8}
  ISIGNATURE(3, 1, 1, 0, 109, 65 , 69 , 0  , 0  , 0  ), // #216 {W:xmm|k, R:xmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 110, 74 , 72 , 0  , 0  , 0  ), //      {W:ymm|k, R:ymm, R:ymm|m256}
  ISIGNATURE(3, 1, 1, 0, 111, 78 , 76 , 0  , 0  , 0  ), //      {W:k, R:zmm, R:zmm|m512}
  ISIGNATURE(2, 1, 1, 0, 121, 65 , 0  , 0  , 0  , 0  ), // #219 {W:xmm|m32, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 68 , 74 , 0  , 0  , 0  , 0  ), //      {W:xmm|m64, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 70 , 78 , 0  , 0  , 0  , 0  ), //      {W:xmm|m128, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 68 , 65 , 0  , 0  , 0  , 0  ), // #222 {W:xmm|m64, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 70 , 74 , 0  , 0  , 0  , 0  ), //      {W:xmm|m128, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 73 , 78 , 0  , 0  , 0  , 0  ), //      {W:ymm|m256, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 122, 65 , 0  , 0  , 0  , 0  ), // #225 {W:xmm|m16, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 121, 74 , 0  , 0  , 0  , 0  ), //      {W:xmm|m32, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 68 , 78 , 0  , 0  , 0  , 0  ), //      {W:xmm|m64, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 123, 0  , 0  , 0  , 0  ), // #228 {W:xmm, R:xmm|m32}
  ISIGNATURE(2, 1, 1, 0, 71 , 67 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 75 , 69 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 66 , 124, 0  , 0  , 0  , 0  ), // #231 {W:xmm, R:xmm|m16}
  ISIGNATURE(2, 1, 1, 0, 71 , 123, 0  , 0  , 0  , 0  ), //      {W:ymm, R:xmm|m32}
  ISIGNATURE(2, 1, 1, 0, 75 , 67 , 0  , 0  , 0  , 0  ), // #233 {W:zmm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 66 , 125, 0  , 0  , 0  , 0  ), // #234 {W:xmm, R:xmm|m64|m32}
  ISIGNATURE(2, 1, 1, 0, 71 , 126, 0  , 0  , 0  , 0  ), //      {W:ymm, R:xmm|m128|m64}
  ISIGNATURE(2, 1, 1, 0, 75 , 69 , 0  , 0  , 0  , 0  ), //      {W:zmm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 127, 65 , 0  , 0  , 0  , 0  ), // #237 {W:vm32x, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 128, 74 , 0  , 0  , 0  , 0  ), //      {W:vm32y, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 129, 78 , 0  , 0  , 0  , 0  ), //      {W:vm32z, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 130, 65 , 0  , 0  , 0  , 0  ), // #240 {W:vm64x, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 131, 74 , 0  , 0  , 0  , 0  ), //      {W:vm64y, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 132, 78 , 0  , 0  , 0  , 0  ), //      {W:vm64z, R:zmm}
  ISIGNATURE(3, 1, 1, 0, 111, 65 , 69 , 0  , 0  , 0  ), // #243 {W:k, R:xmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 111, 74 , 72 , 0  , 0  , 0  ), //      {W:k, R:ymm, R:ymm|m256}
  ISIGNATURE(3, 1, 1, 0, 111, 78 , 76 , 0  , 0  , 0  ), //      {W:k, R:zmm, R:zmm|m512}
  ISIGNATURE(3, 1, 1, 0, 13 , 6  , 39 , 0  , 0  , 0  ), // #246 {W:r32, R:r32, R:r32|m32}
  ISIGNATURE(3, 0, 1, 0, 19 , 25 , 16 , 0  , 0  , 0  ), //      {W:r64, R:r64, R:r64|m64}
  ISIGNATURE(3, 1, 1, 0, 13 , 39 , 6  , 0  , 0  , 0  ), // #248 {W:r32, R:r32|m32, R:r32}
  ISIGNATURE(3, 0, 1, 0, 19 , 16 , 25 , 0  , 0  , 0  ), //      {W:r64, R:r64|m64, R:r64}
  ISIGNATURE(1, 1, 1, 0, 133, 0  , 0  , 0  , 0  , 0  ), // #250 {X:rel32|r64|m64}
  ISIGNATURE(1, 1, 0, 0, 39 , 0  , 0  , 0  , 0  , 0  ), //      {R:r32|m32}
  ISIGNATURE(2, 1, 1, 2, 107, 107, 0  , 0  , 0  , 0  ), // #252 {X:<zsi>, X:<zdi>}
  ISIGNATURE(3, 1, 1, 0, 113, 67 , 27 , 0  , 0  , 0  ), //      {X:xmm, R:xmm|m64, R:i8}
  ISIGNATURE(2, 1, 1, 0, 38 , 134, 0  , 0  , 0  , 0  ), // #254 {X:r32, R:r8lo|r8hi|m8|r16|m16|r32|m32}
  ISIGNATURE(2, 0, 1, 0, 40 , 135, 0  , 0  , 0  , 0  ), //      {X:r64, R:r8lo|r8hi|m8|r64|m64}
  ISIGNATURE(1, 1, 0, 0, 136, 0  , 0  , 0  , 0  , 0  ), // #256 {X:r16|r32}
  ISIGNATURE(1, 1, 1, 0, 26 , 0  , 0  , 0  , 0  , 0  ), // #257 {X:r8lo|r8hi|m8|r16|m16|r32|m32|r64|m64}
  ISIGNATURE(3, 1, 1, 0, 113, 27 , 27 , 0  , 0  , 0  ), // #258 {X:xmm, R:i8, R:i8}
  ISIGNATURE(2, 1, 1, 0, 113, 65 , 0  , 0  , 0  , 0  ), //      {X:xmm, R:xmm}
  ISIGNATURE(0, 1, 1, 0, 0  , 0  , 0  , 0  , 0  , 0  ), // #260 {}
  ISIGNATURE(1, 1, 1, 0, 105, 0  , 0  , 0  , 0  , 0  ), // #261 {X:fp}
  ISIGNATURE(0, 1, 1, 0, 0  , 0  , 0  , 0  , 0  , 0  ), // #262 {}
  ISIGNATURE(1, 1, 1, 0, 137, 0  , 0  , 0  , 0  , 0  ), // #263 {X:m32|m64|fp}
  ISIGNATURE(2, 1, 1, 0, 113, 65 , 0  , 0  , 0  , 0  ), // #264 {X:xmm, R:xmm}
  ISIGNATURE(4, 1, 1, 0, 113, 65 , 27 , 27 , 0  , 0  ), //      {X:xmm, R:xmm, R:i8, R:i8}
  ISIGNATURE(2, 1, 0, 0, 138, 139, 0  , 0  , 0  , 0  ), // #266 {R:cx|ecx, R:rel8}
  ISIGNATURE(2, 0, 1, 0, 140, 139, 0  , 0  , 0  , 0  ), //      {R:ecx|rcx, R:rel8}
  ISIGNATURE(1, 1, 1, 0, 141, 0  , 0  , 0  , 0  , 0  ), // #268 {X:rel8|rel32|r64|m64}
  ISIGNATURE(1, 1, 0, 0, 39 , 0  , 0  , 0  , 0  , 0  ), //      {R:r32|m32}
  ISIGNATURE(2, 1, 1, 0, 111, 142, 0  , 0  , 0  , 0  ), // #270 {W:k, R:k|m8|r32|r64|r8lo|r8hi|r16}
  ISIGNATURE(2, 1, 1, 0, 143, 144, 0  , 0  , 0  , 0  ), //      {W:m8|r32|r64|r8lo|r8hi|r16, R:k}
  ISIGNATURE(2, 1, 1, 0, 111, 145, 0  , 0  , 0  , 0  ), // #272 {W:k, R:k|m32|r32|r64}
  ISIGNATURE(2, 1, 1, 0, 146, 144, 0  , 0  , 0  , 0  ), //      {W:m32|r32|r64, R:k}
  ISIGNATURE(2, 1, 1, 0, 111, 147, 0  , 0  , 0  , 0  ), // #274 {W:k, R:k|m64|r64}
  ISIGNATURE(2, 1, 1, 0, 7  , 144, 0  , 0  , 0  , 0  ), //      {W:m64|r64, R:k}
  ISIGNATURE(2, 1, 1, 0, 111, 148, 0  , 0  , 0  , 0  ), // #276 {W:k, R:k|m16|r32|r64|r16}
  ISIGNATURE(2, 1, 1, 0, 149, 144, 0  , 0  , 0  , 0  ), //      {W:m16|r32|r64|r16, R:k}
  ISIGNATURE(2, 1, 1, 0, 150, 151, 0  , 0  , 0  , 0  ), // #278 {W:mm|xmm, R:r32|m32|r64}
  ISIGNATURE(2, 1, 1, 0, 146, 152, 0  , 0  , 0  , 0  ), //      {W:r32|m32|r64, R:mm|xmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 123, 0  , 0  , 0  , 0  ), // #280 {W:xmm, R:xmm|m32}
  ISIGNATURE(2, 1, 1, 0, 59 , 65 , 0  , 0  , 0  , 0  ), // #281 {W:m32, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 153, 36 , 0  , 0  , 0  , 0  ), // #282 {W:r16|r32|r64, R:r8lo|r8hi|m8}
  ISIGNATURE(2, 1, 1, 0, 154, 12 , 0  , 0  , 0  , 0  ), //      {W:r32|r64, R:r16|m16}
  ISIGNATURE(4, 1, 1, 1, 13 , 13 , 39 , 155, 0  , 0  ), // #284 {W:r32, W:r32, R:r32|m32, R:<edx>}
  ISIGNATURE(4, 0, 1, 1, 19 , 19 , 16 , 156, 0  , 0  ), //      {W:r64, W:r64, R:r64|m64, R:<rdx>}
  ISIGNATURE(0, 1, 1, 0, 0  , 0  , 0  , 0  , 0  , 0  ), // #286 {}
  ISIGNATURE(1, 1, 1, 0, 157, 0  , 0  , 0  , 0  , 0  ), //      {R:r16|m16|r32|m32}
  ISIGNATURE(2, 1, 1, 0, 158, 159, 0  , 0  , 0  , 0  ), // #288 {X:mm, R:mm|m64}
  ISIGNATURE(2, 1, 1, 0, 113, 69 , 0  , 0  , 0  , 0  ), // #289 {X:xmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 158, 159, 27 , 0  , 0  , 0  ), // #290 {X:mm, R:mm|m64, R:i8}
  ISIGNATURE(3, 1, 1, 0, 113, 69 , 27 , 0  , 0  , 0  ), // #291 {X:xmm, R:xmm|m128, R:i8}
  ISIGNATURE(3, 1, 1, 0, 154, 64 , 27 , 0  , 0  , 0  ), // #292 {W:r32|r64, R:mm, R:i8}
  ISIGNATURE(3, 1, 1, 0, 149, 65 , 27 , 0  , 0  , 0  ), // #293 {W:r32|r64|m16|r16, R:xmm, R:i8}
  ISIGNATURE(1, 1, 1, 0, 160, 0  , 0  , 0  , 0  , 0  ), // #294 {W:r16|m16|r64|m64|fs|gs}
  ISIGNATURE(1, 1, 0, 0, 161, 0  , 0  , 0  , 0  , 0  ), //      {W:r32|m32|ds|es|ss}
  ISIGNATURE(2, 1, 1, 0, 61 , 159, 0  , 0  , 0  , 0  ), // #296 {W:mm, R:mm|m64}
  ISIGNATURE(2, 1, 1, 0, 66 , 69 , 0  , 0  , 0  , 0  ), //      {W:xmm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 158, 162, 0  , 0  , 0  , 0  ), // #298 {X:mm, R:i8|mm|m64}
  ISIGNATURE(2, 1, 1, 0, 113, 79 , 0  , 0  , 0  , 0  ), //      {X:xmm, R:i8|xmm|m128}
  ISIGNATURE(1, 1, 1, 0, 163, 0  , 0  , 0  , 0  , 0  ), // #300 {X:r16|m16|r64|m64|i8|i16|i32|fs|gs}
  ISIGNATURE(1, 1, 0, 0, 164, 0  , 0  , 0  , 0  , 0  ), //      {R:r32|m32|cs|ss|ds|es}
  ISIGNATURE(0, 1, 1, 0, 0  , 0  , 0  , 0  , 0  , 0  ), // #302 {}
  ISIGNATURE(1, 1, 1, 0, 165, 0  , 0  , 0  , 0  , 0  ), //      {X:i16}
  ISIGNATURE(3, 1, 1, 0, 13 , 39 , 27 , 0  , 0  , 0  ), // #304 {W:r32, R:r32|m32, R:i8}
  ISIGNATURE(3, 0, 1, 0, 19 , 16 , 27 , 0  , 0  , 0  ), //      {W:r64, R:r64|m64, R:i8}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 69 , 65 , 0  , 0  ), // #306 {W:xmm, R:xmm, R:xmm|m128, R:xmm}
  ISIGNATURE(4, 1, 1, 0, 71 , 74 , 72 , 74 , 0  , 0  ), //      {W:ymm, R:ymm, R:ymm|m256, R:ymm}
  ISIGNATURE(2, 1, 1, 0, 66 , 166, 0  , 0  , 0  , 0  ), // #308 {W:xmm, R:xmm|m128|ymm|m256}
  ISIGNATURE(2, 1, 1, 0, 71 , 76 , 0  , 0  , 0  , 0  ), //      {W:ymm, R:zmm|m512}
  ISIGNATURE(2, 1, 1, 0, 154, 123, 0  , 0  , 0  , 0  ), // #310 {W:r32|r64, R:xmm|m32}
  ISIGNATURE(2, 0, 1, 0, 19 , 67 , 0  , 0  , 0  , 0  ), //      {W:r64, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 13 , 123, 0  , 0  , 0  , 0  ), // #312 {W:r32, R:xmm|m32}
  ISIGNATURE(2, 0, 1, 0, 19 , 67 , 0  , 0  , 0  , 0  ), //      {W:r64, R:xmm|m64}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 65 , 67 , 0  , 0  ), // #314 {W:xmm, R:xmm, R:xmm, R:xmm|m64}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 67 , 65 , 0  , 0  ), //      {W:xmm, R:xmm, R:xmm|m64, R:xmm}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 65 , 123, 0  , 0  ), // #316 {W:xmm, R:xmm, R:xmm, R:xmm|m32}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 123, 65 , 0  , 0  ), //      {W:xmm, R:xmm, R:xmm|m32, R:xmm}
  ISIGNATURE(4, 1, 1, 0, 71 , 74 , 69 , 27 , 0  , 0  ), // #318 {W:ymm, R:ymm, R:xmm|m128, R:i8}
  ISIGNATURE(4, 1, 1, 0, 75 , 78 , 69 , 27 , 0  , 0  ), //      {W:zmm, R:zmm, R:xmm|m128, R:i8}
  ISIGNATURE(2, 1, 1, 0, 146, 65 , 0  , 0  , 0  , 0  ), // #320 {W:r32|m32|r64, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 151, 0  , 0  , 0  , 0  ), //      {W:xmm, R:r32|m32|r64}
  ISIGNATURE(2, 1, 1, 0, 60 , 65 , 0  , 0  , 0  , 0  ), // #322 {W:m64, R:xmm}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 57 , 0  , 0  , 0  ), //      {W:xmm, R:xmm, R:m64}
  ISIGNATURE(2, 1, 1, 0, 167, 168, 0  , 0  , 0  , 0  ), // #324 {W:xmm|ymm|zmm, R:xmm|m8}
  ISIGNATURE(2, 1, 1, 0, 167, 169, 0  , 0  , 0  , 0  ), //      {W:xmm|ymm|zmm, R:r32|r64}
  ISIGNATURE(2, 1, 1, 0, 167, 123, 0  , 0  , 0  , 0  ), // #326 {W:xmm|ymm|zmm, R:xmm|m32}
  ISIGNATURE(2, 1, 1, 0, 167, 169, 0  , 0  , 0  , 0  ), //      {W:xmm|ymm|zmm, R:r32|r64}
  ISIGNATURE(2, 1, 1, 0, 167, 124, 0  , 0  , 0  , 0  ), // #328 {W:xmm|ymm|zmm, R:xmm|m16}
  ISIGNATURE(2, 1, 1, 0, 167, 169, 0  , 0  , 0  , 0  ), //      {W:xmm|ymm|zmm, R:r32|r64}
  ISIGNATURE(3, 1, 1, 0, 66 , 170, 27 , 0  , 0  , 0  ), // #330 {W:xmm, R:r32|m8|r64|r8lo|r8hi|r16, R:i8}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 170, 27 , 0  , 0  ), //      {W:xmm, R:xmm, R:r32|m8|r64|r8lo|r8hi|r16, R:i8}
  ISIGNATURE(3, 1, 1, 0, 66 , 151, 27 , 0  , 0  , 0  ), // #332 {W:xmm, R:r32|m32|r64, R:i8}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 151, 27 , 0  , 0  ), //      {W:xmm, R:xmm, R:r32|m32|r64, R:i8}
  ISIGNATURE(3, 0, 1, 0, 66 , 16 , 27 , 0  , 0  , 0  ), // #334 {W:xmm, R:r64|m64, R:i8}
  ISIGNATURE(4, 0, 1, 0, 66 , 65 , 16 , 27 , 0  , 0  ), //      {W:xmm, R:xmm, R:r64|m64, R:i8}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 69 , 0  , 0  , 0  ), // #336 {W:xmm, R:xmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 66 , 69 , 171, 0  , 0  , 0  ), //      {W:xmm, R:xmm|m128, R:i8|xmm}
  ISIGNATURE(2, 1, 1, 0, 172, 65 , 0  , 0  , 0  , 0  ), // #338 {W:vm64x|vm64y, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 132, 74 , 0  , 0  , 0  , 0  ), //      {W:vm64z, R:ymm}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 69 , 0  , 0  , 0  ), // #340 {W:xmm, R:xmm, R:xmm|m128}
  ISIGNATURE(3, 1, 1, 0, 66 , 69 , 65 , 0  , 0  , 0  ), //      {W:xmm, R:xmm|m128, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 65 , 69 , 0  , 0  , 0  , 0  ), // #342 {R:xmm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 74 , 72 , 0  , 0  , 0  , 0  ), //      {R:ymm, R:ymm|m256}
  ISIGNATURE(2, 1, 1, 0, 127, 173, 0  , 0  , 0  , 0  ), // #344 {W:vm32x, R:xmm|ymm}
  ISIGNATURE(2, 1, 1, 0, 128, 78 , 0  , 0  , 0  , 0  ), //      {W:vm32y, R:zmm}
  ISIGNATURE(2, 1, 1, 0, 113, 67 , 0  , 0  , 0  , 0  ), // #346 {X:xmm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 113, 123, 0  , 0  , 0  , 0  ), // #347 {X:xmm, R:xmm|m32}
  ISIGNATURE(3, 1, 1, 1, 113, 69 , 174, 0  , 0  , 0  ), // #348 {X:xmm, R:xmm|m128, R:<xmm0>}
  ISIGNATURE(1, 1, 1, 0, 175, 0  , 0  , 0  , 0  , 0  ), // #349 {X:r32|r64}
  ISIGNATURE(1, 1, 1, 1, 44 , 0  , 0  , 0  , 0  , 0  ), // #350 {X:<ax>}
  ISIGNATURE(2, 1, 1, 2, 46 , 88 , 0  , 0  , 0  , 0  ), // #351 {W:<edx>, R:<eax>}
  ISIGNATURE(1, 0, 1, 1, 49 , 0  , 0  , 0  , 0  , 0  ), // #352 {X:<rax>}
  ISIGNATURE(1, 1, 1, 0, 176, 0  , 0  , 0  , 0  , 0  ), // #353 {R:mem}
  ISIGNATURE(1, 1, 1, 1, 177, 0  , 0  , 0  , 0  , 0  ), // #354 {R:<zax>}
  ISIGNATURE(3, 1, 1, 0, 113, 123, 27 , 0  , 0  , 0  ), // #355 {X:xmm, R:xmm|m32, R:i8}
  ISIGNATURE(5, 0, 1, 4, 178, 92 , 49 , 179, 180, 0  ), // #356 {X:m128, X:<rdx>, X:<rax>, R:<rcx>, R:<rbx>}
  ISIGNATURE(5, 1, 1, 4, 181, 91 , 47 , 182, 183, 0  ), // #357 {X:m64, X:<edx>, X:<eax>, R:<ecx>, R:<ebx>}
  ISIGNATURE(2, 1, 1, 0, 65 , 67 , 0  , 0  , 0  , 0  ), // #358 {R:xmm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 65 , 123, 0  , 0  , 0  , 0  ), // #359 {R:xmm, R:xmm|m32}
  ISIGNATURE(4, 1, 1, 4, 47 , 184, 185, 46 , 0  , 0  ), // #360 {X:<eax>, W:<ebx>, X:<ecx>, W:<edx>}
  ISIGNATURE(2, 0, 1, 2, 48 , 89 , 0  , 0  , 0  , 0  ), // #361 {W:<rdx>, R:<rax>}
  ISIGNATURE(2, 1, 1, 0, 61 , 69 , 0  , 0  , 0  , 0  ), // #362 {W:mm, R:xmm|m128}
  ISIGNATURE(2, 1, 1, 0, 66 , 159, 0  , 0  , 0  , 0  ), // #363 {W:xmm, R:mm|m64}
  ISIGNATURE(2, 1, 1, 0, 61 , 67 , 0  , 0  , 0  , 0  ), // #364 {W:mm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 154, 67 , 0  , 0  , 0  , 0  ), // #365 {W:r32|r64, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 66 , 42 , 0  , 0  , 0  , 0  ), // #366 {W:xmm, R:r32|m32|r64|m64}
  ISIGNATURE(2, 1, 1, 2, 45 , 87 , 0  , 0  , 0  , 0  ), // #367 {W:<dx>, R:<ax>}
  ISIGNATURE(1, 1, 1, 1, 47 , 0  , 0  , 0  , 0  , 0  ), // #368 {X:<eax>}
  ISIGNATURE(1, 1, 0, 1, 44 , 0  , 0  , 0  , 0  , 0  ), // #369 {X:<ax>}
  ISIGNATURE(2, 1, 1, 0, 165, 27 , 0  , 0  , 0  , 0  ), // #370 {X:i16, R:i8}
  ISIGNATURE(3, 1, 1, 0, 146, 65 , 27 , 0  , 0  , 0  ), // #371 {W:r32|m32|r64, R:xmm, R:i8}
  ISIGNATURE(1, 1, 1, 0, 186, 0  , 0  , 0  , 0  , 0  ), // #372 {X:m80}
  ISIGNATURE(1, 1, 1, 0, 187, 0  , 0  , 0  , 0  , 0  ), // #373 {X:m16|m32}
  ISIGNATURE(1, 1, 1, 0, 188, 0  , 0  , 0  , 0  , 0  ), // #374 {X:m16|m32|m64}
  ISIGNATURE(1, 1, 1, 0, 189, 0  , 0  , 0  , 0  , 0  ), // #375 {X:m32|m64|m80|fp}
  ISIGNATURE(1, 1, 1, 0, 190, 0  , 0  , 0  , 0  , 0  ), // #376 {X:m16}
  ISIGNATURE(1, 1, 1, 0, 191, 0  , 0  , 0  , 0  , 0  ), // #377 {X:mem}
  ISIGNATURE(1, 1, 1, 0, 192, 0  , 0  , 0  , 0  , 0  ), // #378 {X:ax|m16}
  ISIGNATURE(1, 0, 1, 0, 191, 0  , 0  , 0  , 0  , 0  ), // #379 {X:mem}
  ISIGNATURE(1, 1, 1, 0, 193, 0  , 0  , 0  , 0  , 0  ), // #380 {X:i8}
  ISIGNATURE(1, 1, 1, 0, 194, 0  , 0  , 0  , 0  , 0  ), // #381 {X:rel8|rel32}
  ISIGNATURE(1, 1, 1, 0, 195, 0  , 0  , 0  , 0  , 0  ), // #382 {X:rel8}
  ISIGNATURE(3, 1, 1, 0, 111, 144, 144, 0  , 0  , 0  ), // #383 {W:k, R:k, R:k}
  ISIGNATURE(2, 1, 1, 0, 111, 144, 0  , 0  , 0  , 0  ), // #384 {W:k, R:k}
  ISIGNATURE(2, 1, 1, 0, 144, 144, 0  , 0  , 0  , 0  ), // #385 {R:k, R:k}
  ISIGNATURE(3, 1, 1, 0, 111, 144, 27 , 0  , 0  , 0  ), // #386 {W:k, R:k, R:i8}
  ISIGNATURE(1, 1, 1, 1, 196, 0  , 0  , 0  , 0  , 0  ), // #387 {W:<ah>}
  ISIGNATURE(1, 1, 1, 0, 56 , 0  , 0  , 0  , 0  , 0  ), // #388 {R:m32}
  ISIGNATURE(2, 1, 1, 0, 153, 176, 0  , 0  , 0  , 0  ), // #389 {W:r16|r32|r64, R:mem}
  ISIGNATURE(3, 1, 1, 1, 113, 65 , 177, 0  , 0  , 0  ), // #390 {X:xmm, R:xmm, R:<zdi>}
  ISIGNATURE(3, 1, 1, 1, 158, 64 , 177, 0  , 0  , 0  ), // #391 {X:mm, R:mm, R:<zdi>}
  ISIGNATURE(2, 1, 1, 0, 61 , 65 , 0  , 0  , 0  , 0  ), // #392 {W:mm, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 66 , 65 , 0  , 0  , 0  , 0  ), // #393 {W:xmm, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 154, 65 , 0  , 0  , 0  , 0  ), // #394 {W:r32|r64, R:xmm}
  ISIGNATURE(2, 1, 1, 0, 60 , 64 , 0  , 0  , 0  , 0  ), // #395 {W:m64, R:mm}
  ISIGNATURE(2, 1, 1, 0, 66 , 64 , 0  , 0  , 0  , 0  ), // #396 {W:xmm, R:mm}
  ISIGNATURE(2, 0, 1, 0, 19 , 39 , 0  , 0  , 0  , 0  ), // #397 {W:r64, R:r32|m32}
  ISIGNATURE(6, 1, 1, 3, 65 , 69 , 27 , 197, 88 , 155), // #398 {R:xmm, R:xmm|m128, R:i8, W:<ecx>, R:<eax>, R:<edx>}
  ISIGNATURE(6, 1, 1, 3, 65 , 69 , 27 , 198, 88 , 155), // #399 {R:xmm, R:xmm|m128, R:i8, W:<xmm0>, R:<eax>, R:<edx>}
  ISIGNATURE(4, 1, 1, 1, 65 , 69 , 27 , 197, 0  , 0  ), // #400 {R:xmm, R:xmm|m128, R:i8, W:<ecx>}
  ISIGNATURE(4, 1, 1, 1, 65 , 69 , 27 , 198, 0  , 0  ), // #401 {R:xmm, R:xmm|m128, R:i8, W:<xmm0>}
  ISIGNATURE(3, 1, 1, 0, 143, 65 , 27 , 0  , 0  , 0  ), // #402 {W:r32|m8|r64|r8lo|r8hi|r16, R:xmm, R:i8}
  ISIGNATURE(3, 0, 1, 0, 7  , 65 , 27 , 0  , 0  , 0  ), // #403 {W:r64|m64, R:xmm, R:i8}
  ISIGNATURE(3, 1, 1, 0, 113, 170, 27 , 0  , 0  , 0  ), // #404 {X:xmm, R:r32|m8|r64|r8lo|r8hi|r16, R:i8}
  ISIGNATURE(3, 1, 1, 0, 113, 151, 27 , 0  , 0  , 0  ), // #405 {X:xmm, R:r32|m32|r64, R:i8}
  ISIGNATURE(3, 0, 1, 0, 113, 16 , 27 , 0  , 0  , 0  ), // #406 {X:xmm, R:r64|m64, R:i8}
  ISIGNATURE(3, 1, 1, 0, 199, 200, 27 , 0  , 0  , 0  ), // #407 {X:mm|xmm, R:r32|m16|r64|r16, R:i8}
  ISIGNATURE(2, 1, 1, 0, 154, 152, 0  , 0  , 0  , 0  ), // #408 {W:r32|r64, R:mm|xmm}
  ISIGNATURE(0, 1, 0, 0, 0  , 0  , 0  , 0  , 0  , 0  ), // #409 {}
  ISIGNATURE(3, 1, 1, 0, 61 , 159, 27 , 0  , 0  , 0  ), // #410 {W:mm, R:mm|m64, R:i8}
  ISIGNATURE(2, 1, 1, 0, 113, 27 , 0  , 0  , 0  , 0  ), // #411 {X:xmm, R:i8}
  ISIGNATURE(2, 1, 1, 0, 26 , 108, 0  , 0  , 0  , 0  ), // #412 {X:r8lo|r8hi|m8|r16|m16|r32|m32|r64|m64, R:cl|i8}
  ISIGNATURE(1, 0, 1, 0, 154, 0  , 0  , 0  , 0  , 0  ), // #413 {W:r32|r64}
  ISIGNATURE(1, 1, 1, 0, 153, 0  , 0  , 0  , 0  , 0  ), // #414 {W:r16|r32|r64}
  ISIGNATURE(2, 1, 1, 2, 46 , 201, 0  , 0  , 0  , 0  ), // #415 {W:<edx>, W:<eax>}
  ISIGNATURE(3, 1, 1, 3, 46 , 201, 197, 0  , 0  , 0  ), // #416 {W:<edx>, W:<eax>, W:<ecx>}
  ISIGNATURE(3, 1, 1, 0, 66 , 67 , 27 , 0  , 0  , 0  ), // #417 {W:xmm, R:xmm|m64, R:i8}
  ISIGNATURE(3, 1, 1, 0, 66 , 123, 27 , 0  , 0  , 0  ), // #418 {W:xmm, R:xmm|m32, R:i8}
  ISIGNATURE(1, 1, 1, 1, 202, 0  , 0  , 0  , 0  , 0  ), // #419 {R:<ah>}
  ISIGNATURE(1, 1, 1, 0, 1  , 0  , 0  , 0  , 0  , 0  ), // #420 {W:r8lo|r8hi|m8}
  ISIGNATURE(1, 1, 1, 0, 59 , 0  , 0  , 0  , 0  , 0  ), // #421 {W:m32}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 67 , 0  , 0  , 0  ), // #422 {W:xmm, R:xmm, R:xmm|m64}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 123, 0  , 0  , 0  ), // #423 {W:xmm, R:xmm, R:xmm|m32}
  ISIGNATURE(2, 1, 1, 0, 71 , 96 , 0  , 0  , 0  , 0  ), // #424 {W:ymm, R:m128}
  ISIGNATURE(2, 1, 1, 0, 203, 67 , 0  , 0  , 0  , 0  ), // #425 {W:ymm|zmm, R:xmm|m64}
  ISIGNATURE(2, 1, 1, 0, 203, 96 , 0  , 0  , 0  , 0  ), // #426 {W:ymm|zmm, R:m128}
  ISIGNATURE(2, 1, 1, 0, 75 , 97 , 0  , 0  , 0  , 0  ), // #427 {W:zmm, R:m256}
  ISIGNATURE(2, 1, 1, 0, 167, 67 , 0  , 0  , 0  , 0  ), // #428 {W:xmm|ymm|zmm, R:xmm|m64}
  ISIGNATURE(4, 1, 1, 0, 109, 65 , 67 , 27 , 0  , 0  ), // #429 {W:xmm|k, R:xmm, R:xmm|m64, R:i8}
  ISIGNATURE(4, 1, 1, 0, 109, 65 , 123, 27 , 0  , 0  ), // #430 {W:xmm|k, R:xmm, R:xmm|m32, R:i8}
  ISIGNATURE(3, 1, 1, 0, 66 , 65 , 42 , 0  , 0  , 0  ), // #431 {W:xmm, R:xmm, R:r32|m32|r64|m64}
  ISIGNATURE(3, 1, 1, 0, 70 , 204, 27 , 0  , 0  , 0  ), // #432 {W:xmm|m128, R:ymm|zmm, R:i8}
  ISIGNATURE(4, 1, 1, 0, 113, 65 , 67 , 27 , 0  , 0  ), // #433 {X:xmm, R:xmm, R:xmm|m64, R:i8}
  ISIGNATURE(4, 1, 1, 0, 113, 65 , 123, 27 , 0  , 0  ), // #434 {X:xmm, R:xmm, R:xmm|m32, R:i8}
  ISIGNATURE(3, 1, 1, 0, 113, 65 , 67 , 0  , 0  , 0  ), // #435 {X:xmm, R:xmm, R:xmm|m64}
  ISIGNATURE(3, 1, 1, 0, 113, 65 , 123, 0  , 0  , 0  ), // #436 {X:xmm, R:xmm, R:xmm|m32}
  ISIGNATURE(3, 1, 1, 0, 111, 205, 27 , 0  , 0  , 0  ), // #437 {W:k, R:xmm|m128|ymm|m256|zmm|m512, R:i8}
  ISIGNATURE(3, 1, 1, 0, 111, 67 , 27 , 0  , 0  , 0  ), // #438 {W:k, R:xmm|m64, R:i8}
  ISIGNATURE(3, 1, 1, 0, 111, 123, 27 , 0  , 0  , 0  ), // #439 {W:k, R:xmm|m32, R:i8}
  ISIGNATURE(1, 1, 1, 0, 81 , 0  , 0  , 0  , 0  , 0  ), // #440 {R:vm32y}
  ISIGNATURE(1, 1, 1, 0, 82 , 0  , 0  , 0  , 0  , 0  ), // #441 {R:vm32z}
  ISIGNATURE(1, 1, 1, 0, 85 , 0  , 0  , 0  , 0  , 0  ), // #442 {R:vm64z}
  ISIGNATURE(4, 1, 1, 0, 75 , 78 , 72 , 27 , 0  , 0  ), // #443 {W:zmm, R:zmm, R:ymm|m256, R:i8}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 123, 27 , 0  , 0  ), // #444 {W:xmm, R:xmm, R:xmm|m32, R:i8}
  ISIGNATURE(3, 1, 1, 1, 65 , 65 , 177, 0  , 0  , 0  ), // #445 {R:xmm, R:xmm, R:<zdi>}
  ISIGNATURE(2, 1, 1, 0, 154, 173, 0  , 0  , 0  , 0  ), // #446 {W:r32|r64, R:xmm|ymm}
  ISIGNATURE(2, 1, 1, 0, 167, 144, 0  , 0  , 0  , 0  ), // #447 {W:xmm|ymm|zmm, R:k}
  ISIGNATURE(2, 1, 1, 0, 167, 118, 0  , 0  , 0  , 0  ), // #448 {W:xmm|ymm|zmm, R:xmm|m64|r64}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 200, 27 , 0  , 0  ), // #449 {W:xmm, R:xmm, R:r32|m16|r64|r16, R:i8}
  ISIGNATURE(2, 1, 1, 0, 111, 206, 0  , 0  , 0  , 0  ), // #450 {W:k, R:xmm|ymm|zmm}
  ISIGNATURE(4, 1, 1, 0, 66 , 65 , 67 , 27 , 0  , 0  ), // #451 {W:xmm, R:xmm, R:xmm|m64, R:i8}
  ISIGNATURE(1, 0, 1, 0, 169, 0  , 0  , 0  , 0  , 0  ), // #452 {R:r32|r64}
  ISIGNATURE(3, 1, 1, 3, 182, 46 , 201, 0  , 0  , 0  ), // #453 {R:<ecx>, W:<edx>, W:<eax>}
  ISIGNATURE(3, 1, 1, 2, 191, 155, 88 , 0  , 0  , 0  ), // #454 {X:mem, R:<edx>, R:<eax>}
  ISIGNATURE(3, 0, 1, 2, 191, 155, 88 , 0  , 0  , 0  ), // #455 {X:mem, R:<edx>, R:<eax>}
  ISIGNATURE(3, 1, 1, 3, 182, 155, 88 , 0  , 0  , 0  )  // #456 {R:<ecx>, R:<edx>, R:<eax>}
};
#undef ISIGNATURE

#define FLAG(flag) X86Inst::kOp##flag
#define MEM(mem) X86Inst::kMemOp##mem
#define OSIGNATURE(flags, memFlags, extFlags, regId) \
  { uint32_t(flags), uint16_t(memFlags), uint8_t(extFlags), uint8_t(regId) }
static const X86Inst::OSignature _x86InstOSignatureData[] = {
  OSIGNATURE(0, 0, 0, 0xFF),
  OSIGNATURE(FLAG(W) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Mem), MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(I8), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpw) | FLAG(Mem), MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Seg) | FLAG(I16), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpd) | FLAG(Seg) | FLAG(Mem), MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpd), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpq) | FLAG(Mem), MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(Seg) | FLAG(I32), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(GpbLo) | FLAG(GpbHi), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Mem) | FLAG(I8), MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpw) | FLAG(Seg), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Mem), MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpd), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Seg) | FLAG(Mem) | FLAG(I32), MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpq) | FLAG(Seg), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(Mem), MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpw), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(I16), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpq), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Cr) | FLAG(Dr) | FLAG(I64), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpd) | FLAG(Mem), MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(I32), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Cr) | FLAG(Dr), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Cr) | FLAG(Dr), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpq), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M8) | MEM(M16) | MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(I8), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpw) | FLAG(Mem), MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(I16), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Mem), MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpd) | FLAG(Mem), MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpq) | FLAG(Mem), MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(GpbLo) | FLAG(GpbHi), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Mem), MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpw), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpd), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Mem), MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpq), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M8) | MEM(M16) | MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpw), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpw), 0, 0, 0x01),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Gpw), 0, 0, 0x04),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x04),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x01),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 0x04),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 0x01),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Mem) | FLAG(I8) | FLAG(I16), MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Mem) | FLAG(I8) | FLAG(I32), MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(Mem) | FLAG(I8) | FLAG(I32), MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(I8) | FLAG(I16), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(I8) | FLAG(I32), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(Mm) | FLAG(Xmm) | FLAG(Mem), MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpq) | FLAG(Mm) | FLAG(Xmm) | FLAG(Mem), MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Xmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Xmm) | FLAG(Mem), MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(M128), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Xmm) | FLAG(Mem), MEM(M128), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Ymm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Ymm) | FLAG(Mem), MEM(M256), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Ymm) | FLAG(Mem), MEM(M256), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Ymm), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Zmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Zmm) | FLAG(Mem), MEM(M512), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Zmm) | FLAG(Mem), MEM(M512), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Zmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem) | FLAG(I8), MEM(M128), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm32x), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm32y), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm32z), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm64x), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm64y), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm64z), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(GpbLo), 0, 0, 0x01),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpw), 0, 0, 0x01),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x01),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 0x01),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpw), 0, 0, 0x04),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x04),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 0x04),
  OSIGNATURE(FLAG(R) | FLAG(Vm), MEM(Vm64x) | MEM(Vm64y), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(M128), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(M256), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(M128), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(M256), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(I4), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(I8), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(I8), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(I8), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Fp), 0, 0, 0x01),
  OSIGNATURE(FLAG(R) | FLAG(Fp), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Fp), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Fp), 0, 0, 0x01),
  OSIGNATURE(FLAG(X) | FLAG(Implicit), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(I8), 0, 0, 0x02),
  OSIGNATURE(FLAG(W) | FLAG(K) | FLAG(Xmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(K) | FLAG(Ymm), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(K), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Ymm) | FLAG(Mem), MEM(M64) | MEM(M128) | MEM(M256), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Xmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Ymm), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Zmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(M512), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(M512), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(Xmm) | FLAG(Mem), MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mem), MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Xmm) | FLAG(Mem), MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Xmm) | FLAG(Mem), MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(M64) | MEM(M128), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm32x), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm32y), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm32z), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm64x), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm64y), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm64z), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpq) | FLAG(Mem) | FLAG(I32) | FLAG(I64) | FLAG(Rel32), MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Mem), MEM(M8) | MEM(M16) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpq) | FLAG(Mem), MEM(M8) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpw) | FLAG(Gpd), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Fp) | FLAG(Mem), MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Gpd), 0, 0, 0x02),
  OSIGNATURE(FLAG(R) | FLAG(I32) | FLAG(I64) | FLAG(Rel8), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Gpq), 0, 0, 0x02),
  OSIGNATURE(FLAG(X) | FLAG(Gpq) | FLAG(Mem) | FLAG(I32) | FLAG(I64) | FLAG(Rel8) | FLAG(Rel32), MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(K) | FLAG(Mem), MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(K), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Gpq) | FLAG(K) | FLAG(Mem), MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpq) | FLAG(K) | FLAG(Mem), MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(K) | FLAG(Mem), MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Mm) | FLAG(Xmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mm) | FLAG(Xmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpd) | FLAG(Gpq), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x04),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 0x04),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Mem), MEM(M16) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Mm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mm) | FLAG(Mem), MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Gpw) | FLAG(Gpq) | FLAG(Seg) | FLAG(Mem), MEM(M16) | MEM(M64), 0, 0x60),
  OSIGNATURE(FLAG(W) | FLAG(Gpd) | FLAG(Seg) | FLAG(Mem), MEM(M32), 0, 0x1A),
  OSIGNATURE(FLAG(R) | FLAG(Mm) | FLAG(Mem) | FLAG(I8), MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpw) | FLAG(Gpq) | FLAG(Seg) | FLAG(Mem) | FLAG(I8) | FLAG(I16) | FLAG(I32), MEM(M16) | MEM(M64), 0, 0x60),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Seg) | FLAG(Mem), MEM(M32), 0, 0x1E),
  OSIGNATURE(FLAG(X) | FLAG(I16), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Ymm) | FLAG(Mem), MEM(M128) | MEM(M256), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Xmm) | FLAG(Ymm) | FLAG(Zmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Mem), MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpd) | FLAG(Gpq), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(GpbLo) | FLAG(GpbHi) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M8), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(I8), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Vm), MEM(Vm64x) | MEM(Vm64y), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Ymm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Xmm), 0, 0, 0x01),
  OSIGNATURE(FLAG(X) | FLAG(Gpd) | FLAG(Gpq), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Mem), MEM(Any), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Implicit), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(M128), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 0x02),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpq), 0, 0, 0x08),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x02),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x08),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x08),
  OSIGNATURE(FLAG(X) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x02),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(M80), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(M16) | MEM(M32), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(M16) | MEM(M32) | MEM(M64), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Fp) | FLAG(Mem), MEM(M32) | MEM(M64) | MEM(M80), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Mem), MEM(Any), 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(Gpw) | FLAG(Mem), MEM(M16), 0, 0x01),
  OSIGNATURE(FLAG(X) | FLAG(I8), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(I32) | FLAG(I64) | FLAG(Rel8) | FLAG(Rel32), 0, 0, 0x00),
  OSIGNATURE(FLAG(X) | FLAG(I32) | FLAG(I64) | FLAG(Rel8), 0, 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(GpbHi), 0, 0, 0x01),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x02),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Xmm), 0, 0, 0x01),
  OSIGNATURE(FLAG(X) | FLAG(Mm) | FLAG(Xmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Gpw) | FLAG(Gpd) | FLAG(Gpq) | FLAG(Mem), MEM(M16), 0, 0x00),
  OSIGNATURE(FLAG(W) | FLAG(Implicit) | FLAG(Gpd), 0, 0, 0x01),
  OSIGNATURE(FLAG(R) | FLAG(Implicit) | FLAG(GpbHi), 0, 0, 0x01),
  OSIGNATURE(FLAG(W) | FLAG(Ymm) | FLAG(Zmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Ymm) | FLAG(Zmm), 0, 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Ymm) | FLAG(Zmm) | FLAG(Mem), MEM(M128) | MEM(M256) | MEM(M512), 0, 0x00),
  OSIGNATURE(FLAG(R) | FLAG(Xmm) | FLAG(Ymm) | FLAG(Zmm), 0, 0, 0x00)
};
#undef OSIGNATURE
#undef MEM
#undef FLAG
// ----------------------------------------------------------------------------
// ${signatureData:End}

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
  (1U << X86Reg::kRegGpw) | (1U << X86Reg::kRegGpd) | (1U << X86Reg::kRegRip) | (1U << Label::kLabelTag),
  // AllowedMemIndexRegs:
  (1U << X86Reg::kRegGpw) | (1U << X86Reg::kRegGpd) | (1U << X86Reg::kRegXmm) | (1U << X86Reg::kRegYmm) | (1U << X86Reg::kRegZmm)
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
  (1U << X86Reg::kRegGpd) | (1U << X86Reg::kRegGpq) | (1U << X86Reg::kRegRip) | (1U << Label::kLabelTag),
  // AllowedMemIndexRegs:
  (1U << X86Reg::kRegGpd) | (1U << X86Reg::kRegGpq) | (1U << X86Reg::kRegXmm) | (1U << X86Reg::kRegYmm) | (1U << X86Reg::kRegZmm)
};

ASMJIT_FAVOR_SIZE Error X86Inst::validate(
  uint32_t archType,
  uint32_t instId, uint32_t options,
  const Operand_& opExtra, const Operand_* opArray, uint32_t opCount) noexcept {

  uint32_t i;
  uint32_t archMask;
  const X86ValidationData* vd;

  if (!ArchInfo::isX86Family(archType))
    return DebugUtils::errored(kErrorInvalidArch);

  if (archType == ArchInfo::kTypeX86) {
    vd = &_x86ValidationData;
    archMask = X86Inst::kArchMaskX86;
  }
  else {
    vd = &_x64ValidationData;
    archMask = X86Inst::kArchMaskX64;
  }

  if (ASMJIT_UNLIKELY(instId >= X86Inst::_kIdCount))
    return DebugUtils::errored(kErrorInvalidArgument);

  // Get the instruction data.
  const X86Inst* iData = &X86InstDB::instData[instId];

  // Translate the given operands to `X86Inst::OSignature`.
  X86Inst::OSignature oSigTranslated[6];
  uint32_t combinedOpFlags = 0;
  uint32_t combinedRegMask = 0;

  const X86Mem* memOp = nullptr;

  for (i = 0; i < opCount; i++) {
    const Operand_& op = opArray[i];
    if (op.getOp() == Operand::kOpNone) break;

    uint32_t opFlags = 0;
    uint32_t memFlags = 0;
    uint32_t regId = kInvalidReg;

    switch (op.getOp()) {
      case Operand::kOpReg: {
        uint32_t regType = static_cast<const Reg&>(op).getRegType();
        if (ASMJIT_UNLIKELY(regType >= X86Reg::kRegCount))
          return DebugUtils::errored(kErrorInvalidRegType);

        opFlags = _x86OpFlagFromRegType[regType];
        if (ASMJIT_UNLIKELY(opFlags == 0))
          return DebugUtils::errored(kErrorInvalidRegType);

        // If `regId` is equal or greater than Operand::kPackedIdMin it means
        // that the register is virtual and its index will be assigned later
        // by the register allocator. We must pass unless asked to disallow
        // virtual registers.
        // TODO: We need an option to refuse virtual regs here.
        regId = op.getId();
        if (regId < Operand::kPackedIdMin) {
          if (ASMJIT_UNLIKELY(regId >= 32))
            return DebugUtils::errored(kErrorInvalidPhysId);

          uint32_t regMask = Utils::mask(regId);
          if (ASMJIT_UNLIKELY((vd->allowedRegMask[regType] & regMask) == 0))
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

        memOp = &m;

        // TODO: Validate base and index and combine with `combinedRegMask`.
        if (baseType) {
          if (ASMJIT_UNLIKELY((vd->allowedMemBaseRegs & (1U << baseType)) == 0))
            return DebugUtils::errored(kErrorInvalidAddress);
        }

        if (indexType) {
          if (ASMJIT_UNLIKELY((vd->allowedMemIndexRegs & (1U << indexType)) == 0))
            return DebugUtils::errored(kErrorInvalidAddress);

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
    tod.regMask = static_cast<uint8_t>(regId < 8 ? 1 << regId : 0);
    combinedOpFlags |= opFlags;
  }

  // Decrease the number of operands of those that are none. This is important
  // as Assembler and CodeCompiler may just pass more operands where some of
  // them are none (it means that no operand is given at that index). However,
  // validate that there are no gaps (like [reg, none, reg] or [none, reg]).
  if (i < opCount) {
    while (--opCount > i)
      if (ASMJIT_UNLIKELY(!opArray[opCount].isNone()))
        return DebugUtils::errored(kErrorInvalidState);
  }

  // Validate X86 and X64 specific cases.
  if (archMask == kArchMaskX86) {
    // Illegal use of 64-bit register in 32-bit mode.
    if (ASMJIT_UNLIKELY((combinedOpFlags & X86Inst::kOpGpq) != 0))
      return DebugUtils::errored(kErrorInvalidUseOfGpq);
  }
  else {
    // Illegal use of a high 8-bit register with REX prefix.
    if (ASMJIT_UNLIKELY((combinedOpFlags & X86Inst::kOpGpbHi) != 0 && (combinedRegMask & 0xFFFFFF00U) != 0))
      return DebugUtils::errored(kErrorInvalidUseOfGpbHi);
  }

  // Validate instruction operands.
  const X86Inst::CommonData* commonData = &iData->getCommonData();
  const X86Inst::ISignature* iSig = _x86InstISignatureData + commonData->_iSignatureIndex;
  const X86Inst::ISignature* iEnd = iSig                   + commonData->_iSignatureCount;

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
          if (oRef->regMask && !(oRef->regMask & oChk->regMask))
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
          if (oRef->regMask && !(oRef->regMask & oChk->regMask))
            break;
        }

        if (j == opCount)
          break;
      }
    } while (++iSig != iEnd);

    if (iSig == iEnd)
      return DebugUtils::errored(kErrorInvalidInstruction);
  }

  // Validate AVX-512 options:
  const uint32_t kAvx512Options = X86Inst::kOptionK    |
                                  X86Inst::kOption1ToX |
                                  X86Inst::kOptionKZ   |
                                  X86Inst::kOptionSAE  |
                                  X86Inst::kOptionER   ;
  if (options & kAvx512Options) {
    // Validate AVX-512 {k} and {k}{z}.
    if (options & (X86Inst::kOptionK | X86Inst::kOptionKZ)) {
      // Zero {z} without a mask register is invalid.
      if (ASMJIT_UNLIKELY(!(options & X86Inst::kOptionK)))
        return DebugUtils::errored(kErrorInvalidKZeroUse);

      // Mask can only be specified by 'k' register.
      if (ASMJIT_UNLIKELY(!x86::isK(opExtra)))
        return DebugUtils::errored(kErrorInvalidKMaskReg);

      if (ASMJIT_UNLIKELY(!commonData->hasFlag(kInstFlagEvexK_)))
        return DebugUtils::errored(kErrorInvalidKMaskUse);

      if (ASMJIT_UNLIKELY((options & X86Inst::kOptionKZ) != 0 && !commonData->hasFlag(X86Inst::kInstFlagEvexKZ)))
        return DebugUtils::errored(kErrorInvalidKZeroUse);
    }

    // Validate AVX-512 broadcast {1tox}.
    if (options & X86Inst::kOption1ToX) {
      if (ASMJIT_UNLIKELY(!memOp))
        return DebugUtils::errored(kErrorInvalidBroadcast);

      uint32_t size = memOp->getSize();
      if (size != 0) {
        if (ASMJIT_UNLIKELY(commonData->hasFlag(X86Inst::kInstFlagEvexB4) && size != 4))
          return DebugUtils::errored(kErrorInvalidBroadcast);

        if (ASMJIT_UNLIKELY(commonData->hasFlag(X86Inst::kInstFlagEvexB8) && size != 8))
          return DebugUtils::errored(kErrorInvalidBroadcast);
      }
    }

    // Validate AVX-512 {sae} and {er}.
    if (options & (X86Inst::kOptionSAE | X86Inst::kOptionER)) {
      // Rounding control is impossible if the instruction is not reg-to-reg.
      if (ASMJIT_UNLIKELY(memOp))
        return DebugUtils::errored(kErrorInvalidSAEOrER);

      // Check if {sae} or {er} is supported by the instruction.
      if (options & X86Inst::kOptionER) {
        // NOTE: if both {sae} and {er} are set, we don't care, as {sae} is implied.
        if (ASMJIT_UNLIKELY(!(commonData->getFlags() & X86Inst::kInstFlagEvexER)))
          return DebugUtils::errored(kErrorInvalidSAEOrER);

        // {er} is defined for scalar ops or vector ops using zmm (LL = 10). We
        // don't need any more bits in the instruction database to be able to
        // validate this, as each AVX512 instruction that has broadcast is vector
        // instruction (in this case we require zmm registers), otherwise it's a
        // scalar instruction, which is valid.
        if (commonData->getFlags() & (X86Inst::kInstFlagEvexB4 | kInstFlagEvexB8)) {
          // Supports broadcast, thus we require LL to be '10', which means there
          // have to be zmm registers used.

          // If we went here it must be true as there is no {er} enabled
          // instruction that has less than two operands.
          ASMJIT_ASSERT(opCount >= 2);
          if (ASMJIT_UNLIKELY(!x86::isZmm(opArray[0]) && !x86::isZmm(opArray[1])))
            return DebugUtils::errored(kErrorInvalidSAEOrER);
        }
      }
      else {
        // {sae} doesn't have the same limitations as {er}, this is enough.
        if (ASMJIT_UNLIKELY(!(commonData->getFlags() & X86Inst::kInstFlagEvexSAE)))
          return DebugUtils::errored(kErrorInvalidSAEOrER);
      }
    }
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
  EXPECT(X86Inst::kOptionRex  == 0x80000000U, "REX prefix must be at 0x80000000");
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
    uint32_t b = X86Inst::getIdByName(X86Inst::getInst(a).getName());
    EXPECT(a == b,
      "Should match existing instruction \"%s\" {id:%u} != \"%s\" {id:%u}.",
        X86Inst::getInst(a).getName(), a,
        X86Inst::getInst(b).getName(), b);
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
static Error x86_validate(uint32_t instId, const Operand& o0 = Operand(), const Operand& o1 = Operand(), const Operand& o2 = Operand()) {
  Operand opArray[] = { o0, o1, o2 };
  return X86Inst::validate(ArchInfo::kTypeX86, instId, 0, Operand(), opArray, 3);
}

static Error x64_validate(uint32_t instId, const Operand& o0 = Operand(), const Operand& o1 = Operand(), const Operand& o2 = Operand()) {
  Operand opArray[] = { o0, o1, o2 };
  return X86Inst::validate(ArchInfo::kTypeX64, instId, 0, Operand(), opArray, 3);
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
  EXPECT(x64_validate(X86Inst::kIdMov   , x86::ax  , x86::fs  ) == kErrorOk);
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
#include "../asmjit_apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86
