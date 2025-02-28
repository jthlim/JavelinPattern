//============================================================================
// Included from PatternProcessor.cpp to provide X86_X64 search methods
//============================================================================

#define FUNCTION_FIND_BYTE_DEFINED
__attribute__((naked)) const void* PatternProcessor::FindByte(const void* p, uint64_t v, const void* pEnd)
{
	asm("movl %edi, %ecx");
	asm("movq %rsi, %xmm0");
	asm("andq $-32, %rdi");
	asm("andl $31, %ecx");
	
	asm("punpcklbw %xmm0, %xmm0");
	asm("punpcklwd %xmm0, %xmm0");
	asm("pshufd $0, %xmm0, %xmm0");
	
	asm("movdqa (%rdi), %xmm1");
	asm("movdqa 16(%rdi), %xmm2");
	asm("pcmpeqb %xmm0, %xmm1");
	asm("pcmpeqb %xmm0, %xmm2");
	asm("pmovmskb %xmm1, %eax");
	asm("pmovmskb %xmm2, %r10d");
	asm("shll $16, %r10d");
	asm("orl %r10d, %eax");
	asm("shrl %cl, %eax");
	asm("bsfl %eax, %eax");
	asm("jz LFindByteDoMainLoop");
	asm("addq %rcx, %rdi");
	asm("addq %rdi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LFindByteFail");
	asm("retq");
	
	asm("LFindByteDoMainLoop:");
	asm("addq $32, %rdi");
	asm("subq %rdx, %rdi");
	asm("jnc LFindByteFail");
	
	asm("LFindByteMainLoop:");
	asm("movdqa (%rdx, %rdi), %xmm1");
	asm("movdqa 16(%rdx, %rdi), %xmm2");
	asm("prefetchnta 512(%rdx, %rdi)");
	asm("pcmpeqb %xmm0, %xmm1");
	asm("pcmpeqb %xmm0, %xmm2");
	asm("por %xmm1, %xmm2");
	asm("pmovmskb %xmm2, %eax");
	asm("test %eax, %eax");
	asm("jne LFindByteFound");
	asm("addq $32, %rdi");
	asm("jnc LFindByteMainLoop");
	
	asm("LFindByteFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LFindByteFound:");
	asm("pmovmskb %xmm1, %r10d");
	asm("shll $16, %eax");
	asm("orl %r10d, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rdi, %rax");
	asm("jc LFindByteFail");
	asm("addq %rdx, %rax");
	asm("retq");
}

#define FUNCTION_FIND_BYTE_EITHER_OF_2_DEFINED
__attribute__((naked)) const void* PatternProcessor::FindByteEitherOf2(const void* p, uint64_t v, const void* pEnd)
{
	asm("movl %edi, %ecx");
	asm("andq $-32, %rdi");
	asm("andl $31, %ecx");

	asm("movq %rsi, %xmm0");
	asm("punpcklbw %xmm0, %xmm0");
	asm("punpcklwd %xmm0, %xmm0");
	asm("pshufd $0x55, %xmm0, %xmm1");
	asm("pshufd $0, %xmm0, %xmm0");
	
	asm("movdqa (%rdi), %xmm2");
	asm("movdqa (%rdi), %xmm3");
	asm("movdqa 16(%rdi), %xmm4");
	asm("movdqa 16(%rdi), %xmm5");
	asm("prefetchnta 256(%rdi)");
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
	asm("jz LFindByteEitherOf2DoMainLoop");
	asm("addq %rcx, %rdi");
	asm("addq %rdi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LFindByteEitherOf2Fail");
	asm("retq");
	
	asm("LFindByteEitherOf2DoMainLoop:");
	asm("movq %rdi, %r10");
	asm("addq $32, %rdi");
	asm("subq %rdx, %rdi");
	asm("jnc LFindByteEitherOf2Fail");
	
	asm("LFindByteEitherOf2MainLoop:");
	asm("movdqa 32(%r10), %xmm2");
	asm("movdqa 32(%r10), %xmm3");
	asm("movdqa 48(%r10), %xmm4");
	asm("movdqa 48(%r10), %xmm5");
	asm("prefetchnta 288(%r10)");
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
	asm("jne LFindByteEitherOf2Found");
	asm("addq $32, %rdi");
	asm("jnc LFindByteEitherOf2MainLoop");
	
	asm("LFindByteEitherOf2Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LFindByteEitherOf2Found:");
	asm("pmovmskb %xmm3, %r10d");
	asm("shll $16, %eax");
	asm("orl %r10d, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rdi, %rax");
	asm("jc LFindByteEitherOf2Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

#define FUNCTION_FIND_BYTE_EITHER_OF_3_DEFINED
__attribute__((naked)) const void* PatternProcessor::FindByteEitherOf3(const void* p, uint64_t v, const void* pEnd)
{
	asm("movl %edi, %ecx");
	asm("andq $-32, %rdi");
	asm("andl $31, %ecx");

	asm("movq %rsi, %xmm0");
	asm("punpcklbw %xmm0, %xmm0");
	asm("punpcklwd %xmm0, %xmm0");
	asm("pshufd $0xaa, %xmm0, %xmm2");
	asm("pshufd $0x55, %xmm0, %xmm1");
	asm("pshufd $0, %xmm0, %xmm0");
	
	asm("movdqa (%rdi), %xmm3");
	asm("movdqa (%rdi), %xmm4");
	asm("movdqa (%rdi), %xmm5");
	asm("movdqa 16(%rdi), %xmm6");
	asm("movdqa 16(%rdi), %xmm7");
	asm("movdqa 16(%rdi), %xmm8");
	asm("prefetchnta 256(%rdi)");
	
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
	asm("jz LFindByteEitherOf3DoMainLoop");
	asm("addq %rcx, %rdi");
	asm("addq %rdi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LFindByteEitherOf3Fail");
	asm("retq");
	
	asm("LFindByteEitherOf3DoMainLoop:");
	asm("movq %rdi, %r10");
	asm("addq $32, %rdi");
	asm("subq %rdx, %rdi");
	asm("jnc LFindByteEitherOf3Fail");
	
	asm("LFindByteEitherOf3MainLoop:");
	asm("movdqa 32(%r10), %xmm3");
	asm("movdqa 32(%r10), %xmm4");
	asm("movdqa 32(%r10), %xmm5");
	asm("movdqa 48(%r10), %xmm6");
	asm("movdqa 48(%r10), %xmm7");
	asm("movdqa 48(%r10), %xmm8");
	asm("prefetchnta 288(%r10)");
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
	asm("jne LFindByteEitherOf3Found");
	asm("addq $32, %rdi");
	asm("jnc LFindByteEitherOf3MainLoop");
	
	asm("LFindByteEitherOf3Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LFindByteEitherOf3Found:");
	asm("pmovmskb %xmm5, %r10d");
	asm("shll $16, %eax");
	asm("orl %r10d, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rdi, %rax");
	asm("jc LFindByteEitherOf3Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

#define FUNCTION_FIND_BYTE_EITHER_OF_4_DEFINED
__attribute__((naked)) const void* PatternProcessor::FindByteEitherOf4(const void* p, uint64_t v, const void* pEnd)
{
	asm("movl %edi, %ecx");
	asm("andq $-32, %rdi");
	asm("andl $31, %ecx");
	
	asm("movq %rsi, %xmm0");
	asm("punpcklbw %xmm0, %xmm0");
	asm("punpcklwd %xmm0, %xmm0");
	asm("pshufd $0xff, %xmm0, %xmm3");
	asm("pshufd $0xaa, %xmm0, %xmm2");
	asm("pshufd $0x55, %xmm0, %xmm1");
	asm("pshufd $0, %xmm0, %xmm0");

	asm("movdqa (%rdi), %xmm4");
	asm("movdqa (%rdi), %xmm5");
	asm("movdqa (%rdi), %xmm6");
	asm("movdqa (%rdi), %xmm7");
	asm("movdqa 16(%rdi), %xmm8");
	asm("movdqa 16(%rdi), %xmm9");
	asm("movdqa 16(%rdi), %xmm10");
	asm("movdqa 16(%rdi), %xmm11");
	asm("prefetchnta 256(%rdi)");
	
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
	asm("jz LFindByteEitherOf4DoMainLoop");
	asm("addq %rcx, %rdi");
	asm("addq %rdi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LFindByteEitherOf4Fail");
	asm("retq");
	
	asm("LFindByteEitherOf4DoMainLoop:");
	asm("addq $32, %rdi");
	asm("subq %rdx, %rdi");
	asm("jnc LFindByteEitherOf4Fail");
	
	asm("LFindByteEitherOf4MainLoop:");
	asm("movdqa (%rdx, %rdi), %xmm4");
	asm("movdqa (%rdx, %rdi), %xmm5");
	asm("movdqa (%rdx, %rdi), %xmm6");
	asm("movdqa (%rdx, %rdi), %xmm7");
	asm("movdqa 16(%rdx, %rdi), %xmm8");
	asm("movdqa 16(%rdx, %rdi), %xmm9");
	asm("movdqa 16(%rdx, %rdi), %xmm10");
	asm("movdqa 16(%rdx, %rdi), %xmm11");
	asm("prefetchnta 256(%rdx, %rdi)");
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
	asm("jne LFindByteEitherOf4Found");
	asm("addq $32, %rdi");
	asm("jnc LFindByteEitherOf4MainLoop");
	
	asm("LFindByteEitherOf4Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LFindByteEitherOf4Found:");
	asm("pmovmskb %xmm7, %r10d");
	asm("shll $16, %eax");
	asm("orl %r10d, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rdi, %rax");
	asm("jc LFindByteEitherOf4Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

#define FUNCTION_FIND_BYTE_EITHER_OF_5_DEFINED
__attribute__((naked)) const void* PatternProcessor::FindByteEitherOf5(const void* p, uint64_t v, const void* pEnd)
{
	asm("movl %edi, %ecx");
	asm("andq $-16, %rdi");
	asm("andl $15, %ecx");
	
	asm("movq %rsi, %xmm0");
	asm("movq %rsi, %xmm4");
	asm("punpcklbw %xmm0, %xmm0");
	asm("punpcklbw %xmm4, %xmm4");
	asm("punpcklwd %xmm0, %xmm0");
	asm("punpckhwd %xmm4, %xmm4");
	asm("pshufd $0xff, %xmm0, %xmm3");
	asm("pshufd $0xaa, %xmm0, %xmm2");
	asm("pshufd $0x55, %xmm0, %xmm1");
	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm4, %xmm4");
	
	asm("movdqa (%rdi), %xmm8");
	asm("movdqa (%rdi), %xmm9");
	asm("movdqa (%rdi), %xmm10");
	asm("movdqa (%rdi), %xmm11");
	asm("movdqa (%rdi), %xmm12");
	asm("prefetchnta 128(%rdi)");
	
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
	asm("jz LFindByteEitherOf5DoMainLoop");
	asm("addq %rcx, %rdi");
	asm("addq %rdi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LFindByteEitherOf5Fail");
	asm("retq");
	
	asm("LFindByteEitherOf5DoMainLoop:");
	asm("addq $16, %rdi");
	asm("subq %rdx, %rdi");
	asm("jnc LFindByteEitherOf5Fail");
	
	asm("LFindByteEitherOf5MainLoop:");
	asm("movdqa (%rdx, %rdi), %xmm8");
	asm("movdqa (%rdx, %rdi), %xmm9");
	asm("movdqa (%rdx, %rdi), %xmm10");
	asm("movdqa (%rdx, %rdi), %xmm11");
	asm("movdqa (%rdx, %rdi), %xmm12");
	asm("prefetchnta 128(%rdx, %rdi)");
	
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
	asm("jne LFindByteEitherOf5Found");
	asm("addq $16, %rdi");
	asm("jnc LFindByteEitherOf5MainLoop");
	
	asm("LFindByteEitherOf5Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LFindByteEitherOf5Found:");
	asm("bsfl %eax, %eax");
	asm("addq %rdi, %rax");
	asm("jc LFindByteEitherOf5Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

#define FUNCTION_FIND_BYTE_EITHER_OF_6_DEFINED
__attribute__((naked)) const void* PatternProcessor::FindByteEitherOf6(const void* p, uint64_t v, const void* pEnd)
{
	asm("movl %edi, %ecx");
	asm("andq $-16, %rdi");
	asm("andl $15, %ecx");
	
	asm("movq %rsi, %xmm0");
	asm("movq %rsi, %xmm4");
	asm("punpcklbw %xmm0, %xmm0");
	asm("punpcklbw %xmm4, %xmm4");
	asm("punpcklwd %xmm0, %xmm0");
	asm("punpckhwd %xmm4, %xmm4");
	asm("pshufd $0xff, %xmm0, %xmm3");
	asm("pshufd $0xaa, %xmm0, %xmm2");
	asm("pshufd $0x55, %xmm0, %xmm1");
	asm("pshufd $0x55, %xmm4, %xmm5");
	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm4, %xmm4");
	
	asm("movdqa (%rdi), %xmm8");
	asm("movdqa (%rdi), %xmm9");
	asm("movdqa (%rdi), %xmm10");
	asm("movdqa (%rdi), %xmm11");
	asm("movdqa (%rdi), %xmm12");
	asm("movdqa (%rdi), %xmm13");
	asm("prefetchnta 128(%rdi)");
	
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
	asm("jz LFindByteEitherOf6DoMainLoop");
	asm("addq %rcx, %rdi");
	asm("addq %rdi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LFindByteEitherOf6Fail");
	asm("retq");
	
	asm("LFindByteEitherOf6DoMainLoop:");
	asm("addq $16, %rdi");
	asm("subq %rdx, %rdi");
	asm("jnc LFindByteEitherOf6Fail");
	
	asm("LFindByteEitherOf6MainLoop:");
	asm("movdqa (%rdx, %rdi), %xmm8");
	asm("movdqa (%rdx, %rdi), %xmm9");
	asm("movdqa (%rdx, %rdi), %xmm10");
	asm("movdqa (%rdx, %rdi), %xmm11");
	asm("movdqa (%rdx, %rdi), %xmm12");
	asm("movdqa (%rdx, %rdi), %xmm13");
	asm("prefetchnta 128(%rdx, %rdi)");
	
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
	asm("jne LFindByteEitherOf6Found");
	asm("addq $16, %rdi");
	asm("jnc LFindByteEitherOf6MainLoop");
	
	asm("LFindByteEitherOf6Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LFindByteEitherOf6Found:");
	asm("bsfl %eax, %eax");
	asm("addq %rdi, %rax");
	asm("jc LFindByteEitherOf6Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

#define FUNCTION_FIND_BYTE_EITHER_OF_7_DEFINED
__attribute__((naked)) const void* PatternProcessor::FindByteEitherOf7(const void* p, uint64_t v, const void* pEnd)
{
	asm("movl %edi, %ecx");
	asm("andq $-16, %rdi");
	asm("andl $15, %ecx");
	
	asm("movq %rsi, %xmm0");
	asm("movq %rsi, %xmm4");
	asm("punpcklbw %xmm0, %xmm0");
	asm("punpcklbw %xmm4, %xmm4");
	asm("punpcklwd %xmm0, %xmm0");
	asm("punpckhwd %xmm4, %xmm4");
	asm("pshufd $0xff, %xmm0, %xmm3");
	asm("pshufd $0xaa, %xmm0, %xmm2");
	asm("pshufd $0xaa, %xmm4, %xmm6");
	asm("pshufd $0x55, %xmm0, %xmm1");
	asm("pshufd $0x55, %xmm4, %xmm5");
	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm4, %xmm4");
	
	asm("movdqa (%rdi), %xmm8");
	asm("movdqa (%rdi), %xmm9");
	asm("movdqa (%rdi), %xmm10");
	asm("movdqa (%rdi), %xmm11");
	asm("movdqa (%rdi), %xmm12");
	asm("movdqa (%rdi), %xmm13");
	asm("movdqa (%rdi), %xmm14");
	asm("prefetchnta 128(%rdi)");
	
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
	asm("jz LFindByteEitherOf7DoMainLoop");
	asm("addq %rcx, %rdi");
	asm("addq %rdi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LFindByteEitherOf7Fail");
	asm("retq");
	
	asm("LFindByteEitherOf7DoMainLoop:");
	asm("addq $16, %rdi");
	asm("subq %rdx, %rdi");
	asm("jnc LFindByteEitherOf7Fail");
	
	asm("LFindByteEitherOf7MainLoop:");
	asm("movdqa (%rdx, %rdi), %xmm8");
	asm("movdqa (%rdx, %rdi), %xmm9");
	asm("movdqa (%rdx, %rdi), %xmm10");
	asm("movdqa (%rdx, %rdi), %xmm11");
	asm("movdqa (%rdx, %rdi), %xmm12");
	asm("movdqa (%rdx, %rdi), %xmm13");
	asm("movdqa (%rdx, %rdi), %xmm14");
	asm("prefetchnta 128(%rdx, %rdi)");
	
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
	asm("jne LFindByteEitherOf7Found");
	asm("addq $16, %rdi");
	asm("jnc LFindByteEitherOf7MainLoop");
	
	asm("LFindByteEitherOf7Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LFindByteEitherOf7Found:");
	asm("bsfl %eax, %eax");
	asm("addq %rdi, %rax");
	asm("jc LFindByteEitherOf7Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

#define FUNCTION_FIND_BYTE_EITHER_OF_8_DEFINED
__attribute__((naked)) const void* PatternProcessor::FindByteEitherOf8(const void* p, uint64_t v, const void* pEnd)
{
	asm("movl %edi, %ecx");
	asm("andq $-16, %rdi");
	asm("andl $15, %ecx");
	
	asm("movq %rsi, %xmm0");
	asm("movq %rsi, %xmm4");
	asm("punpcklbw %xmm0, %xmm0");
	asm("punpcklbw %xmm4, %xmm4");
	asm("punpcklwd %xmm0, %xmm0");
	asm("punpckhwd %xmm4, %xmm4");
	asm("pshufd $0xff, %xmm0, %xmm3");
	asm("pshufd $0xff, %xmm4, %xmm7");
	asm("pshufd $0xaa, %xmm0, %xmm2");
	asm("pshufd $0xaa, %xmm4, %xmm6");
	asm("pshufd $0x55, %xmm0, %xmm1");
	asm("pshufd $0x55, %xmm4, %xmm5");
	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm4, %xmm4");
	
	asm("movdqa (%rdi), %xmm8");
	asm("movdqa (%rdi), %xmm9");
	asm("movdqa (%rdi), %xmm10");
	asm("movdqa (%rdi), %xmm11");
	asm("movdqa (%rdi), %xmm12");
	asm("movdqa (%rdi), %xmm13");
	asm("movdqa (%rdi), %xmm14");
	asm("movdqa (%rdi), %xmm15");
	asm("prefetchnta 128(%rdi)");
	
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
	asm("jz LFindByteEitherOf8DoMainLoop");
	asm("addq %rcx, %rdi");
	asm("addq %rdi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LFindByteEitherOf8Fail");
	asm("retq");
	
	asm("LFindByteEitherOf8DoMainLoop:");
	asm("addq $16, %rdi");
	asm("subq %rdx, %rdi");
	asm("jnc LFindByteEitherOf8Fail");
	
	asm("LFindByteEitherOf8MainLoop:");
	asm("movdqa (%rdx, %rdi), %xmm8");
	asm("movdqa (%rdx, %rdi), %xmm9");
	asm("movdqa (%rdx, %rdi), %xmm10");
	asm("movdqa (%rdx, %rdi), %xmm11");
	asm("movdqa (%rdx, %rdi), %xmm12");
	asm("movdqa (%rdx, %rdi), %xmm13");
	asm("movdqa (%rdx, %rdi), %xmm14");
	asm("movdqa (%rdx, %rdi), %xmm15");
	asm("prefetchnta 128(%rdx, %rdi)");
	
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
	asm("jne LFindByteEitherOf8Found");
	asm("addq $16, %rdi");
	asm("jnc LFindByteEitherOf8MainLoop");
	
	asm("LFindByteEitherOf8Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LFindByteEitherOf8Found:");
	asm("bsfl %eax, %eax");
	asm("addq %rdi, %rax");
	asm("jc LFindByteEitherOf8Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

#define FUNCTION_FIND_BYTE_PAIR_DEFINED
__attribute__((naked)) const void* PatternProcessor::FindBytePair(const void* p, uint64_t v, const void* pEnd)
{
	asm("movl %edi, %ecx");
	asm("andq $-32, %rdi");
	asm("andl $31, %ecx");
	
	asm("movq %rsi, %xmm0");
	asm("punpcklbw %xmm0, %xmm0");
	asm("punpcklwd %xmm0, %xmm0");
	asm("pshufd $0x55, %xmm0, %xmm1");
	asm("pshufd $0, %xmm0, %xmm0");
	
	asm("movdqa (%rdi), %xmm2");
	asm("movdqa (%rdi), %xmm3");
	asm("movdqa 16(%rdi), %xmm4");
	asm("movdqa 16(%rdi), %xmm5");
	asm("prefetchnta 256(%rdi)");
	
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
	asm("jz LFindBytePairDoMainLoop");
	asm("addq %rcx, %rdi");
	asm("addq %rdi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LFindBytePairFail");
	asm("subq $1, %rax");
	asm("retq");
	
	asm("LFindBytePairDoMainLoop:");
	asm("addq $32, %rdi");
	asm("subq %rdx, %rdi");
	asm("jnc LFindBytePairFail");
	
	asm("LFindBytePairMainLoop:");
	asm("movdqa (%rdx, %rdi), %xmm2");
	asm("movdqa (%rdx, %rdi), %xmm3");
	asm("movdqa 16(%rdx, %rdi), %xmm4");
	asm("movdqa 16(%rdx, %rdi), %xmm5");
	asm("prefetchnta 256(%rdx, %rdi)");
	
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
	asm("jne LFindBytePairFound");
	asm("addq $32, %rdi");
	asm("jnc LFindBytePairMainLoop");
	
	asm("LFindBytePairFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LFindBytePairFound:");
	asm("bsfl %r9d, %eax");
	asm("subq $1, %rdx");
	asm("addq %rdi, %rax");
	asm("jc LFindBytePairFail");
	asm("addq %rdx, %rax");
	asm("retq");
}

#define FUNCTION_FIND_BYTE_PAIR_2_DEFINED
__attribute__((naked)) const void* PatternProcessor::FindBytePair2(const void* p, uint64_t v, const void* pEnd)
{
	asm("movl %edi, %ecx");
	asm("andq $-32, %rdi");
	asm("andl $31, %ecx");
	
	asm("movq %rsi, %xmm0");
	asm("punpcklbw %xmm0, %xmm0");
	asm("punpcklwd %xmm0, %xmm0");
	asm("pshufd $0xff, %xmm0, %xmm3");
	asm("pshufd $0xaa, %xmm0, %xmm2");
	asm("pshufd $0x55, %xmm0, %xmm1");
	asm("pshufd $0, %xmm0, %xmm0");
	
	asm("movdqa (%rdi), %xmm8");
	asm("movdqa (%rdi), %xmm9");
	asm("movdqa (%rdi), %xmm10");
	asm("movdqa (%rdi), %xmm11");
	asm("movdqa 16(%rdi), %xmm12");
	asm("movdqa 16(%rdi), %xmm13");
	asm("movdqa 16(%rdi), %xmm14");
	asm("movdqa 16(%rdi), %xmm15");
	asm("prefetchnta 256(%rdi)");
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
	asm("orl  %r11d, %r9d");
	asm("lea  (%r10,%r8,2), %rax");	// %eax now contains bit mask of first search byte shift left 1
	
	asm("andl %eax, %r9d");			// Combine the bytes.
	asm("shrq %cl, %r9");
	asm("bsfl %r9d, %eax");
	asm("jz LFindBytePair2DoMainLoop");
	asm("addq %rcx, %rdi");
	asm("addq %rdi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LFindBytePair2Fail");
	asm("subq $1, %rax");
	asm("retq");
	
	asm("LFindBytePair2DoMainLoop:");
	asm("addq $32, %rdi");
	asm("subq %rdx, %rdi");
	asm("jnc LFindBytePair2Fail");
	
	asm("LFindBytePair2MainLoop:");
	asm("movdqa (%rdx, %rdi), %xmm8");
	asm("movdqa (%rdx, %rdi), %xmm9");
	asm("movdqa (%rdx, %rdi), %xmm10");
	asm("movdqa (%rdx, %rdi), %xmm11");
	asm("movdqa 16(%rdx, %rdi), %xmm12");
	asm("movdqa 16(%rdx, %rdi), %xmm13");
	asm("movdqa 16(%rdx, %rdi), %xmm14");
	asm("movdqa 16(%rdx, %rdi), %xmm15");
	asm("prefetchnta 256(%rdx, %rdi)");
	
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
	asm("jne LFindBytePair2Found");
	asm("addq $32, %rdi");
	asm("jnc LFindBytePair2MainLoop");
	
	asm("LFindBytePair2Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LFindBytePair2Found:");
	asm("bsfl %r9d, %eax");
	asm("subq $1, %rdx");
	asm("addq %rdi, %rax");
	asm("jc LFindBytePair2Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

#define FUNCTION_FIND_BYTE_PAIR_3_DEFINED
__attribute__((naked)) const void* PatternProcessor::FindBytePair3(const void* p, uint64_t v, const void* pEnd)
{
	asm("movl %edi, %ecx");
	asm("andq $-16, %rdi");
	asm("andl $15, %ecx");
	
	asm("movq %rsi, %xmm0");
	asm("movq %rsi, %xmm4");
	asm("punpcklbw %xmm0, %xmm0");
	asm("punpcklbw %xmm4, %xmm4");
	asm("punpcklwd %xmm0, %xmm0");
	asm("punpckhwd %xmm4, %xmm4");
	asm("pshufd $0xff, %xmm0, %xmm3");
	asm("pshufd $0xaa, %xmm0, %xmm2");
	asm("pshufd $0x55, %xmm0, %xmm1");
	asm("pshufd $0x55, %xmm4, %xmm5");
	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm4, %xmm4");
	
	asm("movdqa (%rdi), %xmm8");
	asm("movdqa (%rdi), %xmm9");
	asm("movdqa (%rdi), %xmm10");
	asm("movdqa (%rdi), %xmm11");
	asm("movdqa (%rdi), %xmm12");
	asm("movdqa (%rdi), %xmm13");
	asm("prefetchnta 128(%rdi)");
	
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
	asm("lea  (%r8,%r8), %rax");
	
	asm("andl %eax, %r10d");
	asm("shrl %cl, %r10d");
	asm("bsfl %r10d, %eax");
	asm("jz LFindBytePair3DoMainLoop");
	asm("addq %rcx, %rdi");
	asm("addq %rdi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LFindBytePair3Fail");
	asm("subq $1, %rax");
	asm("retq");
	
	asm("LFindBytePair3DoMainLoop:");
	asm("addq $16, %rdi");
	asm("subq %rdx, %rdi");
	asm("jnc LFindBytePair3Fail");
	
	asm("LFindBytePair3MainLoop:");
	asm("movdqa (%rdx, %rdi), %xmm8");
	asm("movdqa (%rdx, %rdi), %xmm9");
	asm("movdqa (%rdx, %rdi), %xmm10");
	asm("movdqa (%rdx, %rdi), %xmm11");
	asm("movdqa (%rdx, %rdi), %xmm12");
	asm("movdqa (%rdx, %rdi), %xmm13");
	asm("prefetchnta 128(%rdx, %rdi)");
	
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
	asm("jne LFindBytePair3Found");
	asm("addq $16, %rdi");
	asm("jnc LFindBytePair3MainLoop");
	
	asm("LFindBytePair3Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LFindBytePair3Found:");
	asm("bsfl %r10d, %eax");
	asm("subq $1, %rdx");
	asm("addq %rdi, %rax");
	asm("jc LFindBytePair3Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

#define FUNCTION_FIND_BYTE_PAIR_4_DEFINED
__attribute__((naked)) const void* PatternProcessor::FindBytePair4(const void* p, uint64_t v, const void* pEnd)
{
	asm("movl %edi, %ecx");
	asm("andq $-16, %rdi");
	asm("andl $15, %ecx");

	asm("movq %rsi, %xmm0");
	asm("movq %rsi, %xmm4");
	asm("punpcklbw %xmm0, %xmm0");
	asm("punpcklbw %xmm4, %xmm4");
	asm("punpcklwd %xmm0, %xmm0");
	asm("punpckhwd %xmm4, %xmm4");
	asm("pshufd $0xff, %xmm0, %xmm3");
	asm("pshufd $0xff, %xmm4, %xmm7");
	asm("pshufd $0xaa, %xmm0, %xmm2");
	asm("pshufd $0xaa, %xmm4, %xmm6");
	asm("pshufd $0x55, %xmm0, %xmm1");
	asm("pshufd $0x55, %xmm4, %xmm5");
	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm4, %xmm4");

	asm("movdqa (%rdi), %xmm8");
	asm("movdqa (%rdi), %xmm9");
	asm("movdqa (%rdi), %xmm10");
	asm("movdqa (%rdi), %xmm11");
	asm("movdqa (%rdi), %xmm12");
	asm("movdqa (%rdi), %xmm13");
	asm("movdqa (%rdi), %xmm14");
	asm("movdqa (%rdi), %xmm15");
	asm("prefetchnta 128(%rdi)");
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
	asm("jz LFindBytePair4DoMainLoop");
	asm("addq %rcx, %rdi");
	asm("addq %rdi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LFindBytePair4Fail");
	asm("subq $1, %rax");
	asm("retq");
	
	asm("LFindBytePair4DoMainLoop:");
	asm("addq $16, %rdi");
	asm("subq %rdx, %rdi");
	asm("jnc LFindBytePair4Fail");
	
	asm("LFindBytePair4MainLoop:");
	asm("movdqa (%rdx, %rdi), %xmm8");
	asm("movdqa (%rdx, %rdi), %xmm9");
	asm("movdqa (%rdx, %rdi), %xmm10");
	asm("movdqa (%rdx, %rdi), %xmm11");
	asm("movdqa (%rdx, %rdi), %xmm12");
	asm("movdqa (%rdx, %rdi), %xmm13");
	asm("movdqa (%rdx, %rdi), %xmm14");
	asm("movdqa (%rdx, %rdi), %xmm15");
	asm("prefetchnta 128(%rdx, %rdi)");
	
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
	asm("jne LFindBytePair4Found");
	asm("addq $16, %rdi");
	asm("jnc LFindBytePair4MainLoop");
	
	asm("LFindBytePair4Fail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LFindBytePair4Found:");
	asm("bsfl %r10d, %eax");
	asm("subq $1, %rdx");
	asm("addq %rdi, %rax");
	asm("jc LFindBytePair4Fail");
	asm("addq %rdx, %rax");
	asm("retq");
}

#define FUNCTION_FIND_BYTE_RANGE_DEFINED
__attribute__((naked)) const void* PatternProcessor::FindByteRange(const void* p, uint64_t v, const void* pEnd)
{
	asm("movl %edi, %ecx");
	asm("andq $-32, %rdi");
	asm("andl $31, %ecx");
	
	asm("movdqa LFindByteRangeXmm126Constant(%rip), %xmm6");
	asm("movdqa LFindByteRangeXmm127Constant(%rip), %xmm7");
	
	asm("movq %rsi, %xmm0");
	asm("punpcklbw %xmm0, %xmm0");
	asm("punpcklwd %xmm0, %xmm0");
	asm("pshufd $0x55, %xmm0, %xmm1");
	asm("pshufd $0, %xmm0, %xmm0");

	asm("psubb  %xmm1, %xmm6");
	asm("psubb  %xmm1, %xmm7");
	asm("paddb  %xmm0, %xmm6");
	
	asm("movdqa (%rdi), %xmm2");
	asm("movdqa 16(%rdi), %xmm3");
	asm("prefetchnta 256(%rdi)");
	asm("paddb   %xmm7, %xmm2");
	asm("paddb   %xmm7, %xmm3");
	asm("pcmpgtb %xmm6, %xmm2");
	asm("pcmpgtb %xmm6, %xmm3");
	asm("pmovmskb %xmm2, %eax");
	asm("pmovmskb %xmm3, %r10d");
	asm("shll $16, %r10d");
	asm("orl %r10d, %eax");
	asm("shrl %cl, %eax");
	asm("bsfl %eax, %eax");
	asm("jz LFindByteRangeDoMainLoop");
	asm("addq %rcx, %rdi");
	asm("addq %rdi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LFindByteRangeFail");
	asm("retq");
	
	asm("LFindByteRangeDoMainLoop:");
	asm("addq $32, %rdi");
	asm("subq %rdx, %rdi");
	asm("jnc LFindByteRangeFail");
	
	asm("LFindByteRangeMainLoop:");
	asm("movdqa (%rdx, %rdi), %xmm2");
	asm("movdqa 16(%rdx, %rdi), %xmm3");
	asm("prefetchnta 256(%rdx, %rdi)");
	
	asm("paddb   %xmm7, %xmm2");
	asm("paddb   %xmm7, %xmm3");
	asm("pcmpgtb %xmm6, %xmm2");
	asm("pcmpgtb %xmm6, %xmm3");
	asm("por     %xmm2, %xmm3");
	asm("pmovmskb %xmm3, %eax");
	asm("test %eax, %eax");
	asm("jne LFindByteRangeFound");
	asm("addq $32, %rdi");
	asm("jnc LFindByteRangeMainLoop");
	
	asm("LFindByteRangeFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LFindByteRangeFound:");
	asm("pmovmskb %xmm2, %r10d");
	asm("shll $16, %eax");
	asm("orl %r10d, %eax");
	asm("bsfl %eax, %eax");
	asm("addq %rdi, %rax");
	asm("jc LFindByteRangeFail");
	asm("addq %rdx, %rax");
	asm("retq");
	
	asm(".align 4");
	asm("LFindByteRangeXmm126Constant:");
	asm(".quad 0x7e7e7e7e7e7e7e7e");
	asm(".quad 0x7e7e7e7e7e7e7e7e");
	asm("LFindByteRangeXmm127Constant:");
	asm(".quad 0x7f7f7f7f7f7f7f7f");
	asm(".quad 0x7f7f7f7f7f7f7f7f");
}

#define FUNCTION_FIND_BYTE_RANGE_PAIR_DEFINED
__attribute__((naked)) const void* PatternProcessor::FindByteRangePair(const void* p, uint64_t v, const void* pEnd)
{
	asm("movl %edi, %ecx");
	asm("andq $-32, %rdi");
	asm("andl $31, %ecx");
	
	asm("movdqa LFindByteRangePairXmm126Constant(%rip), %xmm12");
	asm("movdqa LFindByteRangePairXmm127Constant(%rip), %xmm13");
	asm("movdqa LFindByteRangePairXmm126Constant(%rip), %xmm14");
	asm("movdqa LFindByteRangePairXmm127Constant(%rip), %xmm15");
	
	asm("movq %rsi, %xmm0");
	asm("punpcklbw %xmm0, %xmm0");
	asm("punpcklwd %xmm0, %xmm0");
	asm("pshufd $0xff, %xmm0, %xmm3");		// xmm3 = high byte 2
	asm("pshufd $0xaa, %xmm0, %xmm2");		// xmm2 = low byte 2
	asm("pshufd $0x55, %xmm0, %xmm1");		// xmm1 = high
	asm("pshufd $0, %xmm0, %xmm0");			// xmm0 = low

	asm("psubb  %xmm1, %xmm12");
	asm("psubb  %xmm1, %xmm13");
	asm("psubb  %xmm3, %xmm14");
	asm("psubb  %xmm3, %xmm15");
	asm("paddb  %xmm0, %xmm12");
	asm("paddb  %xmm2, %xmm14");
	
	asm("movdqa (%rdi), %xmm4");
	asm("movdqa (%rdi), %xmm5");
	asm("movdqa 16(%rdi), %xmm6");
	asm("movdqa 16(%rdi), %xmm7");
	asm("prefetchnta 256(%rdi)");
	asm("paddb %xmm13, %xmm4");
	asm("paddb %xmm15, %xmm5");
	asm("paddb %xmm13, %xmm6");
	asm("paddb %xmm15, %xmm7");
	asm("pcmpgtb %xmm12, %xmm4");
	asm("pcmpgtb %xmm14, %xmm5");
	asm("pcmpgtb %xmm12, %xmm6");
	asm("pcmpgtb %xmm14, %xmm7");
	
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
	asm("jz LFindByteRangePairDoMainLoop");
	asm("addq %rcx, %rdi");
	asm("addq %rdi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LFindByteRangePairFail");
	asm("subq $1, %rax");
	asm("retq");
	
	asm("LFindByteRangePairDoMainLoop:");
	asm("addq $32, %rdi");				// Advance to next block
	asm("subq %rdx, %rdi");
	asm("jnc LFindByteRangePairFail");
	
	asm("LFindByteRangePairMainLoop:");
	asm("movdqa (%rdx, %rdi), %xmm4");
	asm("movdqa (%rdx, %rdi), %xmm5");
	asm("movdqa 16(%rdx, %rdi), %xmm6");
	asm("movdqa 16(%rdx, %rdi), %xmm7");
	asm("prefetchnta 256(%rdx, %rdi)");
	
	asm("shrq $32, %rax");
	
	asm("paddb %xmm13, %xmm4");
	asm("paddb %xmm15, %xmm5");
	asm("paddb %xmm13, %xmm6");
	asm("paddb %xmm15, %xmm7");
	asm("pcmpgtb %xmm12, %xmm4");
	asm("pcmpgtb %xmm14, %xmm5");
	asm("pcmpgtb %xmm12, %xmm6");
	asm("pcmpgtb %xmm14, %xmm7");
	
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
	asm("jne LFindByteRangePairFound");
	asm("addq $32, %rdi");
	asm("jnc LFindByteRangePairMainLoop");
	
	asm("LFindByteRangePairFail:");
	asm("xor %eax, %eax");
	asm("retq");
	
	asm("LFindByteRangePairFound:");
	asm("bsfl %r9d, %eax");
	asm("subq $1, %rdx");
	asm("addq %rdi, %rax");
	asm("jc LFindByteRangePairFail");
	asm("addq %rdx, %rax");
	asm("retq");

	asm(".align 4");
	asm("LFindByteRangePairXmm126Constant:");
	asm(".quad 0x7e7e7e7e7e7e7e7e");
	asm(".quad 0x7e7e7e7e7e7e7e7e");
	asm("LFindByteRangePairXmm127Constant:");
	asm(".quad 0x7f7f7f7f7f7f7f7f");
	asm(".quad 0x7f7f7f7f7f7f7f7f");
}

#define FUNCTION_FIND_SHIFT_OR_DEFINED
__attribute__((naked)) const void* PatternProcessor::FindShiftOr(const void* p, const ByteCodeSearchData* searchData, const void* pEnd)
{
	asm("subq $8, %rdx");		// %rdx = end
	asm("movl (%rsi), %ecx");	// %ecx = data->offset
	asm("movl $-1, %eax");		// %eax = state
	asm("addq $4, %rsi");		// %rsi now points to data
	
	asm("cmpq %rdx, %rdi");
	asm("jae  LFindShiftOrLoopFinished");
	
	asm("pushq %r12");
	asm("pushq %r13");
	asm("pushq %r14");
	asm("pushq %r15");
	
	asm("LFindShiftOrLoop:");
	asm("movzbq (%rdi), %r8");
	asm("movzbq 1(%rdi), %r9");
	asm("movzbq 2(%rdi), %r10");
	asm("movzbq 3(%rdi), %r11");
	asm("movzbq 4(%rdi), %r12");
	asm("movzbq 5(%rdi), %r13");
	asm("movzbq 6(%rdi), %r14");
	asm("movzbq 7(%rdi), %r15");
	
	asm("movl (%rsi,%r8,4), %r8d");
	asm("movl (%rsi,%r9,4), %r9d");
	asm("movl (%rsi,%r10,4), %r10d");
	asm("movl (%rsi,%r11,4), %r11d");
	asm("movl (%rsi,%r12,4), %r12d");
	asm("movl (%rsi,%r13,4), %r13d");
	asm("movl (%rsi,%r14,4), %r14d");
	asm("movl (%rsi,%r15,4), %r15d");
	
	asm("addq $8, %rdi");

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
	asm("cmpq %rdx, %rdi");
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
	asm("movzbl (%rdi), %r9d");
	asm("movzbl (%rsi,%r9,4), %r9d");
	asm("addq $1, %rdi");
	asm("shll $1, %eax");
	asm("orl %r9d, %eax");
	asm("test %r8d, %eax");
	asm("jz LFindShiftOrSingleByteFound");
	asm("cmpq %rdx, %rdi");
	asm("jb LFindShiftOrSingleByteLoop");
	asm("xorl %eax, %eax");
	asm("retq");
	
	asm("LFindShiftOrSingleByteFound:");
	asm("leaq -1(%rdi), %rax");
	asm("subq %rcx, %rax");
	asm("retq");

	asm("LFindShiftOrFound:");
	asm("leaq -1(%rdi), %rax");
	asm("bsrl %r15d, %edx");
	asm("subq %rcx, %rax");
	asm("subq %rdx, %rax");
	asm("popq %r15");
	asm("popq %r14");
	asm("popq %r13");
	asm("popq %r12");
	asm("retq");
	
	asm("LFindShiftOrFail:");
	asm("xorl %eax, %eax");
	asm("retq");
}

#define FUNCTION_FIND_BYTE_TRIPLET_DEFINED
__attribute__((naked)) const void* PatternProcessor::FindByteTriplet(const void* p, uint64_t v, const void* pEnd)
{
	asm("movl %edi, %ecx");
	asm("andq $-32, %rdi");
	asm("andl $31, %ecx");

	asm("movq %rsi, %xmm0");
	asm("punpcklbw %xmm0, %xmm0");
	asm("punpcklwd %xmm0, %xmm0");
	asm("pshufd $0xaa, %xmm0, %xmm2");
	asm("pshufd $0x55, %xmm0, %xmm1");
	asm("pshufd $0, %xmm0, %xmm0");
	
	asm("pushq %rbx");
	asm("pushq %r12");
	asm("pushq %r13");
	
	asm("movdqa (%rdi), %xmm3");
	asm("movdqa (%rdi), %xmm4");
	asm("movdqa (%rdi), %xmm5");
	asm("movdqa 16(%rdi), %xmm6");
	asm("movdqa 16(%rdi), %xmm7");
	asm("movdqa 16(%rdi), %xmm8");
	asm("prefetchnta 256(%rdi)");
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
	asm("jz LFindByteTripletDoMainLoop");
	asm("addq %rcx, %rdi");
	asm("addq %rdi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LFindByteTripletFail");
	asm("subq $2, %rax");
	
	asm("popq %r13");
	asm("popq %r12");
	asm("popq %rbx");
	asm("retq");
	
	asm("LFindByteTripletDoMainLoop:");
	asm("shrq %cl, %rax");
	asm("addq $32, %rdi");
	asm("shlq %cl, %rax");
	asm("subq %rdx, %rdi");
	asm("jnc LFindByteTripletFail");
	
	asm("LFindByteTripletMainLoop:");
	asm("movdqa (%rdx, %rdi), %xmm3");
	asm("movdqa (%rdx, %rdi), %xmm4");
	asm("movdqa (%rdx, %rdi), %xmm5");
	asm("movdqa 16(%rdx, %rdi), %xmm6");
	asm("movdqa 16(%rdx, %rdi), %xmm7");
	asm("movdqa 16(%rdx, %rdi), %xmm8");
	asm("prefetchnta 256(%rdx, %rdi)");
	
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
	asm("jne LFindByteTripletFound");
	asm("addq $32, %rdi");
	asm("jnc LFindByteTripletMainLoop");
	
	asm("LFindByteTripletFail:");
	asm("xor %eax, %eax");
	asm("popq %r13");
	asm("popq %r12");
	asm("popq %rbx");
	asm("retq");
	
	asm("LFindByteTripletFound:");
	asm("bsfl %r10d, %eax");
	asm("subq $2, %rdx");
	asm("addq %rdi, %rax");
	asm("jc LFindByteTripletFail");
	asm("addq %rdx, %rax");
	asm("popq %r13");
	asm("popq %r12");
	asm("popq %rbx");
	asm("retq");
}

#define FUNCTION_FIND_BYTE_TRIPLET_2_DEFINED
__attribute__((naked)) const void* PatternProcessor::FindByteTriplet2(const void* p, uint64_t v, const void* pEnd)
{
	asm("movl %edi, %ecx");
	asm("andq $-16, %rdi");
	asm("andl $15, %ecx");
	
	asm("movq %rsi, %xmm0");
	asm("movq %rsi, %xmm4");
	asm("punpcklbw %xmm0, %xmm0");
	asm("punpcklbw %xmm4, %xmm4");
	asm("punpcklwd %xmm0, %xmm0");
	asm("punpckhwd %xmm4, %xmm4");
	asm("pshufd $0xff, %xmm0, %xmm3");
	asm("pshufd $0xaa, %xmm0, %xmm2");
	asm("pshufd $0x55, %xmm0, %xmm1");
	asm("pshufd $0x55, %xmm4, %xmm5");
	asm("pshufd $0, %xmm0, %xmm0");
	asm("pshufd $0, %xmm4, %xmm4");
	
	asm("pushq %rbx");
	
	asm("movdqa (%rdi), %xmm8");
	asm("movdqa (%rdi), %xmm9");
	asm("movdqa (%rdi), %xmm10");
	asm("movdqa (%rdi), %xmm11");
	asm("movdqa (%rdi), %xmm12");
	asm("movdqa (%rdi), %xmm13");
	asm("prefetchnta 256(%rdi)");

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
	asm("jz LFindByteTriplet2DoMainLoop");
	asm("addq %rcx, %rdi");
	asm("addq %rdi, %rax");
	asm("cmpq %rdx, %rax");
	asm("jae LFindByteTriplet2Fail");
	asm("subq $2, %rax");
	
	asm("popq %rbx");
	asm("retq");
	
	asm("LFindByteTriplet2DoMainLoop:");
	asm("shrq %cl, %rax");
	asm("addq $16, %rdi");
	asm("shlq %cl, %rax");
	asm("subq %rdx, %rdi");
	asm("jnc LFindByteTriplet2Fail");
	
	asm("LFindByteTriplet2MainLoop:");
	asm("movdqa (%rdx, %rdi), %xmm8");
	asm("movdqa (%rdx, %rdi), %xmm9");
	asm("movdqa (%rdx, %rdi), %xmm10");
	asm("movdqa (%rdx, %rdi), %xmm11");
	asm("movdqa (%rdx, %rdi), %xmm12");
	asm("movdqa (%rdx, %rdi), %xmm13");
	asm("prefetchnta 256(%rdx, %rdi)");
	
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
	asm("jne LFindByteTriplet2Found");
	asm("addq $16, %rdi");
	asm("jnc LFindByteTriplet2MainLoop");
	
	asm("LFindByteTriplet2Fail:");
	asm("xor %eax, %eax");
	asm("popq %rbx");
	asm("retq");
	
	asm("LFindByteTriplet2Found:");
	asm("bsfl %r10d, %eax");
	asm("subq $2, %rdx");
	asm("addq %rdi, %rax");
	asm("jc LFindByteTriplet2Fail");
	asm("addq %rdx, %rax");
	asm("popq %rbx");
	asm("retq");
}

//============================================================================
