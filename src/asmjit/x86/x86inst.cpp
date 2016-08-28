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

// Don't store nameIndex if instruction names are disabled. Since some APIs
// can still count the nameIndex is valid it's safer to just not compile it.
#if defined(ASMJIT_DISABLE_TEXT)
# define NAME_INDEX(X) 0
#else
# define NAME_INDEX(X) X
#endif

#define A512(CPU, VL, MASKING, SAE_ER, BROADCAST) \
  (X86Inst::kInstFlagEvex              | \
   X86Inst::kInstFlagEvexSet_##CPU     | \
   (VL ? X86Inst::kInstFlagEvexVL : 0) | \
   (X86Inst::kInstFlagEvex##MASKING ? (X86Inst::kInstFlagEvex##MASKING | X86Inst::kInstFlagEvexK_) : 0) | \
   X86Inst::kInstFlagEvex##SAE_ER      | \
   X86Inst::kInstFlagEvexB##BROADCAST  )

// Defines an X86/X64 instruction.
#define INST(id, name, encoding, opcode0, opcode1, instFlags, eflags, writeIndex, writeSize, nameIndex, extendedIndex) \
  { opcode0, NAME_INDEX(nameIndex), extendedIndex, 0 }

const X86Inst X86InstDB::instData[] = {
  // <-----------------+-------------------+------------------------+------------------+--------+------------------+--------+---------------------------------------+-------------+-------+-----+----+
  //                   |                   |                        |  Primary OpCode  |#0 EVEX | Secondary OpCode |#1 EVEX |         Instruction Flags             |   E-FLAGS   | Write |     |    |
  //  Instruction Id   | Instruction Name  |  Instruction Encoding  |                  +--------+                  +--------+----------------+----------------------+-------------+---+---|NameX|ExtX|
  //                   |                   |                        |#0:PP-MMM OP/O L|W|W|N|TT. |#1:PP-MMM OP/O L|W|W|N|TT. | Global Flags   |A512(CPU_|V|kz|rnd|b) | EF:OSZAPCDX |Idx|Cnt|     |    |
  // <-----------------+-------------------+------------------------+------------------+--------+------------------+--------+----------------+----------------------+-------------+---+---+-----+----+
// ${X86InstData:Begin}
  INST(None            , ""                , Enc(None)              , 0                         , 0                         , 0                                     , EF(________), 0 , 0 , 0   , 0  ),
  INST(Adc             , "adc"             , Enc(X86Arith)          , O(000000,10,2,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWWWX__), 0 , 0 , 1   , 1  ),
  INST(Adcx            , "adcx"            , Enc(X86Rm)             , O(660F38,F6,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____X__), 0 , 0 , 5   , 2  ),
  INST(Add             , "add"             , Enc(X86Arith)          , O(000000,00,0,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWWWW__), 0 , 0 , 657 , 3  ),
  INST(Addpd           , "addpd"           , Enc(ExtRm)             , O(660F00,58,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4545, 4  ),
  INST(Addps           , "addps"           , Enc(ExtRm)             , O(000F00,58,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4557, 4  ),
  INST(Addsd           , "addsd"           , Enc(ExtRm)             , O(F20F00,58,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4779, 5  ),
  INST(Addss           , "addss"           , Enc(ExtRm)             , O(F30F00,58,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4789, 6  ),
  INST(Addsubpd        , "addsubpd"        , Enc(ExtRm)             , O(660F00,D0,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4284, 4  ),
  INST(Addsubps        , "addsubps"        , Enc(ExtRm)             , O(F20F00,D0,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4296, 4  ),
  INST(Adox            , "adox"            , Enc(X86Rm)             , O(F30F38,F6,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(X_______), 0 , 0 , 10  , 7  ),
  INST(Aesdec          , "aesdec"          , Enc(ExtRm)             , O(660F38,DE,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2790, 4  ),
  INST(Aesdeclast      , "aesdeclast"      , Enc(ExtRm)             , O(660F38,DF,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2798, 4  ),
  INST(Aesenc          , "aesenc"          , Enc(ExtRm)             , O(660F38,DC,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2810, 4  ),
  INST(Aesenclast      , "aesenclast"      , Enc(ExtRm)             , O(660F38,DD,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2818, 4  ),
  INST(Aesimc          , "aesimc"          , Enc(ExtRm)             , O(660F38,DB,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 2830, 8  ),
  INST(Aeskeygenassist , "aeskeygenassist" , Enc(ExtRmi)            , O(660F3A,DF,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 2838, 9  ),
  INST(And             , "and"             , Enc(X86Arith)          , O(000000,20,4,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWUWW__), 0 , 0 , 2034, 3  ),
  INST(Andn            , "andn"            , Enc(VexRvm_Wx)         , V(000F38,F2,_,0,x,_,_,_  ), 0                         , F(RW)                                 , EF(WWWUUW__), 0 , 0 , 6055, 10 ),
  INST(Andnpd          , "andnpd"          , Enc(ExtRm)             , O(660F00,55,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2871, 4  ),
  INST(Andnps          , "andnps"          , Enc(ExtRm)             , O(000F00,55,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2879, 4  ),
  INST(Andpd           , "andpd"           , Enc(ExtRm)             , O(660F00,54,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 3798, 4  ),
  INST(Andps           , "andps"           , Enc(ExtRm)             , O(000F00,54,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 3808, 4  ),
  INST(Bextr           , "bextr"           , Enc(VexRmv_Wx)         , V(000F38,F7,_,0,x,_,_,_  ), 0                         , F(RW)                                 , EF(WUWUUW__), 0 , 0 , 15  , 11 ),
  INST(Blcfill         , "blcfill"         , Enc(VexVm_Wx)          , V(XOP_M9,01,1,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , 21  , 12 ),
  INST(Blci            , "blci"            , Enc(VexVm_Wx)          , V(XOP_M9,02,6,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , 29  , 12 ),
  INST(Blcic           , "blcic"           , Enc(VexVm_Wx)          , V(XOP_M9,01,5,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , 34  , 12 ),
  INST(Blcmsk          , "blcmsk"          , Enc(VexVm_Wx)          , V(XOP_M9,02,1,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , 40  , 12 ),
  INST(Blcs            , "blcs"            , Enc(VexVm_Wx)          , V(XOP_M9,01,3,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , 47  , 12 ),
  INST(Blendpd         , "blendpd"         , Enc(ExtRmi)            , O(660F3A,0D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2957, 13 ),
  INST(Blendps         , "blendps"         , Enc(ExtRmi)            , O(660F3A,0C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2966, 13 ),
  INST(Blendvpd        , "blendvpd"        , Enc(ExtRmXMM0)         , O(660F38,15,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 2975, 14 ),
  INST(Blendvps        , "blendvps"        , Enc(ExtRmXMM0)         , O(660F38,14,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 2985, 14 ),
  INST(Blsfill         , "blsfill"         , Enc(VexVm_Wx)          , V(XOP_M9,01,2,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , 52  , 12 ),
  INST(Blsi            , "blsi"            , Enc(VexVm_Wx)          , V(000F38,F3,3,0,x,_,_,_  ), 0                         , F(RW)                                 , EF(WWWUUW__), 0 , 0 , 60  , 15 ),
  INST(Blsic           , "blsic"           , Enc(VexVm_Wx)          , V(XOP_M9,01,6,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , 65  , 12 ),
  INST(Blsmsk          , "blsmsk"          , Enc(VexVm_Wx)          , V(000F38,F3,2,0,x,_,_,_  ), 0                         , F(RW)                                 , EF(WWWUUW__), 0 , 0 , 71  , 15 ),
  INST(Blsr            , "blsr"            , Enc(VexVm_Wx)          , V(000F38,F3,1,0,x,_,_,_  ), 0                         , F(RW)                                 , EF(WWWUUW__), 0 , 0 , 78  , 15 ),
  INST(Bsf             , "bsf"             , Enc(X86Rm)             , O(000F00,BC,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(UUWUUU__), 0 , 0 , 83  , 16 ),
  INST(Bsr             , "bsr"             , Enc(X86Rm)             , O(000F00,BD,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(UUWUUU__), 0 , 0 , 87  , 16 ),
  INST(Bswap           , "bswap"           , Enc(X86Bswap)          , O(000F00,C8,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 91  , 17 ),
  INST(Bt              , "bt"              , Enc(X86Bt)             , O(000F00,A3,_,_,x,_,_,_  ), O(000F00,BA,4,_,x,_,_,_  ), F(RO)                                 , EF(UU_UUW__), 0 , 0 , 97  , 18 ),
  INST(Btc             , "btc"             , Enc(X86Bt)             , O(000F00,BB,_,_,x,_,_,_  ), O(000F00,BA,7,_,x,_,_,_  ), F(RW)|F(Lock)                         , EF(UU_UUW__), 0 , 0 , 100 , 19 ),
  INST(Btr             , "btr"             , Enc(X86Bt)             , O(000F00,B3,_,_,x,_,_,_  ), O(000F00,BA,6,_,x,_,_,_  ), F(RW)|F(Lock)                         , EF(UU_UUW__), 0 , 0 , 104 , 20 ),
  INST(Bts             , "bts"             , Enc(X86Bt)             , O(000F00,AB,_,_,x,_,_,_  ), O(000F00,BA,5,_,x,_,_,_  ), F(RW)|F(Lock)                         , EF(UU_UUW__), 0 , 0 , 108 , 21 ),
  INST(Bzhi            , "bzhi"            , Enc(VexRmv_Wx)         , V(000F38,F5,_,0,x,_,_,_  ), 0                         , F(RW)                                 , EF(WWWUUW__), 0 , 0 , 112 , 11 ),
  INST(Call            , "call"            , Enc(X86Call)           , O(000000,FF,2,_,_,_,_,_  ), O(000000,E8,_,_,_,_,_,_  ), F(RW)|F(Flow)|F(Volatile)             , EF(________), 0 , 0 , 117 , 22 ),
  INST(Cbw             , "cbw"             , Enc(X86OpAx)           , O(660000,98,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 122 , 23 ),
  INST(Cdq             , "cdq"             , Enc(X86OpDxAx)         , O(000000,99,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 126 , 24 ),
  INST(Cdqe            , "cdqe"            , Enc(X86OpAx)           , O(000000,98,_,_,1,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 130 , 25 ),
  INST(Clac            , "clac"            , Enc(X86Op)             , O(000F01,CA,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(___W____), 0 , 0 , 135 , 26 ),
  INST(Clc             , "clc"             , Enc(X86Op)             , O(000000,F8,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(_____W__), 0 , 0 , 140 , 27 ),
  INST(Cld             , "cld"             , Enc(X86Op)             , O(000000,FC,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(______W_), 0 , 0 , 144 , 28 ),
  INST(Clflush         , "clflush"         , Enc(X86M_Only)         , O(000F00,AE,7,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , 148 , 29 ),
  INST(Clflushopt      , "clflushopt"      , Enc(X86M_Only)         , O(660F00,AE,7,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , 156 , 29 ),
  INST(Clwb            , "clwb"            , Enc(X86M_Only)         , O(660F00,AE,6,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , 167 , 29 ),
  INST(Clzero          , "clzero"          , Enc(X86Op)             , O(000F01,FC,_,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 172 , 30 ),
  INST(Cmc             , "cmc"             , Enc(X86Op)             , O(000000,F5,_,_,_,_,_,_  ), 0                         , 0                                     , EF(_____X__), 0 , 0 , 179 , 31 ),
  INST(Cmova           , "cmova"           , Enc(X86Rm)             , O(000F00,47,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R__R__), 0 , 0 , 183 , 32 ),
  INST(Cmovae          , "cmovae"          , Enc(X86Rm)             , O(000F00,43,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____R__), 0 , 0 , 189 , 33 ),
  INST(Cmovb           , "cmovb"           , Enc(X86Rm)             , O(000F00,42,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____R__), 0 , 0 , 514 , 33 ),
  INST(Cmovbe          , "cmovbe"          , Enc(X86Rm)             , O(000F00,46,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R__R__), 0 , 0 , 521 , 32 ),
  INST(Cmovc           , "cmovc"           , Enc(X86Rm)             , O(000F00,42,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____R__), 0 , 0 , 196 , 33 ),
  INST(Cmove           , "cmove"           , Enc(X86Rm)             , O(000F00,44,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R_____), 0 , 0 , 529 , 34 ),
  INST(Cmovg           , "cmovg"           , Enc(X86Rm)             , O(000F00,4F,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RRR_____), 0 , 0 , 202 , 35 ),
  INST(Cmovge          , "cmovge"          , Enc(X86Rm)             , O(000F00,4D,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RR______), 0 , 0 , 208 , 36 ),
  INST(Cmovl           , "cmovl"           , Enc(X86Rm)             , O(000F00,4C,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RR______), 0 , 0 , 215 , 36 ),
  INST(Cmovle          , "cmovle"          , Enc(X86Rm)             , O(000F00,4E,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RRR_____), 0 , 0 , 221 , 35 ),
  INST(Cmovna          , "cmovna"          , Enc(X86Rm)             , O(000F00,46,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R__R__), 0 , 0 , 228 , 32 ),
  INST(Cmovnae         , "cmovnae"         , Enc(X86Rm)             , O(000F00,42,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____R__), 0 , 0 , 235 , 33 ),
  INST(Cmovnb          , "cmovnb"          , Enc(X86Rm)             , O(000F00,43,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____R__), 0 , 0 , 536 , 33 ),
  INST(Cmovnbe         , "cmovnbe"         , Enc(X86Rm)             , O(000F00,47,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R__R__), 0 , 0 , 544 , 32 ),
  INST(Cmovnc          , "cmovnc"          , Enc(X86Rm)             , O(000F00,43,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_____R__), 0 , 0 , 243 , 33 ),
  INST(Cmovne          , "cmovne"          , Enc(X86Rm)             , O(000F00,45,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R_____), 0 , 0 , 553 , 34 ),
  INST(Cmovng          , "cmovng"          , Enc(X86Rm)             , O(000F00,4E,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RRR_____), 0 , 0 , 250 , 35 ),
  INST(Cmovnge         , "cmovnge"         , Enc(X86Rm)             , O(000F00,4C,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RR______), 0 , 0 , 257 , 36 ),
  INST(Cmovnl          , "cmovnl"          , Enc(X86Rm)             , O(000F00,4D,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RR______), 0 , 0 , 265 , 36 ),
  INST(Cmovnle         , "cmovnle"         , Enc(X86Rm)             , O(000F00,4F,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(RRR_____), 0 , 0 , 272 , 35 ),
  INST(Cmovno          , "cmovno"          , Enc(X86Rm)             , O(000F00,41,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(R_______), 0 , 0 , 280 , 37 ),
  INST(Cmovnp          , "cmovnp"          , Enc(X86Rm)             , O(000F00,4B,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(____R___), 0 , 0 , 287 , 38 ),
  INST(Cmovns          , "cmovns"          , Enc(X86Rm)             , O(000F00,49,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_R______), 0 , 0 , 294 , 39 ),
  INST(Cmovnz          , "cmovnz"          , Enc(X86Rm)             , O(000F00,45,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R_____), 0 , 0 , 301 , 34 ),
  INST(Cmovo           , "cmovo"           , Enc(X86Rm)             , O(000F00,40,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(R_______), 0 , 0 , 308 , 37 ),
  INST(Cmovp           , "cmovp"           , Enc(X86Rm)             , O(000F00,4A,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(____R___), 0 , 0 , 314 , 38 ),
  INST(Cmovpe          , "cmovpe"          , Enc(X86Rm)             , O(000F00,4A,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(____R___), 0 , 0 , 320 , 38 ),
  INST(Cmovpo          , "cmovpo"          , Enc(X86Rm)             , O(000F00,4B,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(____R___), 0 , 0 , 327 , 38 ),
  INST(Cmovs           , "cmovs"           , Enc(X86Rm)             , O(000F00,48,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(_R______), 0 , 0 , 334 , 39 ),
  INST(Cmovz           , "cmovz"           , Enc(X86Rm)             , O(000F00,44,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(__R_____), 0 , 0 , 340 , 34 ),
  INST(Cmp             , "cmp"             , Enc(X86Arith)          , O(000000,38,7,_,x,_,_,_  ), 0                         , F(RO)                                 , EF(WWWWWW__), 0 , 0 , 346 , 40 ),
  INST(Cmppd           , "cmppd"           , Enc(ExtRmi)            , O(660F00,C2,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 3211, 13 ),
  INST(Cmpps           , "cmpps"           , Enc(ExtRmi)            , O(000F00,C2,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 3218, 13 ),
  INST(CmpsB           , "cmps_b"          , Enc(X86Op)             , O(000000,A6,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2292, 41 ),
  INST(CmpsD           , "cmps_d"          , Enc(X86Op)             , O(000000,A7,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2305, 41 ),
  INST(CmpsQ           , "cmps_q"          , Enc(X86Op)             , O(000000,A7,_,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2318, 41 ),
  INST(CmpsW           , "cmps_w"          , Enc(X86Op)             , O(660000,A7,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2331, 41 ),
  INST(Cmpsd           , "cmpsd"           , Enc(ExtRmi)            , O(F20F00,C2,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 3225, 42 ),
  INST(Cmpss           , "cmpss"           , Enc(ExtRmi)            , O(F30F00,C2,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 3232, 43 ),
  INST(Cmpxchg         , "cmpxchg"         , Enc(X86Cmpxchg)        , O(000F00,B0,_,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)|F(Special)              , EF(WWWWWW__), 0 , 0 , 350 , 44 ),
  INST(Cmpxchg16b      , "cmpxchg16b"      , Enc(X86M_Only)         , O(000F00,C7,1,_,1,_,_,_  ), 0                         , F(RW)|F(Lock)|F(Special)              , EF(__W_____), 0 , 0 , 358 , 45 ),
  INST(Cmpxchg8b       , "cmpxchg8b"       , Enc(X86M_Only)         , O(000F00,C7,1,_,_,_,_,_  ), 0                         , F(RW)|F(Lock)|F(Special)              , EF(__W_____), 0 , 0 , 369 , 46 ),
  INST(Comisd          , "comisd"          , Enc(ExtRm)             , O(660F00,2F,_,_,_,_,_,_  ), 0                         , F(RO)                                 , EF(WWWWWW__), 0 , 0 , 9258, 47 ),
  INST(Comiss          , "comiss"          , Enc(ExtRm)             , O(000F00,2F,_,_,_,_,_,_  ), 0                         , F(RO)                                 , EF(WWWWWW__), 0 , 0 , 9267, 48 ),
  INST(Cpuid           , "cpuid"           , Enc(X86Op)             , O(000F00,A2,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 379 , 49 ),
  INST(Cqo             , "cqo"             , Enc(X86OpDxAx)         , O(000000,99,_,_,1,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 385 , 50 ),
  INST(Crc32           , "crc32"           , Enc(X86Crc)            , O(F20F38,F0,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 389 , 51 ),
  INST(Cvtdq2pd        , "cvtdq2pd"        , Enc(ExtRm)             , O(F30F00,E6,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 3279, 52 ),
  INST(Cvtdq2ps        , "cvtdq2ps"        , Enc(ExtRm)             , O(000F00,5B,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 3289, 53 ),
  INST(Cvtpd2dq        , "cvtpd2dq"        , Enc(ExtRm)             , O(F20F00,E6,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 3299, 53 ),
  INST(Cvtpd2pi        , "cvtpd2pi"        , Enc(ExtRm)             , O(660F00,2D,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 395 , 54 ),
  INST(Cvtpd2ps        , "cvtpd2ps"        , Enc(ExtRm)             , O(660F00,5A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 3309, 53 ),
  INST(Cvtpi2pd        , "cvtpi2pd"        , Enc(ExtRm)             , O(660F00,2A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 404 , 55 ),
  INST(Cvtpi2ps        , "cvtpi2ps"        , Enc(ExtRm)             , O(000F00,2A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 413 , 56 ),
  INST(Cvtps2dq        , "cvtps2dq"        , Enc(ExtRm)             , O(660F00,5B,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 3361, 53 ),
  INST(Cvtps2pd        , "cvtps2pd"        , Enc(ExtRm)             , O(000F00,5A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 3371, 52 ),
  INST(Cvtps2pi        , "cvtps2pi"        , Enc(ExtRm)             , O(000F00,2D,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 422 , 57 ),
  INST(Cvtsd2si        , "cvtsd2si"        , Enc(ExtRm_Wx)          , O(F20F00,2D,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 3443, 58 ),
  INST(Cvtsd2ss        , "cvtsd2ss"        , Enc(ExtRm)             , O(F20F00,5A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 4 , 3453, 59 ),
  INST(Cvtsi2sd        , "cvtsi2sd"        , Enc(ExtRm_Wx)          , O(F20F00,2A,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 3474, 60 ),
  INST(Cvtsi2ss        , "cvtsi2ss"        , Enc(ExtRm_Wx)          , O(F30F00,2A,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 4 , 3484, 61 ),
  INST(Cvtss2sd        , "cvtss2sd"        , Enc(ExtRm)             , O(F30F00,5A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 3494, 62 ),
  INST(Cvtss2si        , "cvtss2si"        , Enc(ExtRm_Wx)          , O(F30F00,2D,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 3504, 63 ),
  INST(Cvttpd2dq       , "cvttpd2dq"       , Enc(ExtRm)             , O(660F00,E6,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 3525, 53 ),
  INST(Cvttpd2pi       , "cvttpd2pi"       , Enc(ExtRm)             , O(660F00,2C,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 431 , 54 ),
  INST(Cvttps2dq       , "cvttps2dq"       , Enc(ExtRm)             , O(F30F00,5B,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 3571, 53 ),
  INST(Cvttps2pi       , "cvttps2pi"       , Enc(ExtRm)             , O(000F00,2C,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 441 , 57 ),
  INST(Cvttsd2si       , "cvttsd2si"       , Enc(ExtRm_Wx)          , O(F20F00,2C,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 3617, 58 ),
  INST(Cvttss2si       , "cvttss2si"       , Enc(ExtRm_Wx)          , O(F30F00,2C,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 3640, 63 ),
  INST(Cwd             , "cwd"             , Enc(X86OpDxAx)         , O(660000,99,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 451 , 64 ),
  INST(Cwde            , "cwde"            , Enc(X86OpAx)           , O(000000,98,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 455 , 65 ),
  INST(Daa             , "daa"             , Enc(X86Op)             , O(000000,27,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(UWWXWX__), 0 , 0 , 460 , 66 ),
  INST(Das             , "das"             , Enc(X86Op)             , O(000000,2F,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(UWWXWX__), 0 , 0 , 464 , 66 ),
  INST(Dec             , "dec"             , Enc(X86IncDec)         , O(000000,FE,1,_,x,_,_,_  ), O(000000,48,_,_,x,_,_,_  ), F(RW)|F(Lock)                         , EF(WWWWW___), 0 , 0 , 2793, 67 ),
  INST(Div             , "div"             , Enc(X86M_Bx_MulDiv)    , O(000000,F6,6,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(UUUUUU__), 0 , 0 , 676 , 68 ),
  INST(Divpd           , "divpd"           , Enc(ExtRm)             , O(660F00,5E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 3739, 4  ),
  INST(Divps           , "divps"           , Enc(ExtRm)             , O(000F00,5E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 3746, 4  ),
  INST(Divsd           , "divsd"           , Enc(ExtRm)             , O(F20F00,5E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 3753, 5  ),
  INST(Divss           , "divss"           , Enc(ExtRm)             , O(F30F00,5E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 3760, 6  ),
  INST(Dppd            , "dppd"            , Enc(ExtRmi)            , O(660F3A,41,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 3767, 13 ),
  INST(Dpps            , "dpps"            , Enc(ExtRmi)            , O(660F3A,40,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 3773, 13 ),
  INST(Emms            , "emms"            , Enc(X86Op)             , O(000F00,77,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(________), 0 , 0 , 644 , 69 ),
  INST(Enter           , "enter"           , Enc(X86Enter)          , O(000000,C8,_,_,_,_,_,_  ), 0                         , F(Volatile)|F(Special)                , EF(________), 0 , 0 , 468 , 70 ),
  INST(Extractps       , "extractps"       , Enc(ExtExtract)        , O(660F3A,17,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 3953, 71 ),
  INST(Extrq           , "extrq"           , Enc(ExtExtrq)          , O(660F00,79,_,_,_,_,_,_  ), O(660F00,78,0,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 6769, 72 ),
  INST(F2xm1           , "f2xm1"           , Enc(FpuOp)             , O_FPU(00,D9F0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 474 , 73 ),
  INST(Fabs            , "fabs"            , Enc(FpuOp)             , O_FPU(00,D9E1,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 480 , 73 ),
  INST(Fadd            , "fadd"            , Enc(FpuArith)          , O_FPU(00,C0C0,0)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , 1756, 74 ),
  INST(Faddp           , "faddp"           , Enc(FpuRDef)           , O_FPU(00,DEC0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 485 , 75 ),
  INST(Fbld            , "fbld"            , Enc(X86M_Only)         , O_FPU(00,00DF,4)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 491 , 76 ),
  INST(Fbstp           , "fbstp"           , Enc(X86M_Only)         , O_FPU(00,00DF,6)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 496 , 76 ),
  INST(Fchs            , "fchs"            , Enc(FpuOp)             , O_FPU(00,D9E0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 502 , 73 ),
  INST(Fclex           , "fclex"           , Enc(FpuOp)             , O_FPU(9B,DBE2,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 507 , 73 ),
  INST(Fcmovb          , "fcmovb"          , Enc(FpuR)              , O_FPU(00,DAC0,_)          , 0                         , F(Fp)                                 , EF(_____R__), 0 , 0 , 513 , 77 ),
  INST(Fcmovbe         , "fcmovbe"         , Enc(FpuR)              , O_FPU(00,DAD0,_)          , 0                         , F(Fp)                                 , EF(__R__R__), 0 , 0 , 520 , 78 ),
  INST(Fcmove          , "fcmove"          , Enc(FpuR)              , O_FPU(00,DAC8,_)          , 0                         , F(Fp)                                 , EF(__R_____), 0 , 0 , 528 , 79 ),
  INST(Fcmovnb         , "fcmovnb"         , Enc(FpuR)              , O_FPU(00,DBC0,_)          , 0                         , F(Fp)                                 , EF(_____R__), 0 , 0 , 535 , 77 ),
  INST(Fcmovnbe        , "fcmovnbe"        , Enc(FpuR)              , O_FPU(00,DBD0,_)          , 0                         , F(Fp)                                 , EF(__R__R__), 0 , 0 , 543 , 78 ),
  INST(Fcmovne         , "fcmovne"         , Enc(FpuR)              , O_FPU(00,DBC8,_)          , 0                         , F(Fp)                                 , EF(__R_____), 0 , 0 , 552 , 79 ),
  INST(Fcmovnu         , "fcmovnu"         , Enc(FpuR)              , O_FPU(00,DBD8,_)          , 0                         , F(Fp)                                 , EF(____R___), 0 , 0 , 560 , 80 ),
  INST(Fcmovu          , "fcmovu"          , Enc(FpuR)              , O_FPU(00,DAD8,_)          , 0                         , F(Fp)                                 , EF(____R___), 0 , 0 , 568 , 80 ),
  INST(Fcom            , "fcom"            , Enc(FpuCom)            , O_FPU(00,D0D0,2)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 575 , 81 ),
  INST(Fcomi           , "fcomi"           , Enc(FpuR)              , O_FPU(00,DBF0,_)          , 0                         , F(Fp)                                 , EF(WWWWWW__), 0 , 0 , 580 , 82 ),
  INST(Fcomip          , "fcomip"          , Enc(FpuR)              , O_FPU(00,DFF0,_)          , 0                         , F(Fp)                                 , EF(WWWWWW__), 0 , 0 , 586 , 82 ),
  INST(Fcomp           , "fcomp"           , Enc(FpuCom)            , O_FPU(00,D8D8,3)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 593 , 81 ),
  INST(Fcompp          , "fcompp"          , Enc(FpuOp)             , O_FPU(00,DED9,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 599 , 73 ),
  INST(Fcos            , "fcos"            , Enc(FpuOp)             , O_FPU(00,D9FF,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 606 , 73 ),
  INST(Fdecstp         , "fdecstp"         , Enc(FpuOp)             , O_FPU(00,D9F6,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 611 , 73 ),
  INST(Fdiv            , "fdiv"            , Enc(FpuArith)          , O_FPU(00,F0F8,6)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , 619 , 74 ),
  INST(Fdivp           , "fdivp"           , Enc(FpuRDef)           , O_FPU(00,DEF8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 624 , 75 ),
  INST(Fdivr           , "fdivr"           , Enc(FpuArith)          , O_FPU(00,F8F0,7)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , 630 , 74 ),
  INST(Fdivrp          , "fdivrp"          , Enc(FpuRDef)           , O_FPU(00,DEF0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 636 , 75 ),
  INST(Femms           , "femms"           , Enc(X86Op)             , O(000F00,0E,_,_,_,_,_,_  ), 0                         , F(Fp)                                 , EF(________), 0 , 0 , 643 , 83 ),
  INST(Ffree           , "ffree"           , Enc(FpuR)              , O_FPU(00,DDC0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 649 , 84 ),
  INST(Fiadd           , "fiadd"           , Enc(FpuM)              , O_FPU(00,00DA,0)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , 655 , 85 ),
  INST(Ficom           , "ficom"           , Enc(FpuM)              , O_FPU(00,00DA,2)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , 661 , 85 ),
  INST(Ficomp          , "ficomp"          , Enc(FpuM)              , O_FPU(00,00DA,3)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , 667 , 85 ),
  INST(Fidiv           , "fidiv"           , Enc(FpuM)              , O_FPU(00,00DA,6)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , 674 , 85 ),
  INST(Fidivr          , "fidivr"          , Enc(FpuM)              , O_FPU(00,00DA,7)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , 680 , 85 ),
  INST(Fild            , "fild"            , Enc(FpuM)              , O_FPU(00,00DB,0)          , O_FPU(00,00DF,5)          , F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , EF(________), 0 , 0 , 687 , 86 ),
  INST(Fimul           , "fimul"           , Enc(FpuM)              , O_FPU(00,00DA,1)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , 692 , 85 ),
  INST(Fincstp         , "fincstp"         , Enc(FpuOp)             , O_FPU(00,D9F7,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 698 , 73 ),
  INST(Finit           , "finit"           , Enc(FpuOp)             , O_FPU(9B,DBE3,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 706 , 73 ),
  INST(Fist            , "fist"            , Enc(FpuM)              , O_FPU(00,00DB,2)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , 712 , 85 ),
  INST(Fistp           , "fistp"           , Enc(FpuM)              , O_FPU(00,00DB,3)          , O_FPU(00,00DF,7)          , F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , EF(________), 0 , 0 , 717 , 87 ),
  INST(Fisttp          , "fisttp"          , Enc(FpuM)              , O_FPU(00,00DB,1)          , O_FPU(00,00DD,1)          , F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , EF(________), 0 , 0 , 723 , 88 ),
  INST(Fisub           , "fisub"           , Enc(FpuM)              , O_FPU(00,00DA,4)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , 730 , 85 ),
  INST(Fisubr          , "fisubr"          , Enc(FpuM)              , O_FPU(00,00DA,5)          , 0                         , F(Fp)|F(FPU_M2)|F(FPU_M4)             , EF(________), 0 , 0 , 736 , 85 ),
  INST(Fld             , "fld"             , Enc(FpuFldFst)         , O_FPU(00,00D9,0)          , O_FPU(00,00DB,5)          , F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , EF(________), 0 , 0 , 743 , 89 ),
  INST(Fld1            , "fld1"            , Enc(FpuOp)             , O_FPU(00,D9E8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 747 , 73 ),
  INST(Fldcw           , "fldcw"           , Enc(X86M_Only)         , O_FPU(00,00D9,5)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 752 , 90 ),
  INST(Fldenv          , "fldenv"          , Enc(X86M_Only)         , O_FPU(00,00D9,4)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 758 , 91 ),
  INST(Fldl2e          , "fldl2e"          , Enc(FpuOp)             , O_FPU(00,D9EA,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 765 , 73 ),
  INST(Fldl2t          , "fldl2t"          , Enc(FpuOp)             , O_FPU(00,D9E9,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 772 , 73 ),
  INST(Fldlg2          , "fldlg2"          , Enc(FpuOp)             , O_FPU(00,D9EC,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 779 , 73 ),
  INST(Fldln2          , "fldln2"          , Enc(FpuOp)             , O_FPU(00,D9ED,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 786 , 73 ),
  INST(Fldpi           , "fldpi"           , Enc(FpuOp)             , O_FPU(00,D9EB,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 793 , 73 ),
  INST(Fldz            , "fldz"            , Enc(FpuOp)             , O_FPU(00,D9EE,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 799 , 73 ),
  INST(Fmul            , "fmul"            , Enc(FpuArith)          , O_FPU(00,C8C8,1)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , 1798, 74 ),
  INST(Fmulp           , "fmulp"           , Enc(FpuRDef)           , O_FPU(00,DEC8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 804 , 75 ),
  INST(Fnclex          , "fnclex"          , Enc(FpuOp)             , O_FPU(00,DBE2,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 810 , 73 ),
  INST(Fninit          , "fninit"          , Enc(FpuOp)             , O_FPU(00,DBE3,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 817 , 73 ),
  INST(Fnop            , "fnop"            , Enc(FpuOp)             , O_FPU(00,D9D0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 824 , 73 ),
  INST(Fnsave          , "fnsave"          , Enc(X86M_Only)         , O_FPU(00,00DD,6)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 829 , 91 ),
  INST(Fnstcw          , "fnstcw"          , Enc(X86M_Only)         , O_FPU(00,00D9,7)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 836 , 90 ),
  INST(Fnstenv         , "fnstenv"         , Enc(X86M_Only)         , O_FPU(00,00D9,6)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 843 , 91 ),
  INST(Fnstsw          , "fnstsw"          , Enc(FpuStsw)           , O_FPU(00,00DD,7)          , O_FPU(00,DFE0,_)          , F(Fp)                                 , EF(________), 0 , 0 , 851 , 92 ),
  INST(Fpatan          , "fpatan"          , Enc(FpuOp)             , O_FPU(00,D9F3,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 858 , 73 ),
  INST(Fprem           , "fprem"           , Enc(FpuOp)             , O_FPU(00,D9F8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 865 , 73 ),
  INST(Fprem1          , "fprem1"          , Enc(FpuOp)             , O_FPU(00,D9F5,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 871 , 73 ),
  INST(Fptan           , "fptan"           , Enc(FpuOp)             , O_FPU(00,D9F2,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 878 , 73 ),
  INST(Frndint         , "frndint"         , Enc(FpuOp)             , O_FPU(00,D9FC,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 884 , 73 ),
  INST(Frstor          , "frstor"          , Enc(X86M_Only)         , O_FPU(00,00DD,4)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 892 , 91 ),
  INST(Fsave           , "fsave"           , Enc(X86M_Only)         , O_FPU(9B,00DD,6)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 899 , 91 ),
  INST(Fscale          , "fscale"          , Enc(FpuOp)             , O_FPU(00,D9FD,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 905 , 73 ),
  INST(Fsin            , "fsin"            , Enc(FpuOp)             , O_FPU(00,D9FE,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 912 , 73 ),
  INST(Fsincos         , "fsincos"         , Enc(FpuOp)             , O_FPU(00,D9FB,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 917 , 73 ),
  INST(Fsqrt           , "fsqrt"           , Enc(FpuOp)             , O_FPU(00,D9FA,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 925 , 73 ),
  INST(Fst             , "fst"             , Enc(FpuFldFst)         , O_FPU(00,00D9,2)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , 931 , 93 ),
  INST(Fstcw           , "fstcw"           , Enc(X86M_Only)         , O_FPU(9B,00D9,7)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 935 , 90 ),
  INST(Fstenv          , "fstenv"          , Enc(X86M_Only)         , O_FPU(9B,00D9,6)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 941 , 91 ),
  INST(Fstp            , "fstp"            , Enc(FpuFldFst)         , O_FPU(00,00D9,3)          , O(000000,DB,7,_,_,_,_,_  ), F(Fp)|F(FPU_M4)|F(FPU_M8)|F(FPU_M10)  , EF(________), 0 , 0 , 948 , 94 ),
  INST(Fstsw           , "fstsw"           , Enc(FpuStsw)           , O_FPU(9B,00DD,7)          , O_FPU(9B,DFE0,_)          , F(Fp)                                 , EF(________), 0 , 0 , 953 , 95 ),
  INST(Fsub            , "fsub"            , Enc(FpuArith)          , O_FPU(00,E0E8,4)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , 1876, 74 ),
  INST(Fsubp           , "fsubp"           , Enc(FpuRDef)           , O_FPU(00,DEE8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 959 , 75 ),
  INST(Fsubr           , "fsubr"           , Enc(FpuArith)          , O_FPU(00,E8E0,5)          , 0                         , F(Fp)|F(FPU_M4)|F(FPU_M8)             , EF(________), 0 , 0 , 1882, 74 ),
  INST(Fsubrp          , "fsubrp"          , Enc(FpuRDef)           , O_FPU(00,DEE0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 965 , 75 ),
  INST(Ftst            , "ftst"            , Enc(FpuOp)             , O_FPU(00,D9E4,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 972 , 73 ),
  INST(Fucom           , "fucom"           , Enc(FpuRDef)           , O_FPU(00,DDE0,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 977 , 75 ),
  INST(Fucomi          , "fucomi"          , Enc(FpuR)              , O_FPU(00,DBE8,_)          , 0                         , F(Fp)                                 , EF(WWWWWW__), 0 , 0 , 983 , 82 ),
  INST(Fucomip         , "fucomip"         , Enc(FpuR)              , O_FPU(00,DFE8,_)          , 0                         , F(Fp)                                 , EF(WWWWWW__), 0 , 0 , 990 , 82 ),
  INST(Fucomp          , "fucomp"          , Enc(FpuRDef)           , O_FPU(00,DDE8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 998 , 75 ),
  INST(Fucompp         , "fucompp"         , Enc(FpuOp)             , O_FPU(00,DAE9,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 1005, 73 ),
  INST(Fwait           , "fwait"           , Enc(X86Op)             , O_FPU(00,00DB,_)          , 0                         , F(Fp)|F(Volatile)                     , EF(________), 0 , 0 , 1013, 96 ),
  INST(Fxam            , "fxam"            , Enc(FpuOp)             , O_FPU(00,D9E5,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 1019, 73 ),
  INST(Fxch            , "fxch"            , Enc(FpuR)              , O_FPU(00,D9C8,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 1024, 97 ),
  INST(Fxrstor         , "fxrstor"         , Enc(X86M_Only)         , O(000F00,AE,1,_,_,_,_,_  ), 0                         , F(Fp)                                 , EF(________), 0 , 0 , 1029, 91 ),
  INST(Fxrstor64       , "fxrstor64"       , Enc(X86M_Only)         , O(000F00,AE,1,_,1,_,_,_  ), 0                         , F(Fp)                                 , EF(________), 0 , 0 , 1037, 98 ),
  INST(Fxsave          , "fxsave"          , Enc(X86M_Only)         , O(000F00,AE,0,_,_,_,_,_  ), 0                         , F(Fp)                                 , EF(________), 0 , 0 , 1047, 91 ),
  INST(Fxsave64        , "fxsave64"        , Enc(X86M_Only)         , O(000F00,AE,0,_,1,_,_,_  ), 0                         , F(Fp)                                 , EF(________), 0 , 0 , 1054, 98 ),
  INST(Fxtract         , "fxtract"         , Enc(FpuOp)             , O_FPU(00,D9F4,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 1063, 73 ),
  INST(Fyl2x           , "fyl2x"           , Enc(FpuOp)             , O_FPU(00,D9F1,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 1071, 73 ),
  INST(Fyl2xp1         , "fyl2xp1"         , Enc(FpuOp)             , O_FPU(00,D9F9,_)          , 0                         , F(Fp)                                 , EF(________), 0 , 0 , 1077, 73 ),
  INST(Haddpd          , "haddpd"          , Enc(ExtRm)             , O(660F00,7C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5308, 4  ),
  INST(Haddps          , "haddps"          , Enc(ExtRm)             , O(F20F00,7C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5316, 4  ),
  INST(Hsubpd          , "hsubpd"          , Enc(ExtRm)             , O(660F00,7D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5324, 4  ),
  INST(Hsubps          , "hsubps"          , Enc(ExtRm)             , O(F20F00,7D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5332, 4  ),
  INST(Idiv            , "idiv"            , Enc(X86M_Bx_MulDiv)    , O(000000,F6,7,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(UUUUUU__), 0 , 0 , 675 , 99 ),
  INST(Imul            , "imul"            , Enc(X86Imul)           , O(000000,F6,5,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(WUUUUW__), 0 , 0 , 693 , 100),
  INST(Inc             , "inc"             , Enc(X86IncDec)         , O(000000,FE,0,_,x,_,_,_  ), O(000000,40,_,_,x,_,_,_  ), F(RW)|F(Lock)                         , EF(WWWWW___), 0 , 0 , 1085, 101),
  INST(Insertps        , "insertps"        , Enc(ExtRmi)            , O(660F3A,21,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5468, 43 ),
  INST(Insertq         , "insertq"         , Enc(ExtInsertq)        , O(F20F00,79,_,_,_,_,_,_  ), O(F20F00,78,_,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 1089, 102),
  INST(Int             , "int"             , Enc(X86Int)            , O(000000,CD,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(___W___W), 0 , 0 , 888 , 103),
  INST(Int3            , "int3"            , Enc(X86Op)             , O(000000,CC,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(___W___W), 0 , 0 , 1097, 104),
  INST(Into            , "into"            , Enc(X86Op)             , O(000000,CE,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(___W___W), 0 , 0 , 1102, 104),
  INST(Ja              , "ja"              , Enc(X86Jcc)            , O(000000,77,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(__R__R__), 0 , 0 , 1107, 105),
  INST(Jae             , "jae"             , Enc(X86Jcc)            , O(000000,73,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(_____R__), 0 , 0 , 1110, 106),
  INST(Jb              , "jb"              , Enc(X86Jcc)            , O(000000,72,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(_____R__), 0 , 0 , 1114, 106),
  INST(Jbe             , "jbe"             , Enc(X86Jcc)            , O(000000,76,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(__R__R__), 0 , 0 , 1117, 105),
  INST(Jc              , "jc"              , Enc(X86Jcc)            , O(000000,72,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(_____R__), 0 , 0 , 1121, 107),
  INST(Je              , "je"              , Enc(X86Jcc)            , O(000000,74,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(__R_____), 0 , 0 , 1124, 108),
  INST(Jg              , "jg"              , Enc(X86Jcc)            , O(000000,7F,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(RRR_____), 0 , 0 , 1133, 109),
  INST(Jge             , "jge"             , Enc(X86Jcc)            , O(000000,7D,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(RR______), 0 , 0 , 1136, 110),
  INST(Jl              , "jl"              , Enc(X86Jcc)            , O(000000,7C,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(RR______), 0 , 0 , 1140, 110),
  INST(Jle             , "jle"             , Enc(X86Jcc)            , O(000000,7E,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(RRR_____), 0 , 0 , 1143, 109),
  INST(Jna             , "jna"             , Enc(X86Jcc)            , O(000000,76,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(__R__R__), 0 , 0 , 1151, 105),
  INST(Jnae            , "jnae"            , Enc(X86Jcc)            , O(000000,72,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(_____R__), 0 , 0 , 1155, 106),
  INST(Jnb             , "jnb"             , Enc(X86Jcc)            , O(000000,73,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(_____R__), 0 , 0 , 1160, 106),
  INST(Jnbe            , "jnbe"            , Enc(X86Jcc)            , O(000000,77,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(__R__R__), 0 , 0 , 1164, 105),
  INST(Jnc             , "jnc"             , Enc(X86Jcc)            , O(000000,73,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(_____R__), 0 , 0 , 1169, 107),
  INST(Jne             , "jne"             , Enc(X86Jcc)            , O(000000,75,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(__R_____), 0 , 0 , 1173, 108),
  INST(Jng             , "jng"             , Enc(X86Jcc)            , O(000000,7E,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(RRR_____), 0 , 0 , 1177, 109),
  INST(Jnge            , "jnge"            , Enc(X86Jcc)            , O(000000,7C,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(RR______), 0 , 0 , 1181, 110),
  INST(Jnl             , "jnl"             , Enc(X86Jcc)            , O(000000,7D,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(RR______), 0 , 0 , 1186, 110),
  INST(Jnle            , "jnle"            , Enc(X86Jcc)            , O(000000,7F,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(RRR_____), 0 , 0 , 1190, 109),
  INST(Jno             , "jno"             , Enc(X86Jcc)            , O(000000,71,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(R_______), 0 , 0 , 1195, 111),
  INST(Jnp             , "jnp"             , Enc(X86Jcc)            , O(000000,7B,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(____R___), 0 , 0 , 1199, 112),
  INST(Jns             , "jns"             , Enc(X86Jcc)            , O(000000,79,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(_R______), 0 , 0 , 1203, 113),
  INST(Jnz             , "jnz"             , Enc(X86Jcc)            , O(000000,75,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(__R_____), 0 , 0 , 1207, 108),
  INST(Jo              , "jo"              , Enc(X86Jcc)            , O(000000,70,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(R_______), 0 , 0 , 1211, 111),
  INST(Jp              , "jp"              , Enc(X86Jcc)            , O(000000,7A,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(____R___), 0 , 0 , 1214, 112),
  INST(Jpe             , "jpe"             , Enc(X86Jcc)            , O(000000,7A,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(____R___), 0 , 0 , 1217, 112),
  INST(Jpo             , "jpo"             , Enc(X86Jcc)            , O(000000,7B,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(____R___), 0 , 0 , 1221, 112),
  INST(Js              , "js"              , Enc(X86Jcc)            , O(000000,78,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(_R______), 0 , 0 , 1225, 113),
  INST(Jz              , "jz"              , Enc(X86Jcc)            , O(000000,74,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)                   , EF(__R_____), 0 , 0 , 1228, 108),
  INST(Jecxz           , "jecxz"           , Enc(X86Jecxz)          , O(000000,E3,_,_,_,_,_,_  ), 0                         , F(Flow)|F(Volatile)|F(Special)        , EF(________), 0 , 0 , 1127, 114),
  INST(Jmp             , "jmp"             , Enc(X86Jmp)            , O(000000,FF,4,_,_,_,_,_  ), O(000000,E9,_,_,_,_,_,_  ), F(Flow)|F(Volatile)                   , EF(________), 0 , 0 , 1147, 115),
  INST(Kaddb           , "kaddb"           , Enc(VexRvm)            , V(660F00,4A,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1231, 116),
  INST(Kaddd           , "kaddd"           , Enc(VexRvm)            , V(660F00,4A,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1237, 116),
  INST(Kaddq           , "kaddq"           , Enc(VexRvm)            , V(000F00,4A,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1243, 116),
  INST(Kaddw           , "kaddw"           , Enc(VexRvm)            , V(000F00,4A,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1249, 116),
  INST(Kandb           , "kandb"           , Enc(VexRvm)            , V(660F00,41,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1255, 116),
  INST(Kandd           , "kandd"           , Enc(VexRvm)            , V(660F00,41,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1261, 116),
  INST(Kandnb          , "kandnb"          , Enc(VexRvm)            , V(660F00,42,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1267, 116),
  INST(Kandnd          , "kandnd"          , Enc(VexRvm)            , V(660F00,42,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1274, 116),
  INST(Kandnq          , "kandnq"          , Enc(VexRvm)            , V(000F00,42,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1281, 116),
  INST(Kandnw          , "kandnw"          , Enc(VexRvm)            , V(000F00,42,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1288, 116),
  INST(Kandq           , "kandq"           , Enc(VexRvm)            , V(000F00,41,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1295, 116),
  INST(Kandw           , "kandw"           , Enc(VexRvm)            , V(000F00,41,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1301, 116),
  INST(Kmovb           , "kmovb"           , Enc(VexKmov)           , V(660F00,90,_,0,0,_,_,_  ), V(660F00,92,_,0,0,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1307, 117),
  INST(Kmovd           , "kmovd"           , Enc(VexKmov)           , V(660F00,90,_,0,1,_,_,_  ), V(F20F00,92,_,0,0,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7249, 118),
  INST(Kmovq           , "kmovq"           , Enc(VexKmov)           , V(000F00,90,_,0,1,_,_,_  ), V(F20F00,92,_,0,1,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7260, 119),
  INST(Kmovw           , "kmovw"           , Enc(VexKmov)           , V(000F00,90,_,0,0,_,_,_  ), V(000F00,92,_,0,0,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1313, 120),
  INST(Knotb           , "knotb"           , Enc(VexRm)             , V(660F00,44,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1319, 121),
  INST(Knotd           , "knotd"           , Enc(VexRm)             , V(660F00,44,_,0,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1325, 121),
  INST(Knotq           , "knotq"           , Enc(VexRm)             , V(000F00,44,_,0,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1331, 121),
  INST(Knotw           , "knotw"           , Enc(VexRm)             , V(000F00,44,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1337, 121),
  INST(Korb            , "korb"            , Enc(VexRvm)            , V(660F00,45,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1343, 116),
  INST(Kord            , "kord"            , Enc(VexRvm)            , V(660F00,45,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1348, 116),
  INST(Korq            , "korq"            , Enc(VexRvm)            , V(000F00,45,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1353, 116),
  INST(Kortestb        , "kortestb"        , Enc(VexRm)             , V(660F00,98,_,0,0,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 1358, 122),
  INST(Kortestd        , "kortestd"        , Enc(VexRm)             , V(660F00,98,_,0,1,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 1367, 122),
  INST(Kortestq        , "kortestq"        , Enc(VexRm)             , V(000F00,98,_,0,1,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 1376, 122),
  INST(Kortestw        , "kortestw"        , Enc(VexRm)             , V(000F00,98,_,0,0,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 1385, 122),
  INST(Korw            , "korw"            , Enc(VexRvm)            , V(000F00,45,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1394, 116),
  INST(Kshiftlb        , "kshiftlb"        , Enc(VexRmi)            , V(660F3A,32,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1399, 123),
  INST(Kshiftld        , "kshiftld"        , Enc(VexRmi)            , V(660F3A,33,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1408, 123),
  INST(Kshiftlq        , "kshiftlq"        , Enc(VexRmi)            , V(660F3A,33,_,0,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1417, 123),
  INST(Kshiftlw        , "kshiftlw"        , Enc(VexRmi)            , V(660F3A,32,_,0,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1426, 123),
  INST(Kshiftrb        , "kshiftrb"        , Enc(VexRmi)            , V(660F3A,30,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1435, 123),
  INST(Kshiftrd        , "kshiftrd"        , Enc(VexRmi)            , V(660F3A,31,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1444, 123),
  INST(Kshiftrq        , "kshiftrq"        , Enc(VexRmi)            , V(660F3A,31,_,0,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1453, 123),
  INST(Kshiftrw        , "kshiftrw"        , Enc(VexRmi)            , V(660F3A,30,_,0,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1462, 123),
  INST(Ktestb          , "ktestb"          , Enc(VexRm)             , V(660F00,99,_,0,0,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 1471, 122),
  INST(Ktestd          , "ktestd"          , Enc(VexRm)             , V(660F00,99,_,0,1,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 1478, 122),
  INST(Ktestq          , "ktestq"          , Enc(VexRm)             , V(000F00,99,_,0,1,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 1485, 122),
  INST(Ktestw          , "ktestw"          , Enc(VexRm)             , V(000F00,99,_,0,0,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 1492, 122),
  INST(Kunpckbw        , "kunpckbw"        , Enc(VexRvm)            , V(660F00,4B,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1499, 116),
  INST(Kunpckdq        , "kunpckdq"        , Enc(VexRvm)            , V(000F00,4B,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1508, 116),
  INST(Kunpckwd        , "kunpckwd"        , Enc(VexRvm)            , V(000F00,4B,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1517, 116),
  INST(Kxnorb          , "kxnorb"          , Enc(VexRvm)            , V(660F00,46,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1526, 116),
  INST(Kxnord          , "kxnord"          , Enc(VexRvm)            , V(660F00,46,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1533, 116),
  INST(Kxnorq          , "kxnorq"          , Enc(VexRvm)            , V(000F00,46,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1540, 116),
  INST(Kxnorw          , "kxnorw"          , Enc(VexRvm)            , V(000F00,46,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1547, 116),
  INST(Kxorb           , "kxorb"           , Enc(VexRvm)            , V(660F00,47,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1554, 116),
  INST(Kxord           , "kxord"           , Enc(VexRvm)            , V(660F00,47,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1560, 116),
  INST(Kxorq           , "kxorq"           , Enc(VexRvm)            , V(000F00,47,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1566, 116),
  INST(Kxorw           , "kxorw"           , Enc(VexRvm)            , V(000F00,47,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 1572, 116),
  INST(Lahf            , "lahf"            , Enc(X86Op)             , O(000000,9F,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(_RRRRR__), 0 , 0 , 1578, 124),
  INST(Lddqu           , "lddqu"           , Enc(ExtRm)             , O(F20F00,F0,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 5478, 125),
  INST(Ldmxcsr         , "ldmxcsr"         , Enc(X86M_Only)         , O(000F00,AE,2,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , 5485, 126),
  INST(Lea             , "lea"             , Enc(X86Lea)            , O(000000,8D,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 1583, 127),
  INST(Leave           , "leave"           , Enc(X86Op)             , O(000000,C9,_,_,_,_,_,_  ), 0                         , F(Volatile)|F(Special)                , EF(________), 0 , 0 , 1587, 128),
  INST(Lfence          , "lfence"          , Enc(X86Fence)          , O(000F00,AE,5,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(________), 0 , 0 , 1593, 129),
  INST(LodsB           , "lods_b"          , Enc(X86Op)             , O(000000,AC,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(______R_), 0 , 1 , 2062, 130),
  INST(LodsD           , "lods_d"          , Enc(X86Op)             , O(000000,AD,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(______R_), 0 , 4 , 2073, 131),
  INST(LodsQ           , "lods_q"          , Enc(X86Op)             , O(000000,AD,_,_,1,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(______R_), 0 , 8 , 2084, 132),
  INST(LodsW           , "lods_w"          , Enc(X86Op)             , O(660000,AD,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(______R_), 0 , 2 , 2095, 133),
  INST(Lzcnt           , "lzcnt"           , Enc(X86Rm)             , O(F30F00,BD,_,_,x,_,_,_  ), 0                         , F(RW)                                 , EF(UUWUUW__), 0 , 0 , 1600, 134),
  INST(Maskmovdqu      , "maskmovdqu"      , Enc(ExtRmZDI)          , O(660F00,57,_,_,_,_,_,_  ), 0                         , F(RO)|F(Special)                      , EF(________), 0 , 0 , 5494, 135),
  INST(Maskmovq        , "maskmovq"        , Enc(ExtRmZDI)          , O(000F00,F7,_,_,_,_,_,_  ), 0                         , F(RO)|F(Special)                      , EF(________), 0 , 0 , 7257, 136),
  INST(Maxpd           , "maxpd"           , Enc(ExtRm)             , O(660F00,5F,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5528, 4  ),
  INST(Maxps           , "maxps"           , Enc(ExtRm)             , O(000F00,5F,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5535, 4  ),
  INST(Maxsd           , "maxsd"           , Enc(ExtRm)             , O(F20F00,5F,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7276, 5  ),
  INST(Maxss           , "maxss"           , Enc(ExtRm)             , O(F30F00,5F,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5549, 6  ),
  INST(Mfence          , "mfence"          , Enc(X86Fence)          , O(000F00,AE,6,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)                     , EF(________), 0 , 0 , 1606, 137),
  INST(Minpd           , "minpd"           , Enc(ExtRm)             , O(660F00,5D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5556, 4  ),
  INST(Minps           , "minps"           , Enc(ExtRm)             , O(000F00,5D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5563, 4  ),
  INST(Minsd           , "minsd"           , Enc(ExtRm)             , O(F20F00,5D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7340, 5  ),
  INST(Minss           , "minss"           , Enc(ExtRm)             , O(F30F00,5D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5577, 6  ),
  INST(Monitor         , "monitor"         , Enc(X86Op)             , O(000F01,C8,_,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 1613, 138),
  INST(Mov             , "mov"             , Enc(X86Mov)            , 0                         , 0                         , F(WO)                                 , EF(________), 0 , 0 , 6223, 139),
  INST(Movapd          , "movapd"          , Enc(ExtMov)            , O(660F00,28,_,_,_,_,_,_  ), O(660F00,29,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 5584, 140),
  INST(Movaps          , "movaps"          , Enc(ExtMov)            , O(000F00,28,_,_,_,_,_,_  ), O(000F00,29,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 5592, 141),
  INST(Movbe           , "movbe"           , Enc(ExtMovbe)          , O(000F38,F0,_,_,x,_,_,_  ), O(000F38,F1,_,_,x,_,_,_  ), F(WO)                                 , EF(________), 0 , 0 , 522 , 142),
  INST(Movd            , "movd"            , Enc(ExtMovd)           , O(000F00,6E,_,_,_,_,_,_  ), O(000F00,7E,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 7250, 143),
  INST(Movddup         , "movddup"         , Enc(ExtMov)            , O(F20F00,12,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 5606, 144),
  INST(Movdq2q         , "movdq2q"         , Enc(ExtMov)            , O(F20F00,D6,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 1621, 145),
  INST(Movdqa          , "movdqa"          , Enc(ExtMov)            , O(660F00,6F,_,_,_,_,_,_  ), O(660F00,7F,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 5615, 146),
  INST(Movdqu          , "movdqu"          , Enc(ExtMov)            , O(F30F00,6F,_,_,_,_,_,_  ), O(F30F00,7F,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 5498, 147),
  INST(Movhlps         , "movhlps"         , Enc(ExtMov)            , O(000F00,12,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 5690, 148),
  INST(Movhpd          , "movhpd"          , Enc(ExtMov)            , O(660F00,16,_,_,_,_,_,_  ), O(660F00,17,_,_,_,_,_,_  ), F(RW)                                 , EF(________), 8 , 8 , 5699, 149),
  INST(Movhps          , "movhps"          , Enc(ExtMov)            , O(000F00,16,_,_,_,_,_,_  ), O(000F00,17,_,_,_,_,_,_  ), F(RW)                                 , EF(________), 8 , 8 , 5707, 150),
  INST(Movlhps         , "movlhps"         , Enc(ExtMov)            , O(000F00,16,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 8 , 8 , 5715, 151),
  INST(Movlpd          , "movlpd"          , Enc(ExtMov)            , O(660F00,12,_,_,_,_,_,_  ), O(660F00,13,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 8 , 5724, 152),
  INST(Movlps          , "movlps"          , Enc(ExtMov)            , O(000F00,12,_,_,_,_,_,_  ), O(000F00,13,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 8 , 5732, 153),
  INST(Movmskpd        , "movmskpd"        , Enc(ExtMov)            , O(660F00,50,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 5740, 154),
  INST(Movmskps        , "movmskps"        , Enc(ExtMov)            , O(000F00,50,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 5750, 154),
  INST(Movntdq         , "movntdq"         , Enc(ExtMov)            , 0                         , O(660F00,E7,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 5760, 155),
  INST(Movntdqa        , "movntdqa"        , Enc(ExtMov)            , O(660F38,2A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 5769, 156),
  INST(Movnti          , "movnti"          , Enc(ExtMovnti)         , O(000F00,C3,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 1629, 157),
  INST(Movntpd         , "movntpd"         , Enc(ExtMov)            , 0                         , O(660F00,2B,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 5779, 158),
  INST(Movntps         , "movntps"         , Enc(ExtMov)            , 0                         , O(000F00,2B,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 5788, 159),
  INST(Movntq          , "movntq"          , Enc(ExtMov)            , 0                         , O(000F00,E7,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 8 , 1636, 160),
  INST(Movntsd         , "movntsd"         , Enc(ExtMov)            , 0                         , O(F20F00,2B,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 8 , 1643, 161),
  INST(Movntss         , "movntss"         , Enc(ExtMov)            , 0                         , O(F30F00,2B,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 4 , 1651, 162),
  INST(Movq            , "movq"            , Enc(ExtMovq)           , O(000F00,6E,_,_,x,_,_,_  ), O(000F00,7E,_,_,x,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 7261, 163),
  INST(Movq2dq         , "movq2dq"         , Enc(ExtRm)             , O(F30F00,D6,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 1659, 164),
  INST(MovsB           , "movs_b"          , Enc(X86Op)             , O(000000,A4,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , 2106, 165),
  INST(MovsD           , "movs_d"          , Enc(X86Op)             , O(000000,A5,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , 2117, 165),
  INST(MovsQ           , "movs_q"          , Enc(X86Op)             , O(000000,A5,_,_,1,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , 2128, 165),
  INST(MovsW           , "movs_w"          , Enc(X86Op)             , O(660000,A5,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , 2139, 165),
  INST(Movsd           , "movsd"           , Enc(ExtMov)            , O(F20F00,10,_,_,_,_,_,_  ), O(F20F00,11,_,_,_,_,_,_  ), F(WO)|F(ZeroIfMem)                    , EF(________), 0 , 8 , 5803, 166),
  INST(Movshdup        , "movshdup"        , Enc(ExtRm)             , O(F30F00,16,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 5810, 53 ),
  INST(Movsldup        , "movsldup"        , Enc(ExtRm)             , O(F30F00,12,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 5820, 53 ),
  INST(Movss           , "movss"           , Enc(ExtMov)            , O(F30F00,10,_,_,_,_,_,_  ), O(F30F00,11,_,_,_,_,_,_  ), F(WO)|F(ZeroIfMem)                    , EF(________), 0 , 4 , 5830, 167),
  INST(Movsx           , "movsx"           , Enc(X86MovsxMovzx)     , O(000F00,BE,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 1667, 168),
  INST(Movsxd          , "movsxd"          , Enc(X86Rm)             , O(000000,63,_,_,1,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 1673, 169),
  INST(Movupd          , "movupd"          , Enc(ExtMov)            , O(660F00,10,_,_,_,_,_,_  ), O(660F00,11,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 5837, 170),
  INST(Movups          , "movups"          , Enc(ExtMov)            , O(000F00,10,_,_,_,_,_,_  ), O(000F00,11,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 16, 5845, 171),
  INST(Movzx           , "movzx"           , Enc(X86MovsxMovzx)     , O(000F00,B6,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 1680, 168),
  INST(Mpsadbw         , "mpsadbw"         , Enc(ExtRmi)            , O(660F3A,42,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5853, 13 ),
  INST(Mul             , "mul"             , Enc(X86M_Bx_MulDiv)    , O(000000,F6,4,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(WUUUUW__), 0 , 0 , 694 , 172),
  INST(Mulpd           , "mulpd"           , Enc(ExtRm)             , O(660F00,59,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5862, 4  ),
  INST(Mulps           , "mulps"           , Enc(ExtRm)             , O(000F00,59,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5869, 4  ),
  INST(Mulsd           , "mulsd"           , Enc(ExtRm)             , O(F20F00,59,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5876, 5  ),
  INST(Mulss           , "mulss"           , Enc(ExtRm)             , O(F30F00,59,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5883, 6  ),
  INST(Mulx            , "mulx"            , Enc(VexRvmZDX_Wx)      , V(F20F38,F6,_,0,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 1686, 173),
  INST(Mwait           , "mwait"           , Enc(X86Op)             , O(000F01,C9,_,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 1691, 138),
  INST(Neg             , "neg"             , Enc(X86M_Bx)           , O(000000,F6,3,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWWWW__), 0 , 0 , 1697, 174),
  INST(Nop             , "nop"             , Enc(X86Op)             , O(000000,90,_,_,_,_,_,_  ), 0                         , 0                                     , EF(________), 0 , 0 , 825 , 175),
  INST(Not             , "not"             , Enc(X86M_Bx)           , O(000000,F6,2,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(________), 0 , 0 , 1701, 176),
  INST(Or              , "or"              , Enc(X86Arith)          , O(000000,08,1,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWUWW__), 0 , 0 , 1034, 3  ),
  INST(Orpd            , "orpd"            , Enc(ExtRm)             , O(660F00,56,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 9316, 4  ),
  INST(Orps            , "orps"            , Enc(ExtRm)             , O(000F00,56,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 9323, 4  ),
  INST(Pabsb           , "pabsb"           , Enc(ExtRm_P)           , O(000F38,1C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5902, 177),
  INST(Pabsd           , "pabsd"           , Enc(ExtRm_P)           , O(000F38,1E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5909, 177),
  INST(Pabsw           , "pabsw"           , Enc(ExtRm_P)           , O(000F38,1D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5923, 177),
  INST(Packssdw        , "packssdw"        , Enc(ExtRm_P)           , O(000F00,6B,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5930, 177),
  INST(Packsswb        , "packsswb"        , Enc(ExtRm_P)           , O(000F00,63,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5940, 177),
  INST(Packusdw        , "packusdw"        , Enc(ExtRm)             , O(660F38,2B,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5950, 4  ),
  INST(Packuswb        , "packuswb"        , Enc(ExtRm_P)           , O(000F00,67,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5960, 177),
  INST(Paddb           , "paddb"           , Enc(ExtRm_P)           , O(000F00,FC,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5970, 177),
  INST(Paddd           , "paddd"           , Enc(ExtRm_P)           , O(000F00,FE,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5977, 177),
  INST(Paddq           , "paddq"           , Enc(ExtRm_P)           , O(000F00,D4,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5984, 177),
  INST(Paddsb          , "paddsb"          , Enc(ExtRm_P)           , O(000F00,EC,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5991, 177),
  INST(Paddsw          , "paddsw"          , Enc(ExtRm_P)           , O(000F00,ED,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 5999, 177),
  INST(Paddusb         , "paddusb"         , Enc(ExtRm_P)           , O(000F00,DC,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6007, 177),
  INST(Paddusw         , "paddusw"         , Enc(ExtRm_P)           , O(000F00,DD,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6016, 177),
  INST(Paddw           , "paddw"           , Enc(ExtRm_P)           , O(000F00,FD,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6025, 177),
  INST(Palignr         , "palignr"         , Enc(ExtRmi_P)          , O(000F3A,0F,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6032, 178),
  INST(Pand            , "pand"            , Enc(ExtRm_P)           , O(000F00,DB,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6041, 177),
  INST(Pandn           , "pandn"           , Enc(ExtRm_P)           , O(000F00,DF,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6054, 177),
  INST(Pause           , "pause"           , Enc(X86Op)             , O(F30000,90,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1705, 179),
  INST(Pavgb           , "pavgb"           , Enc(ExtRm_P)           , O(000F00,E0,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6084, 177),
  INST(Pavgusb         , "pavgusb"         , Enc(Ext3dNow)          , O(000F0F,BF,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1711, 180),
  INST(Pavgw           , "pavgw"           , Enc(ExtRm_P)           , O(000F00,E3,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6091, 177),
  INST(Pblendvb        , "pblendvb"        , Enc(ExtRmXMM0)         , O(660F38,10,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 6107, 14 ),
  INST(Pblendw         , "pblendw"         , Enc(ExtRmi)            , O(660F3A,0E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6117, 13 ),
  INST(Pclmulqdq       , "pclmulqdq"       , Enc(ExtRmi)            , O(660F3A,44,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6210, 13 ),
  INST(Pcmpeqb         , "pcmpeqb"         , Enc(ExtRm_P)           , O(000F00,74,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6242, 177),
  INST(Pcmpeqd         , "pcmpeqd"         , Enc(ExtRm_P)           , O(000F00,76,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6251, 177),
  INST(Pcmpeqq         , "pcmpeqq"         , Enc(ExtRm)             , O(660F38,29,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6260, 4  ),
  INST(Pcmpeqw         , "pcmpeqw"         , Enc(ExtRm_P)           , O(000F00,75,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6269, 177),
  INST(Pcmpestri       , "pcmpestri"       , Enc(ExtRmi)            , O(660F3A,61,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , 6278, 181),
  INST(Pcmpestrm       , "pcmpestrm"       , Enc(ExtRmi)            , O(660F3A,60,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , 6289, 182),
  INST(Pcmpgtb         , "pcmpgtb"         , Enc(ExtRm_P)           , O(000F00,64,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6300, 177),
  INST(Pcmpgtd         , "pcmpgtd"         , Enc(ExtRm_P)           , O(000F00,66,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6309, 177),
  INST(Pcmpgtq         , "pcmpgtq"         , Enc(ExtRm)             , O(660F38,37,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6318, 4  ),
  INST(Pcmpgtw         , "pcmpgtw"         , Enc(ExtRm_P)           , O(000F00,65,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6327, 177),
  INST(Pcmpistri       , "pcmpistri"       , Enc(ExtRmi)            , O(660F3A,63,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , 6336, 183),
  INST(Pcmpistrm       , "pcmpistrm"       , Enc(ExtRmi)            , O(660F3A,62,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , 6347, 184),
  INST(Pcommit         , "pcommit"         , Enc(X86Op_O)           , O(660F00,AE,7,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(________), 0 , 0 , 1719, 185),
  INST(Pdep            , "pdep"            , Enc(VexRvm_Wx)         , V(F20F38,F5,_,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 1727, 186),
  INST(Pext            , "pext"            , Enc(VexRvm_Wx)         , V(F30F38,F5,_,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 1732, 186),
  INST(Pextrb          , "pextrb"          , Enc(ExtExtract)        , O(000F3A,14,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 6752, 187),
  INST(Pextrd          , "pextrd"          , Enc(ExtExtract)        , O(000F3A,16,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 6760, 71 ),
  INST(Pextrq          , "pextrq"          , Enc(ExtExtract)        , O(000F3A,16,_,_,1,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 6768, 188),
  INST(Pextrw          , "pextrw"          , Enc(ExtPextrw)         , O(000F00,C5,_,_,_,_,_,_  ), O(000F3A,15,_,_,_,_,_,_  ), F(WO)                                 , EF(________), 0 , 8 , 6776, 189),
  INST(Pf2id           , "pf2id"           , Enc(Ext3dNow)          , O(000F0F,1D,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 1737, 190),
  INST(Pf2iw           , "pf2iw"           , Enc(Ext3dNow)          , O(000F0F,1C,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 1743, 190),
  INST(Pfacc           , "pfacc"           , Enc(Ext3dNow)          , O(000F0F,AE,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1749, 180),
  INST(Pfadd           , "pfadd"           , Enc(Ext3dNow)          , O(000F0F,9E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1755, 180),
  INST(Pfcmpeq         , "pfcmpeq"         , Enc(Ext3dNow)          , O(000F0F,B0,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1761, 180),
  INST(Pfcmpge         , "pfcmpge"         , Enc(Ext3dNow)          , O(000F0F,90,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1769, 180),
  INST(Pfcmpgt         , "pfcmpgt"         , Enc(Ext3dNow)          , O(000F0F,A0,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1777, 180),
  INST(Pfmax           , "pfmax"           , Enc(Ext3dNow)          , O(000F0F,A4,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1785, 180),
  INST(Pfmin           , "pfmin"           , Enc(Ext3dNow)          , O(000F0F,94,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1791, 180),
  INST(Pfmul           , "pfmul"           , Enc(Ext3dNow)          , O(000F0F,B4,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1797, 180),
  INST(Pfnacc          , "pfnacc"          , Enc(Ext3dNow)          , O(000F0F,8A,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1803, 180),
  INST(Pfpnacc         , "pfpnacc"         , Enc(Ext3dNow)          , O(000F0F,8E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1810, 180),
  INST(Pfrcp           , "pfrcp"           , Enc(Ext3dNow)          , O(000F0F,96,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 1818, 190),
  INST(Pfrcpit1        , "pfrcpit1"        , Enc(Ext3dNow)          , O(000F0F,A6,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1824, 180),
  INST(Pfrcpit2        , "pfrcpit2"        , Enc(Ext3dNow)          , O(000F0F,B6,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1833, 180),
  INST(Pfrcpv          , "pfrcpv"          , Enc(Ext3dNow)          , O(000F0F,86,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1842, 180),
  INST(Pfrsqit1        , "pfrsqit1"        , Enc(Ext3dNow)          , O(000F0F,A7,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 1849, 191),
  INST(Pfrsqrt         , "pfrsqrt"         , Enc(Ext3dNow)          , O(000F0F,97,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 1858, 191),
  INST(Pfrsqrtv        , "pfrsqrtv"        , Enc(Ext3dNow)          , O(000F0F,87,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1866, 180),
  INST(Pfsub           , "pfsub"           , Enc(Ext3dNow)          , O(000F0F,9A,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1875, 180),
  INST(Pfsubr          , "pfsubr"          , Enc(Ext3dNow)          , O(000F0F,AA,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1881, 180),
  INST(Phaddd          , "phaddd"          , Enc(ExtRm_P)           , O(000F38,02,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6855, 177),
  INST(Phaddsw         , "phaddsw"         , Enc(ExtRm_P)           , O(000F38,03,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6872, 177),
  INST(Phaddw          , "phaddw"          , Enc(ExtRm_P)           , O(000F38,01,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6941, 177),
  INST(Phminposuw      , "phminposuw"      , Enc(ExtRm)             , O(660F38,41,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6967, 4  ),
  INST(Phsubd          , "phsubd"          , Enc(ExtRm_P)           , O(000F38,06,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 6988, 177),
  INST(Phsubsw         , "phsubsw"         , Enc(ExtRm_P)           , O(000F38,07,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7005, 177),
  INST(Phsubw          , "phsubw"          , Enc(ExtRm_P)           , O(000F38,05,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7014, 177),
  INST(Pi2fd           , "pi2fd"           , Enc(Ext3dNow)          , O(000F0F,0D,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 1888, 190),
  INST(Pi2fw           , "pi2fw"           , Enc(Ext3dNow)          , O(000F0F,0C,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 1894, 190),
  INST(Pinsrb          , "pinsrb"          , Enc(ExtRmi)            , O(660F3A,20,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7031, 192),
  INST(Pinsrd          , "pinsrd"          , Enc(ExtRmi)            , O(660F3A,22,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7039, 193),
  INST(Pinsrq          , "pinsrq"          , Enc(ExtRmi)            , O(660F3A,22,_,_,1,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7047, 194),
  INST(Pinsrw          , "pinsrw"          , Enc(ExtRmi_P)          , O(000F00,C4,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7055, 195),
  INST(Pmaddubsw       , "pmaddubsw"       , Enc(ExtRm_P)           , O(000F38,04,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7225, 177),
  INST(Pmaddwd         , "pmaddwd"         , Enc(ExtRm_P)           , O(000F00,F5,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7236, 177),
  INST(Pmaxsb          , "pmaxsb"          , Enc(ExtRm)             , O(660F38,3C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7267, 4  ),
  INST(Pmaxsd          , "pmaxsd"          , Enc(ExtRm)             , O(660F38,3D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7275, 4  ),
  INST(Pmaxsw          , "pmaxsw"          , Enc(ExtRm_P)           , O(000F00,EE,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7291, 177),
  INST(Pmaxub          , "pmaxub"          , Enc(ExtRm_P)           , O(000F00,DE,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7299, 177),
  INST(Pmaxud          , "pmaxud"          , Enc(ExtRm)             , O(660F38,3F,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7307, 4  ),
  INST(Pmaxuw          , "pmaxuw"          , Enc(ExtRm)             , O(660F38,3E,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7323, 4  ),
  INST(Pminsb          , "pminsb"          , Enc(ExtRm)             , O(660F38,38,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7331, 4  ),
  INST(Pminsd          , "pminsd"          , Enc(ExtRm)             , O(660F38,39,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7339, 4  ),
  INST(Pminsw          , "pminsw"          , Enc(ExtRm_P)           , O(000F00,EA,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7355, 177),
  INST(Pminub          , "pminub"          , Enc(ExtRm_P)           , O(000F00,DA,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7363, 177),
  INST(Pminud          , "pminud"          , Enc(ExtRm)             , O(660F38,3B,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7371, 4  ),
  INST(Pminuw          , "pminuw"          , Enc(ExtRm)             , O(660F38,3A,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7387, 4  ),
  INST(Pmovmskb        , "pmovmskb"        , Enc(ExtRm_P)           , O(000F00,D7,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 7465, 196),
  INST(Pmovsxbd        , "pmovsxbd"        , Enc(ExtRm)             , O(660F38,21,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 7562, 197),
  INST(Pmovsxbq        , "pmovsxbq"        , Enc(ExtRm)             , O(660F38,22,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 7572, 198),
  INST(Pmovsxbw        , "pmovsxbw"        , Enc(ExtRm)             , O(660F38,20,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 7582, 52 ),
  INST(Pmovsxdq        , "pmovsxdq"        , Enc(ExtRm)             , O(660F38,25,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 7592, 52 ),
  INST(Pmovsxwd        , "pmovsxwd"        , Enc(ExtRm)             , O(660F38,23,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 7602, 52 ),
  INST(Pmovsxwq        , "pmovsxwq"        , Enc(ExtRm)             , O(660F38,24,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 7612, 197),
  INST(Pmovzxbd        , "pmovzxbd"        , Enc(ExtRm)             , O(660F38,31,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 7699, 197),
  INST(Pmovzxbq        , "pmovzxbq"        , Enc(ExtRm)             , O(660F38,32,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 7709, 198),
  INST(Pmovzxbw        , "pmovzxbw"        , Enc(ExtRm)             , O(660F38,30,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 7719, 52 ),
  INST(Pmovzxdq        , "pmovzxdq"        , Enc(ExtRm)             , O(660F38,35,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 7729, 52 ),
  INST(Pmovzxwd        , "pmovzxwd"        , Enc(ExtRm)             , O(660F38,33,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 7739, 52 ),
  INST(Pmovzxwq        , "pmovzxwq"        , Enc(ExtRm)             , O(660F38,34,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 7749, 197),
  INST(Pmuldq          , "pmuldq"          , Enc(ExtRm)             , O(660F38,28,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7759, 4  ),
  INST(Pmulhrsw        , "pmulhrsw"        , Enc(ExtRm_P)           , O(000F38,0B,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7767, 177),
  INST(Pmulhrw         , "pmulhrw"         , Enc(Ext3dNow)          , O(000F0F,B7,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 1900, 180),
  INST(Pmulhuw         , "pmulhuw"         , Enc(ExtRm_P)           , O(000F00,E4,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7777, 177),
  INST(Pmulhw          , "pmulhw"          , Enc(ExtRm_P)           , O(000F00,E5,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7786, 177),
  INST(Pmulld          , "pmulld"          , Enc(ExtRm)             , O(660F38,40,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7794, 4  ),
  INST(Pmullw          , "pmullw"          , Enc(ExtRm_P)           , O(000F00,D5,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7810, 177),
  INST(Pmuludq         , "pmuludq"         , Enc(ExtRm_P)           , O(000F00,F4,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7833, 177),
  INST(Pop             , "pop"             , Enc(X86Pop)            , O(000000,8F,0,_,_,_,_,_  ), O(000000,58,_,_,_,_,_,_  ), F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 1908, 199),
  INST(Popa            , "popa"            , Enc(X86Op)             , O(000000,61,_,_,_,_,_,_  ), 0                         , F(Volatile)|F(Special)                , EF(________), 0 , 0 , 1912, 200),
  INST(Popcnt          , "popcnt"          , Enc(X86Rm)             , O(F30F00,B8,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWWWW__), 0 , 0 , 1917, 201),
  INST(Popf            , "popf"            , Enc(X86Op)             , O(000000,9D,_,_,_,_,_,_  ), 0                         , F(Volatile)|F(Special)                , EF(WWWWWWWW), 0 , 0 , 1924, 202),
  INST(Por             , "por"             , Enc(ExtRm_P)           , O(000F00,EB,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 7842, 177),
  INST(Prefetch        , "prefetch"        , Enc(X86Prefetch)       , O(000F00,18,_,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , 1929, 203),
  INST(Prefetch3dNow   , "prefetch3dnow"   , Enc(X86M_Only)         , O(000F00,0D,0,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , 1938, 29 ),
  INST(Prefetchw       , "prefetchw"       , Enc(X86M_Only)         , O(000F00,0D,1,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(UUUUUU__), 0 , 0 , 1952, 204),
  INST(Prefetchwt1     , "prefetchwt1"     , Enc(X86M_Only)         , O(000F00,0D,2,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(UUUUUU__), 0 , 0 , 1962, 204),
  INST(Psadbw          , "psadbw"          , Enc(ExtRm_P)           , O(000F00,F6,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 3731, 177),
  INST(Pshufb          , "pshufb"          , Enc(ExtRm_P)           , O(000F38,00,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8066, 205),
  INST(Pshufd          , "pshufd"          , Enc(ExtRmi)            , O(660F00,70,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8074, 206),
  INST(Pshufhw         , "pshufhw"         , Enc(ExtRmi)            , O(F30F00,70,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8082, 206),
  INST(Pshuflw         , "pshuflw"         , Enc(ExtRmi)            , O(F20F00,70,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8091, 206),
  INST(Pshufw          , "pshufw"          , Enc(ExtRmi_P)          , O(000F00,70,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 1974, 207),
  INST(Psignb          , "psignb"          , Enc(ExtRm_P)           , O(000F38,08,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8100, 177),
  INST(Psignd          , "psignd"          , Enc(ExtRm_P)           , O(000F38,0A,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8108, 177),
  INST(Psignw          , "psignw"          , Enc(ExtRm_P)           , O(000F38,09,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8116, 177),
  INST(Pslld           , "pslld"           , Enc(ExtRmRi_P)         , O(000F00,F2,_,_,_,_,_,_  ), O(000F00,72,6,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 8124, 208),
  INST(Pslldq          , "pslldq"          , Enc(ExtRmRi)           , 0                         , O(660F00,73,7,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 8131, 209),
  INST(Psllq           , "psllq"           , Enc(ExtRmRi_P)         , O(000F00,F3,_,_,_,_,_,_  ), O(000F00,73,6,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 8139, 210),
  INST(Psllw           , "psllw"           , Enc(ExtRmRi_P)         , O(000F00,F1,_,_,_,_,_,_  ), O(000F00,71,6,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 8170, 211),
  INST(Psrad           , "psrad"           , Enc(ExtRmRi_P)         , O(000F00,E2,_,_,_,_,_,_  ), O(000F00,72,4,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 8177, 212),
  INST(Psraw           , "psraw"           , Enc(ExtRmRi_P)         , O(000F00,E1,_,_,_,_,_,_  ), O(000F00,71,4,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 8215, 213),
  INST(Psrld           , "psrld"           , Enc(ExtRmRi_P)         , O(000F00,D2,_,_,_,_,_,_  ), O(000F00,72,2,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 8222, 214),
  INST(Psrldq          , "psrldq"          , Enc(ExtRmRi)           , 0                         , O(660F00,73,3,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 8229, 215),
  INST(Psrlq           , "psrlq"           , Enc(ExtRmRi_P)         , O(000F00,D3,_,_,_,_,_,_  ), O(000F00,73,2,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 8237, 216),
  INST(Psrlw           , "psrlw"           , Enc(ExtRmRi_P)         , O(000F00,D1,_,_,_,_,_,_  ), O(000F00,71,2,_,_,_,_,_  ), F(RW)                                 , EF(________), 0 , 0 , 8268, 217),
  INST(Psubb           , "psubb"           , Enc(ExtRm_P)           , O(000F00,F8,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8275, 177),
  INST(Psubd           , "psubd"           , Enc(ExtRm_P)           , O(000F00,FA,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8282, 177),
  INST(Psubq           , "psubq"           , Enc(ExtRm_P)           , O(000F00,FB,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8289, 177),
  INST(Psubsb          , "psubsb"          , Enc(ExtRm_P)           , O(000F00,E8,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8296, 177),
  INST(Psubsw          , "psubsw"          , Enc(ExtRm_P)           , O(000F00,E9,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8304, 177),
  INST(Psubusb         , "psubusb"         , Enc(ExtRm_P)           , O(000F00,D8,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8312, 177),
  INST(Psubusw         , "psubusw"         , Enc(ExtRm_P)           , O(000F00,D9,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8321, 177),
  INST(Psubw           , "psubw"           , Enc(ExtRm_P)           , O(000F00,F9,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8330, 177),
  INST(Pswapd          , "pswapd"          , Enc(Ext3dNow)          , O(000F0F,BB,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 1981, 190),
  INST(Ptest           , "ptest"           , Enc(ExtRm)             , O(660F38,17,_,_,_,_,_,_  ), 0                         , F(RO)                                 , EF(WWWWWW__), 0 , 0 , 8359, 218),
  INST(Punpckhbw       , "punpckhbw"       , Enc(ExtRm_P)           , O(000F00,68,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8442, 177),
  INST(Punpckhdq       , "punpckhdq"       , Enc(ExtRm_P)           , O(000F00,6A,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8453, 177),
  INST(Punpckhqdq      , "punpckhqdq"      , Enc(ExtRm)             , O(660F00,6D,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8464, 4  ),
  INST(Punpckhwd       , "punpckhwd"       , Enc(ExtRm_P)           , O(000F00,69,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8476, 177),
  INST(Punpcklbw       , "punpcklbw"       , Enc(ExtRm_P)           , O(000F00,60,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8487, 177),
  INST(Punpckldq       , "punpckldq"       , Enc(ExtRm_P)           , O(000F00,62,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8498, 177),
  INST(Punpcklqdq      , "punpcklqdq"      , Enc(ExtRm)             , O(660F00,6C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8509, 4  ),
  INST(Punpcklwd       , "punpcklwd"       , Enc(ExtRm_P)           , O(000F00,61,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8521, 177),
  INST(Push            , "push"            , Enc(X86Push)           , O(000000,FF,6,_,_,_,_,_  ), O(000000,50,_,_,_,_,_,_  ), F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 1988, 219),
  INST(Pusha           , "pusha"           , Enc(X86Op)             , O(000000,60,_,_,_,_,_,_  ), 0                         , F(Volatile)|F(Special)                , EF(________), 0 , 0 , 1993, 200),
  INST(Pushf           , "pushf"           , Enc(X86Op)             , O(000000,9C,_,_,_,_,_,_  ), 0                         , F(Volatile)|F(Special)                , EF(RRRRRRRR), 0 , 0 , 1999, 220),
  INST(Pxor            , "pxor"            , Enc(ExtRm_P)           , O(000F00,EF,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 8532, 177),
  INST(Rcl             , "rcl"             , Enc(X86Rot)            , O(000000,D0,2,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(W____X__), 0 , 0 , 2005, 221),
  INST(Rcpps           , "rcpps"           , Enc(ExtRm)             , O(000F00,53,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8660, 53 ),
  INST(Rcpss           , "rcpss"           , Enc(ExtRm)             , O(F30F00,53,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 4 , 8667, 222),
  INST(Rcr             , "rcr"             , Enc(X86Rot)            , O(000000,D0,3,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(W____X__), 0 , 0 , 2009, 221),
  INST(Rdfsbase        , "rdfsbase"        , Enc(X86M)              , O(F30F00,AE,0,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 2013, 223),
  INST(Rdgsbase        , "rdgsbase"        , Enc(X86M)              , O(F30F00,AE,1,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 2022, 223),
  INST(Rdrand          , "rdrand"          , Enc(X86M)              , O(000F00,C7,6,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWWWW__), 0 , 8 , 2031, 224),
  INST(Rdseed          , "rdseed"          , Enc(X86M)              , O(000F00,C7,7,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWWWW__), 0 , 8 , 2038, 224),
  INST(Rdtsc           , "rdtsc"           , Enc(X86Op)             , O(000F00,31,_,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 2045, 225),
  INST(Rdtscp          , "rdtscp"          , Enc(X86Op)             , O(000F01,F9,_,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 2051, 226),
  INST(RepLodsB        , "rep lods_b"      , Enc(X86Rep)            , O(000000,AC,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 2058, 227),
  INST(RepLodsD        , "rep lods_d"      , Enc(X86Rep)            , O(000000,AD,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 2069, 227),
  INST(RepLodsQ        , "rep lods_q"      , Enc(X86Rep)            , O(000000,AD,1,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 2080, 227),
  INST(RepLodsW        , "rep lods_w"      , Enc(X86Rep)            , O(660000,AD,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 2091, 227),
  INST(RepMovsB        , "rep movs_b"      , Enc(X86Rep)            , O(000000,A4,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 2102, 227),
  INST(RepMovsD        , "rep movs_d"      , Enc(X86Rep)            , O(000000,A5,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 2113, 227),
  INST(RepMovsQ        , "rep movs_q"      , Enc(X86Rep)            , O(000000,A5,1,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 2124, 227),
  INST(RepMovsW        , "rep movs_w"      , Enc(X86Rep)            , O(660000,A5,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 2135, 227),
  INST(RepStosB        , "rep stos_b"      , Enc(X86Rep)            , O(000000,AA,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 2146, 227),
  INST(RepStosD        , "rep stos_d"      , Enc(X86Rep)            , O(000000,AB,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 2157, 227),
  INST(RepStosQ        , "rep stos_q"      , Enc(X86Rep)            , O(000000,AB,1,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 2168, 227),
  INST(RepStosW        , "rep stos_w"      , Enc(X86Rep)            , O(660000,AB,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 2179, 227),
  INST(RepeCmpsB       , "repe cmps_b"     , Enc(X86Rep)            , O(000000,A6,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2190, 228),
  INST(RepeCmpsD       , "repe cmps_d"     , Enc(X86Rep)            , O(000000,A7,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2202, 228),
  INST(RepeCmpsQ       , "repe cmps_q"     , Enc(X86Rep)            , O(000000,A7,1,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2214, 228),
  INST(RepeCmpsW       , "repe cmps_w"     , Enc(X86Rep)            , O(660000,A7,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2226, 228),
  INST(RepeScasB       , "repe scas_b"     , Enc(X86Rep)            , O(000000,AE,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2238, 228),
  INST(RepeScasD       , "repe scas_d"     , Enc(X86Rep)            , O(000000,AF,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2250, 228),
  INST(RepeScasQ       , "repe scas_q"     , Enc(X86Rep)            , O(000000,AF,1,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2262, 228),
  INST(RepeScasW       , "repe scas_w"     , Enc(X86Rep)            , O(660000,AF,1,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2274, 228),
  INST(RepneCmpsB      , "repne cmps_b"    , Enc(X86Rep)            , O(000000,A6,0,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2286, 228),
  INST(RepneCmpsD      , "repne cmps_d"    , Enc(X86Rep)            , O(000000,A7,0,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2299, 228),
  INST(RepneCmpsQ      , "repne cmps_q"    , Enc(X86Rep)            , O(000000,A7,0,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2312, 228),
  INST(RepneCmpsW      , "repne cmps_w"    , Enc(X86Rep)            , O(660000,A7,0,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2325, 228),
  INST(RepneScasB      , "repne scas_b"    , Enc(X86Rep)            , O(000000,AE,0,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2338, 228),
  INST(RepneScasD      , "repne scas_d"    , Enc(X86Rep)            , O(000000,AF,0,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2351, 228),
  INST(RepneScasQ      , "repne scas_q"    , Enc(X86Rep)            , O(000000,AF,0,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2364, 228),
  INST(RepneScasW      , "repne scas_w"    , Enc(X86Rep)            , O(660000,AF,0,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2377, 228),
  INST(Ret             , "ret"             , Enc(X86Ret)            , O(000000,C2,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 2390, 229),
  INST(Rol             , "rol"             , Enc(X86Rot)            , O(000000,D0,0,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(W____W__), 0 , 0 , 2394, 230),
  INST(Ror             , "ror"             , Enc(X86Rot)            , O(000000,D0,1,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(W____W__), 0 , 0 , 2398, 230),
  INST(Rorx            , "rorx"            , Enc(VexRmi_Wx)         , V(F20F3A,F0,_,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 2402, 231),
  INST(Roundpd         , "roundpd"         , Enc(ExtRmi)            , O(660F3A,09,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8762, 206),
  INST(Roundps         , "roundps"         , Enc(ExtRmi)            , O(660F3A,08,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8771, 206),
  INST(Roundsd         , "roundsd"         , Enc(ExtRmi)            , O(660F3A,0B,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 8780, 232),
  INST(Roundss         , "roundss"         , Enc(ExtRmi)            , O(660F3A,0A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 4 , 8789, 233),
  INST(Rsqrtps         , "rsqrtps"         , Enc(ExtRm)             , O(000F00,52,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8886, 53 ),
  INST(Rsqrtss         , "rsqrtss"         , Enc(ExtRm)             , O(F30F00,52,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 4 , 8895, 222),
  INST(Sahf            , "sahf"            , Enc(X86Op)             , O(000000,9E,_,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(_WWWWW__), 0 , 0 , 2407, 234),
  INST(Sal             , "sal"             , Enc(X86Rot)            , O(000000,D0,4,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(WWWUWW__), 0 , 0 , 2412, 235),
  INST(Sar             , "sar"             , Enc(X86Rot)            , O(000000,D0,7,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(WWWUWW__), 0 , 0 , 2416, 235),
  INST(Sarx            , "sarx"            , Enc(VexRmv_Wx)         , V(F30F38,F7,_,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 2420, 236),
  INST(Sbb             , "sbb"             , Enc(X86Arith)          , O(000000,18,3,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWWWX__), 0 , 0 , 2425, 1  ),
  INST(ScasB           , "scas_b"          , Enc(X86Op)             , O(000000,AE,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2344, 41 ),
  INST(ScasD           , "scas_d"          , Enc(X86Op)             , O(000000,AF,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2357, 41 ),
  INST(ScasQ           , "scas_q"          , Enc(X86Op)             , O(000000,AF,_,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2370, 41 ),
  INST(ScasW           , "scas_w"          , Enc(X86Op)             , O(660000,AF,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(WWWWWWR_), 0 , 0 , 2383, 41 ),
  INST(Seta            , "seta"            , Enc(X86Set)            , O(000F00,97,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R__R__), 0 , 1 , 2429, 237),
  INST(Setae           , "setae"           , Enc(X86Set)            , O(000F00,93,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_____R__), 0 , 1 , 2434, 238),
  INST(Setb            , "setb"            , Enc(X86Set)            , O(000F00,92,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_____R__), 0 , 1 , 2440, 238),
  INST(Setbe           , "setbe"           , Enc(X86Set)            , O(000F00,96,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R__R__), 0 , 1 , 2445, 237),
  INST(Setc            , "setc"            , Enc(X86Set)            , O(000F00,92,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_____R__), 0 , 1 , 2451, 238),
  INST(Sete            , "sete"            , Enc(X86Set)            , O(000F00,94,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R_____), 0 , 1 , 2456, 239),
  INST(Setg            , "setg"            , Enc(X86Set)            , O(000F00,9F,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RRR_____), 0 , 1 , 2461, 240),
  INST(Setge           , "setge"           , Enc(X86Set)            , O(000F00,9D,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RR______), 0 , 1 , 2466, 241),
  INST(Setl            , "setl"            , Enc(X86Set)            , O(000F00,9C,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RR______), 0 , 1 , 2472, 241),
  INST(Setle           , "setle"           , Enc(X86Set)            , O(000F00,9E,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RRR_____), 0 , 1 , 2477, 240),
  INST(Setna           , "setna"           , Enc(X86Set)            , O(000F00,96,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R__R__), 0 , 1 , 2483, 237),
  INST(Setnae          , "setnae"          , Enc(X86Set)            , O(000F00,92,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_____R__), 0 , 1 , 2489, 238),
  INST(Setnb           , "setnb"           , Enc(X86Set)            , O(000F00,93,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_____R__), 0 , 1 , 2496, 238),
  INST(Setnbe          , "setnbe"          , Enc(X86Set)            , O(000F00,97,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R__R__), 0 , 1 , 2502, 237),
  INST(Setnc           , "setnc"           , Enc(X86Set)            , O(000F00,93,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_____R__), 0 , 1 , 2509, 238),
  INST(Setne           , "setne"           , Enc(X86Set)            , O(000F00,95,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R_____), 0 , 1 , 2515, 239),
  INST(Setng           , "setng"           , Enc(X86Set)            , O(000F00,9E,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RRR_____), 0 , 1 , 2521, 240),
  INST(Setnge          , "setnge"          , Enc(X86Set)            , O(000F00,9C,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RR______), 0 , 1 , 2527, 241),
  INST(Setnl           , "setnl"           , Enc(X86Set)            , O(000F00,9D,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RR______), 0 , 1 , 2534, 241),
  INST(Setnle          , "setnle"          , Enc(X86Set)            , O(000F00,9F,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(RRR_____), 0 , 1 , 2540, 240),
  INST(Setno           , "setno"           , Enc(X86Set)            , O(000F00,91,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(R_______), 0 , 1 , 2547, 242),
  INST(Setnp           , "setnp"           , Enc(X86Set)            , O(000F00,9B,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(____R___), 0 , 1 , 2553, 243),
  INST(Setns           , "setns"           , Enc(X86Set)            , O(000F00,99,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_R______), 0 , 1 , 2559, 244),
  INST(Setnz           , "setnz"           , Enc(X86Set)            , O(000F00,95,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R_____), 0 , 1 , 2565, 239),
  INST(Seto            , "seto"            , Enc(X86Set)            , O(000F00,90,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(R_______), 0 , 1 , 2571, 242),
  INST(Setp            , "setp"            , Enc(X86Set)            , O(000F00,9A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(____R___), 0 , 1 , 2576, 243),
  INST(Setpe           , "setpe"           , Enc(X86Set)            , O(000F00,9A,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(____R___), 0 , 1 , 2581, 243),
  INST(Setpo           , "setpo"           , Enc(X86Set)            , O(000F00,9B,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(____R___), 0 , 1 , 2587, 243),
  INST(Sets            , "sets"            , Enc(X86Set)            , O(000F00,98,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(_R______), 0 , 1 , 2593, 244),
  INST(Setz            , "setz"            , Enc(X86Set)            , O(000F00,94,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(__R_____), 0 , 1 , 2598, 239),
  INST(Sfence          , "sfence"          , Enc(X86Fence)          , O(000F00,AE,7,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(________), 0 , 0 , 2603, 129),
  INST(Sha1msg1        , "sha1msg1"        , Enc(ExtRm)             , O(000F38,C9,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2610, 4  ),
  INST(Sha1msg2        , "sha1msg2"        , Enc(ExtRm)             , O(000F38,CA,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2619, 4  ),
  INST(Sha1nexte       , "sha1nexte"       , Enc(ExtRm)             , O(000F38,C8,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2628, 4  ),
  INST(Sha1rnds4       , "sha1rnds4"       , Enc(ExtRmi)            , O(000F3A,CC,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2638, 13 ),
  INST(Sha256msg1      , "sha256msg1"      , Enc(ExtRm)             , O(000F38,CC,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2648, 4  ),
  INST(Sha256msg2      , "sha256msg2"      , Enc(ExtRm)             , O(000F38,CD,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 2659, 4  ),
  INST(Sha256rnds2     , "sha256rnds2"     , Enc(ExtRmXMM0)         , O(000F38,CB,_,_,_,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(________), 0 , 0 , 2670, 14 ),
  INST(Shl             , "shl"             , Enc(X86Rot)            , O(000000,D0,4,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(WWWUWW__), 0 , 0 , 2682, 235),
  INST(Shld            , "shld"            , Enc(X86ShldShrd)       , O(000F00,A4,_,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(UWWUWW__), 0 , 0 , 8046, 245),
  INST(Shlx            , "shlx"            , Enc(VexRmv_Wx)         , V(660F38,F7,_,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 2686, 236),
  INST(Shr             , "shr"             , Enc(X86Rot)            , O(000000,D0,5,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(WWWUWW__), 0 , 0 , 2691, 235),
  INST(Shrd            , "shrd"            , Enc(X86ShldShrd)       , O(000F00,AC,_,_,x,_,_,_  ), 0                         , F(RW)|F(Special)                      , EF(UWWUWW__), 0 , 0 , 2695, 245),
  INST(Shrx            , "shrx"            , Enc(VexRmv_Wx)         , V(F20F38,F7,_,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 0 , 2700, 236),
  INST(Shufpd          , "shufpd"          , Enc(ExtRmi)            , O(660F00,C6,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 9156, 13 ),
  INST(Shufps          , "shufps"          , Enc(ExtRmi)            , O(000F00,C6,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 9164, 13 ),
  INST(Sqrtpd          , "sqrtpd"          , Enc(ExtRm)             , O(660F00,51,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 9172, 53 ),
  INST(Sqrtps          , "sqrtps"          , Enc(ExtRm)             , O(000F00,51,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 16, 8887, 53 ),
  INST(Sqrtsd          , "sqrtsd"          , Enc(ExtRm)             , O(F20F00,51,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 8 , 9188, 246),
  INST(Sqrtss          , "sqrtss"          , Enc(ExtRm)             , O(F30F00,51,_,_,_,_,_,_  ), 0                         , F(WO)                                 , EF(________), 0 , 4 , 8896, 222),
  INST(Stac            , "stac"            , Enc(X86Op)             , O(000F01,CB,_,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(___W____), 0 , 0 , 2705, 26 ),
  INST(Stc             , "stc"             , Enc(X86Op)             , O(000000,F9,_,_,_,_,_,_  ), 0                         , 0                                     , EF(_____W__), 0 , 0 , 2710, 247),
  INST(Std             , "std"             , Enc(X86Op)             , O(000000,FD,_,_,_,_,_,_  ), 0                         , 0                                     , EF(______W_), 0 , 0 , 6147, 248),
  INST(Sti             , "sti"             , Enc(X86Op)             , O(000000,FB,_,_,_,_,_,_  ), 0                         , 0                                     , EF(_______W), 0 , 0 , 2714, 249),
  INST(Stmxcsr         , "stmxcsr"         , Enc(X86M_Only)         , O(000F00,AE,3,_,_,_,_,_  ), 0                         , F(Volatile)                           , EF(________), 0 , 0 , 9204, 250),
  INST(StosB           , "stos_b"          , Enc(X86Op)             , O(000000,AA,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 2150, 251),
  INST(StosD           , "stos_d"          , Enc(X86Op)             , O(000000,AB,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 2161, 251),
  INST(StosQ           , "stos_q"          , Enc(X86Op)             , O(000000,AB,_,_,1,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 2172, 251),
  INST(StosW           , "stos_w"          , Enc(X86Op)             , O(660000,AB,_,_,_,_,_,_  ), 0                         , F(RW)|F(Volatile)|F(Special)          , EF(______R_), 0 , 0 , 2183, 251),
  INST(Sub             , "sub"             , Enc(X86Arith)          , O(000000,28,5,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWWWW__), 0 , 0 , 732 , 3  ),
  INST(Subpd           , "subpd"           , Enc(ExtRm)             , O(660F00,5C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4287, 4  ),
  INST(Subps           , "subps"           , Enc(ExtRm)             , O(000F00,5C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4299, 4  ),
  INST(Subsd           , "subsd"           , Enc(ExtRm)             , O(F20F00,5C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4975, 5  ),
  INST(Subss           , "subss"           , Enc(ExtRm)             , O(F30F00,5C,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 4985, 6  ),
  INST(T1mskc          , "t1mskc"          , Enc(VexVm_Wx)          , V(XOP_M9,01,7,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , 2718, 12 ),
  INST(Test            , "test"            , Enc(X86Test)           , O(000000,84,_,_,x,_,_,_  ), O(000000,F6,_,_,x,_,_,_  ), F(RO)                                 , EF(WWWUWW__), 0 , 0 , 8360, 252),
  INST(Tzcnt           , "tzcnt"           , Enc(X86Rm)             , O(F30F00,BC,_,_,x,_,_,_  ), 0                         , F(WO)                                 , EF(UUWUUW__), 0 , 0 , 2725, 201),
  INST(Tzmsk           , "tzmsk"           , Enc(VexVm_Wx)          , V(XOP_M9,01,4,0,x,_,_,_  ), 0                         , F(WO)                                 , EF(WWWUUW__), 0 , 0 , 2731, 12 ),
  INST(Ucomisd         , "ucomisd"         , Enc(ExtRm)             , O(660F00,2E,_,_,_,_,_,_  ), 0                         , F(RO)                                 , EF(WWWWWW__), 0 , 0 , 9257, 47 ),
  INST(Ucomiss         , "ucomiss"         , Enc(ExtRm)             , O(000F00,2E,_,_,_,_,_,_  ), 0                         , F(RO)                                 , EF(WWWWWW__), 0 , 0 , 9266, 48 ),
  INST(Ud2             , "ud2"             , Enc(X86Op)             , O(000F00,0B,_,_,_,_,_,_  ), 0                         , 0                                     , EF(________), 0 , 0 , 2737, 253),
  INST(Unpckhpd        , "unpckhpd"        , Enc(ExtRm)             , O(660F00,15,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 9275, 4  ),
  INST(Unpckhps        , "unpckhps"        , Enc(ExtRm)             , O(000F00,15,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 9285, 4  ),
  INST(Unpcklpd        , "unpcklpd"        , Enc(ExtRm)             , O(660F00,14,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 9295, 4  ),
  INST(Unpcklps        , "unpcklps"        , Enc(ExtRm)             , O(000F00,14,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 9305, 4  ),
  INST(Vaddpd          , "vaddpd"          , Enc(VexRvm_Lx)         , V(660F00,58,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 2741, 254),
  INST(Vaddps          , "vaddps"          , Enc(VexRvm_Lx)         , V(000F00,58,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 2748, 255),
  INST(Vaddsd          , "vaddsd"          , Enc(VexRvm)            , V(F20F00,58,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 2755, 256),
  INST(Vaddss          , "vaddss"          , Enc(VexRvm)            , V(F30F00,58,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 2762, 257),
  INST(Vaddsubpd       , "vaddsubpd"       , Enc(VexRvm_Lx)         , V(660F00,D0,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2769, 258),
  INST(Vaddsubps       , "vaddsubps"       , Enc(VexRvm_Lx)         , V(F20F00,D0,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2779, 258),
  INST(Vaesdec         , "vaesdec"         , Enc(VexRvm)            , V(660F38,DE,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2789, 259),
  INST(Vaesdeclast     , "vaesdeclast"     , Enc(VexRvm)            , V(660F38,DF,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2797, 259),
  INST(Vaesenc         , "vaesenc"         , Enc(VexRvm)            , V(660F38,DC,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2809, 259),
  INST(Vaesenclast     , "vaesenclast"     , Enc(VexRvm)            , V(660F38,DD,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2817, 259),
  INST(Vaesimc         , "vaesimc"         , Enc(VexRm)             , V(660F38,DB,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2829, 260),
  INST(Vaeskeygenassist, "vaeskeygenassist", Enc(VexRmi)            , V(660F3A,DF,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2837, 261),
  INST(Valignd         , "valignd"         , Enc(VexRvmi_Lx)        , V(660F3A,03,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 2854, 262),
  INST(Valignq         , "valignq"         , Enc(VexRvmi_Lx)        , V(660F3A,03,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 2862, 263),
  INST(Vandnpd         , "vandnpd"         , Enc(VexRvm_Lx)         , V(660F00,55,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 2870, 264),
  INST(Vandnps         , "vandnps"         , Enc(VexRvm_Lx)         , V(000F00,55,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 2878, 265),
  INST(Vandpd          , "vandpd"          , Enc(VexRvm_Lx)         , V(660F00,54,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 2886, 264),
  INST(Vandps          , "vandps"          , Enc(VexRvm_Lx)         , V(000F00,54,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 2893, 265),
  INST(Vblendmb        , "vblendmb"        , Enc(VexRvm_Lx)         , V(660F38,66,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 2900, 266),
  INST(Vblendmd        , "vblendmd"        , Enc(VexRvm_Lx)         , V(660F38,64,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 2909, 267),
  INST(Vblendmpd       , "vblendmpd"       , Enc(VexRvm_Lx)         , V(660F38,65,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 2918, 268),
  INST(Vblendmps       , "vblendmps"       , Enc(VexRvm_Lx)         , V(660F38,65,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 2928, 267),
  INST(Vblendmq        , "vblendmq"        , Enc(VexRvm_Lx)         , V(660F38,64,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 2938, 268),
  INST(Vblendmw        , "vblendmw"        , Enc(VexRvm_Lx)         , V(660F38,66,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 2947, 266),
  INST(Vblendpd        , "vblendpd"        , Enc(VexRvmi_Lx)        , V(660F3A,0D,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2956, 269),
  INST(Vblendps        , "vblendps"        , Enc(VexRvmi_Lx)        , V(660F3A,0C,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2965, 269),
  INST(Vblendvpd       , "vblendvpd"       , Enc(VexRvmr_Lx)        , V(660F3A,4B,_,x,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2974, 270),
  INST(Vblendvps       , "vblendvps"       , Enc(VexRvmr_Lx)        , V(660F3A,4A,_,x,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2984, 270),
  INST(Vbroadcastf128  , "vbroadcastf128"  , Enc(VexRm)             , V(660F38,1A,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 2994, 271),
  INST(Vbroadcastf32x2 , "vbroadcastf32x2" , Enc(VexRm_Lx)          , V(660F38,19,_,x,_,0,3,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 3009, 272),
  INST(Vbroadcastf32x4 , "vbroadcastf32x4" , Enc(VexRm_Lx)          , V(660F38,1A,_,x,_,0,4,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 3025, 273),
  INST(Vbroadcastf32x8 , "vbroadcastf32x8" , Enc(VexRm)             , V(660F38,1B,_,2,_,0,5,T8 ), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 3041, 274),
  INST(Vbroadcastf64x2 , "vbroadcastf64x2" , Enc(VexRm_Lx)          , V(660F38,1A,_,x,_,1,4,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 3057, 275),
  INST(Vbroadcastf64x4 , "vbroadcastf64x4" , Enc(VexRm)             , V(660F38,1B,_,2,_,1,5,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 3073, 276),
  INST(Vbroadcasti128  , "vbroadcasti128"  , Enc(VexRm)             , V(660F38,5A,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 3089, 271),
  INST(Vbroadcasti32x2 , "vbroadcasti32x2" , Enc(VexRm_Lx)          , V(660F38,59,_,x,_,0,3,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 3104, 277),
  INST(Vbroadcasti32x4 , "vbroadcasti32x4" , Enc(VexRm_Lx)          , V(660F38,5A,_,x,_,0,4,T4 ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 3120, 278),
  INST(Vbroadcasti32x8 , "vbroadcasti32x8" , Enc(VexRm)             , V(660F38,5B,_,2,_,0,5,T8 ), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 3136, 279),
  INST(Vbroadcasti64x2 , "vbroadcasti64x2" , Enc(VexRm_Lx)          , V(660F38,5A,_,x,_,1,4,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 3152, 272),
  INST(Vbroadcasti64x4 , "vbroadcasti64x4" , Enc(VexRm)             , V(660F38,5B,_,2,_,1,5,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 3168, 280),
  INST(Vbroadcastsd    , "vbroadcastsd"    , Enc(VexRm_Lx)          , V(660F38,19,_,x,0,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 3184, 281),
  INST(Vbroadcastss    , "vbroadcastss"    , Enc(VexRm_Lx)          , V(660F38,18,_,x,0,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 3197, 282),
  INST(Vcmppd          , "vcmppd"          , Enc(VexRvmi_Lx)        , V(660F00,C2,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , 3210, 283),
  INST(Vcmpps          , "vcmpps"          , Enc(VexRvmi_Lx)        , V(000F00,C2,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,4) , EF(________), 0 , 0 , 3217, 284),
  INST(Vcmpsd          , "vcmpsd"          , Enc(VexRvmi)           , V(F20F00,C2,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , 3224, 285),
  INST(Vcmpss          , "vcmpss"          , Enc(VexRvmi)           , V(F30F00,C2,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , 3231, 286),
  INST(Vcomisd         , "vcomisd"         , Enc(VexRm)             , V(660F00,2F,_,I,I,1,3,T1S), 0                         , F(RO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , EF(WWWWWW__), 0 , 0 , 3238, 287),
  INST(Vcomiss         , "vcomiss"         , Enc(VexRm)             , V(000F00,2F,_,I,I,0,2,T1S), 0                         , F(RO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , EF(WWWWWW__), 0 , 0 , 3246, 288),
  INST(Vcompresspd     , "vcompresspd"     , Enc(VexMr_Lx)          , V(660F38,8A,_,x,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 3254, 289),
  INST(Vcompressps     , "vcompressps"     , Enc(VexMr_Lx)          , V(660F38,8A,_,x,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 3266, 289),
  INST(Vcvtdq2pd       , "vcvtdq2pd"       , Enc(VexRm_Lx)          , V(F30F00,E6,_,x,I,0,3,HV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 3278, 290),
  INST(Vcvtdq2ps       , "vcvtdq2ps"       , Enc(VexRm_Lx)          , V(000F00,5B,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 3288, 291),
  INST(Vcvtpd2dq       , "vcvtpd2dq"       , Enc(VexRm_Lx)          , V(F20F00,E6,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 3298, 292),
  INST(Vcvtpd2ps       , "vcvtpd2ps"       , Enc(VexRm_Lx)          , V(660F00,5A,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 3308, 293),
  INST(Vcvtpd2qq       , "vcvtpd2qq"       , Enc(VexRm_Lx)          , V(660F00,7B,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 3318, 294),
  INST(Vcvtpd2udq      , "vcvtpd2udq"      , Enc(VexRm_Lx)          , V(000F00,79,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 3328, 295),
  INST(Vcvtpd2uqq      , "vcvtpd2uqq"      , Enc(VexRm_Lx)          , V(660F00,79,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 3339, 294),
  INST(Vcvtph2ps       , "vcvtph2ps"       , Enc(VexRm_Lx)          , V(660F38,13,_,x,0,0,3,HVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , EF(________), 0 , 0 , 3350, 296),
  INST(Vcvtps2dq       , "vcvtps2dq"       , Enc(VexRm_Lx)          , V(660F00,5B,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 3360, 291),
  INST(Vcvtps2pd       , "vcvtps2pd"       , Enc(VexRm_Lx)          , V(000F00,5A,_,x,I,0,4,HV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 3370, 297),
  INST(Vcvtps2ph       , "vcvtps2ph"       , Enc(VexMri_Lx)         , V(660F3A,1D,_,x,0,0,3,HVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , EF(________), 0 , 0 , 3380, 298),
  INST(Vcvtps2qq       , "vcvtps2qq"       , Enc(VexRm_Lx)          , V(660F00,7B,_,x,_,0,3,HV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 3390, 299),
  INST(Vcvtps2udq      , "vcvtps2udq"      , Enc(VexRm_Lx)          , V(000F00,79,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 3400, 300),
  INST(Vcvtps2uqq      , "vcvtps2uqq"      , Enc(VexRm_Lx)          , V(660F00,79,_,x,_,0,3,HV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 3411, 299),
  INST(Vcvtqq2pd       , "vcvtqq2pd"       , Enc(VexRm_Lx)          , V(F30F00,E6,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 3422, 294),
  INST(Vcvtqq2ps       , "vcvtqq2ps"       , Enc(VexRm_Lx)          , V(000F00,5B,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 3432, 301),
  INST(Vcvtsd2si       , "vcvtsd2si"       , Enc(VexRm)             , V(F20F00,2D,_,I,x,x,3,T1F), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,ER ,0) , EF(________), 0 , 0 , 3442, 302),
  INST(Vcvtsd2ss       , "vcvtsd2ss"       , Enc(VexRvm)            , V(F20F00,5A,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 3452, 256),
  INST(Vcvtsd2usi      , "vcvtsd2usi"      , Enc(VexRm)             , V(F20F00,79,_,I,_,x,3,T1F), 0                         , F(WO)          |A512(F_  ,0,0 ,ER ,0) , EF(________), 0 , 0 , 3462, 303),
  INST(Vcvtsi2sd       , "vcvtsi2sd"       , Enc(VexRvm)            , V(F20F00,2A,_,I,x,x,2,T1W), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,ER ,0) , EF(________), 0 , 0 , 3473, 304),
  INST(Vcvtsi2ss       , "vcvtsi2ss"       , Enc(VexRvm)            , V(F30F00,2A,_,I,x,x,2,T1W), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,ER ,0) , EF(________), 0 , 0 , 3483, 304),
  INST(Vcvtss2sd       , "vcvtss2sd"       , Enc(VexRvm)            , V(F30F00,5A,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , 3493, 305),
  INST(Vcvtss2si       , "vcvtss2si"       , Enc(VexRm)             , V(F20F00,2D,_,I,x,x,2,T1F), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,ER ,0) , EF(________), 0 , 0 , 3503, 306),
  INST(Vcvtss2usi      , "vcvtss2usi"      , Enc(VexRm)             , V(F30F00,79,_,I,_,x,2,T1F), 0                         , F(WO)          |A512(F_  ,0,0 ,ER ,0) , EF(________), 0 , 0 , 3513, 307),
  INST(Vcvttpd2dq      , "vcvttpd2dq"      , Enc(VexRm_Lx)          , V(660F00,E6,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , 3524, 308),
  INST(Vcvttpd2qq      , "vcvttpd2qq"      , Enc(VexRm_Lx)          , V(660F00,7A,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , 3535, 309),
  INST(Vcvttpd2udq     , "vcvttpd2udq"     , Enc(VexRm_Lx)          , V(000F00,78,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , 3546, 310),
  INST(Vcvttpd2uqq     , "vcvttpd2uqq"     , Enc(VexRm_Lx)          , V(660F00,78,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,SAE,8) , EF(________), 0 , 0 , 3558, 311),
  INST(Vcvttps2dq      , "vcvttps2dq"      , Enc(VexRm_Lx)          , V(F30F00,5B,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,4) , EF(________), 0 , 0 , 3570, 312),
  INST(Vcvttps2qq      , "vcvttps2qq"      , Enc(VexRm_Lx)          , V(660F00,7A,_,x,_,0,3,HV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,SAE,4) , EF(________), 0 , 0 , 3581, 313),
  INST(Vcvttps2udq     , "vcvttps2udq"     , Enc(VexRm_Lx)          , V(000F00,78,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,4) , EF(________), 0 , 0 , 3592, 314),
  INST(Vcvttps2uqq     , "vcvttps2uqq"     , Enc(VexRm_Lx)          , V(660F00,78,_,x,_,0,3,HV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,SAE,4) , EF(________), 0 , 0 , 3604, 313),
  INST(Vcvttsd2si      , "vcvttsd2si"      , Enc(VexRm)             , V(F20F00,2C,_,I,x,x,3,T1F), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , EF(________), 0 , 0 , 3616, 315),
  INST(Vcvttsd2usi     , "vcvttsd2usi"     , Enc(VexRm)             , V(F20F00,78,_,I,_,x,3,T1F), 0                         , F(WO)          |A512(F_  ,0,0 ,SAE,0) , EF(________), 0 , 0 , 3627, 316),
  INST(Vcvttss2si      , "vcvttss2si"      , Enc(VexRm)             , V(F30F00,2C,_,I,x,x,2,T1F), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , EF(________), 0 , 0 , 3639, 317),
  INST(Vcvttss2usi     , "vcvttss2usi"     , Enc(VexRm)             , V(F30F00,78,_,I,_,x,2,T1F), 0                         , F(WO)          |A512(F_  ,0,0 ,SAE,0) , EF(________), 0 , 0 , 3650, 318),
  INST(Vcvtudq2pd      , "vcvtudq2pd"      , Enc(VexRm_Lx)          , V(F30F00,7A,_,x,_,0,3,HV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 3662, 319),
  INST(Vcvtudq2ps      , "vcvtudq2ps"      , Enc(VexRm_Lx)          , V(F20F00,7A,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 3673, 300),
  INST(Vcvtuqq2pd      , "vcvtuqq2pd"      , Enc(VexRm_Lx)          , V(F30F00,7A,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 3684, 294),
  INST(Vcvtuqq2ps      , "vcvtuqq2ps"      , Enc(VexRm_Lx)          , V(F20F00,7A,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 3695, 301),
  INST(Vcvtusi2sd      , "vcvtusi2sd"      , Enc(VexRvm)            , V(F20F00,7B,_,I,_,x,2,T1W), 0                         , F(WO)          |A512(F_  ,0,0 ,ER ,0) , EF(________), 0 , 0 , 3706, 320),
  INST(Vcvtusi2ss      , "vcvtusi2ss"      , Enc(VexRvm)            , V(F30F00,7B,_,I,_,x,2,T1W), 0                         , F(WO)          |A512(F_  ,0,0 ,ER ,0) , EF(________), 0 , 0 , 3717, 320),
  INST(Vdbpsadbw       , "vdbpsadbw"       , Enc(VexRvmi_Lx)        , V(660F3A,42,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 3728, 321),
  INST(Vdivpd          , "vdivpd"          , Enc(VexRvm_Lx)         , V(660F00,5E,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 3738, 254),
  INST(Vdivps          , "vdivps"          , Enc(VexRvm_Lx)         , V(000F00,5E,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 3745, 255),
  INST(Vdivsd          , "vdivsd"          , Enc(VexRvm)            , V(F20F00,5E,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 3752, 256),
  INST(Vdivss          , "vdivss"          , Enc(VexRvm)            , V(F30F00,5E,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 3759, 257),
  INST(Vdppd           , "vdppd"           , Enc(VexRvmi_Lx)        , V(660F3A,41,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 3766, 269),
  INST(Vdpps           , "vdpps"           , Enc(VexRvmi_Lx)        , V(660F3A,40,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 3772, 269),
  INST(Vexp2pd         , "vexp2pd"         , Enc(VexRm)             , V(660F38,C8,_,2,_,1,4,FV ), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,8) , EF(________), 0 , 0 , 3778, 322),
  INST(Vexp2ps         , "vexp2ps"         , Enc(VexRm)             , V(660F38,C8,_,2,_,0,4,FV ), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,4) , EF(________), 0 , 0 , 3786, 323),
  INST(Vexpandpd       , "vexpandpd"       , Enc(VexRm_Lx)          , V(660F38,88,_,x,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 3794, 324),
  INST(Vexpandps       , "vexpandps"       , Enc(VexRm_Lx)          , V(660F38,88,_,x,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 3804, 324),
  INST(Vextractf128    , "vextractf128"    , Enc(VexMri)            , V(660F3A,19,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 3814, 325),
  INST(Vextractf32x4   , "vextractf32x4"   , Enc(VexMri_Lx)         , V(660F3A,19,_,x,_,0,4,T4 ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 3827, 326),
  INST(Vextractf32x8   , "vextractf32x8"   , Enc(VexMri)            , V(660F3A,1B,_,2,_,0,5,T8 ), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 3841, 327),
  INST(Vextractf64x2   , "vextractf64x2"   , Enc(VexMri_Lx)         , V(660F3A,19,_,x,_,1,4,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 3855, 328),
  INST(Vextractf64x4   , "vextractf64x4"   , Enc(VexMri)            , V(660F3A,1B,_,2,_,1,5,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 3869, 329),
  INST(Vextracti128    , "vextracti128"    , Enc(VexMri)            , V(660F3A,39,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 3883, 325),
  INST(Vextracti32x4   , "vextracti32x4"   , Enc(VexMri_Lx)         , V(660F3A,39,_,x,_,0,4,T4 ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 3896, 326),
  INST(Vextracti32x8   , "vextracti32x8"   , Enc(VexMri)            , V(660F3A,3B,_,2,_,0,5,T8 ), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 3910, 327),
  INST(Vextracti64x2   , "vextracti64x2"   , Enc(VexMri_Lx)         , V(660F3A,39,_,x,_,1,4,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 3924, 328),
  INST(Vextracti64x4   , "vextracti64x4"   , Enc(VexMri)            , V(660F3A,3B,_,2,_,1,5,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 3938, 329),
  INST(Vextractps      , "vextractps"      , Enc(VexMri)            , V(660F3A,17,_,0,I,I,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , 3952, 330),
  INST(Vfixupimmpd     , "vfixupimmpd"     , Enc(VexRvmi_Lx)        , V(660F3A,54,_,x,_,1,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , 3963, 331),
  INST(Vfixupimmps     , "vfixupimmps"     , Enc(VexRvmi_Lx)        , V(660F3A,54,_,x,_,0,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,SAE,4) , EF(________), 0 , 0 , 3975, 332),
  INST(Vfixupimmsd     , "vfixupimmsd"     , Enc(VexRvmi)           , V(660F3A,55,_,I,_,1,3,T1S), 0                         , F(RW)          |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , 3987, 333),
  INST(Vfixupimmss     , "vfixupimmss"     , Enc(VexRvmi)           , V(660F3A,55,_,I,_,0,2,T1S), 0                         , F(RW)          |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , 3999, 334),
  INST(Vfmadd132pd     , "vfmadd132pd"     , Enc(VexRvm_Lx)         , V(660F38,98,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 4011, 335),
  INST(Vfmadd132ps     , "vfmadd132ps"     , Enc(VexRvm_Lx)         , V(660F38,98,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 4023, 336),
  INST(Vfmadd132sd     , "vfmadd132sd"     , Enc(VexRvm)            , V(660F38,99,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4035, 337),
  INST(Vfmadd132ss     , "vfmadd132ss"     , Enc(VexRvm)            , V(660F38,99,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4047, 338),
  INST(Vfmadd213pd     , "vfmadd213pd"     , Enc(VexRvm_Lx)         , V(660F38,A8,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 4059, 335),
  INST(Vfmadd213ps     , "vfmadd213ps"     , Enc(VexRvm_Lx)         , V(660F38,A8,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 4071, 336),
  INST(Vfmadd213sd     , "vfmadd213sd"     , Enc(VexRvm)            , V(660F38,A9,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4083, 337),
  INST(Vfmadd213ss     , "vfmadd213ss"     , Enc(VexRvm)            , V(660F38,A9,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4095, 338),
  INST(Vfmadd231pd     , "vfmadd231pd"     , Enc(VexRvm_Lx)         , V(660F38,B8,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 4107, 335),
  INST(Vfmadd231ps     , "vfmadd231ps"     , Enc(VexRvm_Lx)         , V(660F38,B8,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 4119, 336),
  INST(Vfmadd231sd     , "vfmadd231sd"     , Enc(VexRvm)            , V(660F38,B9,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4131, 337),
  INST(Vfmadd231ss     , "vfmadd231ss"     , Enc(VexRvm)            , V(660F38,B9,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4143, 338),
  INST(Vfmaddpd        , "vfmaddpd"        , Enc(Fma4_Lx)           , V(660F3A,69,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4155, 339),
  INST(Vfmaddps        , "vfmaddps"        , Enc(Fma4_Lx)           , V(660F3A,68,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4164, 339),
  INST(Vfmaddsd        , "vfmaddsd"        , Enc(Fma4)              , V(660F3A,6B,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4173, 340),
  INST(Vfmaddss        , "vfmaddss"        , Enc(Fma4)              , V(660F3A,6A,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4182, 341),
  INST(Vfmaddsub132pd  , "vfmaddsub132pd"  , Enc(VexRvm_Lx)         , V(660F38,96,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 4191, 335),
  INST(Vfmaddsub132ps  , "vfmaddsub132ps"  , Enc(VexRvm_Lx)         , V(660F38,96,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 4206, 336),
  INST(Vfmaddsub213pd  , "vfmaddsub213pd"  , Enc(VexRvm_Lx)         , V(660F38,A6,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 4221, 335),
  INST(Vfmaddsub213ps  , "vfmaddsub213ps"  , Enc(VexRvm_Lx)         , V(660F38,A6,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 4236, 336),
  INST(Vfmaddsub231pd  , "vfmaddsub231pd"  , Enc(VexRvm_Lx)         , V(660F38,B6,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 4251, 335),
  INST(Vfmaddsub231ps  , "vfmaddsub231ps"  , Enc(VexRvm_Lx)         , V(660F38,B6,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 4266, 336),
  INST(Vfmaddsubpd     , "vfmaddsubpd"     , Enc(Fma4_Lx)           , V(660F3A,5D,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4281, 339),
  INST(Vfmaddsubps     , "vfmaddsubps"     , Enc(Fma4_Lx)           , V(660F3A,5C,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4293, 339),
  INST(Vfmsub132pd     , "vfmsub132pd"     , Enc(VexRvm_Lx)         , V(660F38,9A,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 4305, 335),
  INST(Vfmsub132ps     , "vfmsub132ps"     , Enc(VexRvm_Lx)         , V(660F38,9A,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 4317, 336),
  INST(Vfmsub132sd     , "vfmsub132sd"     , Enc(VexRvm)            , V(660F38,9B,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4329, 337),
  INST(Vfmsub132ss     , "vfmsub132ss"     , Enc(VexRvm)            , V(660F38,9B,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4341, 338),
  INST(Vfmsub213pd     , "vfmsub213pd"     , Enc(VexRvm_Lx)         , V(660F38,AA,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 4353, 335),
  INST(Vfmsub213ps     , "vfmsub213ps"     , Enc(VexRvm_Lx)         , V(660F38,AA,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 4365, 336),
  INST(Vfmsub213sd     , "vfmsub213sd"     , Enc(VexRvm)            , V(660F38,AB,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4377, 337),
  INST(Vfmsub213ss     , "vfmsub213ss"     , Enc(VexRvm)            , V(660F38,AB,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4389, 338),
  INST(Vfmsub231pd     , "vfmsub231pd"     , Enc(VexRvm_Lx)         , V(660F38,BA,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 4401, 335),
  INST(Vfmsub231ps     , "vfmsub231ps"     , Enc(VexRvm_Lx)         , V(660F38,BA,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 4413, 336),
  INST(Vfmsub231sd     , "vfmsub231sd"     , Enc(VexRvm)            , V(660F38,BB,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4425, 337),
  INST(Vfmsub231ss     , "vfmsub231ss"     , Enc(VexRvm)            , V(660F38,BB,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4437, 338),
  INST(Vfmsubadd132pd  , "vfmsubadd132pd"  , Enc(VexRvm_Lx)         , V(660F38,97,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 4449, 335),
  INST(Vfmsubadd132ps  , "vfmsubadd132ps"  , Enc(VexRvm_Lx)         , V(660F38,97,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 4464, 336),
  INST(Vfmsubadd213pd  , "vfmsubadd213pd"  , Enc(VexRvm_Lx)         , V(660F38,A7,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 4479, 335),
  INST(Vfmsubadd213ps  , "vfmsubadd213ps"  , Enc(VexRvm_Lx)         , V(660F38,A7,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 4494, 336),
  INST(Vfmsubadd231pd  , "vfmsubadd231pd"  , Enc(VexRvm_Lx)         , V(660F38,B7,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 4509, 335),
  INST(Vfmsubadd231ps  , "vfmsubadd231ps"  , Enc(VexRvm_Lx)         , V(660F38,B7,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 4524, 336),
  INST(Vfmsubaddpd     , "vfmsubaddpd"     , Enc(Fma4_Lx)           , V(660F3A,5F,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4539, 339),
  INST(Vfmsubaddps     , "vfmsubaddps"     , Enc(Fma4_Lx)           , V(660F3A,5E,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4551, 339),
  INST(Vfmsubpd        , "vfmsubpd"        , Enc(Fma4_Lx)           , V(660F3A,6D,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4563, 339),
  INST(Vfmsubps        , "vfmsubps"        , Enc(Fma4_Lx)           , V(660F3A,6C,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4572, 339),
  INST(Vfmsubsd        , "vfmsubsd"        , Enc(Fma4)              , V(660F3A,6F,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4581, 340),
  INST(Vfmsubss        , "vfmsubss"        , Enc(Fma4)              , V(660F3A,6E,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4590, 341),
  INST(Vfnmadd132pd    , "vfnmadd132pd"    , Enc(VexRvm_Lx)         , V(660F38,9C,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 4599, 335),
  INST(Vfnmadd132ps    , "vfnmadd132ps"    , Enc(VexRvm_Lx)         , V(660F38,9C,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 4612, 336),
  INST(Vfnmadd132sd    , "vfnmadd132sd"    , Enc(VexRvm)            , V(660F38,9D,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4625, 337),
  INST(Vfnmadd132ss    , "vfnmadd132ss"    , Enc(VexRvm)            , V(660F38,9D,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4638, 338),
  INST(Vfnmadd213pd    , "vfnmadd213pd"    , Enc(VexRvm_Lx)         , V(660F38,AC,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 4651, 335),
  INST(Vfnmadd213ps    , "vfnmadd213ps"    , Enc(VexRvm_Lx)         , V(660F38,AC,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 4664, 336),
  INST(Vfnmadd213sd    , "vfnmadd213sd"    , Enc(VexRvm)            , V(660F38,AD,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4677, 337),
  INST(Vfnmadd213ss    , "vfnmadd213ss"    , Enc(VexRvm)            , V(660F38,AD,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4690, 338),
  INST(Vfnmadd231pd    , "vfnmadd231pd"    , Enc(VexRvm_Lx)         , V(660F38,BC,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 4703, 335),
  INST(Vfnmadd231ps    , "vfnmadd231ps"    , Enc(VexRvm_Lx)         , V(660F38,BC,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 4716, 336),
  INST(Vfnmadd231sd    , "vfnmadd231sd"    , Enc(VexRvm)            , V(660F38,BC,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4729, 337),
  INST(Vfnmadd231ss    , "vfnmadd231ss"    , Enc(VexRvm)            , V(660F38,BC,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4742, 338),
  INST(Vfnmaddpd       , "vfnmaddpd"       , Enc(Fma4_Lx)           , V(660F3A,79,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4755, 339),
  INST(Vfnmaddps       , "vfnmaddps"       , Enc(Fma4_Lx)           , V(660F3A,78,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4765, 339),
  INST(Vfnmaddsd       , "vfnmaddsd"       , Enc(Fma4)              , V(660F3A,7B,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4775, 340),
  INST(Vfnmaddss       , "vfnmaddss"       , Enc(Fma4)              , V(660F3A,7A,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4785, 341),
  INST(Vfnmsub132pd    , "vfnmsub132pd"    , Enc(VexRvm_Lx)         , V(660F38,9E,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 4795, 335),
  INST(Vfnmsub132ps    , "vfnmsub132ps"    , Enc(VexRvm_Lx)         , V(660F38,9E,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 4808, 336),
  INST(Vfnmsub132sd    , "vfnmsub132sd"    , Enc(VexRvm)            , V(660F38,9F,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4821, 337),
  INST(Vfnmsub132ss    , "vfnmsub132ss"    , Enc(VexRvm)            , V(660F38,9F,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4834, 338),
  INST(Vfnmsub213pd    , "vfnmsub213pd"    , Enc(VexRvm_Lx)         , V(660F38,AE,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 4847, 335),
  INST(Vfnmsub213ps    , "vfnmsub213ps"    , Enc(VexRvm_Lx)         , V(660F38,AE,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 4860, 336),
  INST(Vfnmsub213sd    , "vfnmsub213sd"    , Enc(VexRvm)            , V(660F38,AF,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4873, 337),
  INST(Vfnmsub213ss    , "vfnmsub213ss"    , Enc(VexRvm)            , V(660F38,AF,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4886, 338),
  INST(Vfnmsub231pd    , "vfnmsub231pd"    , Enc(VexRvm_Lx)         , V(660F38,BE,_,x,1,1,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 4899, 335),
  INST(Vfnmsub231ps    , "vfnmsub231ps"    , Enc(VexRvm_Lx)         , V(660F38,BE,_,x,0,0,4,FV ), 0                         , F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 4912, 336),
  INST(Vfnmsub231sd    , "vfnmsub231sd"    , Enc(VexRvm)            , V(660F38,BF,_,I,1,1,3,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4925, 337),
  INST(Vfnmsub231ss    , "vfnmsub231ss"    , Enc(VexRvm)            , V(660F38,BF,_,I,0,0,2,T1S), 0                         , F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 4938, 338),
  INST(Vfnmsubpd       , "vfnmsubpd"       , Enc(Fma4_Lx)           , V(660F3A,7D,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4951, 339),
  INST(Vfnmsubps       , "vfnmsubps"       , Enc(Fma4_Lx)           , V(660F3A,7C,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4961, 339),
  INST(Vfnmsubsd       , "vfnmsubsd"       , Enc(Fma4)              , V(660F3A,7F,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4971, 340),
  INST(Vfnmsubss       , "vfnmsubss"       , Enc(Fma4)              , V(660F3A,7E,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 4981, 341),
  INST(Vfpclasspd      , "vfpclasspd"      , Enc(VexRmi_Lx)         , V(660F3A,66,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,K_,0  ,8) , EF(________), 0 , 0 , 4991, 342),
  INST(Vfpclassps      , "vfpclassps"      , Enc(VexRmi_Lx)         , V(660F3A,66,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,K_,0  ,4) , EF(________), 0 , 0 , 5002, 343),
  INST(Vfpclasssd      , "vfpclasssd"      , Enc(VexRmi_Lx)         , V(660F3A,67,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(DQ  ,0,K_,0  ,0) , EF(________), 0 , 0 , 5013, 344),
  INST(Vfpclassss      , "vfpclassss"      , Enc(VexRmi_Lx)         , V(660F3A,67,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(DQ  ,0,K_,0  ,0) , EF(________), 0 , 0 , 5024, 345),
  INST(Vfrczpd         , "vfrczpd"         , Enc(VexRm_Lx)          , V(XOP_M9,81,_,x,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 5035, 346),
  INST(Vfrczps         , "vfrczps"         , Enc(VexRm_Lx)          , V(XOP_M9,80,_,x,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 5043, 346),
  INST(Vfrczsd         , "vfrczsd"         , Enc(VexRm)             , V(XOP_M9,83,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 5051, 347),
  INST(Vfrczss         , "vfrczss"         , Enc(VexRm)             , V(XOP_M9,82,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 5059, 348),
  INST(Vgatherdpd      , "vgatherdpd"      , Enc(VexRmvRm_VM)       , V(660F38,92,_,x,1,_,_,_  ), V(660F38,92,_,x,_,1,3,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , 5067, 349),
  INST(Vgatherdps      , "vgatherdps"      , Enc(VexRmvRm_VM)       , V(660F38,92,_,x,0,_,_,_  ), V(660F38,92,_,x,_,0,2,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , 5078, 350),
  INST(Vgatherpf0dpd   , "vgatherpf0dpd"   , Enc(VexM_VM)           , V(660F38,C6,1,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , 5089, 351),
  INST(Vgatherpf0dps   , "vgatherpf0dps"   , Enc(VexM_VM)           , V(660F38,C6,1,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , 5103, 352),
  INST(Vgatherpf0qpd   , "vgatherpf0qpd"   , Enc(VexM_VM)           , V(660F38,C7,1,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , 5117, 353),
  INST(Vgatherpf0qps   , "vgatherpf0qps"   , Enc(VexM_VM)           , V(660F38,C7,1,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , 5131, 353),
  INST(Vgatherpf1dpd   , "vgatherpf1dpd"   , Enc(VexM_VM)           , V(660F38,C6,2,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , 5145, 351),
  INST(Vgatherpf1dps   , "vgatherpf1dps"   , Enc(VexM_VM)           , V(660F38,C6,2,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , 5159, 352),
  INST(Vgatherpf1qpd   , "vgatherpf1qpd"   , Enc(VexM_VM)           , V(660F38,C7,2,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , 5173, 353),
  INST(Vgatherpf1qps   , "vgatherpf1qps"   , Enc(VexM_VM)           , V(660F38,C7,2,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , 5187, 353),
  INST(Vgatherqpd      , "vgatherqpd"      , Enc(VexRmvRm_VM)       , V(660F38,93,_,x,1,_,_,_  ), V(660F38,93,_,x,_,1,3,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , 5201, 354),
  INST(Vgatherqps      , "vgatherqps"      , Enc(VexRmvRm_VM)       , V(660F38,93,_,x,0,_,_,_  ), V(660F38,93,_,x,_,0,2,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , 5212, 355),
  INST(Vgetexppd       , "vgetexppd"       , Enc(VexRm_Lx)          , V(660F38,42,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , 5223, 309),
  INST(Vgetexpps       , "vgetexpps"       , Enc(VexRm_Lx)          , V(660F38,42,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,4) , EF(________), 0 , 0 , 5233, 314),
  INST(Vgetexpsd       , "vgetexpsd"       , Enc(VexRm)             , V(660F38,43,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , 5243, 356),
  INST(Vgetexpss       , "vgetexpss"       , Enc(VexRm)             , V(660F38,43,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , 5253, 357),
  INST(Vgetmantpd      , "vgetmantpd"      , Enc(VexRmi_Lx)         , V(660F3A,26,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , 5263, 358),
  INST(Vgetmantps      , "vgetmantps"      , Enc(VexRmi_Lx)         , V(660F3A,26,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,4) , EF(________), 0 , 0 , 5274, 359),
  INST(Vgetmantsd      , "vgetmantsd"      , Enc(VexRmi)            , V(660F3A,27,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , 5285, 360),
  INST(Vgetmantss      , "vgetmantss"      , Enc(VexRmi)            , V(660F3A,27,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , 5296, 361),
  INST(Vhaddpd         , "vhaddpd"         , Enc(VexRvm_Lx)         , V(660F00,7C,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 5307, 258),
  INST(Vhaddps         , "vhaddps"         , Enc(VexRvm_Lx)         , V(F20F00,7C,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 5315, 258),
  INST(Vhsubpd         , "vhsubpd"         , Enc(VexRvm_Lx)         , V(660F00,7D,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 5323, 258),
  INST(Vhsubps         , "vhsubps"         , Enc(VexRvm_Lx)         , V(F20F00,7D,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 5331, 258),
  INST(Vinsertf128     , "vinsertf128"     , Enc(VexRvmi)           , V(660F3A,18,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 5339, 362),
  INST(Vinsertf32x4    , "vinsertf32x4"    , Enc(VexRvmi_Lx)        , V(660F3A,18,_,x,_,0,4,T4 ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5351, 363),
  INST(Vinsertf32x8    , "vinsertf32x8"    , Enc(VexRvmi)           , V(660F3A,1A,_,2,_,0,5,T8 ), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 5364, 364),
  INST(Vinsertf64x2    , "vinsertf64x2"    , Enc(VexRvmi_Lx)        , V(660F3A,18,_,x,_,1,4,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5377, 365),
  INST(Vinsertf64x4    , "vinsertf64x4"    , Enc(VexRvmi)           , V(660F3A,1A,_,2,_,1,5,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 5390, 366),
  INST(Vinserti128     , "vinserti128"     , Enc(VexRvmi)           , V(660F3A,38,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 5403, 362),
  INST(Vinserti32x4    , "vinserti32x4"    , Enc(VexRvmi_Lx)        , V(660F3A,38,_,x,_,0,4,T4 ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5415, 363),
  INST(Vinserti32x8    , "vinserti32x8"    , Enc(VexRvmi)           , V(660F3A,3A,_,2,_,0,5,T8 ), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 5428, 364),
  INST(Vinserti64x2    , "vinserti64x2"    , Enc(VexRvmi_Lx)        , V(660F3A,38,_,x,_,1,4,T2 ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5441, 365),
  INST(Vinserti64x4    , "vinserti64x4"    , Enc(VexRvmi)           , V(660F3A,3A,_,2,_,1,5,T4 ), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 5454, 366),
  INST(Vinsertps       , "vinsertps"       , Enc(VexRvmi)           , V(660F3A,21,_,0,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , 5467, 367),
  INST(Vlddqu          , "vlddqu"          , Enc(VexRm_Lx)          , V(F20F00,F0,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 5477, 368),
  INST(Vldmxcsr        , "vldmxcsr"        , Enc(VexM)              , V(000F00,AE,2,0,I,_,_,_  ), 0                         , F(RO)|F(Vex)|F(Volatile)              , EF(________), 0 , 0 , 5484, 369),
  INST(Vmaskmovdqu     , "vmaskmovdqu"     , Enc(VexRmZDI)          , V(660F00,F7,_,0,I,_,_,_  ), 0                         , F(RO)|F(Vex)|F(Special)               , EF(________), 0 , 0 , 5493, 370),
  INST(Vmaskmovpd      , "vmaskmovpd"      , Enc(VexRvmMvr_Lx)      , V(660F38,2D,_,x,0,_,_,_  ), V(660F38,2F,_,x,0,_,_,_  ), F(RW)|F(Vex)                          , EF(________), 0 , 0 , 5505, 371),
  INST(Vmaskmovps      , "vmaskmovps"      , Enc(VexRvmMvr_Lx)      , V(660F38,2C,_,x,0,_,_,_  ), V(660F38,2E,_,x,0,_,_,_  ), F(RW)|F(Vex)                          , EF(________), 0 , 0 , 5516, 372),
  INST(Vmaxpd          , "vmaxpd"          , Enc(VexRvm_Lx)         , V(660F00,5F,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , 5527, 373),
  INST(Vmaxps          , "vmaxps"          , Enc(VexRvm_Lx)         , V(000F00,5F,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,4) , EF(________), 0 , 0 , 5534, 374),
  INST(Vmaxsd          , "vmaxsd"          , Enc(VexRvm)            , V(F20F00,5F,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , EF(________), 0 , 0 , 5541, 375),
  INST(Vmaxss          , "vmaxss"          , Enc(VexRvm)            , V(F30F00,5F,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , EF(________), 0 , 0 , 5548, 376),
  INST(Vminpd          , "vminpd"          , Enc(VexRvm_Lx)         , V(660F00,5D,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , 5555, 373),
  INST(Vminps          , "vminps"          , Enc(VexRvm_Lx)         , V(000F00,5D,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,4) , EF(________), 0 , 0 , 5562, 374),
  INST(Vminsd          , "vminsd"          , Enc(VexRvm)            , V(F20F00,5D,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , EF(________), 0 , 0 , 5569, 375),
  INST(Vminss          , "vminss"          , Enc(VexRvm)            , V(F30F00,5D,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , EF(________), 0 , 0 , 5576, 376),
  INST(Vmovapd         , "vmovapd"         , Enc(VexRmMr_Lx)        , V(660F00,28,_,x,I,1,4,FVM), V(660F00,29,_,x,I,1,4,FVM), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5583, 377),
  INST(Vmovaps         , "vmovaps"         , Enc(VexRmMr_Lx)        , V(000F00,28,_,x,I,0,4,FVM), V(000F00,29,_,x,I,0,4,FVM), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5591, 378),
  INST(Vmovd           , "vmovd"           , Enc(VexMovDQ)          , V(660F00,6E,_,0,0,0,2,T1S), V(660F00,7E,_,0,0,0,2,T1S), F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , 5599, 379),
  INST(Vmovddup        , "vmovddup"        , Enc(VexRm_Lx)          , V(F20F00,12,_,x,I,1,3,DUP), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5605, 380),
  INST(Vmovdqa         , "vmovdqa"         , Enc(VexRmMr_Lx)        , V(660F00,6F,_,x,I,_,_,_  ), V(660F00,7F,_,x,I,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 5614, 381),
  INST(Vmovdqa32       , "vmovdqa32"       , Enc(VexRmMr_Lx)        , V(660F00,6F,_,x,_,0,4,FVM), V(660F00,7F,_,x,_,0,4,FVM), F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5622, 382),
  INST(Vmovdqa64       , "vmovdqa64"       , Enc(VexRmMr_Lx)        , V(660F00,6F,_,x,_,1,4,FVM), V(660F00,7F,_,x,_,1,4,FVM), F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5632, 383),
  INST(Vmovdqu         , "vmovdqu"         , Enc(VexRmMr_Lx)        , V(F30F00,6F,_,x,I,_,_,_  ), V(F30F00,7F,_,x,I,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 5642, 384),
  INST(Vmovdqu16       , "vmovdqu16"       , Enc(VexRmMr_Lx)        , V(F20F00,6F,_,x,_,1,4,FVM), V(F20F00,7F,_,x,_,1,4,FVM), F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5650, 385),
  INST(Vmovdqu32       , "vmovdqu32"       , Enc(VexRmMr_Lx)        , V(F30F00,6F,_,x,_,0,4,FVM), V(F30F00,7F,_,x,_,0,4,FVM), F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5660, 386),
  INST(Vmovdqu64       , "vmovdqu64"       , Enc(VexRmMr_Lx)        , V(F30F00,6F,_,x,_,1,4,FVM), V(F30F00,7F,_,x,_,1,4,FVM), F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5670, 387),
  INST(Vmovdqu8        , "vmovdqu8"        , Enc(VexRmMr_Lx)        , V(F20F00,6F,_,x,_,0,4,FVM), V(F20F00,7F,_,x,_,0,4,FVM), F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5680, 388),
  INST(Vmovhlps        , "vmovhlps"        , Enc(VexRvm)            , V(000F00,12,_,0,I,0,_,_  ), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , 5689, 389),
  INST(Vmovhpd         , "vmovhpd"         , Enc(VexRvmMr)          , V(660F00,16,_,0,I,1,3,T1S), V(660F00,17,_,0,I,1,3,T1S), F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , 5698, 390),
  INST(Vmovhps         , "vmovhps"         , Enc(VexRvmMr)          , V(000F00,16,_,0,I,0,3,T2 ), V(000F00,17,_,0,I,0,3,T2 ), F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , 5706, 391),
  INST(Vmovlhps        , "vmovlhps"        , Enc(VexRvm)            , V(000F00,16,_,0,I,0,_,_  ), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , 5714, 389),
  INST(Vmovlpd         , "vmovlpd"         , Enc(VexRvmMr)          , V(660F00,12,_,0,I,1,3,T1S), V(660F00,13,_,0,I,1,3,T1S), F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , 5723, 392),
  INST(Vmovlps         , "vmovlps"         , Enc(VexRvmMr)          , V(000F00,12,_,0,I,0,3,T2 ), V(000F00,13,_,0,I,0,3,T2 ), F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , 5731, 393),
  INST(Vmovmskpd       , "vmovmskpd"       , Enc(VexRm_Lx)          , V(660F00,50,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 5739, 394),
  INST(Vmovmskps       , "vmovmskps"       , Enc(VexRm_Lx)          , V(000F00,50,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 5749, 394),
  INST(Vmovntdq        , "vmovntdq"        , Enc(VexMr_Lx)          , V(660F00,E7,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,0) , EF(________), 0 , 0 , 5759, 395),
  INST(Vmovntdqa       , "vmovntdqa"       , Enc(VexRm_Lx)          , V(660F38,2A,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,0) , EF(________), 0 , 0 , 5768, 396),
  INST(Vmovntpd        , "vmovntpd"        , Enc(VexMr_Lx)          , V(660F00,2B,_,x,I,1,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,0) , EF(________), 0 , 0 , 5778, 395),
  INST(Vmovntps        , "vmovntps"        , Enc(VexMr_Lx)          , V(000F00,2B,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,0) , EF(________), 0 , 0 , 5787, 395),
  INST(Vmovq           , "vmovq"           , Enc(VexMovDQ)          , V(660F00,6E,_,0,I,1,3,T1S), V(660F00,7E,_,0,I,1,3,T1S), F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , EF(________), 0 , 0 , 5796, 397),
  INST(Vmovsd          , "vmovsd"          , Enc(VexMovSsSd)        , V(F20F00,10,_,I,I,1,3,T1S), V(F20F00,11,_,I,I,1,3,T1S), F(WO)|F(Vex)   |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 5802, 398),
  INST(Vmovshdup       , "vmovshdup"       , Enc(VexRm_Lx)          , V(F30F00,16,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5809, 399),
  INST(Vmovsldup       , "vmovsldup"       , Enc(VexRm_Lx)          , V(F30F00,12,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5819, 399),
  INST(Vmovss          , "vmovss"          , Enc(VexMovSsSd)        , V(F30F00,10,_,I,I,0,2,T1S), V(F30F00,11,_,I,I,0,2,T1S), F(WO)|F(Vex)   |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 5829, 400),
  INST(Vmovupd         , "vmovupd"         , Enc(VexRmMr_Lx)        , V(660F00,10,_,x,I,1,4,FVM), V(660F00,11,_,x,I,1,4,FVM), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5836, 401),
  INST(Vmovups         , "vmovups"         , Enc(VexRmMr_Lx)        , V(000F00,10,_,x,I,0,4,FVM), V(000F00,11,_,x,I,0,4,FVM), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5844, 402),
  INST(Vmpsadbw        , "vmpsadbw"        , Enc(VexRvmi_Lx)        , V(660F3A,42,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 5852, 269),
  INST(Vmulpd          , "vmulpd"          , Enc(VexRvm_Lx)         , V(660F00,59,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 5861, 254),
  INST(Vmulps          , "vmulps"          , Enc(VexRvm_Lx)         , V(000F00,59,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 5868, 255),
  INST(Vmulsd          , "vmulsd"          , Enc(VexRvm_Lx)         , V(F20F00,59,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 5875, 403),
  INST(Vmulss          , "vmulss"          , Enc(VexRvm_Lx)         , V(F30F00,59,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 5882, 404),
  INST(Vorpd           , "vorpd"           , Enc(VexRvm_Lx)         , V(660F00,56,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 5889, 264),
  INST(Vorps           , "vorps"           , Enc(VexRvm_Lx)         , V(000F00,56,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 5895, 405),
  INST(Vpabsb          , "vpabsb"          , Enc(VexRm_Lx)          , V(660F38,1C,_,x,I,_,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5901, 406),
  INST(Vpabsd          , "vpabsd"          , Enc(VexRm_Lx)          , V(660F38,1E,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5908, 399),
  INST(Vpabsq          , "vpabsq"          , Enc(VexRm_Lx)          , V(660F38,1F,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5915, 324),
  INST(Vpabsw          , "vpabsw"          , Enc(VexRm_Lx)          , V(660F38,1D,_,x,I,_,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5922, 406),
  INST(Vpackssdw       , "vpackssdw"       , Enc(VexRvm_Lx)         , V(660F00,6B,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 5929, 407),
  INST(Vpacksswb       , "vpacksswb"       , Enc(VexRvm_Lx)         , V(660F00,63,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5939, 408),
  INST(Vpackusdw       , "vpackusdw"       , Enc(VexRvm_Lx)         , V(660F38,2B,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 5949, 407),
  INST(Vpackuswb       , "vpackuswb"       , Enc(VexRvm_Lx)         , V(660F00,67,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5959, 408),
  INST(Vpaddb          , "vpaddb"          , Enc(VexRvm_Lx)         , V(660F00,FC,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5969, 408),
  INST(Vpaddd          , "vpaddd"          , Enc(VexRvm_Lx)         , V(660F00,FE,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 5976, 405),
  INST(Vpaddq          , "vpaddq"          , Enc(VexRvm_Lx)         , V(660F00,D4,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 5983, 409),
  INST(Vpaddsb         , "vpaddsb"         , Enc(VexRvm_Lx)         , V(660F00,EC,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5990, 408),
  INST(Vpaddsw         , "vpaddsw"         , Enc(VexRvm_Lx)         , V(660F00,ED,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 5998, 408),
  INST(Vpaddusb        , "vpaddusb"        , Enc(VexRvm_Lx)         , V(660F00,DC,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 6006, 408),
  INST(Vpaddusw        , "vpaddusw"        , Enc(VexRvm_Lx)         , V(660F00,DD,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 6015, 408),
  INST(Vpaddw          , "vpaddw"          , Enc(VexRvm_Lx)         , V(660F00,FD,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 6024, 408),
  INST(Vpalignr        , "vpalignr"        , Enc(VexRvmi_Lx)        , V(660F3A,0F,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 6031, 410),
  INST(Vpand           , "vpand"           , Enc(VexRvm_Lx)         , V(660F00,DB,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6040, 258),
  INST(Vpandd          , "vpandd"          , Enc(VexRvm_Lx)         , V(660F00,DB,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 6046, 267),
  INST(Vpandn          , "vpandn"          , Enc(VexRvm_Lx)         , V(660F00,DF,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6053, 258),
  INST(Vpandnd         , "vpandnd"         , Enc(VexRvm_Lx)         , V(660F00,DF,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 6060, 267),
  INST(Vpandnq         , "vpandnq"         , Enc(VexRvm_Lx)         , V(660F00,DF,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 6068, 268),
  INST(Vpandq          , "vpandq"          , Enc(VexRvm_Lx)         , V(660F00,DB,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 6076, 268),
  INST(Vpavgb          , "vpavgb"          , Enc(VexRvm_Lx)         , V(660F00,E0,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 6083, 408),
  INST(Vpavgw          , "vpavgw"          , Enc(VexRvm_Lx)         , V(660F00,E3,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 6090, 408),
  INST(Vpblendd        , "vpblendd"        , Enc(VexRvmi_Lx)        , V(660F3A,02,_,x,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6097, 269),
  INST(Vpblendvb       , "vpblendvb"       , Enc(VexRvmr)           , V(660F3A,4C,_,x,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6106, 411),
  INST(Vpblendw        , "vpblendw"        , Enc(VexRvmi_Lx)        , V(660F3A,0E,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6116, 269),
  INST(Vpbroadcastb    , "vpbroadcastb"    , Enc(VexRm_Lx)          , V(660F38,78,_,x,0,0,0,T1S), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 6125, 412),
  INST(Vpbroadcastd    , "vpbroadcastd"    , Enc(VexRm_Lx)          , V(660F38,58,_,x,0,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 6138, 413),
  INST(Vpbroadcastmb2d , "vpbroadcastmb2d" , Enc(VexRm_Lx)          , V(F30F38,3A,_,x,_,0,_,_  ), 0                         , F(WO)          |A512(CDI ,1,0 ,0  ,0) , EF(________), 0 , 0 , 6151, 414),
  INST(Vpbroadcastmb2q , "vpbroadcastmb2q" , Enc(VexRm_Lx)          , V(F30F38,2A,_,x,_,1,_,_  ), 0                         , F(WO)          |A512(CDI ,1,0 ,0  ,0) , EF(________), 0 , 0 , 6167, 414),
  INST(Vpbroadcastq    , "vpbroadcastq"    , Enc(VexRm_Lx)          , V(660F38,59,_,x,0,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 6183, 415),
  INST(Vpbroadcastw    , "vpbroadcastw"    , Enc(VexRm_Lx)          , V(660F38,79,_,x,0,0,1,T1S), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 6196, 416),
  INST(Vpclmulqdq      , "vpclmulqdq"      , Enc(VexRvmi)           , V(660F3A,44,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6209, 417),
  INST(Vpcmov          , "vpcmov"          , Enc(VexRvrmRvmr_Lx)    , V(XOP_M8,A2,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6220, 418),
  INST(Vpcmpb          , "vpcmpb"          , Enc(VexRvm_Lx)         , V(660F3A,3F,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , 6227, 419),
  INST(Vpcmpd          , "vpcmpd"          , Enc(VexRvm_Lx)         , V(660F3A,1F,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,4) , EF(________), 0 , 0 , 6234, 420),
  INST(Vpcmpeqb        , "vpcmpeqb"        , Enc(VexRvm_Lx)         , V(660F00,74,_,x,I,I,4,FV ), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , 6241, 421),
  INST(Vpcmpeqd        , "vpcmpeqd"        , Enc(VexRvm_Lx)         , V(660F00,76,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,K_,0  ,4) , EF(________), 0 , 0 , 6250, 422),
  INST(Vpcmpeqq        , "vpcmpeqq"        , Enc(VexRvm_Lx)         , V(660F38,29,_,x,I,1,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,K_,0  ,8) , EF(________), 0 , 0 , 6259, 423),
  INST(Vpcmpeqw        , "vpcmpeqw"        , Enc(VexRvm_Lx)         , V(660F00,75,_,x,I,I,4,FV ), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , 6268, 421),
  INST(Vpcmpestri      , "vpcmpestri"      , Enc(VexRmi)            , V(660F3A,61,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)|F(Special)               , EF(________), 0 , 0 , 6277, 424),
  INST(Vpcmpestrm      , "vpcmpestrm"      , Enc(VexRmi)            , V(660F3A,60,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)|F(Special)               , EF(________), 0 , 0 , 6288, 425),
  INST(Vpcmpgtb        , "vpcmpgtb"        , Enc(VexRvm_Lx)         , V(660F00,64,_,x,I,I,4,FV ), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , 6299, 421),
  INST(Vpcmpgtd        , "vpcmpgtd"        , Enc(VexRvm_Lx)         , V(660F00,66,_,x,I,0,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,K_,0  ,4) , EF(________), 0 , 0 , 6308, 422),
  INST(Vpcmpgtq        , "vpcmpgtq"        , Enc(VexRvm_Lx)         , V(660F38,37,_,x,I,1,4,FVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,K_,0  ,8) , EF(________), 0 , 0 , 6317, 423),
  INST(Vpcmpgtw        , "vpcmpgtw"        , Enc(VexRvm_Lx)         , V(660F00,65,_,x,I,I,4,FV ), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , 6326, 421),
  INST(Vpcmpistri      , "vpcmpistri"      , Enc(VexRmi)            , V(660F3A,63,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)|F(Special)               , EF(________), 0 , 0 , 6335, 426),
  INST(Vpcmpistrm      , "vpcmpistrm"      , Enc(VexRmi)            , V(660F3A,62,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)|F(Special)               , EF(________), 0 , 0 , 6346, 427),
  INST(Vpcmpq          , "vpcmpq"          , Enc(VexRvm_Lx)         , V(660F3A,1F,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,8) , EF(________), 0 , 0 , 6357, 428),
  INST(Vpcmpub         , "vpcmpub"         , Enc(VexRvm_Lx)         , V(660F3A,3E,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , 6364, 419),
  INST(Vpcmpud         , "vpcmpud"         , Enc(VexRvm_Lx)         , V(660F3A,1E,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,4) , EF(________), 0 , 0 , 6372, 420),
  INST(Vpcmpuq         , "vpcmpuq"         , Enc(VexRvm_Lx)         , V(660F3A,1E,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,8) , EF(________), 0 , 0 , 6380, 428),
  INST(Vpcmpuw         , "vpcmpuw"         , Enc(VexRvm_Lx)         , V(660F3A,3E,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,8) , EF(________), 0 , 0 , 6388, 429),
  INST(Vpcmpw          , "vpcmpw"          , Enc(VexRvm_Lx)         , V(660F3A,3F,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,8) , EF(________), 0 , 0 , 6396, 429),
  INST(Vpcomb          , "vpcomb"          , Enc(VexRvmi)           , V(XOP_M8,CC,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6403, 417),
  INST(Vpcomd          , "vpcomd"          , Enc(VexRvmi)           , V(XOP_M8,CE,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6410, 417),
  INST(Vpcompressd     , "vpcompressd"     , Enc(VexMr_Lx)          , V(660F38,8B,_,x,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 6417, 289),
  INST(Vpcompressq     , "vpcompressq"     , Enc(VexMr_Lx)          , V(660F38,8B,_,x,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 6429, 289),
  INST(Vpcomq          , "vpcomq"          , Enc(VexRvmi)           , V(XOP_M8,CF,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6441, 417),
  INST(Vpcomub         , "vpcomub"         , Enc(VexRvmi)           , V(XOP_M8,EC,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6448, 417),
  INST(Vpcomud         , "vpcomud"         , Enc(VexRvmi)           , V(XOP_M8,EE,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6456, 417),
  INST(Vpcomuq         , "vpcomuq"         , Enc(VexRvmi)           , V(XOP_M8,EF,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6464, 417),
  INST(Vpcomuw         , "vpcomuw"         , Enc(VexRvmi)           , V(XOP_M8,ED,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6472, 417),
  INST(Vpcomw          , "vpcomw"          , Enc(VexRvmi)           , V(XOP_M8,CD,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6480, 417),
  INST(Vpconflictd     , "vpconflictd"     , Enc(VexRm_Lx)          , V(660F38,C4,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(CDI ,1,KZ,0  ,4) , EF(________), 0 , 0 , 6487, 430),
  INST(Vpconflictq     , "vpconflictq"     , Enc(VexRm_Lx)          , V(660F38,C4,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(CDI ,1,KZ,0  ,4) , EF(________), 0 , 0 , 6499, 430),
  INST(Vperm2f128      , "vperm2f128"      , Enc(VexRvmi)           , V(660F3A,06,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6511, 431),
  INST(Vperm2i128      , "vperm2i128"      , Enc(VexRvmi)           , V(660F3A,46,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6522, 431),
  INST(Vpermb          , "vpermb"          , Enc(VexRvm_Lx)         , V(660F38,8D,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(VBMI,1,KZ,0  ,0) , EF(________), 0 , 0 , 6533, 432),
  INST(Vpermd          , "vpermd"          , Enc(VexRvm_Lx)         , V(660F38,36,_,x,0,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 6540, 433),
  INST(Vpermi2b        , "vpermi2b"        , Enc(VexRvm_Lx)         , V(660F38,75,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(VBMI,1,KZ,0  ,0) , EF(________), 0 , 0 , 6547, 432),
  INST(Vpermi2d        , "vpermi2d"        , Enc(VexRvm_Lx)         , V(660F38,76,_,x,_,0,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 6556, 434),
  INST(Vpermi2pd       , "vpermi2pd"       , Enc(VexRvm_Lx)         , V(660F38,77,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 6565, 268),
  INST(Vpermi2ps       , "vpermi2ps"       , Enc(VexRvm_Lx)         , V(660F38,77,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 6575, 267),
  INST(Vpermi2q        , "vpermi2q"        , Enc(VexRvm_Lx)         , V(660F38,76,_,x,_,1,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 6585, 435),
  INST(Vpermi2w        , "vpermi2w"        , Enc(VexRvm_Lx)         , V(660F38,75,_,x,_,1,4,FVM), 0                         , F(RW)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 6594, 436),
  INST(Vpermil2pd      , "vpermil2pd"      , Enc(VexRvrmiRvmri_Lx)  , V(660F3A,49,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6603, 437),
  INST(Vpermil2ps      , "vpermil2ps"      , Enc(VexRvrmiRvmri_Lx)  , V(660F3A,48,_,x,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6614, 437),
  INST(Vpermilpd       , "vpermilpd"       , Enc(VexRvmRmi_Lx)      , V(660F38,0D,_,x,0,1,4,FV ), V(660F3A,05,_,x,0,1,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 6625, 438),
  INST(Vpermilps       , "vpermilps"       , Enc(VexRvmRmi_Lx)      , V(660F38,0C,_,x,0,0,4,FV ), V(660F3A,04,_,x,0,0,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 6635, 439),
  INST(Vpermpd         , "vpermpd"         , Enc(VexRmi)            , V(660F3A,01,_,1,1,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6645, 440),
  INST(Vpermps         , "vpermps"         , Enc(VexRvm)            , V(660F38,16,_,1,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6653, 441),
  INST(Vpermq          , "vpermq"          , Enc(VexRvmRmi_Lx)      , V(660F38,36,_,x,_,1,4,FV ), V(660F3A,00,_,x,1,1,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 6661, 442),
  INST(Vpermt2b        , "vpermt2b"        , Enc(VexRvm_Lx)         , V(660F38,7D,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(VBMI,1,KZ,0  ,0) , EF(________), 0 , 0 , 6668, 432),
  INST(Vpermt2d        , "vpermt2d"        , Enc(VexRvm_Lx)         , V(660F38,7E,_,x,_,0,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 6677, 434),
  INST(Vpermt2pd       , "vpermt2pd"       , Enc(VexRvm_Lx)         , V(660F38,7F,_,x,_,1,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 6686, 435),
  INST(Vpermt2ps       , "vpermt2ps"       , Enc(VexRvm_Lx)         , V(660F38,7F,_,x,_,0,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 6696, 434),
  INST(Vpermt2q        , "vpermt2q"        , Enc(VexRvm_Lx)         , V(660F38,7E,_,x,_,1,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 6706, 435),
  INST(Vpermt2w        , "vpermt2w"        , Enc(VexRvm_Lx)         , V(660F38,7D,_,x,_,1,4,FVM), 0                         , F(RW)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 6715, 436),
  INST(Vpermw          , "vpermw"          , Enc(VexRvm_Lx)         , V(660F38,8D,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 6724, 266),
  INST(Vpexpandd       , "vpexpandd"       , Enc(VexRm_Lx)          , V(660F38,89,_,x,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 6731, 324),
  INST(Vpexpandq       , "vpexpandq"       , Enc(VexRm_Lx)          , V(660F38,89,_,x,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 6741, 324),
  INST(Vpextrb         , "vpextrb"         , Enc(VexMri)            , V(660F3A,14,_,0,0,I,0,T1S), 0                         , F(WO)|F(Vex)   |A512(BW  ,0,0 ,0  ,0) , EF(________), 0 , 0 , 6751, 443),
  INST(Vpextrd         , "vpextrd"         , Enc(VexMri)            , V(660F3A,16,_,0,0,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(DQ  ,0,0 ,0  ,0) , EF(________), 0 , 0 , 6759, 444),
  INST(Vpextrq         , "vpextrq"         , Enc(VexMri)            , V(660F3A,16,_,0,1,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(DQ  ,0,0 ,0  ,0) , EF(________), 0 , 0 , 6767, 445),
  INST(Vpextrw         , "vpextrw"         , Enc(VexMri)            , V(660F3A,15,_,0,0,I,1,T1S), 0                         , F(WO)|F(Vex)   |A512(BW  ,0,0 ,0  ,0) , EF(________), 0 , 0 , 6775, 446),
  INST(Vpgatherdd      , "vpgatherdd"      , Enc(VexRmvRm_VM)       , V(660F38,90,_,x,0,_,_,_  ), V(660F38,90,_,x,_,0,2,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , 6783, 447),
  INST(Vpgatherdq      , "vpgatherdq"      , Enc(VexRmvRm_VM)       , V(660F38,90,_,x,1,_,_,_  ), V(660F38,90,_,x,_,1,3,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , 6794, 448),
  INST(Vpgatherqd      , "vpgatherqd"      , Enc(VexRmvRm_VM)       , V(660F38,91,_,x,0,_,_,_  ), V(660F38,91,_,x,_,0,2,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , 6805, 449),
  INST(Vpgatherqq      , "vpgatherqq"      , Enc(VexRmvRm_VM)       , V(660F38,91,_,x,1,_,_,_  ), V(660F38,91,_,x,_,1,3,T1S), F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , 6816, 450),
  INST(Vphaddbd        , "vphaddbd"        , Enc(VexRm)             , V(XOP_M9,C2,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6827, 260),
  INST(Vphaddbq        , "vphaddbq"        , Enc(VexRm)             , V(XOP_M9,C3,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6836, 260),
  INST(Vphaddbw        , "vphaddbw"        , Enc(VexRm)             , V(XOP_M9,C1,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6845, 260),
  INST(Vphaddd         , "vphaddd"         , Enc(VexRvm_Lx)         , V(660F38,02,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6854, 258),
  INST(Vphadddq        , "vphadddq"        , Enc(VexRm)             , V(XOP_M9,CB,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6862, 260),
  INST(Vphaddsw        , "vphaddsw"        , Enc(VexRvm_Lx)         , V(660F38,03,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6871, 258),
  INST(Vphaddubd       , "vphaddubd"       , Enc(VexRm)             , V(XOP_M9,D2,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6880, 260),
  INST(Vphaddubq       , "vphaddubq"       , Enc(VexRm)             , V(XOP_M9,D3,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6890, 260),
  INST(Vphaddubw       , "vphaddubw"       , Enc(VexRm)             , V(XOP_M9,D1,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6900, 260),
  INST(Vphaddudq       , "vphaddudq"       , Enc(VexRm)             , V(XOP_M9,DB,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6910, 260),
  INST(Vphadduwd       , "vphadduwd"       , Enc(VexRm)             , V(XOP_M9,D6,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6920, 260),
  INST(Vphadduwq       , "vphadduwq"       , Enc(VexRm)             , V(XOP_M9,D7,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6930, 260),
  INST(Vphaddw         , "vphaddw"         , Enc(VexRvm_Lx)         , V(660F38,01,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6940, 258),
  INST(Vphaddwd        , "vphaddwd"        , Enc(VexRm)             , V(XOP_M9,C6,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6948, 260),
  INST(Vphaddwq        , "vphaddwq"        , Enc(VexRm)             , V(XOP_M9,C7,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6957, 260),
  INST(Vphminposuw     , "vphminposuw"     , Enc(VexRm)             , V(660F38,41,_,0,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6966, 260),
  INST(Vphsubbw        , "vphsubbw"        , Enc(VexRm)             , V(XOP_M9,E1,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6978, 260),
  INST(Vphsubd         , "vphsubd"         , Enc(VexRvm_Lx)         , V(660F38,06,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6987, 258),
  INST(Vphsubdq        , "vphsubdq"        , Enc(VexRm)             , V(XOP_M9,E3,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 6995, 260),
  INST(Vphsubsw        , "vphsubsw"        , Enc(VexRvm_Lx)         , V(660F38,07,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7004, 258),
  INST(Vphsubw         , "vphsubw"         , Enc(VexRvm_Lx)         , V(660F38,05,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7013, 258),
  INST(Vphsubwd        , "vphsubwd"        , Enc(VexRm)             , V(XOP_M9,E2,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7021, 260),
  INST(Vpinsrb         , "vpinsrb"         , Enc(VexRvmi)           , V(660F3A,20,_,0,0,I,0,T1S), 0                         , F(WO)|F(Vex)   |A512(BW  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 7030, 451),
  INST(Vpinsrd         , "vpinsrd"         , Enc(VexRvmi)           , V(660F3A,22,_,0,0,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 7038, 452),
  INST(Vpinsrq         , "vpinsrq"         , Enc(VexRvmi)           , V(660F3A,22,_,0,1,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 7046, 453),
  INST(Vpinsrw         , "vpinsrw"         , Enc(VexRvmi)           , V(660F00,C4,_,0,0,I,1,T1S), 0                         , F(WO)|F(Vex)   |A512(BW  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 7054, 454),
  INST(Vplzcntd        , "vplzcntd"        , Enc(VexRm_Lx)          , V(660F38,44,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(CDI ,1,KZ,0  ,4) , EF(________), 0 , 0 , 7062, 430),
  INST(Vplzcntq        , "vplzcntq"        , Enc(VexRm_Lx)          , V(660F38,44,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(CDI ,1,KZ,0  ,8) , EF(________), 0 , 0 , 7071, 455),
  INST(Vpmacsdd        , "vpmacsdd"        , Enc(VexRvmr)           , V(XOP_M8,9E,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7080, 456),
  INST(Vpmacsdqh       , "vpmacsdqh"       , Enc(VexRvmr)           , V(XOP_M8,9F,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7089, 456),
  INST(Vpmacsdql       , "vpmacsdql"       , Enc(VexRvmr)           , V(XOP_M8,97,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7099, 456),
  INST(Vpmacssdd       , "vpmacssdd"       , Enc(VexRvmr)           , V(XOP_M8,8E,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7109, 456),
  INST(Vpmacssdqh      , "vpmacssdqh"      , Enc(VexRvmr)           , V(XOP_M8,8F,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7119, 456),
  INST(Vpmacssdql      , "vpmacssdql"      , Enc(VexRvmr)           , V(XOP_M8,87,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7130, 456),
  INST(Vpmacsswd       , "vpmacsswd"       , Enc(VexRvmr)           , V(XOP_M8,86,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7141, 456),
  INST(Vpmacssww       , "vpmacssww"       , Enc(VexRvmr)           , V(XOP_M8,85,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7151, 456),
  INST(Vpmacswd        , "vpmacswd"        , Enc(VexRvmr)           , V(XOP_M8,96,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7161, 456),
  INST(Vpmacsww        , "vpmacsww"        , Enc(VexRvmr)           , V(XOP_M8,95,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7170, 456),
  INST(Vpmadcsswd      , "vpmadcsswd"      , Enc(VexRvmr)           , V(XOP_M8,A6,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7179, 456),
  INST(Vpmadcswd       , "vpmadcswd"       , Enc(VexRvmr)           , V(XOP_M8,B6,_,0,0,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7190, 456),
  INST(Vpmadd52huq     , "vpmadd52huq"     , Enc(VexRvm_Lx)         , V(660F38,B5,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(IFMA,1,KZ,0  ,8) , EF(________), 0 , 0 , 7200, 457),
  INST(Vpmadd52luq     , "vpmadd52luq"     , Enc(VexRvm_Lx)         , V(660F38,B4,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(IFMA,1,KZ,0  ,8) , EF(________), 0 , 0 , 7212, 457),
  INST(Vpmaddubsw      , "vpmaddubsw"      , Enc(VexRvm_Lx)         , V(660F38,04,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7224, 408),
  INST(Vpmaddwd        , "vpmaddwd"        , Enc(VexRvm_Lx)         , V(660F00,F5,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7235, 408),
  INST(Vpmaskmovd      , "vpmaskmovd"      , Enc(VexRvmMvr_Lx)      , V(660F38,8C,_,x,0,_,_,_  ), V(660F38,8E,_,x,0,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7244, 458),
  INST(Vpmaskmovq      , "vpmaskmovq"      , Enc(VexRvmMvr_Lx)      , V(660F38,8C,_,x,1,_,_,_  ), V(660F38,8E,_,x,1,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7255, 459),
  INST(Vpmaxsb         , "vpmaxsb"         , Enc(VexRvm_Lx)         , V(660F38,3C,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7266, 408),
  INST(Vpmaxsd         , "vpmaxsd"         , Enc(VexRvm_Lx)         , V(660F38,3D,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 7274, 405),
  INST(Vpmaxsq         , "vpmaxsq"         , Enc(VexRvm_Lx)         , V(660F38,3D,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 7282, 268),
  INST(Vpmaxsw         , "vpmaxsw"         , Enc(VexRvm_Lx)         , V(660F00,EE,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7290, 408),
  INST(Vpmaxub         , "vpmaxub"         , Enc(VexRvm_Lx)         , V(660F00,DE,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7298, 408),
  INST(Vpmaxud         , "vpmaxud"         , Enc(VexRvm_Lx)         , V(660F38,3F,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 7306, 405),
  INST(Vpmaxuq         , "vpmaxuq"         , Enc(VexRvm_Lx)         , V(660F38,3F,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 7314, 268),
  INST(Vpmaxuw         , "vpmaxuw"         , Enc(VexRvm_Lx)         , V(660F38,3E,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7322, 408),
  INST(Vpminsb         , "vpminsb"         , Enc(VexRvm_Lx)         , V(660F38,38,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7330, 408),
  INST(Vpminsd         , "vpminsd"         , Enc(VexRvm_Lx)         , V(660F38,39,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 7338, 405),
  INST(Vpminsq         , "vpminsq"         , Enc(VexRvm_Lx)         , V(660F38,39,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 7346, 268),
  INST(Vpminsw         , "vpminsw"         , Enc(VexRvm_Lx)         , V(660F00,EA,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7354, 408),
  INST(Vpminub         , "vpminub"         , Enc(VexRvm_Lx)         , V(660F00,DA,_,x,I,_,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7362, 408),
  INST(Vpminud         , "vpminud"         , Enc(VexRvm_Lx)         , V(660F38,3B,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 7370, 405),
  INST(Vpminuq         , "vpminuq"         , Enc(VexRvm_Lx)         , V(660F38,3B,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 7378, 268),
  INST(Vpminuw         , "vpminuw"         , Enc(VexRvm_Lx)         , V(660F38,3A,_,x,I,_,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7386, 408),
  INST(Vpmovb2m        , "vpmovb2m"        , Enc(VexRm_Lx)          , V(F30F38,29,_,x,_,0,_,_  ), 0                         , F(WO)          |A512(BW  ,1,0 ,0  ,0) , EF(________), 0 , 0 , 7394, 460),
  INST(Vpmovd2m        , "vpmovd2m"        , Enc(VexRm_Lx)          , V(F30F38,39,_,x,_,0,_,_  ), 0                         , F(WO)          |A512(DQ  ,1,0 ,0  ,0) , EF(________), 0 , 0 , 7403, 461),
  INST(Vpmovdb         , "vpmovdb"         , Enc(VexMr_Lx)          , V(F30F38,31,_,x,_,0,2,QVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7412, 462),
  INST(Vpmovdw         , "vpmovdw"         , Enc(VexMr_Lx)          , V(F30F38,33,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7420, 463),
  INST(Vpmovm2b        , "vpmovm2b"        , Enc(VexRm_Lx)          , V(F30F38,28,_,x,_,0,_,_  ), 0                         , F(WO)          |A512(BW  ,1,0 ,0  ,0) , EF(________), 0 , 0 , 7428, 464),
  INST(Vpmovm2d        , "vpmovm2d"        , Enc(VexRm_Lx)          , V(F30F38,38,_,x,_,0,_,_  ), 0                         , F(WO)          |A512(DQ  ,1,0 ,0  ,0) , EF(________), 0 , 0 , 7437, 465),
  INST(Vpmovm2q        , "vpmovm2q"        , Enc(VexRm_Lx)          , V(F30F38,38,_,x,_,1,_,_  ), 0                         , F(WO)          |A512(DQ  ,1,0 ,0  ,0) , EF(________), 0 , 0 , 7446, 465),
  INST(Vpmovm2w        , "vpmovm2w"        , Enc(VexRm_Lx)          , V(F30F38,28,_,x,_,1,_,_  ), 0                         , F(WO)          |A512(BW  ,1,0 ,0  ,0) , EF(________), 0 , 0 , 7455, 464),
  INST(Vpmovmskb       , "vpmovmskb"       , Enc(VexRm_Lx)          , V(660F00,D7,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7464, 394),
  INST(Vpmovq2m        , "vpmovq2m"        , Enc(VexRm_Lx)          , V(F30F38,39,_,x,_,1,_,_  ), 0                         , F(WO)          |A512(DQ  ,1,0 ,0  ,0) , EF(________), 0 , 0 , 7474, 461),
  INST(Vpmovqb         , "vpmovqb"         , Enc(VexMr_Lx)          , V(F30F38,32,_,x,_,0,1,OVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7483, 466),
  INST(Vpmovqd         , "vpmovqd"         , Enc(VexMr_Lx)          , V(F30F38,35,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7491, 463),
  INST(Vpmovqw         , "vpmovqw"         , Enc(VexMr_Lx)          , V(F30F38,34,_,x,_,0,2,QVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7499, 462),
  INST(Vpmovsdb        , "vpmovsdb"        , Enc(VexMr_Lx)          , V(F30F38,21,_,x,_,0,2,QVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7507, 462),
  INST(Vpmovsdw        , "vpmovsdw"        , Enc(VexMr_Lx)          , V(F30F38,23,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7516, 463),
  INST(Vpmovsqb        , "vpmovsqb"        , Enc(VexMr_Lx)          , V(F30F38,22,_,x,_,0,1,OVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7525, 466),
  INST(Vpmovsqd        , "vpmovsqd"        , Enc(VexMr_Lx)          , V(F30F38,25,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7534, 463),
  INST(Vpmovsqw        , "vpmovsqw"        , Enc(VexMr_Lx)          , V(F30F38,24,_,x,_,0,2,QVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7543, 462),
  INST(Vpmovswb        , "vpmovswb"        , Enc(VexMr_Lx)          , V(F30F38,20,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7552, 467),
  INST(Vpmovsxbd       , "vpmovsxbd"       , Enc(VexRm_Lx)          , V(660F38,21,_,x,I,I,2,QVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7561, 468),
  INST(Vpmovsxbq       , "vpmovsxbq"       , Enc(VexRm_Lx)          , V(660F38,22,_,x,I,I,1,OVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7571, 469),
  INST(Vpmovsxbw       , "vpmovsxbw"       , Enc(VexRm_Lx)          , V(660F38,20,_,x,I,I,3,HVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7581, 470),
  INST(Vpmovsxdq       , "vpmovsxdq"       , Enc(VexRm_Lx)          , V(660F38,25,_,x,I,0,3,HVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7591, 471),
  INST(Vpmovsxwd       , "vpmovsxwd"       , Enc(VexRm_Lx)          , V(660F38,23,_,x,I,I,3,HVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7601, 472),
  INST(Vpmovsxwq       , "vpmovsxwq"       , Enc(VexRm_Lx)          , V(660F38,24,_,x,I,I,2,QVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7611, 468),
  INST(Vpmovusdb       , "vpmovusdb"       , Enc(VexMr_Lx)          , V(F30F38,11,_,x,_,0,2,QVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7621, 462),
  INST(Vpmovusdw       , "vpmovusdw"       , Enc(VexMr_Lx)          , V(F30F38,13,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7631, 463),
  INST(Vpmovusqb       , "vpmovusqb"       , Enc(VexMr_Lx)          , V(F30F38,12,_,x,_,0,1,OVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7641, 466),
  INST(Vpmovusqd       , "vpmovusqd"       , Enc(VexMr_Lx)          , V(F30F38,15,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7651, 463),
  INST(Vpmovusqw       , "vpmovusqw"       , Enc(VexMr_Lx)          , V(F30F38,14,_,x,_,0,2,QVM), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7661, 462),
  INST(Vpmovuswb       , "vpmovuswb"       , Enc(VexMr_Lx)          , V(F30F38,10,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7671, 467),
  INST(Vpmovw2m        , "vpmovw2m"        , Enc(VexRm_Lx)          , V(F30F38,29,_,x,_,1,_,_  ), 0                         , F(WO)          |A512(BW  ,1,0 ,0  ,0) , EF(________), 0 , 0 , 7681, 460),
  INST(Vpmovwb         , "vpmovwb"         , Enc(VexMr_Lx)          , V(F30F38,30,_,x,_,0,3,HVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7690, 467),
  INST(Vpmovzxbd       , "vpmovzxbd"       , Enc(VexRm_Lx)          , V(660F38,31,_,x,I,I,2,QVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7698, 468),
  INST(Vpmovzxbq       , "vpmovzxbq"       , Enc(VexRm_Lx)          , V(660F38,32,_,x,I,I,1,OVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7708, 469),
  INST(Vpmovzxbw       , "vpmovzxbw"       , Enc(VexRm_Lx)          , V(660F38,30,_,x,I,I,3,HVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7718, 470),
  INST(Vpmovzxdq       , "vpmovzxdq"       , Enc(VexRm_Lx)          , V(660F38,35,_,x,I,0,3,HVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7728, 471),
  INST(Vpmovzxwd       , "vpmovzxwd"       , Enc(VexRm_Lx)          , V(660F38,33,_,x,I,I,3,HVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7738, 472),
  INST(Vpmovzxwq       , "vpmovzxwq"       , Enc(VexRm_Lx)          , V(660F38,34,_,x,I,I,2,QVM), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7748, 468),
  INST(Vpmuldq         , "vpmuldq"         , Enc(VexRvm_Lx)         , V(660F38,28,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 7758, 409),
  INST(Vpmulhrsw       , "vpmulhrsw"       , Enc(VexRvm_Lx)         , V(660F38,0B,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7766, 408),
  INST(Vpmulhuw        , "vpmulhuw"        , Enc(VexRvm_Lx)         , V(660F00,E4,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7776, 408),
  INST(Vpmulhw         , "vpmulhw"         , Enc(VexRvm_Lx)         , V(660F00,E5,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7785, 408),
  INST(Vpmulld         , "vpmulld"         , Enc(VexRvm_Lx)         , V(660F38,40,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 7793, 405),
  INST(Vpmullq         , "vpmullq"         , Enc(VexRvm_Lx)         , V(660F38,40,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 7801, 473),
  INST(Vpmullw         , "vpmullw"         , Enc(VexRvm_Lx)         , V(660F00,D5,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 7809, 408),
  INST(Vpmultishiftqb  , "vpmultishiftqb"  , Enc(VexRvm_Lx)         , V(660F38,83,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(VBMI,1,KZ,0  ,8) , EF(________), 0 , 0 , 7817, 474),
  INST(Vpmuludq        , "vpmuludq"        , Enc(VexRvm_Lx)         , V(660F00,F4,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 7832, 409),
  INST(Vpor            , "vpor"            , Enc(VexRvm_Lx)         , V(660F00,EB,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7841, 258),
  INST(Vpord           , "vpord"           , Enc(VexRvm_Lx)         , V(660F00,EB,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 7846, 267),
  INST(Vporq           , "vporq"           , Enc(VexRvm_Lx)         , V(660F00,EB,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 7852, 268),
  INST(Vpperm          , "vpperm"          , Enc(VexRvrmRvmr)       , V(XOP_M8,A3,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7858, 475),
  INST(Vprold          , "vprold"          , Enc(VexVmi_Lx)         , V(660F00,72,1,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 7865, 476),
  INST(Vprolq          , "vprolq"          , Enc(VexVmi_Lx)         , V(660F00,72,1,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 7872, 477),
  INST(Vprolvd         , "vprolvd"         , Enc(VexRvm_Lx)         , V(660F38,15,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 7879, 267),
  INST(Vprolvq         , "vprolvq"         , Enc(VexRvm_Lx)         , V(660F38,15,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 7887, 268),
  INST(Vprord          , "vprord"          , Enc(VexVmi_Lx)         , V(660F00,72,0,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 7895, 476),
  INST(Vprorq          , "vprorq"          , Enc(VexVmi_Lx)         , V(660F00,72,0,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 7902, 477),
  INST(Vprorvd         , "vprorvd"         , Enc(VexRvm_Lx)         , V(660F38,14,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 7909, 267),
  INST(Vprorvq         , "vprorvq"         , Enc(VexRvm_Lx)         , V(660F38,14,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 7917, 268),
  INST(Vprotb          , "vprotb"          , Enc(VexRvmRmvRmi)      , V(XOP_M9,90,_,0,x,_,_,_  ), V(XOP_M8,C0,_,0,x,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7925, 478),
  INST(Vprotd          , "vprotd"          , Enc(VexRvmRmvRmi)      , V(XOP_M9,92,_,0,x,_,_,_  ), V(XOP_M8,C2,_,0,x,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7932, 479),
  INST(Vprotq          , "vprotq"          , Enc(VexRvmRmvRmi)      , V(XOP_M9,93,_,0,x,_,_,_  ), V(XOP_M8,C3,_,0,x,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7939, 480),
  INST(Vprotw          , "vprotw"          , Enc(VexRvmRmvRmi)      , V(XOP_M9,91,_,0,x,_,_,_  ), V(XOP_M8,C1,_,0,x,_,_,_  ), F(WO)|F(Vex)                          , EF(________), 0 , 0 , 7946, 481),
  INST(Vpsadbw         , "vpsadbw"         , Enc(VexRvm_Lx)         , V(660F00,F6,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,0 ,0  ,0) , EF(________), 0 , 0 , 7953, 482),
  INST(Vpscatterdd     , "vpscatterdd"     , Enc(VexMr_VM)          , V(660F38,A0,_,x,_,0,2,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , 7961, 483),
  INST(Vpscatterdq     , "vpscatterdq"     , Enc(VexMr_VM)          , V(660F38,A0,_,x,_,1,3,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , 7973, 483),
  INST(Vpscatterqd     , "vpscatterqd"     , Enc(VexMr_VM)          , V(660F38,A1,_,x,_,0,2,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , 7985, 484),
  INST(Vpscatterqq     , "vpscatterqq"     , Enc(VexMr_VM)          , V(660F38,A1,_,x,_,1,3,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , 7997, 485),
  INST(Vpshab          , "vpshab"          , Enc(VexRvmRmv)         , V(XOP_M9,98,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8009, 486),
  INST(Vpshad          , "vpshad"          , Enc(VexRvmRmv)         , V(XOP_M9,9A,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8016, 486),
  INST(Vpshaq          , "vpshaq"          , Enc(VexRvmRmv)         , V(XOP_M9,9B,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8023, 486),
  INST(Vpshaw          , "vpshaw"          , Enc(VexRvmRmv)         , V(XOP_M9,99,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8030, 486),
  INST(Vpshlb          , "vpshlb"          , Enc(VexRvmRmv)         , V(XOP_M9,94,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8037, 486),
  INST(Vpshld          , "vpshld"          , Enc(VexRvmRmv)         , V(XOP_M9,96,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8044, 486),
  INST(Vpshlq          , "vpshlq"          , Enc(VexRvmRmv)         , V(XOP_M9,97,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8051, 486),
  INST(Vpshlw          , "vpshlw"          , Enc(VexRvmRmv)         , V(XOP_M9,95,_,0,x,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8058, 486),
  INST(Vpshufb         , "vpshufb"         , Enc(VexRvm_Lx)         , V(660F38,00,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8065, 408),
  INST(Vpshufd         , "vpshufd"         , Enc(VexRmi_Lx)         , V(660F00,70,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 8073, 487),
  INST(Vpshufhw        , "vpshufhw"        , Enc(VexRmi_Lx)         , V(F30F00,70,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8081, 488),
  INST(Vpshuflw        , "vpshuflw"        , Enc(VexRmi_Lx)         , V(F20F00,70,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8090, 488),
  INST(Vpsignb         , "vpsignb"         , Enc(VexRvm_Lx)         , V(660F38,08,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8099, 258),
  INST(Vpsignd         , "vpsignd"         , Enc(VexRvm_Lx)         , V(660F38,0A,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8107, 258),
  INST(Vpsignw         , "vpsignw"         , Enc(VexRvm_Lx)         , V(660F38,09,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8115, 258),
  INST(Vpslld          , "vpslld"          , Enc(VexRvmVmi_Lx)      , V(660F00,F2,_,x,I,0,4,128), V(660F00,72,6,x,I,0,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 8123, 489),
  INST(Vpslldq         , "vpslldq"         , Enc(VexEvexVmi_Lx)     , V(660F00,73,7,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,0 ,0  ,0) , EF(________), 0 , 0 , 8130, 490),
  INST(Vpsllq          , "vpsllq"          , Enc(VexRvmVmi_Lx)      , V(660F00,F3,_,x,I,1,4,128), V(660F00,73,6,x,I,1,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 8138, 491),
  INST(Vpsllvd         , "vpsllvd"         , Enc(VexRvm_Lx)         , V(660F38,47,_,x,0,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 8145, 405),
  INST(Vpsllvq         , "vpsllvq"         , Enc(VexRvm_Lx)         , V(660F38,47,_,x,1,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 8153, 409),
  INST(Vpsllvw         , "vpsllvw"         , Enc(VexRvm_Lx)         , V(660F38,12,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8161, 266),
  INST(Vpsllw          , "vpsllw"          , Enc(VexRvmVmi_Lx)      , V(660F00,F1,_,x,I,I,4,FVM), V(660F00,71,6,x,I,I,4,FVM), F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8169, 492),
  INST(Vpsrad          , "vpsrad"          , Enc(VexRvmVmi_Lx)      , V(660F00,E2,_,x,I,0,4,128), V(660F00,72,4,x,I,0,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 8176, 493),
  INST(Vpsraq          , "vpsraq"          , Enc(VexRvmVmi_Lx)      , V(660F00,E2,_,x,_,1,4,128), V(660F00,72,4,x,_,1,4,FV ), F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 8183, 494),
  INST(Vpsravd         , "vpsravd"         , Enc(VexRvm_Lx)         , V(660F38,46,_,x,0,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 8190, 405),
  INST(Vpsravq         , "vpsravq"         , Enc(VexRvm_Lx)         , V(660F38,46,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 8198, 268),
  INST(Vpsravw         , "vpsravw"         , Enc(VexRvm_Lx)         , V(660F38,11,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8206, 266),
  INST(Vpsraw          , "vpsraw"          , Enc(VexRvmVmi_Lx)      , V(660F00,E1,_,x,I,I,4,128), V(660F00,71,4,x,I,I,4,FVM), F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8214, 495),
  INST(Vpsrld          , "vpsrld"          , Enc(VexRvmVmi_Lx)      , V(660F00,D2,_,x,I,0,4,128), V(660F00,72,2,x,I,0,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 8221, 496),
  INST(Vpsrldq         , "vpsrldq"         , Enc(VexEvexVmi_Lx)     , V(660F00,73,3,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,0 ,0  ,0) , EF(________), 0 , 0 , 8228, 490),
  INST(Vpsrlq          , "vpsrlq"          , Enc(VexRvmVmi_Lx)      , V(660F00,D3,_,x,I,1,4,128), V(660F00,73,2,x,I,1,4,FV ), F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 8236, 497),
  INST(Vpsrlvd         , "vpsrlvd"         , Enc(VexRvm_Lx)         , V(660F38,45,_,x,0,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 8243, 405),
  INST(Vpsrlvq         , "vpsrlvq"         , Enc(VexRvm_Lx)         , V(660F38,45,_,x,1,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 8251, 409),
  INST(Vpsrlvw         , "vpsrlvw"         , Enc(VexRvm_Lx)         , V(660F38,10,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8259, 266),
  INST(Vpsrlw          , "vpsrlw"          , Enc(VexRvmVmi_Lx)      , V(660F00,D1,_,x,I,I,4,128), V(660F00,71,2,x,I,I,4,FVM), F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8267, 498),
  INST(Vpsubb          , "vpsubb"          , Enc(VexRvm_Lx)         , V(660F00,F8,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8274, 408),
  INST(Vpsubd          , "vpsubd"          , Enc(VexRvm_Lx)         , V(660F00,FA,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 8281, 405),
  INST(Vpsubq          , "vpsubq"          , Enc(VexRvm_Lx)         , V(660F00,FB,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 8288, 409),
  INST(Vpsubsb         , "vpsubsb"         , Enc(VexRvm_Lx)         , V(660F00,E8,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8295, 408),
  INST(Vpsubsw         , "vpsubsw"         , Enc(VexRvm_Lx)         , V(660F00,E9,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8303, 408),
  INST(Vpsubusb        , "vpsubusb"        , Enc(VexRvm_Lx)         , V(660F00,D8,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8311, 408),
  INST(Vpsubusw        , "vpsubusw"        , Enc(VexRvm_Lx)         , V(660F00,D9,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8320, 408),
  INST(Vpsubw          , "vpsubw"          , Enc(VexRvm_Lx)         , V(660F00,F9,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8329, 408),
  INST(Vpternlogd      , "vpternlogd"      , Enc(VexRvmi_Lx)        , V(660F3A,25,_,x,_,0,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 8336, 499),
  INST(Vpternlogq      , "vpternlogq"      , Enc(VexRvmi_Lx)        , V(660F3A,25,_,x,_,1,4,FV ), 0                         , F(RW)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 8347, 500),
  INST(Vptest          , "vptest"          , Enc(VexRm_Lx)          , V(660F38,17,_,x,I,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 8358, 501),
  INST(Vptestmb        , "vptestmb"        , Enc(VexRvm_Lx)         , V(660F38,26,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , 8365, 502),
  INST(Vptestmd        , "vptestmd"        , Enc(VexRvm_Lx)         , V(660F38,27,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,4) , EF(________), 0 , 0 , 8374, 503),
  INST(Vptestmq        , "vptestmq"        , Enc(VexRvm_Lx)         , V(660F38,27,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,8) , EF(________), 0 , 0 , 8383, 504),
  INST(Vptestmw        , "vptestmw"        , Enc(VexRvm_Lx)         , V(660F38,26,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , 8392, 502),
  INST(Vptestnmb       , "vptestnmb"       , Enc(VexRvm_Lx)         , V(F30F38,26,_,x,_,0,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , 8401, 502),
  INST(Vptestnmd       , "vptestnmd"       , Enc(VexRvm_Lx)         , V(F30F38,27,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,4) , EF(________), 0 , 0 , 8411, 503),
  INST(Vptestnmq       , "vptestnmq"       , Enc(VexRvm_Lx)         , V(F30F38,27,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,K_,0  ,8) , EF(________), 0 , 0 , 8421, 504),
  INST(Vptestnmw       , "vptestnmw"       , Enc(VexRvm_Lx)         , V(F30F38,26,_,x,_,1,4,FVM), 0                         , F(WO)          |A512(BW  ,1,K_,0  ,0) , EF(________), 0 , 0 , 8431, 502),
  INST(Vpunpckhbw      , "vpunpckhbw"      , Enc(VexRvm_Lx)         , V(660F00,68,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8441, 408),
  INST(Vpunpckhdq      , "vpunpckhdq"      , Enc(VexRvm_Lx)         , V(660F00,6A,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 8452, 405),
  INST(Vpunpckhqdq     , "vpunpckhqdq"     , Enc(VexRvm_Lx)         , V(660F00,6D,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 8463, 409),
  INST(Vpunpckhwd      , "vpunpckhwd"      , Enc(VexRvm_Lx)         , V(660F00,69,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8475, 408),
  INST(Vpunpcklbw      , "vpunpcklbw"      , Enc(VexRvm_Lx)         , V(660F00,60,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8486, 408),
  INST(Vpunpckldq      , "vpunpckldq"      , Enc(VexRvm_Lx)         , V(660F00,62,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 8497, 405),
  INST(Vpunpcklqdq     , "vpunpcklqdq"     , Enc(VexRvm_Lx)         , V(660F00,6C,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 8508, 409),
  INST(Vpunpcklwd      , "vpunpcklwd"      , Enc(VexRvm_Lx)         , V(660F00,61,_,x,I,I,4,FVM), 0                         , F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , EF(________), 0 , 0 , 8520, 408),
  INST(Vpxor           , "vpxor"           , Enc(VexRvm_Lx)         , V(660F00,EF,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8531, 258),
  INST(Vpxord          , "vpxord"          , Enc(VexRvm_Lx)         , V(660F00,EF,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 8537, 267),
  INST(Vpxorq          , "vpxorq"          , Enc(VexRvm_Lx)         , V(660F00,EF,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 8544, 268),
  INST(Vrangepd        , "vrangepd"        , Enc(VexRvmi_Lx)        , V(660F3A,50,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,SAE,8) , EF(________), 0 , 0 , 8551, 505),
  INST(Vrangeps        , "vrangeps"        , Enc(VexRvmi_Lx)        , V(660F3A,50,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,SAE,4) , EF(________), 0 , 0 , 8560, 506),
  INST(Vrangesd        , "vrangesd"        , Enc(VexRvmi)           , V(660F3A,51,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(DQ  ,0,KZ,SAE,0) , EF(________), 0 , 0 , 8569, 507),
  INST(Vrangess        , "vrangess"        , Enc(VexRvmi)           , V(660F3A,51,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(DQ  ,0,KZ,SAE,0) , EF(________), 0 , 0 , 8578, 508),
  INST(Vrcp14pd        , "vrcp14pd"        , Enc(VexRm_Lx)          , V(660F38,4C,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 8587, 509),
  INST(Vrcp14ps        , "vrcp14ps"        , Enc(VexRm_Lx)          , V(660F38,4C,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 8596, 510),
  INST(Vrcp14sd        , "vrcp14sd"        , Enc(VexRvm)            , V(660F38,4D,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 8605, 511),
  INST(Vrcp14ss        , "vrcp14ss"        , Enc(VexRvm)            , V(660F38,4D,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 8614, 512),
  INST(Vrcp28pd        , "vrcp28pd"        , Enc(VexRm)             , V(660F38,CA,_,2,_,1,4,FV ), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,8) , EF(________), 0 , 0 , 8623, 322),
  INST(Vrcp28ps        , "vrcp28ps"        , Enc(VexRm)             , V(660F38,CA,_,2,_,0,4,FV ), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,4) , EF(________), 0 , 0 , 8632, 323),
  INST(Vrcp28sd        , "vrcp28sd"        , Enc(VexRvm)            , V(660F38,CB,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,0) , EF(________), 0 , 0 , 8641, 513),
  INST(Vrcp28ss        , "vrcp28ss"        , Enc(VexRvm)            , V(660F38,CB,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,0) , EF(________), 0 , 0 , 8650, 514),
  INST(Vrcpps          , "vrcpps"          , Enc(VexRm_Lx)          , V(000F00,53,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8659, 346),
  INST(Vrcpss          , "vrcpss"          , Enc(VexRvm)            , V(F30F00,53,_,I,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8666, 515),
  INST(Vreducepd       , "vreducepd"       , Enc(VexRmi_Lx)         , V(660F3A,56,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 8673, 516),
  INST(Vreduceps       , "vreduceps"       , Enc(VexRmi_Lx)         , V(660F3A,56,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(DQ  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 8683, 517),
  INST(Vreducesd       , "vreducesd"       , Enc(VexRvmi)           , V(660F3A,57,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 8693, 518),
  INST(Vreducess       , "vreducess"       , Enc(VexRvmi)           , V(660F3A,57,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(DQ  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 8703, 519),
  INST(Vrndscalepd     , "vrndscalepd"     , Enc(VexRmi_Lx)         , V(660F3A,09,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,8) , EF(________), 0 , 0 , 8713, 358),
  INST(Vrndscaleps     , "vrndscaleps"     , Enc(VexRmi_Lx)         , V(660F3A,08,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,SAE,4) , EF(________), 0 , 0 , 8725, 359),
  INST(Vrndscalesd     , "vrndscalesd"     , Enc(VexRvmi)           , V(660F3A,0B,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , 8737, 520),
  INST(Vrndscaless     , "vrndscaless"     , Enc(VexRvmi)           , V(660F3A,0A,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,SAE,0) , EF(________), 0 , 0 , 8749, 521),
  INST(Vroundpd        , "vroundpd"        , Enc(VexRmi_Lx)         , V(660F3A,09,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8761, 522),
  INST(Vroundps        , "vroundps"        , Enc(VexRmi_Lx)         , V(660F3A,08,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8770, 522),
  INST(Vroundsd        , "vroundsd"        , Enc(VexRvmi)           , V(660F3A,0B,_,I,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8779, 523),
  INST(Vroundss        , "vroundss"        , Enc(VexRvmi)           , V(660F3A,0A,_,I,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8788, 524),
  INST(Vrsqrt14pd      , "vrsqrt14pd"      , Enc(VexRm_Lx)          , V(660F38,4E,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 8797, 509),
  INST(Vrsqrt14ps      , "vrsqrt14ps"      , Enc(VexRm_Lx)          , V(660F38,4E,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 8808, 510),
  INST(Vrsqrt14sd      , "vrsqrt14sd"      , Enc(VexRvm)            , V(660F38,4F,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 8819, 511),
  INST(Vrsqrt14ss      , "vrsqrt14ss"      , Enc(VexRvm)            , V(660F38,4F,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,0  ,0) , EF(________), 0 , 0 , 8830, 512),
  INST(Vrsqrt28pd      , "vrsqrt28pd"      , Enc(VexRm)             , V(660F38,CC,_,2,_,1,4,FV ), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,8) , EF(________), 0 , 0 , 8841, 322),
  INST(Vrsqrt28ps      , "vrsqrt28ps"      , Enc(VexRm)             , V(660F38,CC,_,2,_,0,4,FV ), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,4) , EF(________), 0 , 0 , 8852, 323),
  INST(Vrsqrt28sd      , "vrsqrt28sd"      , Enc(VexRvm)            , V(660F38,CD,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,0) , EF(________), 0 , 0 , 8863, 513),
  INST(Vrsqrt28ss      , "vrsqrt28ss"      , Enc(VexRvm)            , V(660F38,CD,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(ERI ,0,KZ,SAE,0) , EF(________), 0 , 0 , 8874, 514),
  INST(Vrsqrtps        , "vrsqrtps"        , Enc(VexRm_Lx)          , V(000F00,52,_,x,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8885, 346),
  INST(Vrsqrtss        , "vrsqrtss"        , Enc(VexRvm)            , V(F30F00,52,_,I,I,_,_,_  ), 0                         , F(WO)|F(Vex)                          , EF(________), 0 , 0 , 8894, 515),
  INST(Vscalefpd       , "vscalefpd"       , Enc(VexRvm_Lx)         , V(660F38,2C,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 8903, 525),
  INST(Vscalefps       , "vscalefps"       , Enc(VexRvm_Lx)         , V(660F38,2C,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 8913, 526),
  INST(Vscalefsd       , "vscalefsd"       , Enc(VexRvm)            , V(660F38,2D,_,I,_,1,3,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 8923, 527),
  INST(Vscalefss       , "vscalefss"       , Enc(VexRvm)            , V(660F38,2D,_,I,_,0,2,T1S), 0                         , F(WO)          |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 8933, 528),
  INST(Vscatterdpd     , "vscatterdpd"     , Enc(VexMr_Lx)          , V(660F38,A2,_,x,_,1,3,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , 8943, 529),
  INST(Vscatterdps     , "vscatterdps"     , Enc(VexMr_Lx)          , V(660F38,A2,_,x,_,0,2,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , 8955, 530),
  INST(Vscatterpf0dpd  , "vscatterpf0dpd"  , Enc(VexM_VM)           , V(660F38,C6,5,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , 8967, 351),
  INST(Vscatterpf0dps  , "vscatterpf0dps"  , Enc(VexM_VM)           , V(660F38,C6,5,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , 8982, 352),
  INST(Vscatterpf0qpd  , "vscatterpf0qpd"  , Enc(VexM_VM)           , V(660F38,C7,5,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , 8997, 353),
  INST(Vscatterpf0qps  , "vscatterpf0qps"  , Enc(VexM_VM)           , V(660F38,C7,5,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , 9012, 353),
  INST(Vscatterpf1dpd  , "vscatterpf1dpd"  , Enc(VexM_VM)           , V(660F38,C6,6,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , 9027, 351),
  INST(Vscatterpf1dps  , "vscatterpf1dps"  , Enc(VexM_VM)           , V(660F38,C6,6,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , 9042, 352),
  INST(Vscatterpf1qpd  , "vscatterpf1qpd"  , Enc(VexM_VM)           , V(660F38,C7,6,2,_,1,3,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , 9057, 353),
  INST(Vscatterpf1qps  , "vscatterpf1qps"  , Enc(VexM_VM)           , V(660F38,C7,6,2,_,0,2,T1S), 0                         , F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , EF(________), 0 , 0 , 9072, 353),
  INST(Vscatterqpd     , "vscatterqpd"     , Enc(VexMr_Lx)          , V(660F38,A3,_,x,_,1,3,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , 9087, 531),
  INST(Vscatterqps     , "vscatterqps"     , Enc(VexMr_Lx)          , V(660F38,A3,_,x,_,0,2,T1S), 0                         , F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , EF(________), 0 , 0 , 9099, 532),
  INST(Vshuff32x4      , "vshuff32x4"      , Enc(VexRvmi_Lx)        , V(660F3A,23,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 9111, 533),
  INST(Vshuff64x2      , "vshuff64x2"      , Enc(VexRvmi_Lx)        , V(660F3A,23,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 9122, 534),
  INST(Vshufi32x4      , "vshufi32x4"      , Enc(VexRvmi_Lx)        , V(660F3A,43,_,x,_,0,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 9133, 533),
  INST(Vshufi64x2      , "vshufi64x2"      , Enc(VexRvmi_Lx)        , V(660F3A,43,_,x,_,1,4,FV ), 0                         , F(WO)          |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 9144, 534),
  INST(Vshufpd         , "vshufpd"         , Enc(VexRvmi_Lx)        , V(660F00,C6,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 9155, 535),
  INST(Vshufps         , "vshufps"         , Enc(VexRvmi_Lx)        , V(000F00,C6,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 9163, 536),
  INST(Vsqrtpd         , "vsqrtpd"         , Enc(VexRm_Lx)          , V(660F00,51,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 9171, 537),
  INST(Vsqrtps         , "vsqrtps"         , Enc(VexRm_Lx)          , V(000F00,51,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 9179, 291),
  INST(Vsqrtsd         , "vsqrtsd"         , Enc(VexRvm)            , V(F20F00,51,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 9187, 256),
  INST(Vsqrtss         , "vsqrtss"         , Enc(VexRvm)            , V(F30F00,51,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 9195, 257),
  INST(Vstmxcsr        , "vstmxcsr"        , Enc(VexM)              , V(000F00,AE,3,0,I,_,_,_  ), 0                         , F(Vex)|F(Volatile)                    , EF(________), 0 , 0 , 9203, 538),
  INST(Vsubpd          , "vsubpd"          , Enc(VexRvm_Lx)         , V(660F00,5C,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , EF(________), 0 , 0 , 9212, 254),
  INST(Vsubps          , "vsubps"          , Enc(VexRvm_Lx)         , V(000F00,5C,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , EF(________), 0 , 0 , 9219, 255),
  INST(Vsubsd          , "vsubsd"          , Enc(VexRvm)            , V(F20F00,5C,_,I,I,1,3,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 9226, 256),
  INST(Vsubss          , "vsubss"          , Enc(VexRvm)            , V(F30F00,5C,_,I,I,0,2,T1S), 0                         , F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , EF(________), 0 , 0 , 9233, 257),
  INST(Vtestpd         , "vtestpd"         , Enc(VexRm_Lx)          , V(660F38,0F,_,x,0,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 9240, 501),
  INST(Vtestps         , "vtestps"         , Enc(VexRm_Lx)          , V(660F38,0E,_,x,0,_,_,_  ), 0                         , F(RO)|F(Vex)                          , EF(WWWWWW__), 0 , 0 , 9248, 501),
  INST(Vucomisd        , "vucomisd"        , Enc(VexRm)             , V(660F00,2E,_,I,I,1,3,T1S), 0                         , F(RO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , EF(WWWWWW__), 0 , 0 , 9256, 287),
  INST(Vucomiss        , "vucomiss"        , Enc(VexRm)             , V(000F00,2E,_,I,I,0,2,T1S), 0                         , F(RO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , EF(WWWWWW__), 0 , 0 , 9265, 288),
  INST(Vunpckhpd       , "vunpckhpd"       , Enc(VexRvm_Lx)         , V(660F00,15,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 9274, 409),
  INST(Vunpckhps       , "vunpckhps"       , Enc(VexRvm_Lx)         , V(000F00,15,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 9284, 405),
  INST(Vunpcklpd       , "vunpcklpd"       , Enc(VexRvm_Lx)         , V(660F00,14,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 9294, 409),
  INST(Vunpcklps       , "vunpcklps"       , Enc(VexRvm_Lx)         , V(000F00,14,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 9304, 405),
  INST(Vxorpd          , "vxorpd"          , Enc(VexRvm_Lx)         , V(660F00,57,_,x,I,1,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,8) , EF(________), 0 , 0 , 9314, 264),
  INST(Vxorps          , "vxorps"          , Enc(VexRvm_Lx)         , V(000F00,57,_,x,I,0,4,FV ), 0                         , F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,4) , EF(________), 0 , 0 , 9321, 265),
  INST(Vzeroall        , "vzeroall"        , Enc(VexOp)             , V(000F00,77,_,1,I,_,_,_  ), 0                         , F(Vex)|F(Volatile)                    , EF(________), 0 , 0 , 9328, 539),
  INST(Vzeroupper      , "vzeroupper"      , Enc(VexOp)             , V(000F00,77,_,0,I,_,_,_  ), 0                         , F(Vex)|F(Volatile)                    , EF(________), 0 , 0 , 9337, 539),
  INST(Wrfsbase        , "wrfsbase"        , Enc(X86M)              , O(F30F00,AE,2,_,x,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , 9348, 540),
  INST(Wrgsbase        , "wrgsbase"        , Enc(X86M)              , O(F30F00,AE,3,_,x,_,_,_  ), 0                         , F(RO)|F(Volatile)                     , EF(________), 0 , 0 , 9357, 540),
  INST(Xadd            , "xadd"            , Enc(X86Xadd)           , O(000F00,C0,_,_,x,_,_,_  ), 0                         , F(RW)|F(Xchg)|F(Lock)                 , EF(WWWWWW__), 0 , 0 , 9366, 541),
  INST(Xchg            , "xchg"            , Enc(X86Xchg)           , O(000000,86,_,_,x,_,_,_  ), 0                         , F(RW)|F(Xchg)|F(Lock)                 , EF(________), 0 , 0 , 353 , 542),
  INST(Xgetbv          , "xgetbv"          , Enc(X86Op)             , O(000F01,D0,_,_,_,_,_,_  ), 0                         , F(WO)|F(Special)                      , EF(________), 0 , 0 , 9371, 543),
  INST(Xor             , "xor"             , Enc(X86Arith)          , O(000000,30,6,_,x,_,_,_  ), 0                         , F(RW)|F(Lock)                         , EF(WWWUWW__), 0 , 0 , 8533, 3  ),
  INST(Xorpd           , "xorpd"           , Enc(ExtRm)             , O(660F00,57,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 9315, 4  ),
  INST(Xorps           , "xorps"           , Enc(ExtRm)             , O(000F00,57,_,_,_,_,_,_  ), 0                         , F(RW)                                 , EF(________), 0 , 0 , 9322, 4  ),
  INST(Xrstor          , "xrstor"          , Enc(X86M_Only)         , O(000F00,AE,5,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 1030, 544),
  INST(Xrstor64        , "xrstor64"        , Enc(X86M_Only)         , O(000F00,AE,5,_,1,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 1038, 545),
  INST(Xrstors         , "xrstors"         , Enc(X86M_Only)         , O(000F00,C7,3,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 9378, 544),
  INST(Xrstors64       , "xrstors64"       , Enc(X86M_Only)         , O(000F00,C7,3,_,1,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 9386, 545),
  INST(Xsave           , "xsave"           , Enc(X86M_Only)         , O(000F00,AE,4,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 1048, 546),
  INST(Xsave64         , "xsave64"         , Enc(X86M_Only)         , O(000F00,AE,4,_,1,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 1055, 547),
  INST(Xsavec          , "xsavec"          , Enc(X86M_Only)         , O(000F00,C7,4,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 9396, 546),
  INST(Xsavec64        , "xsavec64"        , Enc(X86M_Only)         , O(000F00,C7,4,_,1,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 9403, 547),
  INST(Xsaveopt        , "xsaveopt"        , Enc(X86M_Only)         , O(000F00,AE,6,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 9412, 546),
  INST(Xsaveopt64      , "xsaveopt64"      , Enc(X86M_Only)         , O(000F00,AE,6,_,1,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 9421, 547),
  INST(Xsaves          , "xsaves"          , Enc(X86M_Only)         , O(000F00,C7,5,_,_,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 9432, 546),
  INST(Xsaves64        , "xsaves64"        , Enc(X86M_Only)         , O(000F00,C7,5,_,1,_,_,_  ), 0                         , F(WO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 9439, 547),
  INST(Xsetbv          , "xsetbv"          , Enc(X86Op)             , O(000F01,D1,_,_,_,_,_,_  ), 0                         , F(RO)|F(Volatile)|F(Special)          , EF(________), 0 , 0 , 9448, 548)
// ${X86InstData:End}
};

// ${X86InstExtendedData:Begin}
// ------------------- Automatically generated, do not edit -------------------
const X86Inst::ExtendedData X86InstDB::iExtData[] = {
  { Enc(None)               , 0  , 0  , 0x00, 0x00, 0  , 0 , 0, 0                                     , 0                          },
  { Enc(X86Arith)           , 0  , 0  , 0x20, 0x3F, 13 , 10, 0, F(RW)|F(Lock)                         , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0x20, 0x20, 21 , 2 , 0, F(RW)                                 , 0                          },
  { Enc(X86Arith)           , 0  , 0  , 0x00, 0x3F, 13 , 10, 0, F(RW)|F(Lock)                         , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 0x00, 0x00, 288, 1 , 0, F(RW)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 0x00, 0x00, 345, 1 , 0, F(RW)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 0x00, 0x00, 346, 1 , 0, F(RW)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0x01, 0x01, 21 , 2 , 0, F(RW)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 0x00, 0x00, 63 , 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 0x00, 0x00, 70 , 1 , 0, F(WO)                                 , 0                          },
  { Enc(VexRvm_Wx)          , 0  , 0  , 0x00, 0x3F, 245, 2 , 0, F(RW)                                 , 0                          },
  { Enc(VexRmv_Wx)          , 0  , 0  , 0x00, 0x3F, 247, 2 , 0, F(RW)                                 , 0                          },
  { Enc(VexVm_Wx)           , 0  , 0  , 0x00, 0x3F, 153, 2 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 0x00, 0x00, 290, 1 , 0, F(RW)                                 , 0                          },
  { Enc(ExtRmXMM0)          , 0  , 0  , 0x00, 0x00, 347, 1 , 0, F(RW)|F(Special)                      , 0                          },
  { Enc(VexVm_Wx)           , 0  , 0  , 0x00, 0x3F, 153, 2 , 0, F(RW)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0x00, 0x3F, 20 , 3 , 0, F(RW)                                 , 0                          },
  { Enc(X86Bswap)           , 0  , 0  , 0x00, 0x00, 348, 1 , 0, F(RW)                                 , 0                          },
  { Enc(X86Bt)              , 0  , 0  , 0x00, 0x3B, 143, 3 , 0, F(RO)                                 , O(000F00,BA,4,_,x,_,_,_  ) },
  { Enc(X86Bt)              , 0  , 0  , 0x00, 0x3B, 146, 3 , 0, F(RW)|F(Lock)                         , O(000F00,BA,7,_,x,_,_,_  ) },
  { Enc(X86Bt)              , 0  , 0  , 0x00, 0x3B, 146, 3 , 0, F(RW)|F(Lock)                         , O(000F00,BA,6,_,x,_,_,_  ) },
  { Enc(X86Bt)              , 0  , 0  , 0x00, 0x3B, 146, 3 , 0, F(RW)|F(Lock)                         , O(000F00,BA,5,_,x,_,_,_  ) },
  { Enc(X86Call)            , 0  , 0  , 0x00, 0x00, 249, 2 , 0, F(RW)|F(Flow)|F(Volatile)             , O(000000,E8,_,_,_,_,_,_  ) },
  { Enc(X86OpAx)            , 0  , 0  , 0x00, 0x00, 349, 1 , 0, F(RW)|F(Special)                      , 0                          },
  { Enc(X86OpDxAx)          , 0  , 0  , 0x00, 0x00, 350, 1 , 0, F(RW)|F(Special)                      , 0                          },
  { Enc(X86OpAx)            , 0  , 0  , 0x00, 0x00, 351, 1 , 0, F(RW)|F(Special)                      , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x08, 259, 1 , 0, F(Volatile)                           , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x20, 259, 1 , 0, F(Volatile)                           , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x40, 259, 1 , 0, F(Volatile)                           , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0x00, 0x00, 352, 1 , 0, F(RO)|F(Volatile)                     , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x00, 353, 1 , 0, F(WO)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x20, 0x20, 259, 1 , 0, 0                                     , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0x24, 0x00, 20 , 3 , 0, F(RW)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0x20, 0x00, 20 , 3 , 0, F(RW)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0x04, 0x00, 20 , 3 , 0, F(RW)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0x07, 0x00, 20 , 3 , 0, F(RW)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0x03, 0x00, 20 , 3 , 0, F(RW)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0x01, 0x00, 20 , 3 , 0, F(RW)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0x10, 0x00, 20 , 3 , 0, F(RW)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0x02, 0x00, 20 , 3 , 0, F(RW)                                 , 0                          },
  { Enc(X86Arith)           , 0  , 0  , 0x00, 0x3F, 23 , 10, 0, F(RO)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x40, 0x3F, 0  , 0 , 0, F(RW)|F(Volatile)|F(Special)          , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 0x00, 0x00, 251, 2 , 0, F(RW)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 0x00, 0x00, 354, 1 , 0, F(RW)                                 , 0                          },
  { Enc(X86Cmpxchg)         , 0  , 0  , 0x00, 0x3F, 107, 4 , 0, F(RW)|F(Lock)|F(Special)              , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0x00, 0x04, 355, 1 , 0, F(RW)|F(Lock)|F(Special)              , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0x00, 0x04, 356, 1 , 0, F(RW)|F(Lock)|F(Special)              , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 0x00, 0x3F, 357, 1 , 0, F(RO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 0  , 0x00, 0x3F, 358, 1 , 0, F(RO)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x00, 359, 1 , 0, F(RW)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86OpDxAx)          , 0  , 0  , 0x00, 0x00, 360, 1 , 0, F(RW)|F(Special)                      , 0                          },
  { Enc(X86Crc)             , 0  , 0  , 0x00, 0x00, 253, 2 , 0, F(RW)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 16 , 0x00, 0x00, 61 , 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 16 , 0x00, 0x00, 63 , 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 8  , 0x00, 0x00, 361, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 16 , 0x00, 0x00, 362, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 8  , 0x00, 0x00, 362, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 8  , 0x00, 0x00, 363, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRm_Wx)           , 0  , 8  , 0x00, 0x00, 364, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 4  , 0x00, 0x00, 61 , 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRm_Wx)           , 0  , 8  , 0x00, 0x00, 365, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRm_Wx)           , 0  , 4  , 0x00, 0x00, 365, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 8  , 0x00, 0x00, 227, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRm_Wx)           , 0  , 8  , 0x00, 0x00, 309, 1 , 0, F(WO)                                 , 0                          },
  { Enc(X86OpDxAx)          , 0  , 0  , 0x00, 0x00, 366, 1 , 0, F(RW)|F(Special)                      , 0                          },
  { Enc(X86OpAx)            , 0  , 0  , 0x00, 0x00, 367, 1 , 0, F(RW)|F(Special)                      , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x28, 0x3F, 368, 1 , 0, F(RW)|F(Special)                      , 0                          },
  { Enc(X86IncDec)          , 0  , 0  , 0x00, 0x1F, 255, 2 , 0, F(RW)|F(Lock)                         , O(000000,48,_,_,x,_,_,_  ) },
  { Enc(X86M_Bx_MulDiv)     , 0  , 0  , 0x00, 0x3F, 111, 4 , 0, F(RW)|F(Special)                      , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x00, 259, 1 , 0, F(Volatile)                           , 0                          },
  { Enc(X86Enter)           , 0  , 0  , 0x00, 0x00, 369, 1 , 0, F(Volatile)|F(Special)                , 0                          },
  { Enc(ExtExtract)         , 0  , 8  , 0x00, 0x00, 370, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtExtrq)           , 0  , 0  , 0x00, 0x00, 257, 2 , 0, F(RW)                                 , O(660F00,78,0,_,_,_,_,_  ) },
  { Enc(FpuOp)              , 0  , 0  , 0x00, 0x00, 259, 1 , 0, F(Fp)                                 , 0                          },
  { Enc(FpuArith)           , 0  , 0  , 0x00, 0x00, 149, 3 , 0, F(Fp)|F(FPU_M4)|F(FPU_M8)             , 0                          },
  { Enc(FpuRDef)            , 0  , 0  , 0x00, 0x00, 259, 2 , 0, F(Fp)                                 , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0x00, 0x00, 371, 1 , 0, F(Fp)                                 , 0                          },
  { Enc(FpuR)               , 0  , 0  , 0x20, 0x00, 260, 1 , 0, F(Fp)                                 , 0                          },
  { Enc(FpuR)               , 0  , 0  , 0x24, 0x00, 260, 1 , 0, F(Fp)                                 , 0                          },
  { Enc(FpuR)               , 0  , 0  , 0x04, 0x00, 260, 1 , 0, F(Fp)                                 , 0                          },
  { Enc(FpuR)               , 0  , 0  , 0x10, 0x00, 260, 1 , 0, F(Fp)                                 , 0                          },
  { Enc(FpuCom)             , 0  , 0  , 0x00, 0x00, 261, 2 , 0, F(Fp)                                 , 0                          },
  { Enc(FpuR)               , 0  , 0  , 0x00, 0x3F, 260, 1 , 0, F(Fp)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x00, 259, 1 , 0, F(Fp)                                 , 0                          },
  { Enc(FpuR)               , 0  , 0  , 0x00, 0x00, 260, 1 , 0, F(Fp)                                 , 0                          },
  { Enc(FpuM)               , 0  , 0  , 0x00, 0x00, 372, 1 , 0, F(Fp)|F(FPU_M2)|F(FPU_M4)             , 0                          },
  { Enc(FpuM)               , 0  , 0  , 0x00, 0x00, 373, 1 , 0, F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , O_FPU(00,00DF,5)           },
  { Enc(FpuM)               , 0  , 0  , 0x00, 0x00, 373, 1 , 0, F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , O_FPU(00,00DF,7)           },
  { Enc(FpuM)               , 0  , 0  , 0x00, 0x00, 373, 1 , 0, F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , O_FPU(00,00DD,1)           },
  { Enc(FpuFldFst)          , 0  , 0  , 0x00, 0x00, 374, 1 , 0, F(Fp)|F(FPU_M2)|F(FPU_M4)|F(FPU_M8)   , O_FPU(00,00DB,5)           },
  { Enc(X86M_Only)          , 0  , 0  , 0x00, 0x00, 375, 1 , 0, F(Fp)                                 , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0x00, 0x00, 376, 1 , 0, F(Fp)                                 , 0                          },
  { Enc(FpuStsw)            , 0  , 0  , 0x00, 0x00, 377, 1 , 0, F(Fp)                                 , O_FPU(00,DFE0,_)           },
  { Enc(FpuFldFst)          , 0  , 0  , 0x00, 0x00, 262, 1 , 0, F(Fp)|F(FPU_M4)|F(FPU_M8)             , 0                          },
  { Enc(FpuFldFst)          , 0  , 0  , 0x00, 0x00, 374, 1 , 0, F(Fp)|F(FPU_M4)|F(FPU_M8)|F(FPU_M10)  , O(000000,DB,7,_,_,_,_,_  ) },
  { Enc(FpuStsw)            , 0  , 0  , 0x00, 0x00, 377, 1 , 0, F(Fp)                                 , O_FPU(9B,DFE0,_)           },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x00, 259, 1 , 0, F(Fp)|F(Volatile)                     , 0                          },
  { Enc(FpuR)               , 0  , 0  , 0x00, 0x00, 259, 2 , 0, F(Fp)                                 , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0x00, 0x00, 378, 1 , 0, F(Fp)                                 , 0                          },
  { Enc(X86M_Bx_MulDiv)     , 0  , 0  , 0x00, 0x3F, 115, 4 , 0, F(RW)|F(Special)                      , 0                          },
  { Enc(X86Imul)            , 0  , 0  , 0x00, 0x3F, 33 , 10, 0, F(RW)|F(Special)                      , 0                          },
  { Enc(X86IncDec)          , 0  , 0  , 0x00, 0x1F, 255, 2 , 0, F(RW)|F(Lock)                         , O(000000,40,_,_,x,_,_,_  ) },
  { Enc(ExtInsertq)         , 0  , 0  , 0x00, 0x00, 263, 2 , 0, F(RW)                                 , O(F20F00,78,_,_,_,_,_,_  ) },
  { Enc(X86Int)             , 0  , 0  , 0x00, 0x88, 379, 1 , 0, F(Volatile)                           , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x88, 259, 1 , 0, F(Volatile)                           , 0                          },
  { Enc(X86Jcc)             , 0  , 0  , 0x24, 0x00, 380, 1 , 0, F(Flow)|F(Volatile)                   , 0                          },
  { Enc(X86Jcc)             , 0  , 0  , 0x20, 0x00, 380, 1 , 0, F(Flow)|F(Volatile)                   , 0                          },
  { Enc(X86Jcc)             , 0  , 0  , 0x20, 0x00, 381, 1 , 0, F(Flow)|F(Volatile)                   , 0                          },
  { Enc(X86Jcc)             , 0  , 0  , 0x04, 0x00, 380, 1 , 0, F(Flow)|F(Volatile)                   , 0                          },
  { Enc(X86Jcc)             , 0  , 0  , 0x07, 0x00, 380, 1 , 0, F(Flow)|F(Volatile)                   , 0                          },
  { Enc(X86Jcc)             , 0  , 0  , 0x03, 0x00, 380, 1 , 0, F(Flow)|F(Volatile)                   , 0                          },
  { Enc(X86Jcc)             , 0  , 0  , 0x01, 0x00, 380, 1 , 0, F(Flow)|F(Volatile)                   , 0                          },
  { Enc(X86Jcc)             , 0  , 0  , 0x10, 0x00, 380, 1 , 0, F(Flow)|F(Volatile)                   , 0                          },
  { Enc(X86Jcc)             , 0  , 0  , 0x02, 0x00, 380, 1 , 0, F(Flow)|F(Volatile)                   , 0                          },
  { Enc(X86Jecxz)           , 0  , 0  , 0x00, 0x00, 265, 2 , 0, F(Flow)|F(Volatile)|F(Special)        , 0                          },
  { Enc(X86Jmp)             , 0  , 0  , 0x00, 0x00, 267, 2 , 0, F(Flow)|F(Volatile)                   , O(000000,E9,_,_,_,_,_,_  ) },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 382, 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexKmov)            , 0  , 0  , 0x00, 0x00, 269, 2 , 0, F(WO)|F(Vex)                          , V(660F00,92,_,0,0,_,_,_  ) },
  { Enc(VexKmov)            , 0  , 0  , 0x00, 0x00, 271, 2 , 0, F(WO)|F(Vex)                          , V(F20F00,92,_,0,0,_,_,_  ) },
  { Enc(VexKmov)            , 0  , 0  , 0x00, 0x00, 273, 2 , 0, F(WO)|F(Vex)                          , V(F20F00,92,_,0,1,_,_,_  ) },
  { Enc(VexKmov)            , 0  , 0  , 0x00, 0x00, 275, 2 , 0, F(WO)|F(Vex)                          , V(000F00,92,_,0,0,_,_,_  ) },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 383, 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x3F, 384, 1 , 0, F(RO)|F(Vex)                          , 0                          },
  { Enc(VexRmi)             , 0  , 0  , 0x00, 0x00, 385, 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x3E, 0x00, 386, 1 , 0, F(RW)|F(Volatile)|F(Special)          , 0                          },
  { Enc(ExtRm)              , 0  , 16 , 0x00, 0x00, 200, 1 , 0, F(WO)                                 , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0x00, 0x00, 387, 1 , 0, F(RO)|F(Volatile)                     , 0                          },
  { Enc(X86Lea)             , 0  , 0  , 0x00, 0x00, 388, 1 , 0, F(WO)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x00, 259, 1 , 0, F(Volatile)|F(Special)                , 0                          },
  { Enc(X86Fence)           , 0  , 0  , 0x00, 0x00, 259, 1 , 0, F(Volatile)                           , 0                          },
  { Enc(X86Op)              , 0  , 1  , 0x40, 0x00, 0  , 0 , 0, F(WO)|F(Special)                      , 0                          },
  { Enc(X86Op)              , 0  , 4  , 0x40, 0x00, 0  , 0 , 0, F(WO)|F(Special)                      , 0                          },
  { Enc(X86Op)              , 0  , 8  , 0x40, 0x00, 0  , 0 , 0, F(WO)|F(Special)                      , 0                          },
  { Enc(X86Op)              , 0  , 2  , 0x40, 0x00, 0  , 0 , 0, F(WO)|F(Special)                      , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0x00, 0x3F, 152, 3 , 0, F(RW)                                 , 0                          },
  { Enc(ExtRmZDI)           , 0  , 0  , 0x00, 0x00, 389, 1 , 0, F(RO)|F(Special)                      , 0                          },
  { Enc(ExtRmZDI)           , 0  , 0  , 0x00, 0x00, 390, 1 , 0, F(RO)|F(Special)                      , 0                          },
  { Enc(X86Fence)           , 0  , 0  , 0x00, 0x00, 259, 1 , 0, F(RW)|F(Volatile)                     , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x00, 0  , 0 , 0, F(RO)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Mov)             , 0  , 0  , 0x00, 0x00, 0  , 13, 0, F(WO)                                 , 0                          },
  { Enc(ExtMov)             , 0  , 16 , 0x00, 0x00, 63 , 2 , 0, F(WO)                                 , O(660F00,29,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 16 , 0x00, 0x00, 63 , 2 , 0, F(WO)                                 , O(000F00,29,_,_,_,_,_,_  ) },
  { Enc(ExtMovbe)           , 0  , 0  , 0x00, 0x00, 51 , 6 , 0, F(WO)                                 , O(000F38,F1,_,_,x,_,_,_  ) },
  { Enc(ExtMovd)            , 0  , 16 , 0x00, 0x00, 277, 2 , 0, F(WO)                                 , O(000F00,7E,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 16 , 0x00, 0x00, 61 , 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtMov)             , 0  , 8  , 0x00, 0x00, 391, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtMov)             , 0  , 16 , 0x00, 0x00, 63 , 2 , 0, F(WO)                                 , O(660F00,7F,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 16 , 0x00, 0x00, 63 , 2 , 0, F(WO)                                 , O(F30F00,7F,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 8  , 0x00, 0x00, 392, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtMov)             , 8  , 8  , 0x00, 0x00, 206, 2 , 0, F(RW)                                 , O(660F00,17,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 8  , 8  , 0x00, 0x00, 206, 2 , 0, F(RW)                                 , O(000F00,17,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 8  , 8  , 0x00, 0x00, 392, 1 , 0, F(RW)                                 , 0                          },
  { Enc(ExtMov)             , 0  , 8  , 0x00, 0x00, 206, 2 , 0, F(WO)                                 , O(660F00,13,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 8  , 0x00, 0x00, 206, 2 , 0, F(WO)                                 , O(000F00,13,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 8  , 0x00, 0x00, 393, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtMov)             , 0  , 16 , 0x00, 0x00, 197, 1 , 0, F(WO)                                 , O(660F00,E7,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 16 , 0x00, 0x00, 200, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtMovnti)          , 0  , 8  , 0x00, 0x00, 55 , 2 , 0, F(WO)                                 , 0                          },
  { Enc(ExtMov)             , 0  , 16 , 0x00, 0x00, 197, 1 , 0, F(WO)                                 , O(660F00,2B,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 16 , 0x00, 0x00, 197, 1 , 0, F(WO)                                 , O(000F00,2B,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 8  , 0x00, 0x00, 394, 1 , 0, F(WO)                                 , O(000F00,E7,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 8  , 0x00, 0x00, 157, 1 , 0, F(WO)                                 , O(F20F00,2B,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 4  , 0x00, 0x00, 280, 1 , 0, F(WO)                                 , O(F30F00,2B,_,_,_,_,_,_  ) },
  { Enc(ExtMovq)            , 0  , 16 , 0x00, 0x00, 57 , 6 , 0, F(WO)                                 , O(000F00,7E,_,_,x,_,_,_  ) },
  { Enc(ExtRm)              , 0  , 16 , 0x00, 0x00, 395, 1 , 0, F(WO)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x00, 0  , 0 , 0, F(WO)|F(Special)                      , 0                          },
  { Enc(ExtMov)             , 0  , 8  , 0x00, 0x00, 155, 3 , 0, F(WO)|F(ZeroIfMem)                    , O(F20F00,11,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 4  , 0x00, 0x00, 279, 2 , 0, F(WO)|F(ZeroIfMem)                    , O(F30F00,11,_,_,_,_,_,_  ) },
  { Enc(X86MovsxMovzx)      , 0  , 0  , 0x00, 0x00, 281, 2 , 0, F(WO)                                 , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0x00, 0x00, 396, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtMov)             , 0  , 16 , 0x00, 0x00, 63 , 2 , 0, F(WO)                                 , O(660F00,11,_,_,_,_,_,_  ) },
  { Enc(ExtMov)             , 0  , 16 , 0x00, 0x00, 63 , 2 , 0, F(WO)                                 , O(000F00,11,_,_,_,_,_,_  ) },
  { Enc(X86M_Bx_MulDiv)     , 0  , 0  , 0x00, 0x3F, 33 , 4 , 0, F(RW)|F(Special)                      , 0                          },
  { Enc(VexRvmZDX_Wx)       , 0  , 0  , 0x00, 0x00, 283, 2 , 0, F(RW)|F(Special)                      , 0                          },
  { Enc(X86M_Bx)            , 0  , 0  , 0x00, 0x3F, 256, 1 , 0, F(RW)|F(Lock)                         , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x00, 285, 2 , 0, 0                                     , 0                          },
  { Enc(X86M_Bx)            , 0  , 0  , 0x00, 0x00, 256, 1 , 0, F(RW)|F(Lock)                         , 0                          },
  { Enc(ExtRm_P)            , 0  , 0  , 0x00, 0x00, 287, 2 , 0, F(RW)                                 , 0                          },
  { Enc(ExtRmi_P)           , 0  , 0  , 0x00, 0x00, 289, 2 , 0, F(RW)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x00, 259, 1 , 0, F(RW)                                 , 0                          },
  { Enc(Ext3dNow)           , 0  , 0  , 0x00, 0x00, 287, 1 , 0, F(RW)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 0x00, 0x00, 397, 1 , 0, F(WO)|F(Special)                      , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 0x00, 0x00, 398, 1 , 0, F(WO)|F(Special)                      , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 0x00, 0x00, 399, 1 , 0, F(WO)|F(Special)                      , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 0x00, 0x00, 400, 1 , 0, F(WO)|F(Special)                      , 0                          },
  { Enc(X86Op_O)            , 0  , 0  , 0x00, 0x00, 259, 1 , 0, F(Volatile)                           , 0                          },
  { Enc(VexRvm_Wx)          , 0  , 0  , 0x00, 0x00, 245, 2 , 0, F(WO)                                 , 0                          },
  { Enc(ExtExtract)         , 0  , 8  , 0x00, 0x00, 401, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtExtract)         , 0  , 8  , 0x00, 0x00, 402, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtPextrw)          , 0  , 8  , 0x00, 0x00, 291, 2 , 0, F(WO)                                 , O(000F3A,15,_,_,_,_,_,_  ) },
  { Enc(Ext3dNow)           , 0  , 8  , 0x00, 0x00, 295, 1 , 0, F(WO)                                 , 0                          },
  { Enc(Ext3dNow)           , 0  , 0  , 0x00, 0x00, 295, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 0x00, 0x00, 403, 1 , 0, F(RW)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 0x00, 0x00, 404, 1 , 0, F(RW)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 0  , 0x00, 0x00, 405, 1 , 0, F(RW)                                 , 0                          },
  { Enc(ExtRmi_P)           , 0  , 0  , 0x00, 0x00, 406, 1 , 0, F(RW)                                 , 0                          },
  { Enc(ExtRm_P)            , 0  , 8  , 0x00, 0x00, 407, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 16 , 0x00, 0x00, 227, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRm)              , 0  , 16 , 0x00, 0x00, 230, 1 , 0, F(WO)                                 , 0                          },
  { Enc(X86Pop)             , 0  , 0  , 0x00, 0x00, 293, 2 , 0, F(WO)|F(Volatile)|F(Special)          , O(000000,58,_,_,_,_,_,_  ) },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x00, 408, 1 , 0, F(Volatile)|F(Special)                , 0                          },
  { Enc(X86Rm)              , 0  , 0  , 0x00, 0x3F, 152, 3 , 0, F(WO)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0xFF, 259, 1 , 0, F(Volatile)|F(Special)                , 0                          },
  { Enc(X86Prefetch)        , 0  , 0  , 0x00, 0x00, 0  , 0 , 0, F(RO)|F(Volatile)                     , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0x00, 0x3F, 352, 1 , 0, F(RO)|F(Volatile)                     , 0                          },
  { Enc(ExtRm_P)            , 0  , 0  , 0x00, 0x00, 295, 2 , 0, F(RW)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 16 , 0x00, 0x00, 70 , 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRmi_P)           , 0  , 8  , 0x00, 0x00, 409, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRmRi_P)          , 0  , 0  , 0x00, 0x00, 297, 2 , 0, F(RW)                                 , O(000F00,72,6,_,_,_,_,_  ) },
  { Enc(ExtRmRi)            , 0  , 0  , 0x00, 0x00, 410, 1 , 0, F(RW)                                 , O(660F00,73,7,_,_,_,_,_  ) },
  { Enc(ExtRmRi_P)          , 0  , 0  , 0x00, 0x00, 297, 2 , 0, F(RW)                                 , O(000F00,73,6,_,_,_,_,_  ) },
  { Enc(ExtRmRi_P)          , 0  , 0  , 0x00, 0x00, 297, 2 , 0, F(RW)                                 , O(000F00,71,6,_,_,_,_,_  ) },
  { Enc(ExtRmRi_P)          , 0  , 0  , 0x00, 0x00, 297, 2 , 0, F(RW)                                 , O(000F00,72,4,_,_,_,_,_  ) },
  { Enc(ExtRmRi_P)          , 0  , 0  , 0x00, 0x00, 297, 2 , 0, F(RW)                                 , O(000F00,71,4,_,_,_,_,_  ) },
  { Enc(ExtRmRi_P)          , 0  , 0  , 0x00, 0x00, 297, 2 , 0, F(RW)                                 , O(000F00,72,2,_,_,_,_,_  ) },
  { Enc(ExtRmRi)            , 0  , 0  , 0x00, 0x00, 410, 1 , 0, F(RW)                                 , O(660F00,73,3,_,_,_,_,_  ) },
  { Enc(ExtRmRi_P)          , 0  , 0  , 0x00, 0x00, 297, 2 , 0, F(RW)                                 , O(000F00,73,2,_,_,_,_,_  ) },
  { Enc(ExtRmRi_P)          , 0  , 0  , 0x00, 0x00, 297, 2 , 0, F(RW)                                 , O(000F00,71,2,_,_,_,_,_  ) },
  { Enc(ExtRm)              , 0  , 0  , 0x00, 0x3F, 341, 1 , 0, F(RO)                                 , 0                          },
  { Enc(X86Push)            , 0  , 0  , 0x00, 0x00, 299, 2 , 0, F(RO)|F(Volatile)|F(Special)          , O(000000,50,_,_,_,_,_,_  ) },
  { Enc(X86Op)              , 0  , 0  , 0xFF, 0x00, 259, 1 , 0, F(Volatile)|F(Special)                , 0                          },
  { Enc(X86Rot)             , 0  , 0  , 0x20, 0x21, 411, 1 , 0, F(RW)|F(Special)                      , 0                          },
  { Enc(ExtRm)              , 0  , 4  , 0x00, 0x00, 227, 1 , 0, F(WO)                                 , 0                          },
  { Enc(X86M)               , 0  , 8  , 0x00, 0x00, 412, 1 , 0, F(WO)                                 , 0                          },
  { Enc(X86M)               , 0  , 8  , 0x00, 0x3F, 413, 1 , 0, F(WO)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x00, 414, 1 , 0, F(WO)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x00, 415, 1 , 0, F(WO)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Rep)             , 0  , 0  , 0x40, 0x00, 0  , 0 , 0, F(RW)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Rep)             , 0  , 0  , 0x40, 0x3F, 0  , 0 , 0, F(RW)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Ret)             , 0  , 0  , 0x00, 0x00, 301, 2 , 0, F(RW)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Rot)             , 0  , 0  , 0x00, 0x21, 411, 1 , 0, F(RW)|F(Special)                      , 0                          },
  { Enc(VexRmi_Wx)          , 0  , 0  , 0x00, 0x00, 303, 2 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 8  , 0x00, 0x00, 416, 1 , 0, F(WO)                                 , 0                          },
  { Enc(ExtRmi)             , 0  , 4  , 0x00, 0x00, 417, 1 , 0, F(WO)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x3E, 418, 1 , 0, F(RO)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Rot)             , 0  , 0  , 0x00, 0x3F, 411, 1 , 0, F(RW)|F(Special)                      , 0                          },
  { Enc(VexRmv_Wx)          , 0  , 0  , 0x00, 0x00, 247, 2 , 0, F(WO)                                 , 0                          },
  { Enc(X86Set)             , 0  , 1  , 0x24, 0x00, 419, 1 , 0, F(WO)                                 , 0                          },
  { Enc(X86Set)             , 0  , 1  , 0x20, 0x00, 419, 1 , 0, F(WO)                                 , 0                          },
  { Enc(X86Set)             , 0  , 1  , 0x04, 0x00, 419, 1 , 0, F(WO)                                 , 0                          },
  { Enc(X86Set)             , 0  , 1  , 0x07, 0x00, 419, 1 , 0, F(WO)                                 , 0                          },
  { Enc(X86Set)             , 0  , 1  , 0x03, 0x00, 419, 1 , 0, F(WO)                                 , 0                          },
  { Enc(X86Set)             , 0  , 1  , 0x01, 0x00, 419, 1 , 0, F(WO)                                 , 0                          },
  { Enc(X86Set)             , 0  , 1  , 0x10, 0x00, 419, 1 , 0, F(WO)                                 , 0                          },
  { Enc(X86Set)             , 0  , 1  , 0x02, 0x00, 419, 1 , 0, F(WO)                                 , 0                          },
  { Enc(X86ShldShrd)        , 0  , 0  , 0x00, 0x3F, 158, 3 , 0, F(RW)|F(Special)                      , 0                          },
  { Enc(ExtRm)              , 0  , 8  , 0x00, 0x00, 61 , 1 , 0, F(WO)                                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x20, 259, 1 , 0, 0                                     , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x40, 259, 1 , 0, 0                                     , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x80, 259, 1 , 0, 0                                     , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0x00, 0x00, 420, 1 , 0, F(Volatile)                           , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x40, 0x00, 0  , 0 , 0, F(RW)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Test)            , 0  , 0  , 0x00, 0x3F, 87 , 5 , 0, F(RO)                                 , O(000000,F6,_,_,x,_,_,_  ) },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x00, 259, 1 , 0, 0                                     , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 421, 1 , 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 422, 1 , 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 2 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 69 , 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 63 , 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRmi)             , 0  , 0  , 0x00, 0x00, 70 , 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 164, 3 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,4) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 164, 3 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,8) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,8) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)|F(Vex)   |A512(DQ  ,1,KZ,0  ,4) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)          |A512(BW  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,4) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,8) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 164, 2 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmr_Lx)         , 0  , 0  , 0x00, 0x00, 305, 2 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 423, 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 424, 1 , 0, F(WO)          |A512(DQ  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 425, 1 , 0, F(WO)          |A512(F_  ,0,KZ,0  ,0) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 426, 1 , 0, F(WO)          |A512(DQ  ,0,KZ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 425, 1 , 0, F(WO)          |A512(DQ  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 426, 1 , 0, F(WO)          |A512(F_  ,0,KZ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 427, 1 , 0, F(WO)          |A512(DQ  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 424, 1 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 232, 1 , 0, F(WO)          |A512(DQ  ,0,KZ,0  ,0) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 232, 1 , 0, F(WO)          |A512(F_  ,0,KZ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 424, 1 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 325, 1 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 167, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,8) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 167, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,4) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 428, 1 , 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,SAE,0) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 429, 1 , 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,SAE,0) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x3F, 357, 1 , 0, F(RO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x3F, 358, 1 , 0, F(RO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 0x00, 0x00, 170, 3 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 173, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 176, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 307, 2 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 179, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 176, 3 , 0, F(WO)          |A512(DQ  ,1,KZ,ER ,8) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 307, 2 , 0, F(WO)          |A512(F_  ,1,KZ,ER ,8) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 173, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 173, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , 0                          },
  { Enc(VexMri_Lx)          , 0  , 0  , 0x00, 0x00, 182, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 173, 3 , 0, F(WO)          |A512(DQ  ,1,KZ,ER ,4) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 176, 3 , 0, F(WO)          |A512(F_  ,1,KZ,ER ,4) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 307, 2 , 0, F(WO)          |A512(DQ  ,1,KZ,ER ,8) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 364, 1 , 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,ER ,0) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 364, 1 , 0, F(WO)          |A512(F_  ,0,0 ,ER ,0) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 430, 1 , 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,ER ,0) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 422, 1 , 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,SAE,0) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 309, 2 , 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,ER ,0) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 311, 2 , 0, F(WO)          |A512(F_  ,0,0 ,ER ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 307, 2 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,8) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 176, 3 , 0, F(WO)          |A512(F_  ,1,KZ,SAE,8) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 307, 2 , 0, F(WO)          |A512(F_  ,1,KZ,SAE,8) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 176, 3 , 0, F(WO)          |A512(DQ  ,1,KZ,SAE,8) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 176, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,4) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 173, 3 , 0, F(WO)          |A512(DQ  ,1,KZ,SAE,4) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 176, 3 , 0, F(WO)          |A512(F_  ,1,KZ,SAE,4) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 364, 1 , 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 364, 1 , 0, F(WO)          |A512(F_  ,0,0 ,SAE,0) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 309, 2 , 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,SAE,0) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 311, 2 , 0, F(WO)          |A512(F_  ,0,0 ,SAE,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 173, 3 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,4) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 430, 1 , 0, F(WO)          |A512(F_  ,0,0 ,ER ,0) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 164, 3 , 0, F(WO)          |A512(BW  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 67 , 1 , 0, F(WO)          |A512(ERI ,0,KZ,SAE,8) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 67 , 1 , 0, F(WO)          |A512(ERI ,0,KZ,SAE,4) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 176, 3 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexMri)             , 0  , 0  , 0x00, 0x00, 183, 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexMri_Lx)          , 0  , 0  , 0x00, 0x00, 431, 1 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexMri)             , 0  , 0  , 0x00, 0x00, 184, 1 , 0, F(WO)          |A512(DQ  ,0,KZ,0  ,0) , 0                          },
  { Enc(VexMri_Lx)          , 0  , 0  , 0x00, 0x00, 431, 1 , 0, F(WO)          |A512(DQ  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexMri)             , 0  , 0  , 0x00, 0x00, 184, 1 , 0, F(WO)          |A512(F_  ,0,KZ,0  ,0) , 0                          },
  { Enc(VexMri)             , 0  , 0  , 0x00, 0x00, 370, 1 , 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 185, 3 , 0, F(RW)          |A512(F_  ,1,KZ,SAE,8) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 185, 3 , 0, F(RW)          |A512(F_  ,1,KZ,SAE,4) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 432, 1 , 0, F(RW)          |A512(F_  ,0,KZ,SAE,0) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 433, 1 , 0, F(RW)          |A512(F_  ,0,KZ,SAE,0) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 188, 3 , 0, F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 188, 3 , 0, F(RW)|F(Vex)   |A512(F_  ,1,KZ,ER ,4) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 434, 1 , 0, F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 435, 1 , 0, F(RW)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , 0                          },
  { Enc(Fma4_Lx)            , 0  , 0  , 0x00, 0x00, 119, 4 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(Fma4)               , 0  , 0  , 0x00, 0x00, 313, 2 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(Fma4)               , 0  , 0  , 0x00, 0x00, 315, 2 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 0x00, 0x00, 436, 1 , 0, F(WO)          |A512(DQ  ,1,K_,0  ,8) , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 0x00, 0x00, 436, 1 , 0, F(WO)          |A512(DQ  ,1,K_,0  ,4) , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 0x00, 0x00, 437, 1 , 0, F(WO)          |A512(DQ  ,0,K_,0  ,0) , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 0x00, 0x00, 438, 1 , 0, F(WO)          |A512(DQ  ,0,K_,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 176, 2 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 61 , 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 227, 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRmvRm_VM)        , 0  , 0  , 0x00, 0x00, 92 , 5 , 0, F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , V(660F38,92,_,x,_,1,3,T1S) },
  { Enc(VexRmvRm_VM)        , 0  , 0  , 0x00, 0x00, 97 , 5 , 0, F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , V(660F38,92,_,x,_,0,2,T1S) },
  { Enc(VexM_VM)            , 0  , 0  , 0x00, 0x00, 439, 1 , 0, F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , 0                          },
  { Enc(VexM_VM)            , 0  , 0  , 0x00, 0x00, 440, 1 , 0, F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , 0                          },
  { Enc(VexM_VM)            , 0  , 0  , 0x00, 0x00, 441, 1 , 0, F(RO)|F(VM)    |A512(PFI ,0,K_,0  ,0) , 0                          },
  { Enc(VexRmvRm_VM)        , 0  , 0  , 0x00, 0x00, 102, 5 , 0, F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , V(660F38,93,_,x,_,1,3,T1S) },
  { Enc(VexRmvRm_VM)        , 0  , 0  , 0x00, 0x00, 123, 4 , 0, F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , V(660F38,93,_,x,_,0,2,T1S) },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 61 , 1 , 0, F(WO)          |A512(F_  ,0,KZ,SAE,0) , 0                          },
  { Enc(VexRm)              , 0  , 0  , 0x00, 0x00, 227, 1 , 0, F(WO)          |A512(F_  ,0,KZ,SAE,0) , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 0x00, 0x00, 191, 3 , 0, F(WO)          |A512(F_  ,1,KZ,SAE,8) , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 0x00, 0x00, 191, 3 , 0, F(WO)          |A512(F_  ,1,KZ,SAE,4) , 0                          },
  { Enc(VexRmi)             , 0  , 0  , 0x00, 0x00, 416, 1 , 0, F(WO)          |A512(F_  ,0,KZ,SAE,0) , 0                          },
  { Enc(VexRmi)             , 0  , 0  , 0x00, 0x00, 417, 1 , 0, F(WO)          |A512(F_  ,0,KZ,SAE,0) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 317, 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 317, 2 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 442, 1 , 0, F(WO)          |A512(DQ  ,0,KZ,0  ,0) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 317, 2 , 0, F(WO)          |A512(DQ  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 442, 1 , 0, F(WO)          |A512(F_  ,0,KZ,0  ,0) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 443, 1 , 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 200, 2 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexM)               , 0  , 0  , 0x00, 0x00, 387, 1 , 0, F(RO)|F(Vex)|F(Volatile)              , 0                          },
  { Enc(VexRmZDI)           , 0  , 0  , 0x00, 0x00, 444, 1 , 0, F(RO)|F(Vex)|F(Special)               , 0                          },
  { Enc(VexRvmMvr_Lx)       , 0  , 0  , 0x00, 0x00, 127, 4 , 0, F(RW)|F(Vex)                          , V(660F38,2F,_,x,0,_,_,_  ) },
  { Enc(VexRvmMvr_Lx)       , 0  , 0  , 0x00, 0x00, 127, 4 , 0, F(RW)|F(Vex)                          , V(660F38,2E,_,x,0,_,_,_  ) },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,8) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,4) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 421, 1 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 422, 1 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,SAE,0) , 0                          },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 0x00, 0x00, 63 , 6 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , V(660F00,29,_,x,I,1,4,FVM) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 0x00, 0x00, 63 , 6 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , V(000F00,29,_,x,I,0,4,FVM) },
  { Enc(VexMovDQ)           , 0  , 0  , 0x00, 0x00, 319, 2 , 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , V(660F00,7E,_,0,0,0,2,T1S) },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 194, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 0x00, 0x00, 63 , 4 , 0, F(WO)|F(Vex)                          , V(660F00,7F,_,x,I,_,_,_  ) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 0x00, 0x00, 63 , 6 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,0) , V(660F00,7F,_,x,_,0,4,FVM) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 0x00, 0x00, 63 , 6 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,0) , V(660F00,7F,_,x,_,1,4,FVM) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 0x00, 0x00, 63 , 4 , 0, F(WO)|F(Vex)                          , V(F30F00,7F,_,x,I,_,_,_  ) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 0x00, 0x00, 63 , 6 , 0, F(WO)          |A512(BW  ,1,KZ,0  ,0) , V(F20F00,7F,_,x,_,1,4,FVM) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 0x00, 0x00, 63 , 6 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,0) , V(F30F00,7F,_,x,_,0,4,FVM) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 0x00, 0x00, 63 , 6 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,0) , V(F30F00,7F,_,x,_,1,4,FVM) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 0x00, 0x00, 63 , 6 , 0, F(WO)          |A512(BW  ,1,KZ,0  ,0) , V(F20F00,7F,_,x,_,0,4,FVM) },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 208, 1 , 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , 0                          },
  { Enc(VexRvmMr)           , 0  , 0  , 0x00, 0x00, 321, 2 , 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , V(660F00,17,_,0,I,1,3,T1S) },
  { Enc(VexRvmMr)           , 0  , 0  , 0x00, 0x00, 321, 2 , 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , V(000F00,17,_,0,I,0,3,T2 ) },
  { Enc(VexRvmMr)           , 0  , 0  , 0x00, 0x00, 321, 2 , 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , V(660F00,13,_,0,I,1,3,T1S) },
  { Enc(VexRvmMr)           , 0  , 0  , 0x00, 0x00, 321, 2 , 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , V(000F00,13,_,0,I,0,3,T2 ) },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 445, 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 0x00, 0x00, 197, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 200, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,0 ,0  ,0) , 0                          },
  { Enc(VexMovDQ)           , 0  , 0  , 0x00, 0x00, 203, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,0,0 ,0  ,0) , V(660F00,7E,_,0,I,1,3,T1S) },
  { Enc(VexMovSsSd)         , 0  , 0  , 0x00, 0x00, 206, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,0  ,0) , V(F20F00,11,_,I,I,1,3,T1S) },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 176, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexMovSsSd)         , 0  , 0  , 0x00, 0x00, 209, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,0  ,0) , V(F30F00,11,_,I,I,0,2,T1S) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 0x00, 0x00, 63 , 6 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , V(660F00,11,_,x,I,1,4,FVM) },
  { Enc(VexRmMr_Lx)         , 0  , 0  , 0x00, 0x00, 63 , 6 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , V(000F00,11,_,x,I,0,4,FVM) },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 421, 1 , 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 422, 1 , 0, F(WO)|F(Vex)   |A512(F_  ,0,KZ,ER ,0) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 176, 3 , 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,4) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 164, 3 , 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRvmr)            , 0  , 0  , 0x00, 0x00, 305, 2 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 323, 2 , 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 325, 2 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 446, 1 , 0, F(WO)          |A512(CDI ,1,0 ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 447, 1 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 327, 2 , 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 164, 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvrmRvmr_Lx)     , 0  , 0  , 0x00, 0x00, 119, 4 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 212, 3 , 0, F(WO)          |A512(BW  ,1,K_,0  ,0) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 212, 3 , 0, F(WO)          |A512(F_  ,1,K_,0  ,4) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 215, 3 , 0, F(WO)|F(Vex)   |A512(BW  ,1,K_,0  ,0) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 215, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,K_,0  ,4) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 215, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,K_,0  ,8) , 0                          },
  { Enc(VexRmi)             , 0  , 0  , 0x00, 0x00, 397, 1 , 0, F(WO)|F(Vex)|F(Special)               , 0                          },
  { Enc(VexRmi)             , 0  , 0  , 0x00, 0x00, 398, 1 , 0, F(WO)|F(Vex)|F(Special)               , 0                          },
  { Enc(VexRmi)             , 0  , 0  , 0x00, 0x00, 399, 1 , 0, F(WO)|F(Vex)|F(Special)               , 0                          },
  { Enc(VexRmi)             , 0  , 0  , 0x00, 0x00, 400, 1 , 0, F(WO)|F(Vex)|F(Special)               , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 212, 3 , 0, F(WO)          |A512(F_  ,1,K_,0  ,8) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 212, 3 , 0, F(WO)          |A512(BW  ,1,K_,0  ,8) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 176, 3 , 0, F(WO)          |A512(CDI ,1,KZ,0  ,4) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 165, 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)          |A512(VBMI,1,KZ,0  ,0) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 136, 2 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 188, 3 , 0, F(RW)          |A512(F_  ,1,KZ,0  ,4) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 188, 3 , 0, F(RW)          |A512(F_  ,1,KZ,0  ,8) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 188, 3 , 0, F(RW)          |A512(BW  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRvrmiRvmri_Lx)   , 0  , 0  , 0x00, 0x00, 131, 4 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmRmi_Lx)       , 0  , 0  , 0x00, 0x00, 69 , 6 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , V(660F3A,05,_,x,0,1,4,FV ) },
  { Enc(VexRvmRmi_Lx)       , 0  , 0  , 0x00, 0x00, 69 , 6 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , V(660F3A,04,_,x,0,0,4,FV ) },
  { Enc(VexRmi)             , 0  , 0  , 0x00, 0x00, 72 , 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 71 , 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmRmi_Lx)       , 0  , 0  , 0x00, 0x00, 135, 4 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , V(660F3A,00,_,x,1,1,4,FV ) },
  { Enc(VexMri)             , 0  , 0  , 0x00, 0x00, 401, 1 , 0, F(WO)|F(Vex)   |A512(BW  ,0,0 ,0  ,0) , 0                          },
  { Enc(VexMri)             , 0  , 0  , 0x00, 0x00, 370, 1 , 0, F(WO)|F(Vex)   |A512(DQ  ,0,0 ,0  ,0) , 0                          },
  { Enc(VexMri)             , 0  , 0  , 0x00, 0x00, 402, 1 , 0, F(WO)|F(Vex)   |A512(DQ  ,0,0 ,0  ,0) , 0                          },
  { Enc(VexMri)             , 0  , 0  , 0x00, 0x00, 292, 1 , 0, F(WO)|F(Vex)   |A512(BW  ,0,0 ,0  ,0) , 0                          },
  { Enc(VexRmvRm_VM)        , 0  , 0  , 0x00, 0x00, 97 , 5 , 0, F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , V(660F38,90,_,x,_,0,2,T1S) },
  { Enc(VexRmvRm_VM)        , 0  , 0  , 0x00, 0x00, 92 , 5 , 0, F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , V(660F38,90,_,x,_,1,3,T1S) },
  { Enc(VexRmvRm_VM)        , 0  , 0  , 0x00, 0x00, 123, 4 , 0, F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , V(660F38,91,_,x,_,0,2,T1S) },
  { Enc(VexRmvRm_VM)        , 0  , 0  , 0x00, 0x00, 102, 5 , 0, F(RW)|F(Vex_VM)|A512(F_  ,1,K_,0  ,0) , V(660F38,91,_,x,_,1,3,T1S) },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 329, 2 , 0, F(WO)|F(Vex)   |A512(BW  ,0,KZ,0  ,0) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 331, 2 , 0, F(WO)|F(Vex)   |A512(DQ  ,0,KZ,0  ,0) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 333, 2 , 0, F(WO)|F(Vex)   |A512(DQ  ,0,KZ,0  ,0) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 448, 1 , 0, F(WO)|F(Vex)   |A512(BW  ,0,KZ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 176, 3 , 0, F(WO)          |A512(CDI ,1,KZ,0  ,8) , 0                          },
  { Enc(VexRvmr)            , 0  , 0  , 0x00, 0x00, 120, 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)          |A512(IFMA,1,KZ,0  ,8) , 0                          },
  { Enc(VexRvmMvr_Lx)       , 0  , 0  , 0x00, 0x00, 127, 4 , 0, F(WO)|F(Vex)                          , V(660F38,8E,_,x,0,_,_,_  ) },
  { Enc(VexRvmMvr_Lx)       , 0  , 0  , 0x00, 0x00, 127, 4 , 0, F(WO)|F(Vex)                          , V(660F38,8E,_,x,1,_,_,_  ) },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 449, 1 , 0, F(WO)          |A512(BW  ,1,0 ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 449, 1 , 0, F(WO)          |A512(DQ  ,1,0 ,0  ,0) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 0x00, 0x00, 218, 3 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 0x00, 0x00, 221, 3 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 446, 1 , 0, F(WO)          |A512(BW  ,1,0 ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 446, 1 , 0, F(WO)          |A512(DQ  ,1,0 ,0  ,0) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 0x00, 0x00, 224, 3 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 0x00, 0x00, 221, 3 , 0, F(WO)          |A512(BW  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 227, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 230, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 173, 3 , 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 233, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 173, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)          |A512(DQ  ,1,KZ,0  ,8) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)          |A512(VBMI,1,KZ,0  ,8) , 0                          },
  { Enc(VexRvrmRvmr)        , 0  , 0  , 0x00, 0x00, 119, 2 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexVmi_Lx)          , 0  , 0  , 0x00, 0x00, 191, 3 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,4) , 0                          },
  { Enc(VexVmi_Lx)          , 0  , 0  , 0x00, 0x00, 191, 3 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,8) , 0                          },
  { Enc(VexRvmRmvRmi)       , 0  , 0  , 0x00, 0x00, 335, 2 , 0, F(WO)|F(Vex)                          , V(XOP_M8,C0,_,0,x,_,_,_  ) },
  { Enc(VexRvmRmvRmi)       , 0  , 0  , 0x00, 0x00, 335, 2 , 0, F(WO)|F(Vex)                          , V(XOP_M8,C2,_,0,x,_,_,_  ) },
  { Enc(VexRvmRmvRmi)       , 0  , 0  , 0x00, 0x00, 335, 2 , 0, F(WO)|F(Vex)                          , V(XOP_M8,C3,_,0,x,_,_,_  ) },
  { Enc(VexRvmRmvRmi)       , 0  , 0  , 0x00, 0x00, 335, 2 , 0, F(WO)|F(Vex)                          , V(XOP_M8,C1,_,0,x,_,_,_  ) },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)|F(Vex)   |A512(BW  ,1,0 ,0  ,0) , 0                          },
  { Enc(VexMr_VM)           , 0  , 0  , 0x00, 0x00, 236, 3 , 0, F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , 0                          },
  { Enc(VexMr_VM)           , 0  , 0  , 0x00, 0x00, 337, 2 , 0, F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , 0                          },
  { Enc(VexMr_VM)           , 0  , 0  , 0x00, 0x00, 239, 3 , 0, F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , 0                          },
  { Enc(VexRvmRmv)          , 0  , 0  , 0x00, 0x00, 339, 2 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 0x00, 0x00, 191, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 0x00, 0x00, 191, 3 , 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , 0                          },
  { Enc(VexRvmVmi_Lx)       , 0  , 0  , 0x00, 0x00, 75 , 6 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , V(660F00,72,6,x,I,0,4,FV ) },
  { Enc(VexEvexVmi_Lx)      , 0  , 0  , 0x00, 0x00, 191, 3 , 0, F(WO)|F(Vex)   |A512(BW  ,1,0 ,0  ,0) , 0                          },
  { Enc(VexRvmVmi_Lx)       , 0  , 0  , 0x00, 0x00, 75 , 6 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , V(660F00,73,6,x,I,1,4,FV ) },
  { Enc(VexRvmVmi_Lx)       , 0  , 0  , 0x00, 0x00, 75 , 6 , 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , V(660F00,71,6,x,I,I,4,FVM) },
  { Enc(VexRvmVmi_Lx)       , 0  , 0  , 0x00, 0x00, 75 , 6 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , V(660F00,72,4,x,I,0,4,FV ) },
  { Enc(VexRvmVmi_Lx)       , 0  , 0  , 0x00, 0x00, 81 , 6 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,8) , V(660F00,72,4,x,_,1,4,FV ) },
  { Enc(VexRvmVmi_Lx)       , 0  , 0  , 0x00, 0x00, 75 , 6 , 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , V(660F00,71,4,x,I,I,4,FVM) },
  { Enc(VexRvmVmi_Lx)       , 0  , 0  , 0x00, 0x00, 75 , 6 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , V(660F00,72,2,x,I,0,4,FV ) },
  { Enc(VexRvmVmi_Lx)       , 0  , 0  , 0x00, 0x00, 75 , 6 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , V(660F00,73,2,x,I,1,4,FV ) },
  { Enc(VexRvmVmi_Lx)       , 0  , 0  , 0x00, 0x00, 75 , 6 , 0, F(WO)|F(Vex)   |A512(BW  ,1,KZ,0  ,0) , V(660F00,71,2,x,I,I,4,FVM) },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 185, 3 , 0, F(RW)          |A512(F_  ,1,KZ,0  ,4) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 185, 3 , 0, F(RW)          |A512(F_  ,1,KZ,0  ,8) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x3F, 341, 2 , 0, F(RO)|F(Vex)                          , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 242, 3 , 0, F(WO)          |A512(BW  ,1,K_,0  ,0) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 242, 3 , 0, F(WO)          |A512(F_  ,1,K_,0  ,4) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 242, 3 , 0, F(WO)          |A512(F_  ,1,K_,0  ,8) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 164, 3 , 0, F(WO)          |A512(DQ  ,1,KZ,SAE,8) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 164, 3 , 0, F(WO)          |A512(DQ  ,1,KZ,SAE,4) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 450, 1 , 0, F(WO)          |A512(DQ  ,0,KZ,SAE,0) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 443, 1 , 0, F(WO)          |A512(DQ  ,0,KZ,SAE,0) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 176, 3 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,8) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 176, 3 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,4) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 421, 1 , 0, F(WO)          |A512(F_  ,0,KZ,0  ,0) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 422, 1 , 0, F(WO)          |A512(F_  ,0,KZ,0  ,0) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 421, 1 , 0, F(WO)          |A512(ERI ,0,KZ,SAE,0) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 422, 1 , 0, F(WO)          |A512(ERI ,0,KZ,SAE,0) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 422, 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 0x00, 0x00, 191, 3 , 0, F(WO)          |A512(DQ  ,1,KZ,0  ,8) , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 0x00, 0x00, 191, 3 , 0, F(WO)          |A512(DQ  ,1,KZ,0  ,4) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 450, 1 , 0, F(WO)          |A512(DQ  ,0,KZ,0  ,0) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 443, 1 , 0, F(WO)          |A512(DQ  ,0,KZ,0  ,0) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 450, 1 , 0, F(WO)          |A512(F_  ,0,KZ,SAE,0) , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 443, 1 , 0, F(WO)          |A512(F_  ,0,KZ,SAE,0) , 0                          },
  { Enc(VexRmi_Lx)          , 0  , 0  , 0x00, 0x00, 77 , 2 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 450, 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvmi)            , 0  , 0  , 0x00, 0x00, 443, 1 , 0, F(WO)|F(Vex)                          , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)          |A512(F_  ,1,KZ,ER ,8) , 0                          },
  { Enc(VexRvm_Lx)          , 0  , 0  , 0x00, 0x00, 161, 3 , 0, F(WO)          |A512(F_  ,1,KZ,ER ,4) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 421, 1 , 0, F(WO)          |A512(F_  ,0,KZ,ER ,0) , 0                          },
  { Enc(VexRvm)             , 0  , 0  , 0x00, 0x00, 422, 1 , 0, F(WO)          |A512(F_  ,0,KZ,ER ,0) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 0x00, 0x00, 343, 2 , 0, F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 0x00, 0x00, 236, 3 , 0, F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 0x00, 0x00, 239, 3 , 0, F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , 0                          },
  { Enc(VexMr_Lx)           , 0  , 0  , 0x00, 0x00, 337, 2 , 0, F(WO)|F(VM)    |A512(F_  ,1,K_,0  ,0) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 165, 2 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,4) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 165, 2 , 0, F(WO)          |A512(F_  ,1,KZ,0  ,8) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 164, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,4) , 0                          },
  { Enc(VexRvmi_Lx)         , 0  , 0  , 0x00, 0x00, 164, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,0  ,8) , 0                          },
  { Enc(VexRm_Lx)           , 0  , 0  , 0x00, 0x00, 176, 3 , 0, F(WO)|F(Vex)   |A512(F_  ,1,KZ,ER ,8) , 0                          },
  { Enc(VexM)               , 0  , 0  , 0x00, 0x00, 420, 1 , 0, F(Vex)|F(Volatile)                    , 0                          },
  { Enc(VexOp)              , 0  , 0  , 0x00, 0x00, 259, 1 , 0, F(Vex)|F(Volatile)                    , 0                          },
  { Enc(X86M)               , 0  , 0  , 0x00, 0x00, 451, 1 , 0, F(RO)|F(Volatile)                     , 0                          },
  { Enc(X86Xadd)            , 0  , 0  , 0x00, 0x3F, 139, 4 , 0, F(RW)|F(Xchg)|F(Lock)                 , 0                          },
  { Enc(X86Xchg)            , 0  , 0  , 0x00, 0x00, 43 , 8 , 0, F(RW)|F(Xchg)|F(Lock)                 , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x00, 452, 1 , 0, F(WO)|F(Special)                      , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0x00, 0x00, 453, 1 , 0, F(RO)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0x00, 0x00, 454, 1 , 0, F(RO)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0x00, 0x00, 453, 1 , 0, F(WO)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86M_Only)          , 0  , 0  , 0x00, 0x00, 454, 1 , 0, F(WO)|F(Volatile)|F(Special)          , 0                          },
  { Enc(X86Op)              , 0  , 0  , 0x00, 0x00, 455, 1 , 0, F(RO)|F(Volatile)|F(Special)          , 0                          }
};
// ----------------------------------------------------------------------------
// ${X86InstExtendedData:End}

#undef INST
#undef A512
#undef NAME_INDEX

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

  const X86Inst* instData = X86InstDB::instData;
  const char* nameData = X86InstDB::nameData;

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
      if (X86Inst_compareName(nameData + base[0].getNameIndex(), name, len) == 0)
        return static_cast<uint32_t>((size_t)(base - instData));
      base++;
    }
  }
  else {
    for (size_t lim = (size_t)(end - base); lim != 0; lim >>= 1) {
      const X86Inst* cur = base + (lim >> 1);
      int result = X86Inst_compareName(nameData + cur[0].getNameIndex(), name, len);

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

  return DebugUtils::errored(kInvalidInst);
}

const char* X86Inst::getNameById(uint32_t id) noexcept {
  if (id >= X86Inst::_kIdCount) return nullptr;
  return X86InstDB::nameData + X86Inst::getInst(id).getNameIndex();
}
#else
const char X86InstDB::nameData[] = "";
#endif // !ASMJIT_DISABLE_TEXT

// ============================================================================
// [asmjit::X86Util - Validation]
// ============================================================================

#if !defined(ASMJIT_DISABLE_VALIDATION)
// ${X86InstSignatureData:Begin}
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
    tod.regId = static_cast<uint8_t>(regId);
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
  const X86Inst::ExtendedData* iExtData = iData->getExtDataPtr();
  const X86Inst::ISignature* iSig = _x86InstISignatureData + iExtData->_iSignatureIndex;
  const X86Inst::ISignature* iEnd = iSig                   + iExtData->_iSignatureCount;

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

      if (ASMJIT_UNLIKELY(!iExtData->hasFlag(kInstFlagEvexK_)))
        return DebugUtils::errored(kErrorInvalidKMaskUse);

      if (ASMJIT_UNLIKELY((options & X86Inst::kOptionKZ) != 0 && !iExtData->hasFlag(X86Inst::kInstFlagEvexKZ)))
        return DebugUtils::errored(kErrorInvalidKZeroUse);
    }

    // Validate AVX-512 broadcast {1tox}.
    if (options & X86Inst::kOption1ToX) {
      if (ASMJIT_UNLIKELY(!memOp))
        return DebugUtils::errored(kErrorInvalidBroadcast);

      uint32_t size = memOp->getSize();
      if (size != 0) {
        if (ASMJIT_UNLIKELY(iExtData->hasFlag(X86Inst::kInstFlagEvexB4) && size != 4))
          return DebugUtils::errored(kErrorInvalidBroadcast);

        if (ASMJIT_UNLIKELY(iExtData->hasFlag(X86Inst::kInstFlagEvexB8) && size != 8))
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
        if (ASMJIT_UNLIKELY(!(iExtData->getFlags() & X86Inst::kInstFlagEvexER)))
          return DebugUtils::errored(kErrorInvalidSAEOrER);

        // {er} is defined for scalar ops or vector ops using zmm (LL = 10). We
        // don't need any more bits in the instruction database to be able to
        // validate this, as each AVX512 instruction that has broadcast is vector
        // instruction (in this case we require zmm registers), otherwise it's a
        // scalar instruction, which is valid.
        if (iExtData->getFlags() & (X86Inst::kInstFlagEvexB4 | kInstFlagEvexB8)) {
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
        if (ASMJIT_UNLIKELY(!(iExtData->getFlags() & X86Inst::kInstFlagEvexSAE)))
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
