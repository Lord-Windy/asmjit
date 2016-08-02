// [Generate-X86]
//
// The purpose of this script is to fetch all instructions' names into a single
// string and to optimize common patterns that appear in instruction data. It
// prevents relocation of small strings (instruction names) that has to be done
// by a linker to make all pointers the binary application/library uses valid.
// This approach decreases the final size of AsmJit binary and relocation data.
//
// NOTE: This script now relies on 'asmdb' package. Either install it by using
// node.js package manager (npm) or copy x86data.js and x86util.js files into
// the asmjit/tools directory.

// IMPORTANT: THIS SCRIPT WILL BE CLEANED UP, IT CONTAINS A LOT OF STUFF FROM
// AVX-512 SCRIPTING, WHICH IS NO LONGER NEEDED.

"use strict";

const fs = require("fs");
const hasOwn = Object.prototype.hasOwnProperty;

// Prefer a local asmdb copy package if possible;
const asmdb = (function() {
  try {
    return {
      x86data: require("./x86data.js"),
      x86util: require("./x86util.js")
    };
  }
  catch (ex) {
    return require("asmdb.js");
  }
})();

// Special cases.
const x86db = new asmdb.x86util.X86DataBase().addDefault();
x86db.addInstructions([
  // Call in [imm] form (absolute address handled by AsmJit).
  ["call"  , "id"         , "D"    , "E8 cd"              , "ANY VOLATILE OF=U SF=U ZF=U AF=U PF=U CF=U"],
  ["call"  , "iq"         , "D"    , "E8 cd"              , "ANY VOLATILE OF=U SF=U ZF=U AF=U PF=U CF=U"],

  // Jmp in [imm] form (absolute address handled by AsmJit).
  ["jmp"   , "id"         , "D"    , "E9 cd"              , "ANY VOLATILE"],
  ["jmp"   , "iq"         , "D"    , "E9 cd"              , "ANY VOLATILE"],

  // Imul in [reg, imm] form (a signature expanded to [reg, reg, imm] by AsmJit).
  ["imul"  , "r16, ib"    , "RM"   , "66 6B /r ib"        , "ANY OF=W SF=W ZF=U AF=U PF=U CF=W"],
  ["imul"  , "r32, ib"    , "RM"   , "6B /r ib"           , "ANY OF=W SF=W ZF=U AF=U PF=U CF=W"],
  ["imul"  , "r64, ib"    , "RM"   , "REX.W 6B /r ib"     , "X64 OF=W SF=W ZF=U AF=U PF=U CF=W"],
  ["imul"  , "r16, iw"    , "RM"   , "66 69 /r iw"        , "ANY OF=W SF=W ZF=U AF=U PF=U CF=W"],
  ["imul"  , "r32, id"    , "RM"   , "69 /r id"           , "ANY OF=W SF=W ZF=U AF=U PF=U CF=W"],
  ["imul"  , "r64, id"    , "RM"   , "REX.W 69 /r id"     , "X64 OF=W SF=W ZF=U AF=U PF=U CF=W"],
]);

// ----------------------------------------------------------------------------
// [Misc]
// ----------------------------------------------------------------------------

const kFileName = "../src/asmjit/x86/x86inst.cpp";
const kIndent = "  ";
const kJustify = 79;
const kX86InstPrefix = "X86Inst::kId";

const kDisclaimerStart = "// ------------------- Automatically generated, do not edit -------------------\n";
const kDisclaimerEnd   = "// ----------------------------------------------------------------------------\n";

// ----------------------------------------------------------------------------
// [Utils]
// ----------------------------------------------------------------------------

class Utils {
  static upFirst(s) {
    if (!s) return "";
    return s[0].toUpperCase() + s.substr(1);
  }

  static trimLeft(s) {
    return s.replace(/^\s+/, "");
  }

  static padLeft(s, n, x) {
    if (!x)
      x = " ";
    s = String(s);
    while (s.length < n)
      s += x;
    return s;
  }

  static inject(s, start, end, code) {
    var iStart = s.indexOf(start);
    var iEnd   = s.indexOf(end);

    if (iStart === -1)
      throw new Error(`Couldn't locate start mark.`);

    if (iEnd === -1)
      throw new Error(`Couldn't locate end mark.`);

    return s.substr(0, iStart + start.length) + code + s.substr(iEnd);
  }
}

// ----------------------------------------------------------------------------
// [IndexedString]
// ----------------------------------------------------------------------------

class IndexedString {
  constructor() {
    this.map = Object.create(null);
    this.size = -1;
    this.array = [];
  }

  add(s) {
    this.map[s] = -1;
  }

  index() {
    const map = this.map;
    const array = this.array;
    const partialMap = Object.create(null);

    var k, kp;
    var i, len;

    // Create a map that will contain all keys and partial keys.
    for (k in map) {
      if (!k) {
        partialMap[k] = k;
      }
      else {
        for (i = 0, len = k.length; i < len; i++) {
          kp = k.substr(i);
          if (!hasOwn.call(partialMap, kp) || partialMap[kp].length < len)
            partialMap[kp] = k;
        }
      }
    }

    // Create an array that will only contain keys that are needed.
    for (k in map)
      if (partialMap[k] === k)
        array.push(k);
    array.sort();

    // Create valid offsets to the `array`.
    var offMap = Object.create(null);
    var offset = 0;

    for (i = 0, len = array.length; i < len; i++) {
      k = array[i];

      offMap[k] = offset;
      offset += k.length + 1;
    }
    this.size = offset;

    // Assign valid offsets to `map`.
    for (kp in map) {
      k = partialMap[kp];
      map[kp] = offMap[k] + k.length - kp.length;
    }
  }

  format(indent, justify) {
    if (this.size === -1)
      throw new Error(`IndexedString - not indexed yet, call index()`);

    const array = this.array;
    if (!justify) justify = 0;

    var i;
    var s = "";
    var line = "";

    for (i = 0; i < array.length; i++) {
      const item = "\"" + array[i] + ((i !== array.length - 1) ? "\\0\"" : "\";");
      const newl = line + (line ? " " : indent) + item;

      if (newl.length <= justify) {
        line = newl;
        continue;
      }
      else {
        s += line + "\n";
        line = indent + item;
      }
    }

    s += line + "\n";
    return s;
  }

  getSize() {
    if (this.size === -1)
      throw new Error(`IndexedString - not indexed yet, call index()`);
    return this.size;
  }

  getIndex(k) {
    if (this.size === -1)
      throw new Error(`IndexedString - not indexed yet, call index()`);

    if (!hasOwn.call(this.map, k))
      throw new Error(`IndexedString - key '${k}' not found.`);

    return this.map[k];
  }
}
// ----------------------------------------------------------------------------
// [Generate]
// ----------------------------------------------------------------------------

function decToHex(n, nPad) {
  var hex = Number(n < 0 ? 0x100000000 + n : n).toString(16);
  while (nPad > hex.length)
    hex = "0" + hex;
  return "0x" + hex.toUpperCase();
}

function getEFlagsMask(eflags, passing) {
  var msk = 0x0;
  var bit = 0x1;

  for (var i = 0; i < 8; i++, bit <<= 1)
    if (passing.indexOf(eflags[i]) !== -1)
      msk |= bit;

  return msk;
}

// Decompose opcode inside `INST` into its components.
// O|V(XXXXXX,XX,O,L,W,EVEX.W, EVEX.N)
function decomposeOpCode(opcode) {
  var m = opcode.match(/^(O|V)\(([A-Z0-9_]{6}),([A-F0-9]{2}),([0-7]|U|_),([a-zA-Z0-9_]+),([a-zA-Z0-9_]+),([a-zA-Z0-9_]+),([a-zA-Z0-9_]+)\)$/);
  if (m) {
    return {
      type  : m[1],
      prefix: m[2],
      opcode: m[3],
      o     : m[4],
      l     : m[5],
      w     : m[6],
      ew    : m[7],
      en    : m[7]
    };
  }
  else {
    console.log(`FAILED TO DECOMPOSE: ${opcode}`);
    return null;
  }
}

// Compose opcode back to what INST expects.
function composeOpCode(obj) {
  var w = obj.w;
  var ew = obj.ew;

  return `${obj.type}(${obj.prefix},${obj.opcode},${obj.o},${obj.l},${w},${ew},${obj.en})`;
}

function isAVXGroup(insts) {
  for (var i = 0; i < insts.length; i++) {
    const inst = insts[i];
    const operands = inst.operands;
    for (var j = 0; j < operands.length; j++) {
      if (/^(?:xmm|ymm|zmm)$/.test(operands[i]))
        return true;
    }
  }
  return false;
}

const MapW = {
  ""   : "_",
  "W0" : "0",
  "W1" : "1",
  "WIG": "I"
}
const MapL = {
  ""   : "_",
  "L0" : "0",
  "L1" : "1",
  "128": "0",
  "256": "1",
  "512": "2",
  "LIG": "I"
}

const AVX512Features = {
  "AVX512F"   : true,
  "AVX512DQ"  : true,
  "AVX512BW"  : true,
  "AVX512CD"  : true,
  "AVX512ER"  : true,
  "AVX512PF"  : true,
  "AVX512IFMA": true,
  "AVX512VBMI": true
};

function AVX512Flags(insts) {
  const cpu = Object.create(null);

  var hasEVEX = false;
  var avx512 = "";
  var exclusive = 1;
  var vl = "0";
  var broadcast = "0";
  var masking = "0";
  var er_sae = "0"
  var tupleType = "";

  for (var i = 0; i < insts.length; i++) {
    const inst = insts[i];
    const icpu = inst.cpu;

    if (inst.prefix === "EVEX")
      hasEVEX = true;

    if (icpu.AVX || icpu.AVX2 || icpu.FMA || icpu.F16C)
      exclusive = 0;

    if (icpu.AVX512VL)
      vl = "1";

    if (inst.tupleType)
      tupleType = inst.tupleType;

    var thisAvx512 = false;
    for (var k in icpu) {
      if (k in AVX512Features) {
        avx512 = k;
        thisAvx512 = true;
      }
    }

    if (thisAvx512) {
      var operands = inst.operands;
      if (inst.broadcast)
        broadcast = String(inst.elementSize / 8);

      if (inst.zmask)
        masking = "kz";
      else if (inst.kmask)
        masking = "k_";

      if (inst.rnd)
        er_sae = "erc";
      else if (inst.sae)
        er_sae = "sae"
    }
  }

  if (!hasEVEX || !avx512)
    return null;

  avx512 = avx512.substr("AVX512".length);

  return {
    exclusive: exclusive,
    feature  : avx512 === "F" ? "F_" : avx512,
    tupleType: tupleType ? tupleType : "0",
    masking  : masking,
    vl       : vl ? vl : "0",
    er_sae   : er_sae ? er_sae : "0",
    broadcast: broadcast ? "B" : "0"
  };
}

const OpSortPriority = {
  "read"    :-9,
  "write"   :-8,
  "rw"      :-7,
  "implicit":-6,

  "r8lo"    : 1,
  "r8hi"    : 2,
  "r16"     : 3,
  "r32"     : 4,
  "r64"     : 5,
  "fp"      : 6,
  "mm"      : 7,
  "k"       : 8,
  "xmm"     : 9,
  "ymm"     : 10,
  "zmm"     : 11,
  "sreg"    : 12,
  "bnd"     : 13,
  "creg"    : 14,
  "dreg"    : 15,

  "mem"     : 30,
  "m8"      : 31,
  "m16"     : 32,
  "m32"     : 33,
  "m64"     : 34,
  "m80"     : 35,
  "m128"    : 36,
  "m256"    : 37,
  "m512"    : 38,
  "m1024"   : 39,
  "mib"     : 40,

  "vm32x"   : 41,
  "vm32y"   : 42,
  "vm32z"   : 43,
  "vm64x"   : 44,
  "vm64y"   : 45,
  "vm64z"   : 46,

  "i4"      : 50,
  "i8"      : 51,
  "i16"     : 52,
  "i32"     : 53,
  "i64"     : 54,

  "rel8"    : 60,
  "rel32"   : 61
};

const OpToAsmJitOp = {
  "read"    : "FLAG(R)",
  "write"   : "FLAG(W)",
  "rw"      : "FLAG(X)",
  "implicit": "FLAG(Implicit)",

  "r8lo"    : "FLAG(GpbLo)",
  "r8hi"    : "FLAG(GpbHi)",
  "r16"     : "FLAG(Gpw)",
  "r32"     : "FLAG(Gpd)",
  "r64"     : "FLAG(Gpq)",
  "fp"      : "FLAG(Fp)",
  "mm"      : "FLAG(Mm)",
  "k"       : "FLAG(K)",
  "xmm"     : "FLAG(Xmm)",
  "ymm"     : "FLAG(Ymm)",
  "zmm"     : "FLAG(Zmm)",
  "sreg"    : "FLAG(Seg)",
  "bnd"     : "FLAG(Bnd)",
  "creg"    : "FLAG(Cr)",
  "dreg"    : "FLAG(Dr)",

  "mem"     : "FLAG(Mem)",
  "vm"      : "FLAG(Vm)",
  "m8"      : "MEM(M8)",
  "m16"     : "MEM(M16)",
  "m32"     : "MEM(M32)",
  "m64"     : "MEM(M64)",
  "m80"     : "MEM(M80)",
  "m128"    : "MEM(M128)",
  "m256"    : "MEM(M256)",
  "m512"    : "MEM(M512)",
  "m1024"   : "MEM(M1024)",
  "mib"     : "MEM(Mib)",
  "mAny"    : "MEM(Any)",

  "vm32x"   : "MEM(Vm32x)",
  "vm32y"   : "MEM(Vm32y)",
  "vm32z"   : "MEM(Vm32z)",
  "vm64x"   : "MEM(Vm64x)",
  "vm64y"   : "MEM(Vm64y)",
  "vm64z"   : "MEM(Vm64z)",

  "i4"      : "FLAG(I4)",
  "i8"      : "FLAG(I8)",
  "i16"     : "FLAG(I16)",
  "i32"     : "FLAG(I32)",
  "i64"     : "FLAG(I64)",

  "rel8"    : "FLAG(Rel8)",
  "rel32"   : "FLAG(Rel32)"
};

function OpSortFunc(a, b) {
  return (OpSortPriority[a] || 0) - (OpSortPriority[b] || 0);
}

function SortOpArray(a) {
  a.sort(OpSortFunc);
  return a;
}

function StringifyArray(a, map) {
  var s = "";
  for (var i = 0; i < a.length; i++) {
    const op = a[i];
    if (!hasOwn.call(map, op))
      throw new Error(`UNHANDLED OPERAND '${op}'`);
    s += (s ? " | " : "") + map[op];
  }
  return s ? s : "0";
}

class OSignature {
  constructor() {
    this.flags = Object.create(null);
  }

  equals(other) {
    const af = this.flags;
    const bf = other.flags;

    for (var k in af) if (!hasOwn.call(bf, k)) return false;
    for (var k in bf) if (!hasOwn.call(af, k)) return false;

    return true;
  }

  xor(other) {
    const result = Object.create(null);

    const af = this.flags;
    const bf = other.flags;

    for (var k in af) if (!hasOwn.call(bf, k)) result[k] = true;
    for (var k in bf) if (!hasOwn.call(af, k)) result[k] = true;

    return Object.getOwnPropertyNames(result).length === 0 ? null : result;
  }

  mergeWith(other) {
    const af = this.flags;
    const bf = other.flags;

    for (var k in bf)
      af[k] = true;
  }

  simplify() {
    const flags = this.flags;

    // Implicit register when also any other register can be specified.
    if (flags.al && flags.r8lo) delete flags["al"];
    if (flags.ah && flags.r8hi) delete flags["ah"];
    if (flags.ax  && flags.r16) delete flags["ax"];
    if (flags.eax && flags.r32) delete flags["eax"];
    if (flags.rax && flags.r64) delete flags["rax"];

    // Individual segment registers used by push/pop/...
    if (flags.es && flags.fs && flags.gs) {
      delete flags["cs"];
      delete flags["ss"];
      delete flags["ds"];
      delete flags["es"];
      delete flags["fs"];
      delete flags["gs"];
      delete flags["es"];
      flags.sreg = true;
    }

    // 32-bit register or 16-bit memory implies also 16-bit reg.
    if (flags.r32 && flags.m16) {
      flags.r16 = true;
    }

    // 32-bit register or 8-bit memory implies also 16-bit and 8-bit reg.
    if (flags.r32 && flags.m8) {
      flags.r8lo = true;
      flags.r8hi = true;
      flags.r16 = true;
    }
  }

  toString() {
    var s = "";
    var flags = this.flags;
    var prefix = (flags.read && flags.write) ? "X:" : (flags.write) ? "W:" : "R:";

    for (var k in flags) {
      if (k === "read" || k === "write" || k === "implicit")
        continue;
      s += (s ? "|" : "") + k;
    }

    if (flags.implicit)
      s = "<" + s + ">";

    return prefix + s;
  }

  toAsmJitOpData() {
    var s = "";
    var flags = this.flags;

    var mFlags = Object.create(null);
    var mMemFlags = Object.create(null);
    var mExtFlags = Object.create(null);
    var sRegIndex = "0xFF";

    if (flags.read && flags.write)
      mFlags.rw = true;
    else if (flags.write)
      mFlags.write = true;
    else
      mFlags.read = true;

    for (var k in flags) {
      switch (k) {
        case "read"    : break;
        case "write"   : break;

        case "implicit":
        case "r8lo"    :
        case "r8hi"    :
        case "r16"     :
        case "r32"     :
        case "r64"     :
        case "creg"    :
        case "dreg"    :
        case "sreg"    :
        case "bnd"     :
        case "fp"      :
        case "k"       :
        case "mm"      :
        case "xmm"     :
        case "ymm"     :
        case "zmm"     : mFlags[k]    = true; break;

        case "m8"      : mFlags.mem   = true; mMemFlags.m8    = true; break;
        case "m16"     : mFlags.mem   = true; mMemFlags.m16   = true; break;
        case "m32"     : mFlags.mem   = true; mMemFlags.m32   = true; break;
        case "m64"     : mFlags.mem   = true; mMemFlags.m64   = true; break;
        case "m80"     : mFlags.mem   = true; mMemFlags.m80   = true; break;
        case "m128"    : mFlags.mem   = true; mMemFlags.m128  = true; break;
        case "m256"    : mFlags.mem   = true; mMemFlags.m256  = true; break;
        case "m512"    : mFlags.mem   = true; mMemFlags.m512  = true; break;
        case "m1024"   : mFlags.mem   = true; mMemFlags.m1024 = true; break;
        case "mib"     : mFlags.mem   = true; mMemFlags.mib   = true; break;
        case "mem"     : mFlags.mem   = true; mMemFlags.mAny  = true; break;

        case "vm32x"   : mFlags.vm    = true; mMemFlags.vm32x = true; break;
        case "vm32y"   : mFlags.vm    = true; mMemFlags.vm32y = true; break;
        case "vm32z"   : mFlags.vm    = true; mMemFlags.vm32z = true; break;
        case "vm64x"   : mFlags.vm    = true; mMemFlags.vm64x = true; break;
        case "vm64y"   : mFlags.vm    = true; mMemFlags.vm64y = true; break;
        case "vm64z"   : mFlags.vm    = true; mMemFlags.vm64z = true; break;

        case "i4"      :
        case "i8"      :
        case "i16"     :
        case "i32"     :
        case "i64"     :
        case "rel8"    :
        case "rel32"   : mFlags[k]    = true; break;

        default: {
          const oldRegIndex = sRegIndex;
          switch (k) {
            case "al"    : mFlags.r8lo = true; sRegIndex = "0"; break;
            case "ah"    : mFlags.r8hi = true; sRegIndex = "0"; break;
            case "ax"    : mFlags.r16  = true; sRegIndex = "0"; break;
            case "eax"   : mFlags.r32  = true; sRegIndex = "0"; break;
            case "rax"   : mFlags.r64  = true; sRegIndex = "0"; break;
            case "bl"    : mFlags.r8lo = true; sRegIndex = "3"; break;
            case "bh"    : mFlags.r8hi = true; sRegIndex = "3"; break;
            case "bx"    : mFlags.r16  = true; sRegIndex = "3"; break;
            case "ebx"   : mFlags.r32  = true; sRegIndex = "3"; break;
            case "rbx"   : mFlags.r64  = true; sRegIndex = "3"; break;
            case "cl"    : mFlags.r8lo = true; sRegIndex = "1"; break;
            case "ch"    : mFlags.r8hi = true; sRegIndex = "1"; break;
            case "cx"    : mFlags.r16  = true; sRegIndex = "1"; break;
            case "ecx"   : mFlags.r32  = true; sRegIndex = "1"; break;
            case "rcx"   : mFlags.r64  = true; sRegIndex = "1"; break;
            case "dl"    : mFlags.r8lo = true; sRegIndex = "2"; break;
            case "dh"    : mFlags.r8hi = true; sRegIndex = "2"; break;
            case "dx"    : mFlags.r16  = true; sRegIndex = "2"; break;
            case "edx"   : mFlags.r32  = true; sRegIndex = "2"; break;
            case "rdx"   : mFlags.r64  = true; sRegIndex = "2"; break;
            case "si"    : mFlags.r16  = true; sRegIndex = "6"; break;
            case "esi"   : mFlags.r32  = true; sRegIndex = "6"; break;
            case "rsi"   : mFlags.r64  = true; sRegIndex = "6"; break;
            case "di"    : mFlags.r16  = true; sRegIndex = "7"; break;
            case "edi"   : mFlags.r32  = true; sRegIndex = "7"; break;
            case "rdi"   : mFlags.r64  = true; sRegIndex = "7"; break;
            case "fp0"   : mFlags.fp   = true; sRegIndex = "0"; break;
            case "xmm0"  : mFlags.xmm  = true; sRegIndex = "0"; break;
            case "ymm0"  : mFlags.ymm  = true; sRegIndex = "0"; break;
            default:
              console.log(`UNKNOWN OPERAND '${k}'`);
          }
          if (oldRegIndex !== "0xFF" && oldRegIndex !== sRegIndex)
            throw new Error(`Register index collission ${oldRegIndex} vs ${sRegIndex}`);
        }
      }
    }

    const sFlags    = StringifyArray(SortOpArray(Object.getOwnPropertyNames(mFlags   )), OpToAsmJitOp);
    const sMemFlags = StringifyArray(SortOpArray(Object.getOwnPropertyNames(mMemFlags)), OpToAsmJitOp);
    const sExtFlags = StringifyArray(SortOpArray(Object.getOwnPropertyNames(mExtFlags)), OpToAsmJitOp);

    return `OSIGNATURE(${sFlags || 0}, ${sMemFlags || 0}, ${sExtFlags || 0}, ${sRegIndex})`;
  }
}

class ISignature extends Array {
  constructor() {
    super();
    this.x86 = false;
    this.x64 = false;
    this.implicit = 0; // Number of implicit operands.
  }

  simplify() {
    for (var i = 0; i < this.length; i++)
      this[i].simplify();
  }

  opEquals(other) {
    const len = this.length;
    if (len !== other.length) return false;

    for (var i = 0; i < len; i++)
      if (!this[i].equals(other[i]))
        return false;

    return true;
  }

  mergeWith(other) {
    var ok = this.x86 === other.x86 && this.x64 === other.x64;
    if (!ok && this.x86 && this.x64 && !other.x86 && other.x64)
      ok = true;

    if (!ok)
      return false;

    //if (this.x86 === false && this.x64 === true && other.x86 === true && other.x64 === false) return false;
    //if (this.x86 === true && this.x64 === false && other.x86 === false && other.x64 === true) return false;

    if (this.implicit !== other.implicit)
      return false;

    const len = this.length;
    if (len !== other.length) return false;

    var xorIndex = -1;
    var canCompact = true;

    for (var i = 0; i < len; i++) {
      const xor = this[i].xor(other[i]);
      if (xor === null) continue;

      if (xorIndex === -1)
        xorIndex = i;
      else
        return false;
    }

    if (xorIndex !== -1)
      this[xorIndex].mergeWith(other[xorIndex]);

    this.x86 = this.x86 || other.x86;
    this.x64 = this.x64 || other.x64;

    return true;
  }

  toString(ops) {
    return "{" + this.join(", ") + "}";
  }
}

class SignatureArray extends Array {
  compact() {
    for (var i = 0; i < this.length; i++) {
      var row = this[i];
      var j = i + 1;
      while (j < this.length) {
        if (row.mergeWith(this[j])) {
          this.splice(j, 1);
          continue;
        }
        j++;
      }
    }
  }

  simplify() {
    for (var i = 0; i < this.length; i++)
      this[i].simplify();
  }

  toString(ops) {
    return "[" + this.join(", ") + "]";
  }
}

class X86Generator {
  constructor() {
    this.instMap = Object.create(null);
    this.instArray = [];

    this.opCombinations = Object.create(null);
    this.maxOpRows = 0;

    this.opBlackList = {
      "moff8" : true,
      "moff16": true,
      "moff32": true,
      "moff64": true
    };

    this.sizeStats = Object.create(null);
  }

  signaturesFromInsts(insts) {
    const signatures = new SignatureArray();

    for (var i = 0; i < insts.length; i++) {
      const inst = insts[i];
      const ops = inst.operands;

      var row = new ISignature();

      row.x86 = inst.arch === "ANY" || inst.arch === "X86";
      row.x64 = inst.arch === "ANY" || inst.arch === "X64";

      for (var j = 0; j < ops.length; j++) {
        var iop = ops[j];

        var reg = iop.reg;
        var mem = iop.mem;
        var imm = iop.imm;
        var rel = iop.rel;

        // Terminate if this operand is something asmjit doesn't support
        // and skip all instructions having implicit `imm` operand of `1`
        // (handled fine by asmjit).
        if (this.opBlackList[mem] === true || iop.immValue !== null) {
          row = null;
          break;
        }

        if (reg === "r8") reg = "r8lo";
        if (reg === "seg") reg = "sreg";
        if (reg === "st(i)") reg = "fp";
        if (reg === "st(0)") reg = "fp0";

        if (mem === "m32fp") mem = "m32";
        if (mem === "m64fp") mem = "m64";
        if (mem === "m80fp") mem = "m80";
        if (mem === "m80bcd") mem = "m80";
        if (mem === "m80dec") mem = "m80";
        if (mem === "m16int") mem = "m16";
        if (mem === "m32int") mem = "m32";
        if (mem === "m64int") mem = "m64";

        const op = new OSignature();

        if (iop.read) op.flags.read = true;
        if (iop.write) op.flags.write = true;

        if (iop.implicit) {
          row.implicit++;
          op.flags.implicit = true;
        }

        if (reg) op.flags[reg] = true;
        if (reg === "r8lo") op.flags.r8hi = true;

        if (mem) op.flags[mem] = true;
        if (imm) op.flags["i" + imm] = true;
        if (rel) op.flags["rel" + rel] = true;

        row.push(op);
      }

      if (row)
        signatures.push(row);
    }

    signatures.compact();
    signatures.simplify();
    signatures.compact();

    return signatures;
  }

  /*
  newInstFromInsts(insts) {
    function GetAccess(inst) {
      var operands = inst.operands;
      if (!operands.length) return "";

      var op = operands[0];
      if (op.read && op.write)
        return "RW";
      else if (op.read)
        return "RO";
      else
        return "WO";
    }

    var inst = insts[0];

    var id       = this.instArray.length;
    var name     = inst.name;
    var enum_    = kX86InstPrefix + name[0].toUpperCase() + name.substr(1);

    var opcode   = inst.opcode;
    var rm       = inst.rm;
    var mm       = inst.mm;
    var pp       = inst.pp;
    var encoding = inst.encoding;
    var prefix   = inst.prefix;

    var access   = GetAccess(inst);

    var eflags = ["_", "_", "_", "_", "_", "_", "_", "_"];

    for (var flag in inst.eflags) {
      var fOp = inst.eflags[flag];
      var fIndex = flag === "OF" ? 0 :
                   flag === "SF" ? 1 :
                   flag === "ZF" ? 2 :
                   flag === "AF" ? 3 :
                   flag === "PF" ? 4 :
                   flag === "CF" ? 5 :
                   flag === "DF" ? 6 : 7;
      var fChar = fOp === "W" ? "W" :
                  fOp === "R" ? "R" :
                  fOp === "X" ? "X" :
                  fOp === "0" ? "W" :
                  fOp === "1" ? "W" :
                  fOp === "U" ? "U" : "#";
      eflags[fIndex] = fChar;
    }

    var vexL     = undefined;
    var vexW     = undefined;
    var evexW    = undefined;

    for (var i = 1; i < insts.length; i++) {
      inst = insts[i];

      if (opcode   !== inst.opcode    ) return null;
      if (rm       !== inst.rm        ) return null;
      if (mm       !== inst.mm        ) return null;
      if (pp       !== inst.pp        ) return null;
      if (encoding !== inst.encoding  ) return null;
      if (prefix   !== inst.prefix    ) return null;
      if (access   !== GetAccess(inst)) return null;
    }

    var obj = AVX512Flags(insts);
    if (obj) {
      vexL = obj.vexL;
      vexW = obj.vexW;
      evexW = obj.evexW;
    }

    var ppmm = Utils.padLeft(pp, 2).replace(/ /g, "0") +
               Utils.padLeft(mm, 4).replace(/ /g, "0") ;

    var composed = composeOpCode({
      type  : prefix === "VEX" || prefix === "EVEX" ? "V" : "O",
      prefix: ppmm,
      opcode: opcode,
      o     : rm === "r" ? "_" : (rm ? rm : "_"),
      l     : vexL,
      w     : vexW,
      ew    : evexW
    });

    var iflags = [];

    if (access)
      iflags.push("F(" + access + ")");

    if (insts[0].prefix === "VEX")
      iflags.push("F(Vex)");

    return {
      id            : id,
      name          : name,
      enum          : enum_,
      encoding      : "Enc(?" + encoding + "?)",
      opcode0       : composed,
      opcode1       : "U",
      iflags        : iflags.join(""),
      eflags        : eflags.join(""),
      writeIndex    : "0",
      writeSize     : "0",
      simdDstSize   : "0",
      simdSrcSize   : "0",
      signatures    : this.signaturesFromInsts(insts),
      nameIndex     : -1,
      extendedData  : "",
      extendedIndex : "",
      signatureIndex: -1,
      signatureCount: -1
    };
  }
  */

  parse(data) {
    // Create database.
    const re = new RegExp(
      "INST\\(([A-Za-z0-9_]+)\\s*," +       // [01] Id.
      "\\s*\\\"([A-Za-z0-9_ ]*)\\\"\\s*," + // [02] Name.
      "([^,]+)," +                          // [03] Encoding.
      "(.{26}[^,]*)," +                     // [04] Opcode[0].
      "(.{26}[^,]*)," +                     // [05] Opcode[1].
      "(.{38}[^,]*)," +                     // [06] IFLAGS.
      "\\s*EF\\(([A-Z_]+)\\)\\s*," +        // [07] EFLAGS.
      "([^,]+)," +                          // [08] Write-Index.
      "([^,]+)," +                          // [09] Write-Size.
      "([^,]+)," +                          // [10] SIMD Dst-Size.
      "([^,]+)," +                          // [11] SIMD Src-Size.
      "([^,]+)," +                          // [12] SignatureTableStart (auto-generated).
      "([^,]+)," +                          // [13] SignatureTableCount (auto-generated).
      "([^\\)]+)\\)",                       // [14] ExtendedIndex (auto-generated).
      "g");

    var id = 0;
    var m;

    function processOpcode(o) {
      return o;
    }

    while (m = re.exec(data)) {
      // Extract instruction ID and Name.
      var enum_ = kX86InstPrefix + m[1];
      var name = m[2];

      // Extract data that goes to the secondary table (X86InstExtendedInfo).
      var encoding    = m[3].trim();
      var opcode0     = m[4].trim();
      var opcode1     = m[5].trim();
      var iflags      = m[6].trim();
      var eflags      = m[7];
      var writeIndex  = Utils.trimLeft(m[8]);
      var writeSize   = Utils.trimLeft(m[9]);
      var simdDstSize = Utils.trimLeft(m[10]);
      var simdSrcSize = Utils.trimLeft(m[11]);
/*
      opcode0 = processOpcode(opcode0);
      opcode1 = processOpcode(opcode1);

      if (iflags.indexOf("A512(") !== -1) {
        var x = iflags.indexOf("A512(") + 12;
        opcode0 = opcode0.replace(",_  ", "," + Utils.padLeft(iflags.substr(x, 3), 3));
        opcode1 = opcode1.replace(",_  ", "," + Utils.padLeft(iflags.substr(x, 3), 3));
        iflags = iflags.substr(0, x) + iflags.substr(x + 4);
      }
*/
      const insts = x86db.map[name];
      if (!insts) {
        console.log(`INSTRUCTION '${name}' not found in asmdb`);
      }
      else {
        /*
        var avx = AVX512Flags(insts);
        if (avx) {
          var avxstr = "A512(" + Utils.padLeft(avx.feature  , 4) + "," +
                                 Utils.padLeft(avx.vl       , 1) + "," +
                                 Utils.padLeft(avx.tupleType, 3) + "," +
                                 Utils.padLeft(avx.masking  , 2) + "," +
                                 Utils.padLeft(avx.er_sae   , 3) + "," +
                                 Utils.padLeft(avx.broadcast, 1) + ")";
          console.log(`INSTRUCTION '${Utils.padLeft(name, 15)}' ${avxstr}`);
          iflags = Utils.padLeft(iflags, 12) + "|" + avxstr;
        }
        */
      }
      const signatures = insts ? this.signaturesFromInsts(insts) : new SignatureArray();

      const inst = {
        id            : id,           // Instruction id.
        name          : name,         // Instruction name.
        enum          : enum_,        // Instruction enum-string (kX86InstId...).
        encoding      : encoding,     // Instruction encoding.
        opcode0       : opcode0,      // Primary opcode.
        opcode1       : opcode1,      // Secondary opcode.
        iflags        : iflags,
        eflags        : eflags,
        writeIndex    : writeIndex,
        writeSize     : writeSize,
        simdDstSize   : simdDstSize,
        simdSrcSize   : simdSrcSize,
        signatures    : signatures,   // Rows containing operands' combinations.
        nameIndex     : -1,           // Instruction name-index.
        extendedData  : "",
        extendedIndex : "",
        signatureIndex: -1,
        signatureCount: -1
      }


      this.instMap[name] = inst;
      this.instArray.push(inst);
      this.maxOpRows = Math.max(this.maxOpRows, signatures.length);

      id++;
    }

    console.log("Number of Instructions: " + this.instArray.length);
  }

  generate(oldData) {
    var data = oldData;

    // Generate new tables.
    const nameData = this.generateNameData();
    const signaturesData = this.generateSignaturesData();
    const extendedData = this.generateExtendedData();
    const instData = this.generateInstData();

    // Inject auto-generated tables back to the input data.
    data = Utils.inject(data,
      "// ${X86InstNameData:Begin}\n",
      "// ${X86InstNameData:End}\n", nameData);

    data = Utils.inject(data,
      "// ${X86InstSignatureData:Begin}\n",
      "// ${X86InstSignatureData:End}\n", signaturesData);

    data = Utils.inject(data,
      "// ${X86InstExtendedData:Begin}\n",
      "// ${X86InstExtendedData:End}\n", extendedData);

    data = Utils.inject(data,
      "// ${X86InstData:Begin}\n",
      "// ${X86InstData:End}\n", instData);

    return data;
  }

  generateNameData() {
    const instArray = this.instArray;

    const instNames = new IndexedString();
    const instAlpha = new Array(26);

    for (var i = 0; i < instArray.length; i++) {
      const inst = instArray[i];
      instNames.add(inst.name);
    }
    instNames.index();

    for (var i = 0; i < instArray.length; i++) {
      const inst = instArray[i];
      const name = inst.name;
      const nameIndex = instNames.getIndex(name);

      const aIndex = name.charCodeAt(0) - 'a'.charCodeAt(0);
      if (aIndex < 0 || aIndex >= 26)
        throw new Error("Alphabetical index error");

      inst.nameIndex = nameIndex;
      if (instAlpha[aIndex] === undefined)
        instAlpha[aIndex] = inst.enum;
    }

    function formatInstNameIndex(indent, justify) {
      var i;
      var s = "";
      var line = "";

      if (!justify) justify = 0;

      for (i = 0; i < instArray; i++) {
        const inst = instArray[i];
        if (inst === undefined)
          throw new Error(`Database - no instruction #${i}`);
      }

      for (i = 0; i < instArray.length; i++) {
        const inst = instArray[i];
        if (inst === undefined)
          throw new Error(`Database - no instruction #${i}`);

        const item = String(inst.nameIndex) + ((i !== instArray.length - 1) ? "," : "");
        const newl = line + (line ? " " : indent) + item;

        if (newl.length <= justify) {
          line = newl;
          continue;
        }
        else {
          s += line + "\n";
          line = indent + item;
        }
      }

      s += line + "\n";
      return s;
    }

    var s = "";

    s += kDisclaimerStart;
    s += "static const char _x86InstNameData[] =\n" + instNames.format(kIndent, kJustify) + "\n";
    s += "static const uint16_t _x86InstNameIndex[] = {\n" + formatInstNameIndex(kIndent, kJustify) + "};\n\n";

    s += `enum X86InstAlphaIndex {\n`;
    s += `  kX86InstAlphaIndexFirst   = 'a',\n`;
    s += `  kX86InstAlphaIndexLast    = 'z',\n`;
    s += `  kX86InstAlphaIndexInvalid = 0xFFFF\n`;
    s += `};\n`;
    s += `\n`;

    s += `static const uint16_t _x86InstAlphaIndex[26] = {\n`;
    for (var i = 0; i < instAlpha.length; i++) {
      const id = instAlpha[i];
      s += kIndent + (id === undefined ? "0xFFFF" : id);
      if (i !== instAlpha.length - 1)
        s += `,`;
      s += `\n`;
    }
    s += `};\n`;
    s += kDisclaimerEnd;

    this.sizeStats.InstructionNames = instNames.getSize() + instArray.length * 2 + instAlpha.length * 2;
    return s;
  }

  generateSignaturesData() {
    const instArray = this.instArray;

    const opMap = Object.create(null);
    const opArr = [];

    const signatureMap = Object.create(null);
    const signatureArr = [];

    const noOperand = "OSIGNATURE(0, 0, 0, 0xFF)";
    opMap[noOperand] = [0];
    opArr.push(noOperand);

    function makeCxxArray(array, code) {
      return `${code} = {\n` +
               kIndent + array.join(`,\n${kIndent}`) + "\n" +
             `};\n`;
    }

    function makeCxxArrayWithComment(array, code) {
      var s = "";
      for (var i = 0; i < array.length; i++) {
        const last = i === array.length - 1;
        s += kIndent + array[i].data + (last ? "  // " : ", // ") + Utils.padLeft(array[i].refs ? "#" + String(i) : "", 5) + array[i].comment + "\n";
      }
      return `${code} = {\n${s}};\n`;
    }

    function findSignaturesIndex(rows) {
      const len = rows.length;
      if (!len) return 0;

      const indexes = signatureMap[rows[0].data];
      if (indexes === undefined) return -1;

      for (var i = 0; i < indexes.length; i++) {
        const index = indexes[i];
        if (index + len > signatureArr.length) continue;

        var ok = true;
        for (var j = 0; j < len; j++) {
          if (signatureArr[index + j].data !== rows[j].data) {
            ok = false;
            break;
          }
        }

        if (ok)
          return index;
      }

      return -1;
    }

    function indexSignatures(signatures) {
      const result = signatureArr.length;

      for (var i = 0; i < signatures.length; i++) {
        const signature = signatures[i];
        const idx = signatureArr.length;

        if (!hasOwn.call(signatureMap, signature.data))
          signatureMap[signature.data] = [];

        signatureMap[signature.data].push(idx);
        signatureArr.push(signature);
      }

      return result;
    }

    for (var len = this.maxOpRows; len >= 0; len--) {
      for (var i = 0; i < instArray.length; i++) {
        const inst = instArray[i];
        const signatures = inst.signatures;
        if (signatures.length !== len) continue;

        const signatureEntries = [];
        for (var j = 0; j < len; j++) {
          const signature = signatures[j];

          var signatureEntry = `ISIGNATURE(${signature.length}, ${signature.x86 ? 1 : 0}, ${signature.x64 ? 1 : 0}, ${signature.implicit}`;
          var signatureComment = signature.toString();

          var x = 0;
          while (x < signature.length) {
            const h = signature[x].toAsmJitOpData();
            var index = -1;
            if (!hasOwn.call(opMap, h)) {
              index = opArr.length;
              opMap[h] = index;
              opArr.push(h);
            }
            else {
              index = opMap[h];
            }

            signatureEntry += `, ${Utils.padLeft(index, 3)}`;
            x++;
          }

          while (x < 6) {
            signatureEntry += `, ${Utils.padLeft(0, 3)}`;
            x++;
          }

          signatureEntry += `)`;
          signatureEntries.push({ data: signatureEntry, comment: signatureComment, refs: 0 });
        }

        var count = signatureEntries.length;
        var index = findSignaturesIndex(signatureEntries);

        if (index === -1)
          index = indexSignatures(signatureEntries);

        signatureArr[index].refs++;
        inst.signatureIndex = index;
        inst.signatureCount = count;
      }
    }

    var s = "";

    s += kDisclaimerStart;
    s += "#define ISIGNATURE(count, x86, x64, implicit, o0, o1, o2, o3, o4, o5) \\\n";
    s += "  { count, (x86 ? uint8_t(X86Inst::kArchMaskX86) : uint8_t(0)) | \\\n";
    s += "           (x64 ? uint8_t(X86Inst::kArchMaskX64) : uint8_t(0)) , \\\n";
    s += "    implicit, \\\n";
    s += "    0, \\\n";
    s += "    o0, o1, o2, o3, o4, o5 \\\n";
    s += "  }\n";
    s += makeCxxArrayWithComment(signatureArr, "static const X86Inst::ISignature _x86InstISignatureData[]");
    s += "#undef ISIGNATURE\n";
    s += "\n";
    s += "#define FLAG(flag) X86Inst::kOp##flag\n";
    s += "#define MEM(mem) X86Inst::kMemOp##mem\n";
    s += "#define OSIGNATURE(flags, memFlags, extFlags, regId) \\\n";
    s += "  { uint32_t(flags), uint16_t(memFlags), uint8_t(extFlags), uint8_t(regId) }\n";
    s += makeCxxArray(opArr, "static const X86Inst::OSignature _x86InstOSignatureData[]");
    s += "#undef OSIGNATURE\n";
    s += "#undef MEM\n";
    s += "#undef FLAG\n";
    s += kDisclaimerEnd;

    this.sizeStats.SignatureData = opArr.length * 8 + signatureArr.length * 8;
    return s;
  }

  generateExtendedData() {
    const instArray = this.instArray;

    const extendedMap = Object.create(null);
    const extendedData = [];

    for (var i = 0; i < instArray.length; i++) {
      const inst = instArray[i];

      const encoding    = inst.encoding;
      const opcode1     = inst.opcode1;
      const iflags      = inst.iflags;
      const eflagsIn    = decToHex(getEFlagsMask(inst.eflags, "RX" ), 2);
      const eflagsOut   = decToHex(getEFlagsMask(inst.eflags, "WXU"), 2);
      const writeIndex  = inst.writeIndex;
      const writeSize   = inst.writeSize;
      const simdDstSize = inst.simdDstSize;
      const simdSrcSize = inst.simdSrcSize;

      var extData = Utils.padLeft(encoding   , 24) + ", " +
                    Utils.padLeft(writeIndex ,  3) + ", " +
                    Utils.padLeft(writeSize  ,  3) + ", " +
                    Utils.padLeft(simdDstSize,  3) + ", " +
                    Utils.padLeft(simdSrcSize,  3) + ", " +
                    eflagsIn                       + ", " +
                    eflagsOut                      + ", " +
                    "0"                            + ", " +
                    Utils.padLeft(iflags     , 38) + ", " +
                    Utils.padLeft(opcode1    , 26);
      var extIndex = -1;

      if (hasOwn.call(extendedMap, extData)) {
        extIndex = extendedMap[extData];
      }
      else {
        extIndex = extendedData.length;
        extendedData.push(extData);
        extendedMap[extData] = extIndex;
      }

      inst.extendedData = extData;
      inst.extendedIndex = extIndex;
    }

    var s = "";

    s += kDisclaimerStart;
    s += "const X86Inst::ExtendedData _x86InstExtendedData[] = {\n";
    for (i = 0; i < extendedData.length; i++) {
      s += `${kIndent}{ ${extendedData[i]} }`;
      s += (i === extendedData.length - 1) ? "\n" : ",\n";
    }
    s += "};\n";
    s += kDisclaimerEnd;

    this.sizeStats.ExtendedData = extendedData.length * 16;
    return s;
  }

  generateInstData() {
    const instArray = this.instArray;
    var s = "";

    for (var i = 0; i < instArray.length; i++) {
      const inst = instArray[i];

      s += "  INST(" +
        Utils.padLeft(inst.enum.substr(kX86InstPrefix.length), 16) + ", " +
        Utils.padLeft(`"${inst.name}"`   , 18) + ", " +
        Utils.padLeft(inst.encoding      , 23) + ", " +
        Utils.padLeft(inst.opcode0       , 26) + ", " +
        Utils.padLeft(inst.opcode1       , 26) + ", " +
        Utils.padLeft(inst.iflags        , 38) + ", " +
        "EF(" + inst.eflags + "), " +
        Utils.padLeft(inst.writeIndex    , 1) + ", " +
        Utils.padLeft(inst.writeSize     , 1) + ", " +
        Utils.padLeft(inst.simdDstSize   , 2) + ", " +
        Utils.padLeft(inst.simdSrcSize   , 2) + ", " +
        Utils.padLeft(inst.signatureIndex, 3) + ", " +
        Utils.padLeft(inst.signatureCount, 2) + ", " +
        Utils.padLeft(inst.extendedIndex , 3) + ")";
      s += (i === instArray.length - 1) ? "\n" : ",\n";
    }

    this.sizeStats.InstructionData = instArray.length * 8;
    return s;
  }

  printMissing() {
    const instMap = this.instMap;
    const instArray = this.instArray;

    var out = "";

    function CPUFlags(insts) {
      var flags = {};
      for (var i = 0; i < insts.length; i++) {
        var inst = insts[i];
        for (var k in inst.cpu)
          flags[k] = true;
      }
      return Object.getOwnPropertyNames(flags).join("|");
    }

    x86db.getInstructionNames().forEach(function(name) {
      var insts = x86db.getGroup(name);
      if (!instMap[name]) {
        console.log(`MISSING INSTRUCTION '${name}'`);
        /*
        var inst = this.newInstFromInsts(insts);
        if (inst) {
          // console.log(`MISSING INSTRUCTION '${name}'`);
          out += "  INST(" +
            Utils.padLeft(inst.enum.substr(kX86InstPrefix.length), 16) + ", " +
            Utils.padLeft(`"${inst.name}"`   , 18) + ", " +
            Utils.padLeft(inst.encoding      , 23) + ", " +
            Utils.padLeft(inst.opcode0       , 22) + ", " +
            Utils.padLeft(inst.opcode1       , 22) + ", " +
            Utils.padLeft(inst.iflags        , 35) + ", " +
            "EF(" + inst.eflags + "), " +
            Utils.padLeft(inst.writeIndex    , 2) + ", " +
            Utils.padLeft(inst.writeSize     , 2) + ", " +
            Utils.padLeft(inst.simdDstSize   , 2) + ", " +
            Utils.padLeft(inst.simdSrcSize   , 2) + ", " +
            Utils.padLeft(inst.signatureIndex, 3) + ", " +
            Utils.padLeft(inst.signatureCount, 2) + ", " +
            Utils.padLeft("0", 3) + "),\n";
        }
        */
      }
    }, this);
    console.log(out);
  }

  printStats() {
    const stats = this.sizeStats;

    var pad = 24;
    var total = 0;

    for (var k in stats) {
      const size = stats[k];
      total += size;
      console.log(Utils.padLeft('Size of ' + k, pad) + ": " + size);
    }

    console.log(Utils.padLeft('Size of all tables', pad) + ": " + total);
  }
}

// ----------------------------------------------------------------------------
// [Main]
// ----------------------------------------------------------------------------
/*
function genAPI() {
  var asm = fs.readFileSync("../src/asmjit/x86/x86assembler.h", "utf8");
  var list = ["AVX512F", "AVX512DQ", "AVX512BW", "AVX512CD", "AVX512ER", "AVX512PF", "AVX512IFMA", "AVX512VBMI"];

  function getAVX512Flag(inst) {
    for (var cpu in inst.cpu) {
      if (list.indexOf(cpu) !== -1) {
        return inst.cpu["AVX512VL"] ? cpu + "-VL" : cpu;
      }
    }
    return "";
  }

  var out = "";
  var signatures = Object.create(null);
  var signNames = [];

  for (var i = 0; i < list.length; i++) {
    var cpu = list[i];

    const EncodeReg = {
      "xmm" : "X86Xmm",
      "ymm" : "X86Ymm",
      "zmm" : "X86Zmm",
      "r32" : "X86Gp",
      "r64" : "X86Gp",
      "k"   : "X86KReg",
      "eax" : "EAX",
      "edx" : "EDX",
      "ecx" : "ECX",
      "zdi" : "ZDI",
      "xmm0": "XMM0"
    }

    x86db.forEach(function(name, inst) {
      if (inst.cpu.AVX || inst.cpu.AVX2 || getAVX512Flag(inst)) {
        var operands = inst.operands;
        var iops = [];
        var hasImm = -1;
        var hasMemReg = -1;
        var memReg = null;

        var enum_ = name[0].toUpperCase() + name.substr(1);
        iops.push(name, enum_);

        var flags = "";
        var avx512Flags = [];

        if (inst.zmask)
          avx512Flags.push("kz");
        else if (inst.kmask)
          avx512Flags.push("k");

        if (inst.rnd)
          avx512Flags.push("er");
        else if (inst.sae)
          avx512Flags.push("sae");

        if (inst.broadcast)
          avx512Flags.push(`b${inst.elementSize}`);

        var flags = getAVX512Flag(inst);
        if (flags) {
          if (flags.indexOf("-VL") !== -1)
            flags = flags.substr(0, flags.length - 3) + (avx512Flags.length ? "{" + avx512Flags.join("|") + "}" : "") + "-VL";
          else
            flags = flags + (avx512Flags.length ? "{" + avx512Flags.join("|") + "}" : "");
        }
        else if (inst.cpu.AVX2) {
          flags = "AVX2";
        }
        else if (inst.cpu.AVX) {
          flags = "AVX";
        }
        else {
          flags = "";
        }
        console.log(`${inst.name}: ${flags}`);

        for (var j = 0; j < operands.length; j++) {
          var operand = operands[j];

          if (operand.reg && operand.mem) {
            if (!EncodeReg[operand.reg])
              console.log(`UNHANDLED REG ${operand.reg}`);

            hasMemReg = iops.length;
            memReg = [EncodeReg[operand.reg], "X86Mem"];
            iops.push("?");
          }
          else if (operand.reg) {
            if (!EncodeReg[operand.reg])
              console.log(`UNHANDLED REG ${operand.reg}`);
            iops.push(EncodeReg[operand.reg]);
          }
          else if (operand.mem) {
            iops.push("X86Mem");
          }
          else if (operand.imm) {
            hasImm = iops.length;
            iops.push("Imm");
          }
          else {
            console.log(`UNHANDLED OPERAND (instruction ${inst.name}`);
          }
        }

        var prefix = "INST_" + (operands.length) + ((hasImm === -1) ? "x" : "i");
        var str = `${prefix}(${iops.join(", ")})`;
        var insts = [];

        if (hasMemReg !== -1) {
          insts.push(str.replace("?", memReg[0]));
          insts.push(str.replace("?", memReg[1]));
        }
        else {
          insts.push(str);
        }

        for (var j = 0; j < insts.length; j++) {
          str = insts[j];
          if (!signatures[str]) {
            signNames.push(str);
            signatures[str] = {};
          }
          signatures[str][flags] = true;
          console.log(`ADDED ${str} <- ${flags}`);
        }
      }
    });
  }

  for (var i = 0; i < signNames.length; i++) {
    var signature = signNames[i];
    var flags = Object.getOwnPropertyNames(signatures[signature]);
    flags.sort();

    var line = signature;
    if (flags.length) {
      var fstr = "";

      if (flags[0] === "AVX") {
        fstr = "AVX1"; flags.splice(0, 1);
      }
      else if (flags[0] === "AVX2") {
        fstr = "AVX2"; flags.splice(0, 1);
      }

      if (flags.length) {
        if (!fstr) fstr = "   ";
        fstr = Utils.padLeft(fstr, 5);
        fstr += flags.join(" ");
      }

      line = Utils.padLeft(line, 72);
      line += "// " + fstr;
    }

    out += line + "\n";
  }


  console.log(out);
}

function genOpcodeH() {
  var asm = fs.readFileSync("../src/asmjit/x86/x86assembler.h", "utf8");
  var list = ["AVX512F", "AVX512DQ", "AVX512BW", "AVX512CD", "AVX512ER", "AVX512PF", "AVX512IFMA", "AVX512VBMI"];

  function getAVX512Flag(inst) {
    for (var cpu in inst.cpu) {
      if (list.indexOf(cpu) !== -1) {
        return inst.cpu["AVX512VL"] ? cpu + "-VL" : cpu;
      }
    }
    return "";
  }

  var out = "";
  var signatures = Object.create(null);
  var signNames = [];

  const Encode = {
    ""      : "",
    "xmm0"  : "xmmA",
    "xmm1"  : "xmmB",
    "xmm2"  : "xmmC",
    "xmm3"  : "xmmD",
    "ymm0"  : "ymmA",
    "ymm1"  : "ymmB",
    "ymm2"  : "ymmC",
    "ymm3"  : "ymmD",
    "zmm0"  : "zmmA",
    "zmm1"  : "zmmB",
    "zmm2"  : "zmmC",
    "zmm3"  : "zmmD",
    "r320"  : "gpdA",
    "r321"  : "gpdB",
    "r322"  : "gpdC",
    "r323"  : "gpdD",
    "r640"  : "gpzA",
    "r641"  : "gpzB",
    "r642"  : "gpzC",
    "r643"  : "gpzD",
    "k0"    : "kA",
    "k1"    : "kB",
    "k2"    : "kC",
    "k3"    : "kD",
    "m0"    : "anyptr_gpA",
    "m1"    : "anyptr_gpB",
    "m2"    : "anyptr_gpC",
    "m3"    : "anyptr_gpD",
    "vm32x0": "vx_ptr",
    "vm32x1": "vx_ptr",
    "vm32x2": "vx_ptr",
    "vm32x3": "vx_ptr",
    "vm32y0": "vy_ptr",
    "vm32y1": "vy_ptr",
    "vm32y2": "vy_ptr",
    "vm32y3": "vy_ptr",
    "vm32z0": "vz_ptr",
    "vm32z1": "vz_ptr",
    "vm32z2": "vz_ptr",
    "vm32z3": "vz_ptr"
  };

  x86db.forEach(function(name, inst) {
    if (getAVX512Flag(inst)) {
      var operands = inst.operands;
      var iops = [];
      var hasImm = -1;
      var hasMemReg = -1;
      var memReg = null;

      var flags = "";
      var avx512Flags = [];

      if (inst.zmask)
        avx512Flags.push("kz");
      else if (inst.kmask)
        avx512Flags.push("k");

      if (inst.rnd)
        avx512Flags.push("er");
      else if (inst.sae)
        avx512Flags.push("sae");

      if (inst.broadcast)
        avx512Flags.push(`b${inst.elementSize}`);

      var flags = getAVX512Flag(inst);
      if (flags) {
        if (flags.indexOf("-VL") !== -1)
          flags = flags.substr(0, flags.length - 3) + (avx512Flags.length ? "{" + avx512Flags.join("|") + "}" : "") + "-VL";
        else
          flags = flags + (avx512Flags.length ? "{" + avx512Flags.join("|") + "}" : "");
      }
      else if (inst.cpu.AVX2) {
        flags = "AVX2";
      }
      else if (inst.cpu.AVX) {
        flags = "AVX";
      }
      else {
        flags = "";
      }

      for (var j = 0; j < operands.length; j++) {
        var operand = operands[j];

        var reg = operand.reg ? operand.reg + j : "";
        var mem = operand.mem ? operand.mem + j : "";

        if (reg && !Encode[reg]) console.log(`UNHANDLED REG ${reg}`);
        reg = Encode[reg];

        if (mem) mem = Encode[mem] ? Encode[mem] : Encode["m" + j];

        if (reg && mem) {
          hasMemReg = iops.length;
          memReg = [reg, mem];
          iops.push("?");
        }
        else if (reg) {
          iops.push(reg);
        }
        else if (mem) {
          iops.push(mem);
        }
        else if (operand.imm) {
          hasImm = iops.length;
          iops.push("0");
        }
        else {
          console.log(`UNHANDLED OPERAND (instruction ${inst.name}`);
        }
      }

      var str = `  a.${name}(${iops.join(", ")});`;
      var insts = [];

      if (hasMemReg !== -1) {
        insts.push(str.replace("?", memReg[0]));
        insts.push(str.replace("?", memReg[1]));
      }
      else {
        insts.push(str);
      }

      for (var j = 0; j < insts.length; j++) {
        str = insts[j];
        out += str + "\n";
      }
    }
  });

  console.log(out);
}
*/
function main() {
  var g = new X86Generator();
  var data = fs.readFileSync(kFileName, "utf8").replace(/\r\n/g, "\n");

  g.parse(data);
  g.printMissing();
  var newData = g.generate(data);
  g.printStats();

  // Save only if modified.
  if (newData !== data) {
    fs.writeFileSync(kFileName + ".backup", data, "utf8");
    fs.writeFileSync(kFileName, newData, "utf8");
  }
}

main();
