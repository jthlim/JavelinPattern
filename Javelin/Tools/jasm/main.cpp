//============================================================================

#include "Javelin/Tools/jasm/AssemblerException.h"
#include "Javelin/Tools/jasm/CommandLine.h"
#include "Javelin/Tools/jasm/File.h"
#include "Javelin/Tools/jasm/Log.h"
#include "Javelin/Tools/jasm/Preprocessor.h"
#include "Javelin/Tools/jasm/SourceFileSegments.h"

#include "Javelin/Tools/jasm/arm64/Assembler.h"
#include "Javelin/Tools/jasm/riscv/Assembler.h"
#include "Javelin/Tools/jasm/x64/Assembler.h"

#include <unistd.h>

//============================================================================

using namespace Javelin::Assembler;

//============================================================================

class Main
{
public:
	Main(int argc, const char** argv) : commandLine(argc, argv) { }

	int Run();
	
private:
	CommandLine commandLine;
	SourceFileSegments fileSegments;

	FILE* CreateOutputFile();
	void ReadInputFile();
	void PreprocessFileSegments();
	void ProcessFileSegments(FILE *outputFile);
};

FILE* Main::CreateOutputFile()
{
	if(!commandLine.GetOutputFilename())
	{
		return stdout;
	}
	else
	{
		FILE *outputFile = fopen(commandLine.GetOutputFilename(), "wt");
		if(!outputFile)
		{
			Log::Error("Unable to open output file: %s", commandLine.GetOutputFilename());
			exit(2);
		}
		return outputFile;
	}

}

// Reads input file and handles .include
void Main::ReadInputFile()
{
	File file(commandLine.GetInputFilename());
	if(!file.IsValid())
	{
		Log::Error("Unable top open input file: %s", commandLine.GetInputFilename());
		exit(1);
	}
	fileSegments.AddSegmentsFromFile(file);
	
	if(Log::IsVerboseEnabled())
	{
		Log::Verbose("File Segments");
		fileSegments.DumpSegments();
	}
}

// Handles .define and .macro
void Main::PreprocessFileSegments()
{
	Preprocessor preprocessor;
	SourceFileSegments preprocessedFileSegments;
	for(CodeSegment *codeSegment : fileSegments)
	{
		try
		{
			switch(codeSegment->contentType)
			{
			case CodeSegment::ContentType::Assembler:
				preprocessor.Preprocess(preprocessedFileSegments, *codeSegment);
				break;
			case CodeSegment::ContentType::AssemblerTypeChange:
			case CodeSegment::ContentType::Literal:
				preprocessedFileSegments.push_back(new CodeSegment(*codeSegment));
				break;
			}
		}
		catch(AssemblerException &ex)
		{
			ex.PopulateLocationIfNecessary(codeSegment->GetStartingLine(), codeSegment->GetStartingFileIndex());
			for(const auto &it : ex) Log::Error("%s", it.GetLogString(fileSegments.GetFilenameList()).c_str());
		}
	}
	
	if(Log::IsVerboseEnabled())
	{
		Log::Verbose("Preprocessed Segments");
		preprocessedFileSegments.DumpSegments();
		Log::Verbose("Preprocessed Tokens");
		preprocessedFileSegments.DumpTokens();
	}
	
	preprocessedFileSegments.AdoptFileNameList(fileSegments);
	fileSegments = std::move(preprocessedFileSegments);
}

void Main::ProcessFileSegments(FILE *outputFile)
{
	int expectedFileIndex = -1;
	for(CodeSegment *codeSegment : fileSegments)
	{
		try
		{
			switch(codeSegment->contentType)
			{
			case CodeSegment::ContentType::AssemblerTypeChange:
				for(const CodeLine &line : codeSegment->contents)
				{
                    AssemblerType type(line.line);
                    
                    // This check is because comment lines can be merged with AssemblerTypeChange.
                    if (type.IsValid())
                    {
                        commandLine.SetAssemblerType(type);
                    }
				}
				break;
			case CodeSegment::ContentType::Assembler:
				{
					if(Log::IsVerboseEnabled())
					{
						Log::Verbose("Preprocessed\n");
						codeSegment->Dump();
					}

					int fileIndex = codeSegment->GetStartingFileIndex();
					int lineNumber = codeSegment->GetStartingLine();
					if(fileIndex != expectedFileIndex)
					{
						fprintf(outputFile, "#line %d \"%s\"\n", lineNumber, fileSegments.GetFilenameList()[fileIndex].c_str());
						expectedFileIndex = fileIndex;
					}
					else
					{
						fprintf(outputFile, "#line %d\n", lineNumber);
					}
					
					switch(commandLine.GetAssemblerType())
					{
					case AssemblerType::Unknown:
						Log::Error("Internal error - unknown AssemblerType");
						break;
					case AssemblerType::Arm64:
						{
							arm64::Assembler assembler;
							assembler.AssembleSegment(*codeSegment, fileSegments.GetFilenameList());
							if(Log::IsVerboseEnabled()) assembler.Dump();
							assembler.Write(outputFile, lineNumber, &expectedFileIndex, fileSegments.GetFilenameList());
						}
						break;
                    case AssemblerType::RiscV:
                        switch (commandLine.GetRiscVCMode())
                        {
                        case RiscVCMode::Maybe:
                            {
                                riscv::Assembler assembler(false);
                                assembler.AssembleSegment(*codeSegment, fileSegments.GetFilenameList());
                                if(Log::IsVerboseEnabled()) assembler.Dump();
                                riscv::Assembler compressedAssembler(true);
                                compressedAssembler.AssembleSegment(*codeSegment, fileSegments.GetFilenameList());
                                if(Log::IsVerboseEnabled()) compressedAssembler.Dump();
                                if (!assembler.IsEquivalentByteCode(compressedAssembler))
                                {
                                    fprintf(outputFile, "#line %d\n", codeSegment->GetStartingLine());
                                    fprintf(outputFile, "if (Javelin::Assembler::CanUseCompressedInstructions()) {\n");
                                    fprintf(outputFile, "#line %d\n", codeSegment->GetStartingLine());
                                    compressedAssembler.Write(outputFile, lineNumber, &expectedFileIndex, fileSegments.GetFilenameList());
                                    fprintf(outputFile, "#line %d\n", codeSegment->GetStartingLine());
                                    fprintf(outputFile, "} else {\n");
                                    fprintf(outputFile, "#line %d\n", codeSegment->GetStartingLine());
                                    assembler.Write(outputFile, lineNumber, &expectedFileIndex, fileSegments.GetFilenameList());
                                    fprintf(outputFile, "#line %d\n", codeSegment->GetStartingLine());
                                    fprintf(outputFile, "}\n");
                                }
                                else
                                {
                                    assembler.Write(outputFile, lineNumber, &expectedFileIndex, fileSegments.GetFilenameList());
                                }
                            }
                            break;
                        case RiscVCMode::Always:
                            {
                                riscv::Assembler assembler(true);
                                assembler.AssembleSegment(*codeSegment, fileSegments.GetFilenameList());
                                if(Log::IsVerboseEnabled()) assembler.Dump();
                                assembler.Write(outputFile, lineNumber, &expectedFileIndex, fileSegments.GetFilenameList());
                            }
                            break;
                        case RiscVCMode::Never:
                            {
                                riscv::Assembler assembler(false);
                                assembler.AssembleSegment(*codeSegment, fileSegments.GetFilenameList());
                                if(Log::IsVerboseEnabled()) assembler.Dump();
                                assembler.Write(outputFile, lineNumber, &expectedFileIndex, fileSegments.GetFilenameList());
                            }
                            break;
                        }
                        break;
					case AssemblerType::X64:
						{
							x64::Assembler assembler;
							assembler.AssembleSegment(*codeSegment, fileSegments.GetFilenameList());
							if(Log::IsVerboseEnabled()) assembler.Dump();
							assembler.Write(outputFile, lineNumber, &expectedFileIndex, fileSegments.GetFilenameList());
						}
						break;
					}
				}
				break;
			case CodeSegment::ContentType::Literal:
				int expectedLineNumber = -1;
				for(const auto& s : codeSegment->contents)
				{
					if(s.fileIndex != expectedFileIndex || s.lineNumber != expectedLineNumber)
					{
						if(s.fileIndex != expectedFileIndex) {
							fprintf(outputFile, "#line %d \"%s\"\n", s.lineNumber, fileSegments.GetFilenameList()[s.fileIndex].c_str());
							expectedFileIndex = s.fileIndex;
						} else {
							fprintf(outputFile, "#line %d\n", s.lineNumber);
						}
						expectedLineNumber = s.lineNumber + 1;
					}
					else ++expectedLineNumber;
					fprintf(outputFile, "%s\n", s.line.c_str());
					
					// clang seems to get line numbers wrong when some preprocessor commands are issued.
					// Force a #line after a preprocessor:
					if(!s.line.empty() && s.line[0] == '#') expectedLineNumber = -1;
				}
				break;
			}
		}
		catch(AssemblerException &ex)
		{
			ex.PopulateLocationIfNecessary(codeSegment->GetStartingLine(), codeSegment->GetStartingFileIndex());
			for(const auto &it : ex) Log::Error("%s", it.GetLogString(fileSegments.GetFilenameList()).c_str());
		}
	}
}

int Main::Run()
{
	FILE *outputFile = CreateOutputFile();
	
	ReadInputFile();
	PreprocessFileSegments();
	ProcessFileSegments(outputFile);
	
	if(outputFile != stdout) fclose(outputFile);
	
	if(Log::GetNumberOfErrors() > 0)
	{
		unlink(commandLine.GetOutputFilename());
		return 1;
	}
	return 0;
}

//============================================================================


int main(int argc, const char** argv)
{
	Main main(argc, argv);
	return main.Run();
}

//============================================================================
