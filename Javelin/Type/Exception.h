//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

#if defined(JBUILDCONFIG_FINAL)
	#define JINCLUDE_STACK_IN_EXCEPTIONS				0
	#define JINCLUDE_STRING_DESCRIPTIONS_IN_EXCEPTIONS	0
#elif defined(JBUILDCONFIG_RELEASE)
	#define JINCLUDE_STACK_IN_EXCEPTIONS				0
	#define JINCLUDE_STRING_DESCRIPTIONS_IN_EXCEPTIONS	1
#elif defined(JBUILDCONFIG_DEBUG)
	#define JINCLUDE_STACK_IN_EXCEPTIONS				1
	#define JINCLUDE_STRING_DESCRIPTIONS_IN_EXCEPTIONS	1
#else
	#error JBUILDCONFIG not defined
#endif

#if JINCLUDE_STACK_IN_EXCEPTIONS
	#include "Javelin/System/StackWalk.h"
#endif

//============================================================================

namespace Javelin
{
//============================================================================

	class String;
	
//============================================================================

	enum class ExceptionCategory : unsigned short
	{
		General,
		Stream,
		Network,
	};
	
	enum class GeneralExceptionType
	{
		Unknown,
		InvalidParameter,		// Invalid or Unexpected
		InvalidData,			// Invalid or Corrupted
		InvalidCall,			// Code shouldn't reach this point
		UnexpectedResult,		// Return value isn't as expected
		UnsupportedFeature,		// Valid, but not supported
	};
	
	enum class StreamExceptionType
	{
		Unknown,
		CannotRead,
		CannotWrite,
		CannotSeek,
		EndOfStream,
	};
	
	enum class NetworkExceptionType
	{
		Unknown,
		AddressInUse,
		AlreadyConnected,
		NetworkUnreachable,
		HostUnreachable,
		TimeOut,
	};
	
//============================================================================	
	
	struct Exception
	{
		ExceptionCategory 	category;
		short 				reason;

#if JINCLUDE_STACK_IN_EXCEPTIONS
		StackWalk			stackWalk;
		Exception(ExceptionCategory aCategory, short aReason);
#else
		Exception(ExceptionCategory aCategory, short aReason) : category(aCategory), reason(aReason) { }
#endif

#if JINCLUDE_STRING_DESCRIPTIONS_IN_EXCEPTIONS
		virtual String GetDescription() const;
		virtual String GetShortDescription() const;
		virtual String GetCategoryString() const;
		virtual String GetReasonString() const;
#else
		String GetDescription() const;
		String GetShortDescription() const;
		String GetCategoryString() const;
		String GetReasonString() const;
#endif
	};

//============================================================================

	struct GeneralException : public Exception
	{
		typedef Exception Inherited;

		GeneralException(GeneralExceptionType reason) : Inherited(ExceptionCategory::General, (short) reason) { }
		GeneralExceptionType GetReason() const { return (GeneralExceptionType) reason; }

#if JINCLUDE_STRING_DESCRIPTIONS_IN_EXCEPTIONS
		String GetCategoryString() const final;
		String GetReasonString() const final;
#endif
	};

	struct StreamException : public Exception
	{
		typedef Exception Inherited;

		StreamException(StreamExceptionType reason) : Inherited(ExceptionCategory::Stream, (short) reason) { }
		
		bool IsEndOfStream() const { return reason == (short) StreamExceptionType::EndOfStream; }
		StreamExceptionType GetReason() const { return (StreamExceptionType) reason; }

#if JINCLUDE_STRING_DESCRIPTIONS_IN_EXCEPTIONS
		String GetCategoryString() const final;
		String GetReasonString() const final;
#endif
	};

	struct NetworkException : public Exception
	{
		typedef Exception Inherited;

		NetworkException(NetworkExceptionType reason) : Inherited(ExceptionCategory::Network, (short) reason) { }
		NetworkExceptionType GetReason() const { return (NetworkExceptionType) reason; }

#if JINCLUDE_STRING_DESCRIPTIONS_IN_EXCEPTIONS
		String GetCategoryString() const final;
		String GetReasonString() const final;
#endif
	};

//============================================================================

	#define JDATA_VERIFY(x) 	((void) (JLIKELY(x) || (throw(Javelin::GeneralException(Javelin::GeneralExceptionType::InvalidData)), 0)))
	#define JDATA_ERROR() 		throw(Javelin::GeneralException(Javelin::GeneralExceptionType::InvalidData))
	#define JRESULT_VERIFY(x) 	((void) (JLIKELY(x) || (throw(Javelin::GeneralException(Javelin::GeneralExceptionType::UnexpectedResult)), 0)))
	#define JRESULT_ERROR()		throw(Javelin::GeneralException(Javelin::GeneralExceptionType::UnexpectedResult))

//============================================================================
} // namespace Javelin
//============================================================================

