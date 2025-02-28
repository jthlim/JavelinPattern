CC=clang++
ODIR=build
CPPFLAGS=-O3 -I. -std=c++17 -fomit-frame-pointer -DJBUILDCONFIG_FINAL -DNDEBUG

ASSEMBLER_SOURCES = \
		    Javelin/Assembler/BitUtilty.cpp \
			 	Javelin/Assembler/JitForwardReferenceMap.cpp \
		    Javelin/Assembler/JitLabelId.cpp \
		    Javelin/Assembler/JitLabelMap.cpp \
		    Javelin/Assembler/JitLabelOffsetQueue.cpp \
		    Javelin/Assembler/JitMemoryManager.cpp \
		    Javelin/Assembler/JitVector.cpp \
		    Javelin/Assembler/JitForwardReferenceMap.cpp \
		    Javelin/Assembler/JitLabelId.cpp \
		    Javelin/Assembler/JitLabelMap.cpp \
		    Javelin/Assembler/JitLabelOffsetQueue.cpp \
		    Javelin/Assembler/JitMemoryManager.cpp \
		    Javelin/Assembler/JitVector.cpp \
		    Javelin/Assembler/arm64/ActionType.cpp \
		    Javelin/Assembler/arm64/Assembler.cpp \
		    Javelin/Assembler/riscv/Assembler.cpp \
		    Javelin/Assembler/x64/Assembler.cpp \

JASM_SOURCES = \
	       Javelin/Assembler/BitUtility.cpp \
	       Javelin/Assembler/JitLabelId.cpp \
				 Javelin/Assembler/arm64/ActionType.cpp \
	       Javelin/Tools/jasm/AssemblerException.cpp \
	       Javelin/Tools/jasm/Character.cpp \
	       Javelin/Tools/jasm/CodeSegment.cpp \
	       Javelin/Tools/jasm/CodeSegmentSource.cpp \
	       Javelin/Tools/jasm/CommandLine.cpp \
	       Javelin/Tools/jasm/File.cpp \
	       Javelin/Tools/jasm/Log.cpp \
	       Javelin/Tools/jasm/Preprocessor.cpp \
	       Javelin/Tools/jasm/SourceFileSegments.cpp \
	       Javelin/Tools/jasm/main.cpp \
	       Javelin/Tools/jasm/Common/Action.cpp \
	       Javelin/Tools/jasm/Common/JumpType.cpp \
	       Javelin/Tools/jasm/arm64/Action.cpp \
	       Javelin/Tools/jasm/arm64/Assembler.cpp \
	       Javelin/Tools/jasm/arm64/Encoder.cpp \
	       Javelin/Tools/jasm/arm64/Instruction.cpp \
	       Javelin/Tools/jasm/arm64/InstructionData.cpp \
	       Javelin/Tools/jasm/arm64/Register.cpp \
	       Javelin/Tools/jasm/arm64/Token.cpp \
	       Javelin/Tools/jasm/arm64/Tokenizer.cpp \
	       Javelin/Tools/jasm/riscv/Action.cpp \
	       Javelin/Tools/jasm/riscv/Assembler.cpp \
	       Javelin/Tools/jasm/riscv/Encoder.cpp \
	       Javelin/Tools/jasm/riscv/Instruction.cpp \
	       Javelin/Tools/jasm/riscv/InstructionData.cpp \
	       Javelin/Tools/jasm/riscv/Register.cpp \
	       Javelin/Tools/jasm/riscv/RoundingMode.cpp \
	       Javelin/Tools/jasm/riscv/Token.cpp \
	       Javelin/Tools/jasm/riscv/Tokenizer.cpp \
	       Javelin/Tools/jasm/x64/Action.cpp \
	       Javelin/Tools/jasm/x64/Assembler.cpp \
	       Javelin/Tools/jasm/x64/Encoder.cpp \
	       Javelin/Tools/jasm/x64/Instruction.cpp \
	       Javelin/Tools/jasm/x64/InstructionData.cpp \
	       Javelin/Tools/jasm/x64/Register.cpp \
	       Javelin/Tools/jasm/x64/Token.cpp \
	       Javelin/Tools/jasm/x64/Tokenizer.cpp \

PATTERN_JASM_SOURCES = \
				Javelin/Pattern/Internal/Amd64PatternProcessors.cpp.jasm \
				Javelin/Pattern/Internal/Amd64ReverseProcessors.cpp.jasm \
				Javelin/Pattern/Internal/Arm64PatternProcessors.cpp.jasm \
				Javelin/Pattern/Internal/Arm64ReverseProcessor.cpp.jasm \

PATTERN_SOURCES = \
				Javelin/JavelinStatics.cpp \
        Javelin/Allocators/FreeListAllocator.cpp \
        Javelin/Container/BitTable.cpp \
        Javelin/Container/ConcurrentQueue.cpp \
        Javelin/Container/List.cpp \
        Javelin/Cryptography/Crc32.cpp \
        Javelin/Cryptography/Crc64.cpp \
        Javelin/Data/Utf8DecodeTable.cpp \
        Javelin/Math/BitUtility.cpp \
        Javelin/Pattern/JavelinPattern.cpp \
        Javelin/Pattern/Pattern.cpp \
        Javelin/Pattern/Internal/Amd64DfaPatternProcessor.cpp \
        Javelin/Pattern/Internal/Amd64PatternProcessors.cpp \
        Javelin/Pattern/Internal/Amd64PatternProcessorsFindMethods.cpp \
        Javelin/Pattern/Internal/ArmNeonDfaPatternProcessor.cpp \
        Javelin/Pattern/Internal/ArmNeonPatternProcessorsFindMethods.cpp \
        Javelin/Pattern/Internal/AnchoredReverseProcessor.cpp \
        Javelin/Pattern/Internal/BackTrackingPatternProcessor.cpp \
        Javelin/Pattern/Internal/BackTrackingReverseProcessor.cpp \
        Javelin/Pattern/Internal/BitFieldGlushkovNfaPatternProcessor.cpp \
        Javelin/Pattern/Internal/BitFieldGlushkovNfaReverseProcessor.cpp \
        Javelin/Pattern/Internal/BitStateBackTrackingPatternProcessor.cpp \
        Javelin/Pattern/Internal/ConsistencyCheckPatternProcessor.cpp \
        Javelin/Pattern/Internal/DfaPatternProcessor.cpp \
        Javelin/Pattern/Internal/DfaProcessorBase.cpp \
        Javelin/Pattern/Internal/DfaReverseProcessor.cpp \
        Javelin/Pattern/Internal/FixedLengthReverseProcessor.cpp \
        Javelin/Pattern/Internal/NfaOrBitStateBackTrackingPatternProcessor.cpp \
        Javelin/Pattern/Internal/NfaOrDfaPatternProcessor.cpp \
        Javelin/Pattern/Internal/NfaOrDfaReverseProcessor.cpp \
        Javelin/Pattern/Internal/NfaPatternProcessor.cpp \
        Javelin/Pattern/Internal/OnePassPatternProcessor.cpp \
        Javelin/Pattern/Internal/OnePassReverseProcessor.cpp \
        Javelin/Pattern/Internal/PatternByteCode.cpp \
        Javelin/Pattern/Internal/PatternCaseFold.cpp \
        Javelin/Pattern/Internal/PatternCompiler.cpp \
        Javelin/Pattern/Internal/PatternComponent.cpp \
        Javelin/Pattern/Internal/PatternData.cpp \
        Javelin/Pattern/Internal/PatternDfaMemoryManager.cpp \
        Javelin/Pattern/Internal/PatternDfaState.cpp \
        Javelin/Pattern/Internal/PatternInstruction.cpp \
        Javelin/Pattern/Internal/PatternInstructionList.cpp \
        Javelin/Pattern/Internal/PatternInstructionWalker.cpp \
        Javelin/Pattern/Internal/PatternNfaState.cpp \
        Javelin/Pattern/Internal/PatternNibbleMask.cpp \
        Javelin/Pattern/Internal/PatternProcessor.cpp \
        Javelin/Pattern/Internal/PatternProcessorBase.cpp \
        Javelin/Pattern/Internal/PatternReverseProcessor.cpp \
        Javelin/Pattern/Internal/PatternTokenizer.cpp \
        Javelin/Pattern/Internal/PatternTokenizerGlob.cpp \
        Javelin/Pattern/Internal/PatternTypes.cpp \
        Javelin/Pattern/Internal/PikeNfaPatternProcessor.cpp \
        Javelin/Pattern/Internal/ScanAndCapturePatternProcessor.cpp \
        Javelin/Pattern/Internal/SimplePikeNfaPatternProcessor.cpp \
        Javelin/Pattern/Internal/ThompsonNfaPatternProcessor.cpp \
        Javelin/Pattern/Internal/ThompsonNfaReverseProcessor.cpp \
        Javelin/Stream/CountWriter.cpp \
        Javelin/Stream/DataBlockWriter.cpp \
        Javelin/Stream/ICharacterWriter.cpp \
        Javelin/Stream/IWriter.cpp \
        Javelin/Stream/StandardWriter.cpp \
        Javelin/Stream/StringWriter.cpp \
        Javelin/System/Assert.cpp \
        Javelin/System/Machine.cpp \
        Javelin/System/StackWalk.cpp \
        Javelin/Thread/RecursiveMutex.cpp \
        Javelin/Thread/Semaphore.cpp \
        Javelin/Thread/Task.cpp \
        Javelin/Thread/Thread.cpp \
        Javelin/Thread/ThreadPool.cpp \
        Javelin/Type/Character.cpp \
        Javelin/Type/DataBlock.cpp \
        Javelin/Type/Function.cpp \
        Javelin/Type/String.cpp \
        Javelin/Type/Url.cpp \
        Javelin/Type/Utf8Character.cpp \
        Javelin/Type/Utf8Pointer.cpp \
        Javelin/Type/VariableName.cpp \
				$(ASSEMBLER_SOURCES) \
				$(patsubst %.cpp.jasm,$(ODIR)/DerivedSources/%.cpp,$(PATTERN_JASM_SOURCES))

help:
	@echo
	@echo "[1mMake Targets[m"
	@echo
	@echo "all               - Build all targets"
	@echo "clean             - Clean all targets"
	@echo "assembler-library - Build libJavelinAssembler"
	@echo "pattern-library   - Build libJavelinPattern"
	@echo "jasm              - Build jasm"
	@echo

.PHONY : all clean jasm assembler-library pattern-library example
.PRECIOUS: $(ODIR)/DerivedSources/%.cpp

all: jasm assembler-library pattern-library example

clean:
	@echo "Removing [31;1m$(ODIR)[m"
	-@rm -rf $(ODIR)

$(ODIR)/Objects/%.o: %.cpp
	@echo "Compiling: [34;1m$<[m"
	@mkdir -p $(dir $@)
	@$(CC) -c -o $@ $< $(CPPFLAGS) 

$(ODIR)/Objects/%.o: $(patsubst %.cpp.jasm,$(ODIR)/DerivedSources/%.cpp,$(PATTERN_JASM_SOURCES))
	@echo "Compiling: [34;1m$<[m"
	@mkdir -p $(dir $@)
	@$(CC) -c -o $@ $< $(CPPFLAGS) 

$(ODIR)/DerivedSources/%.cpp: %.cpp.jasm $(ODIR)/jasm
	@echo "jasm: [34;1m$<[m"
	@mkdir -p $(dir $@)
	@$(ODIR)/jasm -o $@ -f $<

$(ODIR)/libJavelinAssembler.a: $(patsubst %.cpp,$(ODIR)/Objects/%.o,$(ASSEMBLER_SOURCES))
	@echo "Linking: [32;1m$@[m"
	@ar rcs $@ $^ 

$(ODIR)/libJavelinPattern.a: $(patsubst %.cpp,$(ODIR)/Objects/%.o,$(PATTERN_SOURCES))
	@echo "Linking: [32;1m$@[m"
	@ar rcs $@ $^ 

$(ODIR)/jasm: $(patsubst %.cpp,$(ODIR)/Objects/%.o,$(JASM_SOURCES))
	@echo "Linking: [32;1m$@[m"
	@$(CC) -o $@ $^

jasm: $(ODIR)/jasm

assembler-library: $(ODIR)/libJavelinAssembler.a

pattern-library: $(ODIR)/libJavelinPattern.a

example: pattern-library
	@echo "Creating: [32;1m$(ODIR)/example[m"
	@$(CC) example.c -l JavelinPattern -L $(ODIR) -o $(ODIR)/example $(CPPFLAGS) 
