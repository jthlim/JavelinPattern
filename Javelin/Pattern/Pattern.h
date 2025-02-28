//============================================================================
//
// Currently supports:
//
// (...)		Capture
// (?:...)		Cluster
// (?...)		Option change
// (?#...)		Comment
// (?=...)  	Positive look-ahead
// (?!...)  	Negative look-ahead
// (?<=...) 	Positive look-behind
// (?<!...) 	Negative look-behind
// (?>...) 		Atomic group
// (?(c)t:f)	Conditional regex
// (?R)			Recursive regex
// (?1)			Recursive to capture group 1
// (?-n)		Recursive to relative capture group
// (?+n)		Recursive to relative capture group
// |			Alternate
// .			Any character
// *			Zero or more maximal
// *?			Zero or more minimal
// *+			Zero or more possessive
// +			One or more maximal
// +?			One or more minimal
// ++			One or more possessive
// ?			Zero or one maximal
// ??			Zero or one minimal
// ?+			Zero or one possessive
// {###}		Exactly ### times
// {M,N}		Between M and N times maximal
// {M,N}?		Between M and N times minimal
// {M,N}+		Between M and N times possessive
// {M,}			At least M times maximal
// {M,}?		At least M times minimal
// {M,}+		At least M times possessive
// {,N}			At most N times maximal
// {,N}?		At most N times minimal
// {,N}+		At most N times possessive
// ^			Start of line
// $			End of Line
// \a			Alarm (ASCII 07)
// \A			Start of Input
// \b			Word Boundary
// \B			Not Word Boundary
// \cX			Control-X
// \C			Any Byte
// \d			Digit
// \D			Not Digit
// \e			Escape character
// \f			Form feed (ASCII 0C)
// \G			Start of search
// \h       	Space or tab
// \K			Reset capture
// \n			Newline (ASCII 0A)
// \Q{lit}\E    Literals started by \Q. \E to end literal mode and return to normal parsing
// \r			Carriage return (ASCII 0D)
// \s			Whitespace
// \S			Not Whitespace
// \t			Horizontal tab (ASCII 09)
// \u####		Unicode 4 hex digits
// \u{###}		Unicode hex
// \v			Vertical tab (ASCII 0B)
// \w			Word Character
// \W			Not Word Character
// \x##			2 hex digits
// \x{###}		Unicode hex
// \z			End of Input
// \1			Back reference
// \\			Backslash
// [###-##] 	Character List
// [^##-##] 	Not character list
//
//============================================================================

#pragma once
#include "Javelin/Container/Table.h"
#include "Javelin/Type/Exception.h"
#include "Javelin/Type/String.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	class DataBlock;
	class ICharacterWriter;
	class String;

//============================================================================

	namespace PatternInternal
	{
		union ByteCodeInstruction;
		class Compiler;
		class PatternProcessor;
		enum class PatternProcessorType : uint16_t;
	}
	
//============================================================================
	
	class MatchResult
	{
	public:
		bool operator!()	const								{ return captureList.GetCount() == 0;		}
		operator bool()		const								{ return captureList.GetCount() != 0;		}

		JINLINE size_t GetCount() const							{ return captureList.GetCount(); 			}
		JEXPORT const String JCALL operator[](size_t i) const	{ return captureList[i].GetString(); 		}
		JEXPORT bool HasResult(size_t i) const					{ return captureList[i].capture.IsValid(); 	}
		JEXPORT bool IsEmpty(size_t i) const					{ return captureList[i].capture.max <= captureList[i].capture.min; }
		
		JEXPORT const char* GetCaptureBegin(size_t captureIndex) const;
		JEXPORT const char* GetCaptureEnd(size_t captureIndex) const;

	private:
		struct MatchData
		{
			Interval<const char*, true>	capture;
			MatchData() : capture(nullptr, nullptr) { }
			const String GetString() const;
		};
		
		MatchResult(const char** captures, uint32_t numberOfCaptures);
		
		typedef Table<MatchData, TableStorage_Dynamic<DynamicTableAllocatePolicy_NewDeleteWithInlineStorage<8>::Policy>> CaptureList;		
		CaptureList	captureList;
		
		friend class Pattern;
	};

	// Unit test function
	String ToString(const MatchResult& a);

//============================================================================
	
	class PatternException : public GeneralException
	{
	public:
		enum class Type
		{
			None = 0,
			InternalError,

			ExpectedCloseGroup,
			InvalidBackReference,
			InvalidOptions,
			LookBehindNotConstantByteLength,
			MalformedConditional,
			MaximumRepetitionCountExceeded,
			MinimumCountExceedsMaximumCount,
			TooManyByteCodeInstructions,
			TooManyCaptures,
			TooManyProgressCheckInstructions,
			UnableToParseGroupType,
			UnableToParseRepetition,
			UnableToResolveRecurseTarget,
			UnexpectedControlCharacter,
			UnexpectedEndOfPattern,
			UnexpectedGroupOptions,
			UnexpectedHexCharacter,
			UnexpectedLookBehindType,
			UnexpectedToken,
			UnknownEscape,
			UnknownPosixCharacterClass,
		};
		
		PatternException(Type aType, const void* p) : GeneralException(GeneralExceptionType::InvalidData), type(aType) { }

		Type 		GetType() 	 const 	{ return type; 	}
		const void*	GetContext() const 	{ return p; 	}
		
	private:
		Type 		type;
		const void* p;
	};

//============================================================================

	enum class DfaMemoryManagerMode : uint8_t
	{
		Unlimited,
		GlobalLimit,
		PerPatternLimit,
	};
	
//============================================================================

	class Pattern
	{
	public:
		static const int IGNORE_CASE				= 1;		// "i" option
		static const int MULTILINE					= 2;		// "m" option. This allows ^ and $ to match \n lines mid-string
		static const int DOTALL						= 4;		// "s" option. This allows . to match newlines
		static const int UNICODE_CASE				= 8;		// "u" option. when IGNORE_CASE is used, this allows unicode case folding
		static const int UNGREEDY					= 0x10;		// "U" option
		
		static const int UTF8						= 0x100;	// Without this, text is treated as ASCII
		static const int AUTO_CLUSTER				= 0x200;	// Automatically use clusters instead of captures when possible.
		
		static const int GLOB_SYNTAX				= 0x400;
		
		static const int ANCHORED					= 0x800;
		
		static const int NO_OPTIMIZE				= 0x1000;
		
		static const int PREFER_NFA					= 0x2000;
		static const int PREFER_BACK_TRACKING		= 0x4000;
		static const int PREFER_SCAN_AND_CAPTURE	= 0x6000;
		static const int PREFER_NO_SCAN				= 0x8000;
		
		static const int PREFER_MASK				= 0xe000;
		
		static const int NO_FULL_MATCH				= 0x40000000;
		static const int DO_CONSISTENCY_CHECK		= 0x80000000;
		
		JEXPORT Pattern(const String& pattern, int options=0); // Can throw PatternException
		
		JEXPORT Pattern(const void* data, size_t length, bool makeCopy=true);
		
		JEXPORT ~Pattern();

		JINLINE size_t GetNumberOfCaptures() const { return numberOfCaptures; }

        JINLINE bool        HasFullMatch(const void* data, size_t length) const                         { return InternalHasFullMatch(data, length) != nullptr; }
        JINLINE bool        HasPartialMatch(const void* data, size_t length, size_t offset = 0) const   { return InternalHasPartialMatch(data, length) != nullptr; }

		JEXPORT bool JCALL   FullMatch(const void* data, size_t length, const void** captures) const;
		JEXPORT bool JCALL   PartialMatch(const void* data, size_t length, const void** captures, size_t offset = 0) const;
		JEXPORT size_t JCALL CountPartialMatches(const void* data, size_t length, size_t offset = 0) const;

		JINLINE bool 		HasFullMatch(const String& s) const							{ return HasFullMatch(s.GetData(), s.GetNumberOfBytes()); }
		JINLINE bool 		HasPartialMatch(const String& s, size_t offset=0) const		{ return HasPartialMatch(s.GetData(), s.GetNumberOfBytes(), offset); }

		JEXPORT MatchResult FullMatch(const String& s) const;
		JEXPORT MatchResult PartialMatch(const String& s, size_t offset=0) const;
		
		JINLINE size_t 		CountPartialMatches(const String& s, size_t offset=0) const	{ return CountPartialMatches(s.GetData(), s.GetNumberOfBytes(), offset); }
		
		static String		EscapeString(const String& literal);
		static DataBlock 	CreateByteCode(const String& pattern, int options=0); // Can throw PatternException
		static void 		DumpInstructions(IWriter& output, const DataBlock& data);
		JINLINE size_t 		GetMinimumLength() const 					{ return minimumMatchLength; }
		JINLINE size_t		GetMaximumLength() const 					{ return size_t(ssize_t(maximumMatchLength)); }

		class StackGrowthHandler
		{
		public:
			virtual void* Allocate(size_t size) const = 0;
			virtual void Free(void* p) const = 0;
		};		

		static void SetNoStackGrowth();
		static const StackGrowthHandler* GetStackGrowthHandler() 		{ return stackGrowthHandler; }
		static void SetStackGrowthHandler(const StackGrowthHandler* a) 	{ stackGrowthHandler = a; }
		
		static void SetDfaMemoryModeConfiguration(DfaMemoryManagerMode mode, size_t limit=1024*1024);
		
	private:
		JDISABLE_COPY_AND_ASSIGNMENT(Pattern);
		
		uint16_t							flags;
		int32_t								matchLengthCheck;
		PatternInternal::PatternProcessor* 	partialMatchProcessor;
		PatternInternal::PatternProcessor* 	fullMatchProcessor;
		uint32_t							numberOfCaptures;
		uint32_t							minimumMatchLength;
		int32_t								maximumMatchLength;

		static const StackGrowthHandler*	stackGrowthHandler;
		
		bool AlwaysRequiresCaptures() const;
		bool HasStartAnchor() const;
		bool HasEndAnchor() const;
		
		void Set(const void* data, size_t length, bool makeCopy);
		
        JEXPORT const void* JCALL InternalHasFullMatch(const void* data, size_t length) const;
        JEXPORT const void* JCALL InternalHasPartialMatch(const void* data, size_t length, size_t offset = 0) const;

		const void* HasFullMatchWithCaptures(const void* data, size_t length) const;
		const void* HasPartialMatchWithCaptures(const void* data, size_t length, size_t offset) const;
		size_t CountPartialMatchesWithCaptures(const void* data, size_t length, size_t offset) const;
		size_t GetMatchLengthCheck() const;

		static void DumpInstructionList(IWriter& output, const PatternInternal::ByteCodeInstruction* instructions, size_t numberOfInstructions, size_t offset);

		static PatternInternal::PatternProcessor* CreateProcessor(DataBlock&& dataBlock, PatternInternal::PatternProcessorType type);
		static PatternInternal::PatternProcessor* CreateProcessor(const void* data, size_t length, bool makeCopy, PatternInternal::PatternProcessorType type);

		friend class PatternInternal::Compiler;
	};

#define JPATTERN_ERROR(y, z) 		throw Javelin::PatternException(Javelin::PatternException::Type::y, z)
#define JPATTERN_VERIFY(x, y, z) 	((void) (JLIKELY(x) || (throw Javelin::PatternException(Javelin::PatternException::Type::y, z), 0)))

//============================================================================
} // namespace Javelin
//============================================================================
