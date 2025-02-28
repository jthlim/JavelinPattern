//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

namespace Javelin
{
	void Assert(const char* message, const char* fileName, unsigned lineNumber, const char* function) JANALYZER_NO_RETURN;
}

//============================================================================

// We generally use sizeof() so that the compiler does not warn about unused variables that are there in debug versions
// The problem comes with any lambdas inside an assert. In these cases, use JXASSERT

#if defined(JBUILDCONFIG_FINAL)
	#define JASSERT(x)			((void) sizeof(x))
	#define JASSERT_MSG(x, msg)	((void) sizeof(x))
	#define JXASSERT(x)			((void) 0)
	#define JXASSERT_MSG(x, msg)((void) 0)
	#define JVERIFY(x)			((void) (x))
	#define JVERIFY_ALT(x, y)	((void) sizeof(x), (void) (y))
	#define JVERIFY_MSG(x, msg)	((void) (x))
	#define JASSUME(x)			((x) || JUNREACHABLE)
#elif defined(JBUILDCONFIG_DEBUG) || defined(JBUILDCONFIG_RELEASE)
	#define JASSERT(x)			((void) ( JLIKELY(x) || (Javelin::Assert(#x,  __FILE__, __LINE__, __FUNCTION__), 0)))
	#define JASSERT_MSG(x, msg)	((void) ( JLIKELY(x) || (Javelin::Assert(msg, __FILE__, __LINE__, __FUNCTION__), 0)))
	#define JXASSERT(x)			((void) ( JLIKELY(x) || (Javelin::Assert(#x,  __FILE__, __LINE__, __FUNCTION__), 0)))
	#define JXASSERT_MSG(x, msg)((void) ( JLIKELY(x) || (Javelin::Assert(msg, __FILE__, __LINE__, __FUNCTION__), 0)))
	#define JVERIFY(x)			((void) ( JLIKELY(x) || (Javelin::Assert(#x,  __FILE__, __LINE__, __FUNCTION__), 0)))
	#define JVERIFY_ALT(x, y)	((void) ( JLIKELY(x) || (Javelin::Assert(#x,  __FILE__, __LINE__, __FUNCTION__), 0)))
	#define JVERIFY_MSG(x, msg)	((void) ( JLIKELY(x) || (Javelin::Assert(msg, __FILE__, __LINE__, __FUNCTION__), 0)))
	#define JASSUME(x)			((void) ( JLIKELY(x) || (Javelin::Assert(#x,  __FILE__, __LINE__, __FUNCTION__), 0)))
#else
	#error JBUILDCONFIG not defined
#endif

#define JERROR(x)			Javelin::Assert(x, __FILE__, __LINE__, __FUNCTION__)

//============================================================================
