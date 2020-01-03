CC=clang++
ODIR=build
CPPFLAGS=-O3 -I. -std=c++17 -fomit-frame-pointer -DJBUILDCONFIG_FINAL -DNDEBUG

ASSEMBLER_SOURCES = Javelin/Assembler/JitForwardReferenceMap.cpp \
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
		    Javelin/Assembler/x64/Assembler.cpp

JASM_SOURCES = Javelin/Assembler/arm64/ActionType.cpp \
	       Javelin/Assembler/JitLabelId.cpp \
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
	       Javelin/Tools/jasm/x64/Action.cpp \
	       Javelin/Tools/jasm/x64/Assembler.cpp \
	       Javelin/Tools/jasm/x64/Encoder.cpp \
	       Javelin/Tools/jasm/x64/Instruction.cpp \
	       Javelin/Tools/jasm/x64/InstructionData.cpp \
	       Javelin/Tools/jasm/x64/Register.cpp \
	       Javelin/Tools/jasm/x64/Token.cpp \
	       Javelin/Tools/jasm/x64/Tokenizer.cpp \
	       Javelin/Tools/jasm/arm64/Action.cpp \
	       Javelin/Tools/jasm/arm64/Assembler.cpp \
	       Javelin/Tools/jasm/arm64/Encoder.cpp \
	       Javelin/Tools/jasm/arm64/Instruction.cpp \
	       Javelin/Tools/jasm/arm64/InstructionData.cpp \
	       Javelin/Tools/jasm/arm64/Register.cpp \
	       Javelin/Tools/jasm/arm64/Token.cpp \
	       Javelin/Tools/jasm/arm64/Tokenizer.cpp

help:
	@echo
	@echo "[1mMake Targets[m"
	@echo
	@echo "all               - Build all targets"
	@echo "clean             - Clean all targets"
	@echo "assembler-library - Build libJavelinAssembler"
	@echo "jasm              - Build jasm"
	@echo

.PHONY : all clean assembler-library jasm
.PRECIOUS: $(ODIR)/DerivedSources/%.cpp

all: jasm assembler-library

clean:
	@echo "Removing [31;1m$(ODIR)[m"
	-@rm -rf $(ODIR)

$(ODIR)/Objects/%.o: %.cpp $(PATTERN_HEADERS)
	@echo "Compiling: [34;1m$<[m"
	@mkdir -p $(basename $@)
	@$(CC) -c -o $@ $< $(CPPFLAGS) 

$(ODIR)/Objects/%.o: $(patsubst %.cpp.jasm,$(ODIR)/DerivedSources/%.cpp,$(PATTERN_JASM_SOURCES)) $(PATTERN_HEADERS)
	@echo "Compiling: [34;1m$<[m"
	@mkdir -p $(basename $@)
	@$(CC) -c -o $@ $< $(CPPFLAGS) 

$(ODIR)/DerivedSources/%.cpp: %.cpp.jasm $(ODIR)/jasm
	@echo "jasm: [34;1m$<[m"
	@mkdir -p $(basename $@)
	@$(ODIR)/jasm -o $@ -f $<

$(ODIR)/libJavelinAssembler.a: $(patsubst %.cpp,$(ODIR)/Objects/%.o,$(ASSEMBLER_SOURCES))
	@echo "Linking: [34;1m$@[m"
	@ar rcs $@ $^ 

$(ODIR)/libJavelinPattern.a: $(patsubst %.cpp,$(ODIR)/Objects/%.o,$(PATTERN_SOURCES)) $(patsubst %.cpp,$(ODIR)/Objects/%.o,$(ASSEMBLER_SOURCES)) $(patsubst %.cpp.jasm,$(ODIR)/Objects/%.o,$(PATTERN_JASM_SOURCES))
	@echo "Linking: [34;1m$@[m"
	@ar rcs $@ $^ 

$(ODIR)/jasm: $(patsubst %.cpp,$(ODIR)/Objects/%.o,$(JASM_SOURCES))
	@echo "Linking: [34;1m$@[m"
	@$(CC) -o $@ $^

jasm: $(ODIR)/jasm

assembler-library: $(ODIR)/libJavelinAssembler.a

