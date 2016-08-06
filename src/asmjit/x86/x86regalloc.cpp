// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER) && defined(ASMJIT_BUILD_X86)

// [Dependencies]
#include "../base/cpuinfo.h"
#include "../base/utils.h"
#include "../x86/x86assembler.h"
#include "../x86/x86compiler.h"
#include "../x86/x86regalloc_p.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [Forward Declarations]
// ============================================================================

static Error X86RAPass_translateOperands(X86RAPass* self, Operand_* opArray, uint32_t opCount);

// ============================================================================
// [asmjit::X86RAPass - Utils]
// ============================================================================

// Getting `VarClass` is the only safe operation when dealing with denormalized
// `varType`. Any other property would require to map typeId to regType.
static ASMJIT_INLINE uint32_t x86TypeIdToClass(uint32_t typeId) noexcept {
  ASMJIT_ASSERT(typeId < VirtType::kIdCount);
  return _x86TypeData.typeInfo[typeId].getRegClass();
}

// ============================================================================
// [asmjit::X86RAPass - Annotate]
// ============================================================================

#if !defined(ASMJIT_DISABLE_LOGGING)
static Error ASMJIT_CDECL X86VirtRegHandler(StringBuilder& out, uint32_t logOptions, const Reg& r, void* handlerData) noexcept {
  X86RAPass* self = static_cast<X86RAPass*>(handlerData);
  X86Compiler* cc = self->cc();

  uint32_t id = r.getId();
  if (!cc->isVirtRegValid(id))
    return DebugUtils::errored(kErrorInvalidState);

  VirtReg* vreg = cc->getVirtRegById(id);
  ASMJIT_ASSERT(vreg != nullptr);

  const char* name = vreg->getName();
  if (name && name[0] != '\0') {
    out.appendString(name);
  }
  else {
    out.appendChar('v');
    out.appendUInt(Operand::unpackId(id));
  }

  return kErrorOk;
}
#endif // !ASMJIT_DISABLE_LOGGING

#if defined(ASMJIT_TRACE)
static void ASMJIT_CDECL X86RAPass_traceNode(X86RAPass* self, CBNode* node_, const char* prefix) {
  StringBuilderTmp<256> sb;

  switch (node_->getType()) {
    case CBNode::kNodeAlign: {
      CBAlign* node = static_cast<CBAlign*>(node_);
      sb.appendFormat(".align %u (%s)",
        node->getAlignment(),
        node->getMode() == kAlignCode ? "code" : "data");
      break;
    }

    case CBNode::kNodeData: {
      CBData* node = static_cast<CBData*>(node_);
      sb.appendFormat(".embed (%u bytes)", node->getSize());
      break;
    }

    case CBNode::kNodeComment: {
      CBComment* node = static_cast<CBComment*>(node_);
      sb.appendFormat("; %s", node->getInlineComment());
      break;
    }

    case CBNode::kNodeHint: {
      CCHint* node = static_cast<CCHint*>(node_);
      static const char* hint[16] = {
        "alloc",
        "spill",
        "save",
        "save-unuse",
        "unuse"
      };
      sb.appendFormat("[%s] %s",
        hint[node->getHint()], node->getVReg()->getName());
      break;
    }

    case CBNode::kNodeLabel: {
      CBLabel* node = static_cast<CBLabel*>(node_);
      sb.appendFormat("L%u: (NumRefs=%u)", Operand::unpackId(node->getId()), node->getNumRefs());
      break;
    }

    case CBNode::kNodeInst: {
      CBInst* node = static_cast<CBInst*>(node_);
      self->_formatter.formatInstruction(sb, 0,
        node->getInstId(),
        node->getOptions(),
        node->getOpMask(),
        node->getOpArray(), node->getOpCount());
      break;
    }

    case CBNode::kNodeSentinel: {
      CBSentinel* node = static_cast<CBSentinel*>(node_);
      sb.appendFormat("[end]");
      break;
    }

    case CBNode::kNodeFunc: {
      CCFunc* node = static_cast<CCFunc*>(node_);
      sb.appendFormat("[func]");
      break;
    }

    case CBNode::kNodeFuncExit: {
      CCFuncRet* node = static_cast<CCFuncRet*>(node_);
      sb.appendFormat("[ret]");
      break;
    }

    case CBNode::kNodeCall: {
      CCCall* node = static_cast<CCCall*>(node_);
      sb.appendFormat("[call]");
      break;
    }

    case CBNode::kNodePushArg: {
      CCPushArg* node = static_cast<CCPushArg*>(node_);
      sb.appendFormat("[sarg]");
      break;
    }

    default: {
      sb.appendFormat("[unknown]");
      break;
    }
  }

  ASMJIT_TLOG("%s[%05u] %s\n", prefix, node_->getFlowId(), sb.getData());
}
#endif // ASMJIT_TRACE

// ============================================================================
// [asmjit::X86RAPass - Construction / Destruction]
// ============================================================================

X86RAPass::X86RAPass() noexcept : RAPass() {
#if defined(ASMJIT_TRACE)
  _traceNode = (TraceNodeFunc)X86RAPass_traceNode;
#endif // ASMJIT_TRACE

#if !defined(ASMJIT_DISABLE_LOGGING)
  _formatter.setVirtRegHandler(X86VirtRegHandler, this);
#endif // !ASMJIT_DISABLE_LOGGING

  _state = &_x86State;
  _varMapToVaListOffset = ASMJIT_OFFSET_OF(X86RAData, tiedArray);
}
X86RAPass::~X86RAPass() noexcept {}

// ============================================================================
// [asmjit::X86RAPass - Interface]
// ============================================================================

Error X86RAPass::process(CodeBuilder* cb, Zone* zone) noexcept {
  return Base::process(cb, zone);
}

Error X86RAPass::prepare(CCFunc* func) noexcept {
  ASMJIT_PROPAGATE(Base::prepare(func));

  uint32_t archType = _cc->getArchType();
  _regCount._gp  = archType == Arch::kTypeX86 ? 8 : 16;
  _regCount._mm  = 8;
  _regCount._k   = 8;
  _regCount._xyz = archType == Arch::kTypeX86 ? 8 : 16;
  _zsp = cc()->zsp();
  _zbp = cc()->zbp();

  _gaRegs[X86Reg::kClassGp ] = Utils::bits(_regCount.getGp()) & ~Utils::mask(X86Gp::kIdSp);
  _gaRegs[X86Reg::kClassMm ] = Utils::bits(_regCount.getMm());
  _gaRegs[X86Reg::kClassK  ] = Utils::bits(_regCount.getK());
  _gaRegs[X86Reg::kClassXyz] = Utils::bits(_regCount.getXyz());

  _x86State.reset(0);
  _clobberedRegs.reset();

  _stackFrameCell = nullptr;

  _argBaseReg = kInvalidReg; // Used by patcher.
  _varBaseReg = kInvalidReg; // Used by patcher.

  _argBaseOffset = 0;        // Used by patcher.
  _varBaseOffset = 0;        // Used by patcher.

  _argActualDisp = 0;        // Used by translator.
  _varActualDisp = 0;        // Used by translator.

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86SpecialInst]
// ============================================================================

struct X86SpecialInst {
  uint8_t inReg;
  uint8_t outReg;
  uint16_t flags;
};

static ASMJIT_INLINE const X86SpecialInst* X86SpecialInst_get(uint32_t instId, const Operand* opArray, uint32_t opCount) noexcept {
#define R(ri) { uint8_t(ri)         , uint8_t(kInvalidReg), uint16_t(TiedReg::kRReg) }
#define W(ri) { uint8_t(kInvalidReg), uint8_t(ri)         , uint16_t(TiedReg::kWReg) }
#define X(ri) { uint8_t(ri)         , uint8_t(ri)         , uint16_t(TiedReg::kXReg) }
#define NONE() { 0, 0, 0 }
  static const X86SpecialInst instCpuid[]        = { X(X86Gp::kIdAx), W(X86Gp::kIdBx), X(X86Gp::kIdCx), W(X86Gp::kIdDx) };
  static const X86SpecialInst instCbwCdqeCwde[]  = { X(X86Gp::kIdAx) };
  static const X86SpecialInst instCdqCwdCqo[]    = { W(X86Gp::kIdDx), R(X86Gp::kIdAx) };
  static const X86SpecialInst instCmpxchg[]      = { X(kInvalidReg), R(kInvalidReg), X(X86Gp::kIdAx) };
  static const X86SpecialInst instCmpxchg8b16b[] = { NONE(), X(X86Gp::kIdDx), X(X86Gp::kIdAx), R(X86Gp::kIdCx), R(X86Gp::kIdBx) };
  static const X86SpecialInst instDaaDas[]       = { X(X86Gp::kIdAx) };
  static const X86SpecialInst instDiv2[]         = { X(X86Gp::kIdAx), R(kInvalidReg) };
  static const X86SpecialInst instDiv3[]         = { X(X86Gp::kIdDx), X(X86Gp::kIdAx), R(kInvalidReg) };
  static const X86SpecialInst instJecxz[]        = { R(X86Gp::kIdCx) };
  static const X86SpecialInst instLods[]         = { W(X86Gp::kIdAx), X(X86Gp::kIdSi), X(X86Gp::kIdCx) };
  static const X86SpecialInst instMul2[]         = { X(X86Gp::kIdAx), R(kInvalidReg) };
  static const X86SpecialInst instMul3[]         = { W(X86Gp::kIdDx), X(X86Gp::kIdAx), R(kInvalidReg) };
  static const X86SpecialInst instMulx[]         = { W(kInvalidReg), W(kInvalidReg), R(kInvalidReg), R(X86Gp::kIdDx) };
  static const X86SpecialInst instMovsCmps[]     = { X(X86Gp::kIdDi), X(X86Gp::kIdSi), X(X86Gp::kIdCx) };
  static const X86SpecialInst instLahf[]         = { W(X86Gp::kIdAx) };
  static const X86SpecialInst instSahf[]         = { R(X86Gp::kIdAx) };
  static const X86SpecialInst instMaskmovq[]     = { R(kInvalidReg), R(kInvalidReg), R(X86Gp::kIdDi) };
  static const X86SpecialInst instRdtscRdtscp[]  = { W(X86Gp::kIdDx), W(X86Gp::kIdAx), W(X86Gp::kIdCx) };
  static const X86SpecialInst instRot[]          = { X(kInvalidReg), R(X86Gp::kIdCx) };
  static const X86SpecialInst instScas[]         = { X(X86Gp::kIdDi), R(X86Gp::kIdAx), X(X86Gp::kIdCx) };
  static const X86SpecialInst instShldShrd[]     = { X(kInvalidReg), R(kInvalidReg), R(X86Gp::kIdCx) };
  static const X86SpecialInst instStos[]         = { R(X86Gp::kIdDi), R(X86Gp::kIdAx), X(X86Gp::kIdCx) };
  static const X86SpecialInst instThirdXMM0[]    = { W(kInvalidReg), R(kInvalidReg), R(0) };
  static const X86SpecialInst instPcmpestri[]    = { R(kInvalidReg), R(kInvalidReg), NONE(), W(X86Gp::kIdCx) };
  static const X86SpecialInst instPcmpestrm[]    = { R(kInvalidReg), R(kInvalidReg), NONE(), W(0) };
  static const X86SpecialInst instPcmpistri[]    = { R(kInvalidReg), R(kInvalidReg), NONE(), W(X86Gp::kIdCx), R(X86Gp::kIdAx), R(X86Gp::kIdDx) };
  static const X86SpecialInst instPcmpistrm[]    = { R(kInvalidReg), R(kInvalidReg), NONE(), W(0)           , R(X86Gp::kIdAx), R(X86Gp::kIdDx) };
  static const X86SpecialInst instXsaveXrstor[]  = { W(kInvalidReg), R(X86Gp::kIdDx), R(X86Gp::kIdAx) };
  static const X86SpecialInst instXgetbv[]       = { W(X86Gp::kIdDx), W(X86Gp::kIdAx), R(X86Gp::kIdCx) };
  static const X86SpecialInst instXsetbv[]       = { R(X86Gp::kIdDx), R(X86Gp::kIdAx), R(X86Gp::kIdCx) };
#undef NONE
#undef X
#undef W
#undef R

  switch (instId) {
    case X86Inst::kIdCpuid      : return instCpuid;
    case X86Inst::kIdCbw        :
    case X86Inst::kIdCdqe       :
    case X86Inst::kIdCwde       : return instCbwCdqeCwde;
    case X86Inst::kIdCdq        :
    case X86Inst::kIdCwd        :
    case X86Inst::kIdCqo        : return instCdqCwdCqo;
    case X86Inst::kIdCmpsB      :
    case X86Inst::kIdCmpsD      :
    case X86Inst::kIdCmpsQ      :
    case X86Inst::kIdCmpsW      :
    case X86Inst::kIdRepeCmpsB  :
    case X86Inst::kIdRepeCmpsD  :
    case X86Inst::kIdRepeCmpsQ  :
    case X86Inst::kIdRepeCmpsW  :
    case X86Inst::kIdRepneCmpsB :
    case X86Inst::kIdRepneCmpsD :
    case X86Inst::kIdRepneCmpsQ :
    case X86Inst::kIdRepneCmpsW : return instMovsCmps;
    case X86Inst::kIdCmpxchg    : return instCmpxchg;
    case X86Inst::kIdCmpxchg8b  :
    case X86Inst::kIdCmpxchg16b : return instCmpxchg8b16b;
    case X86Inst::kIdDaa        :
    case X86Inst::kIdDas        : return instDaaDas;
    case X86Inst::kIdDiv        : return (opCount == 2) ? instDiv2 : instDiv3;
    case X86Inst::kIdIdiv       : return (opCount == 2) ? instDiv2 : instDiv3;
    case X86Inst::kIdImul       : if (opCount == 2) return nullptr;
                                  if (opCount == 3 && !(opArray[0].isReg() && opArray[1].isReg() && opArray[2].isRegOrMem())) return nullptr;
                                  ASMJIT_FALLTHROUGH;
    case X86Inst::kIdMul        : return (opCount == 2) ? instMul2 : instMul3;
    case X86Inst::kIdMulx       : return instMulx;
    case X86Inst::kIdJecxz      : return instJecxz;
    case X86Inst::kIdLodsB      :
    case X86Inst::kIdLodsD      :
    case X86Inst::kIdLodsQ      :
    case X86Inst::kIdLodsW      :
    case X86Inst::kIdRepLodsB   :
    case X86Inst::kIdRepLodsD   :
    case X86Inst::kIdRepLodsQ   :
    case X86Inst::kIdRepLodsW   : return instLods;
    case X86Inst::kIdMovsB      :
    case X86Inst::kIdMovsD      :
    case X86Inst::kIdMovsQ      :
    case X86Inst::kIdMovsW      :
    case X86Inst::kIdRepMovsB   :
    case X86Inst::kIdRepMovsD   :
    case X86Inst::kIdRepMovsQ   :
    case X86Inst::kIdRepMovsW   : return instMovsCmps;
    case X86Inst::kIdLahf       : return instLahf;
    case X86Inst::kIdSahf       : return instSahf;
    case X86Inst::kIdMaskmovq   :
    case X86Inst::kIdMaskmovdqu :
    case X86Inst::kIdVmaskmovdqu: return instMaskmovq;
    case X86Inst::kIdEnter      : return nullptr; // Not supported.
    case X86Inst::kIdLeave      : return nullptr; // Not supported.
    case X86Inst::kIdRet        : return nullptr; // Not supported.
    case X86Inst::kIdMonitor    : return nullptr; // TODO: [COMPILER] Monitor/MWait.
    case X86Inst::kIdMwait      : return nullptr; // TODO: [COMPILER] Monitor/MWait.
    case X86Inst::kIdPop        : return nullptr; // TODO: [COMPILER] Pop/Push.
    case X86Inst::kIdPush       : return nullptr; // TODO: [COMPILER] Pop/Push.
    case X86Inst::kIdPopa       : return nullptr; // Not supported.
    case X86Inst::kIdPopf       : return nullptr; // Not supported.
    case X86Inst::kIdPusha      : return nullptr; // Not supported.
    case X86Inst::kIdPushf      : return nullptr; // Not supported.
    case X86Inst::kIdRcl        :
    case X86Inst::kIdRcr        :
    case X86Inst::kIdRol        :
    case X86Inst::kIdRor        :
    case X86Inst::kIdSal        :
    case X86Inst::kIdSar        :
    case X86Inst::kIdShl        : // Rot instruction is special only if the last operand is a variable.
    case X86Inst::kIdShr        : if (!opArray[1].isReg()) return nullptr;
                                  return instRot;
    case X86Inst::kIdShld       : // Shld/Shrd instruction is special only if the last operand is a variable.
    case X86Inst::kIdShrd       : if (!opArray[2].isReg()) return nullptr;
                                  return instShldShrd;
    case X86Inst::kIdRdtsc      :
    case X86Inst::kIdRdtscp     : return instRdtscRdtscp;
    case X86Inst::kIdScasB      :
    case X86Inst::kIdScasD      :
    case X86Inst::kIdScasQ      :
    case X86Inst::kIdScasW      :
    case X86Inst::kIdRepeScasB  :
    case X86Inst::kIdRepeScasD  :
    case X86Inst::kIdRepeScasQ  :
    case X86Inst::kIdRepeScasW  :
    case X86Inst::kIdRepneScasB :
    case X86Inst::kIdRepneScasD :
    case X86Inst::kIdRepneScasQ :
    case X86Inst::kIdRepneScasW : return instScas;
    case X86Inst::kIdStosB      :
    case X86Inst::kIdStosD      :
    case X86Inst::kIdStosQ      :
    case X86Inst::kIdStosW      :
    case X86Inst::kIdRepStosB   :
    case X86Inst::kIdRepStosD   :
    case X86Inst::kIdRepStosQ   :
    case X86Inst::kIdRepStosW   : return instStos;
    case X86Inst::kIdBlendvpd   :
    case X86Inst::kIdBlendvps   :
    case X86Inst::kIdPblendvb   :
    case X86Inst::kIdSha256rnds2: return instThirdXMM0;
    case X86Inst::kIdPcmpestri  :
    case X86Inst::kIdVpcmpestri : return instPcmpestri;
    case X86Inst::kIdPcmpistri  :
    case X86Inst::kIdVpcmpistri : return instPcmpistri;
    case X86Inst::kIdPcmpestrm  :
    case X86Inst::kIdVpcmpestrm : return instPcmpestrm;
    case X86Inst::kIdPcmpistrm  :
    case X86Inst::kIdVpcmpistrm : return instPcmpistrm;
    case X86Inst::kIdXrstor     :
    case X86Inst::kIdXrstor64   :
    case X86Inst::kIdXsave      :
    case X86Inst::kIdXsave64    :
    case X86Inst::kIdXsaveopt   :
    case X86Inst::kIdXsaveopt64 : return instXsaveXrstor;
    case X86Inst::kIdXgetbv     : return instXgetbv;
    case X86Inst::kIdXsetbv     : return instXsetbv;
    default                     : return nullptr;
  }
}

// ============================================================================
// [asmjit::X86RAPass - EmitLoad]
// ============================================================================

void X86RAPass::emitLoad(VirtReg* vreg, uint32_t physId, const char* reason) {
  ASMJIT_ASSERT(physId != kInvalidReg);
  X86Mem m = getVarMem(vreg);

  switch (vreg->getTypeId()) {
    case VirtType::kIdI8      :
    case VirtType::kIdU8      : cc()->emit(X86Inst::kIdMov   , x86::gpb_lo(physId), m); break;
    case VirtType::kIdI16     :
    case VirtType::kIdU16     : cc()->emit(X86Inst::kIdMov   , x86::gpw(physId), m); break;
    case VirtType::kIdI32     :
    case VirtType::kIdU32     : cc()->emit(X86Inst::kIdMov   , x86::gpd(physId), m); break;
    case VirtType::kIdI64     :
    case VirtType::kIdU64     : cc()->emit(X86Inst::kIdMov   , x86::gpq(physId), m); break;
    case VirtType::kIdX86Mm   : cc()->emit(X86Inst::kIdMovq  , x86::mm(physId), m); break;
    case VirtType::kIdX86XmmSs: cc()->emit(X86Inst::kIdMovss , x86::xmm(physId), m); break;
    case VirtType::kIdX86XmmSd: cc()->emit(X86Inst::kIdMovsd , x86::xmm(physId), m); break;
    case VirtType::kIdX86Xmm  :
    case VirtType::kIdX86XmmPs:
    case VirtType::kIdX86XmmPd: cc()->emit(X86Inst::kIdMovaps, x86::xmm(physId), m); break;

    // Compiler doesn't manage FPU stack.
    case VirtType::kIdF32     :
    case VirtType::kIdF64     :
    default                   : ASMJIT_NOT_REACHED();
  }

  if (_emitComments)
    cc()->getCursor()->setInlineComment(cc()->_dataAllocator.sformat("[%s] %s", reason, vreg->getName()));
}

// ============================================================================
// [asmjit::X86RAPass - EmitSave]
// ============================================================================

void X86RAPass::emitSave(VirtReg* vreg, uint32_t physId, const char* reason) {
  ASMJIT_ASSERT(physId != kInvalidReg);
  X86Mem m = getVarMem(vreg);

  switch (vreg->getTypeId()) {
    case VirtType::kIdI8      :
    case VirtType::kIdU8      : cc()->emit(X86Inst::kIdMov   , m, x86::gpb_lo(physId)); break;
    case VirtType::kIdI16     :
    case VirtType::kIdU16     : cc()->emit(X86Inst::kIdMov   , m, x86::gpw(physId)); break;
    case VirtType::kIdI32     :
    case VirtType::kIdU32     : cc()->emit(X86Inst::kIdMov   , m, x86::gpd(physId)); break;
    case VirtType::kIdI64     :
    case VirtType::kIdU64     : cc()->emit(X86Inst::kIdMov   , m, x86::gpq(physId)); break;
    case VirtType::kIdX86Mm   : cc()->emit(X86Inst::kIdMovq  , m, x86::mm(physId)); break;
    case VirtType::kIdX86XmmSs: cc()->emit(X86Inst::kIdMovss , m, x86::xmm(physId)); break;
    case VirtType::kIdX86XmmSd: cc()->emit(X86Inst::kIdMovsd , m, x86::xmm(physId)); break;
    case VirtType::kIdX86Xmm  :
    case VirtType::kIdX86XmmPs:
    case VirtType::kIdX86XmmPd: cc()->emit(X86Inst::kIdMovaps, m, x86::xmm(physId)); break;

    // Compiler doesn't manage FPU stack.
    case VirtType::kIdF32     :
    case VirtType::kIdF64     :
    default                   : ASMJIT_NOT_REACHED();
  }

  if (_emitComments)
    cc()->getCursor()->setInlineComment(cc()->_dataAllocator.sformat("[%s] %s", reason, vreg->getName()));
}

// ============================================================================
// [asmjit::X86RAPass - EmitMove]
// ============================================================================

void X86RAPass::emitMove(VirtReg* vreg, uint32_t toPhysId, uint32_t fromPhysId, const char* reason) {
  ASMJIT_ASSERT(toPhysId != kInvalidReg);
  ASMJIT_ASSERT(fromPhysId != kInvalidReg);

  switch (vreg->getTypeId()) {
    case VirtType::kIdI8      :
    case VirtType::kIdU8      :
    case VirtType::kIdI16     :
    case VirtType::kIdU16     :
    case VirtType::kIdI32     :
    case VirtType::kIdU32     : cc()->emit(X86Inst::kIdMov   , x86::gpd(toPhysId), x86::gpd(fromPhysId)); break;
    case VirtType::kIdI64     :
    case VirtType::kIdU64     : cc()->emit(X86Inst::kIdMov   , x86::gpq(toPhysId), x86::gpq(fromPhysId)); break;
    case VirtType::kIdX86Mm   : cc()->emit(X86Inst::kIdMovq  , x86::mm(toPhysId), x86::mm(fromPhysId)); break;
    case VirtType::kIdX86XmmSs: cc()->emit(X86Inst::kIdMovss , x86::xmm(toPhysId), x86::xmm(fromPhysId)); break;
    case VirtType::kIdX86XmmSd: cc()->emit(X86Inst::kIdMovsd , x86::xmm(toPhysId), x86::xmm(fromPhysId)); break;
    case VirtType::kIdX86Xmm  :
    case VirtType::kIdX86XmmPs:
    case VirtType::kIdX86XmmPd: cc()->emit(X86Inst::kIdMovaps, x86::xmm(toPhysId), x86::xmm(fromPhysId)); break;

    // Compiler doesn't manage FPU stack.
    case VirtType::kIdF32     :
    case VirtType::kIdF64     :
    default                   : ASMJIT_NOT_REACHED();
  }

  if (_emitComments)
    cc()->getCursor()->setInlineComment(cc()->_dataAllocator.sformat("[%s] %s", reason, vreg->getName()));
}

// ============================================================================
// [asmjit::X86RAPass - EmitSwap]
// ============================================================================

void X86RAPass::emitSwapGp(VirtReg* aVReg, VirtReg* bVReg, uint32_t aIndex, uint32_t bIndex, const char* reason) {
  ASMJIT_ASSERT(aIndex != kInvalidReg);
  ASMJIT_ASSERT(bIndex != kInvalidReg);

  uint32_t typeId = Utils::iMax(aVReg->getTypeId(), bVReg->getTypeId());
  if (typeId == VirtType::kIdI64 || typeId == VirtType::kIdU64)
    cc()->emit(X86Inst::kIdXchg, x86::gpq(aIndex), x86::gpq(bIndex));
  else
    cc()->emit(X86Inst::kIdXchg, x86::gpd(aIndex), x86::gpd(bIndex));

  if (_emitComments)
    cc()->getCursor()->setInlineComment(cc()->_dataAllocator.sformat("[%s] %s, %s", reason, aVReg->getName(), bVReg->getName()));
}

// ============================================================================
// [asmjit::X86RAPass - EmitPushSequence / EmitPopSequence]
// ============================================================================

void X86RAPass::emitPushSequence(uint32_t regMask) {
  uint32_t i = 0;

  X86Gp gpr(_zsp);
  while (regMask != 0) {
    ASMJIT_ASSERT(i < _regCount.getGp());
    if ((regMask & 0x1) != 0) {
      gpr.setId(i);
      cc()->emit(X86Inst::kIdPush, gpr);
    }
    i++;
    regMask >>= 1;
  }
}

void X86RAPass::emitPopSequence(uint32_t regMask) {
  if (regMask == 0) return;

  uint32_t i = static_cast<int32_t>(_regCount.getGp());
  uint32_t mask = 0x1 << static_cast<uint32_t>(i - 1);

  X86Gp gpr(_zsp);
  while (i) {
    i--;
    if ((regMask & mask) != 0) {
      gpr.setId(i);
      cc()->emit(X86Inst::kIdPop, gpr);
    }
    mask >>= 1;
  }
}

// ============================================================================
// [asmjit::X86RAPass - EmitConvertVarToVar]
// ============================================================================

void X86RAPass::emitConvertVarToVar(uint32_t dstType, uint32_t dstIndex, uint32_t srcType, uint32_t srcIndex) {
  switch (dstType) {
    case VirtType::kIdI8:
    case VirtType::kIdU8:
    case VirtType::kIdI16:
    case VirtType::kIdU16:
    case VirtType::kIdI32:
    case VirtType::kIdU32:
    case VirtType::kIdI64:
    case VirtType::kIdU64:
      break;

    case VirtType::kIdX86XmmPs:
      if (srcType == VirtType::kIdX86XmmPd || srcType == VirtType::kIdX86YmmPd) {
        cc()->emit(X86Inst::kIdCvtpd2ps, x86::xmm(dstIndex), x86::xmm(srcIndex));
        return;
      }
      ASMJIT_FALLTHROUGH;

    case VirtType::kIdX86XmmSs:
      if (srcType == VirtType::kIdX86XmmSd || srcType == VirtType::kIdX86XmmPd || srcType == VirtType::kIdX86YmmPd) {
        cc()->emit(X86Inst::kIdCvtsd2ss, x86::xmm(dstIndex), x86::xmm(srcIndex));
        return;
      }

      if (VirtType::isIntTypeId(srcType)) {
        // TODO: [COMPILER] Variable conversion not supported.
        ASMJIT_NOT_REACHED();
      }
      break;

    case VirtType::kIdX86XmmPd:
      if (srcType == VirtType::kIdX86XmmPs || srcType == VirtType::kIdX86YmmPs) {
        cc()->emit(X86Inst::kIdCvtps2pd, x86::xmm(dstIndex), x86::xmm(srcIndex));
        return;
      }
      ASMJIT_FALLTHROUGH;

    case VirtType::kIdX86XmmSd:
      if (srcType == VirtType::kIdX86XmmSs || srcType == VirtType::kIdX86XmmPs || srcType == VirtType::kIdX86YmmPs) {
        cc()->emit(X86Inst::kIdCvtss2sd, x86::xmm(dstIndex), x86::xmm(srcIndex));
        return;
      }

      if (VirtType::isIntTypeId(srcType)) {
        // TODO: [COMPILER] Variable conversion not supported.
        ASMJIT_NOT_REACHED();
      }
      break;
  }
}

// ============================================================================
// [asmjit::X86RAPass - EmitMoveVarOnStack / EmitMoveImmOnStack]
// ============================================================================

void X86RAPass::emitMoveVarOnStack(
  uint32_t dstType, const X86Mem* dst,
  uint32_t srcType, uint32_t srcIndex) {

  ASMJIT_ASSERT(srcIndex != kInvalidReg);

  X86Mem m0(*dst);
  X86Reg r0, r1;

  uint32_t gpSize = cc()->getGpSize();
  uint32_t instId;

  switch (dstType) {
    case VirtType::kIdI8:
    case VirtType::kIdU8:
      // Move DWORD (GP).
      if (VirtType::isIntTypeId(srcType)) goto _MovGpD;

      // Move DWORD (MMX).
      if (srcType == VirtType::kIdX86Mm) goto _MovMmD;

      // Move DWORD (XMM).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdX86Xmm, VirtType::kIdX86XmmPd)) goto _MovXmmD;
      break;

    case VirtType::kIdI16:
    case VirtType::kIdU16:
      // Extend BYTE->WORD (GP).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdI8, VirtType::kIdU8)) {
        r1.setX86RegT<X86Reg::kRegGpbLo>(srcIndex);

        instId = (dstType == VirtType::kIdI16 && srcType == VirtType::kIdI8) ? X86Inst::kIdMovsx : X86Inst::kIdMovzx;
        goto _ExtendMovGpD;
      }

      // Move DWORD (GP).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdI16, VirtType::kIdU64))
        goto _MovGpD;

      // Move DWORD (MMX).
      if (srcType == VirtType::kIdX86Mm)
        goto _MovMmD;

      // Move DWORD (XMM).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdX86Xmm, VirtType::kIdX86XmmPd))
        goto _MovXmmD;
      break;

    case VirtType::kIdI32:
    case VirtType::kIdU32:
      // Extend BYTE->DWORD (GP).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdI8, VirtType::kIdU8)) {
        r1.setX86RegT<X86Reg::kRegGpbLo>(srcIndex);

        instId = (dstType == VirtType::kIdI32 && srcType == VirtType::kIdI8) ? X86Inst::kIdMovsx : X86Inst::kIdMovzx;
        goto _ExtendMovGpD;
      }

      // Extend WORD->DWORD (GP).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdI16, VirtType::kIdU16)) {
        r1.setX86RegT<X86Reg::kRegGpw>(srcIndex);

        instId = (dstType == VirtType::kIdI32 && srcType == VirtType::kIdI16) ? X86Inst::kIdMovsx : X86Inst::kIdMovzx;
        goto _ExtendMovGpD;
      }

      // Move DWORD (GP).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdI32, VirtType::kIdU64))
        goto _MovGpD;

      // Move DWORD (MMX).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdX86Mm, VirtType::kIdX86Mm))
        goto _MovMmD;

      // Move DWORD (XMM).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdX86Xmm, VirtType::kIdX86XmmPd))
        goto _MovXmmD;
      break;

    case VirtType::kIdI64:
    case VirtType::kIdU64:
      // Extend BYTE->QWORD (GP).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdI8, VirtType::kIdU8)) {
        r1.setX86RegT<X86Reg::kRegGpbLo>(srcIndex);

        instId = (dstType == VirtType::kIdI64 && srcType == VirtType::kIdI8) ? X86Inst::kIdMovsx : X86Inst::kIdMovzx;
        goto _ExtendMovGpXQ;
      }

      // Extend WORD->QWORD (GP).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdI16, VirtType::kIdU16)) {
        r1.setX86RegT<X86Reg::kRegGpw>(srcIndex);

        instId = (dstType == VirtType::kIdI64 && srcType == VirtType::kIdI16) ? X86Inst::kIdMovsx : X86Inst::kIdMovzx;
        goto _ExtendMovGpXQ;
      }

      // Extend DWORD->QWORD (GP).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdI32, VirtType::kIdU32)) {
        r1.setX86RegT<X86Reg::kRegGpd>(srcIndex);

        instId = X86Inst::kIdMovsxd;
        if (dstType == VirtType::kIdI64 && srcType == VirtType::kIdI32)
          goto _ExtendMovGpXQ;
        else
          goto _ZeroExtendGpDQ;
      }

      // Move QWORD (GP).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdI64, VirtType::kIdU64))
        goto _MovGpQ;

      // Move QWORD (MMX).
      if (srcType == VirtType::kIdX86Mm)
        goto _MovMmQ;

      // Move QWORD (XMM).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdX86Xmm, VirtType::kIdX86XmmPd))
        goto _MovXmmQ;
      break;

    case VirtType::kIdX86Mm:
      // Extend BYTE->QWORD (GP).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdI8, VirtType::kIdU8)) {
        r1.setX86RegT<X86Reg::kRegGpbLo>(srcIndex);

        instId = X86Inst::kIdMovzx;
        goto _ExtendMovGpXQ;
      }

      // Extend WORD->QWORD (GP).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdI16, VirtType::kIdU16)) {
        r1.setX86RegT<X86Reg::kRegGpw>(srcIndex);

        instId = X86Inst::kIdMovzx;
        goto _ExtendMovGpXQ;
      }

      // Extend DWORD->QWORD (GP).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdI32, VirtType::kIdU32))
        goto _ExtendMovGpDQ;

      // Move QWORD (GP).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdI64, VirtType::kIdU64))
        goto _MovGpQ;

      // Move QWORD (MMX).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdX86Mm, VirtType::kIdX86Mm))
        goto _MovMmQ;

      // Move QWORD (XMM).
      if (Utils::inInterval<uint32_t>(srcType, VirtType::kIdX86Xmm, VirtType::kIdX86XmmPd))
        goto _MovXmmQ;
      break;

    case VirtType::kIdF32:
    case VirtType::kIdX86XmmSs:
      // Move FLOAT.
      if (srcType == VirtType::kIdX86XmmSs || srcType == VirtType::kIdX86XmmPs || srcType == VirtType::kIdX86Xmm)
        goto _MovXmmD;

      ASMJIT_NOT_REACHED();
      break;

    case VirtType::kIdF64:
    case VirtType::kIdX86XmmSd:
      // Move DOUBLE.
      if (srcType == VirtType::kIdX86XmmSd || srcType == VirtType::kIdX86XmmPd || srcType == VirtType::kIdX86Xmm)
        goto _MovXmmQ;

      ASMJIT_NOT_REACHED();
      break;

    case VirtType::kIdX86Xmm:
      // TODO: [COMPILER].
      ASMJIT_NOT_REACHED();
      break;

    case VirtType::kIdX86XmmPs:
      // TODO: [COMPILER].
      ASMJIT_NOT_REACHED();
      break;

    case VirtType::kIdX86XmmPd:
      // TODO: [COMPILER].
      ASMJIT_NOT_REACHED();
      break;
  }
  return;

  // Extend+Move Gp.
_ExtendMovGpD:
  m0.setSize(4);
  r0.setX86RegT<X86Reg::kRegGpd>(srcIndex);

  cc()->emit(instId, r0, r1);
  cc()->emit(X86Inst::kIdMov, m0, r0);
  return;

_ExtendMovGpXQ:
  if (gpSize == 8) {
    m0.setSize(8);
    r0.setX86RegT<X86Reg::kRegGpq>(srcIndex);

    cc()->emit(instId, r0, r1);
    cc()->emit(X86Inst::kIdMov, m0, r0);
  }
  else {
    m0.setSize(4);
    r0.setX86RegT<X86Reg::kRegGpd>(srcIndex);

    cc()->emit(instId, r0, r1);

_ExtendMovGpDQ:
    cc()->emit(X86Inst::kIdMov, m0, r0);
    m0.addOffsetLo32(4);
    cc()->emit(X86Inst::kIdAnd, m0, 0);
  }
  return;

_ZeroExtendGpDQ:
  m0.setSize(4);
  r0.setX86RegT<X86Reg::kRegGpd>(srcIndex);
  goto _ExtendMovGpDQ;

  // Move Gp.
_MovGpD:
  m0.setSize(4);
  r0.setX86RegT<X86Reg::kRegGpd>(srcIndex);
  cc()->emit(X86Inst::kIdMov, m0, r0);
  return;

_MovGpQ:
  m0.setSize(8);
  r0.setX86RegT<X86Reg::kRegGpq>(srcIndex);
  cc()->emit(X86Inst::kIdMov, m0, r0);
  return;

  // Move Mm.
_MovMmD:
  m0.setSize(4);
  r0.setX86RegT<X86Reg::kRegMm>(srcIndex);
  cc()->emit(X86Inst::kIdMovd, m0, r0);
  return;

_MovMmQ:
  m0.setSize(8);
  r0.setX86RegT<X86Reg::kRegMm>(srcIndex);
  cc()->emit(X86Inst::kIdMovq, m0, r0);
  return;

  // Move XMM.
_MovXmmD:
  m0.setSize(4);
  r0.setX86RegT<X86Reg::kRegXmm>(srcIndex);
  cc()->emit(X86Inst::kIdMovss, m0, r0);
  return;

_MovXmmQ:
  m0.setSize(8);
  r0.setX86RegT<X86Reg::kRegXmm>(srcIndex);
  cc()->emit(X86Inst::kIdMovlps, m0, r0);
}

void X86RAPass::emitMoveImmOnStack(uint32_t dstType, const X86Mem* dst, const Imm* src) {
  X86Mem mem(*dst);
  Imm imm(*src);

  uint32_t gpSize = cc()->getGpSize();

  // One stack entry is equal to the native register size. That means that if
  // we want to move 32-bit integer on the stack, we need to extend it to 64-bit
  // integer.
  mem.setSize(gpSize);

  switch (dstType) {
    case VirtType::kIdI8:
    case VirtType::kIdU8:
      imm.truncateTo8Bits();
      goto _Move32;

    case VirtType::kIdI16:
    case VirtType::kIdU16:
      imm.truncateTo16Bits();
      goto _Move32;

    case VirtType::kIdI32:
    case VirtType::kIdU32:
_Move32:
      imm.truncateTo32Bits();
      cc()->emit(X86Inst::kIdMov, mem, imm);
      break;

    case VirtType::kIdI64:
    case VirtType::kIdU64:
_Move64:
      if (gpSize == 4) {
        uint32_t hi = imm.getUInt32Hi();

        // Lo-Part.
        imm.truncateTo32Bits();
        cc()->emit(X86Inst::kIdMov, mem, imm);
        mem.addOffsetLo32(gpSize);

        // Hi-Part.
        imm.setUInt32(hi);
        cc()->emit(X86Inst::kIdMov, mem, imm);
      }
      else {
        cc()->emit(X86Inst::kIdMov, mem, imm);
      }
      break;

    case VirtType::kIdF32:
      goto _Move32;

    case VirtType::kIdF64:
      goto _Move64;

    case VirtType::kIdX86Mm:
      goto _Move64;

    case VirtType::kIdX86Xmm:
    case VirtType::kIdX86XmmSs:
    case VirtType::kIdX86XmmPs:
    case VirtType::kIdX86XmmSd:
    case VirtType::kIdX86XmmPd:
      if (gpSize == 4) {
        uint32_t hi = imm.getUInt32Hi();

        // Lo part.
        imm.truncateTo32Bits();
        cc()->emit(X86Inst::kIdMov, mem, imm);
        mem.addOffsetLo32(gpSize);

        // Hi part.
        imm.setUInt32(hi);
        cc()->emit(X86Inst::kIdMov, mem, imm);
        mem.addOffsetLo32(gpSize);

        // Zero part.
        imm.setUInt32(0);
        cc()->emit(X86Inst::kIdMov, mem, imm);
        mem.addOffsetLo32(gpSize);

        cc()->emit(X86Inst::kIdMov, mem, imm);
      }
      else {
        // Lo/Hi parts.
        cc()->emit(X86Inst::kIdMov, mem, imm);
        mem.addOffsetLo32(gpSize);

        // Zero part.
        imm.setUInt32(0);
        cc()->emit(X86Inst::kIdMov, mem, imm);
      }
      break;

    default:
      ASMJIT_NOT_REACHED();
      break;
  }
}

// ============================================================================
// [asmjit::X86RAPass - EmitMoveImmToReg]
// ============================================================================

void X86RAPass::emitMoveImmToReg(uint32_t dstType, uint32_t dstIndex, const Imm* src) {
  ASMJIT_ASSERT(dstIndex != kInvalidReg);

  X86Reg r0;
  Imm imm(*src);

  switch (dstType) {
    case VirtType::kIdI8:
    case VirtType::kIdU8:
      imm.truncateTo8Bits();
      goto _Move32;

    case VirtType::kIdI16:
    case VirtType::kIdU16:
      imm.truncateTo16Bits();
      goto _Move32;

    case VirtType::kIdI32:
    case VirtType::kIdU32:
_Move32Truncate:
      imm.truncateTo32Bits();
_Move32:
      r0.setX86RegT<X86Reg::kRegGpd>(dstIndex);
      cc()->emit(X86Inst::kIdMov, r0, imm);
      break;

    case VirtType::kIdI64:
    case VirtType::kIdU64:
      // Move to GPD register will also clear high DWORD of GPQ register in
      // 64-bit mode.
      if (imm.isUInt32())
        goto _Move32Truncate;

      r0.setX86RegT<X86Reg::kRegGpq>(dstIndex);
      cc()->emit(X86Inst::kIdMov, r0, imm);
      break;

    case VirtType::kIdF32:
    case VirtType::kIdF64:
      // cc() doesn't manage FPU stack.
      ASMJIT_NOT_REACHED();
      break;

    case VirtType::kIdX86Mm:
      // TODO: [COMPILER] EmitMoveImmToReg.
      break;

    case VirtType::kIdX86Xmm:
    case VirtType::kIdX86XmmSs:
    case VirtType::kIdX86XmmSd:
    case VirtType::kIdX86XmmPs:
    case VirtType::kIdX86XmmPd:
      // TODO: [COMPILER] EmitMoveImmToReg.
      break;

    default:
      ASMJIT_NOT_REACHED();
      break;
  }
}

// ============================================================================
// [asmjit::X86RAPass - Register Management]
// ============================================================================

#if defined(ASMJIT_DEBUG)
template<int C>
static ASMJIT_INLINE void X86RAPass_checkStateVars(X86RAPass* self) {
  X86RAState* state = self->getState();
  VirtReg** sVars = state->getListByRC(C);

  uint32_t physId;
  uint32_t regMask;
  uint32_t regCount = self->_regCount.get(C);

  uint32_t occupied = state->_occupied.get(C);
  uint32_t modified = state->_modified.get(C);

  for (physId = 0, regMask = 1; physId < regCount; physId++, regMask <<= 1) {
    VirtReg* vreg = sVars[physId];

    if (!vreg) {
      ASMJIT_ASSERT((occupied & regMask) == 0);
      ASMJIT_ASSERT((modified & regMask) == 0);
    }
    else {
      ASMJIT_ASSERT((occupied & regMask) != 0);
      ASMJIT_ASSERT((modified & regMask) == (static_cast<uint32_t>(vreg->isModified()) << physId));

      ASMJIT_ASSERT(vreg->getRegClass() == C);
      ASMJIT_ASSERT(vreg->getState() == VirtReg::kStateReg);
      ASMJIT_ASSERT(vreg->getPhysId() == physId);
    }
  }
}

void X86RAPass::_checkState() {
  X86RAPass_checkStateVars<X86Reg::kClassGp >(this);
  X86RAPass_checkStateVars<X86Reg::kClassMm >(this);
  X86RAPass_checkStateVars<X86Reg::kClassXyz>(this);
}
#else
void X86RAPass::_checkState() {}
#endif // ASMJIT_DEBUG

// ============================================================================
// [asmjit::X86RAPass - State - Load]
// ============================================================================

template<int C>
static ASMJIT_INLINE void X86RAPass_loadStateVars(X86RAPass* self, X86RAState* src) {
  X86RAState* cur = self->getState();

  VirtReg** cVars = cur->getListByRC(C);
  VirtReg** sVars = src->getListByRC(C);

  uint32_t physId;
  uint32_t modified = src->_modified.get(C);
  uint32_t regCount = self->_regCount.get(C);

  for (physId = 0; physId < regCount; physId++, modified >>= 1) {
    VirtReg* vreg = sVars[physId];
    cVars[physId] = vreg;
    if (!vreg) continue;

    vreg->setState(VirtReg::kStateReg);
    vreg->setPhysId(physId);
    vreg->setModified(modified & 0x1);
  }
}

void X86RAPass::loadState(RAState* src_) {
  X86RAState* cur = getState();
  X86RAState* src = static_cast<X86RAState*>(src_);

  VirtReg** vregs = _contextVd.getData();
  uint32_t count = static_cast<uint32_t>(_contextVd.getLength());

  // Load allocated variables.
  X86RAPass_loadStateVars<X86Reg::kClassGp >(this, src);
  X86RAPass_loadStateVars<X86Reg::kClassMm >(this, src);
  X86RAPass_loadStateVars<X86Reg::kClassXyz>(this, src);

  // Load masks.
  cur->_occupied = src->_occupied;
  cur->_modified = src->_modified;

  // Load states of other variables and clear their 'Modified' flags.
  for (uint32_t i = 0; i < count; i++) {
    uint32_t vState = src->_cells[i].getState();

    if (vState == VirtReg::kStateReg)
      continue;

    vregs[i]->setState(vState);
    vregs[i]->setPhysId(kInvalidReg);
    vregs[i]->setModified(false);
  }

  ASMJIT_X86_CHECK_STATE
}

// ============================================================================
// [asmjit::X86RAPass - State - Save]
// ============================================================================

RAState* X86RAPass::saveState() {
  VirtReg** vregs = _contextVd.getData();
  uint32_t count = static_cast<uint32_t>(_contextVd.getLength());

  size_t size = Utils::alignTo<size_t>(
    sizeof(X86RAState) + count * sizeof(X86StateCell), sizeof(void*));

  X86RAState* cur = getState();
  X86RAState* dst = _zone->allocT<X86RAState>(size);
  if (!dst) return nullptr;

  // Store links.
  ::memcpy(dst->_list, cur->_list, X86RAState::kAllCount * sizeof(VirtReg*));

  // Store masks.
  dst->_occupied = cur->_occupied;
  dst->_modified = cur->_modified;

  // Store cells.
  for (uint32_t i = 0; i < count; i++) {
    VirtReg* vreg = static_cast<VirtReg*>(vregs[i]);
    X86StateCell& cell = dst->_cells[i];

    cell.reset();
    cell.setState(vreg->getState());
  }

  return dst;
}

// ============================================================================
// [asmjit::X86RAPass - State - Switch]
// ============================================================================

template<int C>
static ASMJIT_INLINE void X86RAPass_switchStateVars(X86RAPass* self, X86RAState* src) {
  X86RAState* dst = self->getState();

  VirtReg** dVars = dst->getListByRC(C);
  VirtReg** sVars = src->getListByRC(C);

  X86StateCell* cells = src->_cells;
  uint32_t regCount = self->_regCount.get(C);
  bool didWork;

  do {
    didWork = false;

    for (uint32_t physId = 0, regMask = 0x1; physId < regCount; physId++, regMask <<= 1) {
      VirtReg* dVReg = dVars[physId];
      VirtReg* sVd = sVars[physId];
      if (dVReg == sVd) continue;

      if (dVReg) {
        const X86StateCell& cell = cells[dVReg->_raId];

        if (cell.getState() != VirtReg::kStateReg) {
          if (cell.getState() == VirtReg::kStateMem)
            self->spill<C>(dVReg);
          else
            self->unuse<C>(dVReg);

          dVReg = nullptr;
          didWork = true;
          if (!sVd) continue;
        }
      }

      if (!dVReg && sVd) {
_MoveOrLoad:
        if (sVd->getPhysId() != kInvalidReg)
          self->move<C>(sVd, physId);
        else
          self->load<C>(sVd, physId);

        didWork = true;
        continue;
      }

      if (dVReg) {
        const X86StateCell& cell = cells[dVReg->_raId];
        if (!sVd) {
          if (cell.getState() == VirtReg::kStateReg)
            continue;

          if (cell.getState() == VirtReg::kStateMem)
            self->spill<C>(dVReg);
          else
            self->unuse<C>(dVReg);

          didWork = true;
          continue;
        }
        else {
          if (cell.getState() == VirtReg::kStateReg) {
            if (dVReg->getPhysId() != kInvalidReg && sVd->getPhysId() != kInvalidReg) {
              if (C == X86Reg::kClassGp) {
                self->swapGp(dVReg, sVd);
              }
              else {
                self->spill<C>(dVReg);
                self->move<C>(sVd, physId);
              }

              didWork = true;
              continue;
            }
            else {
              didWork = true;
              continue;
            }
          }

          if (cell.getState() == VirtReg::kStateMem)
            self->spill<C>(dVReg);
          else
            self->unuse<C>(dVReg);
          goto _MoveOrLoad;
        }
      }
    }
  } while (didWork);

  uint32_t dModified = dst->_modified.get(C);
  uint32_t sModified = src->_modified.get(C);

  if (dModified != sModified) {
    for (uint32_t physId = 0, regMask = 0x1; physId < regCount; physId++, regMask <<= 1) {
      VirtReg* vreg = dVars[physId];
      if (!vreg) continue;

      if ((dModified & regMask) && !(sModified & regMask)) {
        self->save<C>(vreg);
        continue;
      }

      if (!(dModified & regMask) && (sModified & regMask)) {
        self->modify<C>(vreg);
        continue;
      }
    }
  }
}

void X86RAPass::switchState(RAState* src_) {
  ASMJIT_ASSERT(src_ != nullptr);

  X86RAState* cur = getState();
  X86RAState* src = static_cast<X86RAState*>(src_);

  // Ignore if both states are equal.
  if (cur == src)
    return;

  // Switch variables.
  X86RAPass_switchStateVars<X86Reg::kClassGp >(this, src);
  X86RAPass_switchStateVars<X86Reg::kClassMm >(this, src);
  X86RAPass_switchStateVars<X86Reg::kClassXyz>(this, src);

  // Calculate changed state.
  VirtReg** vregs = _contextVd.getData();
  uint32_t count = static_cast<uint32_t>(_contextVd.getLength());

  X86StateCell* cells = src->_cells;
  for (uint32_t i = 0; i < count; i++) {
    VirtReg* vreg = static_cast<VirtReg*>(vregs[i]);
    const X86StateCell& cell = cells[i];
    uint32_t vState = cell.getState();

    if (vState != VirtReg::kStateReg) {
      vreg->setState(vState);
      vreg->setModified(false);
    }
  }

  ASMJIT_X86_CHECK_STATE
}

// ============================================================================
// [asmjit::X86RAPass - State - Intersect]
// ============================================================================

// The algorithm is actually not so smart, but tries to find an intersection od
// `a` and `b` and tries to move/alloc a variable into that location if it's
// possible. It also finds out which variables will be spilled/unused  by `a`
// and `b` and performs that action here. It may improve the switch state code
// in certain cases, but doesn't necessarily do the best job possible.
template<int C>
static ASMJIT_INLINE void X86RAPass_intersectStateVars(X86RAPass* self, X86RAState* a, X86RAState* b) {
  X86RAState* dst = self->getState();

  VirtReg** dVars = dst->getListByRC(C);
  VirtReg** aVars = a->getListByRC(C);
  VirtReg** bVars = b->getListByRC(C);

  X86StateCell* aCells = a->_cells;
  X86StateCell* bCells = b->_cells;

  uint32_t regCount = self->_regCount.get(C);
  bool didWork;

  // Similar to `switchStateVars()`, we iterate over and over until there is
  // no work to be done.
  do {
    didWork = false;

    for (uint32_t physId = 0, regMask = 0x1; physId < regCount; physId++, regMask <<= 1) {
      VirtReg* dVReg = dVars[physId]; // Destination reg.
      VirtReg* aVReg = aVars[physId]; // State-a reg.
      VirtReg* bVReg = bVars[physId]; // State-b reg.

      if (dVReg == aVReg) continue;

      if (dVReg) {
        const X86StateCell& aCell = aCells[dVReg->_raId];
        const X86StateCell& bCell = bCells[dVReg->_raId];

        if (aCell.getState() != VirtReg::kStateReg && bCell.getState() != VirtReg::kStateReg) {
          if (aCell.getState() == VirtReg::kStateMem || bCell.getState() == VirtReg::kStateMem)
            self->spill<C>(dVReg);
          else
            self->unuse<C>(dVReg);

          dVReg = nullptr;
          didWork = true;
          if (!aVReg) continue;
        }
      }

      if (!dVReg && aVReg) {
        if (aVReg->getPhysId() != kInvalidReg)
          self->move<C>(aVReg, physId);
        else
          self->load<C>(aVReg, physId);

        didWork = true;
        continue;
      }

      if (dVReg) {
        const X86StateCell& aCell = aCells[dVReg->_raId];
        const X86StateCell& bCell = bCells[dVReg->_raId];

        if (!aVReg) {
          if (aCell.getState() == VirtReg::kStateReg || bCell.getState() == VirtReg::kStateReg)
            continue;

          if (aCell.getState() == VirtReg::kStateMem || bCell.getState() == VirtReg::kStateMem)
            self->spill<C>(dVReg);
          else
            self->unuse<C>(dVReg);

          didWork = true;
          continue;
        }
        else if (C == X86Reg::kClassGp) {
          if (aCell.getState() == VirtReg::kStateReg) {
            if (dVReg->getPhysId() != kInvalidReg && aVReg->getPhysId() != kInvalidReg) {
              self->swapGp(dVReg, aVReg);

              didWork = true;
              continue;
            }
          }
        }
      }
    }
  } while (didWork);

  uint32_t dModified = dst->_modified.get(C);
  uint32_t aModified = a->_modified.get(C);

  if (dModified != aModified) {
    for (uint32_t physId = 0, regMask = 0x1; physId < regCount; physId++, regMask <<= 1) {
      VirtReg* vreg = dVars[physId];
      if (!vreg) continue;

      const X86StateCell& aCell = aCells[vreg->_raId];
      if ((dModified & regMask) && !(aModified & regMask) && aCell.getState() == VirtReg::kStateReg)
        self->save<C>(vreg);
    }
  }
}

void X86RAPass::intersectStates(RAState* a_, RAState* b_) {
  X86RAState* a = static_cast<X86RAState*>(a_);
  X86RAState* b = static_cast<X86RAState*>(b_);

  ASMJIT_ASSERT(a != nullptr);
  ASMJIT_ASSERT(b != nullptr);

  X86RAPass_intersectStateVars<X86Reg::kClassGp >(this, a, b);
  X86RAPass_intersectStateVars<X86Reg::kClassMm >(this, a, b);
  X86RAPass_intersectStateVars<X86Reg::kClassXyz>(this, a, b);

  ASMJIT_X86_CHECK_STATE
}

// ============================================================================
// [asmjit::X86RAPass - GetJccFlow / GetOppositeJccFlow]
// ============================================================================

//! \internal
static ASMJIT_INLINE CBNode* X86RAPass_getJccFlow(CBJump* jNode) {
  if (jNode->isTaken())
    return jNode->getTarget();
  else
    return jNode->getNext();
}

//! \internal
static ASMJIT_INLINE CBNode* X86RAPass_getOppositeJccFlow(CBJump* jNode) {
  if (jNode->isTaken())
    return jNode->getNext();
  else
    return jNode->getTarget();
}

// ============================================================================
// [asmjit::X86RAPass - SingleVarInst]
// ============================================================================

//! \internal
static void X86RAPass_prepareSingleVarInst(uint32_t instId, TiedReg* tr) {
  switch (instId) {
    // - andn     reg, reg ; Set all bits in reg to 0.
    // - xor/pxor reg, reg ; Set all bits in reg to 0.
    // - sub/psub reg, reg ; Set all bits in reg to 0.
    // - pcmpgt   reg, reg ; Set all bits in reg to 0.
    // - pcmpeq   reg, reg ; Set all bits in reg to 1.
    case X86Inst::kIdPandn     :
    case X86Inst::kIdXor       : case X86Inst::kIdXorpd     : case X86Inst::kIdXorps     : case X86Inst::kIdPxor      :
    case X86Inst::kIdSub:
    case X86Inst::kIdPsubb     : case X86Inst::kIdPsubw     : case X86Inst::kIdPsubd     : case X86Inst::kIdPsubq     :
    case X86Inst::kIdPsubsb    : case X86Inst::kIdPsubsw    : case X86Inst::kIdPsubusb   : case X86Inst::kIdPsubusw   :
    case X86Inst::kIdPcmpeqb   : case X86Inst::kIdPcmpeqw   : case X86Inst::kIdPcmpeqd   : case X86Inst::kIdPcmpeqq   :
    case X86Inst::kIdPcmpgtb   : case X86Inst::kIdPcmpgtw   : case X86Inst::kIdPcmpgtd   : case X86Inst::kIdPcmpgtq   :
      tr->flags &= ~TiedReg::kRReg;
      break;

    // - and      reg, reg ; Nop.
    // - or       reg, reg ; Nop.
    // - xchg     reg, reg ; Nop.
    case X86Inst::kIdAnd       : case X86Inst::kIdAndpd     : case X86Inst::kIdAndps     : case X86Inst::kIdPand      :
    case X86Inst::kIdOr        : case X86Inst::kIdOrpd      : case X86Inst::kIdOrps      : case X86Inst::kIdPor       :
    case X86Inst::kIdXchg      :
      tr->flags &= ~TiedReg::kWReg;
      break;
  }
}

// ============================================================================
// [asmjit::X86RAPass - Helpers]
// ============================================================================

//! \internal
//!
//! Get mask of all registers actually used to pass function arguments.
static ASMJIT_INLINE X86RegMask X86RAPass_getUsedArgs(X86RAPass* self, X86CCCall* node, X86FuncDecl* decl) {
  X86RegMask regs;
  regs.reset();

  uint32_t i;
  uint32_t argCount = decl->getNumArgs();

  for (i = 0; i < argCount; i++) {
    const FuncInOut& arg = decl->getArg(i);
    if (!arg.hasRegId()) continue;
    regs.or_(x86TypeIdToClass(arg.getTypeId()), Utils::mask(arg.getRegId()));
  }

  return regs;
}

// ============================================================================
// [asmjit::X86RAPass - SArg Insertion]
// ============================================================================

struct SArgData {
  VirtReg* sVd;
  VirtReg* cVd;
  CCPushArg* sArg;
  uint32_t aType;
};

#define SARG(dst, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15, s16, s17, s18, s19, s20, s21, s22, s23, s24) \
  (s0  <<  0) | (s1  <<  1) | (s2  <<  2) | (s3  <<  3) | (s4  <<  4) | (s5  <<  5) | (s6  <<  6) | (s7  <<  7) | \
  (s8  <<  8) | (s9  <<  9) | (s10 << 10) | (s11 << 11) | (s12 << 12) | (s13 << 13) | (s14 << 14) | (s15 << 15) | \
  (s16 << 16) | (s17 << 17) | (s18 << 18) | (s19 << 19) | (s20 << 20) | (s21 << 21) | (s22 << 22) | (s23 << 23) | \
  (s24 << 24)
#define A 0 // Auto-convert (doesn't need conversion step).
static const uint32_t X86RAPass_sArgConvTable[VirtType::kIdCount] = {
  // dst <- | i8| u8|i16|u16|i32|u32|i64|u64| iP| uP|f32|f64|mmx| k |xmm|xSs|xPs|xSd|xPd|ymm|yPs|yPd|zmm|zPs|zPd|
  //--------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  SARG(i8   , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , A , A , 0 , 0 , 0 , 1 , 1 , 1 , 1 , 0 , 1 , 1 , 0 , 1 , 1 ),
  SARG(u8   , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , A , A , 0 , 0 , 0 , 1 , 1 , 1 , 1 , 0 , 1 , 1 , 0 , 1 , 1 ),
  SARG(i16  , A , A , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , A , A , 0 , 0 , 0 , 1 , 1 , 1 , 1 , 0 , 1 , 1 , 0 , 1 , 1 ),
  SARG(u16  , A , A , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , A , A , 0 , 0 , 0 , 1 , 1 , 1 , 1 , 0 , 1 , 1 , 0 , 1 , 1 ),
  SARG(i32  , A , A , A , A , 0 , 0 , 0 , 0 , 0 , 0 , A , A , 0 , 0 , 0 , 1 , 1 , 1 , 1 , 0 , 1 , 1 , 0 , 1 , 1 ),
  SARG(u32  , A , A , A , A , 0 , 0 , 0 , 0 , 0 , 0 , A , A , 0 , 0 , 0 , 1 , 1 , 1 , 1 , 0 , 1 , 1 , 0 , 1 , 1 ),
  SARG(i64  , A , A , A , A , A , A , 0 , 0 , A , A , A , A , 0 , 0 , 0 , 1 , 1 , 1 , 1 , 0 , 1 , 1 , 0 , 1 , 1 ),
  SARG(u64  , A , A , A , A , A , A , 0 , 0 , A , A , A , A , 0 , 0 , 0 , 1 , 1 , 1 , 1 , 0 , 1 , 1 , 0 , 1 , 1 ),
  SARG(iPtr , A , A , A , A , A , A , A , A , 0 , 0 , A , A , 0 , 0 , 0 , 1 , 1 , 1 , 1 , 0 , 1 , 1 , 0 , 1 , 1 ),
  SARG(uPtr , A , A , A , A , A , A , A , A , 0 , 0 , A , A , 0 , 0 , 0 , 1 , 1 , 1 , 1 , 0 , 1 , 1 , 0 , 1 , 1 ),
  SARG(f32  , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 0 , A , 0 , 0 , 0 , 0 , 0 , 1 , 1 , 0 , 0 , 1 , 0 , 0 , 1 ),
  SARG(f64  , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , A , 0 , 0 , 0 , 0 , 1 , 1 , 0 , 0 , 0 , 1 , 0 , 0 , 1 , 0 ),
  SARG(mmx  , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ),
  SARG(k    , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ),
  SARG(xmm  , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ),
  SARG(xSs  , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 0 , 0 , 0 , 0 , 1 , 1 , 0 , 0 , 1 , 0 , 0 , 1 ),
  SARG(xPs  , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 0 , 0 , 0 , 0 , 1 , 1 , 0 , 0 , 1 , 0 , 0 , 1 ),
  SARG(xSd  , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 0 , 0 , 1 , 1 , 0 , 0 , 0 , 1 , 0 , 0 , 1 , 0 ),
  SARG(xPd  , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 0 , 0 , 1 , 1 , 0 , 0 , 0 , 1 , 0 , 0 , 1 , 0 ),
  SARG(ymm  , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ),
  SARG(yPs  , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 0 , 0 , 0 , 0 , 1 , 1 , 0 , 0 , 1 , 0 , 0 , 1 ),
  SARG(yPd  , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 0 , 0 , 1 , 1 , 0 , 0 , 0 , 1 , 0 , 0 , 1 , 0 ),
  SARG(zmm  , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ),
  SARG(zPs  , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 0 , 0 , 0 , 0 , 1 , 1 , 0 , 0 , 1 , 0 , 0 , 1 ),
  SARG(zPd  , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 0 , 0 , 1 , 1 , 0 , 0 , 0 , 1 , 0 , 0 , 1 , 0 )
};
#undef A
#undef SARG

static ASMJIT_INLINE bool X86RAPass_mustConvertSArg(X86RAPass* self, uint32_t aType, uint32_t sType) {
  return (X86RAPass_sArgConvTable[aType] & (1 << sType)) != 0;
}

static ASMJIT_INLINE uint32_t X86RAPass_typeOfConvertedSArg(X86RAPass* self, uint32_t aType, uint32_t sType) {
  ASMJIT_ASSERT(X86RAPass_mustConvertSArg(self, aType, sType));
  if (VirtType::isIntTypeId(aType)) return aType;

  if (aType == VirtType::kIdF32) return VirtType::kIdX86XmmSs;
  if (aType == VirtType::kIdF64) return VirtType::kIdX86XmmSd;

  return aType;
}

static ASMJIT_INLINE Error X86RAPass_insertPushArg(
  X86RAPass* self, X86CCCall* call,
  VirtReg* sReg, const uint32_t* gaRegs,
  const FuncInOut& arg, uint32_t argIndex,
  SArgData* sArgList, uint32_t& sArgCount) {

  X86Compiler* cc = self->cc();
  uint32_t i;
  uint32_t aType = arg.getTypeId();
  uint32_t sType = sReg->getTypeId();

  // First locate or create sArgBase.
  for (i = 0; i < sArgCount; i++)
    if (sArgList[i].sVd == sReg && !sArgList[i].cVd)
      break;

  SArgData* sArgData = &sArgList[i];
  if (i == sArgCount) {
    sArgData->sVd = sReg;
    sArgData->cVd = nullptr;
    sArgData->sArg = nullptr;
    sArgData->aType = 0xFF;
    sArgCount++;
  }

  const VirtType& sInfo = _x86TypeData.typeInfo[sType];
  uint32_t sClass = sInfo.getRegClass();

  if (X86RAPass_mustConvertSArg(self, aType, sType)) {
    uint32_t cType = X86RAPass_typeOfConvertedSArg(self, aType, sType);

    const VirtType& cInfo = _x86TypeData.typeInfo[cType];
    uint32_t cClass = cInfo.getRegClass();

    while (++i < sArgCount) {
      sArgData = &sArgList[i];
      if (sArgData->sVd != sReg)
        break;

      if (sArgData->cVd->getTypeId() != cType || sArgData->aType != aType)
        continue;

      sArgData->sArg->_args |= Utils::mask(argIndex);
      return kErrorOk;
    }

    VirtReg* cReg = cc->newVirtReg(cInfo, nullptr);
    if (!cReg) return DebugUtils::errored(kErrorNoHeapMemory);

    CCPushArg* sArg = cc->newNodeT<CCPushArg>(call, sReg, cReg);
    if (!sArg) return DebugUtils::errored(kErrorNoHeapMemory);

    X86RAData* raData = self->newRAData(2);
    if (!raData) return DebugUtils::errored(kErrorNoHeapMemory);

    ASMJIT_PROPAGATE(self->assignRAId(cReg));
    ASMJIT_PROPAGATE(self->assignRAId(sReg));

    raData->tiedTotal = 2;
    raData->tiedCount.reset();
    raData->tiedCount.add(sClass);
    raData->tiedCount.add(cClass);

    raData->tiedIndex.reset();
    raData->inRegs.reset();
    raData->outRegs.reset();
    raData->clobberedRegs.reset();

    if (sClass <= cClass) {
      raData->tiedArray[0].setup(sReg, TiedReg::kRReg, 0, gaRegs[sClass]);
      raData->tiedArray[1].setup(cReg, TiedReg::kWReg, 0, gaRegs[cClass]);
      raData->tiedIndex.set(cClass, sClass != cClass);
    }
    else {
      raData->tiedArray[0].setup(cReg, TiedReg::kWReg, 0, gaRegs[cClass]);
      raData->tiedArray[1].setup(sReg, TiedReg::kRReg, 0, gaRegs[sClass]);
      raData->tiedIndex.set(sClass, 1);
    }

    sArg->setPassData(raData);
    sArg->_args |= Utils::mask(argIndex);

    cc->addBefore(sArg, call);
    ::memmove(sArgData + 1, sArgData, (sArgCount - i) * sizeof(SArgData));

    sArgData->sVd = sReg;
    sArgData->cVd = cReg;
    sArgData->sArg = sArg;
    sArgData->aType = aType;

    sArgCount++;
    return kErrorOk;
  }
  else {
    CCPushArg* sArg = sArgData->sArg;
    ASMJIT_PROPAGATE(self->assignRAId(sReg));

    if (!sArg) {
      sArg = cc->newNodeT<CCPushArg>(call, sReg, (VirtReg*)nullptr);
      if (!sArg) return DebugUtils::errored(kErrorNoHeapMemory);

      X86RAData* raData = self->newRAData(1);
      if (!raData) return DebugUtils::errored(kErrorNoHeapMemory);

      raData->tiedTotal = 1;
      raData->tiedIndex.reset();
      raData->tiedCount.reset();
      raData->tiedCount.add(sClass);
      raData->inRegs.reset();
      raData->outRegs.reset();
      raData->clobberedRegs.reset();
      raData->tiedArray[0].setup(sReg, TiedReg::kRReg, 0, gaRegs[sClass]);

      sArg->setPassData(raData);
      sArgData->sArg = sArg;

      cc->addBefore(sArg, call);
    }

    sArg->_args |= Utils::mask(argIndex);
    return kErrorOk;
  }
}

// ============================================================================
// [asmjit::X86RAPass - Fetch]
// ============================================================================

//! \internal
//!
//! Prepare the given function `func`.
//!
//! For each node:
//! - Create and assign groupId and flowId.
//! - Collect all variables and merge them to vaList.
Error X86RAPass::fetch() {
  ASMJIT_TLOG("[F] ======= Fetch (Begin)\n");

  uint32_t archType = cc()->getArchType();
  X86Func* func = getFunc();

  CBNode* node_ = func;
  CBNode* next = nullptr;
  CBNode* stop = getStop();

  TiedReg agTmp[80];
  SArgData sArgList[80];

  uint32_t flowId = 0;
  PodList<CBNode*>::Link* jLink = nullptr;

  // Function flags.
  func->clearFuncFlags(kFuncFlagIsNaked   |
                       kFuncFlagX86Emms   |
                       kFuncFlagX86SFence |
                       kFuncFlagX86LFence );

  if (func->getHint(kFuncHintNaked    ) != 0) func->addFuncFlags(kFuncFlagIsNaked);
  if (func->getHint(kFuncHintCompact  ) != 0) func->addFuncFlags(kFuncFlagX86Leave);
  if (func->getHint(kFuncHintX86Emms  ) != 0) func->addFuncFlags(kFuncFlagX86Emms);
  if (func->getHint(kFuncHintX86SFence) != 0) func->addFuncFlags(kFuncFlagX86SFence);
  if (func->getHint(kFuncHintX86LFence) != 0) func->addFuncFlags(kFuncFlagX86LFence);

  // Global allocable registers.
  uint32_t* gaRegs = _gaRegs;

  if (!func->hasFuncFlag(kFuncFlagIsNaked))
    gaRegs[X86Reg::kClassGp] &= ~Utils::mask(X86Gp::kIdBp);

  // Allowed index registers (GP/XMM/YMM).
  const uint32_t indexMask = Utils::bits(_regCount.getGp()) & ~(Utils::mask(4));

  // --------------------------------------------------------------------------
  // [VI Macros]
  // --------------------------------------------------------------------------

#define RA_POPULATE(NODE) \
  do { \
    X86RAData* raData = newRAData(0); \
    if (!raData) goto NoMem; \
    NODE->setPassData(raData); \
  } while (0)

#define RA_DECLARE() \
  do { \
    X86RegCount tiedCount; \
    X86RegCount tiedIndex; \
    uint32_t tiedTotal = 0; \
    \
    X86RegMask inRegs; \
    X86RegMask outRegs; \
    X86RegMask clobberedRegs; \
    \
    tiedCount.reset(); \
    inRegs.reset(); \
    outRegs.reset(); \
    clobberedRegs.reset()

#define RA_FINALIZE(NODE) \
    { \
      X86RAData* raData = newRAData(tiedTotal); \
      if (!raData) goto NoMem; \
      \
      tiedIndex.indexFromRegCount(tiedCount); \
      raData->tiedCount = tiedCount; \
      raData->tiedIndex = tiedIndex; \
      \
      raData->inRegs = inRegs; \
      raData->outRegs = outRegs; \
      raData->clobberedRegs = clobberedRegs; \
      \
      TiedReg* tied = agTmp; \
      while (tiedTotal) { \
        VirtReg* vreg = tied->vreg; \
        \
        uint32_t _class = vreg->getRegClass(); \
        uint32_t _index = tiedIndex.get(_class); \
        \
        tiedIndex.add(_class); \
        if (tied->inRegs) \
          tied->allocableRegs = tied->inRegs; \
        else if (tied->outPhysId != kInvalidReg) \
          tied->allocableRegs = Utils::mask(tied->outPhysId); \
        else \
          tied->allocableRegs &= ~inRegs.get(_class); \
        \
        vreg->_tied = nullptr; \
        raData->setTiedAt(_index, *tied); \
        \
        tied++; \
        tiedTotal--; \
      } \
      NODE->setPassData(raData); \
     } \
  } while (0)

#define RA_INSERT(REG, LNK, FLAGS, NEW_ALLOCABLE) \
  do { \
    ASMJIT_ASSERT(REG->_tied == nullptr); \
    LNK = &agTmp[tiedTotal++]; \
    LNK->setup(REG, FLAGS, 0, NEW_ALLOCABLE); \
    LNK->refCount++; \
    REG->_tied = LNK; \
    \
    if (assignRAId(REG) != kErrorOk) goto NoMem; \
    tiedCount.add(REG->getRegClass()); \
  } while (0)

#define RA_MERGE(REG, LNK, FLAGS, NEW_ALLOCABLE) \
  do { \
    LNK = REG->_tied; \
    \
    if (!LNK) { \
      LNK = &agTmp[tiedTotal++]; \
      LNK->setup(REG, 0, 0, NEW_ALLOCABLE); \
      REG->_tied = LNK; \
      \
      if (assignRAId(REG) != kErrorOk) goto NoMem; \
      tiedCount.add(REG->getRegClass()); \
    } \
    \
    LNK->flags |= FLAGS; \
    LNK->refCount++; \
  } while (0)

  // --------------------------------------------------------------------------
  // [Loop]
  // --------------------------------------------------------------------------

  do {
_Do:
    while (node_->hasPassData()) {
_NextGroup:
      if (!jLink)
        jLink = _jccList.getFirst();
      else
        jLink = jLink->getNext();

      if (!jLink) goto _Done;
      node_ = X86RAPass_getOppositeJccFlow(static_cast<CBJump*>(jLink->getValue()));
    }

    flowId++;

    next = node_->getNext();
    node_->setFlowId(flowId);
    ASMJIT_TSEC({ this->_traceNode(this, node_, "[F] "); });

    switch (node_->getType()) {
      // ----------------------------------------------------------------------
      // [Align/Embed]
      // ----------------------------------------------------------------------

      case CBNode::kNodeAlign:
      case CBNode::kNodeData:
      default:
        RA_POPULATE(node_);
        break;

      // ----------------------------------------------------------------------
      // [Hint]
      // ----------------------------------------------------------------------

      case CBNode::kNodeHint: {
        CCHint* node = static_cast<CCHint*>(node_);
        RA_DECLARE();

        if (node->getHint() == CCHint::kHintAlloc) {
          uint32_t remain[X86Reg::_kClassManagedCount];
          CCHint* cur = node;

          remain[X86Reg::kClassGp ] = _regCount.getGp() - 1 - func->hasFuncFlag(kFuncFlagIsNaked);
          remain[X86Reg::kClassMm ] = _regCount.getMm();
          remain[X86Reg::kClassK  ] = _regCount.getK();
          remain[X86Reg::kClassXyz] = _regCount.getXyz();

          // Merge as many alloc-hints as possible.
          for (;;) {
            VirtReg* vreg = static_cast<VirtReg*>(cur->getVReg());
            TiedReg* tied = vreg->_tied;

            uint32_t regClass = vreg->getRegClass();
            uint32_t physId = cur->getValue();
            uint32_t regMask = 0;

            // We handle both kInvalidReg and kInvalidValue.
            if (physId < kInvalidReg)
              regMask = Utils::mask(physId);

            if (!tied) {
              if (inRegs.has(regClass, regMask) || remain[regClass] == 0)
                break;
              RA_INSERT(vreg, tied, TiedReg::kRReg, gaRegs[regClass]);

              if (regMask != 0) {
                inRegs.xor_(regClass, regMask);
                tied->inRegs = regMask;
                tied->setInPhysId(physId);
              }
              remain[regClass]--;
            }
            else if (regMask != 0) {
              if (inRegs.has(regClass, regMask) && tied->inRegs != regMask)
                break;

              inRegs.xor_(regClass, tied->inRegs | regMask);
              tied->inRegs = regMask;
              tied->setInPhysId(physId);
            }

            if (cur != node)
              cc()->removeNode(cur);

            cur = static_cast<CCHint*>(node->getNext());
            if (!cur || cur->getType() != CBNode::kNodeHint || cur->getHint() != CCHint::kHintAlloc)
              break;
          }

          next = node->getNext();
        }
        else  {
          VirtReg* vreg = static_cast<VirtReg*>(node->getVReg());
          TiedReg* tied;

          uint32_t flags = 0;
          switch (node->getHint()) {
            case CCHint::kHintSpill       : flags = TiedReg::kRMem | TiedReg::kSpill; break;
            case CCHint::kHintSave        : flags = TiedReg::kRMem                  ; break;
            case CCHint::kHintSaveAndUnuse: flags = TiedReg::kRMem | TiedReg::kUnuse; break;
            case CCHint::kHintUnuse       : flags = TiedReg::kUnuse                 ; break;
          }
          RA_INSERT(vreg, tied, flags, 0);
        }

        RA_FINALIZE(node_);
        break;
      }

      // ----------------------------------------------------------------------
      // [Label]
      // ----------------------------------------------------------------------

      case CBNode::kNodeLabel: {
        RA_POPULATE(node_);
        if (node_ == func->getExitNode()) {
          ASMJIT_PROPAGATE(addReturningNode(node_));
          goto _NextGroup;
        }
        break;
      }

      // ----------------------------------------------------------------------
      // [Inst]
      // ----------------------------------------------------------------------

      case CBNode::kNodeInst: {
        CBInst* node = static_cast<CBInst*>(node_);

        uint32_t instId = node->getInstId();
        uint32_t flags = node->getFlags();

        Operand* opArray = node->getOpArray();
        uint32_t opCount = node->getOpCount();

        RA_DECLARE();
        if (opCount) {
          const X86Inst::ExtendedData& extendedData = _x86InstData[instId].getExtendedData();
          const X86SpecialInst* special = nullptr;

          // Collect instruction flags and merge all 'TiedReg's.
          if (extendedData.isFp())
            flags |= CBNode::kFlagIsFp;

          if (extendedData.isSpecial() && (special = X86SpecialInst_get(instId, opArray, opCount)) != nullptr)
            flags |= CBNode::kFlagIsSpecial;

          uint32_t gpAllowedMask = 0xFFFFFFFF;
          for (uint32_t i = 0; i < opCount; i++) {
            Operand* op = &opArray[i];
            VirtReg* vreg;
            TiedReg* tied;

            if (op->isVirtReg()) {
              vreg = cc()->getVirtRegById(op->getId());
              RA_MERGE(vreg, tied, 0, gaRegs[vreg->getRegClass()] & gpAllowedMask);

              if (static_cast<X86Reg*>(op)->isGpb()) {
                tied->flags |= static_cast<X86Gp*>(op)->isGpbLo() ? TiedReg::kX86GpbLo : TiedReg::kX86GpbHi;
                if (archType == Arch::kTypeX86) {
                  // If a byte register is accessed in 32-bit mode we have to limit
                  // all allocable registers for that variable to eax/ebx/ecx/edx.
                  // Other variables are not affected.
                  tied->allocableRegs &= 0x0F;
                }
                else {
                  // It's fine if lo-byte register is accessed in 64-bit mode;
                  // however, hi-byte has to be checked and if it's used all
                  // registers (GP/XMM) could be only allocated in the lower eight
                  // half. To do that, we patch 'allocableRegs' of all variables
                  // we collected until now and change the allocable restriction
                  // for variables that come after.
                  if (static_cast<X86Gp*>(op)->isGpbHi()) {
                    tied->allocableRegs &= 0x0F;
                    if (gpAllowedMask != 0xFF) {
                      for (uint32_t j = 0; j < i; j++)
                        agTmp[j].allocableRegs &= (agTmp[j].flags & TiedReg::kX86GpbHi) ? 0x0F : 0xFF;
                      gpAllowedMask = 0xFF;
                    }
                  }
                }
              }

              if (special) {
                uint32_t inReg = special[i].inReg;
                uint32_t outReg = special[i].outReg;
                uint32_t c;

                if (static_cast<const X86Reg*>(op)->isGp())
                  c = X86Reg::kClassGp;
                else
                  c = X86Reg::kClassXyz;

                if (inReg != kInvalidReg) {
                  uint32_t mask = Utils::mask(inReg);
                  inRegs.or_(c, mask);
                  tied->inRegs |= mask;
                }

                if (outReg != kInvalidReg) {
                  uint32_t mask = Utils::mask(outReg);
                  outRegs.or_(c, mask);
                  tied->setOutPhysId(outReg);
                }

                tied->flags |= special[i].flags;
              }
              else {
                uint32_t inFlags = TiedReg::kRReg;
                uint32_t outFlags = TiedReg::kWReg;
                uint32_t combinedFlags;

                if (i == 0) {
                  // Read/Write is usually the combination of the first operand.
                  combinedFlags = inFlags | outFlags;

                  if (node->getOptions() & CodeEmitter::kOptionOverwrite) {
                    // Manually forcing write-only.
                    combinedFlags = outFlags;
                  }
                  else if (extendedData.isWO()) {
                    // Write-only instruction.
                    uint32_t movSize = extendedData.getWriteSize();
                    uint32_t regSize = vreg->getSize();

                    // Exception - If the source operand is a memory location
                    // promote move size into 16 bytes.
                    if (extendedData.isZeroIfMem() && opArray[1].isMem())
                      movSize = 16;

                    if (static_cast<const X86Reg*>(op)->isGp()) {
                      uint32_t opSize = static_cast<const X86Reg*>(op)->getSize();

                      // Move size is zero in case that it should be determined
                      // from the destination register.
                      if (movSize == 0)
                        movSize = opSize;

                      // Handle the case that a 32-bit operation in 64-bit mode
                      // always clears the rest of the destination register and
                      // the case that move size is actually greater than or
                      // equal to the size of the variable.
                      if (movSize >= 4 || movSize >= regSize)
                        combinedFlags = outFlags;
                    }
                    else if (movSize >= regSize) {
                      // If move size is greater than or equal to the size of
                      // the variable there is nothing to do, because the move
                      // will overwrite the variable in all cases.
                      combinedFlags = outFlags;
                    }
                  }
                  else if (extendedData.isRO()) {
                    // Comparison/Test instructions don't modify any operand.
                    combinedFlags = inFlags;
                  }
                  else if (instId == X86Inst::kIdImul && opCount == 3) {
                    // Imul.
                    combinedFlags = outFlags;
                  }
                }
                else {
                  // Read-Only is usualy the combination of the second/third/fourth operands.
                  combinedFlags = inFlags;

                  // Idiv is a special instruction, never handled here.
                  ASMJIT_ASSERT(instId != X86Inst::kIdIdiv);

                  // Xchg/Xadd/Imul.
                  if (extendedData.isXchg() || (instId == X86Inst::kIdImul && opCount == 3 && i == 1))
                    combinedFlags = inFlags | outFlags;
                }
                tied->flags |= combinedFlags;
              }
            }
            else if (op->isMem()) {
              X86Mem* m = static_cast<X86Mem*>(op);
              node->setMemOpIndex(i);

              if (m->hasBaseReg()) {
                uint32_t id = m->getBaseId();
                if (cc()->isVirtRegValid(id)) {
                  vreg = cc()->getVirtRegById(id);
                  if (!vreg->isStack()) {
                    RA_MERGE(vreg, tied, 0, gaRegs[vreg->getRegClass()] & gpAllowedMask);
                    if (m->isRegHome()) {
                      uint32_t inFlags = TiedReg::kRMem;
                      uint32_t outFlags = TiedReg::kWMem;
                      uint32_t combinedFlags;

                      if (i == 0) {
                        // Default for the first operand.
                        combinedFlags = inFlags | outFlags;

                        if (extendedData.isWO()) {
                          // Move to memory - setting the right flags is important
                          // as if it's just move to the register. It's just a bit
                          // simpler as there are no special cases.
                          uint32_t movSize = Utils::iMax<uint32_t>(extendedData.getWriteSize(), m->getSize());
                          uint32_t regSize = vreg->getSize();

                          if (movSize >= regSize)
                            combinedFlags = outFlags;
                        }
                        else if (extendedData.isRO()) {
                          // Comparison/Test instructions don't modify any operand.
                          combinedFlags = inFlags;
                        }
                      }
                      else {
                        // Default for the second operand.
                        combinedFlags = inFlags;

                        // Handle Xchg instruction (modifies both operands).
                        if (extendedData.isXchg())
                          combinedFlags = inFlags | outFlags;
                      }

                      tied->flags |= combinedFlags;
                    }
                    else {
                      tied->flags |= TiedReg::kRReg;
                    }
                  }
                }
              }

              if (m->hasIndexReg()) {
                uint32_t id = m->getIndexId();
                if (cc()->isVirtRegValid(id)) {
                  // TODO: AVX vector operands support.
                  // Restrict allocation to all registers except ESP/RSP/R12.
                  vreg = cc()->getVirtRegById(m->getIndexId());
                  RA_MERGE(vreg, tied, 0, gaRegs[X86Reg::kClassGp] & gpAllowedMask);
                  tied->allocableRegs &= indexMask;
                  tied->flags |= TiedReg::kRReg;
                }
              }
            }
          }

          node->setFlags(flags);
          if (tiedTotal) {
            // Handle instructions which result in zeros/ones or nop if used with the
            // same destination and source operand.
            if (tiedTotal == 1 && opCount >= 2 && opArray[0].isVirtReg() && opArray[1].isVirtReg() && !node->hasMemOp())
              X86RAPass_prepareSingleVarInst(instId, &agTmp[0]);
          }
        }
        RA_FINALIZE(node_);

        // Handle conditional/unconditional jump.
        if (node->isJmpOrJcc()) {
          CBJump* jNode = static_cast<CBJump*>(node);
          CBLabel* jTarget = jNode->getTarget();

          // If this jump is unconditional we put next node to unreachable node
          // list so we can eliminate possible dead code. We have to do this in
          // all cases since we are unable to translate without fetch() step.
          //
          // We also advance our node pointer to the target node to simulate
          // natural flow of the function.
          if (jNode->isJmp()) {
            if (!next->hasPassData())
              ASMJIT_PROPAGATE(addUnreachableNode(next));

            // Jump not followed.
            if (!jTarget) {
              ASMJIT_PROPAGATE(addReturningNode(jNode));
              goto _NextGroup;
            }

            node_ = jTarget;
            goto _Do;
          }
          else {
            // Jump not followed.
            if (!jTarget) break;

            if (jTarget->hasPassData()) {
              uint32_t jTargetFlowId = jTarget->getFlowId();

              // Update CBNode::kFlagIsTaken to true if this is a conditional
              // backward jump. This behavior can be overridden by using
              // `X86Inst::kOptionTaken` when the instruction is created.
              if (!jNode->isTaken() && opCount == 1 && jTargetFlowId <= flowId) {
                jNode->_flags |= CBNode::kFlagIsTaken;
              }
            }
            else if (next->hasPassData()) {
              node_ = jTarget;
              goto _Do;
            }
            else {
              ASMJIT_PROPAGATE(addJccNode(jNode));
              node_ = X86RAPass_getJccFlow(jNode);
              goto _Do;
            }
          }
        }
        break;
      }

      // ----------------------------------------------------------------------
      // [Func-Entry]
      // ----------------------------------------------------------------------

      case CBNode::kNodeFunc: {
        ASMJIT_ASSERT(node_ == func);
        X86FuncDecl* decl = func->getDecl();

        RA_DECLARE();
        for (uint32_t i = 0, argCount = decl->getNumArgs(); i < argCount; i++) {
          const FuncInOut& arg = decl->getArg(i);

          VirtReg* vreg = func->getArg(i);
          if (!vreg) continue;

          // Overlapped function arguments.
          if (vreg->_tied)
            return DebugUtils::errored(kErrorOverlappedArgs);

          TiedReg* tied;
          RA_INSERT(vreg, tied, 0, 0);

          uint32_t aType = arg.getTypeId();
          uint32_t typeId = vreg->getTypeId();

          if (arg.hasRegId()) {
            if (x86TypeIdToClass(aType) == vreg->getRegClass()) {
              tied->flags |= TiedReg::kWReg;
              tied->setOutPhysId(arg.getRegId());
            }
            else {
              tied->flags |= TiedReg::kWConv;
            }
          }
          else {
            if ((x86TypeIdToClass(aType) == vreg->getRegClass()) ||
                (typeId == VirtType::kIdX86XmmSs && aType == VirtType::kIdF32) ||
                (typeId == VirtType::kIdX86XmmSd && aType == VirtType::kIdF64)) {
              tied->flags |= TiedReg::kWMem;
            }
            else {
              // TODO: [COMPILER] Not implemented.
              ASMJIT_ASSERT(!"Implemented");
            }
          }
        }
        RA_FINALIZE(node_);
        break;
      }

      // ----------------------------------------------------------------------
      // [End]
      // ----------------------------------------------------------------------

      case CBNode::kNodeSentinel: {
        RA_POPULATE(node_);
        ASMJIT_PROPAGATE(addReturningNode(node_));
        goto _NextGroup;
      }

      // ----------------------------------------------------------------------
      // [Func-Exit]
      // ----------------------------------------------------------------------

      case CBNode::kNodeFuncExit: {
        CCFuncRet* node = static_cast<CCFuncRet*>(node_);
        ASMJIT_PROPAGATE(addReturningNode(node));

        X86FuncDecl* decl = func->getDecl();
        RA_DECLARE();

        if (decl->hasRet()) {
          const FuncInOut& ret = decl->getRet(0);
          uint32_t retClass = x86TypeIdToClass(ret.getTypeId());

          for (uint32_t i = 0; i < 2; i++) {
            Operand_* op = &node->_ret[i];
            if (op->isVirtReg()) {
              VirtReg* vreg = cc()->getVirtRegById(op->getId());
              TiedReg* tied;
              RA_MERGE(vreg, tied, 0, 0);

              if (retClass == vreg->getRegClass()) {
                // TODO: [COMPILER] Fix CCFuncRet fetch.
                tied->flags |= TiedReg::kRReg;
                tied->inRegs = (i == 0) ? Utils::mask(X86Gp::kIdAx) : Utils::mask(X86Gp::kIdDx);
                inRegs.or_(retClass, tied->inRegs);
              }
              else if (retClass == X86Reg::kClassFp) {
                uint32_t fldFlag = ret.getTypeId() == VirtType::kIdF32 ? TiedReg::kX86Fld4 : TiedReg::kX86Fld8;
                tied->flags |= TiedReg::kRMem | fldFlag;
              }
              else {
                // TODO: Fix possible other return type conversions.
                ASMJIT_NOT_REACHED();
              }
            }
          }
        }
        RA_FINALIZE(node_);

        if (!next->hasPassData())
          ASMJIT_PROPAGATE(addUnreachableNode(next));
        goto _NextGroup;
      }

      // ----------------------------------------------------------------------
      // [Func-Call]
      // ----------------------------------------------------------------------

      case CBNode::kNodeCall: {
        X86CCCall* node = static_cast<X86CCCall*>(node_);
        X86FuncDecl* decl = node->getDecl();

        Operand_* target = node->_opArray;
        Operand_* args = node->_args;
        Operand_* rets = node->_ret;

        func->addFuncFlags(kFuncFlagIsCaller);
        func->mergeCallStackSize(node->_x86Decl.getArgStackSize());
        node->_usedArgs = X86RAPass_getUsedArgs(this, node, decl);

        uint32_t i;
        uint32_t argCount = decl->getNumArgs();
        uint32_t sArgCount = 0;
        uint32_t gpAllocableMask = gaRegs[X86Reg::kClassGp] & ~node->_usedArgs.get(X86Reg::kClassGp);

        VirtReg* vreg;
        TiedReg* tied;

        RA_DECLARE();

        // Function-call operand.
        if (target->isVirtReg()) {
          vreg = cc()->getVirtRegById(target->getId());
          RA_MERGE(vreg, tied, 0, 0);

          tied->flags |= TiedReg::kRReg | TiedReg::kRCall;
          if (tied->inRegs == 0)
            tied->allocableRegs |= gpAllocableMask;
        }
        else if (target->isMem()) {
          X86Mem* m = static_cast<X86Mem*>(target);

          if (m->hasBaseReg() &&  Operand::isPackedId(m->getBaseId())) {
            vreg = cc()->getVirtRegById(m->getBaseId());
            if (!vreg->isStack()) {
              RA_MERGE(vreg, tied, 0, 0);
              if (m->isRegHome()) {
                tied->flags |= TiedReg::kRMem | TiedReg::kRCall;
              }
              else {
                tied->flags |= TiedReg::kRReg | TiedReg::kRCall;
                if (tied->inRegs == 0)
                  tied->allocableRegs |= gpAllocableMask;
              }
            }
          }

          if (m->hasIndexReg() && Operand::isPackedId(m->getIndexId())) {
            // Restrict allocation to all registers except ESP/RSP/R12.
            vreg = cc()->getVirtRegById(m->getIndexId());
            RA_MERGE(vreg, tied, 0, 0);

            tied->flags |= TiedReg::kRReg | TiedReg::kRCall;
            if ((tied->inRegs & ~indexMask) == 0)
              tied->allocableRegs &= gpAllocableMask & indexMask;
          }
        }

        // Function-call arguments.
        for (i = 0; i < argCount; i++) {
          Operand_* op = &args[i];
          if (!op->isVirtReg()) continue;

          vreg = cc()->getVirtRegById(op->getId());
          const FuncInOut& arg = decl->getArg(i);

          if (arg.hasRegId()) {
            RA_MERGE(vreg, tied, 0, 0);

            uint32_t argType = arg.getTypeId();
            uint32_t argClass = x86TypeIdToClass(argType);

            if (vreg->getRegClass() == argClass) {
              tied->inRegs |= Utils::mask(arg.getRegId());
              tied->flags |= TiedReg::kRReg | TiedReg::kRFunc;
            }
            else {
              tied->flags |= TiedReg::kRConv | TiedReg::kRFunc;
            }
          }
          // If this is a stack-based argument we insert CCPushArg instead of
          // using TiedReg. It improves the code, because the argument can be
          // moved onto stack as soon as it is ready and the register used by
          // the variable can be reused for something else. It is also much
          // easier to handle argument conversions, because there will be at
          // most only one node per conversion.
          else {
            if (X86RAPass_insertPushArg(this, node, vreg, gaRegs, arg, i, sArgList, sArgCount) != kErrorOk)
              goto NoMem;
          }
        }

        // Function-call return(s).
        for (i = 0; i < 2; i++) {
          Operand_* op = &rets[i];
          if (!op->isVirtReg()) continue;

          const FuncInOut& ret = decl->getRet(i);
          if (ret.hasRegId()) {
            uint32_t retType = ret.getTypeId();
            uint32_t retClass = x86TypeIdToClass(retType);

            vreg = cc()->getVirtRegById(op->getId());
            RA_MERGE(vreg, tied, 0, 0);

            if (vreg->getRegClass() == retClass) {
              tied->setOutPhysId(ret.getRegId());
              tied->flags |= TiedReg::kWReg | TiedReg::kWFunc;
            }
            else {
              tied->flags |= TiedReg::kWConv | TiedReg::kWFunc;
            }
          }
        }

        // Init clobbered.
        clobberedRegs.set(X86Reg::kClassGp , Utils::bits(_regCount.getGp())  & (~decl->getPreserved(X86Reg::kClassGp )));
        clobberedRegs.set(X86Reg::kClassMm , Utils::bits(_regCount.getMm())  & (~decl->getPreserved(X86Reg::kClassMm )));
        clobberedRegs.set(X86Reg::kClassK  , Utils::bits(_regCount.getK())   & (~decl->getPreserved(X86Reg::kClassK  )));
        clobberedRegs.set(X86Reg::kClassXyz, Utils::bits(_regCount.getXyz()) & (~decl->getPreserved(X86Reg::kClassXyz)));

        RA_FINALIZE(node_);
        break;
      }
    }

    node_ = next;
  } while (node_ != stop);

_Done:
  // Mark exit label and end node as fetched, otherwise they can be removed by
  // `removeUnreachableCode()`, which would lead to a crash in some later step.
  node_ = func->getEnd();
  if (!node_->hasPassData()) {
    CBLabel* fExit = func->getExitNode();
    RA_POPULATE(fExit);
    fExit->setFlowId(++flowId);

    RA_POPULATE(node_);
    node_->setFlowId(++flowId);
  }

  ASMJIT_TLOG("[F] ======= Fetch (Done)\n");
  return kErrorOk;

  // --------------------------------------------------------------------------
  // [Failure]
  // --------------------------------------------------------------------------

NoMem:
  ASMJIT_TLOG("[F] ======= Fetch (Out of Memory)\n");
  return DebugUtils::errored(kErrorNoHeapMemory);
}

// ============================================================================
// [asmjit::X86RAPass - Annotate]
// ============================================================================

Error X86RAPass::annotate() {
#if !defined(ASMJIT_DISABLE_LOGGING)
  CCFunc* func = getFunc();

  CBNode* node_ = func;
  CBNode* end = func->getEnd();

  Zone& zone = cc()->_dataAllocator;
  StringBuilderTmp<256> sb;

  uint32_t maxLen = 0;
  while (node_ != end) {
    if (!node_->hasInlineComment()) {
      if (node_->getType() == CBNode::kNodeInst) {
        CBInst* node = static_cast<CBInst*>(node_);
        _formatter.formatInstruction(sb, 0,
          node->getInstId(),
          node->getOptions(),
          node->getOpMask(),
          node->getOpArray(), node->getOpCount());

        node_->setInlineComment(
          static_cast<char*>(zone.dup(sb.getData(), sb.getLength() + 1)));
        maxLen = Utils::iMax<uint32_t>(maxLen, static_cast<uint32_t>(sb.getLength()));

        sb.clear();
      }
    }

    node_ = node_->getNext();
  }
  _annotationLength = maxLen + 1;
#endif // !ASMJIT_DISABLE_LOGGING

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86BaseAlloc]
// ============================================================================

struct X86BaseAlloc {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86BaseAlloc(X86RAPass* context) {
    _context = context;
    _cc = context->cc();
  }
  ASMJIT_INLINE ~X86BaseAlloc() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the context.
  ASMJIT_INLINE X86RAPass* getContext() const { return _context; }
  //! Get the current state (always the same instance as X86RAPass::_x86State).
  ASMJIT_INLINE X86RAState* getState() const { return _context->getState(); }

  //! Get the node.
  ASMJIT_INLINE CBNode* getNode() const { return _node; }

  //! Get TiedReg list (all).
  ASMJIT_INLINE TiedReg* getTiedArray() const { return _tiedArray[0]; }
  //! Get TiedReg list (per class).
  ASMJIT_INLINE TiedReg* getTiedArrayByRC(uint32_t rc) const { return _tiedArray[rc]; }

  //! Get TiedReg count (all).
  ASMJIT_INLINE uint32_t getTiedCount() const { return _tiedTotal; }
  //! Get TiedReg count (per class).
  ASMJIT_INLINE uint32_t getTiedCountByRC(uint32_t rc) const { return _tiedCount.get(rc); }

  //! Get whether all variables of class `c` are done.
  ASMJIT_INLINE bool isTiedDone(uint32_t rc) const { return _tiedDone.get(rc) == _tiedCount.get(rc); }

  //! Get how many variables have been allocated.
  ASMJIT_INLINE uint32_t getTiedDone(uint32_t rc) const { return _tiedDone.get(rc); }
  //! Add to the count of variables allocated.
  ASMJIT_INLINE void addTiedDone(uint32_t rc, uint32_t n = 1) { _tiedDone.add(rc, n); }

  //! Get number of allocable registers per class.
  ASMJIT_INLINE uint32_t getGaRegs(uint32_t rc) const {
    return _context->_gaRegs[rc];
  }

  // --------------------------------------------------------------------------
  // [Init / Cleanup]
  // --------------------------------------------------------------------------

protected:
  // Just to prevent calling these methods by X86RAPass::translate().

  ASMJIT_INLINE void init(CBNode* node, X86RAData* map);
  ASMJIT_INLINE void cleanup();

  // --------------------------------------------------------------------------
  // [Unuse]
  // --------------------------------------------------------------------------

  template<int C>
  ASMJIT_INLINE void unuseBefore();

  template<int C>
  ASMJIT_INLINE void unuseAfter();

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! RA context.
  X86RAPass* _context;
  //! Compiler.
  X86Compiler* _cc;

  //! Node.
  CBNode* _node;

  //! Register allocator (RA) data.
  X86RAData* _raData;
  //! TiedReg list (per register class).
  TiedReg* _tiedArray[X86Reg::_kClassManagedCount];

  //! Count of all TiedReg's.
  uint32_t _tiedTotal;

  //! TiedReg's total counter.
  X86RegCount _tiedCount;
  //! TiedReg's done counter.
  X86RegCount _tiedDone;
};

// ============================================================================
// [asmjit::X86BaseAlloc - Init / Cleanup]
// ============================================================================

ASMJIT_INLINE void X86BaseAlloc::init(CBNode* node, X86RAData* raData) {
  _node = node;
  _raData = raData;

  // We have to set the correct cursor in case any instruction is emitted
  // during the allocation phase; it has to be emitted before the current
  // instruction.
  _cc->_setCursor(node->getPrev());

  // Setup the lists of variables.
  {
    TiedReg* tied = raData->getTiedArray();
    _tiedArray[X86Reg::kClassGp ] = tied;
    _tiedArray[X86Reg::kClassMm ] = tied + raData->getTiedStart(X86Reg::kClassMm );
    _tiedArray[X86Reg::kClassK  ] = tied + raData->getTiedStart(X86Reg::kClassK  );
    _tiedArray[X86Reg::kClassXyz] = tied + raData->getTiedStart(X86Reg::kClassXyz);
  }

  // Setup counters.
  _tiedTotal = raData->tiedTotal;
  _tiedCount = raData->tiedCount;
  _tiedDone.reset();

  // Connect VREG->TIED.
  for (uint32_t i = 0; i < _tiedTotal; i++) {
    TiedReg* tied = &_tiedArray[0][i];
    VirtReg* vreg = tied->vreg;
    vreg->_tied = tied;
  }
}

ASMJIT_INLINE void X86BaseAlloc::cleanup() {
  // Disconnect VREG->TIED.
  for (uint32_t i = 0; i < _tiedTotal; i++) {
    TiedReg* tied = &_tiedArray[0][i];
    VirtReg* vreg = tied->vreg;
    vreg->_tied = nullptr;
  }
}

// ============================================================================
// [asmjit::X86BaseAlloc - Unuse]
// ============================================================================

template<int C>
ASMJIT_INLINE void X86BaseAlloc::unuseBefore() {
  TiedReg* tiedArray = getTiedArrayByRC(C);
  uint32_t tiedCount = getTiedCountByRC(C);

  const uint32_t checkFlags =
    TiedReg::kXReg  |
    TiedReg::kRMem  |
    TiedReg::kRFunc |
    TiedReg::kRCall |
    TiedReg::kRConv ;

  for (uint32_t i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedArray[i];
    if ((tied->flags & checkFlags) == TiedReg::kWReg)
      _context->unuse<C>(tied->vreg);
  }
}

template<int C>
ASMJIT_INLINE void X86BaseAlloc::unuseAfter() {
  TiedReg* tiedArray = getTiedArrayByRC(C);
  uint32_t tiedCount = getTiedCountByRC(C);

  for (uint32_t i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedArray[i];
    if (tied->flags & TiedReg::kUnuse)
      _context->unuse<C>(tied->vreg);
  }
}

// ============================================================================
// [asmjit::X86VarAlloc]
// ============================================================================

//! \internal
//!
//! Register allocator context (asm instructions).
struct X86VarAlloc : public X86BaseAlloc {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86VarAlloc(X86RAPass* context) : X86BaseAlloc(context) {}
  ASMJIT_INLINE ~X86VarAlloc() {}

  // --------------------------------------------------------------------------
  // [Run]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Error run(CBNode* node);

  // --------------------------------------------------------------------------
  // [Init / Cleanup]
  // --------------------------------------------------------------------------

protected:
  // Just to prevent calling these methods by X86RAPass::translate().

  ASMJIT_INLINE void init(CBNode* node, X86RAData* map);
  ASMJIT_INLINE void cleanup();

  // --------------------------------------------------------------------------
  // [Plan / Spill / Alloc]
  // --------------------------------------------------------------------------

  template<int C>
  ASMJIT_INLINE void plan();

  template<int C>
  ASMJIT_INLINE void spill();

  template<int C>
  ASMJIT_INLINE void alloc();

  // --------------------------------------------------------------------------
  // [GuessAlloc / GuessSpill]
  // --------------------------------------------------------------------------

  //! Guess which register is the best candidate for `vreg` from `allocableRegs`.
  //!
  //! The guess is based on looking ahead and inspecting register allocator
  //! instructions. The main reason is to prevent allocation to a register
  //! which is needed by next instruction(s). The guess look tries to go as far
  //! as possible, after the remaining registers are zero, the mask of previous
  //! registers (called 'safeRegs') is returned.
  template<int C>
  ASMJIT_INLINE uint32_t guessAlloc(VirtReg* vreg, uint32_t allocableRegs);

  //! Guess whether to move the given `vreg` instead of spill.
  template<int C>
  ASMJIT_INLINE uint32_t guessSpill(VirtReg* vreg, uint32_t allocableRegs);

  // --------------------------------------------------------------------------
  // [Modified]
  // --------------------------------------------------------------------------

  template<int C>
  ASMJIT_INLINE void modified();

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Will alloc to these registers.
  X86RegMask _willAlloc;
  //! Will spill these registers.
  X86RegMask _willSpill;
};

// ============================================================================
// [asmjit::X86VarAlloc - Run]
// ============================================================================

ASMJIT_INLINE Error X86VarAlloc::run(CBNode* node_) {
  // Initialize.
  X86RAData* raData = node_->getPassData<X86RAData>();
  // Initialize the allocator; connect Vd->Va.
  init(node_, raData);

  if (raData->tiedTotal != 0) {
    // Unuse overwritten variables.
    unuseBefore<X86Reg::kClassGp>();
    unuseBefore<X86Reg::kClassMm>();
    unuseBefore<X86Reg::kClassXyz>();

    // Plan the allocation. Planner assigns input/output registers for each
    // variable and decides whether to allocate it in register or stack.
    plan<X86Reg::kClassGp>();
    plan<X86Reg::kClassMm>();
    plan<X86Reg::kClassXyz>();

    // Spill all variables marked by plan().
    spill<X86Reg::kClassGp>();
    spill<X86Reg::kClassMm>();
    spill<X86Reg::kClassXyz>();

    // Alloc all variables marked by plan().
    alloc<X86Reg::kClassGp>();
    alloc<X86Reg::kClassMm>();
    alloc<X86Reg::kClassXyz>();

    // Translate node operands.
    if (node_->getType() == CBNode::kNodeInst) {
      CBInst* node = static_cast<CBInst*>(node_);
      ASMJIT_PROPAGATE(X86RAPass_translateOperands(_context, node->getOpArray(), node->getOpCount()));
    }
    else if (node_->getType() == CBNode::kNodePushArg) {
      CCPushArg* node = static_cast<CCPushArg*>(node_);

      X86CCCall* call = static_cast<X86CCCall*>(node->getCall());
      X86FuncDecl* decl = call->getDecl();

      uint32_t argIndex = 0;
      uint32_t argMask = node->_args;

      VirtReg* srcReg = node->getSrcReg();
      VirtReg* cvtReg = node->getCvtReg();

      // Convert first.
      ASMJIT_ASSERT(srcReg->getPhysId() != kInvalidReg);

      if (cvtReg) {
        ASMJIT_ASSERT(cvtReg->getPhysId() != kInvalidReg);
        _context->emitConvertVarToVar(
          cvtReg->getTypeId(), cvtReg->getPhysId(),
          srcReg->getTypeId(), srcReg->getPhysId());
        srcReg = cvtReg;
      }

      while (argMask != 0) {
        if (argMask & 0x1) {
          FuncInOut& arg = decl->getArg(argIndex);
          ASMJIT_ASSERT(arg.hasStackOffset());

          X86Mem dst = x86::ptr(_context->_zsp, -static_cast<int>(_context->getGpSize()) + arg.getStackOffset());
          _context->emitMoveVarOnStack(arg.getTypeId(), &dst, srcReg->getTypeId(), srcReg->getPhysId());
        }

        argIndex++;
        argMask >>= 1;
      }
    }

    // Mark variables as modified.
    modified<X86Reg::kClassGp>();
    modified<X86Reg::kClassMm>();
    modified<X86Reg::kClassXyz>();

    // Cleanup; disconnect Vd->Va.
    cleanup();

    // Update clobbered mask.
    _context->_clobberedRegs.or_(_willAlloc);
  }

  // Update clobbered mask.
  _context->_clobberedRegs.or_(raData->clobberedRegs);

  // Unuse.
  if (raData->tiedTotal != 0) {
    unuseAfter<X86Reg::kClassGp>();
    unuseAfter<X86Reg::kClassMm>();
    unuseAfter<X86Reg::kClassXyz>();
  }

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86VarAlloc - Init / Cleanup]
// ============================================================================

ASMJIT_INLINE void X86VarAlloc::init(CBNode* node, X86RAData* raData) {
  X86BaseAlloc::init(node, raData);

  // These will block planner from assigning them during planning. Planner will
  // add more registers when assigning registers to variables that don't need
  // any specific register.
  _willAlloc = raData->inRegs;
  _willAlloc.or_(raData->outRegs);
  _willSpill.reset();
}

ASMJIT_INLINE void X86VarAlloc::cleanup() {
  X86BaseAlloc::cleanup();
}

// ============================================================================
// [asmjit::X86VarAlloc - Plan / Spill / Alloc]
// ============================================================================

template<int C>
ASMJIT_INLINE void X86VarAlloc::plan() {
  if (isTiedDone(C)) return;

  uint32_t i;
  uint32_t willAlloc = _willAlloc.get(C);
  uint32_t willFree = 0;

  TiedReg* tiedArray = getTiedArrayByRC(C);
  uint32_t tiedCount = getTiedCountByRC(C);
  X86RAState* state = getState();

  // Calculate 'willAlloc' and 'willFree' masks based on mandatory masks.
  for (i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedArray[i];
    VirtReg* vreg = tied->vreg;

    uint32_t vaFlags = tied->flags;
    uint32_t physId = vreg->getPhysId();
    uint32_t regMask = (physId != kInvalidReg) ? Utils::mask(physId) : 0;

    if ((vaFlags & TiedReg::kXReg) != 0) {
      // Planning register allocation. First check whether the variable is
      // already allocated in register and if it can stay allocated there.
      //
      // The following conditions may happen:
      //
      // a) Allocated register is one of the mandatoryRegs.
      // b) Allocated register is one of the allocableRegs.
      uint32_t mandatoryRegs = tied->inRegs;
      uint32_t allocableRegs = tied->allocableRegs;

      ASMJIT_TLOG("[RA-PLAN] %s (%s)\n", vreg->getName(),
        (vaFlags & TiedReg::kXReg) == TiedReg::kWReg ? "R-Reg" : "X-Reg");

      ASMJIT_TLOG("[RA-PLAN] RegMask=%08X Mandatory=%08X Allocable=%08X\n",
        regMask, mandatoryRegs, allocableRegs);

      if (regMask != 0) {
        // Special path for planning output-only registers.
        if ((vaFlags & TiedReg::kXReg) == TiedReg::kWReg) {
          uint32_t outPhysId = tied->outPhysId;
          mandatoryRegs = (outPhysId != kInvalidReg) ? Utils::mask(outPhysId) : 0;

          if ((mandatoryRegs | allocableRegs) & regMask) {
            tied->setOutPhysId(physId);
            tied->flags |= TiedReg::kWDone;

            if (mandatoryRegs & regMask) {
              // Case 'a' - 'willAlloc' contains initially all inRegs from all TiedReg's.
              ASMJIT_ASSERT((willAlloc & regMask) != 0);
            }
            else {
              // Case 'b'.
              tied->setOutPhysId(physId);
              willAlloc |= regMask;
            }

            ASMJIT_TLOG("[RA-PLAN] WillAlloc\n");
            addTiedDone(C);

            continue;
          }
        }
        else {
          if ((mandatoryRegs | allocableRegs) & regMask) {
            tied->setInPhysId(physId);
            tied->flags |= TiedReg::kRDone;

            if (mandatoryRegs & regMask) {
              // Case 'a' - 'willAlloc' contains initially all inRegs from all TiedReg's.
              ASMJIT_ASSERT((willAlloc & regMask) != 0);
            }
            else {
              // Case 'b'.
              tied->inRegs |= regMask;
              willAlloc |= regMask;
            }

            ASMJIT_TLOG("[RA-PLAN] WillAlloc\n");
            addTiedDone(C);

            continue;
          }
        }

        // Trace it here so we don't pollute log by `WillFree` of zero regMask.
        ASMJIT_TLOG("[RA-PLAN] WillFree\n");
      }

      // Variable is not allocated or allocated in register that doesn't
      // match inRegs or allocableRegs. The next step is to pick the best
      // register for this variable. If `inRegs` contains any register the
      // decision is simple - we have to follow, in other case will use
      // the advantage of `guessAlloc()` to find a register (or registers)
      // by looking ahead. But the best way to find a good register is not
      // here since now we have no information about the registers that
      // will be freed. So instead of finding register here, we just mark
      // the current register (if variable is allocated) as `willFree` so
      // the planner can use this information in the second step to plan the
      // allocation as a whole.
      willFree |= regMask;
      continue;
    }
    else {
      // Memory access - if variable is allocated it has to be freed.
      ASMJIT_TLOG("[RA-PLAN] %s (Memory)\n", vreg->getName());

      if (regMask != 0) {
        ASMJIT_TLOG("[RA-PLAN] WillFree\n");
        willFree |= regMask;
        continue;
      }
      else {
        ASMJIT_TLOG("[RA-PLAN] Done\n");
        tied->flags |= TiedReg::kRDone;
        addTiedDone(C);
        continue;
      }
    }
  }

  // Occupied registers without 'willFree' registers; contains basically
  // all the registers we can use to allocate variables without inRegs
  // speficied.
  uint32_t occupied = state->_occupied.get(C) & ~willFree;
  uint32_t willSpill = 0;

  // Find the best registers for variables that are not allocated yet.
  for (i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedArray[i];
    VirtReg* vreg = tied->vreg;
    uint32_t vaFlags = tied->flags;

    if ((vaFlags & TiedReg::kXReg) != 0) {
      if ((vaFlags & TiedReg::kXReg) == TiedReg::kWReg) {
        if (vaFlags & TiedReg::kWDone)
          continue;

        // Skip all registers that have assigned outPhysId. Spill if occupied.
        if (tied->hasOutPhysId()) {
          uint32_t outRegs = Utils::mask(tied->outPhysId);
          willSpill |= occupied & outRegs;
          continue;
        }
      }
      else {
        if (vaFlags & TiedReg::kRDone)
          continue;

        // We skip all registers that have assigned inPhysId, indicates that
        // the register to allocate in is known.
        if (tied->hasInPhysId()) {
          uint32_t inRegs = tied->inRegs;
          willSpill |= occupied & inRegs;
          continue;
        }
      }

      uint32_t m = tied->inRegs;
      if (tied->hasOutPhysId())
        m |= Utils::mask(tied->outPhysId);

      m = tied->allocableRegs & ~(willAlloc ^ m);
      m = guessAlloc<C>(vreg, m);
      ASMJIT_ASSERT(m != 0);

      uint32_t candidateRegs = m & ~occupied;
      uint32_t homeMask = vreg->getHomeMask();

      uint32_t physId;
      uint32_t regMask;

      if (candidateRegs == 0) {
        candidateRegs = m & occupied & ~state->_modified.get(C);
        if (candidateRegs == 0)
          candidateRegs = m;
      }
      if (candidateRegs & homeMask) candidateRegs &= homeMask;

      physId = Utils::findFirstBit(candidateRegs);
      regMask = Utils::mask(physId);

      if ((vaFlags & TiedReg::kXReg) == TiedReg::kWReg) {
        tied->setOutPhysId(physId);
      }
      else {
        tied->setInPhysId(physId);
        tied->inRegs = regMask;
      }

      willAlloc |= regMask;
      willSpill |= regMask & occupied;
      willFree  &=~regMask;
      occupied  |= regMask;

      continue;
    }
    else if ((vaFlags & TiedReg::kXMem) != 0) {
      uint32_t physId = vreg->getPhysId();
      if (physId != kInvalidReg && (vaFlags & TiedReg::kXMem) != TiedReg::kWMem) {
        willSpill |= Utils::mask(physId);
      }
    }
  }

  // Set calculated masks back to the allocator; needed by spill() and alloc().
  _willSpill.set(C, willSpill);
  _willAlloc.set(C, willAlloc);
}

template<int C>
ASMJIT_INLINE void X86VarAlloc::spill() {
  uint32_t m = _willSpill.get(C);
  uint32_t i = static_cast<uint32_t>(0) - 1;
  if (m == 0) return;

  X86RAState* state = getState();
  VirtReg** vregs = state->getListByRC(C);

  // Available registers for decision if move has any benefit over spill.
  uint32_t availableRegs = getGaRegs(C) & ~(state->_occupied.get(C) | m | _willAlloc.get(C));

  do {
    // We always advance one more to destroy the bit that we have found.
    uint32_t bitIndex = Utils::findFirstBit(m) + 1;

    i += bitIndex;
    m >>= bitIndex;

    VirtReg* vreg = vregs[i];
    ASMJIT_ASSERT(vreg);

    TiedReg* tied = vreg->_tied;
    ASMJIT_ASSERT(!tied || (tied->flags & TiedReg::kXReg) == 0);

    if (vreg->isModified() && availableRegs) {
      // Don't check for alternatives if the variable has to be spilled.
      if (!tied || (tied->flags & TiedReg::kSpill) == 0) {
        uint32_t altRegs = guessSpill<C>(vreg, availableRegs);

        if (altRegs != 0) {
          uint32_t physId = Utils::findFirstBit(altRegs);
          uint32_t regMask = Utils::mask(physId);

          _context->move<C>(vreg, physId);
          availableRegs ^= regMask;
          continue;
        }
      }
    }

    _context->spill<C>(vreg);
  } while (m != 0);
}

template<int C>
ASMJIT_INLINE void X86VarAlloc::alloc() {
  if (isTiedDone(C)) return;

  uint32_t i;
  bool didWork;

  TiedReg* tiedArray = getTiedArrayByRC(C);
  uint32_t tiedCount = getTiedCountByRC(C);

  // Alloc `in` regs.
  do {
    didWork = false;
    for (i = 0; i < tiedCount; i++) {
      TiedReg* aTied = &tiedArray[i];
      VirtReg* aVReg = aTied->vreg;

      if ((aTied->flags & (TiedReg::kRReg | TiedReg::kRDone)) != TiedReg::kRReg)
        continue;

      uint32_t aPhysId = aVReg->getPhysId();
      uint32_t bPhysId = aTied->inPhysId;

      // Shouldn't be the same.
      ASMJIT_ASSERT(aPhysId != bPhysId);

      VirtReg* bVReg = getState()->getListByRC(C)[bPhysId];
      if (bVReg) {
        // Gp registers only - Swap two registers if we can solve two
        // allocation tasks by a single 'xchg' instruction, swapping
        // two registers required by the instruction/node or one register
        // required with another non-required.
        if (C == X86Reg::kClassGp && aPhysId != kInvalidReg) {
          TiedReg* bTied = bVReg->_tied;
          _context->swapGp(aVReg, bVReg);

          aTied->flags |= TiedReg::kRDone;
          addTiedDone(C);

          // Doublehit, two registers allocated by a single swap.
          if (bTied && bTied->inPhysId == aPhysId) {
            bTied->flags |= TiedReg::kRDone;
            addTiedDone(C);
          }

          didWork = true;
          continue;
        }
      }
      else if (aPhysId != kInvalidReg) {
        _context->move<C>(aVReg, bPhysId);

        aTied->flags |= TiedReg::kRDone;
        addTiedDone(C);

        didWork = true;
        continue;
      }
      else {
        _context->alloc<C>(aVReg, bPhysId);

        aTied->flags |= TiedReg::kRDone;
        addTiedDone(C);

        didWork = true;
        continue;
      }
    }
  } while (didWork);

  // Alloc 'out' regs.
  for (i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedArray[i];
    VirtReg* vreg = tied->vreg;

    if ((tied->flags & (TiedReg::kXReg | TiedReg::kWDone)) != TiedReg::kWReg)
      continue;

    uint32_t physId = tied->outPhysId;
    ASMJIT_ASSERT(physId != kInvalidReg);

    if (vreg->getPhysId() != physId) {
      ASMJIT_ASSERT(getState()->getListByRC(C)[physId] == nullptr);
      _context->attach<C>(vreg, physId, false);
    }

    tied->flags |= TiedReg::kWDone;
    addTiedDone(C);
  }
}

// ============================================================================
// [asmjit::X86VarAlloc - GuessAlloc / GuessSpill]
// ============================================================================

#if 0
// TODO: This works, but should be improved a bit. The idea is to follow code
// flow and to restrict the possible registers where to allocate as much as
// possible so we won't allocate to a register which is home of some variable
// that's gonna be used together with `vreg`. The previous implementation didn't
// care about it and produced suboptimal results even in code which didn't
// require any allocs & spills.
enum { kMaxGuessFlow = 10 };

struct GuessFlowData {
  ASMJIT_INLINE void init(CBNode* node, uint32_t counter, uint32_t safeRegs) {
    _node = node;
    _counter = counter;
    _safeRegs = safeRegs;
  }

  //! Node to start.
  CBNode* _node;
  //! Number of instructions processed from the beginning.
  uint32_t _counter;
  //! Safe registers, which can be used for the allocation.
  uint32_t _safeRegs;
};

template<int C>
ASMJIT_INLINE uint32_t X86VarAlloc::guessAlloc(VirtReg* vreg, uint32_t allocableRegs) {
  ASMJIT_TLOG("[RA-GUESS] === %s (Input=%08X) ===\n", vreg->getName(), allocableRegs);
  ASMJIT_ASSERT(allocableRegs != 0);

  return allocableRegs;

  // Stop now if there is only one bit (register) set in `allocableRegs` mask.
  uint32_t safeRegs = allocableRegs;
  if (Utils::isPowerOf2(safeRegs))
    return safeRegs;

  uint32_t counter = 0;
  uint32_t maxInst = _cc->getMaxLookAhead();

  uint32_t raId = vreg->_raId;
  uint32_t localToken = _cc->_generateUniqueToken();

  uint32_t gfIndex = 0;
  GuessFlowData gfArray[kMaxGuessFlow];

  CBNode* node = _node;

  // Mark this node and also exit node, it will terminate the loop if encountered.
  node->setTokenId(localToken);
  _context->getFunc()->getExitNode()->setTokenId(localToken);

  // TODO: I don't like this jump, maybe some refactor would help to eliminate it.
  goto _Advance;

  // Look ahead and calculate mask of special registers on both - input/output.
  for (;;) {
    do {
      ASMJIT_TSEC({
        _context->_traceNode(_context, node, "  ");
      });

      // Terminate if we have seen this node already.
      if (node->matchesToken(localToken))
        break;

      node->setTokenId(localToken);
      counter++;

      // Terminate if the variable is dead here.
      if (node->hasLiveness() && !node->getLiveness()->getBit(raId)) {
        ASMJIT_TLOG("[RA-GUESS] %s (Terminating, Not alive here)\n", vreg->getName());
        break;
      }

      if (node->hasState()) {
        // If this node contains a state, we have to consider only the state
        // and then we can terminate safely - this happens if we jumped to a
        // label that is backward (i.e. start of the loop). If we survived
        // the liveness check it means that the variable is actually used.
        X86RAState* state = node->getState<X86RAState>();
        uint32_t homeRegs = 0;
        uint32_t tempRegs = 0;

        VirtReg** vregs = state->getListByRC(C);
        uint32_t count = _cc->getRegCount().get(C);

        for (uint32_t i = 0; i < count; i++) {
          if (vregs[i]) tempRegs |= Utils::mask(i);
          if (vregs[i] == vreg) homeRegs = Utils::mask(i);
        }

        tempRegs = safeRegs & ~tempRegs;
        if (!tempRegs)
          goto _Done;
        safeRegs = tempRegs;

        tempRegs = safeRegs & homeRegs;
        if (!tempRegs)
          goto _Done;
        safeRegs = tempRegs;

        goto _Done;
      }
      else {
        // Process the current node if it has any variables associated in.
        X86RAData* raData = node->getMap<X86RAData>();
        if (raData) {
          TiedReg* tiedArray = raData->getTiedArrayByRC(C);
          uint32_t tiedCount = raData->getTiedCountByRC(C);

          uint32_t homeRegs = 0;
          uint32_t tempRegs = safeRegs;
          bool found = false;

          for (uint32_t tiedIndex = 0; tiedIndex < tiedCount; tiedIndex++) {
            TiedReg* tied = &tiedArray[tiedIndex];

            if (tied->vreg == vreg) {
              found = true;

              // Terminate if the variable is overwritten here.
              if (!(tied->getFlags() & TiedReg::kRAll))
                goto _Done;

              uint32_t mask = tied->allocableRegs;
              if (mask != 0) {
                tempRegs &= mask;
                if (!tempRegs)
                  goto _Done;
                safeRegs = tempRegs;
              }

              mask = tied->inRegs;
              if (mask != 0) {
                tempRegs &= mask;
                if (!tempRegs)
                  goto _Done;

                safeRegs = tempRegs;
                goto _Done;
              }
            }
            else {
              // It happens often that one variable is used across many blocks of
              // assembly code. It can sometimes cause one variable to be allocated
              // in a different register, which can cause state switch to generate
              // moves in case of jumps and state intersections. We try to prevent
              // this case by also considering variables' home registers.
              homeRegs |= tied->vreg->getHomeMask();
            }
          }

          tempRegs &= ~(map->_outRegs.get(C) | map->_clobberedRegs.get(C));
          if (!found)
            tempRegs &= ~map->_inRegs.get(C);

          if (!tempRegs)
            goto _Done;
          safeRegs = tempRegs;

          if (homeRegs) {
            tempRegs = safeRegs & ~homeRegs;
            if (!tempRegs)
              goto _Done;
            safeRegs = tempRegs;
          }
        }
      }

_Advance:
      // Terminate if this is a return node.
      if (node->hasFlag(CBNode::kFlagIsRet))
        goto _Done;

      // Advance on non-conditional jump.
      if (node->hasFlag(CBNode::kFlagIsJmp)) {
        // Stop on a jump that is not followed.
        node = static_cast<CBJump*>(node)->getTarget();
        if (!node) break;
        continue;
      }

      // Split flow on a conditional jump.
      if (node->hasFlag(CBNode::kFlagIsJcc)) {
        // Put the next node on the stack and follow the target if possible.
        CBNode* next = node->getNext();
        if (next && gfIndex < kMaxGuessFlow)
          gfArray[gfIndex++].init(next, counter, safeRegs);

        node = static_cast<CBJump*>(node)->getTarget();
        if (!node) break;
        continue;
      }

      node = node->getNext();
      ASMJIT_ASSERT(node != nullptr);
    } while (counter < maxInst);

_Done:
    for (;;) {
      if (gfIndex == 0)
        goto _Ret;

      GuessFlowData* data = &gfArray[--gfIndex];
      node = data->_node;
      counter = data->_counter;

      uint32_t tempRegs = safeRegs & data->_safeRegs;
      if (!tempRegs)
        continue;

      safeRegs = tempRegs;
      break;
    }
  }

_Ret:
  ASMJIT_TLOG("[RA-GUESS] === %s (Output=%08X) ===\n", vreg->getName(), safeRegs);
  return safeRegs;
}
#endif

template<int C>
ASMJIT_INLINE uint32_t X86VarAlloc::guessAlloc(VirtReg* vreg, uint32_t allocableRegs) {
  ASMJIT_ASSERT(allocableRegs != 0);

  // Stop now if there is only one bit (register) set in `allocableRegs` mask.
  if (Utils::isPowerOf2(allocableRegs)) return allocableRegs;

  uint32_t raId = vreg->_raId;
  uint32_t safeRegs = allocableRegs;

  uint32_t i;
  uint32_t maxLookAhead = _cc->getMaxLookAhead();

  // Look ahead and calculate mask of special registers on both - input/output.
  CBNode* node = _node;
  for (i = 0; i < maxLookAhead; i++) {
    X86RAData* raData = node->getPassData<X86RAData>();
    RABits* liveness = raData ? raData->liveness : static_cast<RABits*>(nullptr);

    // If the variable becomes dead it doesn't make sense to continue.
    if (liveness && !liveness->getBit(raId)) break;

    // Stop on `CBSentinel` and `CCFuncRet`.
    if (node->hasFlag(CBNode::kFlagIsRet)) break;

    // Stop on conditional jump, we don't follow them.
    if (node->hasFlag(CBNode::kFlagIsJcc)) break;

    // Advance on non-conditional jump.
    if (node->hasFlag(CBNode::kFlagIsJmp)) {
      node = static_cast<CBJump*>(node)->getTarget();
      // Stop on jump that is not followed.
      if (!node) break;
    }

    node = node->getNext();
    ASMJIT_ASSERT(node != nullptr);

    raData = node->getPassData<X86RAData>();
    if (raData) {
      TiedReg* tied = raData->findTiedByRC(C, vreg);
      uint32_t mask;

      if (tied) {
        // If the variable is overwritten it doesn't mase sense to continue.
        if ((tied->flags & TiedReg::kRAll) == 0)
          break;

        mask = tied->allocableRegs;
        if (mask != 0) {
          allocableRegs &= mask;
          if (allocableRegs == 0) break;
          safeRegs = allocableRegs;
        }

        mask = tied->inRegs;
        if (mask != 0) {
          allocableRegs &= mask;
          if (allocableRegs == 0) break;
          safeRegs = allocableRegs;
          break;
        }

        allocableRegs &= ~(raData->outRegs.get(C) | raData->clobberedRegs.get(C));
        if (allocableRegs == 0) break;
      }
      else {
        allocableRegs &= ~(raData->inRegs.get(C) | raData->outRegs.get(C) | raData->clobberedRegs.get(C));
        if (allocableRegs == 0) break;
      }

      safeRegs = allocableRegs;
    }
  }

  return safeRegs;
}

template<int C>
ASMJIT_INLINE uint32_t X86VarAlloc::guessSpill(VirtReg* vreg, uint32_t allocableRegs) {
  ASMJIT_ASSERT(allocableRegs != 0);

  return 0;
}

// ============================================================================
// [asmjit::X86VarAlloc - Modified]
// ============================================================================

template<int C>
ASMJIT_INLINE void X86VarAlloc::modified() {
  TiedReg* tiedArray = getTiedArrayByRC(C);
  uint32_t tiedCount = getTiedCountByRC(C);

  for (uint32_t i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedArray[i];

    if (tied->flags & TiedReg::kWReg) {
      VirtReg* vreg = tied->vreg;

      uint32_t physId = vreg->getPhysId();
      uint32_t regMask = Utils::mask(physId);

      vreg->setModified(true);
      _context->_x86State._modified.or_(C, regMask);
    }
  }
}

// ============================================================================
// [asmjit::X86CallAlloc]
// ============================================================================

//! \internal
//!
//! Register allocator context (function call).
struct X86CallAlloc : public X86BaseAlloc {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86CallAlloc(X86RAPass* context) : X86BaseAlloc(context) {}
  ASMJIT_INLINE ~X86CallAlloc() {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the node.
  ASMJIT_INLINE X86CCCall* getNode() const { return static_cast<X86CCCall*>(_node); }

  // --------------------------------------------------------------------------
  // [Run]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Error run(X86CCCall* node);

  // --------------------------------------------------------------------------
  // [Init / Cleanup]
  // --------------------------------------------------------------------------

protected:
  // Just to prevent calling these methods from X86RAPass::translate().
  ASMJIT_INLINE void init(X86CCCall* node, X86RAData* raData);
  ASMJIT_INLINE void cleanup();

  // --------------------------------------------------------------------------
  // [Plan / Alloc / Spill / Move]
  // --------------------------------------------------------------------------

  template<int C>
  ASMJIT_INLINE void plan();

  template<int C>
  ASMJIT_INLINE void spill();

  template<int C>
  ASMJIT_INLINE void alloc();

  // --------------------------------------------------------------------------
  // [AllocImmsOnStack]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void allocImmsOnStack();

  // --------------------------------------------------------------------------
  // [Duplicate]
  // --------------------------------------------------------------------------

  template<int C>
  ASMJIT_INLINE void duplicate();

  // --------------------------------------------------------------------------
  // [GuessAlloc / GuessSpill]
  // --------------------------------------------------------------------------

  template<int C>
  ASMJIT_INLINE uint32_t guessAlloc(VirtReg* vreg, uint32_t allocableRegs);

  template<int C>
  ASMJIT_INLINE uint32_t guessSpill(VirtReg* vreg, uint32_t allocableRegs);

  // --------------------------------------------------------------------------
  // [Save]
  // --------------------------------------------------------------------------

  template<int C>
  ASMJIT_INLINE void save();

  // --------------------------------------------------------------------------
  // [Clobber]
  // --------------------------------------------------------------------------

  template<int C>
  ASMJIT_INLINE void clobber();

  // --------------------------------------------------------------------------
  // [Ret]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void ret();

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Will alloc to these registers.
  X86RegMask _willAlloc;
  //! Will spill these registers.
  X86RegMask _willSpill;
};

// ============================================================================
// [asmjit::X86CallAlloc - Run]
// ============================================================================

ASMJIT_INLINE Error X86CallAlloc::run(X86CCCall* node) {
  // Initialize the allocator; prepare basics and connect Vd->Va.
  X86RAData* raData = node->getPassData<X86RAData>();
  init(node, raData);

  // Plan register allocation. Planner is only able to assign one register per
  // variable. If any variable is used multiple times it will be handled later.
  plan<X86Reg::kClassGp >();
  plan<X86Reg::kClassMm >();
  plan<X86Reg::kClassXyz>();

  // Spill.
  spill<X86Reg::kClassGp >();
  spill<X86Reg::kClassMm >();
  spill<X86Reg::kClassXyz>();

  // Alloc.
  alloc<X86Reg::kClassGp >();
  alloc<X86Reg::kClassMm >();
  alloc<X86Reg::kClassXyz>();

  // Unuse clobbered registers that are not used to pass function arguments and
  // save variables used to pass function arguments that will be reused later on.
  save<X86Reg::kClassGp >();
  save<X86Reg::kClassMm >();
  save<X86Reg::kClassXyz>();

  // Allocate immediates in registers and on the stack.
  allocImmsOnStack();

  // Duplicate.
  duplicate<X86Reg::kClassGp >();
  duplicate<X86Reg::kClassMm >();
  duplicate<X86Reg::kClassXyz>();

  // Translate call operand.
  ASMJIT_PROPAGATE(X86RAPass_translateOperands(_context, node->getOpArray(), node->getOpCount()));

  // To emit instructions after call.
  _cc->_setCursor(node);

  // If the callee pops stack it has to be manually adjusted back.
  X86FuncDecl* decl = node->getDecl();
  if (decl->getCalleePopsStack() && decl->getArgStackSize() != 0)
    _cc->emit(X86Inst::kIdSub, _context->_zsp, static_cast<int>(decl->getArgStackSize()));

  // Clobber.
  clobber<X86Reg::kClassGp >();
  clobber<X86Reg::kClassMm >();
  clobber<X86Reg::kClassXyz>();

  // Return.
  ret();

  // Unuse.
  unuseAfter<X86Reg::kClassGp >();
  unuseAfter<X86Reg::kClassMm >();
  unuseAfter<X86Reg::kClassXyz>();

  // Cleanup; disconnect Vd->Va.
  cleanup();

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86CallAlloc - Init / Cleanup]
// ============================================================================

ASMJIT_INLINE void X86CallAlloc::init(X86CCCall* node, X86RAData* raData) {
  X86BaseAlloc::init(node, raData);

  // Create mask of all registers that will be used to pass function arguments.
  _willAlloc = node->_usedArgs;
  _willSpill.reset();
}

ASMJIT_INLINE void X86CallAlloc::cleanup() {
  X86BaseAlloc::cleanup();
}

// ============================================================================
// [asmjit::X86CallAlloc - Plan / Spill / Alloc]
// ============================================================================

template<int C>
ASMJIT_INLINE void X86CallAlloc::plan() {
  uint32_t i;
  uint32_t clobbered = _raData->clobberedRegs.get(C);

  uint32_t willAlloc = _willAlloc.get(C);
  uint32_t willFree = clobbered & ~willAlloc;

  TiedReg* tiedArray = getTiedArrayByRC(C);
  uint32_t tiedCount = getTiedCountByRC(C);

  X86RAState* state = getState();

  // Calculate 'willAlloc' and 'willFree' masks based on mandatory masks.
  for (i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedArray[i];
    VirtReg* vreg = tied->vreg;

    uint32_t vaFlags = tied->flags;
    uint32_t physId = vreg->getPhysId();
    uint32_t regMask = (physId != kInvalidReg) ? Utils::mask(physId) : 0;

    if ((vaFlags & TiedReg::kRReg) != 0) {
      // Planning register allocation. First check whether the variable is
      // already allocated in register and if it can stay there. Function
      // arguments are passed either in a specific register or in stack so
      // we care mostly of mandatory registers.
      uint32_t inRegs = tied->inRegs;

      if (inRegs == 0) {
        inRegs = tied->allocableRegs;
      }

      // Optimize situation where the variable has to be allocated in a
      // mandatory register, but it's already allocated in register that
      // is not clobbered (i.e. it will survive function call).
      if ((regMask & inRegs) != 0 || ((regMask & ~clobbered) != 0 && (vaFlags & TiedReg::kUnuse) == 0)) {
        tied->setInPhysId(physId);
        tied->flags |= TiedReg::kRDone;
        addTiedDone(C);
      }
      else {
        willFree |= regMask;
      }
    }
    else {
      // Memory access - if variable is allocated it has to be freed.
      if (regMask != 0) {
        willFree |= regMask;
      }
      else {
        tied->flags |= TiedReg::kRDone;
        addTiedDone(C);
      }
    }
  }

  // Occupied registers without 'willFree' registers; contains basically
  // all the registers we can use to allocate variables without inRegs
  // speficied.
  uint32_t occupied = state->_occupied.get(C) & ~willFree;
  uint32_t willSpill = 0;

  // Find the best registers for variables that are not allocated yet. Only
  // useful for Gp registers used as call operand.
  for (i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedArray[i];
    VirtReg* vreg = tied->vreg;

    uint32_t vaFlags = tied->flags;
    if ((vaFlags & TiedReg::kRDone) != 0 || (vaFlags & TiedReg::kRReg) == 0)
      continue;

    // All registers except Gp used by call itself must have inPhysId.
    uint32_t m = tied->inRegs;
    if (C != X86Reg::kClassGp || m) {
      ASMJIT_ASSERT(m != 0);
      tied->setInPhysId(Utils::findFirstBit(m));
      willSpill |= occupied & m;
      continue;
    }

    m = tied->allocableRegs & ~(willAlloc ^ m);
    m = guessAlloc<C>(vreg, m);
    ASMJIT_ASSERT(m != 0);

    uint32_t candidateRegs = m & ~occupied;
    if (candidateRegs == 0) {
      candidateRegs = m & occupied & ~state->_modified.get(C);
      if (candidateRegs == 0)
        candidateRegs = m;
    }

    if (!(vaFlags & (TiedReg::kWReg | TiedReg::kUnuse)) && (candidateRegs & ~clobbered))
      candidateRegs &= ~clobbered;

    uint32_t physId = Utils::findFirstBit(candidateRegs);
    uint32_t regMask = Utils::mask(physId);

    tied->setInPhysId(physId);
    tied->inRegs = regMask;

    willAlloc |= regMask;
    willSpill |= regMask & occupied;
    willFree &= ~regMask;

    occupied |= regMask;
    continue;
  }

  // Set calculated masks back to the allocator; needed by spill() and alloc().
  _willSpill.set(C, willSpill);
  _willAlloc.set(C, willAlloc);
}

template<int C>
ASMJIT_INLINE void X86CallAlloc::spill() {
  uint32_t m = _willSpill.get(C);
  uint32_t i = static_cast<uint32_t>(0) - 1;

  if (m == 0)
    return;

  X86RAState* state = getState();
  VirtReg** sVars = state->getListByRC(C);

  // Available registers for decision if move has any benefit over spill.
  uint32_t availableRegs = getGaRegs(C) & ~(state->_occupied.get(C) | m | _willAlloc.get(C));

  do {
    // We always advance one more to destroy the bit that we have found.
    uint32_t bitIndex = Utils::findFirstBit(m) + 1;

    i += bitIndex;
    m >>= bitIndex;

    VirtReg* vreg = sVars[i];
    ASMJIT_ASSERT(vreg && !vreg->_tied);

    if (vreg->isModified() && availableRegs) {
      uint32_t available = guessSpill<C>(vreg, availableRegs);
      if (available != 0) {
        uint32_t physId = Utils::findFirstBit(available);
        uint32_t regMask = Utils::mask(physId);

        _context->move<C>(vreg, physId);
        availableRegs ^= regMask;
        continue;
      }
    }

    _context->spill<C>(vreg);
  } while (m != 0);
}

template<int C>
ASMJIT_INLINE void X86CallAlloc::alloc() {
  if (isTiedDone(C)) return;

  TiedReg* tiedArray = getTiedArrayByRC(C);
  uint32_t tiedCount = getTiedCountByRC(C);

  X86RAState* state = getState();
  VirtReg** sVars = state->getListByRC(C);

  uint32_t i;
  bool didWork;

  do {
    didWork = false;
    for (i = 0; i < tiedCount; i++) {
      TiedReg* aTied = &tiedArray[i];
      VirtReg* aVReg = aTied->vreg;
      if ((aTied->flags & (TiedReg::kRReg | TiedReg::kRDone)) != TiedReg::kRReg) continue;

      uint32_t sPhysId = aVReg->getPhysId();
      uint32_t bPhysId = aTied->inPhysId;

      // Shouldn't be the same.
      ASMJIT_ASSERT(sPhysId != bPhysId);

      VirtReg* bVReg = getState()->getListByRC(C)[bPhysId];
      if (bVReg) {
        TiedReg* bTied = bVReg->_tied;

        // GP registers only - Swap two registers if we can solve two
        // allocation tasks by a single 'xchg' instruction, swapping
        // two registers required by the instruction/node or one register
        // required with another non-required.
        if (C == X86Reg::kClassGp) {
          _context->swapGp(aVReg, bVReg);

          aTied->flags |= TiedReg::kRDone;
          addTiedDone(C);

          // Double-hit, two registers allocated by a single swap.
          if (bTied && bTied->inPhysId == sPhysId) {
            bTied->flags |= TiedReg::kRDone;
            addTiedDone(C);
          }

          didWork = true;
          continue;
        }
      }
      else if (sPhysId != kInvalidReg) {
        _context->move<C>(aVReg, bPhysId);
        _context->_clobberedRegs.or_(C, Utils::mask(bPhysId));

        aTied->flags |= TiedReg::kRDone;
        addTiedDone(C);

        didWork = true;
        continue;
      }
      else {
        _context->alloc<C>(aVReg, bPhysId);
        _context->_clobberedRegs.or_(C, Utils::mask(bPhysId));

        aTied->flags |= TiedReg::kRDone;
        addTiedDone(C);

        didWork = true;
        continue;
      }
    }
  } while (didWork);
}

// ============================================================================
// [asmjit::X86CallAlloc - AllocImmsOnStack]
// ============================================================================

ASMJIT_INLINE void X86CallAlloc::allocImmsOnStack() {
  X86CCCall* node = getNode();
  X86FuncDecl* decl = node->getDecl();

  uint32_t argCount = decl->getNumArgs();
  Operand_* args = node->_args;

  for (uint32_t i = 0; i < argCount; i++) {
    Operand_& op = args[i];
    if (!op.isImm()) continue;

    const Imm& imm = static_cast<const Imm&>(op);
    const FuncInOut& arg = decl->getArg(i);
    uint32_t varType = arg.getTypeId();

    if (arg.hasStackOffset()) {
      X86Mem dst = x86::ptr(_context->_zsp, -static_cast<int>(_context->getGpSize()) + arg.getStackOffset());
      _context->emitMoveImmOnStack(varType, &dst, &imm);
    }
    else {
      _context->emitMoveImmToReg(varType, arg.getRegId(), &imm);
    }
  }
}

// ============================================================================
// [asmjit::X86CallAlloc - Duplicate]
// ============================================================================

template<int C>
ASMJIT_INLINE void X86CallAlloc::duplicate() {
  TiedReg* tiedArray = getTiedArrayByRC(C);
  uint32_t tiedCount = getTiedCountByRC(C);

  for (uint32_t i = 0; i < tiedCount; i++) {
    TiedReg* tied = &tiedArray[i];
    if ((tied->flags & TiedReg::kRReg) == 0) continue;

    uint32_t inRegs = tied->inRegs;
    if (!inRegs) continue;

    VirtReg* vreg = tied->vreg;
    uint32_t physId = vreg->getPhysId();

    ASMJIT_ASSERT(physId != kInvalidReg);

    inRegs &= ~Utils::mask(physId);
    if (!inRegs) continue;

    for (uint32_t dupIndex = 0; inRegs != 0; dupIndex++, inRegs >>= 1) {
      if (inRegs & 0x1) {
        _context->emitMove(vreg, dupIndex, physId, "Duplicate");
        _context->_clobberedRegs.or_(C, Utils::mask(dupIndex));
      }
    }
  }
}

// ============================================================================
// [asmjit::X86CallAlloc - GuessAlloc / GuessSpill]
// ============================================================================

template<int C>
ASMJIT_INLINE uint32_t X86CallAlloc::guessAlloc(VirtReg* vreg, uint32_t allocableRegs) {
  ASMJIT_ASSERT(allocableRegs != 0);

  // Stop now if there is only one bit (register) set in 'allocableRegs' mask.
  if (Utils::isPowerOf2(allocableRegs))
    return allocableRegs;

  uint32_t i;
  uint32_t safeRegs = allocableRegs;
  uint32_t maxLookAhead = _cc->getMaxLookAhead();

  // Look ahead and calculate mask of special registers on both - input/output.
  CBNode* node = _node;
  for (i = 0; i < maxLookAhead; i++) {
    // Stop on `CCFuncRet` and `CBSentinel`.
    if (node->hasFlag(CBNode::kFlagIsRet))
      break;

    // Stop on conditional jump, we don't follow them.
    if (node->hasFlag(CBNode::kFlagIsJcc))
      break;

    // Advance on non-conditional jump.
    if (node->hasFlag(CBNode::kFlagIsJmp)) {
      node = static_cast<CBJump*>(node)->getTarget();
      // Stop on jump that is not followed.
      if (!node) break;
    }

    node = node->getNext();
    ASMJIT_ASSERT(node != nullptr);

    X86RAData* raData = node->getPassData<X86RAData>();
    if (raData) {
      TiedReg* tied = raData->findTiedByRC(C, vreg);
      if (tied) {
        uint32_t inRegs = tied->inRegs;
        if (inRegs != 0) {
          safeRegs = allocableRegs;
          allocableRegs &= inRegs;

          if (allocableRegs == 0)
            goto _UseSafeRegs;
          else
            return allocableRegs;
        }
      }

      safeRegs = allocableRegs;
      allocableRegs &= ~(raData->inRegs.get(C) | raData->outRegs.get(C) | raData->clobberedRegs.get(C));

      if (allocableRegs == 0)
        break;
    }
  }

_UseSafeRegs:
  return safeRegs;
}

template<int C>
ASMJIT_INLINE uint32_t X86CallAlloc::guessSpill(VirtReg* vreg, uint32_t allocableRegs) {
  ASMJIT_ASSERT(allocableRegs != 0);
  return 0;
}

// ============================================================================
// [asmjit::X86CallAlloc - Save]
// ============================================================================

template<int C>
ASMJIT_INLINE void X86CallAlloc::save() {
  X86RAState* state = getState();
  VirtReg** sVars = state->getListByRC(C);

  uint32_t i;
  uint32_t affected = _raData->clobberedRegs.get(C) & state->_occupied.get(C) & state->_modified.get(C);

  for (i = 0; affected != 0; i++, affected >>= 1) {
    if (affected & 0x1) {
      VirtReg* vreg = sVars[i];
      ASMJIT_ASSERT(vreg != nullptr);
      ASMJIT_ASSERT(vreg->isModified());

      TiedReg* tied = vreg->_tied;
      if (!tied || (tied->flags & (TiedReg::kWReg | TiedReg::kUnuse)) == 0)
        _context->save<C>(vreg);
    }
  }
}

// ============================================================================
// [asmjit::X86CallAlloc - Clobber]
// ============================================================================

template<int C>
ASMJIT_INLINE void X86CallAlloc::clobber() {
  X86RAState* state = getState();
  VirtReg** sVars = state->getListByRC(C);

  uint32_t i;
  uint32_t affected = _raData->clobberedRegs.get(C) & state->_occupied.get(C);

  for (i = 0; affected != 0; i++, affected >>= 1) {
    if (affected & 0x1) {
      VirtReg* vreg = sVars[i];
      ASMJIT_ASSERT(vreg != nullptr);

      TiedReg* tied = vreg->_tied;
      uint32_t vdState = VirtReg::kStateNone;

      if (!vreg->isModified() || (tied && (tied->flags & (TiedReg::kWAll | TiedReg::kUnuse)) != 0))
        vdState = VirtReg::kStateMem;
      _context->unuse<C>(vreg, vdState);
    }
  }
}

// ============================================================================
// [asmjit::X86CallAlloc - Ret]
// ============================================================================

ASMJIT_INLINE void X86CallAlloc::ret() {
  X86CCCall* node = getNode();
  X86FuncDecl* decl = node->getDecl();

  uint32_t i;
  Operand_* rets = node->_ret;

  for (i = 0; i < 2; i++) {
    const FuncInOut& ret = decl->getRet(i);
    Operand_* op = &rets[i];
    if (!ret.hasRegId() || !op->isVirtReg()) continue;

    VirtReg* vreg = _cc->getVirtRegById(op->getId());
    uint32_t flags = _x86TypeData.typeInfo[vreg->getTypeId()].getTypeFlags();
    uint32_t regId = ret.getRegId();

    switch (vreg->getRegClass()) {
      case X86Reg::kClassGp:
        ASMJIT_ASSERT(x86TypeIdToClass(ret.getTypeId()) == vreg->getRegClass());

        _context->unuse<X86Reg::kClassGp>(vreg);
        _context->attach<X86Reg::kClassGp>(vreg, regId, true);
        break;

      case X86Reg::kClassMm:
        ASMJIT_ASSERT(x86TypeIdToClass(ret.getTypeId()) == vreg->getRegClass());

        _context->unuse<X86Reg::kClassMm>(vreg);
        _context->attach<X86Reg::kClassMm>(vreg, regId, true);
        break;

      case X86Reg::kClassXyz:
        if (ret.getTypeId() == VirtType::kIdF32 || ret.getTypeId() == VirtType::kIdF64) {
          X86Mem m = _context->getVarMem(vreg);
          m.setSize((flags & VirtType::kFlagF32) ? 4 :
                    (flags & VirtType::kFlagF64) ? 8 :
                    (ret.getTypeId() == VirtType::kIdF32) ? 4 : 8);

          _context->unuse<X86Reg::kClassXyz>(vreg, VirtReg::kStateMem);
          _cc->fstp(m);
        }
        else {
          ASMJIT_ASSERT(x86TypeIdToClass(ret.getTypeId()) == vreg->getRegClass());

          _context->unuse<X86Reg::kClassXyz>(vreg);
          _context->attach<X86Reg::kClassXyz>(vreg, regId, true);
        }
        break;
    }
  }
}

// ============================================================================
// [asmjit::X86RAPass - TranslateOperands]
// ============================================================================

//! \internal
static Error X86RAPass_translateOperands(X86RAPass* self, Operand_* opArray, uint32_t opCount) {
  X86Compiler* cc = self->cc();

  // Translate variables into isters.
  for (uint32_t i = 0; i < opCount; i++) {
    Operand_* op = &opArray[i];
    if (op->isVirtReg()) {
      VirtReg* vreg = cc->getVirtRegById(op->getId());
      ASMJIT_ASSERT(vreg != nullptr);
      ASMJIT_ASSERT(vreg->getPhysId() != kInvalidReg);
      op->_reg.id = vreg->getPhysId();
    }
    else if (op->isMem()) {
      X86Mem* m = static_cast<X86Mem*>(op);

      if (m->hasBaseReg() && cc->isVirtRegValid(m->getBaseId())) {
        VirtReg* vreg = cc->getVirtRegById(m->getBaseId());

        if (m->isRegHome()) {
          if (!vreg->isMemArg())
            self->getVarCell(vreg);

          // Offset will be patched later by X86RAPass_patchFuncMem().
          m->addOffsetLo32(vreg->isMemArg() ? self->_argActualDisp : self->_varActualDisp);
        }
        else {
          ASMJIT_ASSERT(vreg->getPhysId() != kInvalidReg);
          op->_mem.base = vreg->getPhysId();
        }
      }

      if (m->hasIndexReg() && cc->isVirtRegValid(m->getIndexId())) {
        VirtReg* vreg = cc->getVirtRegById(m->getIndexId());
        ASMJIT_ASSERT(vreg->getPhysId() != kInvalidReg);
        ASMJIT_ASSERT(vreg->getPhysId() != X86Gp::kIdR12);
        op->_mem.index = vreg->getPhysId();
      }
    }
  }

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86RAPass - TranslatePrologEpilog]
// ============================================================================

//! \internal
static Error X86RAPass_initFunc(X86RAPass* self, X86Func* func) {
  X86Compiler* cc = self->cc();
  X86FuncDecl* decl = func->getDecl();

  X86RegMask& clobberedRegs = self->_clobberedRegs;
  uint32_t gpSize = cc->getGpSize();

  // Setup "Save-Restore" registers.
  func->_saveRestoreRegs.set(X86Reg::kClassGp , clobberedRegs.get(X86Reg::kClassGp ) & decl->getPreserved(X86Reg::kClassGp ));
  func->_saveRestoreRegs.set(X86Reg::kClassMm , clobberedRegs.get(X86Reg::kClassMm ) & decl->getPreserved(X86Reg::kClassMm ));
  func->_saveRestoreRegs.set(X86Reg::kClassK  , 0);
  func->_saveRestoreRegs.set(X86Reg::kClassXyz, clobberedRegs.get(X86Reg::kClassXyz) & decl->getPreserved(X86Reg::kClassXyz));

  ASMJIT_ASSERT(!func->_saveRestoreRegs.has(X86Reg::kClassGp, Utils::mask(X86Gp::kIdSp)));

  // Setup required stack alignment and kFuncFlagIsStackMisaligned.
  {
    uint32_t requiredStackAlignment = Utils::iMax(self->_memMaxAlign, gpSize);

    if (requiredStackAlignment < 16) {
      // Require 16-byte alignment if 8-byte vars are used.
      if (self->_mem8ByteVarsUsed)
        requiredStackAlignment = 16;
      else if (func->_saveRestoreRegs.get(X86Reg::kClassMm) || func->_saveRestoreRegs.get(X86Reg::kClassXyz))
        requiredStackAlignment = 16;
      else if (Utils::inInterval<uint32_t>(func->getRequiredStackAlignment(), 8, 16))
        requiredStackAlignment = 16;
    }

    if (func->getRequiredStackAlignment() < requiredStackAlignment)
      func->setRequiredStackAlignment(requiredStackAlignment);

    func->updateRequiredStackAlignment();
  }

  // Adjust stack pointer if function is caller.
  if (func->isCaller()) {
    func->addFuncFlags(kFuncFlagIsStackAdjusted);
    func->_callStackSize = Utils::alignTo<uint32_t>(func->getCallStackSize(), func->getRequiredStackAlignment());
  }

  // Adjust stack pointer if manual stack alignment is needed.
  if (func->isStackMisaligned() && func->isNaked()) {
    // Get a memory cell where the original stack frame will be stored.
    RACell* cell = self->_newStackCell(gpSize, gpSize);
    if (ASMJIT_UNLIKELY(!cell))
      return DebugUtils::errored(kErrorNoHeapMemory);

    func->addFuncFlags(kFuncFlagIsStackAdjusted);
    self->_stackFrameCell = cell;

    if (decl->getArgStackSize() > 0) {
      func->addFuncFlags(kFuncFlagX86MoveArgs);
      func->setExtraStackSize(decl->getArgStackSize());
    }

    // Get temporary register which will be used to align the stack frame.
    uint32_t fRegMask = Utils::bits(self->_regCount.getGp());
    uint32_t stackFrameCopyRegs;

    fRegMask &= ~(decl->getUsed(X86Reg::kClassGp) | Utils::mask(X86Gp::kIdSp));
    stackFrameCopyRegs = fRegMask;

    // Try to remove modified registers from the mask.
    uint32_t tRegMask = fRegMask & ~self->getClobberedRegs(X86Reg::kClassGp);
    if (tRegMask != 0)
      fRegMask = tRegMask;

    // Try to remove preserved registers from the mask.
    tRegMask = fRegMask & ~decl->getPreserved(X86Reg::kClassGp);
    if (tRegMask != 0)
      fRegMask = tRegMask;

    ASMJIT_ASSERT(fRegMask != 0);

    uint32_t fRegIndex = Utils::findFirstBit(fRegMask);
    func->_stackFrameRegIndex = static_cast<uint8_t>(fRegIndex);

    // We have to save the register on the stack (it will be the part of prolog
    // and epilog), however we shouldn't save it twice, so we will remove it
    // from '_saveRestoreRegs' in case that it is preserved.
    fRegMask = Utils::mask(fRegIndex);
    if ((fRegMask & decl->getPreserved(X86Reg::kClassGp)) != 0) {
      func->_saveRestoreRegs.andNot(X86Reg::kClassGp, fRegMask);
      func->_isStackFrameRegPreserved = true;
    }

    if (func->hasFuncFlag(kFuncFlagX86MoveArgs)) {
      uint32_t maxRegs = (func->getArgStackSize() + gpSize - 1) / gpSize;
      stackFrameCopyRegs &= ~fRegMask;

      tRegMask = stackFrameCopyRegs & self->getClobberedRegs(X86Reg::kClassGp);
      uint32_t tRegCnt = Utils::bitCount(tRegMask);

      if (tRegCnt > 1 || (tRegCnt > 0 && tRegCnt <= maxRegs))
        stackFrameCopyRegs = tRegMask;
      else
        stackFrameCopyRegs = Utils::keepNOnesFromRight(stackFrameCopyRegs, Utils::iMin<uint32_t>(maxRegs, 2));

      func->_saveRestoreRegs.or_(X86Reg::kClassGp, stackFrameCopyRegs & decl->getPreserved(X86Reg::kClassGp));
      Utils::indexNOnesFromRight(func->_stackFrameCopyGpIndex, stackFrameCopyRegs, maxRegs);
    }
  }
  // If function is not naked we generate standard "EBP/RBP" stack frame.
  else if (!func->isNaked()) {
    uint32_t fRegIndex = X86Gp::kIdBp;

    func->_stackFrameRegIndex = static_cast<uint8_t>(fRegIndex);
    func->_isStackFrameRegPreserved = true;
  }

  ASMJIT_PROPAGATE(self->resolveCellOffsets());

  // Adjust stack pointer if requested memory can't fit into "Red Zone" or "Spill Zone".
  if (self->_memAllTotal > Utils::iMax<uint32_t>(func->getRedZoneSize(), func->getSpillZoneSize())) {
    func->addFuncFlags(kFuncFlagIsStackAdjusted);
  }

  // Setup stack size used to save preserved registers.
  {
    uint32_t memGpSize  = Utils::bitCount(func->_saveRestoreRegs.get(X86Reg::kClassGp )) * gpSize;
    uint32_t memMmSize  = Utils::bitCount(func->_saveRestoreRegs.get(X86Reg::kClassMm )) * 8;
    uint32_t memXmmSize = Utils::bitCount(func->_saveRestoreRegs.get(X86Reg::kClassXyz)) * 16;

    func->_pushPopStackSize = memGpSize;
    func->_moveStackSize = memXmmSize + Utils::alignTo<uint32_t>(memMmSize, 16);
  }

  // Setup adjusted stack size.
  if (func->isStackMisaligned()) {
    func->_alignStackSize = 0;
  }
  else {
    // If function is aligned, the RETURN address is stored in the aligned
    // [ZSP - PtrSize] which makes current ZSP unaligned.
    int32_t v = static_cast<int32_t>(gpSize);

    // If we have to store function frame pointer we have to count it as well,
    // because it is the first thing pushed on the stack.
    if (func->hasStackFrameReg() && func->isStackFrameRegPreserved())
      v += gpSize;

    // Count push/pop sequence.
    v += func->getPushPopStackSize();

    // Count save/restore sequence for XMM registers (should be already aligned).
    v += func->getMoveStackSize();

    // Maximum memory required to call all functions within this function.
    v += func->getCallStackSize();

    // Calculate the final offset to keep stack alignment.
    func->_alignStackSize = Utils::alignDiff<uint32_t>(v, func->getRequiredStackAlignment());
  }

  // Memory stack size.
  func->_memStackSize = self->_memAllTotal;
  func->_alignedMemStackSize = Utils::alignTo<uint32_t>(func->_memStackSize, func->getRequiredStackAlignment());

  if (func->isNaked()) {
    self->_argBaseReg = X86Gp::kIdSp;

    if (func->isStackAdjusted()) {
      if (func->isStackMisaligned()) {
        self->_argBaseOffset = static_cast<int32_t>(
          func->getCallStackSize() +
          func->getAlignedMemStackSize() +
          func->getMoveStackSize() +
          func->getAlignStackSize());
        self->_argBaseOffset -= gpSize;
      }
      else {
        self->_argBaseOffset = static_cast<int32_t>(
          func->getCallStackSize() +
          func->getAlignedMemStackSize() +
          func->getMoveStackSize() +
          func->getPushPopStackSize() +
          func->getExtraStackSize() +
          func->getAlignStackSize());
      }
    }
    else {
      self->_argBaseOffset = func->getPushPopStackSize();
    }
  }
  else {
    self->_argBaseReg = X86Gp::kIdBp;
    // Caused by "push zbp".
    self->_argBaseOffset = gpSize;
  }

  self->_varBaseReg = X86Gp::kIdSp;
  self->_varBaseOffset = func->getCallStackSize();

  if (!func->isStackAdjusted()) {
    self->_varBaseOffset = -static_cast<int32_t>(
      func->_alignStackSize +
      func->_alignedMemStackSize +
      func->_moveStackSize);
  }

  return kErrorOk;
}

//! \internal
static Error X86RAPass_patchFuncMem(X86RAPass* self, X86Func* func, CBNode* stop) {
  X86Compiler* cc = self->cc();
  CBNode* node = func;

  do {
    if (node->getType() == CBNode::kNodeInst) {
      CBInst* iNode = static_cast<CBInst*>(node);

      if (iNode->hasMemOp()) {
        X86Mem* m = iNode->getMemOp<X86Mem>();

        if (m->isRegHome() && Operand::isPackedId(m->getBaseId())) {
          VirtReg* vreg = cc->getVirtRegById(m->getBaseId());
          ASMJIT_ASSERT(vreg != nullptr);

          if (vreg->isMemArg()) {
            m->_setBase(cc->_nativeGpReg.getRegType(), self->_argBaseReg);
            m->addOffsetLo32(self->_argBaseOffset + vreg->getMemOffset());
            m->clearRegHome();
          }
          else {
            RACell* cell = vreg->getMemCell();
            ASMJIT_ASSERT(cell != nullptr);

            m->_setBase(cc->_nativeGpReg.getRegType(), self->_varBaseReg);
            m->addOffsetLo32(self->_varBaseOffset + cell->offset);
            m->clearRegHome();
          }
        }
      }
    }

    node = node->getNext();
  } while (node != stop);

  return kErrorOk;
}

//! \internal
static Error X86RAPass_translatePrologEpilog(X86RAPass* self, X86Func* func) {
  X86Compiler* cc = self->cc();
  X86FuncDecl* decl = func->getDecl();

  uint32_t gpSize = cc->getGpSize();

  int32_t stackSize = static_cast<int32_t>(
    func->getAlignStackSize() +
    func->getCallStackSize() +
    func->getAlignedMemStackSize() +
    func->getMoveStackSize() +
    func->getExtraStackSize());
  int32_t stackAlignment = func->getRequiredStackAlignment();

  int32_t stackBase;
  int32_t stackPtr;

  if (func->isStackAdjusted()) {
    stackBase = static_cast<int32_t>(
      func->getCallStackSize() +
      func->getAlignedMemStackSize());
  }
  else {
    stackBase = -static_cast<int32_t>(
      func->getAlignedMemStackSize() +
      func->getAlignStackSize() +
      func->getExtraStackSize());
  }

  uint32_t i, mask;
  uint32_t regsGp  = func->getSaveRestoreRegs(X86Reg::kClassGp );
  uint32_t regsMm  = func->getSaveRestoreRegs(X86Reg::kClassMm );
  uint32_t regsXmm = func->getSaveRestoreRegs(X86Reg::kClassXyz);

  bool earlyPushPop = false;
  bool useLeaEpilog = false;

  X86Gp gpReg(self->_zsp);
  X86Gp fpReg(self->_zbp);

  X86Mem fpOffset;

  // --------------------------------------------------------------------------
  // [Prolog]
  // --------------------------------------------------------------------------

  cc->_setCursor(func);

  // Entry.
  if (func->isNaked()) {
    if (func->isStackMisaligned()) {
      fpReg.setId(func->getStackFrameRegIndex());
      fpOffset = x86::ptr(self->_zsp, self->_varBaseOffset + static_cast<int32_t>(self->_stackFrameCell->offset));

      earlyPushPop = true;
      self->emitPushSequence(regsGp);

      if (func->isStackFrameRegPreserved())
        cc->emit(X86Inst::kIdPush, fpReg);

      cc->emit(X86Inst::kIdMov, fpReg, self->_zsp);
    }
  }
  else {
    cc->emit(X86Inst::kIdPush, fpReg);
    cc->emit(X86Inst::kIdMov, fpReg, self->_zsp);
  }

  if (!earlyPushPop) {
    self->emitPushSequence(regsGp);
    if (func->isStackMisaligned() && regsGp != 0)
      useLeaEpilog = true;
  }

  // Adjust stack pointer.
  if (func->isStackAdjusted()) {
    stackBase = static_cast<int32_t>(func->getAlignedMemStackSize() + func->getCallStackSize());

    if (stackSize)
      cc->emit(X86Inst::kIdSub, self->_zsp, stackSize);

    if (func->isStackMisaligned())
      cc->emit(X86Inst::kIdAnd, self->_zsp, -stackAlignment);

    if (func->isStackMisaligned() && func->isNaked())
      cc->emit(X86Inst::kIdMov, fpOffset, fpReg);
  }
  else {
    stackBase = -static_cast<int32_t>(func->getAlignStackSize() + func->getMoveStackSize());
  }

  // Save XMM/MMX/GP (Mov).
  stackPtr = stackBase;
  for (i = 0, mask = regsXmm; mask != 0; i++, mask >>= 1) {
    if (mask & 0x1) {
      cc->emit(X86Inst::kIdMovaps, x86::oword_ptr(self->_zsp, stackPtr), x86::xmm(i));
      stackPtr += 16;
    }
  }

  for (i = 0, mask = regsMm; mask != 0; i++, mask >>= 1) {
    if (mask & 0x1) {
      cc->emit(X86Inst::kIdMovq, x86::qword_ptr(self->_zsp, stackPtr), x86::mm(i));
      stackPtr += 8;
    }
  }

  // --------------------------------------------------------------------------
  // [Move-Args]
  // --------------------------------------------------------------------------

  if (func->hasFuncFlag(kFuncFlagX86MoveArgs)) {
    uint32_t argStackPos = 0;
    uint32_t argStackSize = decl->getArgStackSize();

    uint32_t movIndex = 0;
    uint32_t movCount = (argStackSize + gpSize - 1) / gpSize;

    X86Gp r[8];
    uint32_t numRegs = 0;

    for (i = 0; i < ASMJIT_ARRAY_SIZE(func->_stackFrameCopyGpIndex); i++) {
      if (func->_stackFrameCopyGpIndex[i] != kInvalidReg) {
        gpReg.setId(func->_stackFrameCopyGpIndex[i]);
        r[numRegs++] = gpReg;
      }
    }
    ASMJIT_ASSERT(numRegs > 0);

    int32_t dSrc = func->getPushPopStackSize() + gpSize;
    int32_t dDst = func->getAlignStackSize() +
                   func->getCallStackSize() +
                   func->getAlignedMemStackSize() +
                   func->getMoveStackSize();

    if (func->isStackFrameRegPreserved())
      dSrc += gpSize;

    X86Mem mSrc = x86::ptr(fpReg, dSrc);
    X86Mem mDst = x86::ptr(self->_zsp, dDst);

    while (movIndex < movCount) {
      uint32_t n = Utils::iMin<uint32_t>(movCount - movIndex, numRegs);

      for (i = 0; i < n; i++) cc->emit(X86Inst::kIdMov, r[i], mSrc.adjusted((movIndex + i) * gpSize));
      for (i = 0; i < n; i++) cc->emit(X86Inst::kIdMov, mDst.adjusted((movIndex + i) * gpSize), r[i]);

      argStackPos += n * gpSize;
      movIndex += n;
    }
  }

  // --------------------------------------------------------------------------
  // [Epilog]
  // --------------------------------------------------------------------------

  cc->_setCursor(func->getExitNode());

  // Restore XMM/MMX/GP (Mov).
  stackPtr = stackBase;
  for (i = 0, mask = regsXmm; mask != 0; i++, mask >>= 1) {
    if (mask & 0x1) {
      cc->emit(X86Inst::kIdMovaps, x86::xmm(i), x86::oword_ptr(self->_zsp, stackPtr));
      stackPtr += 16;
    }
  }

  for (i = 0, mask = regsMm; mask != 0; i++, mask >>= 1) {
    if (mask & 0x1) {
      cc->emit(X86Inst::kIdMovq, x86::mm(i), x86::qword_ptr(self->_zsp, stackPtr));
      stackPtr += 8;
    }
  }

  // Adjust stack.
  if (useLeaEpilog) {
    cc->emit(X86Inst::kIdLea, self->_zsp, x86::ptr(fpReg, -static_cast<int32_t>(func->getPushPopStackSize())));
  }
  else if (!func->isStackMisaligned()) {
    if (func->isStackAdjusted() && stackSize != 0)
      cc->emit(X86Inst::kIdAdd, self->_zsp, stackSize);
  }

  // Restore Gp (Push/Pop).
  if (!earlyPushPop)
    self->emitPopSequence(regsGp);

  // Emms.
  if (func->hasFuncFlag(kFuncFlagX86Emms))
    cc->emit(X86Inst::kIdEmms);

  // MFence/SFence/LFence.
  if (func->hasFuncFlag(kFuncFlagX86SFence) & func->hasFuncFlag(kFuncFlagX86LFence))
    cc->emit(X86Inst::kIdMfence);
  else if (func->hasFuncFlag(kFuncFlagX86SFence))
    cc->emit(X86Inst::kIdSfence);
  else if (func->hasFuncFlag(kFuncFlagX86LFence))
    cc->emit(X86Inst::kIdLfence);

  // Leave.
  if (func->isNaked()) {
    if (func->isStackMisaligned()) {
      cc->emit(X86Inst::kIdMov, self->_zsp, fpOffset);

      if (func->isStackFrameRegPreserved())
        cc->emit(X86Inst::kIdPop, fpReg);

      if (earlyPushPop)
        self->emitPopSequence(regsGp);
    }
  }
  else {
    if (useLeaEpilog) {
      cc->emit(X86Inst::kIdPop, fpReg);
    }
    else if (func->hasFuncFlag(kFuncFlagX86Leave)) {
      cc->emit(X86Inst::kIdLeave);
    }
    else {
      cc->emit(X86Inst::kIdMov, self->_zsp, fpReg);
      cc->emit(X86Inst::kIdPop, fpReg);
    }
  }

  // Emit return.
  if (decl->getCalleePopsStack())
    cc->emit(X86Inst::kIdRet, static_cast<int32_t>(decl->getArgStackSize()));
  else
    cc->emit(X86Inst::kIdRet);

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86RAPass - Translate - Jump]
// ============================================================================

//! \internal
static void X86RAPass_translateJump(X86RAPass* self, CBJump* jNode, CBLabel* jTarget) {
  X86Compiler* cc = self->cc();
  CBNode* extNode = self->getExtraBlock();

  cc->_setCursor(extNode);
  self->switchState(jTarget->getPassData<RAData>()->state);

  // If one or more instruction has been added during switchState() it will be
  // moved at the end of the function body.
  if (cc->getCursor() != extNode) {
    // TODO: Can fail.
    CBLabel* jTrampolineTarget = cc->newLabelNode();

    // Add the jump to the target.
    cc->jmp(jTarget->getLabel());

    // Add the trampoline-label we jump to change the state.
    extNode = cc->setCursor(extNode);
    cc->addNode(jTrampolineTarget);

    // Finally, patch the jump target.
    ASMJIT_ASSERT(jNode->getOpCount() > 0);
    jNode->_opArray[0] = jTrampolineTarget->getLabel();
    jNode->_target = jTrampolineTarget;
  }

  // Store the `extNode` and load the state back.
  self->setExtraBlock(extNode);
  self->loadState(jNode->getPassData<RAData>()->state);
}

// ============================================================================
// [asmjit::X86RAPass - Translate - Ret]
// ============================================================================

static Error X86RAPass_translateRet(X86RAPass* self, CCFuncRet* rNode, CBLabel* exitTarget) {
  X86Compiler* cc = self->cc();
  CBNode* node = rNode->getNext();

  // 32-bit mode requires to push floating point return value(s), handle it
  // here as it's a special case.
  X86RAData* raData = rNode->getPassData<X86RAData>();
  if (raData) {
    TiedReg* tiedArray = raData->tiedArray;
    uint32_t tiedTotal = raData->tiedTotal;

    for (uint32_t i = 0; i < tiedTotal; i++) {
      TiedReg* tied = &tiedArray[i];
      if (tied->flags & (TiedReg::kX86Fld4 | TiedReg::kX86Fld8)) {
        VirtReg* vreg = tied->vreg;
        X86Mem m(self->getVarMem(vreg));

        uint32_t flags = _x86TypeData.typeInfo[vreg->getTypeId()].getTypeFlags();
        m.setSize((flags & VirtType::kFlagF32) ? 4 :
                  (flags & VirtType::kFlagF64) ? 8 :
                  (tied->flags & TiedReg::kX86Fld4) ? 4 : 8);

        cc->fld(m);
      }
    }
  }

  // Decide whether to `jmp` or not in case we are next to the return label.
  while (node) {
    switch (node->getType()) {
      // If we have found an exit label we just return, there is no need to
      // emit jump to that.
      case CBNode::kNodeLabel:
        if (static_cast<CBLabel*>(node) == exitTarget)
          return kErrorOk;
        goto _EmitRet;

      case CBNode::kNodeData:
      case CBNode::kNodeInst:
      case CBNode::kNodeCall:
      case CBNode::kNodeFuncExit:
        goto _EmitRet;

      // Continue iterating.
      case CBNode::kNodeComment:
      case CBNode::kNodeAlign:
      case CBNode::kNodeHint:
        break;

      // Invalid node to be here.
      case CBNode::kNodeFunc:
        return DebugUtils::errored(kErrorInvalidState);

      // We can't go forward from here.
      case CBNode::kNodeSentinel:
        return kErrorOk;
    }

    node = node->getNext();
  }

_EmitRet:
  {
    cc->_setCursor(rNode);
    cc->jmp(exitTarget->getLabel());
  }
  return kErrorOk;
}

// ============================================================================
// [asmjit::X86RAPass - Translate - Func]
// ============================================================================

Error X86RAPass::translate() {
  ASMJIT_TLOG("[T] ======= Translate (Begin)\n");

  X86Compiler* cc = this->cc();
  X86Func* func = getFunc();

  // Register allocator contexts.
  X86VarAlloc vAlloc(this);
  X86CallAlloc cAlloc(this);

  // Flow.
  CBNode* node_ = func;
  CBNode* next = nullptr;
  CBNode* stop = getStop();

  PodList<CBNode*>::Link* jLink = _jccList.getFirst();

  for (;;) {
    while (node_->isTranslated()) {
      // Switch state if we went to a node that is already translated.
      if (node_->getType() == CBNode::kNodeLabel) {
        CBLabel* node = static_cast<CBLabel*>(node_);
        cc->_setCursor(node->getPrev());
        switchState(node->getPassData<RAData>()->state);
      }

_NextGroup:
      if (!jLink) {
        goto _Done;
      }
      else {
        node_ = jLink->getValue();
        jLink = jLink->getNext();

        CBNode* jFlow = X86RAPass_getOppositeJccFlow(static_cast<CBJump*>(node_));
        loadState(node_->getPassData<RAData>()->state);

        if (jFlow->hasPassData() && jFlow->getPassData<RAData>()->state) {
          X86RAPass_translateJump(this, static_cast<CBJump*>(node_), static_cast<CBLabel*>(jFlow));

          node_ = jFlow;
          if (node_->isTranslated())
            goto _NextGroup;
        }
        else {
          node_ = jFlow;
        }

        break;
      }
    }

    next = node_->getNext();
    node_->_flags |= CBNode::kFlagIsTranslated;
    ASMJIT_TSEC({ this->_traceNode(this, node_, "[T] "); });

    if (node_->hasPassData()) {
      switch (node_->getType()) {
        // --------------------------------------------------------------------
        // [Align / Embed]
        // --------------------------------------------------------------------

        case CBNode::kNodeAlign:
        case CBNode::kNodeData:
          break;

        // --------------------------------------------------------------------
        // [Label]
        // --------------------------------------------------------------------

        case CBNode::kNodeLabel: {
          CBLabel* node = static_cast<CBLabel*>(node_);
          ASMJIT_ASSERT(node->getPassData<RAData>()->state == nullptr);
          node->getPassData<RAData>()->state = saveState();
          break;
        }

        // --------------------------------------------------------------------
        // [Inst/Call/SArg/Ret]
        // --------------------------------------------------------------------

        case CBNode::kNodeInst:
        case CBNode::kNodeCall:
        case CBNode::kNodePushArg:
          // Update TiedReg's unuse flags based on liveness of the next node.
          if (!node_->isJcc()) {
            X86RAData* raData = node_->getPassData<X86RAData>();
            RABits* liveness;

            if (raData && next && next->hasPassData() && (liveness = next->getPassData<RAData>()->liveness)) {
              TiedReg* tiedArray = raData->tiedArray;
              uint32_t tiedTotal = raData->tiedTotal;

              for (uint32_t i = 0; i < tiedTotal; i++) {
                TiedReg* tied = &tiedArray[i];
                VirtReg* vreg = tied->vreg;

                if (!liveness->getBit(vreg->_raId))
                  tied->flags |= TiedReg::kUnuse;
              }
            }
          }

          if (node_->getType() == CBNode::kNodeCall) {
            ASMJIT_PROPAGATE(cAlloc.run(static_cast<X86CCCall*>(node_)));
            break;
          }
          ASMJIT_FALLTHROUGH;

        case CBNode::kNodeHint:
        case CBNode::kNodeFuncExit: {
          ASMJIT_PROPAGATE(vAlloc.run(node_));

          // Handle conditional/unconditional jump.
          if (node_->isJmpOrJcc()) {
            CBJump* node = static_cast<CBJump*>(node_);
            CBLabel* jTarget = node->getTarget();

            // Target not followed.
            if (!jTarget) {
              if (node->isJmp())
                goto _NextGroup;
              else
                break;
            }

            if (node->isJmp()) {
              if (jTarget->hasPassData() && jTarget->getPassData<RAData>()->state) {
                cc->_setCursor(node->getPrev());
                switchState(jTarget->getPassData<RAData>()->state);

                goto _NextGroup;
              }
              else {
                next = jTarget;
              }
            }
            else {
              CBNode* jNext = node->getNext();

              if (jTarget->isTranslated()) {
                if (jNext->isTranslated()) {
                  ASMJIT_ASSERT(jNext->getType() == CBNode::kNodeLabel);
                  cc->_setCursor(node->getPrev());
                  intersectStates(
                    jTarget->getPassData<RAData>()->state,
                    jNext->getPassData<RAData>()->state);
                }

                RAState* savedState = saveState();
                node->getPassData<RAData>()->state = savedState;

                X86RAPass_translateJump(this, node, jTarget);
                next = jNext;
              }
              else if (jNext->isTranslated()) {
                ASMJIT_ASSERT(jNext->getType() == CBNode::kNodeLabel);

                RAState* savedState = saveState();
                node->getPassData<RAData>()->state = savedState;

                cc->_setCursor(node);
                switchState(jNext->getPassData<RAData>()->state);
                next = jTarget;
              }
              else {
                node->getPassData<RAData>()->state = saveState();
                next = X86RAPass_getJccFlow(node);
              }
            }
          }
          else if (node_->isRet()) {
            ASMJIT_PROPAGATE(
              X86RAPass_translateRet(this, static_cast<CCFuncRet*>(node_), func->getExitNode()));
          }
          break;
        }

        // --------------------------------------------------------------------
        // [Func]
        // --------------------------------------------------------------------

        case CBNode::kNodeFunc: {
          ASMJIT_ASSERT(node_ == func);

          X86FuncDecl* decl = func->getDecl();
          X86RAData* raData = func->getPassData<X86RAData>();

          uint32_t i;
          uint32_t argCount = func->_x86Decl.getNumArgs();

          for (i = 0; i < argCount; i++) {
            const FuncInOut& arg = decl->getArg(i);

            VirtReg* vreg = func->getArg(i);
            if (!vreg) continue;

            TiedReg* tied = raData->findTied(vreg);
            ASMJIT_ASSERT(tied != nullptr);

            if (tied->flags & TiedReg::kUnuse)
              continue;

            uint32_t physId = tied->outPhysId;
            if (physId != kInvalidReg && (tied->flags & TiedReg::kWConv) == 0) {
              switch (vreg->getRegClass()) {
                case X86Reg::kClassGp : attach<X86Reg::kClassGp >(vreg, physId, true); break;
                case X86Reg::kClassMm : attach<X86Reg::kClassMm >(vreg, physId, true); break;
                case X86Reg::kClassXyz: attach<X86Reg::kClassXyz>(vreg, physId, true); break;
              }
            }
            else if (tied->flags & TiedReg::kWConv) {
              // TODO: [COMPILER] Function Argument Conversion.
              ASMJIT_NOT_REACHED();
            }
            else {
              vreg->_isMemArg = true;
              vreg->setMemOffset(arg.getStackOffset());
              vreg->setState(VirtReg::kStateMem);
            }
          }
          break;
        }

        // --------------------------------------------------------------------
        // [End]
        // --------------------------------------------------------------------

        case CBNode::kNodeSentinel: {
          goto _NextGroup;
        }

        default:
          break;
      }
    }

    if (next == stop)
      goto _NextGroup;
    node_ = next;
  }

_Done:
  ASMJIT_PROPAGATE(X86RAPass_initFunc(this, func));
  ASMJIT_PROPAGATE(X86RAPass_patchFuncMem(this, func, stop));
  ASMJIT_PROPAGATE(X86RAPass_translatePrologEpilog(this, func));

  ASMJIT_TLOG("[T] ======= Translate (End)\n");
  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER && ASMJIT_BUILD_X86
