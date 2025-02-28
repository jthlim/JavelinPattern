//============================================================================

#include "Javelin/Pattern/Internal/Amd64PatternProcessorsFindMethods.h"
#if defined(JABI_AMD64_SYSTEM_V)

//============================================================================

#define NOP1  asm("nop")
#define NOP2  asm(".byte 0x66, 0x90")
#define NOP3  asm(".byte 0x0f, 0x1f, 0x00")
#define NOP4  asm(".byte 0x0f, 0x1f, 0x40, 0x00")
#define NOP5  asm(".byte 0x0f, 0x1f, 0x44, 0x00, 0x00")
#define NOP6  asm(".byte 0x66, 0x0f, 0x1f, 0x44, 0x00, 0x00")
#define NOP7  asm(".byte 0x0f, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00")
#define NOP8  asm(".byte 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00")
#define NOP9  asm(".byte 0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00")
#define NOP10 asm(".byte 0x66, 0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00")
#define NOP11 asm(".byte 0x66, 0x66, 0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00")
#define NOP12 asm(".byte 0x66, 0x66, 0x66, 0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00")
#define NOP13 asm(".byte 0x66, 0x66, 0x66, 0x66, 0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00")
#define NOP14 asm(".byte 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00")
#define NOP15 asm(".byte 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00")
#define NOP16 NOP15; NOP1
#define NOP17 NOP15; NOP2
#define NOP18 NOP15; NOP3
#define NOP19 NOP15; NOP4
#define NOP20 NOP15; NOP5
#define NOP21 NOP15; NOP6
#define NOP22 NOP15; NOP7
#define NOP23 NOP15; NOP8
#define NOP24 NOP15; NOP9
#define NOP25 NOP15; NOP10
#define NOP26 NOP15; NOP11
#define NOP27 NOP15; NOP12
#define NOP28 NOP15; NOP13
#define NOP29 NOP15; NOP14
#define NOP30 NOP15; NOP15
#define NOP31 NOP15; NOP15; NOP1

//============================================================================

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvxFindNibbleMask()
{
	// %xmm0 loaded with low nibble mask
	// %xmm1 loaded with high nibble mask
	
	asm("pushq %rcx");
	asm("movq %rsi, %rcx");
	asm("movl $0x0f0f0f0f, %eax");
	asm("andq $-32, %rsi");

	asm("vmovd %eax, %xmm7");
	
	asm("vpxor %xmm6, %xmm6, %xmm6");
	asm("vpshufd $0, %xmm7, %xmm7");

	asm("vmovdqa (%rsi), %xmm2");
	asm("vmovdqa 16(%rsi), %xmm3");
	asm("prefetchnta 512(%rsi)");
	
	asm("vpsrlq $4, %xmm2, %xmm4");
	asm("vpsrlq $4, %xmm3, %xmm5");
	asm("vpand %xmm2, %xmm7, %xmm2");		// 2,3 hold low nibbles
	asm("vpand %xmm3, %xmm7, %xmm3");
	asm("vpand %xmm4, %xmm7, %xmm4");		// 4,5 hold high nibbles
	asm("vpand %xmm5, %xmm7, %xmm5");

	asm("vpshufb %xmm2, %xmm0, %xmm2");
	asm("vpshufb %xmm3, %xmm0, %xmm3");
	asm("vpshufb %xmm4, %xmm1, %xmm4");
	asm("vpshufb %xmm5, %xmm1, %xmm5");
	
	asm("vpand %xmm2, %xmm4, %xmm2");		// If there's a match, then xmm2 or xmm3 hold non-zero
	asm("vpand %xmm3, %xmm5, %xmm3");
	asm("vpcmpeqb %xmm2, %xmm6, %xmm2");	// Convert them to masks
	asm("vpcmpeqb %xmm3, %xmm6, %xmm3");
	asm("vpmovmskb %xmm2, %eax");
	asm("vpmovmskb %xmm3, %r10d");
	
	asm("shll $16, %r10d");
	asm("orl %r10d, %eax");
	asm("not %eax");
	asm("shrl %cl, %eax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalAvxFindByteMaskDoMainLoop");
	asm("addq %rcx, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvxFindByteMaskFail");
	asm("retq");

	NOP5;
	
	asm("LInternalAvxFindByteMaskDoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $32, %rsi");					// Advance to next block
											// Want: rdx = end, rdx+rsi = current
											// -> rsi = current - end
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalAvxFindByteMaskFail");
	
	asm(".balign 32");
	asm("LInternalAvxFindByteMaskMainLoop:");
	asm("vmovdqa 32(%r10), %xmm2");
	asm("vmovdqa 48(%r10), %xmm3");
	asm("prefetchnta 544(%r10)");
	asm("addq $32, %r10");

	asm("vpsrlq $4, %xmm2, %xmm4");
	asm("vpsrlq $4, %xmm3, %xmm5");
	asm("vpand %xmm2, %xmm7, %xmm2");		// 2,3 hold low nibbles
	asm("vpand %xmm3, %xmm7, %xmm3");
	asm("vpand %xmm4, %xmm7, %xmm4");		// 4,5 hold high nibbles
	asm("vpand %xmm5, %xmm7, %xmm5");
	
	asm("vpshufb %xmm2, %xmm0, %xmm2");
	asm("vpshufb %xmm3, %xmm0, %xmm3");
	asm("vpshufb %xmm4, %xmm1, %xmm4");
	asm("vpshufb %xmm5, %xmm1, %xmm5");
	
	asm("vpand %xmm2, %xmm4, %xmm2");		// If there's a match, then xmm2 or xmm3 hold non-zero
	asm("vpand %xmm3, %xmm5, %xmm3");
	asm("vpcmpeqb %xmm2, %xmm6, %xmm2");	// Convert them to masks. This is faster than vpor, vptest
	asm("vpcmpeqb %xmm3, %xmm6, %xmm3");	// on avx1 architectures.
	asm("vpand %xmm2, %xmm3, %xmm3");
	
	asm("vpmovmskb %xmm3, %eax");
	asm("cmp $0xffff, %eax");
	asm("jne LInternalAvxFindByteMaskFound");
	asm("addq $32, %rsi");
	asm("jnc LInternalAvxFindByteMaskMainLoop");
	
	asm("LInternalAvxFindByteMaskFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvxFindByteMaskFound:");
	asm("vpmovmskb %xmm2, %r10d");
	asm("shll $16, %eax");
	asm("orl %r10d, %eax");
	asm("not %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvxFindByteMaskFail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx2FindNibbleMask()
{
	// %xmm0 loaded with low nibble mask
	// %xmm1 loaded with high nibble mask
	
	asm("pushq %rcx");
	asm("movq %rsi, %rcx");
	asm("movl $0x0f, %eax");
	asm("andq $-64, %rsi");
	
	asm("vmovd %eax, %xmm7");
	asm("vpxor %ymm6, %ymm6, %ymm6");
	
	asm("vmovdqa (%rsi), %ymm2");
	asm("vmovdqa 32(%rsi), %ymm3");
	asm("prefetchnta 1536(%rsi)");
	asm("vpermq $0x44, %ymm0, %ymm0");
	asm("vpermq $0x44, %ymm1, %ymm1");
	asm("vpbroadcastb %xmm7, %ymm7");

	asm("vpsrlq $4, %ymm2, %ymm4");
	asm("vpsrlq $4, %ymm3, %ymm5");
	asm("vpand %ymm7, %ymm2, %ymm2");		// 2,3 hold low nibbles
	asm("vpand %ymm7, %ymm3, %ymm3");
	asm("vpand %ymm7, %ymm4, %ymm4");		// 4,5 hold high nibbles
	asm("vpand %ymm7, %ymm5, %ymm5");
	
	asm("vpshufb %ymm2, %ymm0, %ymm2");
	asm("vpshufb %ymm3, %ymm0, %ymm3");
	asm("vpshufb %ymm4, %ymm1, %ymm4");
	asm("vpshufb %ymm5, %ymm1, %ymm5");
	
	asm("vpand %ymm2, %ymm4, %ymm2");
	asm("vpand %ymm3, %ymm5, %ymm3");
	asm("vpcmpeqb %ymm2, %ymm6, %ymm2");	// Convert them to masks
	asm("vpcmpeqb %ymm3, %ymm6, %ymm3");
	asm("vpmovmskb %ymm2, %eax");
	asm("vpmovmskb %ymm3, %r10d");
	
	asm("shlq $32, %r10");
	asm("orq %r10, %rax");
	asm("notq %rax");
	asm("shrq %cl, %rax");
	asm("bsfq %rax, %rax");
	asm("jz LInternalAvx2FindByteMaskDoMainLoop");
	asm("addq %rcx, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvx2FindByteMaskFail");
	asm("retq");

	NOP22;
	
	asm("LInternalAvx2FindByteMaskDoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $64, %rsi");
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalAvx2FindByteMaskFail");
	
	asm(".balign 32");
	asm("LInternalAvx2FindByteMaskMainLoop:");
	asm("vmovdqa 64(%r10), %ymm2");
	asm("vmovdqa 96(%r10), %ymm3");
	asm("prefetchnta 1536(%r10)");
	asm("addq $64, %r10");
	
	asm("vpsrlq $4, %ymm2, %ymm4");
	asm("vpsrlq $4, %ymm3, %ymm5");
	asm("vpand %ymm7, %ymm2, %ymm2");		// 2,3 hold low nibbles
	asm("vpand %ymm7, %ymm3, %ymm3");
	asm("vpand %ymm7, %ymm4, %ymm4");		// 4,5 hold high nibbles
	asm("vpand %ymm7, %ymm5, %ymm5");
	
	asm("vpshufb %ymm2, %ymm0, %ymm2");
	asm("vpshufb %ymm3, %ymm0, %ymm3");
	asm("vpshufb %ymm4, %ymm1, %ymm4");
	asm("vpshufb %ymm5, %ymm1, %ymm5");
	
	asm("vpand %ymm2, %ymm4, %ymm2");
	asm("vpand %ymm3, %ymm5, %ymm3");
    
    asm("vpor %ymm2, %ymm3, %ymm4");
	asm("vptest %ymm4, %ymm4");
	asm("jnz LInternalAvx2FindByteMaskFound");
	asm("addq $64, %rsi");
	asm("jnc LInternalAvx2FindByteMaskMainLoop");
	
	asm("LInternalAvx2FindByteMaskFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvx2FindByteMaskFound:");
	asm("vpcmpeqb %ymm3, %ymm6, %ymm3");	// Convert them to masks
    asm("vpcmpeqb %ymm2, %ymm6, %ymm2");
    asm("vpmovmskb %ymm3, %eax");
	asm("vpmovmskb %ymm2, %r10d");
	
	asm("shlq $32, %rax");
	asm("orq %r10, %rax");
	asm("notq %rax");
	asm("bsfq %rax, %rax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvx2FindByteMaskFail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx512FindNibbleMask()
{
    // %xmm0 loaded with low nibble mask
    // %xmm1 loaded with high nibble mask
    
    asm("pushq %rcx");
    asm("movq %rsi, %rcx");
    asm("movl $0x0f, %eax");
    asm("andq $-64, %rsi");
    
    asm("vmovdqa64 (%rsi), %zmm2");
    asm("prefetcht0 2048(%rsi)");
    asm("vshufi32x4 $0, %zmm0, %zmm0, %zmm0");
    asm("vshufi32x4 $0, %zmm1, %zmm1, %zmm1");
    asm("vpbroadcastb %eax, %zmm7");

    asm("vpsrlq $4, %zmm2, %zmm4");
    asm("vpandq %zmm7, %zmm2, %zmm2");        // 2 holds low nibbles
    asm("vpandq %zmm7, %zmm4, %zmm4");        // 4 holds high nibbles
    
    asm("vpshufb %zmm2, %zmm0, %zmm2");
    asm("vpshufb %zmm4, %zmm1, %zmm4");
    
    asm("vptestmb %zmm2, %zmm4, %k1");
    asm("kmovq %k1, %rax");
    
    asm("shrq %cl, %rax");
    asm("jz LInternalAvx512FindByteMaskDoMainLoop");
    
    asm("bsfq %rax, %rax");
    asm("addq %rcx, %rax");
    asm("popq %rcx");
    asm("cmpq %rdx, %rax");
    asm("jae LInternalAvx512FindByteMaskFail");
    asm("retq");
    
    NOP8;

    asm("LInternalAvx512FindByteMaskDoMainLoop:");
    asm("movq %rsi, %r10");
    asm("addq $64, %rsi");
    asm("subq %rdx, %rsi");
    asm("popq %rcx");
    asm("jnc LInternalAvx512FindByteMaskFail");
    
    asm(".balign 32");
    asm("LInternalAvx512FindByteMaskMainLoop:");
    asm("vmovdqa64 64(%r10), %zmm2");
    asm("prefetcht0 2048(%r10)");
    asm("addq $64, %r10");
    
    asm("vpsrlq $4, %zmm2, %zmm4");
    asm("vpandq %zmm7, %zmm2, %zmm2");        // 2 holds low nibbles
    asm("vpandq %zmm7, %zmm4, %zmm4");        // 4 holds high nibbles
    
    asm("vpshufb %zmm2, %zmm0, %zmm2");
    asm("vpshufb %zmm4, %zmm1, %zmm4");
    
    asm("vptestmb %zmm2, %zmm4, %k1");

    asm("kortestq %k1, %k1");
    asm("jnz LInternalAvx512FindByteMaskFound");
    asm("addq $64, %rsi");
    asm("jnc LInternalAvx512FindByteMaskMainLoop");
    
    asm("LInternalAvx512FindByteMaskFail:");
    asm("xor %eax, %eax");
    asm("retq");
    
    asm("LInternalAvx512FindByteMaskFound:");
    asm("kmovq %k1, %rax");
    asm("bsfq %rax, %rax");
    asm("addq %rsi, %rax");
    asm("jc LInternalAvx512FindByteMaskFail");
    asm("addq %rdx, %rax");
    asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvxFindPairNibbleMask()
{
	// %xmm0 loaded with low nibble mask
	// %xmm1 loaded with high nibble mask
	// %xmm2 loaded with low nibble mask of byte 2
	// %xmm3 loaded with high nibble mask of byte 2
	
	asm("pushq %rcx");
	asm("movl %esi, %ecx");
	asm("movl $0x0f0f0f0f, %eax");
	asm("andq $-32, %rsi");
	asm("andl $31, %ecx");
	asm("vmovd %eax, %xmm15");
	
	asm("vpxor %xmm0, %xmm0, %xmm14");
	asm("vpshufd $0, %xmm15, %xmm15");
	
	asm("vmovdqa (%rsi), %xmm4");
	asm("vmovdqa 16(%rsi), %xmm5");
	asm("prefetchnta 512(%rsi)");
	
	asm("vpsrlq $4, %xmm4, %xmm6");
	asm("vpsrlq $4, %xmm5, %xmm7");
	asm("vpand %xmm4, %xmm15, %xmm4");		// 4,5 hold low nibbles
	asm("vpand %xmm5, %xmm15, %xmm5");
	asm("vpand %xmm6, %xmm15, %xmm6");		// 6,7 hold high nibbles
	asm("vpand %xmm7, %xmm15, %xmm7");

	asm("vpshufb %xmm4, %xmm0, %xmm8");
	asm("vpshufb %xmm5, %xmm0, %xmm9");
	asm("vpshufb %xmm6, %xmm3, %xmm10");
	asm("vpshufb %xmm7, %xmm3, %xmm11");
	
	asm("vpshufb %xmm4, %xmm2, %xmm4");
	asm("vpshufb %xmm5, %xmm2, %xmm5");
	asm("vpshufb %xmm6, %xmm1, %xmm6");
	asm("vpshufb %xmm7, %xmm1, %xmm7");
	
	asm("vpand %xmm6, %xmm8, %xmm6");
	asm("vpand %xmm7, %xmm9, %xmm12");
	asm("vpand %xmm4, %xmm10, %xmm4");
	asm("vpand %xmm5, %xmm11, %xmm5");

	// slide the first byte window to overlap with the second
    asm("vpalignr $15, %xmm6, %xmm12, %xmm7");
	asm("vpslldq $1, %xmm6, %xmm6");
	
	asm("vpcmpeqb %xmm4, %xmm14, %xmm4");	// Convert them to masks
	asm("vpcmpeqb %xmm5, %xmm14, %xmm5");
	asm("vpcmpeqb %xmm6, %xmm14, %xmm6");
	asm("vpcmpeqb %xmm7, %xmm14, %xmm7");
	
	asm("vpor %xmm6, %xmm4, %xmm4");
	asm("vpor %xmm7, %xmm5, %xmm5");
	
	asm("vpmovmskb %xmm4, %eax");
	asm("vpmovmskb %xmm5, %r10d");
	
	asm("shll $16, %r10d");
	asm("orl %r10d, %eax");
	asm("addl $1, %ecx");
	asm("not %eax");
	asm("shrq %cl, %rax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalAvxFindBytePairMaskDoMainLoop");
	asm("addq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvxFindBytePairMaskFail");
	asm("retq");
	
	asm("LInternalAvxFindBytePairMaskDoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $32, %rsi");					// Advance to next block
											// Want: rdx = end, rdx+rsi = current
											// -> rsi = current - end
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalAvxFindBytePairMaskFail");
	
	asm(".balign 32");
	asm("LInternalAvxFindBytePairMaskMainLoop:");
	asm("vmovdqa 32(%r10), %xmm4");
	asm("vmovdqa 48(%r10), %xmm5");
	asm("prefetchnta 544(%r10)");
	asm("addq $32, %r10");
	
	asm("vpsrlq $4, %xmm4, %xmm6");
	asm("vpsrlq $4, %xmm5, %xmm7");
	asm("vpand %xmm4, %xmm15, %xmm4");		// 4,5 hold low nibbles
	asm("vpand %xmm6, %xmm15, %xmm6");		// 6,7 hold high nibbles

	asm("vpshufb %xmm4, %xmm0, %xmm8");
	asm("vpand %xmm5, %xmm15, %xmm5");
	asm("vpshufb %xmm6, %xmm1, %xmm10");
	asm("vpand %xmm7, %xmm15, %xmm7");
	
	asm("vpshufb %xmm5, %xmm0, %xmm9");
	asm("vpand %xmm8, %xmm10, %xmm8");
	asm("vpshufb %xmm7, %xmm1, %xmm11");
	asm("vpand %xmm9, %xmm11, %xmm9");
	
	asm("vpshufb %xmm4, %xmm2, %xmm4");
	asm("vpalignr $15, %xmm12, %xmm8, %xmm10");
	asm("vpshufb %xmm6, %xmm3, %xmm6");
	asm("vpand %xmm4, %xmm6, %xmm4");
	asm("vpshufb %xmm5, %xmm2, %xmm5");
	asm("vpalignr $15, %xmm8, %xmm9, %xmm11");
	asm("vpshufb %xmm7, %xmm3, %xmm7");
	asm("vpand %xmm5, %xmm7, %xmm5");
	
	asm("vpcmpeqb %xmm4, %xmm14, %xmm4");	// Convert them to masks
	asm("vpcmpeqb %xmm5, %xmm14, %xmm5");
	asm("vpcmpeqb %xmm10, %xmm14, %xmm10");
	asm("vpcmpeqb %xmm11, %xmm14, %xmm11");

	asm("vpor %xmm10, %xmm4, %xmm4");
	asm("vpor %xmm11, %xmm5, %xmm5");
	asm("vpand %xmm4, %xmm5, %xmm5");
	asm("vmovdqa %xmm9, %xmm12");
	asm("vpmovmskb %xmm5, %eax");
	
	asm("cmp $0xffff, %eax");
	asm("jne LInternalAvxFindBytePairMaskFound");
	asm("addq $32, %rsi");
	asm("jnc LInternalAvxFindBytePairMaskMainLoop");
	
	asm("LInternalAvxFindBytePairMaskFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvxFindBytePairMaskFound:");
	asm("vpmovmskb %xmm4, %r10d");
	asm("shll $16, %eax");
	asm("orl %r10d, %eax");
	asm("not %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvxFindBytePairMaskFail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx2FindPairNibbleMask()
{
	// %xmm0 loaded with low nibble mask
	// %xmm1 loaded with high nibble mask
	// %xmm2 loaded with low nibble mask of byte 2
	// %xmm3 loaded with high nibble mask of byte 2
	
	asm("movl %esi, %r10d");
	asm("movl $0x0f, %eax");
	asm("andq $-64, %rsi");
	asm("andl $63, %r10d");
	asm("vmovd %eax, %xmm15");
	
	asm("vpxor %ymm0, %ymm0, %ymm14");
	asm("vpermq $0x44, %ymm0, %ymm0");
	asm("vpermq $0x44, %ymm1, %ymm1");
	asm("vpermq $0x44, %ymm2, %ymm2");
	asm("vpermq $0x44, %ymm3, %ymm3");
	asm("vpbroadcastb %xmm15, %ymm15");
	
	asm("vmovq %r10, %xmm11");
	
	asm("vmovdqa (%rsi), %ymm4");
	asm("vmovdqa 32(%rsi), %ymm5");
	asm("vmovdqa LInternalAvx2FindBytePairMaskShiftMask1(%rip), %ymm12");
	asm("vmovdqa LInternalAvx2FindBytePairMaskShiftMask2(%rip), %ymm13");
	asm("prefetchnta 1536(%rsi)");
	asm("vpbroadcastb %xmm11, %ymm11");
	
	asm("vpsrlq $4, %ymm4, %ymm6");
	asm("vpsrlq $4, %ymm5, %ymm7");
	asm("vpcmpgtb %ymm11, %ymm12, %ymm12");
	asm("vpcmpgtb %ymm11, %ymm13, %ymm13");
	asm("vpand %ymm4, %ymm15, %ymm4");		// 4,5 hold low nibbles
	asm("vpand %ymm5, %ymm15, %ymm5");
	asm("vpand %ymm6, %ymm15, %ymm6");		// 6,7 hold high nibbles
	asm("vpand %ymm7, %ymm15, %ymm7");
	
	asm("vpshufb %ymm4, %ymm0, %ymm8");
	asm("vpshufb %ymm5, %ymm0, %ymm9");
	asm("vpshufb %ymm6, %ymm1, %ymm10");
	asm("vpshufb %ymm7, %ymm1, %ymm11");
	
	asm("vpshufb %ymm4, %ymm2, %ymm4");
	asm("vpshufb %ymm5, %ymm2, %ymm5");
	asm("vpshufb %ymm6, %ymm3, %ymm6");
	asm("vpshufb %ymm7, %ymm3, %ymm7");
	
	asm("vpand %ymm8, %ymm12, %ymm8");
	asm("vpand %ymm9, %ymm13, %ymm9");
	
	asm("vpand %ymm8, %ymm10, %ymm8");
	asm("vpand %ymm9, %ymm11, %ymm12");
	asm("vpand %ymm4, %ymm6, %ymm4");
	asm("vpand %ymm5, %ymm7, %ymm5");
	
	// slide the first byte window to overlap with the second
	asm("vperm2i128 $0x8, %ymm8, %ymm8, %ymm10");
	asm("vperm2i128 $0x3, %ymm8, %ymm12, %ymm11");
	asm("vpalignr $15, %ymm10, %ymm8, %ymm6");
	asm("vpalignr $15, %ymm11, %ymm12, %ymm7");
	
	asm("vpcmpeqb %ymm4, %ymm14, %ymm4");	// Convert them to masks
	asm("vpcmpeqb %ymm5, %ymm14, %ymm5");
	asm("vpcmpeqb %ymm6, %ymm14, %ymm6");
	asm("vpcmpeqb %ymm7, %ymm14, %ymm7");
	
	asm("vpor %ymm6, %ymm4, %ymm4");
	asm("vpor %ymm7, %ymm5, %ymm5");
	
	asm("vpmovmskb %ymm4, %eax");
	asm("vpmovmskb %ymm5, %r10d");
	
	asm("shlq $32, %r10");
	asm("orq %r10, %rax");
	asm("not %rax");
	asm("bsfq %rax, %rax");
	asm("jz LInternalAvx2FindBytePairMaskDoMainLoop");
	asm("addq %rsi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvx2FindBytePairMaskFail");
	asm("retq");
	
	asm(".balign 32");
	asm("LInternalAvx2FindBytePairMaskShiftMask1:");
	asm(".byte 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16");
	asm(".byte 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32");
	asm("LInternalAvx2FindBytePairMaskShiftMask2:");
	asm(".byte 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48");
	asm(".byte 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64");
	
	asm(".byte 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90");
	
	asm("LInternalAvx2FindBytePairMaskDoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $64, %rsi");
	asm("subq %rdx, %rsi");
	asm("jnc LInternalAvx2FindBytePairMaskFail");
	
	asm(".balign 32");
	asm("LInternalAvx2FindBytePairMaskMainLoop:");
	asm("vmovdqu 63(%r10), %ymm4");
	asm("vmovdqa 64(%r10), %ymm5");
	asm("vmovdqu 95(%r10), %ymm6");
	asm("vmovdqa 96(%r10), %ymm7");
	asm("prefetchnta 1536(%r10)");
	asm("addq $64, %r10");
	
	asm("vpsrlq $4, %ymm4, %ymm8");
	asm("vpsrlq $4, %ymm5, %ymm9");
	asm("vpsrlq $4, %ymm6, %ymm10");
	asm("vpsrlq $4, %ymm7, %ymm11");
	asm("vpand %ymm15, %ymm4, %ymm4");
	asm("vpand %ymm15, %ymm5, %ymm5");
	asm("vpand %ymm15, %ymm6, %ymm6");
	asm("vpshufb %ymm4, %ymm0, %ymm4");
	asm("vpand %ymm15, %ymm7, %ymm7");
	asm("vpshufb %ymm5, %ymm2, %ymm5");
	asm("vpand %ymm15, %ymm8, %ymm8");
	asm("vpshufb %ymm6, %ymm0, %ymm6");
	asm("vpand %ymm15, %ymm9, %ymm9");
	asm("vpshufb %ymm8, %ymm1, %ymm8");
	asm("vpand %ymm15, %ymm10, %ymm10");
	asm("vpshufb %ymm9, %ymm3, %ymm9");
	asm("vpand %ymm15, %ymm11, %ymm11");
	
	asm("vpshufb %ymm10, %ymm1, %ymm10");
	asm("vpand %ymm4, %ymm8, %ymm4");
	asm("vpshufb %ymm7, %ymm2, %ymm7");
	asm("vpand %ymm5, %ymm9, %ymm5");
	asm("vpshufb %ymm11, %ymm3, %ymm11");
	
	asm("vpand %ymm6, %ymm10, %ymm6");
	asm("vpand %ymm7, %ymm11, %ymm7");
	
	asm("vpcmpeqb %ymm4, %ymm14, %ymm4");
	asm("vpcmpeqb %ymm5, %ymm14, %ymm5");
	asm("vpcmpeqb %ymm6, %ymm14, %ymm6");
	asm("vpcmpeqb %ymm7, %ymm14, %ymm7");
	
	asm("vpor %ymm4, %ymm5, %ymm4");
	asm("vpor %ymm6, %ymm7, %ymm6");
	
	asm("vpand %ymm4, %ymm6, %ymm6");
	
	asm("vpmovmskb %ymm6, %eax");
	
	asm("cmp $0xffffffff, %eax");
	asm("jne LInternalAvx2FindBytePairMaskFound");
	asm("addq $64, %rsi");
	asm("jnc LInternalAvx2FindBytePairMaskMainLoop");
	
	asm("LInternalAvx2FindBytePairMaskFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvx2FindBytePairMaskFound:");
	asm("vpmovmskb %ymm4, %r10d");
	asm("shlq $32, %rax");
	asm("orq %r10, %rax");
	asm("not %rax");
	asm("bsfq %rax, %rax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvx2FindBytePairMaskFail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx512FindPairNibbleMask()
{
    // %xmm0 loaded with low nibble mask
    // %xmm1 loaded with high nibble mask
    // %xmm2 loaded with low nibble mask of byte 2
    // %xmm3 loaded with high nibble mask of byte 2
    
    asm("pushq %rcx");
    asm("movq %rsi, %rcx");
    asm("movl $0x0f, %eax");
    asm("andq $-64, %rsi");
    
    asm("vmovdqa64 (%rsi), %zmm4");
    asm("prefetcht0 2048(%rsi)");

    asm("vshufi32x4 $0, %zmm0, %zmm0, %zmm0");
    asm("vshufi32x4 $0, %zmm1, %zmm1, %zmm1");
    asm("vshufi32x4 $0, %zmm2, %zmm2, %zmm2");
    asm("vshufi32x4 $0, %zmm3, %zmm3, %zmm3");
    asm("vpbroadcastb %eax, %zmm15");

    asm("vpsrlq $4, %zmm4, %zmm5");

    asm("vpandq %zmm15, %zmm4, %zmm4");          // low nibbles
    asm("vpandq %zmm15, %zmm5, %zmm5");          // high nibbles

    asm("vpshufb %zmm4, %zmm0, %zmm6");
    asm("vpshufb %zmm5, %zmm1, %zmm7");
    asm("vpshufb %zmm4, %zmm2, %zmm4");
    asm("vpshufb %zmm5, %zmm3, %zmm5");

    asm("vptestmb %zmm4, %zmm5, %k1");
    asm("vptestmb %zmm6, %zmm7, %k2");
    
    asm("kshiftlq $1, %k2, %k2");

    asm("kandq %k1, %k2, %k1");
    asm("kmovq %k1, %rax");
    
    asm("shrq %cl, %rax");
    asm("andq $-2, %rax");
    asm("jz LInternalAvx512FindPairNibbleMaskDoMainLoop");

    asm("bsfq %rax, %rax");
    asm("addq %rcx, %rax");
    asm("popq %rcx");
    asm("cmpq %rdx, %rax");
    asm("jae LInternalAvx512FindPairNibbleMaskFail");
    asm("retq");
    
    NOP21;

    asm("LInternalAvx512FindPairNibbleMaskDoMainLoop:");
    asm("movq %rsi, %r10");
    asm("addq $64, %rsi");
    asm("subq %rdx, %rsi");
    asm("popq %rcx");
    asm("jnc LInternalAvx512FindPairNibbleMaskFail");
    
    asm(".balign 32");
    asm("LInternalAvx512FindPairNibbleMaskMainLoop:");
    asm("vmovdqa64 64(%r10), %zmm4");
    asm("vmovdqu64 63(%r10), %zmm8");
    asm("prefetcht0 2048(%r10)");
    asm("addq $64, %r10");

    asm("vpsrlq $4, %zmm4, %zmm6");
    asm("vpsrlq $4, %zmm8, %zmm10");

    asm("vpandq %zmm15, %zmm4, %zmm4");          // 4 holds low nibbles of byte 2
    asm("vpandq %zmm15, %zmm6, %zmm6");          // 6 holds high nibbles of byte 2
    asm("vpandq %zmm15, %zmm8, %zmm8");          // 8 holds low nibbles of byte 1
    asm("vpandq %zmm15, %zmm10, %zmm10");        // 10 holds high nibbles of byte 1

    asm("vpshufb %zmm8, %zmm0, %zmm8");
    asm("vpshufb %zmm10, %zmm1, %zmm10");
    asm("vpshufb %zmm4, %zmm2, %zmm4");
    asm("vpshufb %zmm6, %zmm3, %zmm6");

    asm("vptestmb %zmm4, %zmm6, %k1");
    asm("vptestmb %zmm8, %zmm10, %k2");

    asm("ktestq %k1, %k2");
    asm("jnz LInternalAvx512FindPairNibbleMaskFound");
    asm("addq $64, %rsi");
    asm("jnc LInternalAvx512FindPairNibbleMaskMainLoop");
    
    asm("LInternalAvx512FindPairNibbleMaskFail:");
    asm("xor %eax, %eax");
    asm("retq");
    
    asm("LInternalAvx512FindPairNibbleMaskFound:");
    asm("kandq %k1, %k2, %k1");
    asm("kmovq %k1, %rax");
    asm("bsfq %rax, %rax");
    asm("addq %rsi, %rax");
    asm("jc LInternalAvx512FindPairNibbleMaskFail");
    asm("addq %rdx, %rax");
    asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvxFindTripletNibbleMask()
{
	// %xmm0 loaded with low nibble mask
	// %xmm1 loaded with high nibble mask
	// %xmm2 loaded with low nibble mask of byte 2
	// %xmm3 loaded with high nibble mask of byte 2
	// %xmm4 loaded with low nibble mask of byte 3
	// %xmm5 loaded with high nibble mask of byte 3
	
	asm("movl %esi, %r10d");
	asm("movl $0x0f0f0f0f, %eax");
	asm("andq $-16, %rsi");
	asm("andl $15, %r10d");
	asm("vmovd %eax, %xmm15");
	
	asm("vpxor %xmm14, %xmm14, %xmm14");
	asm("vpshufd $0, %xmm15, %xmm15");
	asm("vmovq %r10, %xmm13");
	
	asm("vmovdqa (%rsi), %xmm10");
	asm("vmovdqa LInternalAvxFindByteTripletMaskShiftMask(%rip), %xmm12");
	asm("prefetchnta 128(%rsi)");
	asm("vpshufb %xmm14, %xmm13, %xmm13");
	
	asm("vpsrlq $4, %xmm10, %xmm11");
	asm("vpcmpgtb %xmm13, %xmm12, %xmm13");
	asm("vpand %xmm15, %xmm10, %xmm10");
	asm("vpand %xmm15, %xmm11, %xmm11");

	asm("vpshufb %xmm10, %xmm0, %xmm6");
	asm("vpshufb %xmm11, %xmm1, %xmm7");
	asm("vpand %xmm6, %xmm7, %xmm6");

	asm("vpshufb %xmm10, %xmm2, %xmm7");
	asm("vpshufb %xmm11, %xmm3, %xmm8");
	asm("vpand %xmm13, %xmm6, %xmm6");
	asm("vpand %xmm7, %xmm8, %xmm7");

	asm("vpshufb %xmm10, %xmm4, %xmm8");
	asm("vpshufb %xmm11, %xmm5, %xmm9");
	asm("vpand %xmm8, %xmm9, %xmm8");
	
	asm("vpslldq $2, %xmm6, %xmm10");
	asm("vpslldq $1, %xmm7, %xmm9");
	
	asm("vpcmpeqb %xmm8, %xmm14, %xmm8");	// Convert them to masks
	asm("vpcmpeqb %xmm9, %xmm14, %xmm9");
	asm("vpcmpeqb %xmm10, %xmm14, %xmm10");
	
	asm("vpor %xmm9, %xmm8, %xmm8");
	asm("vmovdqa %xmm6, %xmm12");
	asm("vpor %xmm10, %xmm8, %xmm8");
	asm("vmovdqa %xmm7, %xmm13");
	
	asm("vpmovmskb %xmm8, %eax");

	asm("xorl $0xffff, %eax");
	asm("jz LInternalAvxFindByteTripletMaskDoMainLoop");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvxFindByteTripletMaskFail");
	asm("retq");
	
	asm(".balign 32");
	asm("LInternalAvxFindByteTripletMaskShiftMask:");
	asm(".byte 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16");
	// Padding so that MainLoop ends up on a 32-byte boundary
	
	asm("LInternalAvxFindByteTripletMaskDoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $16, %rsi");
	asm("subq %rdx, %rsi");
	asm("jnc LInternalAvxFindByteTripletMaskFail");
	
	asm(".balign 32");
	asm("LInternalAvxFindByteTripletMaskMainLoop:");
	asm("vmovdqa 16(%r10), %xmm10");
	asm("prefetchnta 144(%r10)");
	asm("addq $16, %r10");
	
	asm("vpsrlq $4, %xmm10, %xmm11");
	asm("vpand %xmm15, %xmm10, %xmm10");
	asm("vpand %xmm15, %xmm11, %xmm11");
	
	asm("vpshufb %xmm10, %xmm0, %xmm6");
	asm("vpshufb %xmm11, %xmm1, %xmm7");
	asm("vpand %xmm6, %xmm7, %xmm6");
	
	asm("vpshufb %xmm10, %xmm2, %xmm7");
	asm("vpshufb %xmm11, %xmm3, %xmm8");
	asm("vpand %xmm7, %xmm8, %xmm7");
	
	asm("vpshufb %xmm10, %xmm4, %xmm8");
	asm("vpshufb %xmm11, %xmm5, %xmm9");
	asm("vpand %xmm8, %xmm9, %xmm8");
	
	asm("vpalignr $14, %xmm12, %xmm6, %xmm10");
	asm("vpalignr $15, %xmm13, %xmm7, %xmm9");
	
	asm("vpcmpeqb %xmm8, %xmm14, %xmm8");	// Convert them to masks
	asm("vpcmpeqb %xmm9, %xmm14, %xmm9");
	asm("vpcmpeqb %xmm10, %xmm14, %xmm10");
	
	asm("vpor %xmm9, %xmm8, %xmm8");
	asm("vmovdqa %xmm6, %xmm12");
	asm("vpor %xmm10, %xmm8, %xmm8");
	asm("vmovdqa %xmm7, %xmm13");
	
	asm("vpmovmskb %xmm8, %eax");
	
	asm("cmpl $0xffff, %eax");
	asm("jnz LInternalAvxFindByteTripletMaskFound");
	asm("addq $16, %rsi");
	asm("jnc LInternalAvxFindByteTripletMaskMainLoop");
	
	asm("LInternalAvxFindByteTripletMaskFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvxFindByteTripletMaskFound:");
	asm("xorl $0xffff, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvxFindByteTripletMaskFail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvxFindPairNibbleMaskPath()
{
	// %xmm0 loaded with low nibble mask
	// %xmm1 loaded with high nibble mask
	// %xmm2 loaded with low nibble mask of byte 2
	// %xmm3 loaded with high nibble mask of byte 2
	
	asm("pushq %rcx");
	asm("movl %esi, %ecx");
	asm("movl $0x0f0f0f0f, %eax");
	asm("andq $-32, %rsi");
	asm("andl $31, %ecx");
	asm("vmovd %eax, %xmm15");
	
	asm("vpxor %xmm14, %xmm14, %xmm14");
	asm("vpshufd $0, %xmm15, %xmm15");
	
	asm("vmovdqa (%rsi), %xmm4");
	asm("vmovdqa 16(%rsi), %xmm5");
	asm("prefetchnta 256(%rsi)");
	
	asm("vpsrlq $4, %xmm4, %xmm6");
	asm("vpsrlq $4, %xmm5, %xmm7");
	asm("vpand %xmm15, %xmm4, %xmm4");		// 4,5 hold low nibbles
	asm("vpand %xmm15, %xmm5, %xmm5");
	asm("vpand %xmm15, %xmm6, %xmm6");		// 6,7 hold high nibbles
	asm("vpand %xmm15, %xmm7, %xmm7");
	
	asm("vpshufb %xmm4, %xmm0, %xmm8");
	asm("vpshufb %xmm5, %xmm0, %xmm9");
	asm("vpshufb %xmm6, %xmm1, %xmm10");
	asm("vpshufb %xmm7, %xmm1, %xmm11");
	
	asm("vpshufb %xmm4, %xmm2, %xmm4");
	asm("vpshufb %xmm5, %xmm2, %xmm5");
	asm("vpshufb %xmm6, %xmm3, %xmm6");
	asm("vpshufb %xmm7, %xmm3, %xmm7");
	
	asm("vpand %xmm8, %xmm10, %xmm8");
	asm("vpand %xmm9, %xmm11, %xmm12");
	asm("vpand %xmm4, %xmm6, %xmm4");
	asm("vpand %xmm5, %xmm7, %xmm5");
	
	// slide the first byte window to overlap with the second
	asm("vpslldq $1, %xmm8, %xmm6");
	asm("vpalignr $15, %xmm8, %xmm12, %xmm7");
	
	asm("vpand %xmm6, %xmm4, %xmm4");
	asm("vpand %xmm7, %xmm5, %xmm5");

	asm("vpcmpeqb %xmm4, %xmm14, %xmm4");	// Convert them to masks
	asm("vpcmpeqb %xmm5, %xmm14, %xmm5");
	
	asm("vpmovmskb %xmm4, %eax");
	asm("vpmovmskb %xmm5, %r10d");
	
	asm("shll $16, %r10d");
	asm("orl %r10d, %eax");
	asm("addl $1, %ecx");
	asm("not %eax");
	asm("shrq %cl, %rax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalAvxFindBytePairMaskPathDoMainLoop");
	asm("addq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvxFindBytePairMaskPathFail");
	asm("retq");
	
	asm(".balign 32");
	asm(".byte 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90");
	
	asm("LInternalAvxFindBytePairMaskPathDoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $32, %rsi");
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalAvxFindBytePairMaskPathFail");
	
	asm(".balign 32");
	asm("LInternalAvxFindBytePairMaskPathMainLoop:");
	asm("vmovdqa 32(%r10), %xmm4");
	asm("vmovdqa 48(%r10), %xmm5");
	asm("prefetchnta 256(%r10)");
	asm("addq $32, %r10");
	
	asm("vpsrlq $4, %xmm4, %xmm6");
	asm("vpsrlq $4, %xmm5, %xmm7");
	asm("vpand %xmm15, %xmm4, %xmm4");		// 4,5 hold low nibbles
	asm("vpand %xmm15, %xmm6, %xmm6");		// 6,7 hold high nibbles
	
	asm("vpshufb %xmm4, %xmm0, %xmm8");
	asm("vpand %xmm15, %xmm5, %xmm5");
	asm("vpshufb %xmm6, %xmm1, %xmm10");
	asm("vpand %xmm15, %xmm7, %xmm7");
	
	asm("vpshufb %xmm5, %xmm0, %xmm9");
	asm("vpand %xmm8, %xmm10, %xmm8");
	asm("vpshufb %xmm7, %xmm1, %xmm11");
	asm("vpand %xmm9, %xmm11, %xmm9");
	
	asm("vpshufb %xmm4, %xmm2, %xmm4");
	asm("vpalignr $15, %xmm12, %xmm8, %xmm10");
	asm("vpshufb %xmm6, %xmm3, %xmm6");
	asm("vpand %xmm4, %xmm6, %xmm4");
	asm("vpshufb %xmm5, %xmm2, %xmm5");
	asm("vpalignr $15, %xmm8, %xmm9, %xmm11");
	asm("vpshufb %xmm7, %xmm3, %xmm7");
	asm("vpand %xmm5, %xmm7, %xmm5");
	
	asm("vpand %xmm10, %xmm4, %xmm4");
	asm("vpand %xmm11, %xmm5, %xmm5");
	
	asm("vpcmpeqb %xmm4, %xmm14, %xmm4");	// Convert them to masks
	asm("vpcmpeqb %xmm5, %xmm14, %xmm5");

	asm("vpand %xmm4, %xmm5, %xmm5");
	asm("vmovdqa %xmm9, %xmm12");
	asm("vpmovmskb %xmm5, %eax");
	
	asm("cmp $0xffff, %eax");
	asm("jne LInternalAvxFindBytePairMaskPathFound");
	asm("addq $32, %rsi");
	asm("jnc LInternalAvxFindBytePairMaskPathMainLoop");
	
	asm("LInternalAvxFindBytePairMaskPathFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvxFindBytePairMaskPathFound:");
	asm("vpmovmskb %xmm4, %r10d");
	asm("shll $16, %eax");
	asm("orl %r10d, %eax");
	asm("not %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvxFindBytePairMaskPathFail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx2FindPairNibbleMaskPath()
{
	// %xmm0 loaded with low nibble mask
	// %xmm1 loaded with high nibble mask
	// %xmm2 loaded with low nibble mask of byte 2
	// %xmm3 loaded with high nibble mask of byte 2
	
	asm("movl %esi, %r10d");
	asm("movl $0x0f, %eax");
	asm("andq $-64, %rsi");
	asm("andl $63, %r10d");
	asm("vmovd %eax, %xmm15");
	
	asm("vpxor %ymm14, %ymm14, %ymm14");
	asm("vpermq $0x44, %ymm0, %ymm0");
	asm("vpermq $0x44, %ymm1, %ymm1");
	asm("vpermq $0x44, %ymm2, %ymm2");
	asm("vpermq $0x44, %ymm3, %ymm3");
	asm("vpbroadcastb %xmm15, %ymm15");
	
	asm("vmovq %r10, %xmm11");

	asm("vmovdqa (%rsi), %ymm4");
	asm("vmovdqa 32(%rsi), %ymm5");
	asm("vmovdqa LInternalAvx2FindBytePairMaskPathShiftMask1(%rip), %ymm12");
	asm("vmovdqa LInternalAvx2FindBytePairMaskPathShiftMask2(%rip), %ymm13");
	asm("prefetchnta 1536(%rsi)");
	asm("vpbroadcastb %xmm11, %ymm11");
	
	asm("vpsrlq $4, %ymm4, %ymm6");
	asm("vpsrlq $4, %ymm5, %ymm7");
	asm("vpcmpgtb %ymm11, %ymm12, %ymm12");
	asm("vpcmpgtb %ymm11, %ymm13, %ymm13");
	asm("vpand %ymm15, %ymm4, %ymm4");		// 4,5 hold low nibbles
	asm("vpand %ymm15, %ymm5, %ymm5");
	asm("vpand %ymm15, %ymm6, %ymm6");		// 6,7 hold high nibbles
	asm("vpand %ymm15, %ymm7, %ymm7");
	
	asm("vpshufb %ymm4, %ymm0, %ymm8");
	asm("vpshufb %ymm5, %ymm0, %ymm9");
	asm("vpshufb %ymm6, %ymm1, %ymm10");
	asm("vpshufb %ymm7, %ymm1, %ymm11");
	
	asm("vpshufb %ymm4, %ymm2, %ymm4");
	asm("vpshufb %ymm5, %ymm2, %ymm5");
	asm("vpshufb %ymm6, %ymm3, %ymm6");
	asm("vpshufb %ymm7, %ymm3, %ymm7");
	
	asm("vpand %ymm8, %ymm12, %ymm8");
	asm("vpand %ymm9, %ymm13, %ymm9");
	
	asm("vpand %ymm8, %ymm10, %ymm8");
	asm("vpand %ymm9, %ymm11, %ymm12");
	asm("vpand %ymm4, %ymm6, %ymm4");
	asm("vpand %ymm5, %ymm7, %ymm5");
	
	// slide the first byte window to overlap with the second
	asm("vperm2i128 $0x8, %ymm8, %ymm8, %ymm10");
	asm("vperm2i128 $0x3, %ymm8, %ymm12, %ymm11");
	asm("vpalignr $15, %ymm10, %ymm8, %ymm6");
	asm("vpalignr $15, %ymm11, %ymm12, %ymm7");
	
	asm("vpand %ymm6, %ymm4, %ymm4");
	asm("vpand %ymm7, %ymm5, %ymm5");
	
	asm("vpcmpeqb %ymm4, %ymm14, %ymm4");	// Convert them to masks
	asm("vpcmpeqb %ymm5, %ymm14, %ymm5");
	
	asm("vpmovmskb %ymm4, %eax");
	asm("vpmovmskb %ymm5, %r10d");
	
	asm("shlq $32, %r10");
	asm("orq %r10, %rax");
	asm("not %rax");
	asm("bsfq %rax, %rax");
	asm("jz LInternalAvx2FindBytePairMaskPathDoMainLoop");
	asm("addq %rsi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvx2FindBytePairMaskPathFail");
	asm("retq");
	
	asm(".balign 32");
	asm("LInternalAvx2FindBytePairMaskPathShiftMask1:");
	asm(".byte 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16");
	asm(".byte 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32");
	asm("LInternalAvx2FindBytePairMaskPathShiftMask2:");
	asm(".byte 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48");
	asm(".byte 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64");

	asm(".byte 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90");
	
	asm("LInternalAvx2FindBytePairMaskPathDoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $64, %rsi");
	asm("subq %rdx, %rsi");
	asm("jnc LInternalAvx2FindBytePairMaskPathFail");
	
	asm(".balign 32");
	asm("LInternalAvx2FindBytePairMaskPathMainLoop:");
	asm("vmovdqu 63(%r10), %ymm4");
	asm("vmovdqa 64(%r10), %ymm5");
	asm("vmovdqu 95(%r10), %ymm6");
	asm("vmovdqa 96(%r10), %ymm7");
	asm("prefetchnta 1536(%r10)");
	asm("addq $64, %r10");
	
	asm("vpsrlq $4, %ymm4, %ymm8");
	asm("vpsrlq $4, %ymm5, %ymm9");
	asm("vpsrlq $4, %ymm6, %ymm10");
	asm("vpsrlq $4, %ymm7, %ymm11");
	asm("vpand %ymm15, %ymm4, %ymm4");
	asm("vpand %ymm15, %ymm5, %ymm5");
	asm("vpand %ymm15, %ymm6, %ymm6");
	asm("vpshufb %ymm4, %ymm0, %ymm4");
	asm("vpand %ymm15, %ymm7, %ymm7");
	asm("vpshufb %ymm5, %ymm2, %ymm5");
	asm("vpand %ymm15, %ymm8, %ymm8");
	asm("vpshufb %ymm6, %ymm0, %ymm6");
	asm("vpand %ymm15, %ymm9, %ymm9");
	asm("vpshufb %ymm8, %ymm1, %ymm8");
	asm("vpand %ymm15, %ymm10, %ymm10");
	asm("vpshufb %ymm9, %ymm3, %ymm9");
	asm("vpand %ymm15, %ymm11, %ymm11");
	
	asm("vpshufb %ymm10, %ymm1, %ymm10");
	asm("vpand %ymm4, %ymm8, %ymm4");
	asm("vpshufb %ymm7, %ymm2, %ymm7");
	asm("vpand %ymm5, %ymm9, %ymm5");
	asm("vpshufb %ymm11, %ymm3, %ymm11");
	
	asm("vpand %ymm6, %ymm10, %ymm6");
	asm("vpand %ymm7, %ymm11, %ymm7");
	
	asm("vpand %ymm4, %ymm5, %ymm4");
	asm("vpand %ymm6, %ymm7, %ymm6");
    
    asm("vpor %ymm4, %ymm6, %ymm7");
    asm("vptest %ymm7, %ymm7");
	asm("jnz LInternalAvx2FindBytePairMaskPathFound");
	asm("addq $64, %rsi");
	asm("jnc LInternalAvx2FindBytePairMaskPathMainLoop");
	
	asm("LInternalAvx2FindBytePairMaskPathFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvx2FindBytePairMaskPathFound:");
    asm("vpcmpeqb %ymm4, %ymm14, %ymm4");
    asm("vpcmpeqb %ymm6, %ymm14, %ymm6");
    
    asm("vpmovmskb %ymm6, %eax");
	asm("vpmovmskb %ymm4, %r10d");
	asm("shlq $32, %rax");
	asm("orq %r10, %rax");
	asm("not %rax");
	asm("bsfq %rax, %rax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvx2FindBytePairMaskPathFail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx512FindPairNibbleMaskPath()
{
    // %xmm0 loaded with low nibble mask
    // %xmm1 loaded with high nibble mask
    // %xmm2 loaded with low nibble mask of byte 2
    // %xmm3 loaded with high nibble mask of byte 2
    
    asm("pushq %rcx");
    asm("movq %rsi, %rcx");
    asm("movl $0x0f, %eax");
    asm("andq $-64, %rsi");
    
    asm("vmovdqa64 (%rsi), %zmm4");
    asm("prefetcht0 2048(%rsi)");

    asm("vshufi32x4 $0, %zmm0, %zmm0, %zmm0");
    asm("vshufi32x4 $0, %zmm1, %zmm1, %zmm1");
    asm("vshufi32x4 $0, %zmm2, %zmm2, %zmm2");
    asm("vshufi32x4 $0, %zmm3, %zmm3, %zmm3");
    asm("vpbroadcastb %eax, %zmm15");

    asm("vpsrlq $4, %zmm4, %zmm5");
    asm("vpandq %zmm15, %zmm4, %zmm4");          // low nibbles
    asm("vpandq %zmm15, %zmm5, %zmm5");          // high nibbles

    asm("vpshufb %zmm4, %zmm0, %zmm6");
    asm("vpshufb %zmm5, %zmm1, %zmm7");
    asm("vpshufb %zmm4, %zmm2, %zmm4");
    asm("vpshufb %zmm5, %zmm3, %zmm5");

    asm("vpandq %zmm4, %zmm5, %zmm4");
    asm("vpandq %zmm6, %zmm7, %zmm6");

    asm("vshufi32x4 $0x90, %zmm6, %zmm6, %zmm7");
    asm("vpalignr $15, %zmm7, %zmm6, %zmm7");

    asm("vptestmb %zmm4, %zmm7, %k1");

    asm("kmovq %k1, %rax");
    
    asm("shrq %cl, %rax");
    asm("andq $-2, %rax");
    asm("jz LInternalAvx512FindPairNibbleMaskPathDoMainLoop");

    asm("bsfq %rax, %rax");
    asm("addq %rcx, %rax");
    asm("popq %rcx");
    asm("cmpq %rdx, %rax");
    asm("jae LInternalAvx512FindPairNibbleMaskPathFail");
    asm("retq");
    
    NOP8;

    asm("LInternalAvx512FindPairNibbleMaskPathDoMainLoop:");
    asm("movq %rsi, %r10");
    asm("addq $64, %rsi");
    asm("subq %rdx, %rsi");
    asm("popq %rcx");
    asm("jnc LInternalAvx512FindPairNibbleMaskPathFail");
    
    asm(".balign 32");
    asm("LInternalAvx512FindPairNibbleMaskPathMainLoop:");
    asm("vmovdqa64 64(%r10), %zmm4");
    asm("vmovdqu64 63(%r10), %zmm8");
    asm("prefetcht0 2048(%r10)");
    asm("addq $64, %r10");

    asm("vpsrlq $4, %zmm4, %zmm6");
    asm("vpsrlq $4, %zmm8, %zmm10");

    asm("vpandq %zmm15, %zmm4, %zmm4");          // 4 holds low nibbles of byte 2
    asm("vpandq %zmm15, %zmm6, %zmm6");          // 6 holds high nibbles of byte 2
    asm("vpandq %zmm15, %zmm8, %zmm8");          // 8 holds low nibbles of byte 1
    asm("vpandq %zmm15, %zmm10, %zmm10");        // 10 holds high nibbles of byte 1

    asm("vpshufb %zmm8, %zmm0, %zmm8");
    asm("vpshufb %zmm10, %zmm1, %zmm10");
    asm("vpshufb %zmm4, %zmm2, %zmm4");
    asm("vpshufb %zmm6, %zmm3, %zmm6");

    asm("vpandq %zmm4, %zmm6, %zmm4");
    asm("vpandq %zmm8, %zmm10, %zmm8");

    asm("vptestmb %zmm4, %zmm8, %k1");

    asm("ktestq %k1, %k1");
    asm("jnz LInternalAvx512FindPairNibbleMaskPathFound");
    asm("addq $64, %rsi");
    asm("jnc LInternalAvx512FindPairNibbleMaskPathMainLoop");
    
    asm("LInternalAvx512FindPairNibbleMaskPathFail:");
    asm("xor %eax, %eax");
    asm("retq");
    
    asm("LInternalAvx512FindPairNibbleMaskPathFound:");
    asm("kmovq %k1, %rax");
    asm("bsfq %rax, %rax");
    asm("addq %rsi, %rax");
    asm("jc LInternalAvx512FindPairNibbleMaskPathFail");
    asm("addq %rdx, %rax");
    asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvxFindTripletNibbleMaskPath()
{
	// %xmm0 loaded with low nibble mask
	// %xmm1 loaded with high nibble mask
	// %xmm2 loaded with low nibble mask of byte 2
	// %xmm3 loaded with high nibble mask of byte 2
	// %xmm4 loaded with low nibble mask of byte 3
	// %xmm5 loaded with high nibble mask of byte 3
	
	asm("movl %esi, %r10d");
	asm("movl $0x0f0f0f0f, %eax");
	asm("andq $-16, %rsi");
	asm("andl $15, %r10d");
	asm("vmovd %eax, %xmm15");
	
	asm("vpxor %xmm14, %xmm14, %xmm14");
	asm("vpshufd $0, %xmm15, %xmm15");
	asm("vmovq %r10, %xmm13");
	
	asm("vmovdqa (%rsi), %xmm10");
	asm("vmovdqa LInternalAvxFindByteTripletMaskPathShiftMask(%rip), %xmm12");
	asm("prefetchnta 128(%rsi)");
	asm("vpshufb %xmm14, %xmm13, %xmm13");
	
	asm("vpsrlq $4, %xmm10, %xmm11");
	asm("vpcmpgtb %xmm13, %xmm12, %xmm13");
	asm("vpand %xmm15, %xmm10, %xmm10");
	asm("vpand %xmm15, %xmm11, %xmm11");
	
	asm("vpshufb %xmm10, %xmm0, %xmm6");
	asm("vpshufb %xmm11, %xmm1, %xmm7");
	asm("vpshufb %xmm10, %xmm2, %xmm8");
	asm("vpshufb %xmm11, %xmm3, %xmm9");
	asm("vpand %xmm6, %xmm13, %xmm6");
	asm("vpshufb %xmm10, %xmm4, %xmm10");
	asm("vpshufb %xmm11, %xmm5, %xmm11");
	
	asm("vpand %xmm6, %xmm7, %xmm6");
	asm("vpand %xmm8, %xmm9, %xmm8");
	
	asm("vpslldq $2, %xmm6, %xmm12");
	asm("vpslldq $1, %xmm8, %xmm13");
	
	asm("vpand %xmm10, %xmm11, %xmm10");
	asm("vpand %xmm12, %xmm13, %xmm11");
	asm("vmovdqa %xmm6, %xmm12");
	asm("vmovdqa %xmm8, %xmm13");
	
	asm("vptest %xmm10, %xmm11");
	asm("vpand %xmm10, %xmm11, %xmm10");
	asm("jz LInternalAvxFindByteTripletMaskPathDoMainLoop");
	
	asm("vpcmpeqb %xmm10, %xmm14, %xmm10");
	asm("vpmovmskb %xmm10, %eax");
	asm("xorl $0xffff, %eax");

	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvxFindByteTripletMaskPathFail");
	asm("retq");
	
	asm(".balign 32");
	asm("LInternalAvxFindByteTripletMaskPathShiftMask:");
	asm(".byte 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16");
	// Padding so that MainLoop ends up on a 32-byte boundary
	asm(".byte 0x90, 0x90, 0x90, 0x90");

	asm("LInternalAvxFindByteTripletMaskPathDoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $16, %rsi");
	asm("subq %rdx, %rsi");
	asm("jnc LInternalAvxFindByteTripletMaskPathFail");
	
	asm(".balign 32");
	asm("LInternalAvxFindByteTripletMaskPathMainLoop:");
	asm("vmovdqa 16(%r10), %xmm10");
	asm("prefetchnta 144(%r10)");
	asm("addq $16, %r10");
	
	asm("vpsrlq $4, %xmm10, %xmm11");
	asm("vpand %xmm15, %xmm10, %xmm10");
	asm("vpand %xmm15, %xmm11, %xmm11");
	
	asm("vpshufb %xmm10, %xmm0, %xmm6");
	asm("vpshufb %xmm11, %xmm1, %xmm7");
	asm("vpshufb %xmm10, %xmm2, %xmm8");
	asm("vpshufb %xmm11, %xmm3, %xmm9");
	asm("vpshufb %xmm10, %xmm4, %xmm10");
	asm("vpshufb %xmm11, %xmm5, %xmm11");

	asm("vpand %xmm6, %xmm7, %xmm6");
	asm("vpand %xmm8, %xmm9, %xmm8");
	
	asm("vpalignr $14, %xmm12, %xmm6, %xmm12");
	asm("vpalignr $15, %xmm13, %xmm8, %xmm13");

	asm("vpand %xmm10, %xmm11, %xmm10");
	asm("vpand %xmm12, %xmm13, %xmm11");

	asm("vptest %xmm10, %xmm11");
	asm("vmovdqa %xmm6, %xmm12");
	asm("vmovdqa %xmm8, %xmm13");
	
	asm("jnz LInternalAvxFindByteTripletMaskPathFound");
	asm("addq $16, %rsi");
	asm("jnc LInternalAvxFindByteTripletMaskPathMainLoop");
	
	asm("LInternalAvxFindByteTripletMaskPathFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvxFindByteTripletMaskPathFound:");
	asm("vpand %xmm10, %xmm11, %xmm10");
	asm("vpcmpeqb %xmm10, %xmm14, %xmm10");
	asm("vpmovmskb %xmm10, %eax");
	asm("xorl $0xffff, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvxFindByteTripletMaskPathFail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx2FindTripletNibbleMaskPath()
{
	// %xmm0 loaded with low nibble mask
	// %xmm1 loaded with high nibble mask
	// %xmm2 loaded with low nibble mask of byte 2
	// %xmm3 loaded with high nibble mask of byte 2
	// %xmm4 loaded with low nibble mask of byte 3
	// %xmm5 loaded with high nibble mask of byte 3
	
	asm("movl %esi, %eax");
	asm("movl $0x0f, %r10d");
	asm("andq $-32, %rsi");
	asm("andl $31, %eax");
	asm("vmovq %r10, %xmm15");
	
	asm("vpxor %ymm14, %ymm14, %ymm14");
	asm("vpermq $0x44, %ymm0, %ymm0");
	asm("vpermq $0x44, %ymm1, %ymm1");
	asm("vpermq $0x44, %ymm2, %ymm2");
	asm("vpermq $0x44, %ymm3, %ymm3");
	asm("vpermq $0x44, %ymm4, %ymm4");
	asm("vpermq $0x44, %ymm5, %ymm5");
	asm("vpbroadcastb %xmm15, %ymm15");

	asm("cmpl $31, %eax");
	asm("je LInternalAvx2FindByteTripletMaskPathDoMainLoopMaskR11");
	
	asm("vmovd %eax, %xmm13");
	
	asm("vmovdqa (%rsi), %ymm10");
	asm("vmovdqa LInternalAvx2FindByteTripletMaskPathShiftMask(%rip), %ymm12");
	asm("prefetchnta 1536(%rsi)");
	asm("vpbroadcastb %xmm13, %ymm13");
	
	asm("vpsrlq $4, %ymm10, %ymm11");
	asm("vpcmpgtb %ymm13, %ymm12, %ymm13");
	asm("vpand %ymm15, %ymm10, %ymm10");
	asm("vpand %ymm15, %ymm11, %ymm11");
	
	asm("vpshufb %ymm10, %ymm0, %ymm6");
	asm("vpshufb %ymm11, %ymm1, %ymm7");
	asm("vpshufb %ymm10, %ymm2, %ymm8");
	asm("vpand %ymm6, %ymm13, %ymm6");
	asm("vpshufb %ymm11, %ymm3, %ymm9");
	asm("vpshufb %ymm10, %ymm4, %ymm10");
	asm("vpshufb %ymm11, %ymm5, %ymm11");
	
	asm("vpand %ymm6, %ymm7, %ymm6");
	asm("vpand %ymm8, %ymm9, %ymm8");
	
	asm("vperm2i128 $0x3, %ymm14, %ymm6, %ymm7");
	asm("vperm2i128 $0x3, %ymm14, %ymm8, %ymm9");
	asm("vpalignr $14, %ymm7, %ymm6, %ymm12");
	asm("vpalignr $15, %ymm9, %ymm8, %ymm13");
	
	asm("vpand %ymm10, %ymm11, %ymm10");
	asm("vpand %ymm12, %ymm13, %ymm11");
	asm("vptest %ymm10, %ymm11");
	asm("jz LInternalAvx2FindByteTripletMaskPathDoMainLoop");

	asm("vpand %ymm10, %ymm11, %ymm10");
	asm("vpcmpeqb %ymm10, %ymm14, %ymm10");
	asm("vpmovmskb %ymm10, %eax");
	
	asm("xorl $0xffffffff, %eax");	
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvx2FindByteTripletMaskPathFail");
	asm("retq");
	
	asm(".balign 32");
	asm("LInternalAvx2FindByteTripletMaskPathShiftMask:");
	asm(".byte 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16");
	asm(".byte 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32");
	
	// Padding so that MainLoop ends up on a 32-byte boundary
	asm(".byte 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90");

	asm("LInternalAvx2FindByteTripletMaskPathDoMainLoopMaskR11:");
	asm("movq %rsi, %r10");
	asm("addq $32, %rsi");
	asm("subq %rdx, %rsi");
	asm("jnc LInternalAvx2FindByteTripletMaskPathFail");
	
	asm("vmovdqu 31(%r10), %ymm12");
	asm("vperm2i128 $0x20, %ymm12, %ymm14, %ymm11");
	asm("vpalignr $15, %ymm11, %ymm12, %ymm11");
	asm("jmp LInternalAvx2FindByteTripletMaskPathMainLoopR11B0Zeroed");

	asm("LInternalAvx2FindByteTripletMaskPathDoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $32, %rsi");
	asm("subq %rdx, %rsi");
	asm("jnc LInternalAvx2FindByteTripletMaskPathFail");

	asm(".balign 32");
	asm("LInternalAvx2FindByteTripletMaskPathMainLoop:");
	asm("vmovdqu 30(%r10), %ymm11");
	asm("vmovdqu 31(%r10), %ymm12");
	asm("LInternalAvx2FindByteTripletMaskPathMainLoopR11B0Zeroed:");
	asm("vmovdqa 32(%r10), %ymm13");
	asm("prefetchnta 1536(%r10)");
	asm("addq $32, %r10");

	asm("vpsrlq $4, %ymm11, %ymm8");
	asm("vpand %ymm15, %ymm11, %ymm11");
	asm("vpsrlq $4, %ymm12, %ymm9");
	asm("vpand %ymm15, %ymm12, %ymm12");
	asm("vpsrlq $4, %ymm13, %ymm10");
	asm("vpand %ymm15, %ymm13, %ymm13");
	
	asm("vpand %ymm15, %ymm8, %ymm8");
	asm("vpand %ymm15, %ymm9, %ymm9");
	asm("vpand %ymm15, %ymm10, %ymm10");
	
	asm("vpshufb %ymm11, %ymm0, %ymm11");
	asm("vpshufb %ymm8, %ymm1, %ymm8");
	asm("vpshufb %ymm12, %ymm2, %ymm12");
	asm("vpshufb %ymm9, %ymm3, %ymm9");
	asm("vpshufb %ymm13, %ymm4, %ymm13");
	asm("vpshufb %ymm10, %ymm5, %ymm10");
	
	asm("vpand %ymm8, %ymm11, %ymm8");
	asm("vpand %ymm9, %ymm12, %ymm9");
	asm("vpand %ymm10, %ymm13, %ymm10");
	asm("vpand %ymm8, %ymm9, %ymm8");

	asm("vptest %ymm8, %ymm10");
	asm("jnz LInternalAvx2FindByteTripletMaskPathFound");
	asm("addq $32, %rsi");
	asm("jnc LInternalAvx2FindByteTripletMaskPathMainLoop");
	
	asm("LInternalAvx2FindByteTripletMaskPathFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvx2FindByteTripletMaskPathFound:");
	asm("vpand %ymm8, %ymm10, %ymm8");
	asm("vpcmpeqb %ymm8, %ymm14, %ymm8");
	asm("vpmovmskb %ymm8, %eax");
	asm("not %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvx2FindByteTripletMaskPathFail");
	asm("addq %rdx, %rax");
	asm("retq");
}

// %xmm0 = value to search for in lowest 4 bytes
// %rsi = start of search
// %rdx = end
// Result in %rax
__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalFindByte()
{
	asm("pushq %rcx");
	asm("movq %rsi, %rcx");
	asm("andq $-32, %rsi");

	asm("pshufd $0, %xmm0, %xmm0");
	
	asm("movdqa (%rsi), %xmm1");
	asm("movdqa 16(%rsi), %xmm2");
	asm("prefetchnta 512(%rsi)");
	
	asm("pcmpeqb %xmm0, %xmm1");
	asm("pcmpeqb %xmm0, %xmm2");
	asm("pmovmskb %xmm1, %eax");
	asm("pmovmskb %xmm2, %r10d");
	asm("shll $16, %r10d");
	asm("orl %r10d, %eax");
	asm("shrl %cl, %eax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalFindByteDoMainLoop");
	asm("addq %rcx, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalFindByteFail");
	asm("retq");

	NOP16;
	
	asm("LInternalFindByteDoMainLoop:");
	asm("addq $32, %rsi");				// Advance to next block
										// Want: rdx = end, rdx+rsi = current
										// -> rsi = current - end
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalFindByteFail");

	asm(".balign 32");
	asm("LInternalFindByteMainLoop:");
	asm("movdqa (%rdx, %rsi), %xmm1");
	asm("movdqa 16(%rdx, %rsi), %xmm2");
	asm("prefetchnta 512(%rdx,%rsi)");
	asm("pcmpeqb %xmm0, %xmm1");
	asm("pcmpeqb %xmm0, %xmm2");
	asm("por %xmm1, %xmm2");
	asm("pmovmskb %xmm2, %eax");
	asm("test %eax, %eax");
	asm("jne LInternalFindByteFound");
	asm("addq $32, %rsi");
	asm("jnc LInternalFindByteMainLoop");

	asm("LInternalFindByteFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalFindByteFound:");
	asm("pmovmskb %xmm1, %r10d");
	asm("shll $16, %eax");
	asm("orl %r10d, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalFindByteFail");
	asm("addq %rdx, %rax");
	asm("retq");
}

// %xmm0 = value to search for in lowest byte of xmm0
// %rsi = start of search
// %rdx = end
// Result in %rax
__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx2FindByte()
{
    asm("pushq %rcx");
    asm("movq %rsi, %rcx");
    asm("andq $-64, %rsi");

    asm("vpbroadcastb %xmm0, %ymm0");

    asm("vmovdqa (%rsi), %ymm1");
    asm("vmovdqa 32(%rsi), %ymm2");
    asm("vpcmpeqb %ymm0, %ymm1, %ymm1");
    asm("vpcmpeqb %ymm0, %ymm2, %ymm2");
    asm("vpmovmskb %ymm1, %eax");
    asm("vpmovmskb %ymm2, %r10d");
    asm("shlq $32, %r10");
    asm("orq %r10, %rax");
    asm("shrq %cl, %rax");
    asm("bsfq %rax, %rax");
    asm("jz LInternalAvx2FindByteDoMainLoop");
    asm("addq %rcx, %rax");
    asm("popq %rcx");
    asm("cmpq %rdx, %rax");
    asm("jae LInternalAvx2FindByteFail");
    asm("retq");

    NOP22;
    
    asm("LInternalAvx2FindByteDoMainLoop:");
    asm("addq $64, %rsi");
    asm("subq %rdx, %rsi");
    asm("popq %rcx");
    asm("jnc LInternalAvx2FindByteFail");
    
    asm(".balign 32");
    asm("LInternalAvx2FindByteMainLoop:");
    asm("vmovdqa (%rdx, %rsi), %ymm1");
    asm("vmovdqa 32(%rdx, %rsi), %ymm2");
    asm("prefetchnta 1536(%rdx,%rsi)");
    asm("vpcmpeqb %ymm0, %ymm1, %ymm1");
    asm("vpcmpeqb %ymm0, %ymm2, %ymm2");
    asm("vpor %ymm1, %ymm2, %ymm2");
    asm("vpmovmskb %ymm2, %eax");
    asm("test %eax, %eax");
    asm("jne LInternalAvx2FindByteFound");
    asm("addq $64, %rsi");
    asm("jnc LInternalAvx2FindByteMainLoop");
    
    asm("LInternalAvx2FindByteFail:");
    asm("xor %eax, %eax");
    asm("retq");
    
    asm("LInternalAvx2FindByteFound:");
    asm("vpmovmskb %ymm1, %r10d");
    asm("shlq $32, %rax");
    asm("orq %r10, %rax");
    asm("bsfq %rax, %rax");
    asm("addq %rsi, %rax");
    asm("jc LInternalAvx2FindByteFail");
    asm("addq %rdx, %rax");
    asm("retq");
}

// %xmm0 = value to search for in lowest byte of xmm0
// %rsi = start of search
// %rdx = end
// Result in %rax
__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx512FindByte()
{
    asm("pushq %rcx");
    asm("movq %rsi, %rcx");
    asm("andq $-64, %rsi");

    asm("vpbroadcastb %xmm0, %zmm0");

    asm("vmovdqa64 (%rsi), %zmm1");
    asm("prefetcht0 2048(%rsi)");
    
    asm("vpcmpeqb %zmm0, %zmm1, %k1");
    
    asm("kmovq %k1, %rax");
    asm("shrq %cl, %rax");
    asm("jz LInternalAvx512FindByteDoMainLoop");
    
    asm("bsfq %rax, %rax");
    asm("addq %rcx, %rax");
    asm("popq %rcx");
    asm("cmpq %rdx, %rax");
    asm("jae LInternalAvx512FindByteFail");
    asm("retq");

    NOP26;
    
    asm("LInternalAvx512FindByteDoMainLoop:");
    asm("movq %rsi, %r10");
    asm("addq $64, %rsi");
    asm("subq %rdx, %rsi");
    asm("popq %rcx");
    asm("jnc LInternalAvx512FindByteFail");
    
    asm(".balign 32");
    asm("LInternalAvx512FindByteMainLoop:");
    asm("vmovdqa64 64(%r10), %zmm1");
    asm("prefetcht0 2048(%r10)");
    asm("add $64, %r10");
    asm("vpcmpeqb %zmm0, %zmm1, %k1");
    asm("kortestq %k1, %k1");
    asm("jnz LInternalAvx512FindByteFound");
    asm("addq $64, %rsi");
    asm("jnc LInternalAvx512FindByteMainLoop");
    
    asm("LInternalAvx512FindByteFail:");
    asm("xor %eax, %eax");
    asm("retq");
    
    asm("LInternalAvx512FindByteFound:");
    asm("kmovq %k1, %rax");
    asm("bsfq %rax, %rax");
    asm("addq %rsi, %rax");
    asm("jc LInternalAvx512FindByteFail");
    asm("addq %rdx, %rax");
    asm("retq");
}

// %xmm0 must be loaded with the first byte to search in the lower 32 bits
// %xmm1 must be loaded with the second byte to search in the lower 32 bits
__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalFindEitherOf2()
{
	asm("pushq %rcx");
	asm("movq %rsi, %rcx");
	asm("andq $-32, %rsi");
	
	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm1, %xmm1");
	
	asm("movdqa (%rsi), %xmm2");
	asm("movdqa (%rsi), %xmm3");
	asm("movdqa 16(%rsi), %xmm4");
	asm("movdqa 16(%rsi), %xmm5");
	asm("prefetchnta 512(%rsi)");
	asm("pcmpeqb %xmm0, %xmm2");
	asm("pcmpeqb %xmm1, %xmm3");
	asm("pcmpeqb %xmm0, %xmm4");
	asm("pcmpeqb %xmm1, %xmm5");
	asm("por     %xmm2, %xmm3");
	asm("por     %xmm4, %xmm5");
	asm("pmovmskb %xmm3, %eax");
	asm("pmovmskb %xmm5, %r10d");
	asm("shll $16, %r10d");
	asm("orl %r10d, %eax");
	asm("shrl %cl, %eax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalFindEitherOf2DoMainLoop");
	asm("addq %rcx, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalFindEitherOf2Fail");
	asm("retq");
	
	NOP15;
	
	asm("LInternalFindEitherOf2DoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $32, %rsi");				// Advance to next block
										// Want: rdx = end, rdx+rsi = current
										// -> rsi = current - end
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalFindEitherOf2Fail");
	
	asm(".balign 32");
	asm("LInternalFindEitherOf2MainLoop:");
	asm("movdqa 32(%r10), %xmm2");
	asm("movdqa 32(%r10), %xmm3");
	asm("movdqa 48(%r10), %xmm4");
	asm("movdqa 48(%r10), %xmm5");
	asm("prefetchnta 544(%r10)");
	asm("addq $32, %r10");
	asm("pcmpeqb %xmm0, %xmm2");
	asm("pcmpeqb %xmm1, %xmm3");
	asm("pcmpeqb %xmm0, %xmm4");
	asm("pcmpeqb %xmm1, %xmm5");
	asm("por     %xmm2, %xmm3");
	asm("por     %xmm4, %xmm5");
	asm("por 	 %xmm3, %xmm5");
	asm("pmovmskb %xmm5, %eax");
	asm("test %eax, %eax");
	asm("jne LInternalFindEitherOf2Found");
	asm("addq $32, %rsi");
	asm("jnc LInternalFindEitherOf2MainLoop");
	
	asm("LInternalFindEitherOf2Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalFindEitherOf2Found:");
	asm("pmovmskb %xmm3, %r10d");
	asm("shll $16, %eax");
	asm("orl %r10d, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalFindEitherOf2Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

// %xmm0 must be loaded with the first byte to search in the lower 32 bits
// %xmm1 must be loaded with the second byte to search in the lower 32 bits
__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvxFindEitherOf2()
{
	asm("pushq %rcx");
	asm("movq %rsi, %rcx");
	asm("andq $-32, %rsi");
	
	asm("vpshufd $0, %xmm0, %xmm0");
	asm("vpshufd $0, %xmm1, %xmm1");
	
	asm("vmovdqa (%rsi), %xmm3");
	asm("vmovdqa 16(%rsi), %xmm5");
	asm("prefetchnta 512(%rsi)");
	asm("vpcmpeqb %xmm0, %xmm3, %xmm2");
	asm("vpcmpeqb %xmm1, %xmm3, %xmm3");
	asm("vpcmpeqb %xmm0, %xmm5, %xmm4");
	asm("vpcmpeqb %xmm1, %xmm5, %xmm5");
	asm("vpor     %xmm2, %xmm3, %xmm3");
	asm("vpor     %xmm4, %xmm5, %xmm5");
	asm("vpmovmskb %xmm3, %eax");
	asm("vpmovmskb %xmm5, %r10d");
	asm("shll $16, %r10d");
	asm("orl %r10d, %eax");
	asm("shrl %cl, %eax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalAvxFindEitherOf2DoMainLoop");
	asm("addq %rcx, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvxFindEitherOf2Fail");
	asm("retq");
	
	asm(".balign 32");
	asm(".byte 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90");
	
	asm("LInternalAvxFindEitherOf2DoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $32, %rsi");
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalAvxFindEitherOf2Fail");
	
	asm(".balign 32");
	asm("LInternalAvxFindEitherOf2MainLoop:");
	asm("vmovdqa 32(%r10), %xmm3");
	asm("vmovdqa 48(%r10), %xmm5");
	asm("prefetchnta 544(%r10)");
	asm("addq $32, %r10");

	asm("vpcmpeqb %xmm0, %xmm3, %xmm2");
	asm("vpcmpeqb %xmm1, %xmm3, %xmm3");
	asm("vpcmpeqb %xmm0, %xmm5, %xmm4");
	asm("vpcmpeqb %xmm1, %xmm5, %xmm5");
	asm("vpor     %xmm2, %xmm3, %xmm3");
	asm("vpor     %xmm4, %xmm5, %xmm5");
	asm("vpor 	  %xmm3, %xmm5, %xmm5");
	asm("vpmovmskb %xmm5, %eax");
	asm("test %eax, %eax");
	asm("jne LInternalAvxFindEitherOf2Found");
	asm("addq $32, %rsi");
	asm("jnc LInternalAvxFindEitherOf2MainLoop");
	
	asm("LInternalAvxFindEitherOf2Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvxFindEitherOf2Found:");
	asm("vpmovmskb %xmm3, %r10d");
	asm("shll $16, %eax");
	asm("orl %r10d, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvxFindEitherOf2Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

// %xmm0 must be loaded with the first byte to search in the lower 32 bits
// %xmm1 must be loaded with the second byte to search in the lower 32 bits
__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx2FindEitherOf2()
{
	asm("pushq %rcx");
	asm("movq %rsi, %rcx");
	asm("andq $-64, %rsi");
	
	asm("vmovdqa (%rsi), %ymm3");
	asm("vmovdqa 32(%rsi), %ymm5");
	asm("prefetchnta 1536(%rsi)");
	
	asm("vpbroadcastb %xmm0, %ymm0");
	asm("vpbroadcastb %xmm1, %ymm1");

	asm("vpcmpeqb %ymm0, %ymm3, %ymm2");
	asm("vpcmpeqb %ymm1, %ymm3, %ymm3");
	asm("vpcmpeqb %ymm0, %ymm5, %ymm4");
	asm("vpcmpeqb %ymm1, %ymm5, %ymm5");
	asm("vpor     %ymm2, %ymm3, %ymm3");
	asm("vpor     %ymm4, %ymm5, %ymm5");
	asm("vpmovmskb %ymm3, %eax");
	asm("vpmovmskb %ymm5, %r10d");
	asm("shlq $32, %r10");
	asm("orq %r10, %rax");
	asm("shrq %cl, %rax");
	asm("bsfq %rax, %rax");
	asm("jz LInternalAvx2FindEitherOf2DoMainLoop");
	asm("addq %rcx, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvx2FindEitherOf2Fail");
	asm("retq");
	
	asm(".balign 32");
	asm(".byte 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90");
	
	asm("LInternalAvx2FindEitherOf2DoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $64, %rsi");
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalAvx2FindEitherOf2Fail");
	
	asm(".balign 32");
	asm("LInternalAvx2FindEitherOf2MainLoop:");
	asm("vmovdqa 64(%r10), %ymm3");
	asm("vmovdqa 96(%r10), %ymm5");
	asm("prefetchnta 1536(%r10)");
	asm("addq $64, %r10");
	
	asm("vpcmpeqb %ymm0, %ymm3, %ymm2");
	asm("vpcmpeqb %ymm1, %ymm3, %ymm3");
	asm("vpcmpeqb %ymm0, %ymm5, %ymm4");
	asm("vpcmpeqb %ymm1, %ymm5, %ymm5");
	asm("vpor     %ymm2, %ymm3, %ymm3");
	asm("vpor     %ymm4, %ymm5, %ymm5");
	asm("vpor 	  %ymm3, %ymm5, %ymm5");
	asm("vpmovmskb %ymm5, %eax");
	asm("test %eax, %eax");
	asm("jne LInternalAvx2FindEitherOf2Found");
	asm("addq $64, %rsi");
	asm("jnc LInternalAvx2FindEitherOf2MainLoop");
	
	asm("LInternalAvx2FindEitherOf2Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvx2FindEitherOf2Found:");
	asm("vpmovmskb %ymm3, %r10d");
	asm("shlq $32, %rax");
	asm("orq %r10, %rax");
	asm("bsfq %rax, %rax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvx2FindEitherOf2Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

// %xmm0 must be loaded with the first byte to search in the lower 32 bits
// %xmm1 must be loaded with the second byte to search in the lower 32 bits
__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx512FindEitherOf2()
{
    asm("pushq %rcx");
    asm("movq %rsi, %rcx");
    asm("andq $-64, %rsi");
    
    asm("vmovdqa64 (%rsi), %zmm2");
    asm("prefetcht0 2048(%rsi)");
    
    asm("vpbroadcastb %xmm0, %zmm0");
    asm("vpbroadcastb %xmm1, %zmm1");

    asm("vpcmpeqb %zmm0, %zmm2, %k1");
    asm("vpcmpeqb %zmm1, %zmm2, %k2");
    
    asm("korq %k1, %k2, %k1");
    asm("kmovq %k1, %rax");
    asm("shrq %cl, %rax");
    asm("jz LInternalAvx512FindEitherOf2DoMainLoop");
    
    asm("bsfq %rax, %rax");
    asm("addq %rcx, %rax");
    asm("popq %rcx");
    asm("cmpq %rdx, %rax");
    asm("jae LInternalAvx512FindEitherOf2Fail");
    asm("retq");
    
    NOP9;
    
    asm("LInternalAvx512FindEitherOf2DoMainLoop:");
    asm("mov %rsi, %r10");
    asm("addq $64, %rsi");
    asm("subq %rdx, %rsi");
    asm("popq %rcx");
    asm("jnc LInternalAvx512FindEitherOf2Fail");
    
    asm(".balign 32");
    asm("LInternalAvx512FindEitherOf2MainLoop:");
    asm("vmovdqa64 64(%r10), %zmm2");
    asm("prefetcht0 2048(%r10)");
    asm("add $64, %r10");

    asm("vpcmpeqb %zmm0, %zmm2, %k1");
    asm("vpcmpeqb %zmm1, %zmm2, %k2");
    asm("kortestq %k1, %k2");
    asm("jne LInternalAvx512FindEitherOf2Found");
    asm("addq $64, %rsi");
    asm("jnc LInternalAvx512FindEitherOf2MainLoop");
    
    asm("LInternalAvx512FindEitherOf2Fail:");
    asm("xor %eax, %eax");
    asm("retq");
    
    asm("LInternalAvx512FindEitherOf2Found:");
    asm("korq %k1, %k2, %k1");
    asm("kmovq %k1, %rax");
    asm("bsfq %rax, %rax");
    asm("addq %rsi, %rax");
    asm("jc LInternalAvx512FindEitherOf2Fail");
    asm("addq %rdx, %rax");
    asm("retq");
}

// %xmm0 must be loaded with the first byte to search in the lower 32 bits
// %xmm1 must be loaded with the second byte to search in the lower 32 bits
// %xmm2 must be loaded with the third byte to search in the lower 32 bits
__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalFindEitherOf3()
{
	asm("pushq %rcx");
	asm("movq %rsi, %rcx");
	asm("andq $-32, %rsi");

	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm1, %xmm1");
	asm("pshufd $0, %xmm2, %xmm2");
	
	asm("movdqa (%rsi), %xmm3");
	asm("movdqa (%rsi), %xmm4");
	asm("movdqa (%rsi), %xmm5");
	asm("movdqa 16(%rsi), %xmm6");
	asm("movdqa 16(%rsi), %xmm7");
	asm("movdqa 16(%rsi), %xmm8");
	asm("prefetchnta 512(%rsi)");
	asm("pcmpeqb %xmm0, %xmm3");
	asm("pcmpeqb %xmm1, %xmm4");
	asm("pcmpeqb %xmm2, %xmm5");
	asm("pcmpeqb %xmm0, %xmm6");
	asm("pcmpeqb %xmm1, %xmm7");
	asm("pcmpeqb %xmm2, %xmm8");
	asm("por     %xmm3, %xmm4");
	asm("por     %xmm6, %xmm7");
	asm("por     %xmm4, %xmm5");
	asm("por     %xmm7, %xmm8");
	asm("pmovmskb %xmm5, %eax");
	asm("pmovmskb %xmm8, %r10d");
	asm("shll $16, %r10d");
	asm("orl %r10d, %eax");
	asm("shrl %cl, %eax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalFindEitherOf3DoMainLoop");
	asm("addq %rcx, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalFindEitherOf3Fail");
	asm("retq");
	
	NOP7;
	NOP3;
	
	asm("LInternalFindEitherOf3DoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $32, %rsi");				// Advance to next block
										// Want: rdx = end, rdx+rsi = current
										// -> rsi = current - end
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalFindEitherOf3Fail");
	
	asm(".balign 32");
	asm("LInternalFindEitherOf3MainLoop:");
	asm("movdqa 32(%r10), %xmm3");
	asm("movdqa 32(%r10), %xmm4");
	asm("movdqa 32(%r10), %xmm5");
	asm("movdqa 48(%r10), %xmm6");
	asm("movdqa 48(%r10), %xmm7");
	asm("movdqa 48(%r10), %xmm8");
	asm("prefetchnta 544(%r10)");
	asm("addq $32, %r10");
	asm("pcmpeqb %xmm0, %xmm3");
	asm("pcmpeqb %xmm1, %xmm4");
	asm("pcmpeqb %xmm2, %xmm5");
	asm("pcmpeqb %xmm0, %xmm6");
	asm("pcmpeqb %xmm1, %xmm7");
	asm("pcmpeqb %xmm2, %xmm8");
	asm("por     %xmm3, %xmm4");
	asm("por     %xmm6, %xmm7");
	asm("por     %xmm4, %xmm5");
	asm("por     %xmm7, %xmm8");
	asm("por     %xmm5, %xmm8");
	asm("pmovmskb %xmm8, %eax");
	asm("test %eax, %eax");
	asm("jne LInternalFindEitherOf3Found");
	asm("addq $32, %rsi");
	asm("jnc LInternalFindEitherOf3MainLoop");
	
	asm("LInternalFindEitherOf3Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalFindEitherOf3Found:");
	asm("pmovmskb %xmm5, %r10d");
	asm("shll $16, %eax");
	asm("orl %r10d, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalFindEitherOf3Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvxFindEitherOf3()
{
	asm("pushq %rcx");
	asm("movq %rsi, %rcx");
	asm("andq $-32, %rsi");
	
	asm("vpshufd $0, %xmm0, %xmm0");
	asm("vpshufd $0, %xmm1, %xmm1");
	asm("vpshufd $0, %xmm2, %xmm2");
	
	asm("vmovdqa (%rsi), %xmm5");
	asm("vmovdqa 16(%rsi), %xmm8");
	asm("prefetchnta 512(%rsi)");
	asm("vpcmpeqb %xmm0, %xmm5, %xmm3");
	asm("vpcmpeqb %xmm1, %xmm5, %xmm4");
	asm("vpcmpeqb %xmm2, %xmm5, %xmm5");
	asm("vpcmpeqb %xmm0, %xmm8, %xmm6");
	asm("vpcmpeqb %xmm1, %xmm8, %xmm7");
	asm("vpcmpeqb %xmm2, %xmm8, %xmm8");
	asm("vpor     %xmm3, %xmm4, %xmm4");
	asm("vpor     %xmm6, %xmm7, %xmm7");
	asm("vpor     %xmm4, %xmm5, %xmm5");
	asm("vpor     %xmm7, %xmm8, %xmm8");
	asm("vpmovmskb %xmm5, %eax");
	asm("vpmovmskb %xmm8, %r10d");
	asm("shll $16, %r10d");
	asm("orl %r10d, %eax");
	asm("shrl %cl, %eax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalAvxFindEitherOf3DoMainLoop");
	asm("addq %rcx, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvxFindEitherOf3Fail");
	asm("retq");
	
	NOP3;
	
	asm("LInternalAvxFindEitherOf3DoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $32, %rsi");
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalAvxFindEitherOf3Fail");
	
	asm(".balign 32");
	asm("LInternalAvxFindEitherOf3MainLoop:");
	asm("vmovdqa 32(%r10), %xmm5");
	asm("vmovdqa 48(%r10), %xmm8");
	asm("prefetchnta 544(%r10)");
	asm("addq $32, %r10");
	asm("vpcmpeqb %xmm0, %xmm5, %xmm3");
	asm("vpcmpeqb %xmm1, %xmm5, %xmm4");
	asm("vpcmpeqb %xmm2, %xmm5, %xmm5");
	asm("vpcmpeqb %xmm0, %xmm8, %xmm6");
	asm("vpcmpeqb %xmm1, %xmm8, %xmm7");
	asm("vpcmpeqb %xmm2, %xmm8, %xmm8");
	asm("vpor     %xmm3, %xmm4, %xmm4");
	asm("vpor     %xmm6, %xmm7, %xmm7");
	asm("vpor     %xmm4, %xmm5, %xmm5");
	asm("vpor     %xmm7, %xmm8, %xmm8");
	asm("vpor     %xmm5, %xmm8, %xmm8");
	asm("vpmovmskb %xmm8, %eax");
	asm("test %eax, %eax");
	asm("jne LInternalAvxFindEitherOf3Found");
	asm("addq $32, %rsi");
	asm("jnc LInternalAvxFindEitherOf3MainLoop");
	
	asm("LInternalAvxFindEitherOf3Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvxFindEitherOf3Found:");
	asm("vpmovmskb %xmm5, %r10d");
	asm("shll $16, %eax");
	asm("orl %r10d, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvxFindEitherOf3Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx2FindEitherOf3()
{
	asm("pushq %rcx");
	asm("movq %rsi, %rcx");
	asm("andq $-64, %rsi");
	
	asm("vmovdqa (%rsi), %ymm5");
	asm("vmovdqa 32(%rsi), %ymm8");
	asm("prefetchnta 1536(%rsi)");
	
	asm("vpbroadcastb %xmm0, %ymm0");
	asm("vpbroadcastb %xmm1, %ymm1");
	asm("vpbroadcastb %xmm2, %ymm2");

	asm("vpcmpeqb %ymm0, %ymm5, %ymm3");
	asm("vpcmpeqb %ymm1, %ymm5, %ymm4");
	asm("vpcmpeqb %ymm2, %ymm5, %ymm5");
	asm("vpcmpeqb %ymm0, %ymm8, %ymm6");
	asm("vpcmpeqb %ymm1, %ymm8, %ymm7");
	asm("vpcmpeqb %ymm2, %ymm8, %ymm8");
	asm("vpor     %ymm3, %ymm4, %ymm4");
	asm("vpor     %ymm6, %ymm7, %ymm7");
	asm("vpor     %ymm4, %ymm5, %ymm5");
	asm("vpor     %ymm7, %ymm8, %ymm8");
	asm("vpmovmskb %ymm5, %eax");
	asm("vpmovmskb %ymm8, %r10d");
	asm("shlq $32, %r10");
	asm("orq %r10, %rax");
	asm("shrq %cl, %rax");
	asm("bsfq %rax, %rax");
	asm("jz LInternalAvx2FindEitherOf3DoMainLoop");
	asm("addq %rcx, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvx2FindEitherOf3Fail");
	asm("retq");
	
	NOP1;
	
	asm("LInternalAvx2FindEitherOf3DoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $64, %rsi");
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalAvx2FindEitherOf3Fail");
	
	asm(".balign 32");
	asm("LInternalAvx2FindEitherOf3MainLoop:");
	asm("vmovdqa 64(%r10), %ymm5");
	asm("vmovdqa 96(%r10), %ymm8");
	asm("prefetchnta 1536(%r10)");
	asm("addq $64, %r10");
	asm("vpcmpeqb %ymm0, %ymm5, %ymm3");
	asm("vpcmpeqb %ymm1, %ymm5, %ymm4");
	asm("vpcmpeqb %ymm2, %ymm5, %ymm5");
	asm("vpcmpeqb %ymm0, %ymm8, %ymm6");
	asm("vpcmpeqb %ymm1, %ymm8, %ymm7");
	asm("vpcmpeqb %ymm2, %ymm8, %ymm8");
	asm("vpor     %ymm3, %ymm4, %ymm4");
	asm("vpor     %ymm6, %ymm7, %ymm7");
	asm("vpor     %ymm4, %ymm5, %ymm5");
	asm("vpor     %ymm7, %ymm8, %ymm8");
	asm("vpor     %ymm5, %ymm8, %ymm8");
	asm("vpmovmskb %ymm8, %eax");
	asm("test %eax, %eax");
	asm("jne LInternalAvx2FindEitherOf3Found");
	asm("addq $64, %rsi");
	asm("jnc LInternalAvx2FindEitherOf3MainLoop");
	
	asm("LInternalAvx2FindEitherOf3Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvx2FindEitherOf3Found:");
	asm("vpmovmskb %ymm5, %r10d");
	asm("shlq $32, %rax");
	asm("orq %r10, %rax");
	asm("bsfq %rax, %rax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvx2FindEitherOf3Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx512FindEitherOf3()
{
    asm("pushq %rcx");
    asm("movq %rsi, %rcx");
    asm("andq $-64, %rsi");
    
    asm("vmovdqa64 (%rsi), %zmm5");
    asm("prefetcht0 2048(%rsi)");
    
    asm("vpbroadcastb %xmm0, %zmm0");
    asm("vpbroadcastb %xmm1, %zmm1");
    asm("vpbroadcastb %xmm2, %zmm2");

    asm("vpcmpeqb %zmm0, %zmm5, %k1");
    asm("vpcmpeqb %zmm1, %zmm5, %k2");
    asm("vpcmpeqb %zmm2, %zmm5, %k3");
    
    asm("korq %k1, %k2, %k1");
    asm("korq %k1, %k3, %k1");
    asm("kmovq %k1, %rax");
    asm("shrq %cl, %rax");
    asm("bsfq %rax, %rax");
    asm("jz LInternalAvx512FindEitherOf3DoMainLoop");
    
    asm("addq %rcx, %rax");
    asm("popq %rcx");
    asm("cmpq %rdx, %rax");
    asm("jae LInternalAvx512FindEitherOf3Fail");
    asm("retq");
    
    NOP24;
    
    asm("LInternalAvx512FindEitherOf3DoMainLoop:");
    asm("movq %rsi, %r10");
    asm("addq $64, %rsi");
    asm("subq %rdx, %rsi");
    asm("popq %rcx");
    asm("jnc LInternalAvx512FindEitherOf3Fail");
    
    asm(".balign 32");
    asm("LInternalAvx512FindEitherOf3MainLoop:");
    asm("vmovdqa64 64(%r10), %zmm5");
    asm("prefetcht0 2048(%r10)");
    asm("addq $64, %r10");
    asm("vpcmpeqb %zmm0, %zmm5, %k1");
    asm("vpcmpeqb %zmm1, %zmm5, %k2");
    asm("vpcmpeqb %zmm2, %zmm5, %k3");
    asm("korq %k1, %k2, %k1");
    asm("kortestq %k1, %k3");
    asm("jne LInternalAvx512FindEitherOf3Found");
    asm("addq $64, %rsi");
    asm("jnc LInternalAvx512FindEitherOf3MainLoop");
    
    asm("LInternalAvx512FindEitherOf3Fail:");
    asm("xor %eax, %eax");
    asm("retq");
    
    asm("LInternalAvx512FindEitherOf3Found:");
    asm("korq %k1, %k3, %k1");
    asm("kmovq %k1, %rax");
    asm("bsfq %rax, %rax");
    asm("addq %rsi, %rax");
    asm("jc LInternalAvx512FindEitherOf3Fail");
    asm("addq %rdx, %rax");
    asm("retq");
}

// %xmm0 must be loaded with the first byte to search in the lower 32 bits
// %xmm1 must be loaded with the second byte to search in the lower 32 bits
// %xmm2 must be loaded with the third byte to search in the lower 32 bits
// %xmm3 must be loaded with the fourth byte to search in the lower 32 bits
__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalFindEitherOf4()
{
	asm("pushq %rcx");
	asm("movq %rsi, %rcx");
	asm("andq $-32, %rsi");

	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm1, %xmm1");
	asm("pshufd $0, %xmm2, %xmm2");
	asm("pshufd $0, %xmm3, %xmm3");
	
	asm("movdqa (%rsi), %xmm4");
	asm("movdqa (%rsi), %xmm5");
	asm("movdqa (%rsi), %xmm6");
	asm("movdqa (%rsi), %xmm7");
	asm("movdqa 16(%rsi), %xmm8");
	asm("movdqa 16(%rsi), %xmm9");
	asm("movdqa 16(%rsi), %xmm10");
	asm("movdqa 16(%rsi), %xmm11");
	asm("prefetchnta 256(%rsi)");
	asm("pcmpeqb %xmm0, %xmm4");
	asm("pcmpeqb %xmm1, %xmm5");
	asm("pcmpeqb %xmm2, %xmm6");
	asm("pcmpeqb %xmm3, %xmm7");
	asm("pcmpeqb %xmm0, %xmm8");
	asm("pcmpeqb %xmm1, %xmm9");
	asm("pcmpeqb %xmm2, %xmm10");
	asm("pcmpeqb %xmm3, %xmm11");
	asm("por     %xmm4, %xmm5");
	asm("por     %xmm6, %xmm7");
	asm("por     %xmm8, %xmm9");
	asm("por     %xmm10, %xmm11");
	asm("por     %xmm5, %xmm7");
	asm("por     %xmm9, %xmm11");
	asm("pmovmskb %xmm7, %eax");
	asm("pmovmskb %xmm11, %r10d");
	asm("shll $16, %r10d");
	asm("orl %r10d, %eax");
	asm("shrl %cl, %eax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalFindEitherOf4DoMainLoop");
	asm("addq %rcx, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalFindEitherOf4Fail");
	asm("retq");
	
	NOP3;
	
	asm("LInternalFindEitherOf4DoMainLoop:");
	asm("addq $32, %rsi");				// Advance to next block
										// Want: rdx = end, rdx+rsi = current
										// -> rsi = current - end
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalFindEitherOf4Fail");
	
	asm(".balign 32");
	asm("LInternalFindEitherOf4MainLoop:");
	asm("movdqa (%rdx, %rsi), %xmm4");
	asm("movdqa (%rdx, %rsi), %xmm5");
	asm("movdqa (%rdx, %rsi), %xmm6");
	asm("movdqa (%rdx, %rsi), %xmm7");
	asm("movdqa 16(%rdx, %rsi), %xmm8");
	asm("movdqa 16(%rdx, %rsi), %xmm9");
	asm("movdqa 16(%rdx, %rsi), %xmm10");
	asm("movdqa 16(%rdx, %rsi), %xmm11");
	asm("prefetchnta 256(%rdx, %rsi)");
	asm("pcmpeqb %xmm0, %xmm4");
	asm("pcmpeqb %xmm1, %xmm5");
	asm("pcmpeqb %xmm2, %xmm6");
	asm("pcmpeqb %xmm3, %xmm7");
	asm("pcmpeqb %xmm0, %xmm8");
	asm("pcmpeqb %xmm1, %xmm9");
	asm("pcmpeqb %xmm2, %xmm10");
	asm("pcmpeqb %xmm3, %xmm11");
	asm("por     %xmm4, %xmm5");
	asm("por     %xmm6, %xmm7");
	asm("por     %xmm8, %xmm9");
	asm("por     %xmm10, %xmm11");
	asm("por     %xmm5, %xmm7");
	asm("por     %xmm9, %xmm11");
	asm("por     %xmm7, %xmm11");
	asm("pmovmskb %xmm11, %eax");
	asm("test %eax, %eax");
	asm("jne LInternalFindEitherOf4Found");
	asm("addq $32, %rsi");
	asm("jnc LInternalFindEitherOf4MainLoop");
	
	asm("LInternalFindEitherOf4Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalFindEitherOf4Found:");
	asm("pmovmskb %xmm7, %r10d");
	asm("shll $16, %eax");
	asm("orl %r10d, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalFindEitherOf4Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalFindEitherOf5()
{
	asm("pushq %rcx");
	asm("movl %esi, %ecx");
	asm("andq $-16, %rsi");
	asm("andl $15, %ecx");
	
	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm1, %xmm1");
	asm("pshufd $0, %xmm2, %xmm2");
	asm("pshufd $0, %xmm3, %xmm3");
	asm("pshufd $0, %xmm4, %xmm4");
	
	asm("movdqa (%rsi), %xmm8");
	asm("movdqa (%rsi), %xmm9");
	asm("movdqa (%rsi), %xmm10");
	asm("movdqa (%rsi), %xmm11");
	asm("movdqa (%rsi), %xmm12");
	asm("prefetchnta 128(%rsi)");
	
	asm("pcmpeqb %xmm0, %xmm8");
	asm("pcmpeqb %xmm1, %xmm9");
	asm("pcmpeqb %xmm2, %xmm10");
	asm("pcmpeqb %xmm3, %xmm11");
	asm("pcmpeqb %xmm4, %xmm12");
	
	asm("por     %xmm8, %xmm9");
	asm("por     %xmm10, %xmm11");
	asm("por     %xmm12, %xmm9");
	asm("por     %xmm11, %xmm9");
	
	asm("pmovmskb %xmm9, %eax");
	
	asm("shrl %cl, %eax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalFindEitherOf5DoMainLoop");
	asm("addq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalFindEitherOf5Fail");
	asm("retq");
	
	NOP13;
	
	asm("LInternalFindEitherOf5DoMainLoop:");
	asm("addq $16, %rsi");					// Advance to next block
											// Want: rdx = end, rdx+rsi = current
											// -> rsi = current - end
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalFindEitherOf5Fail");
	
	asm(".balign 32");
	asm("LInternalFindEitherOf5MainLoop:");
	asm("movdqa (%rdx, %rsi), %xmm8");
	asm("movdqa (%rdx, %rsi), %xmm9");
	asm("movdqa (%rdx, %rsi), %xmm10");
	asm("movdqa (%rdx, %rsi), %xmm11");
	asm("movdqa (%rdx, %rsi), %xmm12");
	asm("prefetchnta 128(%rdx, %rsi)");
	
	asm("pcmpeqb %xmm0, %xmm8");
	asm("pcmpeqb %xmm1, %xmm9");
	asm("pcmpeqb %xmm2, %xmm10");
	asm("pcmpeqb %xmm3, %xmm11");
	asm("pcmpeqb %xmm4, %xmm12");
	
	asm("por     %xmm8, %xmm9");
	asm("por     %xmm10, %xmm11");
	asm("por     %xmm12, %xmm9");
	asm("por     %xmm11, %xmm9");
	
	asm("pmovmskb %xmm9, %eax");
	
	asm("test %eax, %eax");
	asm("jne LInternalFindEitherOf5Found");
	asm("addq $16, %rsi");
	asm("jnc LInternalFindEitherOf5MainLoop");
	
	asm("LInternalFindEitherOf5Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalFindEitherOf5Found:");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalFindEitherOf5Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalFindEitherOf6()
{
	asm("pushq %rcx");
	asm("movl %esi, %ecx");
	asm("andq $-16, %rsi");
	asm("andl $15, %ecx");
	
	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm1, %xmm1");
	asm("pshufd $0, %xmm2, %xmm2");
	asm("pshufd $0, %xmm3, %xmm3");
	asm("pshufd $0, %xmm4, %xmm4");
	asm("pshufd $0, %xmm5, %xmm5");
	
	asm("movdqa (%rsi), %xmm8");
	asm("movdqa (%rsi), %xmm9");
	asm("movdqa (%rsi), %xmm10");
	asm("movdqa (%rsi), %xmm11");
	asm("movdqa (%rsi), %xmm12");
	asm("movdqa (%rsi), %xmm13");
	asm("prefetchnta 128(%rsi)");

	asm("pcmpeqb %xmm0, %xmm8");
	asm("pcmpeqb %xmm1, %xmm9");
	asm("pcmpeqb %xmm2, %xmm10");
	asm("pcmpeqb %xmm3, %xmm11");
	asm("pcmpeqb %xmm4, %xmm12");
	asm("pcmpeqb %xmm5, %xmm13");
	
	asm("por     %xmm8, %xmm9");
	asm("por     %xmm10, %xmm11");
	asm("por     %xmm12, %xmm13");
	asm("por     %xmm9, %xmm11");
	asm("por     %xmm11, %xmm13");
	
	asm("pmovmskb %xmm13, %eax");
	
	asm("shrl %cl, %eax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalFindEitherOf6DoMainLoop");
	asm("addq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalFindEitherOf6Fail");
	asm("retq");
	
	NOP21;
	
	asm("LInternalFindEitherOf6DoMainLoop:");
	asm("addq $16, %rsi");					// Advance to next block
											// Want: rdx = end, rdx+rsi = current
											// -> rsi = current - end
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalFindEitherOf6Fail");
	
	asm(".balign 32");
	asm("LInternalFindEitherOf6MainLoop:");
	asm("movdqa (%rdx, %rsi), %xmm8");
	asm("movdqa (%rdx, %rsi), %xmm9");
	asm("movdqa (%rdx, %rsi), %xmm10");
	asm("movdqa (%rdx, %rsi), %xmm11");
	asm("movdqa (%rdx, %rsi), %xmm12");
	asm("movdqa (%rdx, %rsi), %xmm13");
	asm("prefetchnta 128(%rdx, %rsi)");
	
	asm("pcmpeqb %xmm0, %xmm8");
	asm("pcmpeqb %xmm1, %xmm9");
	asm("pcmpeqb %xmm2, %xmm10");
	asm("pcmpeqb %xmm3, %xmm11");
	asm("pcmpeqb %xmm4, %xmm12");
	asm("pcmpeqb %xmm5, %xmm13");
	
	asm("por     %xmm8, %xmm9");
	asm("por     %xmm10, %xmm11");
	asm("por     %xmm12, %xmm13");
	asm("por     %xmm9, %xmm11");
	asm("por     %xmm11, %xmm13");
	
	asm("pmovmskb %xmm13, %eax");
	
	asm("test %eax, %eax");
	asm("jne LInternalFindEitherOf6Found");
	asm("addq $16, %rsi");
	asm("jnc LInternalFindEitherOf6MainLoop");
	
	asm("LInternalFindEitherOf6Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalFindEitherOf6Found:");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalFindEitherOf6Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalFindEitherOf7()
{
	asm("pushq %rcx");
	asm("movl %esi, %ecx");
	asm("andq $-16, %rsi");
	asm("andl $15, %ecx");
	
	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm1, %xmm1");
	asm("pshufd $0, %xmm2, %xmm2");
	asm("pshufd $0, %xmm3, %xmm3");
	asm("pshufd $0, %xmm4, %xmm4");
	asm("pshufd $0, %xmm5, %xmm5");
	asm("pshufd $0, %xmm6, %xmm6");

	asm("movdqa (%rsi), %xmm8");
	asm("movdqa (%rsi), %xmm9");
	asm("movdqa (%rsi), %xmm10");
	asm("movdqa (%rsi), %xmm11");
	asm("movdqa (%rsi), %xmm12");
	asm("movdqa (%rsi), %xmm13");
	asm("movdqa (%rsi), %xmm14");
	asm("prefetchnta 128(%rsi)");
	
	asm("pcmpeqb %xmm0, %xmm8");
	asm("pcmpeqb %xmm1, %xmm9");
	asm("pcmpeqb %xmm2, %xmm10");
	asm("pcmpeqb %xmm3, %xmm11");
	asm("pcmpeqb %xmm4, %xmm12");
	asm("pcmpeqb %xmm5, %xmm13");
	asm("pcmpeqb %xmm6, %xmm14");
	
	asm("por     %xmm8, %xmm9");
	asm("por     %xmm10, %xmm11");
	asm("por     %xmm12, %xmm13");
	asm("por     %xmm14, %xmm9");
	asm("por     %xmm11, %xmm13");
	asm("por     %xmm9, %xmm13");
	
	asm("pmovmskb %xmm13, %eax");
	
	asm("shrl %cl, %eax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalFindEitherOf7DoMainLoop");
	asm("addq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalFindEitherOf7Fail");
	asm("retq");
	
	asm(".balign 32");
	asm(".byte 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90");
	
	asm("LInternalFindEitherOf7DoMainLoop:");
	asm("addq $16, %rsi");					// Advance to next block
											// Want: rdx = end, rdx+rsi = current
											// -> rsi = current - end
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalFindEitherOf7Fail");
	
	asm(".balign 32");
	asm("LInternalFindEitherOf7MainLoop:");
	asm("movdqa (%rdx, %rsi), %xmm8");
	asm("movdqa (%rdx, %rsi), %xmm9");
	asm("movdqa (%rdx, %rsi), %xmm10");
	asm("movdqa (%rdx, %rsi), %xmm11");
	asm("movdqa (%rdx, %rsi), %xmm12");
	asm("movdqa (%rdx, %rsi), %xmm13");
	asm("movdqa (%rdx, %rsi), %xmm14");
	asm("prefetchnta 128(%rdx, %rsi)");
	
	asm("pcmpeqb %xmm0, %xmm8");
	asm("pcmpeqb %xmm1, %xmm9");
	asm("pcmpeqb %xmm2, %xmm10");
	asm("pcmpeqb %xmm3, %xmm11");
	asm("pcmpeqb %xmm4, %xmm12");
	asm("pcmpeqb %xmm5, %xmm13");
	asm("pcmpeqb %xmm6, %xmm14");
	
	asm("por     %xmm8, %xmm9");
	asm("por     %xmm10, %xmm11");
	asm("por     %xmm12, %xmm13");
	asm("por     %xmm14, %xmm9");
	asm("por     %xmm11, %xmm13");
	asm("por     %xmm9, %xmm13");
	
	asm("pmovmskb %xmm13, %eax");
	
	asm("test %eax, %eax");
	asm("jne LInternalFindEitherOf7Found");
	asm("addq $16, %rsi");
	asm("jnc LInternalFindEitherOf7MainLoop");
	
	asm("LInternalFindEitherOf7Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalFindEitherOf7Found:");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalFindEitherOf7Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalFindEitherOf8()
{
	asm("pushq %rcx");
	asm("movl %esi, %ecx");
	asm("andq $-16, %rsi");
	asm("andl $15, %ecx");
	
	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm1, %xmm1");
	asm("pshufd $0, %xmm2, %xmm2");
	asm("pshufd $0, %xmm3, %xmm3");
	asm("pshufd $0, %xmm4, %xmm4");
	asm("pshufd $0, %xmm5, %xmm5");
	asm("pshufd $0, %xmm6, %xmm6");
	asm("pshufd $0, %xmm7, %xmm7");
	
	asm("movdqa (%rsi), %xmm8");
	asm("movdqa (%rsi), %xmm9");
	asm("movdqa (%rsi), %xmm10");
	asm("movdqa (%rsi), %xmm11");
	asm("movdqa (%rsi), %xmm12");
	asm("movdqa (%rsi), %xmm13");
	asm("movdqa (%rsi), %xmm14");
	asm("movdqa (%rsi), %xmm15");
	asm("prefetchnta 128(%rsi)");
	
	asm("pcmpeqb %xmm0, %xmm8");
	asm("pcmpeqb %xmm1, %xmm9");
	asm("pcmpeqb %xmm2, %xmm10");
	asm("pcmpeqb %xmm3, %xmm11");
	asm("pcmpeqb %xmm4, %xmm12");
	asm("pcmpeqb %xmm5, %xmm13");
	asm("pcmpeqb %xmm6, %xmm14");
	asm("pcmpeqb %xmm7, %xmm15");
	
	asm("por     %xmm8, %xmm9");
	asm("por     %xmm10, %xmm11");
	asm("por     %xmm12, %xmm13");
	asm("por     %xmm14, %xmm15");
	asm("por     %xmm9, %xmm11");
	asm("por     %xmm13, %xmm15");
	asm("por     %xmm11, %xmm15");
	
	asm("pmovmskb %xmm15, %eax");
	
	asm("shrl %cl, %eax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalFindEitherOf8DoMainLoop");
	asm("addq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalFindEitherOf8Fail");
	asm("retq");
	
	NOP9;
	
	asm("LInternalFindEitherOf8DoMainLoop:");
	asm("addq $16, %rsi");					// Advance to next block
											// Want: rdx = end, rdx+rsi = current
											// -> rsi = current - end
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalFindEitherOf8Fail");
	
	asm(".balign 32");
	asm("LInternalFindEitherOf8MainLoop:");
	asm("movdqa (%rdx, %rsi), %xmm8");
	asm("movdqa (%rdx, %rsi), %xmm9");
	asm("movdqa (%rdx, %rsi), %xmm10");
	asm("movdqa (%rdx, %rsi), %xmm11");
	asm("movdqa (%rdx, %rsi), %xmm12");
	asm("movdqa (%rdx, %rsi), %xmm13");
	asm("movdqa (%rdx, %rsi), %xmm14");
	asm("movdqa (%rdx, %rsi), %xmm15");
	asm("prefetchnta 128(%rdx, %rsi)");

	asm("pcmpeqb %xmm0, %xmm8");
	asm("pcmpeqb %xmm1, %xmm9");
	asm("pcmpeqb %xmm2, %xmm10");
	asm("pcmpeqb %xmm3, %xmm11");
	asm("pcmpeqb %xmm4, %xmm12");
	asm("pcmpeqb %xmm5, %xmm13");
	asm("pcmpeqb %xmm6, %xmm14");
	asm("pcmpeqb %xmm7, %xmm15");
	
	asm("por     %xmm8, %xmm9");
	asm("por     %xmm10, %xmm11");
	asm("por     %xmm12, %xmm13");
	asm("por     %xmm14, %xmm15");
	asm("por     %xmm9, %xmm11");
	asm("por     %xmm13, %xmm15");
	asm("por     %xmm11, %xmm15");
	
	asm("pmovmskb %xmm15, %eax");
	
	asm("test %eax, %eax");
	asm("jne LInternalFindEitherOf8Found");
	asm("addq $16, %rsi");
	asm("jnc LInternalFindEitherOf8MainLoop");
	
	asm("LInternalFindEitherOf8Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalFindEitherOf8Found:");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalFindEitherOf8Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}


// %xmm0 must be loaded with the first byte to search in the lower 32 bits
// %xmm1 must be loaded with the second byte to search in the lower 32 bits
// Returns a pointer to the SECOND byte
__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalFindPair()
{
	asm("pushq %rcx");
	asm("movl %esi, %ecx");
	asm("andq $-32, %rsi");
	asm("andl $31, %ecx");

	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm1, %xmm1");

	asm("movdqa (%rsi), %xmm2");
	asm("movdqa (%rsi), %xmm3");
	asm("movdqa 16(%rsi), %xmm4");
	asm("movdqa 16(%rsi), %xmm5");
	asm("prefetchnta 256(%rsi)");
	asm("pcmpeqb %xmm0, %xmm2");
	asm("pcmpeqb %xmm1, %xmm3");
	asm("pcmpeqb %xmm0, %xmm4");
	asm("pcmpeqb %xmm1, %xmm5");
	
	asm("pmovmskb %xmm2, %r8d");
	asm("pmovmskb %xmm3, %r9d");
	asm("pmovmskb %xmm4, %r10d");
	asm("pmovmskb %xmm5, %r11d");

	asm("shlq $17, %r10");
	asm("shll $16, %r11d");
	asm("addl $1, %ecx");
	asm("orl  %r11d, %r9d");		// %r9d now contains bit mask of second search byte
	asm("lea  (%r10,%r8,2), %rax");	// %eax now contains bit mask of first search byte shift left 1
	
	asm("andl %eax, %r9d");			// Combine the bytes.
	asm("shrq %cl, %r9");
	asm("bsfl %r9d, %eax");
	asm("jz LInternalFindPairDoMainLoop");
	asm("addq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalFindPairFail");
	asm("retq");
	
	asm(".balign 32");
	asm(".byte 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90");
	
	asm("LInternalFindPairDoMainLoop:");
	asm("addq $32, %rsi");				// Advance to next block
										// Want: rdx = end, rdx+rsi = current
										// -> rsi = current - end
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalFindPairFail");
	
	asm(".balign 32");
	asm("LInternalFindPairMainLoop:");
	asm("movdqa (%rdx, %rsi), %xmm2");
	asm("movdqa (%rdx, %rsi), %xmm3");
	asm("movdqa 16(%rdx, %rsi), %xmm4");
	asm("movdqa 16(%rdx, %rsi), %xmm5");
	asm("prefetchnta 256(%rdx, %rsi)");

	asm("shrq $32, %rax");
	
	asm("pcmpeqb %xmm0, %xmm2");
	asm("pcmpeqb %xmm1, %xmm3");
	asm("pcmpeqb %xmm0, %xmm4");
	asm("pcmpeqb %xmm1, %xmm5");

	asm("pmovmskb %xmm2, %r8d");
	asm("pmovmskb %xmm3, %r9d");
	asm("pmovmskb %xmm4, %r10d");
	asm("pmovmskb %xmm5, %r11d");

	asm("lea (%rax,%r8,2), %rax");
	asm("shlq $17, %r10");
	asm("shll $16, %r11d");
	asm("addq %r10, %rax");
	asm("orl  %r11d, %r9d");			// %r9d now contains bit mask of second search byte
	
	asm("and %eax, %r9d");
	asm("jne LInternalFindPairFound");
	asm("addq $32, %rsi");
	asm("jnc LInternalFindPairMainLoop");
	
	asm("LInternalFindPairFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalFindPairFound:");
	asm("bsfl %r9d, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalFindPairFail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvxFindPair()
{
	asm("pushq %rcx");
	asm("movl %esi, %ecx");
	asm("andq $-32, %rsi");
	asm("andl $31, %ecx");
	
	asm("vpshufd $0, %xmm0, %xmm0");
	asm("vpshufd $0, %xmm1, %xmm1");
	
	asm("vmovdqa (%rsi), %xmm2");
	asm("vmovdqa 16(%rsi), %xmm3");
	asm("prefetchnta 512(%rsi)");
	
	asm("vpcmpeqb %xmm0, %xmm2, %xmm4");
	asm("vpcmpeqb %xmm1, %xmm2, %xmm5");
	asm("vpcmpeqb %xmm0, %xmm3, %xmm12");
	asm("vpcmpeqb %xmm1, %xmm3, %xmm7");
	
	asm("vpslldq $1, %xmm4, %xmm10");
	asm("vpalignr $15, %xmm4, %xmm12, %xmm11");
	asm("vpand %xmm5, %xmm10, %xmm5");
	asm("vpand %xmm7, %xmm11, %xmm7");
	
	asm("vpmovmskb %xmm5, %eax");
	asm("vpmovmskb %xmm7, %r10d");
	
	asm("shlq $16, %r10");
	asm("addl $1, %ecx");
	asm("orl %r10d, %eax");
	
	asm("shrq %cl, %rax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalAvxFindPairDoMainLoop");
	asm("addq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvxFindPairFail");
	asm("retq");
		
	asm("LInternalAvxFindPairFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvxFindPairDoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $32, %rsi");
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalAvxFindPairFail");
	
	asm("xor %eax, %eax");
	
	asm(".balign 32");
	asm("LInternalAvxFindPairMainLoop:");
	asm("vmovdqa 32(%r10), %xmm2");
	asm("vmovdqa 48(%r10), %xmm3");
	asm("prefetchnta 768(%r10)");
	asm("addq $32, %r10");
	
	asm("test %eax, %eax");
	asm("jnz LInternalAvxFindPairFound");
	
	asm("vpcmpeqb %xmm0, %xmm2, %xmm4");
	asm("vpcmpeqb %xmm0, %xmm3, %xmm6");
	asm("vpalignr $15, %xmm12, %xmm4, %xmm10");
	asm("vpalignr $15, %xmm4, %xmm6, %xmm11");
	asm("vpcmpeqb %xmm1, %xmm2, %xmm5");
	asm("vpcmpeqb %xmm1, %xmm3, %xmm7");
	
	asm("vpand %xmm5, %xmm10, %xmm5");
	asm("vpand %xmm7, %xmm11, %xmm7");
	
	asm("vpor %xmm5, %xmm7, %xmm7");
	asm("vmovdqa %xmm6, %xmm12");
	
	asm("vpmovmskb %xmm7, %eax");
	asm("addq $32, %rsi");
	asm("jnc LInternalAvxFindPairMainLoop");
	
	asm("LInternalAvxFindPairLastLoop:");
	asm("test %eax, %eax");
	asm("jz LInternalAvxFindPairExit");
	
	asm("LInternalAvxFindPairFound:");
	asm("vpmovmskb %xmm5, %r10d");
	asm("shll $16, %eax");
	asm("subq $32, %rsi");
	asm("orl %r10d, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvxFindPairFail");
	asm("addq %rdx, %rax");
	asm("LInternalAvxFindPairExit:");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx2FindPair()
{
	asm("pushq %rcx");
	asm("movq %rsi, %rcx");
	asm("andq $-64, %rsi");
	
	asm("vmovdqa (%rsi), %ymm2");
	asm("vmovdqa 32(%rsi), %ymm3");
	asm("prefetchnta 1536(%rsi)");

	asm("vpbroadcastb %xmm0, %ymm0");
	asm("vpbroadcastb %xmm1, %ymm1");

	asm("vpcmpeqb %ymm0, %ymm2, %ymm4");
	asm("vpcmpeqb %ymm1, %ymm2, %ymm5");
	asm("vpcmpeqb %ymm0, %ymm3, %ymm12");
	asm("vpcmpeqb %ymm1, %ymm3, %ymm7");

	asm("vperm2i128 $0x8, %ymm4, %ymm4, %ymm13");
	asm("vperm2i128 $0x3, %ymm4, %ymm12, %ymm14");
	asm("vpalignr $15, %ymm13, %ymm4, %ymm10");
	asm("vpalignr $15, %ymm14, %ymm12, %ymm11");
	asm("vpand %ymm5, %ymm10, %ymm5");
	asm("vpand %ymm7, %ymm11, %ymm7");
	
	asm("vpmovmskb %ymm5, %eax");
	asm("vpmovmskb %ymm7, %r10d");
	
	asm("shlq $31, %r10");
	asm("shrl $1, %eax");
	asm("orq %r10, %rax");
	asm("shrq %cl, %rax");
	asm("bsfq %rax, %rax");
	asm("jz LInternalAvx2FindPairDoMainLoop");
	asm("addl $1, %eax");
	asm("addq %rcx, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvx2FindPairFail2");
	asm("retq");

	asm("LInternalAvx2FindPairFail2:");
	asm("xor %eax, %eax");
	asm("retq");
	
	NOP26;

	asm("LInternalAvx2FindPairDoMainLoop:");
	asm("addq $64, %rsi");
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalAvx2FindPairFail");
	
	asm(".balign 32");
	asm("LInternalAvx2FindPairMainLoop:");
	asm("vmovdqa (%rdx,%rsi), %ymm2");
	asm("vmovdqa 32(%rdx,%rsi), %ymm3");
	asm("prefetchnta 1536(%rdx,%rsi)");
	
	asm("vpcmpeqb %ymm0, %ymm2, %ymm4");
	asm("vpcmpeqb %ymm1, %ymm2, %ymm5");
	asm("vpcmpeqb %ymm0, %ymm3, %ymm6");
	asm("vpcmpeqb %ymm1, %ymm3, %ymm7");
	
	asm("vperm2i128 $0x3, %ymm12, %ymm4, %ymm13");
	asm("vperm2i128 $0x3, %ymm4, %ymm6, %ymm14");	
	asm("vpalignr $15, %ymm13, %ymm4, %ymm10");
	asm("vpalignr $15, %ymm14, %ymm6, %ymm11");
	asm("vpand %ymm5, %ymm10, %ymm5");
	asm("vpand %ymm7, %ymm11, %ymm7");
	
	asm("vpor %ymm5, %ymm7, %ymm7");
	asm("vmovdqa %ymm6, %ymm12");
	asm("vpmovmskb %ymm7, %eax");
	
	asm("test %eax, %eax");
	asm("jne LInternalAvx2FindPairFound");
	asm("addq $64, %rsi");
	asm("jnc LInternalAvx2FindPairMainLoop");
	
	asm("LInternalAvx2FindPairFail:");
	asm("xor %eax, %eax");
	asm("retq");

	asm(".balign 32");
	asm("LInternalAvx2FindPairFound:");
	asm("vpmovmskb %ymm5, %r10d");
	asm("shlq $32, %rax");
	asm("orq %r10, %rax");
	asm("bsfq %rax, %rax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvx2FindPairFail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx512FindPair()
{
    asm("pushq %rcx");
    asm("movq %rsi, %rcx");
    asm("andq $-64, %rsi");
    
    asm("vmovdqa64 (%rsi), %zmm2");
    asm("prefetcht0 2048(%rsi)");

    asm("vpbroadcastb %xmm0, %zmm0");
    asm("vpbroadcastb %xmm1, %zmm1");

    asm("vpcmpeqb %zmm0, %zmm2, %k1");
    asm("vpcmpeqb %zmm1, %zmm2, %k2");
    asm("kshiftlq $1, %k1, %k1");
    asm("kandq %k1, %k2, %k1");
    asm("kmovq %k1, %rax");
    
    asm("shrq %cl, %rax");
    asm("andq $-2, %rax");
    asm("jz LInternalAvx512FindPairDoMainLoop");

    asm("bsfq %rax, %rax");
    asm("addq %rcx, %rax");
    asm("popq %rcx");
    asm("cmpq %rdx, %rax");
    asm("jae LInternalAvx512FindPairFail");
    asm("retq");
    
    NOP2;
    
    asm("LInternalAvx512FindPairDoMainLoop:");
    asm("popq %rcx");
    asm("addq $64, %rsi");
    asm("cmp %rdx, %rsi");
    asm("jae LInternalAvx512FindPairFail");
    
    asm(".balign 32");
    asm("LInternalAvx512FindPairMainLoop:");
    asm("vmovdqa64 (%rsi), %zmm2");
    asm("vmovdqu64 -1(%rsi), %zmm3");
    asm("prefetcht0 2048(%rsi)");
    asm("addq $64, %rsi");

    asm("vpcmpeqb %zmm1, %zmm2, %k2");
    asm("vpcmpeqb %zmm0, %zmm3, %k1");

    asm("ktestq %k1, %k2");
    asm("jnz LInternalAvx512FindPairFound");
    asm("cmpq %rdx, %rsi");
    asm("jb LInternalAvx512FindPairMainLoop");
    
    asm("LInternalAvx512FindPairFail:");
    asm("xor %eax, %eax");
    asm("retq");
    
    asm("LInternalAvx512FindPairFound:");
    asm("kandq %k1, %k2, %k1");
    asm("kmovq %k1, %rax");
    asm("bsfq %rax, %rax");
    asm("leaq -64(%rsi,%rax), %rax");
    asm("cmpq %rdx, %rax");
    asm("jae LInternalAvx512FindPairFail");
    asm("retq");
}

// Returns a pointer to the SECOND byte
__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalFindPair2()
{
	asm("pushq %rcx");
	asm("movl %esi, %ecx");
	asm("andq $-32, %rsi");
	asm("andl $31, %ecx");

	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm1, %xmm1");
	asm("pshufd $0, %xmm2, %xmm2");
	asm("pshufd $0, %xmm3, %xmm3");
	
	asm("movdqa (%rsi), %xmm8");
	asm("movdqa (%rsi), %xmm9");
	asm("movdqa (%rsi), %xmm10");
	asm("movdqa (%rsi), %xmm11");
	asm("movdqa 16(%rsi), %xmm12");
	asm("movdqa 16(%rsi), %xmm13");
	asm("movdqa 16(%rsi), %xmm14");
	asm("movdqa 16(%rsi), %xmm15");
	asm("prefetchnta 256(%rsi)");
	asm("pcmpeqb %xmm0, %xmm8");
	asm("pcmpeqb %xmm1, %xmm9");
	asm("pcmpeqb %xmm2, %xmm10");
	asm("pcmpeqb %xmm3, %xmm11");
	asm("pcmpeqb %xmm0, %xmm12");
	asm("pcmpeqb %xmm1, %xmm13");
	asm("pcmpeqb %xmm2, %xmm14");
	asm("pcmpeqb %xmm3, %xmm15");
	asm("por     %xmm9, %xmm8");
	asm("por     %xmm11, %xmm10");
	asm("por     %xmm13, %xmm12");
	asm("por     %xmm15, %xmm14");
	
	asm("pmovmskb %xmm8, %r8d");
	asm("pmovmskb %xmm10, %r9d");
	asm("pmovmskb %xmm12, %r10d");
	asm("pmovmskb %xmm14, %r11d");
	
	asm("shlq $17, %r10");
	asm("shll $16, %r11d");
	asm("addl $1, %ecx");
	asm("orl  %r11d, %r9d");		// %r9d now contains bit mask of second search byte
	asm("lea  (%r10,%r8,2), %rax");	// %eax now contains bit mask of first search byte shift left 1
	
	asm("andl %eax, %r9d");			// Combine the bytes.
	asm("shrq %cl, %r9");
	asm("bsfl %r9d, %eax");
	asm("jz LInternalFindPair2DoMainLoop");
	asm("addq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalFindPair2Fail");
	asm("retq");
	
	asm(".balign 32");
	asm(".byte 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90");
	
	asm("LInternalFindPair2DoMainLoop:");
	asm("addq $32, %rsi");				// Advance to next block
										// Want: rdx = end, rdx+rsi = current
										// -> rsi = current - end
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalFindPair2Fail");
	
	asm(".balign 32");
	asm("LInternalFindPair2MainLoop:");
	asm("movdqa (%rdx, %rsi), %xmm8");
	asm("movdqa (%rdx, %rsi), %xmm9");
	asm("movdqa (%rdx, %rsi), %xmm10");
	asm("movdqa (%rdx, %rsi), %xmm11");
	asm("movdqa 16(%rdx, %rsi), %xmm12");
	asm("movdqa 16(%rdx, %rsi), %xmm13");
	asm("movdqa 16(%rdx, %rsi), %xmm14");
	asm("movdqa 16(%rdx, %rsi), %xmm15");
	asm("prefetchnta 256(%rdx, %rsi)");

	asm("shrq $32, %rax");
	
	asm("pcmpeqb %xmm0, %xmm8");
	asm("pcmpeqb %xmm1, %xmm9");
	asm("pcmpeqb %xmm2, %xmm10");
	asm("pcmpeqb %xmm3, %xmm11");
	asm("pcmpeqb %xmm0, %xmm12");
	asm("pcmpeqb %xmm1, %xmm13");
	asm("pcmpeqb %xmm2, %xmm14");
	asm("pcmpeqb %xmm3, %xmm15");

	asm("por     %xmm9, %xmm8");
	asm("por     %xmm11, %xmm10");
	asm("por     %xmm13, %xmm12");
	asm("por     %xmm15, %xmm14");
	
	asm("pmovmskb %xmm8, %r8d");
	asm("pmovmskb %xmm10, %r9d");
	asm("pmovmskb %xmm12, %r10d");
	asm("pmovmskb %xmm14, %r11d");
	
	asm("lea (%rax,%r8,2), %rax");
	asm("shlq $17, %r10");
	asm("shll $16, %r11d");
	asm("addq %r10, %rax");
	asm("orl  %r11d, %r9d");			// %r10d now contains bit mask of second search byte
	
	asm("and %eax, %r9d");
	asm("jne LInternalFindPair2Found");
	asm("addq $32, %rsi");
	asm("jnc LInternalFindPair2MainLoop");
	
	asm("LInternalFindPair2Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalFindPair2Found:");
	asm("bsfl %r9d, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalFindPair2Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvxFindPair2()
{
	asm("pushq %rcx");
	asm("movl %esi, %ecx");
	asm("andq $-32, %rsi");
	asm("andl $31, %ecx");
	
	asm("vpshufd $0, %xmm0, %xmm0");
	asm("vpshufd $0, %xmm1, %xmm1");
	asm("vpshufd $0, %xmm2, %xmm2");
	asm("vpshufd $0, %xmm3, %xmm3");
	
	asm("vmovdqa (%rsi), %xmm4");
	asm("vmovdqa 16(%rsi), %xmm5");
	asm("prefetchnta 512(%rsi)");
	
	asm("vpcmpeqb %xmm0, %xmm4, %xmm6");
	asm("vpcmpeqb %xmm1, %xmm4, %xmm7");
	asm("vpcmpeqb %xmm2, %xmm4, %xmm8");
	asm("vpcmpeqb %xmm3, %xmm4, %xmm9");
	asm("vpcmpeqb %xmm0, %xmm5, %xmm10");
	asm("vpcmpeqb %xmm1, %xmm5, %xmm11");
	asm("vpcmpeqb %xmm2, %xmm5, %xmm12");
	asm("vpcmpeqb %xmm3, %xmm5, %xmm13");
	
	asm("vpor %xmm6, %xmm7, %xmm6");
	asm("vpor %xmm8, %xmm9, %xmm8");
	asm("vpor %xmm10, %xmm11, %xmm14");
	asm("vpor %xmm12, %xmm13, %xmm12");
	
	asm("vpslldq $1, %xmm6, %xmm4");
	asm("vpalignr $15, %xmm6, %xmm14, %xmm5");
	asm("vpand %xmm4, %xmm8, %xmm4");
	asm("vpand %xmm5, %xmm12, %xmm5");
	
	asm("vpmovmskb %xmm4, %eax");
	asm("vpmovmskb %xmm5, %r10d");
	
	asm("shlq $16, %r10");
	asm("addl $1, %ecx");
	asm("orl %r10d, %eax");
	
	asm("shrq %cl, %rax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalAvxFindPair2DoMainLoop");
	asm("addq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvxFindPair2Fail");
	asm("retq");
	
	asm(".balign 32");
	asm(".byte 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90");
	
	asm("LInternalAvxFindPair2DoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $32, %rsi");				// Advance to next block
										// Want: rdx = end, rdx+rsi = current
										// -> rsi = current - end
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalAvxFindPair2Fail");
	
	asm(".balign 32");
	asm("LInternalAvxFindPair2MainLoop:");
	asm("vmovdqa 32(%r10), %xmm4");
	asm("vmovdqa 48(%r10), %xmm5");
	asm("prefetchnta 544(%r10)");
	asm("addq $32, %r10");
	
	asm("vpcmpeqb %xmm0, %xmm4, %xmm6");
	asm("vpcmpeqb %xmm1, %xmm4, %xmm7");
	asm("vpcmpeqb %xmm2, %xmm4, %xmm8");
	asm("vpcmpeqb %xmm3, %xmm4, %xmm9");
	asm("vpcmpeqb %xmm0, %xmm5, %xmm10");
	asm("vpcmpeqb %xmm1, %xmm5, %xmm11");
	asm("vpcmpeqb %xmm2, %xmm5, %xmm12");
	asm("vpcmpeqb %xmm3, %xmm5, %xmm13");
	
	asm("vpor %xmm6, %xmm7, %xmm6");
	asm("vpor %xmm8, %xmm9, %xmm8");
	asm("vpor %xmm10, %xmm11, %xmm10");
	asm("vpor %xmm12, %xmm13, %xmm12");
	
	asm("vpalignr $15, %xmm14, %xmm6, %xmm4");
	asm("vpalignr $15, %xmm6, %xmm10, %xmm5");
	asm("vpand %xmm4, %xmm8, %xmm4");
	asm("vpand %xmm5, %xmm12, %xmm5");
	
	asm("vpor %xmm4, %xmm5, %xmm5");
	asm("vmovdqa %xmm10, %xmm14");
	asm("vpmovmskb %xmm5, %eax");
	
	asm("test %eax, %eax");
	asm("jne LInternalAvxFindPair2Found");
	asm("addq $32, %rsi");
	asm("jnc LInternalAvxFindPair2MainLoop");
	
	asm("LInternalAvxFindPair2Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvxFindPair2Found:");
	asm("vpmovmskb %xmm4, %r10d");
	asm("shll $16, %eax");
	asm("orl %r10d, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvxFindPair2Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvxFindPairPath2()
{
	asm("pushq %rcx");
	asm("movl %esi, %ecx");
	asm("andq $-32, %rsi");
	asm("andl $31, %ecx");
	
	asm("vpshufd $0, %xmm0, %xmm0");
	asm("vpshufd $0, %xmm1, %xmm1");
	asm("vpshufd $0, %xmm2, %xmm2");
	asm("vpshufd $0, %xmm3, %xmm3");
	
	asm("vmovdqa (%rsi), %xmm4");
	asm("vmovdqa 16(%rsi), %xmm5");
	asm("prefetchnta 512(%rsi)");
	
	asm("vpslldq $1, %xmm4, %xmm6");
	asm("vpalignr $15, %xmm4, %xmm5, %xmm7");
	
	asm("vpcmpeqb %xmm0, %xmm6, %xmm8");
	asm("vpcmpeqb %xmm1, %xmm6, %xmm9");
	asm("vpcmpeqb %xmm2, %xmm4, %xmm10");
	asm("vpcmpeqb %xmm3, %xmm4, %xmm11");
	asm("vpcmpeqb %xmm0, %xmm7, %xmm12");
	asm("vpcmpeqb %xmm1, %xmm7, %xmm13");
	asm("vpcmpeqb %xmm2, %xmm5, %xmm14");
	asm("vpcmpeqb %xmm3, %xmm5, %xmm15");
	
	asm("vpand %xmm8, %xmm10, %xmm8");
	asm("vpand %xmm9, %xmm11, %xmm9");
	asm("vpand %xmm12, %xmm14, %xmm12");
	asm("vpand %xmm13, %xmm15, %xmm13");
	
	asm("vpor %xmm8, %xmm9, %xmm8");
	asm("vpor %xmm12, %xmm13, %xmm12");
	
	asm("vpmovmskb %xmm8, %eax");
	asm("vpmovmskb %xmm12, %r10d");
	
	asm("vmovdqa %xmm5, %xmm6");
	
	asm("shlq $16, %r10");
	asm("addl $1, %ecx");
	asm("orl %r10d, %eax");
	
	asm("shrq %cl, %rax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalAvxFindPairPath2DoMainLoop");
	asm("addq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvxFindPairPath2Fail");
	asm("retq");
	
	asm(".balign 32");
	asm(".byte 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90");
	
	asm("LInternalAvxFindPairPath2DoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $32, %rsi");
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalAvxFindPairPath2Fail");
	
	asm(".balign 32");
	asm("LInternalAvxFindPairPath2MainLoop:");
	asm("vmovdqa 32(%r10), %xmm4");
	asm("vmovdqa 48(%r10), %xmm5");
	asm("prefetchnta 512(%r10)");
	asm("addq $32, %r10");
	
	asm("vpalignr $15, %xmm6, %xmm4, %xmm6");
	asm("vpalignr $15, %xmm4, %xmm5, %xmm7");
	
	asm("vpcmpeqb %xmm0, %xmm6, %xmm8");
	asm("vpcmpeqb %xmm1, %xmm6, %xmm9");
	asm("vpcmpeqb %xmm2, %xmm4, %xmm10");
	asm("vpcmpeqb %xmm3, %xmm4, %xmm11");
	asm("vpcmpeqb %xmm0, %xmm7, %xmm12");
	asm("vpcmpeqb %xmm1, %xmm7, %xmm13");
	asm("vpcmpeqb %xmm2, %xmm5, %xmm14");
	asm("vpcmpeqb %xmm3, %xmm5, %xmm15");
	
	asm("vpand %xmm8, %xmm10, %xmm8");
	asm("vpand %xmm9, %xmm11, %xmm9");
	asm("vpand %xmm12, %xmm14, %xmm12");
	asm("vpand %xmm13, %xmm15, %xmm13");
	
	asm("vpor %xmm8, %xmm9, %xmm8");
	asm("vpor %xmm12, %xmm13, %xmm12");
	
	asm("vpor %xmm8, %xmm12, %xmm12");
	asm("vpmovmskb %xmm12, %eax");
	asm("vmovdqa %xmm5, %xmm6");
	
	asm("test %eax, %eax");
	asm("jne LInternalAvxFindPairPath2Found");
	asm("addq $32, %rsi");
	asm("jnc LInternalAvxFindPairPath2MainLoop");
	
	asm("LInternalAvxFindPairPath2Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvxFindPairPath2Found:");
	asm("vpmovmskb %xmm8, %r10d");
	asm("shll $16, %eax");
	asm("orl %r10d, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvxFindPairPath2Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx2FindPair2()
{
	asm("pushq %rcx");
	asm("movq %rsi, %rcx");
	asm("andq $-64, %rsi");
	
	asm("vmovdqa (%rsi), %ymm4");
	asm("vmovdqa 32(%rsi), %ymm5");
	asm("prefetchnta 1536(%rsi)");

	asm("vpbroadcastb %xmm0, %ymm0");
	asm("vpbroadcastb %xmm1, %ymm1");
	asm("vpbroadcastb %xmm2, %ymm2");
	asm("vpbroadcastb %xmm3, %ymm3");
	
	asm("vpcmpeqb %ymm0, %ymm4, %ymm6");
	asm("vpcmpeqb %ymm1, %ymm4, %ymm7");
	asm("vpcmpeqb %ymm2, %ymm4, %ymm8");
	asm("vpcmpeqb %ymm3, %ymm4, %ymm9");
	asm("vpcmpeqb %ymm0, %ymm5, %ymm10");
	asm("vpcmpeqb %ymm1, %ymm5, %ymm11");
	asm("vpcmpeqb %ymm2, %ymm5, %ymm12");
	asm("vpcmpeqb %ymm3, %ymm5, %ymm13");
	
	asm("vpor %ymm6, %ymm7, %ymm6");
	asm("vpor %ymm8, %ymm9, %ymm8");
	asm("vpor %ymm10, %ymm11, %ymm14");
	asm("vpor %ymm12, %ymm13, %ymm12");

	asm("vperm2i128 $0x8, %ymm6, %ymm6, %ymm7");
	asm("vperm2i128 $0x3, %ymm6, %ymm14, %ymm9");
	asm("vpalignr $15, %ymm7, %ymm6, %ymm4");
	asm("vpalignr $15, %ymm9, %ymm14, %ymm5");
	asm("vpand %ymm4, %ymm8, %ymm4");
	asm("vpand %ymm5, %ymm12, %ymm5");
	
	asm("vpmovmskb %ymm4, %eax");
	asm("vpmovmskb %ymm5, %r10d");
	
	asm("shlq $31, %r10");
	asm("shrl $1, %eax");
	asm("orq %r10, %rax");
	asm("shrq %cl, %rax");
	asm("bsfq %rax, %rax");
	asm("jz LInternalAvx2FindPair2DoMainLoop");
	asm("addl $1, %eax");
	asm("addq %rcx, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvx2FindPair2Fail");
	asm("retq");
	
	NOP6;
	NOP2;
	
	asm("LInternalAvx2FindPair2DoMainLoop:");
	asm("addq $64, %rsi");
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalAvx2FindPair2Fail");
	
	asm(".balign 32");
	asm("LInternalAvx2FindPair2MainLoop:");
	asm("vmovdqa (%rdx,%rsi), %ymm4");
	asm("vmovdqa 32(%rdx,%rsi), %ymm5");
	asm("prefetchnta 1536(%rdx,%rsi)");
	
	asm("vpcmpeqb %ymm0, %ymm4, %ymm6");
	asm("vpcmpeqb %ymm1, %ymm4, %ymm7");
	asm("vpcmpeqb %ymm2, %ymm4, %ymm8");
	asm("vpcmpeqb %ymm3, %ymm4, %ymm9");
	asm("vpcmpeqb %ymm0, %ymm5, %ymm10");
	asm("vpcmpeqb %ymm1, %ymm5, %ymm11");
	asm("vpcmpeqb %ymm2, %ymm5, %ymm12");
	asm("vpcmpeqb %ymm3, %ymm5, %ymm13");
	
	asm("vpor %ymm6, %ymm7, %ymm6");
	asm("vpor %ymm8, %ymm9, %ymm8");
	asm("vpor %ymm10, %ymm11, %ymm10");
	asm("vpor %ymm12, %ymm13, %ymm12");
	
	asm("vperm2i128 $0x3, %ymm14, %ymm6, %ymm7");
	asm("vperm2i128 $0x3, %ymm6, %ymm10, %ymm9");
	asm("vpalignr $15, %ymm7, %ymm6, %ymm4");
	asm("vpalignr $15, %ymm9, %ymm10, %ymm5");
	asm("vpand %ymm4, %ymm8, %ymm4");
	asm("vpand %ymm5, %ymm12, %ymm5");
	
	asm("vpor %ymm4, %ymm5, %ymm5");
	asm("vmovdqa %ymm10, %ymm14");
	asm("vpmovmskb %ymm5, %eax");
	
	asm("test %eax, %eax");
	asm("jne LInternalAvx2FindPair2Found");
	asm("addq $64, %rsi");
	asm("jnc LInternalAvx2FindPair2MainLoop");
	
	asm("LInternalAvx2FindPair2Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvx2FindPair2Found:");
	asm("vpmovmskb %ymm4, %r10d");
	asm("shlq $32, %rax");
	asm("orq %r10, %rax");
	asm("bsfq %rax, %rax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvx2FindPair2Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx2FindPairPath2()
{
	asm("pushq %rcx");
	asm("movl %esi, %ecx");
	asm("andq $-64, %rsi");
	asm("andl $63, %ecx");
	
	asm("vmovdqa (%rsi), %ymm4");
	asm("vmovdqa 32(%rsi), %ymm5");
	asm("prefetchnta 1536(%rsi)");
	
	asm("vpbroadcastb %xmm0, %ymm0");
	asm("vpbroadcastb %xmm1, %ymm1");
	asm("vpbroadcastb %xmm2, %ymm2");
	asm("vpbroadcastb %xmm3, %ymm3");

	asm("vperm2i128 $0x8, %ymm4, %ymm4, %ymm6");
	asm("vperm2i128 $0x3, %ymm4, %ymm5, %ymm7");
	asm("vpalignr $15, %ymm6, %ymm4, %ymm6");
	asm("vpalignr $15, %ymm7, %ymm5, %ymm7");
	
	asm("vpcmpeqb %ymm0, %ymm6, %ymm8");
	asm("vpcmpeqb %ymm1, %ymm6, %ymm9");
	asm("vpcmpeqb %ymm2, %ymm4, %ymm10");
	asm("vpcmpeqb %ymm3, %ymm4, %ymm11");
	asm("vpcmpeqb %ymm0, %ymm7, %ymm12");
	asm("vpcmpeqb %ymm1, %ymm7, %ymm13");
	asm("vpcmpeqb %ymm2, %ymm5, %ymm14");
	asm("vpcmpeqb %ymm3, %ymm5, %ymm15");
	
	asm("vpand %ymm8, %ymm10, %ymm8");
	asm("vpand %ymm9, %ymm11, %ymm9");
	asm("vpand %ymm12, %ymm14, %ymm12");
	asm("vpand %ymm13, %ymm15, %ymm13");

	asm("vpor %ymm8, %ymm9, %ymm8");
	asm("vpor %ymm12, %ymm13, %ymm12");
	
	asm("vpmovmskb %ymm8, %eax");
	asm("vpmovmskb %ymm12, %r10d");
	
	asm("vmovdqa %ymm5, %ymm6");
	
	asm("shlq $31, %r10");
	asm("shrl $1, %eax");
	asm("orq %r10, %rax");
	asm("shrq %cl, %rax");
	asm("bsfq %rax, %rax");
	asm("jz LInternalAvx2FindPairPath2DoMainLoop");
	asm("addl $1, %eax");
	asm("addq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvx2FindPairPath2Fail");
	asm("retq");
	
	asm(".balign 32");
	asm(".byte 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90");
	
	asm("LInternalAvx2FindPairPath2DoMainLoop:");
	asm("addq $64, %rsi");
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalAvx2FindPairPath2Fail");
	
	asm(".balign 32");
	asm("LInternalAvx2FindPairPath2MainLoop:");
	asm("vmovdqa (%rdx,%rsi), %ymm4");
	asm("vmovdqa 32(%rdx,%rsi), %ymm5");
	asm("prefetchnta 1536(%rdx,%rsi)");
	
	asm("vperm2i128 $0x3, %ymm6, %ymm4, %ymm6");
	asm("vperm2i128 $0x3, %ymm4, %ymm5, %ymm7");
	asm("vpalignr $15, %ymm6, %ymm4, %ymm6");
	asm("vpalignr $15, %ymm7, %ymm5, %ymm7");
	
	asm("vpcmpeqb %ymm0, %ymm6, %ymm8");
	asm("vpcmpeqb %ymm1, %ymm6, %ymm9");
	asm("vpcmpeqb %ymm2, %ymm4, %ymm10");
	asm("vpcmpeqb %ymm3, %ymm4, %ymm11");
	asm("vpcmpeqb %ymm0, %ymm7, %ymm12");
	asm("vpcmpeqb %ymm1, %ymm7, %ymm13");
	asm("vpcmpeqb %ymm2, %ymm5, %ymm14");
	asm("vpcmpeqb %ymm3, %ymm5, %ymm15");
	
	asm("vpand %ymm8, %ymm10, %ymm8");
	asm("vpand %ymm9, %ymm11, %ymm9");
	asm("vpand %ymm12, %ymm14, %ymm12");
	asm("vpand %ymm13, %ymm15, %ymm13");
	
	asm("vpor %ymm8, %ymm9, %ymm8");
	asm("vpor %ymm12, %ymm13, %ymm12");

	asm("vpor %ymm8, %ymm12, %ymm12");
	asm("vmovdqa %ymm5, %ymm6");
	asm("vpmovmskb %ymm12, %eax");
	
	asm("test %eax, %eax");
	asm("jne LInternalAvx2FindPairPath2Found");
	asm("addq $64, %rsi");
	asm("jnc LInternalAvx2FindPairPath2MainLoop");
	
	asm("LInternalAvx2FindPairPath2Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvx2FindPairPath2Found:");
	asm("vpmovmskb %ymm8, %r10d");
	asm("shlq $32, %rax");
	asm("orq %r10, %rax");
	asm("bsfq %rax, %rax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvx2FindPairPath2Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx512FindPair2()
{
    asm("pushq %rcx");
    asm("movq %rsi, %rcx");
    asm("andq $-64, %rsi");
    
    asm("vmovdqa64 (%rsi), %zmm4");
    asm("prefetcht0 2048(%rsi)");

    asm("vpbroadcastb %xmm0, %zmm0");
    asm("vpbroadcastb %xmm1, %zmm1");
    asm("vpbroadcastb %xmm2, %zmm2");
    asm("vpbroadcastb %xmm3, %zmm3");

    asm("vpcmpeqb %zmm0, %zmm4, %k1");
    asm("vpcmpeqb %zmm1, %zmm4, %k2");
    asm("vpcmpeqb %zmm2, %zmm4, %k3");
    asm("vpcmpeqb %zmm3, %zmm4, %k4");

    asm("korq %k1, %k2, %k1");
    asm("korq %k3, %k4, %k3");
    asm("kshiftlq $1, %k1, %k1");

    asm("kandq %k1, %k3, %k1");
    asm("kmovq %k1, %rax");
    
    asm("shrq %cl, %rax");
    asm("andq $-2, %rax");
    asm("jz LInternalAvx512FindPair2DoMainLoop");

    asm("bsfq %rax, %rax");
    asm("addq %rcx, %rax");
    asm("popq %rcx");
    asm("cmpq %rdx, %rax");
    asm("jae LInternalAvx512FindPair2Fail");
    asm("retq");
    
    asm("LInternalAvx512FindPair2DoMainLoop:");
    asm("popq %rcx");
    asm("addq $64, %rsi");
    asm("cmp %rdx, %rsi");
    asm("jae LInternalAvx512FindPair2Fail");

    asm(".balign 32");
    asm("LInternalAvx512FindPair2MainLoop:");
    asm("vmovdqa64 (%rsi), %zmm4");
    asm("vmovdqu64 -1(%rsi), %zmm5");
    asm("prefetcht0 2048(%rsi)");
    asm("addq $64, %rsi");

    asm("vpcmpeqb %zmm0, %zmm5, %k1");
    asm("vpcmpeqb %zmm1, %zmm5, %k2");
    asm("vpcmpeqb %zmm2, %zmm4, %k3");
    asm("vpcmpeqb %zmm3, %zmm4, %k4");

    asm("korq %k1, %k2, %k1");
    asm("korq %k3, %k4, %k3");

    asm("ktestq %k1, %k3");
    asm("jnz LInternalAvx512FindPair2Found");
    
    asm("cmpq %rdx, %rsi");
    asm("jb LInternalAvx512FindPair2MainLoop");

    asm("LInternalAvx512FindPair2Fail:");
    asm("xor %eax, %eax");
    asm("retq");

    asm("LInternalAvx512FindPair2Found:");
    asm("kandq %k1, %k3, %k1");
    asm("kmovq %k1, %rax");
    asm("bsfq %rax, %rax");
    asm("leaq -64(%rsi,%rax), %rax");
    asm("cmpq %rdx, %rax");
    asm("jae LInternalAvx512FindPair2Fail");
    asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx512FindPairPath2()
{
    asm("pushq %rcx");
    asm("movq %rsi, %rcx");
    asm("andq $-64, %rsi");
    
    asm("vmovdqa64 (%rsi), %zmm4");
    asm("prefetcht0 2048(%rsi)");

    asm("vpbroadcastb %xmm0, %zmm0");
    asm("vpbroadcastb %xmm1, %zmm1");
    asm("vpbroadcastb %xmm2, %zmm2");
    asm("vpbroadcastb %xmm3, %zmm3");
    asm("vshufi32x4 $0x90, %zmm4, %zmm4, %zmm5");
    asm("vpalignr $15, %zmm5, %zmm4, %zmm5");

    asm("vpcmpeqb %zmm0, %zmm5, %k1");
    asm("vpcmpeqb %zmm1, %zmm5, %k2");
    asm("vpcmpeqb %zmm2, %zmm4, %k3");
    asm("vpcmpeqb %zmm3, %zmm4, %k4");

    asm("kandq %k1, %k3, %k1");
    asm("kandq %k2, %k4, %k2");

    asm("korq %k1, %k2, %k1");
    asm("kmovq %k1, %rax");
    
    asm("shrq %cl, %rax");
    asm("andq $-2, %rax");
    asm("jz LInternalAvx512FindPairPath2DoMainLoop");

    asm("bsfq %rax, %rax");
    asm("addq %rcx, %rax");
    asm("popq %rcx");
    asm("cmpq %rdx, %rax");
    asm("jae LInternalAvx512FindPairPath2Fail");
    asm("retq");
    
    NOP21;
    
    asm("LInternalAvx512FindPairPath2DoMainLoop:");
    asm("movq %rsi, %r10");
    asm("addq $64, %rsi");
    asm("subq %rdx, %rsi");
    asm("popq %rcx");
    asm("jnc LInternalAvx512FindPairPath2Fail");
    
    asm(".balign 32");
    asm("LInternalAvx512FindPairPath2MainLoop:");
    asm("vmovdqa64 64(%r10), %zmm4");
    asm("vmovdqu64 63(%r10), %zmm5");
    asm("prefetcht0 2048(%r10)");
    asm("addq $64, %r10");

    asm("vpcmpeqb %zmm0, %zmm5, %k1");
    asm("vpcmpeqb %zmm1, %zmm5, %k2");
    asm("vpcmpeqb %zmm2, %zmm4, %k3");
    asm("vpcmpeqb %zmm3, %zmm4, %k4");

    asm("kandq %k1, %k3, %k1");
    asm("kandq %k2, %k4, %k2");

    asm("kortestq %k1, %k2");
    asm("jnz LInternalAvx512FindPairPath2Found");
    asm("addq $64, %rsi");
    asm("jnc LInternalAvx512FindPairPath2MainLoop");
    
    asm("LInternalAvx512FindPairPath2Fail:");
    asm("xor %eax, %eax");
    asm("retq");
    
    asm("LInternalAvx512FindPairPath2Found:");
    asm("korq %k1, %k2, %k1");
    asm("kmovq %k1, %rax");
    asm("bsfq %rax, %rax");
    asm("addq %rsi, %rax");
    asm("jc LInternalAvx512FindPairPath2Fail");
    asm("addq %rdx, %rax");
    asm("retq");
}

// Returns a pointer to the SECOND byte
__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalFindPair3()
{
	asm("pushq %rcx");
	asm("movl %esi, %ecx");
	asm("andq $-16, %rsi");
	asm("andl $15, %ecx");
	
	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm1, %xmm1");
	asm("pshufd $0, %xmm2, %xmm2");
	asm("pshufd $0, %xmm3, %xmm3");
	asm("pshufd $0, %xmm4, %xmm4");
	asm("pshufd $0, %xmm5, %xmm5");

	asm("movdqa (%rsi), %xmm8");
	asm("movdqa (%rsi), %xmm9");
	asm("movdqa (%rsi), %xmm10");
	asm("movdqa (%rsi), %xmm11");
	asm("movdqa (%rsi), %xmm12");
	asm("movdqa (%rsi), %xmm13");
	asm("prefetchnta 128(%rsi)");

	asm("pcmpeqb %xmm0, %xmm8");
	asm("pcmpeqb %xmm1, %xmm9");
	asm("pcmpeqb %xmm2, %xmm10");
	asm("pcmpeqb %xmm3, %xmm11");
	asm("pcmpeqb %xmm4, %xmm12");
	asm("pcmpeqb %xmm5, %xmm13");

	asm("por     %xmm9, %xmm8");
	asm("por     %xmm12, %xmm11");
	asm("por     %xmm10, %xmm8");
	asm("por     %xmm13, %xmm11");
	
	asm("pmovmskb %xmm8, %r8d");
	asm("pmovmskb %xmm11, %r10d");
	
	asm("addl $1, %ecx");
	asm("lea  (%r8,%r8), %rax");		// %eax now contains bit mask of first search byte shift left 1
	
	asm("andl %eax, %r10d");			// Combine the bytes.
	asm("shrl %cl, %r10d");
	asm("bsfl %r10d, %eax");
	asm("jz LInternalFindPair3DoMainLoop");
	asm("addq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalFindPair3Fail");
	asm("retq");
	
	asm(".balign 32");
	asm(".byte 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90");
	
	asm("LInternalFindPair3DoMainLoop:");
	asm("addq $16, %rsi");				// Advance to next block
										// Want: rdx = end, rdx+rsi = current
										// -> rsi = current - end
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalFindPair3Fail");
	
	asm(".balign 32");
	asm("LInternalFindPair3MainLoop:");
	asm("movdqa (%rdx, %rsi), %xmm8");
	asm("movdqa (%rdx, %rsi), %xmm9");
	asm("movdqa (%rdx, %rsi), %xmm10");
	asm("movdqa (%rdx, %rsi), %xmm11");
	asm("movdqa (%rdx, %rsi), %xmm12");
	asm("movdqa (%rdx, %rsi), %xmm13");
	asm("prefetchnta 128(%rdx, %rsi)");

	asm("shrl $16, %eax");
	
	asm("pcmpeqb %xmm0, %xmm8");
	asm("pcmpeqb %xmm1, %xmm9");
	asm("pcmpeqb %xmm2, %xmm10");
	asm("pcmpeqb %xmm3, %xmm11");
	asm("pcmpeqb %xmm4, %xmm12");
	asm("pcmpeqb %xmm5, %xmm13");
	
	asm("por     %xmm9, %xmm8");
	asm("por     %xmm12, %xmm11");
	asm("por     %xmm10, %xmm8");
	asm("por     %xmm13, %xmm11");
	
	asm("pmovmskb %xmm8, %r8d");
	asm("pmovmskb %xmm11, %r10d");
	
	asm("lea (%rax,%r8,2), %rax");
	
	asm("and %eax, %r10d");
	asm("jne LInternalFindPair3Found");
	asm("addq $16, %rsi");
	asm("jnc LInternalFindPair3MainLoop");
	
	asm("LInternalFindPair3Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalFindPair3Found:");
	asm("bsfl %r10d, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalFindPair3Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

// Returns a pointer to the SECOND byte
__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalFindPair4()
{
	asm("pushq %rcx");
	asm("movl %esi, %ecx");
	asm("andq $-16, %rsi");
	asm("andl $15, %ecx");

	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm1, %xmm1");
	asm("pshufd $0, %xmm2, %xmm2");
	asm("pshufd $0, %xmm3, %xmm3");
	asm("pshufd $0, %xmm4, %xmm4");
	asm("pshufd $0, %xmm5, %xmm5");
	asm("pshufd $0, %xmm6, %xmm6");
	asm("pshufd $0, %xmm7, %xmm7");
	
	asm("movdqa (%rsi), %xmm8");
	asm("movdqa (%rsi), %xmm9");
	asm("movdqa (%rsi), %xmm10");
	asm("movdqa (%rsi), %xmm11");
	asm("movdqa (%rsi), %xmm12");
	asm("movdqa (%rsi), %xmm13");
	asm("movdqa (%rsi), %xmm14");
	asm("movdqa (%rsi), %xmm15");
	asm("prefetchnta 128(%rsi)");
	asm("pcmpeqb %xmm0, %xmm8");
	asm("pcmpeqb %xmm1, %xmm9");
	asm("pcmpeqb %xmm2, %xmm10");
	asm("pcmpeqb %xmm3, %xmm11");
	asm("pcmpeqb %xmm4, %xmm12");
	asm("pcmpeqb %xmm5, %xmm13");
	asm("pcmpeqb %xmm6, %xmm14");
	asm("pcmpeqb %xmm7, %xmm15");
	asm("por     %xmm9, %xmm8");
	asm("por     %xmm11, %xmm10");
	asm("por     %xmm13, %xmm12");
	asm("por     %xmm15, %xmm14");
	asm("por     %xmm10, %xmm8");
	asm("por     %xmm14, %xmm12");
	
	asm("pmovmskb %xmm8, %r8d");
	asm("pmovmskb %xmm12, %r10d");
	
	asm("addl $1, %ecx");
	asm("lea  (%r8,%r8), %rax");		// %eax now contains bit mask of first search byte shift left 1
	
	asm("andl %eax, %r10d");			// Combine the bytes.
	asm("shrl %cl, %r10d");
	asm("bsfl %r10d, %eax");
	asm("jz LInternalFindPair4DoMainLoop");
	asm("addq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalFindPair4Fail");
	asm("retq");
	
	asm(".balign 32");
	asm(".byte 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90");
	
	asm("LInternalFindPair4DoMainLoop:");
	asm("addq $16, %rsi");				// Advance to next block
										// Want: rdx = end, rdx+rsi = current
										// -> rsi = current - end
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalFindPair4Fail");
	
	asm(".balign 32");
	asm("LInternalFindPair4MainLoop:");
	asm("movdqa (%rdx, %rsi), %xmm8");
	asm("movdqa (%rdx, %rsi), %xmm9");
	asm("movdqa (%rdx, %rsi), %xmm10");
	asm("movdqa (%rdx, %rsi), %xmm11");
	asm("movdqa (%rdx, %rsi), %xmm12");
	asm("movdqa (%rdx, %rsi), %xmm13");
	asm("movdqa (%rdx, %rsi), %xmm14");
	asm("movdqa (%rdx, %rsi), %xmm15");
	asm("prefetchnta 128(%rdx, %rsi)");

	asm("shrl $16, %eax");
	
	asm("pcmpeqb %xmm0, %xmm8");
	asm("pcmpeqb %xmm1, %xmm9");
	asm("pcmpeqb %xmm2, %xmm10");
	asm("pcmpeqb %xmm3, %xmm11");
	asm("pcmpeqb %xmm4, %xmm12");
	asm("pcmpeqb %xmm5, %xmm13");
	asm("pcmpeqb %xmm6, %xmm14");
	asm("pcmpeqb %xmm7, %xmm15");
	
	asm("por     %xmm9, %xmm8");
	asm("por     %xmm11, %xmm10");
	asm("por     %xmm13, %xmm12");
	asm("por     %xmm15, %xmm14");
	asm("por     %xmm10, %xmm8");
	asm("por     %xmm14, %xmm12");
	
	asm("pmovmskb %xmm8, %r8d");
	asm("pmovmskb %xmm12, %r10d");
	
	asm("lea (%rax,%r8,2), %rax");
	
	asm("and %eax, %r10d");
	asm("jne LInternalFindPair4Found");
	asm("addq $16, %rsi");
	asm("jnc LInternalFindPair4MainLoop");
	
	asm("LInternalFindPair4Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalFindPair4Found:");
	asm("bsfl %r10d, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalFindPair4Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalFindByteRange()
{
	asm("pushq %rcx");
	asm("movq %rsi, %rcx");
	asm("andq $-32, %rsi");

	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm1, %xmm1");
	
	asm("movdqa (%rsi), %xmm2");
	asm("movdqa 16(%rsi), %xmm3");
	asm("prefetchnta 256(%rsi)");
	asm("paddb   %xmm0, %xmm2");
	asm("paddb   %xmm0, %xmm3");
	asm("pcmpgtb %xmm1, %xmm2");
	asm("pcmpgtb %xmm1, %xmm3");
	asm("pmovmskb %xmm2, %eax");
	asm("pmovmskb %xmm3, %r10d");
	asm("shll $16, %r10d");
	asm("orl %r10d, %eax");
	asm("shrl %cl, %eax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalFindByteRangeDoMainLoop");
	asm("addq %rcx, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalFindByteRangeFail");
	asm("retq");

	NOP3;

	asm("LInternalFindByteRangeDoMainLoop:");
	asm("addq $32, %rsi");
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalFindByteRangeFail");
	
	asm(".balign 32");
	asm("LInternalFindByteRangeMainLoop:");
	asm("movdqa (%rdx, %rsi), %xmm2");
	asm("movdqa 16(%rdx, %rsi), %xmm3");
	asm("prefetchnta 256(%rdx, %rsi)");

	asm("paddb   %xmm0, %xmm2");
	asm("paddb   %xmm0, %xmm3");
	asm("pcmpgtb %xmm1, %xmm2");
	asm("pcmpgtb %xmm1, %xmm3");
	asm("por     %xmm2, %xmm3");
	asm("pmovmskb %xmm3, %eax");
	asm("test %eax, %eax");
	asm("jne LInternalFindByteRangeFound");
	asm("addq $32, %rsi");
	asm("jnc LInternalFindByteRangeMainLoop");
	
	asm("LInternalFindByteRangeFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalFindByteRangeFound:");
	asm("pmovmskb %xmm2, %r10d");
	asm("shll $16, %eax");
	asm("orl %r10d, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalFindByteRangeFail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvxFindByteRange()
{
    asm("pushq %rcx");
    asm("movq %rsi, %rcx");
    asm("andq $-32, %rsi");
    
    asm("vpshufd $0, %xmm0, %xmm0");
    asm("vpshufd $0, %xmm1, %xmm1");
    
    asm("vmovdqa (%rsi), %xmm2");
    asm("vmovdqa 16(%rsi), %xmm3");
    asm("prefetchnta 768(%rsi)");
    asm("vpaddb   %xmm0, %xmm2, %xmm2");
    asm("vpaddb   %xmm0, %xmm3, %xmm3");
    asm("vpcmpgtb %xmm1, %xmm2, %xmm2");
    asm("vpcmpgtb %xmm1, %xmm3, %xmm3");
    asm("vpmovmskb %xmm2, %eax");
    asm("vpmovmskb %xmm3, %r10d");
    asm("shll $16, %r10d");
    asm("orl %r10d, %eax");
    asm("shrl %cl, %eax");
    asm("bsfl %eax, %eax");
    asm("jz LInternalAvxFindByteRangeDoMainLoop");
    asm("addq %rcx, %rax");
    asm("popq %rcx");
    asm("cmpq %rdx, %rax");
    asm("jae LInternalAvxFindByteRangeFail");
    asm("retq");
    
    NOP1;
    
    asm("LInternalAvxFindByteRangeDoMainLoop:");
    asm("movq %rsi, %r10");
    asm("addq $32, %rsi");
    asm("subq %rdx, %rsi");
    asm("popq %rcx");
    asm("jnc LInternalAvxFindByteRangeFail");
    
    asm(".balign 32");
    asm("LInternalAvxFindByteRangeMainLoop:");
    asm("vmovdqa 32(%r10), %xmm2");
    asm("vmovdqa 48(%r10), %xmm3");
    asm("prefetchnta 768(%r10)");
    asm("addq $32, %r10");
    
    asm("vpaddb   %xmm0, %xmm2, %xmm2");
    asm("vpaddb   %xmm0, %xmm3, %xmm3");
    asm("vpcmpgtb %xmm1, %xmm2, %xmm2");
    asm("vpcmpgtb %xmm1, %xmm3, %xmm3");
    asm("vpor     %xmm2, %xmm3, %xmm3");
    asm("vpmovmskb %xmm3, %eax");
    asm("test %eax, %eax");
    asm("jne LInternalAvxFindByteRangeFound");
    asm("addq $32, %rsi");
    asm("jnc LInternalAvxFindByteRangeMainLoop");
    
    asm("LInternalAvxFindByteRangeFail:");
    asm("xor %eax, %eax");
    asm("retq");
    
    asm("LInternalAvxFindByteRangeFound:");
    asm("vpmovmskb %xmm2, %r10d");
    asm("shll $16, %eax");
    asm("orl %r10d, %eax");
    asm("bsfl %eax, %eax");
    asm("addq %rsi, %rax");
    asm("jc LInternalAvxFindByteRangeFail");
    asm("addq %rdx, %rax");
    asm("retq");
}

// %xmm0 must be loaded with the first byte to search in the lower 32 bits
// %xmm1 must be loaded with the second byte to search in the lower 32 bits
__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx512FindByteRange()
{
    asm("pushq %rcx");
    asm("movq %rsi, %rcx");
    asm("andq $-64, %rsi");
    
    asm("vmovdqa64 (%rsi), %zmm2");
    asm("prefetcht0 2048(%rsi)");

    asm("vpbroadcastb %xmm0, %zmm0");
    asm("vpbroadcastb %xmm1, %zmm1");
    
    asm("vpaddb   %zmm0, %zmm2, %zmm2");
    asm("vpcmpgtb %zmm1, %zmm2, %k1");

    asm("kmovq %k1, %rax");
    asm("shrq %cl, %rax");
    asm("bsfq %rax, %rax");
    asm("jz LInternalAvx512FindByteRangeDoMainLoop");
    asm("addq %rcx, %rax");
    asm("popq %rcx");
    asm("cmpq %rdx, %rax");
    asm("jae LInternalAvx512FindByteRangeFail");
    asm("retq");

    NOP17;
    
    asm("LInternalAvx512FindByteRangeDoMainLoop:");
    asm("addq $64, %rsi");
    asm("popq %rcx");
    asm("cmpq %rdx, %rsi");
    asm("jae LInternalAvx512FindByteRangeFail");

    asm(".balign 32");
    asm("LInternalAvx512FindByteRangeMainLoop:");
    asm("vmovdqa64 (%rsi), %zmm2");
    asm("prefetcht0 2048(%rsi)");
    asm("addq $64, %rsi");
    
    asm("vpaddb   %zmm0, %zmm2, %zmm2");
    asm("vpcmpgtb %zmm1, %zmm2, %k1");
    asm("ktestq %k1, %k1");
    asm("jne LInternalAvx512FindByteRangeFound");
    asm("cmpq %rdx, %rsi");
    asm("jb LInternalAvx512FindByteRangeMainLoop");
    
    asm("LInternalAvx512FindByteRangeFail:");
    asm("xor %eax, %eax");
    asm("retq");
    
    asm("LInternalAvx512FindByteRangeFound:");
    asm("kmovq %k1, %rax");
    asm("bsfq %rax, %rax");
    asm("leaq -64(%rsi, %rax), %rax");
    asm("cmpq %rdx, %rax");
    asm("jae LInternalAvx512FindByteRangeFail");
    asm("retq");
}

// %xmm0 must be loaded with the first byte to search in the lower 32 bits
// %xmm1 must be loaded with the second byte to search in the lower 32 bits
__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx2FindByteRange()
{
    asm("pushq %rcx");
    asm("movq %rsi, %rcx");
    asm("andq $-64, %rsi");
    
    asm("vmovdqa (%rsi), %ymm2");
    asm("vmovdqa 32(%rsi), %ymm3");
    
    asm("vpbroadcastb %xmm0, %ymm0");
    asm("vpbroadcastb %xmm1, %ymm1");
    
    asm("vpaddb   %ymm0, %ymm2, %ymm2");
    asm("vpaddb   %ymm0, %ymm3, %ymm3");
    asm("vpcmpgtb %ymm1, %ymm2, %ymm2");
    asm("vpcmpgtb %ymm1, %ymm3, %ymm3");
    asm("vpmovmskb %ymm2, %eax");
    asm("vpmovmskb %ymm3, %r10d");
    asm("shlq $32, %r10");
    asm("orq %r10, %rax");
    asm("shrq %cl, %rax");
    asm("bsfq %rax, %rax");
    asm("jz LInternalAvx2FindByteRangeDoMainLoop");
    asm("addq %rcx, %rax");
    asm("popq %rcx");
    asm("cmpq %rdx, %rax");
    asm("jae LInternalAvx2FindByteRangeFail");
    asm("retq");

    NOP6;
    
    asm("LInternalAvx2FindByteRangeDoMainLoop:");
    asm("movq %rsi, %r10");
    asm("addq $64, %rsi");
    asm("subq %rdx, %rsi");
    asm("popq %rcx");
    asm("jnc LInternalAvx2FindByteRangeFail");
    
    asm(".balign 32");
    asm("LInternalAvx2FindByteRangeMainLoop:");
    asm("vmovdqa 64(%r10), %ymm2");
    asm("vmovdqa 96(%r10), %ymm3");
    asm("prefetchnta 1536(%r10)");
    asm("addq $64, %r10");
    
    asm("vpaddb   %ymm0, %ymm2, %ymm2");
    asm("vpaddb   %ymm0, %ymm3, %ymm3");
    asm("vpcmpgtb %ymm1, %ymm2, %ymm2");
    asm("vpcmpgtb %ymm1, %ymm3, %ymm3");
    asm("vpor     %ymm2, %ymm3, %ymm3");
    asm("vpmovmskb %ymm3, %eax");
    asm("test %eax, %eax");
    asm("jne LInternalAvx2FindByteRangeFound");
    asm("addq $64, %rsi");
    asm("jnc LInternalAvx2FindByteRangeMainLoop");
    
    asm("LInternalAvx2FindByteRangeFail:");
    asm("xor %eax, %eax");
    asm("retq");
    
    asm("LInternalAvx2FindByteRangeFound:");
    asm("vpmovmskb %ymm2, %r10d");
    asm("shlq $32, %rax");
    asm("orq %r10, %rax");
    asm("bsfq %rax, %rax");
    asm("addq %rsi, %rax");
    asm("jc LInternalAvx2FindByteRangeFail");
    asm("addq %rdx, %rax");
    asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalFindByteRangePair()
{
	asm("pushq %rcx");
	asm("movl %esi, %ecx");
	asm("andq $-32, %rsi");
	asm("andl $31, %ecx");

	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm1, %xmm1");
	asm("pshufd $0, %xmm2, %xmm2");
	asm("pshufd $0, %xmm3, %xmm3");

	asm("movdqa (%rsi), %xmm4");
	asm("movdqa (%rsi), %xmm5");
	asm("movdqa 16(%rsi), %xmm6");
	asm("movdqa 16(%rsi), %xmm7");
	asm("prefetchnta 256(%rsi)");
	asm("paddb %xmm0, %xmm4");
	asm("paddb %xmm2, %xmm5");
	asm("paddb %xmm0, %xmm6");
	asm("paddb %xmm2, %xmm7");
	asm("pcmpgtb %xmm1, %xmm4");
	asm("pcmpgtb %xmm3, %xmm5");
	asm("pcmpgtb %xmm1, %xmm6");
	asm("pcmpgtb %xmm3, %xmm7");
	
	asm("pmovmskb %xmm4, %r8d");
	asm("pmovmskb %xmm5, %r9d");
	asm("pmovmskb %xmm6, %r10d");
	asm("pmovmskb %xmm7, %r11d");
	
	asm("shlq $17, %r10");
	asm("shll $16, %r11d");
	asm("addl $1, %ecx");
	asm("orl  %r11d, %r9d");		// %r9d now contains bit mask of second search byte
	asm("lea  (%r10,%r8,2), %rax");	// %eax now contains bit mask of first search byte shift left 1
	
	asm("andl %eax, %r9d");			// Combine the bytes.
	asm("shrq %cl, %r9");
	asm("bsfl %r9d, %eax");
	asm("jz LInternalFindByteRangePairDoMainLoop");
	asm("addq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalFindByteRangePairFail");
	asm("retq");
	
	asm("LInternalFindByteRangePairDoMainLoop:");
	asm("addq $32, %rsi");				// Advance to next block
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalFindByteRangePairFail");
	
	asm(".balign 32");
	asm("LInternalFindByteRangePairMainLoop:");
	asm("movdqa (%rdx, %rsi), %xmm4");
	asm("movdqa (%rdx, %rsi), %xmm5");
	asm("movdqa 16(%rdx, %rsi), %xmm6");
	asm("movdqa 16(%rdx, %rsi), %xmm7");
	asm("prefetchnta 256(%rdx, %rsi)");

	asm("shrq $32, %rax");
	
	asm("paddb %xmm0, %xmm4");
	asm("paddb %xmm2, %xmm5");
	asm("paddb %xmm0, %xmm6");
	asm("paddb %xmm2, %xmm7");
	asm("pcmpgtb %xmm1, %xmm4");
	asm("pcmpgtb %xmm3, %xmm5");
	asm("pcmpgtb %xmm1, %xmm6");
	asm("pcmpgtb %xmm3, %xmm7");
	
	asm("pmovmskb %xmm4, %r8d");
	asm("pmovmskb %xmm5, %r9d");
	asm("pmovmskb %xmm6, %r10d");
	asm("pmovmskb %xmm7, %r11d");
	
	asm("lea (%rax,%r8,2), %rax");
	asm("shlq $17, %r10");
	asm("shll $16, %r11d");
	asm("addq %r10, %rax");
	asm("orl  %r11d, %r9d");			// %r9d now contains bit mask of second search byte
	
	asm("and %eax, %r9d");
	asm("jne LInternalFindByteRangePairFound");
	asm("addq $32, %rsi");
	asm("jnc LInternalFindByteRangePairMainLoop");
	
	asm("LInternalFindByteRangePairFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalFindByteRangePairFound:");
	asm("bsfl %r9d, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalFindByteRangePairFail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvxFindByteRangePair()
{
	asm("pushq %rcx");
	asm("movl %esi, %ecx");
	asm("andq $-32, %rsi");
	asm("andl $31, %ecx");
	
	asm("vmovdqa (%rsi), %xmm4");
	asm("vmovdqa 16(%rsi), %xmm5");
	asm("prefetchnta 768(%rsi)");
	
	asm("vpshufd $0, %xmm0, %xmm0");
	asm("vpshufd $0, %xmm1, %xmm1");
	asm("vpshufd $0, %xmm2, %xmm2");
	asm("vpshufd $0, %xmm3, %xmm3");
	
	asm("vpaddb %xmm0, %xmm4, %xmm6");
	asm("vpaddb %xmm2, %xmm4, %xmm7");
	asm("vpaddb %xmm0, %xmm5, %xmm8");
	asm("vpaddb %xmm2, %xmm5, %xmm9");
	
	asm("vpcmpgtb %xmm1, %xmm6, %xmm6");
	asm("vpcmpgtb %xmm3, %xmm7, %xmm7");
	asm("vpcmpgtb %xmm1, %xmm8, %xmm12");
	asm("vpcmpgtb %xmm3, %xmm9, %xmm9");
	
	asm("vpslldq $1, %xmm6, %xmm10");
	asm("vpalignr $15, %xmm6, %xmm12, %xmm11");
	asm("vpand %xmm7, %xmm10, %xmm7");
	asm("vpand %xmm9, %xmm11, %xmm9");
	
	asm("vpmovmskb %xmm7, %eax");
	asm("vpmovmskb %xmm9, %r10d");
	
	asm("shlq $16, %r10");
	asm("addl $1, %ecx");
	asm("orl %r10d, %eax");
	
	asm("shrq %cl, %rax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalAvxFindByteRangePairDoMainLoop");
	asm("addq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvxFindByteRangePairFail");
	asm("retq");
	
	asm(".byte 0x90, 0x90, 0x90, 0x90");
	
	asm("LInternalAvxFindByteRangePairFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvxFindByteRangePairDoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $32, %rsi");
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jnc LInternalAvxFindByteRangePairFail");

	asm("xor %eax, %eax");

	asm(".balign 32");
	asm("LInternalAvxFindByteRangePairMainLoop:");
	asm("vmovdqa 32(%r10), %xmm4");
	asm("vmovdqa 48(%r10), %xmm5");
	asm("prefetchnta 768(%r10)");
	asm("addq $32, %r10");
	
	asm("test %eax, %eax");
	asm("jnz LInternalAvxFindByteRangePairFound");
	
	asm("vpaddb %xmm0, %xmm4, %xmm6");
	asm("vpaddb %xmm2, %xmm4, %xmm7");
	asm("vpaddb %xmm0, %xmm5, %xmm8");
	asm("vpaddb %xmm2, %xmm5, %xmm9");
	
	asm("vpcmpgtb %xmm1, %xmm6, %xmm6");
	asm("vpcmpgtb %xmm3, %xmm7, %xmm7");
	asm("vpcmpgtb %xmm1, %xmm8, %xmm8");
	asm("vpcmpgtb %xmm3, %xmm9, %xmm9");
	
	asm("vpalignr $15, %xmm12, %xmm6, %xmm10");
	asm("vpalignr $15, %xmm6, %xmm8, %xmm11");
	asm("vpand %xmm7, %xmm10, %xmm7");
	asm("vpand %xmm9, %xmm11, %xmm9");
	asm("vpor %xmm7, %xmm9, %xmm9");
	asm("vmovdqa %xmm8, %xmm12");
	
	asm("vpmovmskb %xmm9, %eax");
	asm("addq $32, %rsi");
	asm("jnc LInternalAvxFindByteRangePairMainLoop");
	
	asm("test %eax, %eax");
	asm("jz LInternalAvxFindByteRangePairLastExit");
	
	asm("LInternalAvxFindByteRangePairFound:");
	asm("vpmovmskb %xmm7, %r10d");
	asm("shll $16, %eax");
	asm("subq $32, %rsi");
	asm("orl %r10d, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvxFindByteRangePairFail");
	asm("addq %rdx, %rax");
	asm("LInternalAvxFindByteRangePairLastExit:");
	asm("retq");
}


// %rcx = data->offset
// %rsi = data
// %rdx = pEnd
// %rdi = bitmask table.
__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalFindShiftOr()
{
	asm("movl $-1, %eax");		// %eax = state
	asm("subq $8, %rdx");
	
	asm("cmpq %rdx, %rsi");
	asm("jae  LFindShiftOrLoopFinished");
	
	asm("pushq %r12");
	asm("pushq %r13");
	asm("pushq %r14");
	asm("pushq %r15");
	
	asm(".balign 32");
	
	asm("LFindShiftOrLoop:");
	asm("movzbq (%rsi), %r8");
	asm("movzbq 1(%rsi), %r9");
	asm("movzbq 2(%rsi), %r10");
	asm("movzbq 3(%rsi), %r11");
	asm("movzbq 4(%rsi), %r12");
	asm("movzbq 5(%rsi), %r13");
	asm("movzbq 6(%rsi), %r14");
	asm("movzbq 7(%rsi), %r15");
	
	asm("movl (%rdi,%r8,4), %r8d");
	asm("movl (%rdi,%r9,4), %r9d");
	asm("movl (%rdi,%r10,4), %r10d");
	asm("movl (%rdi,%r11,4), %r11d");
	asm("movl (%rdi,%r12,4), %r12d");
	asm("movl (%rdi,%r13,4), %r13d");
	asm("movl (%rdi,%r14,4), %r14d");
	asm("movl (%rdi,%r15,4), %r15d");
	
	asm("addq $8, %rsi");
	
	asm("shll $8, %eax");
	asm("shll $7, %r8d");
	asm("shll $6, %r9d");
	asm("shll $5, %r10d");
	asm("shll $4, %r11d");
	asm("shll $3, %r12d");
	asm("shll $2, %r13d");
	asm("shll $1, %r14d");
	
	asm("orl %r15d, %eax");
	asm("orl %r9d, %r8d");
	asm("orl %r11d, %r10d");
	asm("orl %r13d, %r12d");
	asm("orl %r14d, %eax");
	asm("orl %r10d, %r8d");
	asm("orl %r12d, %eax");
	asm("orl %r8d, %eax");
	
	asm("movl %eax, %r15d");
	asm("notl %r15d");
	asm("shrl %cl, %r15d");
	asm("jnz LFindShiftOrFound");
	asm("cmpq %rdx, %rsi");
	asm("jb LFindShiftOrLoop");
	
	asm("popq %r15");
	asm("popq %r14");
	asm("popq %r13");
	asm("popq %r12");
	
	asm("LFindShiftOrLoopFinished:");
	asm("movl $1, %r8d");
	asm("addq $8, %rdx");
	asm("shll %cl, %r8d");
	
	asm("LFindShiftOrSingleByteLoop:");
	asm("movzbl (%rsi), %r9d");
	asm("movzbl (%rdi,%r9,4), %r9d");
	asm("addq $1, %rsi");
	asm("shll $1, %eax");
	asm("orl %r9d, %eax");
	asm("test %r8d, %eax");
	asm("jz LFindShiftOrSingleByteFound");
	asm("cmpq %rdx, %rsi");
	asm("jb LFindShiftOrSingleByteLoop");

	asm("xorl %esi, %esi");
	asm("retq");
	
	asm("LFindShiftOrSingleByteFound:");
	asm("subq $1, %rsi");
	asm("subq %rcx, %rsi");
	asm("retq");
	
	asm("LFindShiftOrFound:");
	asm("subq $1, %rsi");
	asm("bsrl %r15d, %r10d");
	asm("subq %rcx, %rsi");
	asm("subq %r10, %rsi");
	asm("addq $8, %rdx");
	asm("popq %r15");
	asm("popq %r14");
	asm("popq %r13");
	asm("popq %r12");
	asm("retq");
	
	asm("LFindShiftOrFail:");
	asm("xorl %esi, %esi");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalFindTriplet()
{
	asm("pushq %rbx");
	asm("pushq %r12");
	asm("pushq %r13");
	asm("pushq %rcx");
	
	asm("movl %esi, %ecx");
	asm("andq $-32, %rsi");
	asm("andl $31, %ecx");
	
	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm1, %xmm1");
	asm("pshufd $0, %xmm2, %xmm2");

	asm("movdqa (%rsi), %xmm3");
	asm("movdqa (%rsi), %xmm4");
	asm("movdqa (%rsi), %xmm5");
	asm("movdqa 16(%rsi), %xmm6");
	asm("movdqa 16(%rsi), %xmm7");
	asm("movdqa 16(%rsi), %xmm8");
	asm("prefetchnta 256(%rsi)");
	asm("pcmpeqb %xmm0, %xmm3");
	asm("pcmpeqb %xmm1, %xmm4");
	asm("pcmpeqb %xmm2, %xmm5");
	asm("pcmpeqb %xmm0, %xmm6");
	asm("pcmpeqb %xmm1, %xmm7");
	asm("pcmpeqb %xmm2, %xmm8");
	
	asm("pmovmskb %xmm3, %r8d");
	asm("pmovmskb %xmm4, %r9d");
	asm("pmovmskb %xmm5, %r10d");
	asm("pmovmskb %xmm6, %r11d");
	asm("pmovmskb %xmm7, %r12d");
	asm("pmovmskb %xmm8, %r13d");
	
	asm("shlq $18, %r11");
	asm("shlq $17, %r12");
	asm("shll $16, %r13d");
	asm("addl $2, %ecx");
	asm("leaq (%r11,%r8,4), %rax");		// %eax now contains bit mask of first search byte shift left 2
	asm("leaq (%r12,%r9,2), %rbx");		// %rbx now contains bit mask of second search byte
	asm("orl  %r13d, %r10d");			// %r10d now contains bit mask of third search byte
	
	asm("andl %eax, %r10d");			// Combine the bytes.
	asm("andl %ebx, %r10d");
	
	asm("shrq %cl, %r10");
	asm("bsfl %r10d, %eax");
	asm("jz LInternalFindTripletDoMainLoop");
	asm("addq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalFindTripletFail");
	
	asm("popq %r13");
	asm("popq %r12");
	asm("popq %rbx");
	asm("retq");
	
	asm(".byte 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90");

	asm("LInternalFindTripletDoMainLoop:");
	asm("shrq %cl, %rax");
	asm("addq $32, %rsi");
	asm("shlq %cl, %rax");
	asm("popq %rcx");
	asm("subq %rdx, %rsi");
	asm("jnc LInternalFindTripletFail");
	
	asm(".balign 32");
	asm("LInternalFindTripletMainLoop:");
	asm("movdqa (%rdx, %rsi), %xmm3");
	asm("movdqa (%rdx, %rsi), %xmm4");
	asm("movdqa (%rdx, %rsi), %xmm5");
	asm("movdqa 16(%rdx, %rsi), %xmm6");
	asm("movdqa 16(%rdx, %rsi), %xmm7");
	asm("movdqa 16(%rdx, %rsi), %xmm8");
	asm("prefetchnta 256(%rdx, %rsi)");
	
	asm("shrq $32, %rax");
	asm("shrq $32, %rbx");
	
	asm("pcmpeqb %xmm0, %xmm3");
	asm("pcmpeqb %xmm1, %xmm4");
	asm("pcmpeqb %xmm2, %xmm5");
	asm("pcmpeqb %xmm0, %xmm6");
	asm("pcmpeqb %xmm1, %xmm7");
	asm("pcmpeqb %xmm2, %xmm8");
	
	asm("pmovmskb %xmm3, %r8d");
	asm("pmovmskb %xmm4, %r9d");
	asm("pmovmskb %xmm5, %r10d");
	asm("pmovmskb %xmm6, %r11d");
	asm("pmovmskb %xmm7, %r12d");
	asm("pmovmskb %xmm8, %r13d");
	
	asm("lea (%rax,%r8,4), %rax");
	asm("lea (%rbx,%r9,2), %rbx");
	asm("shlq $18, %r11");
	asm("shlq $17, %r12");
	asm("shll $16, %r13d");
	asm("addq %r11, %rax");
	asm("addq %r12, %rbx");
	asm("orl  %r13d, %r10d");
	
	asm("andl %eax, %r10d");
	asm("andl %ebx, %r10d");
	asm("jne LInternalFindTripletFound");
	asm("addq $32, %rsi");
	asm("jnc LInternalFindTripletMainLoop");
	
	asm("LInternalFindTripletFail:");
	asm("xor %eax, %eax");
	asm("popq %r13");
	asm("popq %r12");
	asm("popq %rbx");
	asm("retq");
	
	asm("LInternalFindTripletFound:");
	asm("bsfl %r10d, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalFindTripletFail");
	asm("addq %rdx, %rax");
	asm("popq %r13");
	asm("popq %r12");
	asm("popq %rbx");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvxFindTriplet()
{
	asm("movl %esi, %r10d");
	asm("andq $-32, %rsi");
	asm("andl $31, %r10d");
	
	asm("vpshufd $0, %xmm0, %xmm0");
	asm("vpshufd $0, %xmm1, %xmm1");
	asm("vpshufd $0, %xmm2, %xmm2");
	asm("vpxor %xmm15, %xmm15, %xmm15");
	asm("vmovq %r10, %xmm14");

	asm("vmovdqa (%rsi), %xmm3");
	asm("vmovdqa 16(%rsi), %xmm4");
	asm("vmovdqa LInternalAvxFindTripletShiftMask(%rip), %xmm12");
	asm("vmovdqa LInternalAvxFindTripletShiftMask2(%rip), %xmm13");
	asm("prefetchnta 512(%rsi)");
	
	asm("vpshufb %xmm15, %xmm14, %xmm14");
	asm("vpcmpgtb %xmm14, %xmm12, %xmm12");
	asm("vpcmpgtb %xmm14, %xmm13, %xmm13");

	asm("vpcmpeqb %xmm0, %xmm3, %xmm5");
	asm("vpcmpeqb %xmm1, %xmm3, %xmm6");
	asm("vpcmpeqb %xmm2, %xmm3, %xmm7");
	asm("vpand %xmm12, %xmm5, %xmm5");
	asm("vpcmpeqb %xmm0, %xmm4, %xmm8");
	asm("vpcmpeqb %xmm1, %xmm4, %xmm9");
	asm("vpcmpeqb %xmm2, %xmm4, %xmm10");
	asm("vpand %xmm13, %xmm8, %xmm8");
	
	asm("vpslldq $2, %xmm5, %xmm11");
	asm("vpslldq $1, %xmm6, %xmm12");
	asm("vpalignr $14, %xmm5, %xmm8, %xmm13");
	asm("vpalignr $15, %xmm6, %xmm9, %xmm14");
	asm("vpand %xmm7, %xmm11, %xmm7");
	asm("vpand %xmm10, %xmm13, %xmm10");
	asm("vpand %xmm7, %xmm12, %xmm7");
	asm("vpand %xmm10, %xmm14, %xmm10");
	
	asm("vpmovmskb %xmm7, %eax");
	asm("vpmovmskb %xmm10, %r10d");
	
	asm("shlq $16, %r10");
	asm("orl %r10d, %eax");
	asm("bsfl %eax, %eax");
	asm("jz LInternalAvxFindTripletDoMainLoop");
	asm("addq %rsi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvxFindTripletFail");
	
	asm("retq");
	
	// This is so that the MainLoop aligns on a 32-byte boundary.
	asm(".balign 32");
	asm("LInternalAvxFindTripletShiftMask:");
	asm(".byte 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16");
	asm("LInternalAvxFindTripletShiftMask2:");
	asm(".byte 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32");
	// This is so that the MainLoop aligns on a 32-byte boundary.
	asm(".byte 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90");
	
	asm("LInternalAvxFindTripletDoMainLoop:");
	asm("movq %rsi, %r10");
	asm("addq $32, %rsi");
	asm("subq %rdx, %rsi");
	asm("jnc LInternalAvxFindTripletFail");
	
	asm(".balign 32");
	asm("LInternalAvxFindTripletMainLoop:");
	asm("vmovdqa 32(%r10), %xmm3");
	asm("vmovdqa 48(%r10), %xmm4");
	asm("prefetchnta 544(%r10)");
	asm("addq $32, %r10");

	asm("vpcmpeqb %xmm0, %xmm3, %xmm5");
	asm("vpcmpeqb %xmm1, %xmm3, %xmm6");
	asm("vpcmpeqb %xmm2, %xmm3, %xmm7");
	asm("vpcmpeqb %xmm0, %xmm4, %xmm13");
	asm("vpcmpeqb %xmm1, %xmm4, %xmm14");
	asm("vpcmpeqb %xmm2, %xmm4, %xmm10");
	
	asm("vpalignr $14, %xmm8, %xmm5, %xmm11");
	asm("vpalignr $15, %xmm9, %xmm6, %xmm12");
	asm("vpalignr $14, %xmm5, %xmm13, %xmm5");
	asm("vpalignr $15, %xmm6, %xmm14, %xmm6");
	
	asm("vpand %xmm7, %xmm11, %xmm7");
	asm("vpand %xmm10, %xmm5, %xmm10");
	asm("vpand %xmm7, %xmm12, %xmm7");
	asm("vpand %xmm10, %xmm6, %xmm10");
	asm("vpor %xmm7, %xmm10, %xmm10");
	
	asm("vmovdqa %xmm13, %xmm8");
	asm("vpmovmskb %xmm10, %eax");
	
	asm("vmovdqa %xmm14, %xmm9");
	
	asm("test %eax, %eax");
	asm("jne LInternalAvxFindTripletFound");
	asm("addq $32, %rsi");
	asm("jnc LInternalAvxFindTripletMainLoop");
	
	asm("LInternalAvxFindTripletFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvxFindTripletFound:");
	asm("vpmovmskb %xmm7, %r10d");
	asm("shll $16, %eax");
	asm("orl %r10d, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvxFindTripletFail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalAvx2FindTriplet()
{
	asm("movl %esi, %r10d");
	asm("andq $-64, %rsi");
	asm("andl $63, %r10d");
	
	asm("vmovq %r10, %xmm14");
	
	asm("vmovdqa (%rsi), %ymm3");
	asm("vmovdqa 32(%rsi), %ymm4");
	asm("vmovdqa LInternalAvx2FindTripletShiftMask(%rip), %ymm12");
	asm("vmovdqa LInternalAvx2FindTripletShiftMask2(%rip), %ymm13");
	asm("prefetchnta 1024(%rsi)");
	
	asm("vpbroadcastb %xmm0, %ymm0");
	asm("vpbroadcastb %xmm1, %ymm1");
	asm("vpbroadcastb %xmm2, %ymm2");
	asm("vpbroadcastb %xmm14, %ymm14");
	
	asm("cmp $63, %r10d");
	asm("je LInternalAvx2FindByteTripletDoMainLoopMaskR3");
	
	asm("vpcmpgtb %ymm14, %ymm12, %ymm12");
	asm("vpcmpgtb %ymm14, %ymm13, %ymm13");
	asm("vpcmpeqb %ymm0, %ymm3, %ymm5");
	asm("vpcmpeqb %ymm0, %ymm4, %ymm8");
	asm("vpcmpeqb %ymm1, %ymm3, %ymm6");
	asm("vpcmpeqb %ymm1, %ymm4, %ymm9");
	asm("vpand %ymm12, %ymm5, %ymm5");
	asm("vpand %ymm13, %ymm8, %ymm8");
	asm("vpcmpeqb %ymm2, %ymm3, %ymm7");
	asm("vpcmpeqb %ymm2, %ymm4, %ymm10");
	
	asm("vperm2i128 $0x8, %ymm5, %ymm5, %ymm3");
	asm("vperm2i128 $0x8, %ymm6, %ymm6, %ymm4");
	asm("vpalignr $14, %ymm3, %ymm5, %ymm11");
	asm("vpalignr $15, %ymm4, %ymm6, %ymm12");
	asm("vperm2i128 $0x3, %ymm5, %ymm8, %ymm3");
	asm("vperm2i128 $0x3, %ymm6, %ymm9, %ymm4");
	asm("vpalignr $14, %ymm3, %ymm8, %ymm13");
	asm("vpalignr $15, %ymm4, %ymm9, %ymm14");
	asm("vpand %ymm7, %ymm11, %ymm7");
	asm("vpand %ymm10, %ymm13, %ymm10");
	asm("vpand %ymm7, %ymm12, %ymm7");
	asm("vpand %ymm10, %ymm14, %ymm10");
	
	asm("vpmovmskb %ymm7, %eax");
	asm("vpmovmskb %ymm10, %r10d");
	
	asm("shlq $32, %r10");
	asm("orq %r10, %rax");
	asm("bsfq %rax, %rax");
	asm("jz LInternalAvx2FindTripletDoMainLoop");
	asm("addq %rsi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalAvx2FindTripletFail");
	asm("retq");
	
	asm(".balign 32");
	asm("LInternalAvx2FindTripletShiftMask:");
	asm(".byte 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16");
	asm(".byte 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32");
	asm("LInternalAvx2FindTripletShiftMask2:");
	asm(".byte 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48");
	asm(".byte 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64");
	// This is so that the MainLoop aligns on a 32-byte boundary.
	asm(".byte 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90");
	
	asm("LInternalAvx2FindByteTripletDoMainLoopMaskR3:");
	asm("addq $64, %rsi");
	asm("subq %rdx, %rsi");
	asm("jnc LInternalAvx2FindTripletFail");
	
	asm("vmovdqu -1(%rdx, %rsi), %ymm4");
	asm("vperm2i128 $0x8, %ymm4, %ymm4, %ymm3");
	asm("vpalignr $15, %ymm3, %ymm4, %ymm3");
	asm("jmp LInternalAvx2FindByteTripletMainLoopR3B0Zeroed");
	
	asm("LInternalAvx2FindTripletDoMainLoop:");
	asm("addq $64, %rsi");
	asm("subq %rdx, %rsi");
	asm("jnc LInternalAvx2FindTripletFail");
	
	asm(".balign 32");
	asm("LInternalAvx2FindTripletMainLoop:");
	asm("vmovdqu -2(%rdx, %rsi), %ymm3");
	asm("vmovdqu -1(%rdx, %rsi), %ymm4");
	asm("LInternalAvx2FindByteTripletMainLoopR3B0Zeroed:");
	asm("vmovdqa (%rdx, %rsi), %ymm5");
	asm("vmovdqu 30(%rdx, %rsi), %ymm6");
	asm("vmovdqu 31(%rdx, %rsi), %ymm7");
	asm("vmovdqa 32(%rdx, %rsi), %ymm8");
	asm("prefetchnta 1024(%rdx, %rsi)");
	
	asm("vpcmpeqb %ymm0, %ymm3, %ymm3");
	asm("vpcmpeqb %ymm1, %ymm4, %ymm4");
	asm("vpcmpeqb %ymm2, %ymm5, %ymm5");
	asm("vpcmpeqb %ymm0, %ymm6, %ymm6");
	asm("vpcmpeqb %ymm1, %ymm7, %ymm7");
	asm("vpcmpeqb %ymm2, %ymm8, %ymm8");
	
	asm("vpand %ymm3, %ymm4, %ymm3");
	asm("vpand %ymm6, %ymm7, %ymm6");
	
	asm("vpand %ymm3, %ymm5, %ymm3");
	asm("vpand %ymm6, %ymm8, %ymm6");
	
	asm("vpor %ymm3, %ymm6, %ymm6");
	asm("vpmovmskb %ymm6, %eax");
	
	asm("test %eax, %eax");
	asm("jne LInternalAvx2FindTripletFound");
	asm("addq $64, %rsi");
	asm("jnc LInternalAvx2FindTripletMainLoop");
	
	asm("LInternalAvx2FindTripletFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalAvx2FindTripletFound:");
	asm("vpmovmskb %ymm3, %r10d");
	asm("shlq $32, %rax");
	asm("orq %r10, %rax");
	asm("bsfq %rax, %rax");
	asm("addq %rsi, %rax");
	asm("jc LInternalAvx2FindTripletFail");
	asm("addq %rdx, %rax");
	asm("retq");
}

__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalFindTriplet2()
{
	asm("pushq %rbx");
	asm("pushq %rcx");
	asm("movl %esi, %ecx");
	asm("andq $-16, %rsi");
	asm("andl $15, %ecx");
	
	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm1, %xmm1");
	asm("pshufd $0, %xmm2, %xmm2");
	asm("pshufd $0, %xmm3, %xmm3");
	asm("pshufd $0, %xmm4, %xmm4");
	asm("pshufd $0, %xmm5, %xmm5");
	
	asm("movdqa (%rsi), %xmm8");
	asm("movdqa (%rsi), %xmm9");
	asm("movdqa (%rsi), %xmm10");
	asm("movdqa (%rsi), %xmm11");
	asm("movdqa (%rsi), %xmm12");
	asm("movdqa (%rsi), %xmm13");
	asm("prefetchnta 256(%rsi)");
	
	asm("pcmpeqb %xmm0, %xmm8");
	asm("pcmpeqb %xmm1, %xmm9");
	asm("pcmpeqb %xmm2, %xmm10");
	asm("pcmpeqb %xmm3, %xmm11");
	asm("pcmpeqb %xmm4, %xmm12");
	asm("pcmpeqb %xmm5, %xmm13");
	
	asm("por %xmm8, %xmm9");
	asm("por %xmm10, %xmm11");
	asm("por %xmm12, %xmm13");
	asm("xorl %r11d, %r11d");
	
	asm("pmovmskb %xmm9, %r8d");
	asm("pmovmskb %xmm11, %r9d");
	asm("pmovmskb %xmm13, %r10d");
	
	asm("addl $2, %ecx");
	asm("leaq (%r11,%r8,4), %rax");		// %eax now contains bit mask of first search byte shift left 2
	asm("leaq (%r11,%r9,2), %rbx");		// %rbx now contains bit mask of second search byte
	
	asm("andl %eax, %r10d");		// Combine the bytes.
	asm("andl %ebx, %r10d");
	
	asm("shrl %cl, %r10d");
	asm("bsfl %r10d, %eax");
	asm("jz LInternalFindTriplet2DoMainLoop");
	asm("addq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jae LInternalFindTriplet2Fail");
	
	asm("popq %rbx");
	asm("retq");

	asm(".byte 0x90, 0x90, 0x90");
	
	asm("LInternalFindTriplet2DoMainLoop:");
	asm("shrq %cl, %rax");
	asm("addq $16, %rsi");
	asm("shlq %cl, %rax");
	asm("popq %rcx");
	asm("subq %rdx, %rsi");
	asm("jnc LInternalFindTriplet2Fail");
	
	asm(".balign 32");
	asm("LInternalFindTriplet2MainLoop:");
	asm("movdqa (%rdx, %rsi), %xmm8");
	asm("movdqa (%rdx, %rsi), %xmm9");
	asm("movdqa (%rdx, %rsi), %xmm10");
	asm("movdqa (%rdx, %rsi), %xmm11");
	asm("movdqa (%rdx, %rsi), %xmm12");
	asm("movdqa (%rdx, %rsi), %xmm13");
	asm("prefetchnta 256(%rdx, %rsi)");
	
	asm("shrl $16, %eax");
	asm("shrl $16, %ebx");
	
	asm("pcmpeqb %xmm0, %xmm8");
	asm("pcmpeqb %xmm1, %xmm9");
	asm("pcmpeqb %xmm2, %xmm10");
	asm("pcmpeqb %xmm3, %xmm11");
	asm("pcmpeqb %xmm4, %xmm12");
	asm("pcmpeqb %xmm5, %xmm13");
	
	asm("por %xmm8, %xmm9");
	asm("por %xmm10, %xmm11");
	asm("por %xmm12, %xmm13");
	
	asm("pmovmskb %xmm9, %r8d");
	asm("pmovmskb %xmm11, %r9d");
	asm("pmovmskb %xmm13, %r10d");
	
	asm("lea (%rax,%r8,4), %rax");
	asm("lea (%rbx,%r9,2), %rbx");
	
	asm("andl %eax, %r10d");
	asm("andl %ebx, %r10d");
	asm("jne LInternalFindTriplet2Found");
	asm("addq $16, %rsi");
	asm("jnc LInternalFindTriplet2MainLoop");
	
	asm("LInternalFindTriplet2Fail:");
	asm("xor %eax, %eax");
	asm("popq %rbx");
	asm("retq");
	
	asm("LInternalFindTriplet2Found:");
	asm("bsfl %r10d, %eax");
	asm("addq %rsi, %rax");
	asm("jc LInternalFindTriplet2Fail");
	asm("addq %rdx, %rax");
	asm("popq %rbx");
	asm("retq");
}

//============================================================================

// %xmm0 = value to search for in all bytes
// %rsi = start of search.
// %rdx = stop
// Result in %rax
// rsi and rdx are inclusive
__attribute__((naked, aligned(32))) void* Javelin::PatternInternal::Amd64FindMethods::InternalFindByteReverse()
{
	asm("pushq %rcx");
	asm("movl %esi, %ecx");
	asm("andq $-32, %rsi");
	asm("not  %ecx");
	asm("andl $31, %ecx");
	
	asm("pshufd $0, %xmm0, %xmm0");
	
	asm("movdqa (%rsi), %xmm1");
	asm("movdqa 16(%rsi), %xmm2");
	asm("prefetchnta -256(%rsi)");
	asm("pcmpeqb %xmm0, %xmm1");
	asm("pcmpeqb %xmm0, %xmm2");
	asm("pmovmskb %xmm1, %eax");
	asm("pmovmskb %xmm2, %r10d");
	asm("shll $16, %r10d");
	asm("orl %r10d, %eax");
	asm("shll %cl, %eax");
	asm("bsrl %eax, %eax");
	asm("jz LInternalFindByteReverseDoMainLoop");
	asm("subq %rcx, %rsi");
	asm("addq %rsi, %rax");
	asm("popq %rcx");
	asm("cmpq %rdx, %rax");
	asm("jb LInternalFindByteReverseFail");
	asm("retq");
	
	asm(".byte 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90");
	
	asm("LInternalFindByteReverseDoMainLoop:");
	asm("subq %rdx, %rsi");
	asm("popq %rcx");
	asm("jbe LInternalFindByteReverseFail");
	
	asm(".balign 32");
	asm("LInternalFindByteReverseMainLoop:");
	asm("movdqa -32(%rdx, %rsi), %xmm1");
	asm("movdqa -16(%rdx, %rsi), %xmm2");
	asm("prefetchnta -256(%rdx, %rsi)");
	asm("pcmpeqb %xmm0, %xmm1");
	asm("pcmpeqb %xmm0, %xmm2");
	asm("por %xmm2, %xmm1");
	asm("pmovmskb %xmm1, %eax");
	asm("test %eax, %eax");
	asm("jne LInternalFindByteReverseFound");
	asm("subq $32, %rsi");
	asm("jbe LInternalFindByteReverseMainLoop");
	
	asm("LInternalFindByteReverseFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LInternalFindByteReverseFound:");
	asm("pmovmskb %xmm2, %r10d");
	asm("shll $16, %r10d");
	asm("orl %r10d, %eax");
	asm("bsrl %eax, %eax");
	asm("addq %rsi, %rax");
	asm("sub $32, %rax");
	asm("jb LInternalFindByteReverseFail");
	asm("addq %rdx, %rax");
	asm("retq");
}


//============================================================================
#endif
//============================================================================
