//============================================================================

#pragma once
#include <stdio.h>
#include <stdarg.h>
#include <string>

//============================================================================

namespace Javelin::Assembler
{
//============================================================================
	
	/// Simple FILE* wrapper
	class File
	{
	public:
		File(const char* aFilename)	: filename(aFilename) 			{ f = fopen(aFilename, "rb"); 			}
		File(const std::string &aFilename)	: filename(aFilename) 	{ f = fopen(filename.c_str(), "rb"); 	}
		File(FILE* aF)				{ f = aF;						}
		~File()						{ if(f) fclose(f); 				}
		
		std::string ReadLine();
		const std::string &GetFilename() const { return filename; }
		
		bool IsValid() const		{ return f != nullptr; 			}
		bool IsEof() const			{ return feof(f); 				}
		
		int getc() 					{ return fgetc(f);				}
	
#if defined(__clang__)
		__attribute__((__format__ (__printf__, 2, 3)))
		void PrintF(const char* p, ...)
#else
		void PrintF(const char* p, ...)
#endif
		{
			va_list args;
			va_start(args, p);
			vfprintf(f, p, args);
			va_end(args);
		}
		
	private:
		FILE *f;
		std::string filename;
	};
	
//============================================================================
}
//============================================================================
