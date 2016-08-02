// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/assembler.h"
#include "../base/utils.h"
#include "../base/vmem.h"
#include <stdarg.h>

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::CodeGen - Construction / Destruction]
// ============================================================================

CodeGen::CodeGen(uint32_t type) noexcept
  : _holder(nullptr),
    _cgNext(nullptr),
    _archInfo(),
    _type(static_cast<uint8_t>(type)),
    _destroyed(false),
    _finalized(false),
    _reserved(false),
    _lastError(kErrorNotInitialized),
    _globalOptions(kOptionMaybeFailureCase),
    _options(0),
    _inlineComment(nullptr),
    _op4(),
    _op5(),
    _opMask(),
    _none() {}

CodeGen::~CodeGen() noexcept {
  if (_holder) {
    _destroyed = true;
    _holder->detach(this);
  }
}

// ============================================================================
// [asmjit::CodeGen - Events]
// ============================================================================

Error CodeGen::onAttach(CodeHolder* holder) noexcept {
  _archInfo = holder->getArchInfo();
  _lastError = kErrorOk;

  _globalHints = holder->getGlobalHints();
  _globalOptions = holder->getGlobalOptions();

  return kErrorOk;
}

Error CodeGen::onDetach(CodeHolder* holder) noexcept {
  _archInfo.reset();
  _finalized = false;

  _lastError = kErrorNotInitialized;
  _globalHints = 0;
  _globalOptions = kOptionMaybeFailureCase;

  _options = 0;
  _inlineComment = nullptr;
  _op4.reset();
  _op5.reset();
  _opMask.reset();

  return kErrorOk;
}

// ============================================================================
// [asmjit::CodeGen - Finalize]
// ============================================================================

Error CodeGen::finalize() {
  // Finalization does nothing by default, overridden by `X86Compiler`.
  return kErrorOk;
}

// ============================================================================
// [asmjit::CodeGen - Error Handling]
// ============================================================================

Error CodeGen::setLastError(Error error, const char* message) {
  // This is fatal, CodeGen can't set error without being attached to `CodeHolder`.
  ASMJIT_ASSERT(_holder != nullptr);

  // Special case used to reset the last error.
  if (error == kErrorOk)  {
    _lastError = kErrorOk;
    _globalOptions &= ~kOptionMaybeFailureCase;
    return kErrorOk;
  }

  if (!message)
    message = DebugUtils::errorAsString(error);

  // Logging is skipped if the error is handled by `ErrorHandler`.
  ErrorHandler* handler = _holder->_errorHandler;
  ASMJIT_TLOG("[ERROR] 0x%0.8u: %s%s\n",
    static_cast<unsigned int>(error), message, handler ? "" : " (ErrorHandler not attached)");

  if (handler && handler->handleError(error, message, this))
    return error;

  // The handler->handleError() function may throw an exception or longjmp()
  // to terminate the execution of `setLastError()`. This is the reason why
  // we have delayed changing the `_error` member until now.
  _lastError = error;

  return error;
}

// ============================================================================
// [asmjit::CodeGen - Helpers]
// ============================================================================

bool CodeGen::isLabelValid(uint32_t id) const noexcept {
  size_t index = Operand::unpackId(id);
  return _holder && id < _holder->_labels.getLength();
}

Error CodeGen::commentf(const char* fmt, ...) {
  Error err = _lastError;
  if (err) return err;

#if !defined(ASMJIT_DISABLE_LOGGING)
  if (_globalOptions & kOptionLoggingEnabled) {
    va_list ap;
    va_start(ap, fmt);
    Error err = _holder->_logger->logv(fmt, ap);
    va_end(ap);
  }
#else
  ASMJIT_UNUSED(fmt);
#endif

  return err;
}

Error CodeGen::commentv(const char* fmt, va_list ap) {
  Error err = _lastError;
  if (err) return err;

#if !defined(ASMJIT_DISABLE_LOGGING)
  if (_globalOptions & kOptionLoggingEnabled)
    err = _holder->_logger->logv(fmt, ap);
#else
  ASMJIT_UNUSED(fmt);
  ASMJIT_UNUSED(ap);
#endif

  return err;
}

// ============================================================================
// [asmjit::CodeGen - Emit]
// ============================================================================

#define OP const Operand_&
#define NO _none

Error CodeGen::emit(uint32_t instId) { return _emit(instId, NO, NO, NO, NO); }
Error CodeGen::emit(uint32_t instId, OP o0) { return _emit(instId, o0, NO, NO, NO); }
Error CodeGen::emit(uint32_t instId, OP o0, OP o1) { return _emit(instId, o0, o1, NO, NO); }
Error CodeGen::emit(uint32_t instId, OP o0, OP o1, OP o2) { return _emit(instId, o0, o1, o2, NO); }
Error CodeGen::emit(uint32_t instId, OP o0, OP o1, OP o2, OP o3) { return _emit(instId, o0, o1, o2, o3); }

Error CodeGen::emit(uint32_t instId, OP o0, OP o1, OP o2, OP o3, OP o4) {
  _op4 = o4;
  if (!o4.isNone()) _options |= kOptionHasOp4;
  return _emit(instId, o0, o1, o2, o3);
}

Error CodeGen::emit(uint32_t instId, OP o0, OP o1, OP o2, OP o3, OP o4, OP o5) {
  _op4 = o4;
  _op5 = o5;
  if (!o4.isNone()) _options |= kOptionHasOp4;
  if (!o5.isNone()) _options |= kOptionHasOp5;
  return _emit(instId, o0, o1, o2, o3);
}

Error CodeGen::emit(uint32_t instId, int o0) { return _emit(instId, Imm(o0), NO, NO, NO); }
Error CodeGen::emit(uint32_t instId, OP o0, int o1) { return _emit(instId, o0, Imm(o1), NO, NO); }
Error CodeGen::emit(uint32_t instId, OP o0, OP o1, int o2) { return _emit(instId, o0, o1, Imm(o2), NO); }
Error CodeGen::emit(uint32_t instId, OP o0, OP o1, OP o2, int o3) { return _emit(instId, o0, o1, o2, Imm(o3)); }

Error CodeGen::emit(uint32_t instId, OP o0, OP o1, OP o2, OP o3, int o4) {
  _options |= kOptionHasOp4;
  _op4 = Imm(o4);
  return _emit(instId, o0, o1, o2, o3);
}

Error CodeGen::emit(uint32_t instId, OP o0, OP o1, OP o2, OP o3, OP o4, int o5) {
  _options |= kOptionHasOp4 | kOptionHasOp5;
  _op4 = o4;
  _op5 = Imm(o5);
  return _emit(instId, o0, o1, o2, o3);
}

Error CodeGen::emit(uint32_t instId, int64_t o0) { return _emit(instId, Imm(o0), NO, NO, NO); }
Error CodeGen::emit(uint32_t instId, OP o0, int64_t o1) { return _emit(instId, o0, Imm(o1), NO, NO); }
Error CodeGen::emit(uint32_t instId, OP o0, OP o1, int64_t o2) { return _emit(instId, o0, o1, Imm(o2), NO); }
Error CodeGen::emit(uint32_t instId, OP o0, OP o1, OP o2, int64_t o3) { return _emit(instId, o0, o1, o2, Imm(o3)); }

Error CodeGen::emit(uint32_t instId, OP o0, OP o1, OP o2, OP o3, int64_t o4) {
  _options |= kOptionHasOp4;
  _op4 = Imm(o4);
  return _emit(instId, o0, o1, o2, o3);
}

Error CodeGen::emit(uint32_t instId, OP o0, OP o1, OP o2, OP o3, OP o4, int64_t o5) {
  _options |= kOptionHasOp4 | kOptionHasOp5;
  _op4 = o4;
  _op5 = Imm(o5);
  return _emit(instId, o0, o1, o2, o3);
}

#undef NO
#undef OP

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
