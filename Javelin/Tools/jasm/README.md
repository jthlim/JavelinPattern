# Overview

_jasm_ is a jit-assembler. It enables writing inline JIT assembler with C++ source code.

It comprises two parts

* _jasm_ tool executable

  `source_file.cpp.jasm` --- _jasm_ ---> `source_file.cpp` --- _compiler_ ---> `source_file.o`

* Runtime libraries

# Motivation

Writing code that generates run time assembler can look something like:

```c++
program.add(Register.ECX, Register.EBX);
program.sub(Register.ECX, Register.EDX);

auto loopStart = program.position();
program.call(function);
program.dec(Register.ECX);
program.jnz(loopStart);

...

void (*jitFunction)() = program.Build();
```

This suffers from several drawbacks:

* The source code is quite verbose.

* There is likely to be run time overhead on every line -- checking that there
  is sufficient buffer space for any intermediate structures -- just to append
  a few bytes of machine code.

* The knowledge of how to encode the complete instruction set is included
  in the run time.

* The `jnz` instruction could be encoded in either 2 bytes or 5 bytes.
  This decision is either resolved at run time, or a pessmisitic 5 bytes is
  reserved.

_jasm_ addresses these by using a preprocessor pass.

```
»  add ecx, ebx
»  sub ecx, edx
» 1:
»  call {function}
»  dec ecx
»  jnz 1b
```

In this case, the entire code block is processed at build time and the `jnz` 
can be determined to fit within 2 bytes. Since the delta can also be 
determined at build time, so the entire run time process can simplify to 
roughly a memcpy and a patch of `function`'s address.

This results in:

* Code that is faster to assemble.

* Code that has a smaller footprint.

  Converting JavelinPattern to use jasm saved almost 100kb of runtime 
  executable size. 

* Code that is easier to maintain.

  _jasm_ code is shorter and looks like inline assembler.

The cost of this is having an extra step at build time.

# Features

* x64, arm64 and initial riscv support

* Source level debugging
 
  Warning, error messages and debugging for jasm and C++ code will refer to the original .jasm source file.

* Compact representation of JIT assembler code.

  When converting JavelinPattern from a hand written code generator to _jasm_, the executable size dropped by almost 100kb.

* Optimized engine
  * Speed optimized
  * Size optimized
    * The x64 runtime is ~11kb
    * The arm64 runtime is ~4kb. 

* Parameterizable values, labels *and* registers.

  Most operands can be replaced by expressions written inside `{}`.

  ```c++
  int registerIndex = ...;
  int n = ...;
  void (*functionPointer)() = ...;

  // x64
  » add reg32{registerIndex}, {n} 
  » call {functionPointer}

  // arm64
  » add reg32{registerIndex}, reg32{registerIndex}, #{n} 
  » bl {functionPointer}
  ```

* Conditional code generation

  ```
  » .if {a+b == 0}
  »   xor eax, eax
  » .else
  »   mov rax, {a+b} 
  » .endif 
  ```

* Compact code generation.
  
  _jasm_ will use the shortest code sequence available to represent an instruction.

  For example: On x64,

  ```c++
  int64_t imm0 = 0x12345678;
  int64_t imm1 = -0x12345678;
  int64_t imm2 = 0x123456789abcdef0;
  int32_t offset0 = 0;
  int32_t offset1 = 100;
  int32_t offset2 = 10000;

  » mov rax, {imm0} 
  » mov rax, {imm1} 
  » mov rax, {imm2} 
  » mov eax, [rbx + {offset0}]
  » mov eax, [rbx + {offset1}]
  » mov eax, [rbx + {offset2}]
  ```
  Generates:

  ```
  b8 78 56 34 12                 mov eax, 0x12345678
  48 c7 c0 88 a9 cb ed           mov rax, -0x12345678
  48 b8 f0 de bc 9a 78 56 34 12  mov rax, 0x123456789abcdef0
  8b 03                          mov eax, [rbx]
  8b 43 64                       mov eax, [rbx + 100]
  8b 83 10 27 00 00              mov eax, [rbx + 10000]
  ```
          
  On x64, jump offset sizes and values are determined and resolved at build
  time if possible, otherwise a single pass approach is used to determine 
  the offset size required.

  ```
  »   jz 1f  // Resolved at build time, since the delta is known.
  »   add eax, ebx
  » 1:
  »   jz 2f  // Determined at build time to require one byte offset.
  »          // The subsequent instruction could have different
  »          // lengths depending on the value of `imm`, but all less
  »          // than 128 bytes.
  »   add eax, {imm}
  » 2:
  ```

# Example

`example.cpp.jasm`:
```c++
#include "Javelin/Assembler/Assembler.h"
#include <stdio.h>

static int (*CreateTest(int returnValue))()
{
  Javelin::Assembler assembler(Javelin::SimpleJitMemoryManager::GetInstance());

#if defined(__amd64__)
  » .x64
  » mov eax, {returnValue}
  » ret
#elif defined(__arm64__)
  » .arm64
  » mov w0, #{returnValue}
  » ret
#else
  #error "Unsupported runtime"
#endif
  return (int (*)()) assembler.Build();
}

int main()
{
  int (*test)() = CreateTest(123);
  printf("Result: %d\n", test());
  Javelin::SimpleJitMemoryManager::StaticInstanceRelease((void*) test);

  return 0;
}
```

_jasm_ processes blocks of JIT assembler at a time and emits a function call
for each block.

With simplification, the output of jasm for the above is:

`example.cpp`:
```c++
#include "Javelin/Assembler/Assembler.h"
#include <stdio.h>

static int (*CreateTest(int returnValue))()
{
  Javelin::Assembler assembler(Javelin::SimpleJitMemoryManager::GetInstance());

#if defined(__amd64__)
  {  // begin jasm block
    struct JasmExpressionData { ... };
    static const uint8_t *jasmData = (const uint8_t*) "...";
    JasmExpressionData *data = assembler.AppendInstructionData(6, jasmData, 24);
    data->data0 = returnValue;
  }  // end jasm block
#elif defined(__arm64__)
  {  // begin jasm block
    struct JasmExpressionData { ... };
    static const uint8_t *jasmData = (const uint8_t*) "...";
    JasmExpressionData *data = assembler.AppendInstructionData(8, jasmData, 16);
    data->data0 = returnValue;
  }  // end jasm block
#else
  #error "Unsupported runtime"
#endif
  return (int (*)()) assembler.Build();
}

int main()
{
  int (*test)() = CreateTest(123);
  printf("Result: %d\n", test());
  Javelin::SimpleJitMemoryManager::StaticInstanceRelease((void*) test);

  return 0;
}
```

# Syntax

* Lines preceded by `»` are preprocessed and compiled by _jasm_

* Lines preceded by `«` are preprocessed by _jasm_

Example;

```c++
  » define maxmiumSize 1024

  » .macro myMacro
  «   if(size < maximumSize) {
  »     add eax, ebx
  «   }
  » .endm
```

## Labels

Labels come in several varieties:


| Label Type        | Declaration   | Reference                                          |
| ----------------- | ------------- | -------------------------------------------------- |
| Named local       | `labelName:`  | `labelName`                                        |
| Named global      | `*labelName:` | `*labelName`                                       |
| Numeric local     | `1:`          | `1b` (backwards), `1f` (forwards)                  |
| Numeric global    | `*1:`         | `*1b`, `*1f`, `*1bf` (backwards, or else forwards) |
| Expression global | `{`_expr_`}:` | `{`_expr_`}b`, `{`_expr_`}f`,  `{`_expr_`}bf`      |

Local labels need to resolve within the block being processed.

Label types are namespaced, and can only be used to reference the type it was declared with. Specifically, expressions _cannot_ reference numeric local or numeric global labels.

Examples

```
» 1:
»   call func
»   dec ecx
»   jnz 1b
»
»   ...
»
» func:
»   call *func2
»   call *2f
*   call {functionIndex}f
»   ret
```

```
» *func2:
»   ...
» *2:
»   ...
» {functionIndex}:
»   ...
```

## Comments

Comments are written using `//`

    » add rsi, 1  // Increment pointer.

## Commands

Commands begin with a period (`.`)

### Architectures

These commands define the instruction set being processed.

* `.x64`
* `.arm64`

### Segments

* `.code`
* `.data`

### Run-time conditionals

```
» .if <cond>
»   ...
» .elif <cond>
»   ...
» .else
»   ...
» .endif
```

Expressions can be used as conditions on all architectures, e.g.:

```c++
» .if {cc_expr == 1234}
```

There are extra `<cond>` available per architecture detailed below too.

### Preprocessor

* `.include` _filename_

* `.define` _keyword_ _expansion_

      » .define pEnd rdi
      » .define pData rbx + rsi
        
* `.macro`: Parameterized expansions

      » .macro addMacro PARAM1, PARAM2
      »   add PARAM1, PARAM2
      » .endm

      » .macro addDataToEaxMacro
      »   addMacro eax, [pData]
      » .endm

  Macros have their own local label scope.

      » .macro repeat FUNC, N
      »   mov ecx, N
      » 1:
      »   call FUNC
      »   dec ecx
      »   jnz 1b
      » .endm

### Alignment

* `.align` _n_
* `.unalign` _n_

  _n_ must be a power of two, and greater than the minimum instruction size.

  For example:

      » .align 16
      » opCode  # This will be 16 byte aligned

      » .unalign 16
      » opCode  # This will NOT be 16 byte aligned

* `.aligned` _n_

  Informs the assembler that it can assume the code is aligned to _n_ at this point.

### Local Variable Control

* `.assembler_variable_name` _varname_

  By default, jasm assumes that the local variable, of type `Javelin::Assembler` is named `assembler`. This can be controlled via jasm command line:

      > jasm -varname $localVarName ...

  This command allows specifying the assembler variable on a per-block basis.

## Architecture Specific

### x64

Format uses intel syntax.

    » mov eax, ebx
    » movzx eax, byte ptr [rsi]
    » call [rbx + rdx]

Most instructions are supported from:

* abm
* adx
* aes
* avx
* avx2
* bmi1
* bmi2
* mmx
* sse
* sse2
* sse3
* ssse3
* sse4.1 (many missing)
* sse4.2
* x87: Note that _jasm_ refers to stack registers as `st0`, `st1`, ...

#### Literals

* `db`: Single byte
* `dw`: Two bytes (word)
* `dd`: Four bytes (double word)
* `dq`: Eight bytes (quad word)

#### Parameterized operands

The following keywords are used for parameterized registers

* `reg8`
* `reg16`
* `reg32`
* `reg64`
* `st`
* `mm`
* `xmm`
* `ymm`
* `zmm`

Example:
```c++
int regIndex = ... // register index between 0 and 15
» mov reg64{regIndex}, 1

int fpIndex = ... // fp register index between 0 and 7
» fmul st{fpIndex}
```

#### Extra conditions for `.if` statements

* `delta32{` _expr_ `}`

### arm64

Most instructions supported from:

* armv8
* armv8.1
* armv8.2

Including A64 SIMD.

#### Literals

* `.byte`: Single byte
* `.hword`: Two bytes (half word)
* `.word`: Four bytes
* `.quad`: Eight Bytes

#### Parameterized operands

The following keywords are used for parameterized registers:

* General Purpose Registers
  * `reg32` == `regw` 
  * `reg64` == `regx`

* A64 SIMD Registers
  * `regb`
  * `regh`
  * `regs`
  * `regd`
  * `regq`
  * `regvb`
  * `regv4b`
  * `regv8b`
  * `regv16b`
  * `regvh`
  * `regvh`
  * `regv2h`
  * `regv4h`
  * `regv8h`
  * `regvs`
  * `regv2s`
  * `regv4s`
  * `regvd`
  * `regv1d`
  * `regv2d`
  * `regv1q`  

Example:

```
» mov regx{regIndex}, #1
```

#### Extra conditions for `.if` statements

* `delta21{` _expr_ `}` or  `adr{` _expr_ `}`
* `delta26x4{` _expr_ `}` 
* `adrp{` _expr_ `}`

### risc-v

Many instructions supported from:

* rv32i
* rv64i
* M multiply extension
* C compressed instruction extension
* F,D,Q floating point extensions

#### Literals

* `.byte`: Single byte
* `.hword`: Two bytes (half word)
* `.word`: Four bytes
* `.quad`: Eight Bytes

#### Parameterized operands

The following keywords are used for parameterized registers:

* General Purpose Registers
  * `regx` 
  * `regf`

