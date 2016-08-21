cmake_minimum_required(VERSION 3.1)

# =============================================================================
# [AsmJit - Configuration]
# =============================================================================

# set(ASMJIT_EMBED FALSE)                       # Embed (no targets, just project).
# set(ASMJIT_STATIC FALSE)                      # Whether to build a static library.
# set(ASMJIT_TRACE FALSE)                       # Used for debugging asmjit itself.

# set(ASMJIT_BUILD_ARM FALSE)                   # Whether to build ARM backend.
# set(ASMJIT_BUILD_X86 FALSE)                   # Whether to build X86 backend.
# set(ASMJIT_BUILD_TEST FALSE)                  # Whether to build tests and samples.

# =============================================================================
# [AsmJit - Project]
# =============================================================================

if(NOT ASMJIT_DIR)
  set(ASMJIT_DIR ${CMAKE_CURRENT_LIST_DIR})
endif()

include("${ASMJIT_DIR}/CxxProject.cmake")
cxx_project(asmjit)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(ASMJIT_PRIVATE_LFLAGS "/OPT:REF /OPT:ICF")

  list(APPEND ASMJIT_PRIVATE_CFLAGS /GF)
  list(APPEND ASMJIT_PRIVATE_CFLAGS_DBG /GS /GR-)
  list(APPEND ASMJIT_PRIVATE_CFLAGS_REL /Oi /Oy /GS- /GR-)
  if(NOT MSVC60 AND NOT MSVC70 AND NOT MSVC71)
    list(APPEND ASMJIT_PRIVATE_CFLAGS /MP)      # Enable multi-process compilation.
  endif()
endif()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "^(GNU|Clang)$")
  cxx_detect_standard(ASMJIT_PRIVATE_CFLAGS)
  cxx_detect_cflags(ASMJIT_PRIVATE_CFLAGS
    "-fno-tree-vectorize"
    "-fvisibility=hidden"
    "-Winconsistent-missing-override")
  list(APPEND ASMJIT_PRIVATE_CFLAGS_REL -fmerge-all-constants)
endif()

if(ASMJIT_TRACE)
  list(APPEND ASMJIT_PRIVATE_CFLAGS "${CXX_DEFINE}ASMJIT_TRACE")
endif()

if(WIN32)
  list(APPEND ASMJIT_PRIVATE_CFLAGS "${CXX_DEFINE}_UNICODE")
else()
  list(APPEND ASMJIT_DEPS pthread)
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
  list(APPEND ASMJIT_DEPS rt)
endif()

set(ASMJIT_LIBS ${ASMJIT_DEPS})
if(NOT ASMJIT_EMBED)
  list(INSERT ASMJIT_LIBS 0 asmjit)
endif()

if(ASMJIT_BUILD_ARM)
  List(APPEND ASMJIT_CFLAGS         "${CXX_DEFINE}ASMJIT_BUILD_ARM")
  List(APPEND ASMJIT_PRIVATE_CFLAGS "${CXX_DEFINE}ASMJIT_BUILD_ARM")
endif()

if(ASMJIT_BUILD_X86)
  List(APPEND ASMJIT_CFLAGS         "${CXX_DEFINE}ASMJIT_BUILD_X86")
  List(APPEND ASMJIT_PRIVATE_CFLAGS "${CXX_DEFINE}ASMJIT_BUILD_X86")
endif()

cxx_project_info(asmjit)

# =============================================================================
# [AsmJit - Source]
# =============================================================================

set(ASMJIT_SRC "")

cxx_define_source(asmjit ASMJIT_SRC asmjit
  apibegin.h
  apiend.h

  asmjit.h
  base.h
  build.h

  arm.h
  x86.h
)

cxx_define_source(asmjit ASMJIT_SRC asmjit/base
  assembler.cpp
  assembler.h
  codebuilder.cpp
  codebuilder.h
  codecompiler.cpp
  codecompiler.h
  codeemitter.cpp
  codeemitter.h
  codeholder.cpp
  codeholder.h
  constpool.cpp
  constpool.h
  cpuinfo.cpp
  cpuinfo.h
  func.cpp
  func.h
  globals.cpp
  globals.h
  logging.cpp
  logging.h
  operand.cpp
  operand.h
  osutils.cpp
  osutils.h
  regalloc.cpp
  regalloc_p.h
  runtime.cpp
  runtime.h
  simdtypes.h
  string.cpp
  string.h
  utils.cpp
  utils.h
  vmem.cpp
  vmem.h
  zone.cpp
  zone.h
  zonecontainers.cpp
  zonecontainers.h
  zoneheap.cpp
  zoneheap.h
)

if(0)
cxx_define_source(asmjit ASMJIT_SRC asmjit/arm
  armassembler.cpp
  armassembler.h
  arminst.cpp
  arminst.h
  armoperand.cpp
  armoperand_regs.cpp
  armoperand.h
)
endif()

cxx_define_source(asmjit ASMJIT_SRC asmjit/x86
  x86assembler.cpp
  x86assembler.h
  x86compiler.cpp
  x86compiler.h
  x86emitter.h
  x86func.cpp
  x86func_p.h
  x86inst.cpp
  x86inst.h
  x86logging.cpp
  x86logging.h
  x86misc.h
  x86operand.cpp
  x86operand_regs.cpp
  x86operand.h
  x86regalloc.cpp
  x86regalloc_p.h
)

# =============================================================================
# [AsmJit - Targets]
# =============================================================================

if(NOT ASMJIT_EMBED)
  # Add `asmjit` library.
  cxx_define_library(asmjit asmjit
    "${ASMJIT_SRC}"
    "${ASMJIT_DEPS}"
    "${ASMJIT_PRIVATE_CFLAGS}"
    "${ASMJIT_PRIVATE_CFLAGS_DBG}"
    "${ASMJIT_PRIVATE_CFLAGS_REL}")

  foreach(_src_file ${ASMJIT_SRC})
    get_filename_component(_src_dir ${_src_file} PATH)
    get_filename_component(_src_name ${_src_file} NAME)
    string(REGEX REPLACE "^${ASMJIT_SOURCE_DIR}/" "" targetpath "${_src_dir}")
    if("${_src_name}" MATCHES ".h$")
      if(NOT "${_src_name}" MATCHES "_p.h$")
        install(FILES ${_src_file} DESTINATION "include/${targetpath}")
      endif()
    endif()
  endforeach()

  # Add `asmjit` tests and samples.
  if(ASMJIT_BUILD_TEST)
    cxx_define_source(asmjit ASMJIT_TEST_SRC test asmjit_test_unit.cpp broken.cpp broken.h)
    cxx_define_executable(asmjit asmjit_test_unit
      "${ASMJIT_SRC};${ASMJIT_TEST_SRC}"
      "${ASMJIT_DEPS}"
      "${ASMJIT_PRIVATE_CFLAGS};${CXX_DEFINE}ASMJIT_TEST;${CXX_DEFINE}ASMJIT_EMBED"
      "${ASMJIT_PRIVATE_CFLAGS_DBG}"
      "${ASMJIT_PRIVATE_CFLAGS_REL}")

    foreach(_target asmjit_bench_x86 asmjit_test_opcode asmjit_test_x86)
      cxx_define_executable(asmjit ${_target} "src/test/${_target}.cpp" "${ASMJIT_LIBS}" "${ASMJIT_CFLAGS}" "" "")
    endforeach()
  endif()
endif()
