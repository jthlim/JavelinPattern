//============================================================================
//
// JavelinBase defines the following macros
//
// JCOMPILER_MSVC		- Microsoft Visual C compiler
// JCOMPILER_GCC		- GCC
// JCOMPILER_CLANG      - Clang
//
// JPLATFORM_WIN32
// JPLATFORM_WIN64
// JPLATFORM_LINUX
// JPLATFORM_OPENBSD
// JPLATFORM_MACOS			\
// JPLATFORM_IOS			 +- JPLATFORM_APPLEOS
// JPLATFORM_IOS_SIMULATOR	/
// JPLATFORM_POSIX
//
// JCOMPUTE_OPENCL
//
// JENDIAN_LITTLE
// JENDIAN_BIG
//
// JWCHAR_BYTES			- 2 or 4, depending on compiler
//
// JASM_NONE			- No inline assembler supported
// JASM_MSC_X86
// JASM_GNUC_ARM
// JASM_GNUC_ARM_NEON
// JASM_GNUC_ARM_VFP4
// JASM_GNUC_X86
// JASM_GNUC_X86_64
// JASM_GNUC_PPC
// JASM_GNUC_PPC64
//
// JABI_AMD64_SYSTEM_V
// JABI_ARM64_PCS
//
//============================================================================

#pragma once
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>

//============================================================================

#if defined(_WIN64) || defined(_WIN32)
	#define NOMINMAX
	#include <Winsock.h>
	#include <Windows.h>

	#if defined(_MSC_VER)
		#include <intrin.h>
	#endif
	#undef NOMINMAX
	#undef Yield
	#undef MoveMemory
#elif defined(__APPLE__)
	#include <TargetConditionals.h>
#endif

//============================================================================

namespace Javelin 
{
//============================================================================

#if !defined(__OBJC__)
	typedef void* id;
#endif

#if defined(_MSC_VER)
	#pragma warning(disable : 4068)	// Disable unknown pragma warnings!

	#if defined(JAVELIN_DLL)
	  #if defined(JAVELIN_EXPORTS)
		#define JEXPORT	__declspec(dllexport)
	  #else
		#define JEXPORT	__declspec(dllimport)
	  #endif
	#else
	  #define JEXPORT
	#endif
	#define JVTABLE_EXPORT
	#define JABSTRACT		__declspec(novtable)

	#define JALIGN(x)		__declspec(align(x))
	#define JINLINE			__forceinline
	#define JNOINLINE
	#define JCALL			__fastcall
	#define JVCALL			__cdecl
	#define JTHREADCALL		__stdcall
	#define JUNREACHABLE
	#define JNOTHROW		throw()
	#define JLIKELY(x)		(x)
	#define JUNLIKELY(x)	(x)

	#define JANALYZER_NO_RETURN
	
	#define JCOMPILER_MSVC
	#define JENDIAN_LITTLE
	#define JWCHAR_BYTES	2

	#define final
	#define constexpr	const
	
	JINLINE void BreakPoint()			{ __debugbreak();	}

	#define INFINITY					(DBL_MAX+DBL_MAX)
	#define NAN							(INFINITY-INFINITY)

	#undef CopyMemory
	#define fseeko _fseeki64
	#define ftello _ftelli64

	#if defined(_M_IX86)
		#define JASM_MSC_X86
	#else
		#define JASM_NONE
	#endif

	#if defined(_WIN64)
		#define JPLATFORM_WIN64
	#elif defined(_WIN32)
		#define JPLATFORM_WIN32
		#pragma intrinsic (_InterlockedIncrement) 
		#pragma intrinsic (_InterlockedDecrement) 
		#pragma intrinsic (_InterlockedExchangeAdd) 
		#define InterlockedIncrement _InterlockedIncrement 
		#define InterlockedDecrement _InterlockedDecrement
		#define InterlockedExchangeAdd _InterlockedExchangeAdd
	#else
		#error "Unsupported Platform"
	#endif

#elif defined(__GNUC__)
	
	#ifdef JAVELIN_EXPORTS
		#define JEXPORT			__attribute__((visibility("default")))
		#define JVTABLE_EXPORT	__attribute__((visibility("default")))
	#else
		#define JEXPORT
		#define JVTABLE_EXPORT
	#endif

	#define JALIGN(x)		__attribute__((aligned(x)))
	#define JINLINE			__attribute__((always_inline)) inline
	#define JNOINLINE		__attribute__((noinline))
	#define JVCALL
	#define JTHREADCALL
	#define JUNREACHABLE	__builtin_unreachable()
	#define JNOTHROW		noexcept
	#define JLIKELY(x)		(__builtin_expect(!!(x),1))
	#define JUNLIKELY(x)	(__builtin_expect((x),0))

    #if __clang__
        #define JCOMPILER_CLANG
		#define JANALYZER_NO_RETURN __attribute__((analyzer_noreturn))
    #else
        #define JCOMPILER_GCC
		#define JANALYZER_NO_RETURN
    #endif
    
	#define JABSTRACT
	#define JWCHAR_BYTES	4


	#if defined(__i386__)
		#define JENDIAN_LITTLE
		#define JASM_GNUC_X86
        #define JCALL            __fastcall
	#elif defined(__amd64__)
		#define JENDIAN_LITTLE
		#define JASM_GNUC_X86_64
		#define JABI_AMD64_SYSTEM_V
        #define JCALL            __fastcall
	#elif defined(__arm64__)
		#if defined(__BIG_ENDIAN__)
			#define JENDIAN_BIG
		#elif defined(__LITTLE_ENDIAN__)
			#define JENDIAN_LITTLE
		#else
			#error "Expected GCC to define __BIG_ENDIAN__ or __LITTLE_ENDIAN__ for ARM target"
		#endif

		#define JASM_GNUC_ARM64
		#define JABI_ARM64_PCS
        #define JCALL
    #elif defined(__arm__)
        #if defined(__BIG_ENDIAN__)
            #define JENDIAN_BIG
        #elif defined(__LITTLE_ENDIAN__)
            #define JENDIAN_LITTLE
        #else
            #error "Expected GCC to define __BIG_ENDIAN__ or __LITTLE_ENDIAN__ for ARM target"
        #endif
		#if defined(_ARM_ARCH_7) || defined(__ARM_NEON__)
			#define JASM_GNUC_ARM_NEON
		#endif
		#if defined(__ARM_ARCH_7S__)
			#define JASM_GNUC_ARM_VFP4
			#define JASM_GNUC_ARM_NEON
		#endif
        #define JASM_GNUC_ARM
        #define JCALL
	#elif defined(__POWERPC__)
		#if defined(__BIG_ENDIAN__)
			#define JENDIAN_BIG
		#elif defined(__LITTLE_ENDIAN__)
			#define JENDIAN_LITTLE
		#else
			#error "Expected GCC to define __BIG_ENDIAN__ or __LITTLE_ENDIAN__ for PPC target"
		#endif

		#if defined(__ppc__)
			#define JASM_GNUC_PPC
		#elif defined(__ppc64__)
			#define JASM_GNUC_PPC64
		#else
			#error "Expected GCC to define __ppc__ or __ppc64__"
		#endif
        #define JCALL
	#else
		#error "Unsupported CPU"
	#endif

	#if TARGET_OS_IPHONE
		#define JPLATFORM_POSIX
		#define JPLATFORM_APPLEOS
		#if TARGET_IPHONE_SIMULATOR
			#define JPLATFORM_IOS_SIMULATOR
			#define JASM_GNUC_X86_SSE3
		#else
			#define JPLATFORM_IOS
		#endif

		// Disable __fastcall for all iOS targets to stop compiler warnings
		#undef JCALL
		#define JCALL
	#elif defined(__MACH__)
		#define JPLATFORM_POSIX
        #define JPLATFORM_MACOS
		#define JPLATFORM_APPLEOS
		#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
            #if defined(__i386__)
                #define JCOMPUTE_OPENCL
            #elif defined(__amd64__)
                #define JCOMPUTE_OPENCL
				#undef JCALL
				#define JCALL
            #endif
        #endif
	#elif defined(__linux__)
		#undef JCALL
		#define JCALL
		#define JPLATFORM_POSIX
		#define JPLATFORM_LINUX
	#elif defined(__OpenBSD__)
		#define JPLATFORM_POSIX
		#define JPLATFORM_OPENBSD
	#else
		#error "Unsupported OS!"
	#endif

	JINLINE void BreakPoint()						{ __builtin_trap();		}
	

#endif

//============================================================================

	class PlacementPointer;
	JINLINE constexpr PlacementPointer* Placement(void* a) { return (PlacementPointer*) a; }
	class ZeroMemory;
	constexpr const ZeroMemory* ZERO_MEMORY = nullptr;
	class NoInitialize;
	constexpr const NoInitialize* NO_INITIALIZE = nullptr;
	class MakeReference;
	constexpr const MakeReference* MAKE_REFERENCE = nullptr;
	class RawInitialize;
	constexpr const RawInitialize* RAW_INITIALIZE = nullptr;
	
	// Allow the use of iterator syntax without duplicating begin/end methods!
	template<typename T> JINLINE auto begin(T&& a) -> decltype(a.Begin()) 		{ return a.Begin(); }
	template<typename T> JINLINE auto end(T&& a) -> decltype(a.End()) 			{ return a.End(); 	}
	
	// Helper methods to extract Class/Member types
	template <typename T, typename M> T GetClassType(M T::*);
	template <typename T, typename M> M GetMemberType(M T::*);
	template <typename T, typename R, R T::*M> constexpr size_t OffsetOf()			{ return reinterpret_cast<size_t>(&(((T*)0)->*M));	}
	
	#define JCLASS_MEMBER(x) decltype(Javelin::GetClassType(x)), decltype(Javelin::GetMemberType(x)), x
	
	#undef offsetof
	#define offsetof(m) OffsetOf<JCLASS_MEMBER(m)>()
	
//============================================================================
} // namespace Javelin
//============================================================================

JINLINE void* operator new   (size_t, Javelin::PlacementPointer* p)					{ return p; }
JINLINE void  operator delete(void *, Javelin::PlacementPointer* p)					{ }
JINLINE void* operator new   [](size_t, Javelin::PlacementPointer* p)				{ return p; }
JINLINE void  operator delete[](void *, Javelin::PlacementPointer* p)				{ }

JINLINE void* operator new   (size_t t, const Javelin::ZeroMemory*)					{ void *p = ::operator new(t); memset(p, 0, t); return p; }
JINLINE void  operator delete(void *p,  const Javelin::ZeroMemory*)		JNOTHROW	{ ::operator delete(p); }
JINLINE void* operator new   [](size_t t, const Javelin::ZeroMemory*)				{ void *p = ::operator new[](t); memset(p, 0, t); return p; }
JINLINE void  operator delete[](void *p,  const Javelin::ZeroMemory*)	JNOTHROW	{ ::operator delete(p); }

JINLINE void* operator new   (size_t t, Javelin::PlacementPointer* p, const Javelin::ZeroMemory*)	{ memset(p, 0, t); return p; }
JINLINE void  operator delete(void *,   Javelin::PlacementPointer* p, const Javelin::ZeroMemory*)	{ }
JINLINE void* operator new   [](size_t t, Javelin::PlacementPointer* p, const Javelin::ZeroMemory*)	{ memset(p, 0, t); return p; }
JINLINE void  operator delete[](void *,   Javelin::PlacementPointer* p, const Javelin::ZeroMemory*)	{ }

//============================================================================

#define breakable switch(0) default:

#define JDECLARE_ADDITIONAL_ALLOCATORS																	\
																										\
	inline static void* JCALL operator new(size_t, Javelin::PlacementPointer* p)  	{ return p; }		\
	inline static void  JCALL operator delete(void *, Javelin::PlacementPointer*) 	{ }					\
	inline static void* JCALL operator new(size_t t, const ZeroMemory*)									\
	{																									\
		void *returnValue = operator new(t);															\
		memset(returnValue, 0, t);																		\
		return returnValue;																				\
	}																									\
	inline static void  JCALL operator delete(void *p,  const ZeroMemory*)	{ operator delete(p);	}	\
	inline static void* JCALL operator new[](size_t t, const ZeroMemory*)								\
	{																									\
		void *returnValue = operator new[](t);															\
		memset(returnValue, 0, t);																		\
		return returnValue;																				\
	}																									\
	inline static void  JCALL operator delete[](void *p,  const ZeroMemory*) { operator delete[](p); }	

#if defined(JCOMPILER_CLANG)
  #define JDISABLE_COPY_AND_ASSIGNMENT(CLASS) CLASS(const CLASS&) = delete; void operator=(const CLASS&) = delete
#else
  #define JDISABLE_COPY_AND_ASSIGNMENT(CLASS) CLASS(const CLASS&); void operator=(const CLASS&)
#endif

//============================================================================
