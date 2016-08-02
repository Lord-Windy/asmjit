// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_CODEHOLDER_H
#define _ASMJIT_BASE_CODEHOLDER_H

// [Dependencies]
#include "../base/archinfo.h"
#include "../base/containers.h"
#include "../base/logging.h"
#include "../base/operand.h"
#include "../base/runtime.h"
#include "../base/simdtypes.h"
#include "../base/utils.h"
#include "../base/zone.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

class Assembler;
class CodeHolder;
class CodeGen;

// ============================================================================
// [asmjit::AlignMode]
// ============================================================================

//! Code/Data align-mode.
ASMJIT_ENUM(AlignMode) {
  kAlignCode = 0,                        //!< Align executable code.
  kAlignData = 1,                        //!< Align non-executable code.
  kAlignZero = 2                         //!< Align by a sequence of zeros.
};

// ============================================================================
// [asmjit::RelocMode]
// ============================================================================

//! Relocation mode.
ASMJIT_ENUM(RelocMode) {
  kRelocAbsToAbs = 0,                    //!< Relocate absolute to absolute.
  kRelocRelToAbs = 1,                    //!< Relocate relative to absolute.
  kRelocAbsToRel = 2,                    //!< Relocate absolute to relative.
  kRelocTrampoline = 3                   //!< Relocate absolute to relative or use trampoline.
};

// ============================================================================
// [asmjit::ErrorHandler]
// ============================================================================

//! Error handler can be used to override the default behavior of error handling
//! available to all classes that inherit \ref CodeGen. See \ref handleError().
class ASMJIT_VIRTAPI ErrorHandler {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `ErrorHandler` instance.
  ASMJIT_API ErrorHandler() noexcept;
  //! Destroy the `ErrorHandler` instance.
  ASMJIT_API virtual ~ErrorHandler() noexcept;

  // --------------------------------------------------------------------------
  // [Handle Error]
  // --------------------------------------------------------------------------

  //! Error handler (abstract).
  //!
  //! Error handler is called after an error happened and before it's propagated
  //! to the caller. There are multiple ways how the error handler can be used:
  //!
  //! 1. Returning `true` or `false` from `handleError()`. If `true` is returned
  //!    it means that the error was reported and AsmJit can continue execution.
  //!    The reported error still be propagated to the caller, but won't put the
  //!    code-generator into an error state (it won't set last-error). However,
  //!    returning `false` means that the error cannot be handled - in such case
  //!    it stores the error, which can be then retrieved by using `getLastError()`.
  //!    Returning `false` is the default behavior when no error handler is provided
  //!    To put the assembler into a non-error state again a `resetLastError()`
  //!    must be called.
  //!
  //! 2. Throwing an exception. AsmJit doesn't use exceptions and is completely
  //!    exception-safe, but you can throw exception from your error handler if
  //!    this way is the preferred way of handling errors in your product. Throwing
  //!    an exception acts virtually as returning `true` as AsmJit won't be able
  //!    to store the error because of the exception thrown.
  //!
  //! 3. Using plain old C's `setjmp()` and `longjmp()`. Asmjit always puts
  //!    `Assembler` and `Compiler` to a consistent state before calling the
  //!    `handleError()` so `longjmp()` can be used without any issues to cancel
  //!    the code-generation if an error occurred. There is no difference between
  //!    exceptions and longjmp() from AsmJit's perspective.
  virtual bool handleError(Error err, const char* message, CodeGen* origin) = 0;
};

// ============================================================================
// [asmjit::CodeSection]
// ============================================================================

//! Code or data section.
struct CodeSection {
  //! Section flags.
  ASMJIT_ENUM(Flags) {
    kFlagExec        = 0x00000001U,      //!< Executable (.text sections).
    kFlagConst       = 0x00000002U,      //!< Read-only (.text and .data sections).
    kFlagZero        = 0x00000004U,      //!< Zero initialized by the loader (BSS).
    kFlagInfo        = 0x00000008U       //!< Info / comment flag.
  };

  uint32_t id;                           //!< Section id.
  uint32_t flags;                        //!< Section flags.
  uint32_t alignment;                    //!< Section alignment requirements (0 if no requirements).
  char name[36];                         //!< Section name (max 35 characters, PE allows max 8).
};

// ============================================================================
// [asmjit::CodeBuffer]
// ============================================================================

//! Code or data buffer.
struct CodeBuffer {
  uint8_t* data;                         //!< The content of the buffer (data).
  size_t length;                         //!< Number of bytes of `data` used.
  size_t capacity;                       //!< Buffer capacity (in bytes).
  bool isExternal;                       //!< True if this is external buffer.
  bool isFixedSize;                      //!< True if this buffer cannot grow.
};

// ============================================================================
// [asmjit::CodeHolder]
// ============================================================================

//! Contains basic information about the target architecture plus its settings,
//! and holds code & data (including sections, labels, and relocation information).
//! CodeHolder can store both binary and intermediate representation of assembly,
//! which can be generated by \ref Assembler and/or \ref AsmBuilder.
//!
//! NOTE: CodeHolder has ability to attach an \ref ErrorHandler, however, this
//! error handler is not triggered by holder itself, it's only used by the
//! attached code generators.
class ASMJIT_VIRTAPI CodeHolder {
public:
  ASMJIT_NO_COPY(CodeHolder)

  //! Code or data section entry.
  struct SectionEntry {
    CodeSection info;                    //!< Section information (name, flags, alignment).
    CodeBuffer buffer;                   //!< Machine code & data of this section.
  };

  //! Data structure used to link labels.
  struct LabelLink {
    LabelLink* prev;                     //!< Previous link (single-linked list).
    intptr_t offset;                     //!< Label offset relative to the start of the section.
    intptr_t displacement;               //!< Inlined displacement.
    intptr_t relocId;                    //!< Relocation id (in case it's needed).
  };

  //! Label data.
  struct LabelEntry {
    intptr_t offset;                     //!< Label offset.
    LabelLink* links;                    //!< Label links.
  };

  //! Code relocation data.
  //!
  //! X86/X64 Specific
  //! ----------------
  //!
  //! X86 architecture uses 32-bit absolute addressing model encoded in memory
  //! operands, but 64-bit mode uses relative addressing model (RIP + displacement).
  struct RelocEntry {
    uint32_t type;                       //!< Type of the relocation.
    uint32_t size;                       //!< Size of the relocation (4 or 8 bytes).
    uint64_t from;                       //!< Offset from the initial address.
    uint64_t data;                       //!< Displacement from the initial/absolute address.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_API CodeHolder(uint32_t archId = ArchInfo::kIdHost) noexcept;
  ASMJIT_API ~CodeHolder() noexcept;

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  //! Detach all code-generators attached and reset the \ref CodeHolder.
  ASMJIT_API void reset(bool releaseMemory = false) noexcept;

  // --------------------------------------------------------------------------
  // [Attach / Detach]
  // --------------------------------------------------------------------------

  //! Attach a \ref CodeGen to this \ref CodeHolder.
  ASMJIT_API Error attach(CodeGen* cg) noexcept;
  //! Detach a \ref CodeGen from this \ref CodeHolder.
  ASMJIT_API Error detach(CodeGen* cg) noexcept;

  // --------------------------------------------------------------------------
  // [Sync]
  // --------------------------------------------------------------------------

  //! Synchronize all states of all `CodeGen`s associated with the CodeHolder.
  //! This is required as some code generators don't sync every time they do
  //! something - for example \ref Assembler generally syncs when it needs to
  //! reallocate the \ref CodeBuffer, but not each time it encodes instruction
  //! or directive.
  ASMJIT_API void sync() noexcept;

  // --------------------------------------------------------------------------
  // [Architecture Information]
  // --------------------------------------------------------------------------

  //! Get information about the target architecture, see \ref ArchInfo.
  ASMJIT_INLINE const ArchInfo& getArchInfo() const noexcept { return _archInfo; }
  //! Get the target architecture.
  ASMJIT_INLINE uint32_t getArchId() const noexcept { return _archInfo._archId; }
  //! Get the target architecture's GP register size (4 or 8 bytes).
  ASMJIT_INLINE uint32_t getGpSize() const noexcept { return _archInfo._gpSize; }

  //! Switch the `CodeHolder`' architecture to `archId`.
  //!
  //! NOTE: It's not allowed to change the architecture after a
  ASMJIT_API Error setArchId(uint32_t archId) noexcept;

  // --------------------------------------------------------------------------
  // [Global Information]
  // --------------------------------------------------------------------------

  //! Get if a static base-address is set.
  ASMJIT_INLINE bool hasBaseAddress() const noexcept { return _baseAddress != kNoBaseAddress; }
  //! Get a static base-address (uint64_t).
  ASMJIT_INLINE uint64_t getBaseAddress() const noexcept { return _baseAddress; }

  //! Get global hints, internally propagated to all `CodeGen`s attached.
  ASMJIT_INLINE uint32_t getGlobalHints() const noexcept { return _globalHints; }
  //! Get global options, internally propagated to all `CodeGen`s attached.
  ASMJIT_INLINE uint32_t getGlobalOptions() const noexcept { return _globalOptions; }

  // --------------------------------------------------------------------------
  // [Result Information]
  // --------------------------------------------------------------------------

  //! Get the size code & data of all sections.
  ASMJIT_API size_t getCodeSize() const noexcept;

  //! Get size of all possible trampolines.
  //!
  //! Trampolines are needed to successfully generate relative jumps to absolute
  //! addresses. This value is only non-zero if jmp of call instructions were
  //! used with immediate operand (this means jumping or calling an absolute
  //! address directly).
  ASMJIT_INLINE size_t getTrampolinesSize() const noexcept { return _trampolinesSize; }

  // --------------------------------------------------------------------------
  // [Logging & Error Handling]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_DISABLE_LOGGING)
  //! Get if a logger attached.
  ASMJIT_INLINE bool hasLogger() const noexcept { return _logger != nullptr; }
  //! Get the attached logger.
  ASMJIT_INLINE Logger* getLogger() const noexcept { return _logger; }
  //! Attach a `logger` to CodeHolder and propagate it to all attached `CodeGen`s.
  ASMJIT_API void setLogger(Logger* logger) noexcept;
  //! Reset the logger (does nothing if not attached).
  ASMJIT_INLINE void resetLogger() noexcept { setLogger(nullptr); }
#endif // !ASMJIT_DISABLE_LOGGING

  //! Get if error-handler is attached.
  ASMJIT_INLINE bool hasErrorHandler() const noexcept { return _errorHandler != nullptr; }
  //! Get the error-handler.
  ASMJIT_INLINE ErrorHandler* getErrorHandler() const noexcept { return _errorHandler; }
  //! Set the error handler, will affect all attached `CodeGen`s.
  ASMJIT_API Error setErrorHandler(ErrorHandler* handler) noexcept;
  //! Reset the error handler (does nothing if not attached).
  ASMJIT_INLINE Error resetErrorHandler() noexcept { return setErrorHandler(nullptr); }

  // --------------------------------------------------------------------------
  // [Sections]
  // --------------------------------------------------------------------------

  //! Get array of `SectionEntry*` records.
  ASMJIT_INLINE const PodVector<SectionEntry*>& getSections() const noexcept { return _sections; }

  ASMJIT_API Error growBuffer(CodeBuffer* cb, size_t n) noexcept;
  ASMJIT_API Error reserveBuffer(CodeBuffer* cb, size_t n) noexcept;

  // --------------------------------------------------------------------------
  // [Labels & Symbols]
  // --------------------------------------------------------------------------

  //! Create a new label id which can be associated with \ref Label.
  //!
  //! Returns `Error`, does not trigger \ref ErrorHandler on error.
  ASMJIT_API Error newLabelId(uint32_t& out) noexcept;

  //! Create a new label-link used to store information about yet unbound labels.
  //!
  //! Returns `null` if the allocation failed.
  ASMJIT_API LabelLink* newLabelLink() noexcept;

  //! Get array of `LabelEntry*` records.
  ASMJIT_INLINE const PodVector<LabelEntry*>& getLabels() const noexcept { return _labels; }

  //! Get number of labels created.
  ASMJIT_INLINE size_t getLabelsCount() const noexcept { return _labels.getLength(); }

  //! Get if the `label` is valid (i.e. created by `newLabelId()`).
  ASMJIT_INLINE bool isLabelValid(const Label& label) const noexcept {
    return isLabelValid(label.getId());
  }
  //! Get if the label having `id` is valid (i.e. created by `newLabelId()`).
  ASMJIT_INLINE bool isLabelValid(uint32_t labelId) const noexcept {
    size_t index = Operand::unpackId(labelId);
    return index < _labels.getLength();
  }

  //! Get if the `label` is already bound.
  //!
  //! Returns `false` if the `label` is not valid.
  ASMJIT_INLINE bool isLabelBound(const Label& label) const noexcept {
    return isLabelBound(label.getId());
  }
  //! \overload
  ASMJIT_INLINE bool isLabelBound(uint32_t id) const noexcept {
    size_t index = Operand::unpackId(id);
    return index < _labels.getLength() && _labels[index]->offset != -1;
  }

  //! Get a `label` offset or -1 if the label is not yet bound.
  ASMJIT_INLINE intptr_t getLabelOffset(const Label& label) const noexcept {
    return getLabelOffset(label.getId());
  }
  //! \overload
  ASMJIT_INLINE intptr_t getLabelOffset(uint32_t id) const noexcept {
    ASMJIT_ASSERT(isLabelValid(id));
    return _labels[Operand::unpackId(id)]->offset;
  }

  //! Get information about the given `label`.
  ASMJIT_INLINE LabelEntry* getLabelEntry(const Label& label) const noexcept {
    return getLabelEntry(label.getId());
  }
  //! Get information about a label having the given `id`.
  ASMJIT_INLINE LabelEntry* getLabelEntry(uint32_t id) const noexcept {
    size_t index = static_cast<size_t>(Operand::unpackId(id));
    return index < _labels.getLength() ? _labels[index] : static_cast<LabelEntry*>(nullptr);
  }

  // --------------------------------------------------------------------------
  // [Relocate]
  // --------------------------------------------------------------------------

  //! Relocate the code to `baseAddress` and copy it to `dst`.
  //!
  //! \param dst Contains the location where the relocated code should be
  //! copied. The pointer can be address returned by virtual memory allocator
  //! or any other address that has sufficient space.
  //!
  //! \param baseAddress Base address used for relocation. The `JitRuntime`
  //! always sets the `baseAddress` address to be the same as `dst`.
  //!
  //! \return The number bytes actually used. If the code generator reserved
  //! space for possible trampolines, but didn't use it, the number of bytes
  //! used can actually be less than the expected worst case. Virtual memory
  //! allocator can shrink the memory allocated first time.
  //!
  //! A given buffer will be overwritten, to get the number of bytes required,
  //! use `getCodeSize()`.
  ASMJIT_API size_t relocate(void* dst, uint64_t baseAddress = kNoBaseAddress) const noexcept;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  ArchInfo _archInfo;                    //!< Basic info about the target architecture.
  uint64_t _baseAddress;                 //!< Static base-address of the generated code (optional).

  uint8_t _isLocked;                     //!< If the `CodeHolder` settings are locked.
  uint8_t _reserved[3];

  uint32_t _globalHints;                 //!< Global hints, propagated to all `CodeGen`s.
  uint32_t _globalOptions;               //!< Global options, propagated to all `CodeGen`s.

  CodeGen* _cgList;                      //!< List of attached `CodeGen`s.
  Assembler* _cgAsm;                     //!< Attached \ref Assembler (only one at a time).

  Logger* _logger;                       //!< Attached \ref Logger, used by all consumers.
  ErrorHandler* _errorHandler;           //!< Attached \ref ErrorHandler.

  uint32_t _trampolinesSize;             //!< Size of all possible trampolines.

  Zone _baseAllocator;                   //!< Base allocator (sections, labels, and relocations).
  PodVectorTmp<SectionEntry*, 1> _sections; //!< Section entries.
  PodVector<LabelEntry*> _labels;        //!< Label entries.
  LabelLink* _unusedLinks;               //!< Pool of unused `LabelLink`s.
  PodVector<RelocEntry> _relocations;    //!< Relocation entries.

  SectionEntry _defaultSection;          //!< Statically allocated default section.
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_CODEHOLDER_H
