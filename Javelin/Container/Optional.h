//============================================================================
//
// Optional<T> is used to represent a value that may be T, or not.
// If it's not T, then it remains unconstructed.
//
//============================================================================

#pragma once
#include "Javelin/Container/Optional/OptionalStorage_Boolean.h"
#include "Javelin/Container/Optional/OptionalStorage_EmptyMemberProvider.h"
#include "Javelin/Container/Optional/OptionalStorage_EmptyProvider.h"
#include "Javelin/Container/Optional/OptionalStorage_EmptyValue.h"
#include "Javelin/Container/Optional/OptionalStorage_NullPointer.h"
#include "Javelin/Container/Optional/OptionalStorage_NullPointerMember.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class String;

//============================================================================

	struct OptionalStorage_Automatic
	{
		template<typename T> class Policy : public OptionalStorage_Boolean::Policy<T> { };
	};
	
	template<> class OptionalStorage_Automatic::Policy<String> : public OptionalStorage_NullPointer::Policy<String> { };
	
//============================================================================
	
	template<typename T, typename StoragePolicy = OptionalStorage_Automatic>
		class Optional
	{
	public:
		typedef typename StoragePolicy::template Policy<T> Storage;
		typedef typename Storage::Type StorageType;
		
		JINLINE Optional() 								{ }
		
		template<typename... A>
		JINLINE Optional(A&&... a) : data((A&&) a...)	{ }
		
		JINLINE Optional(Optional& a) 					{ Storage::CopyFrom(data, a.data); }
		JINLINE Optional(const Optional& a) 			{ Storage::CopyFrom(data, a.data); }
		JINLINE Optional(Optional&& a) 					{ Storage::MoveFrom(data, (StorageType&&) a.data); }
		
		JINLINE ~Optional()								{ if(HasValue()) Storage::DestroyValue(data); }
		
		template<typename A>
		JINLINE Optional& operator=(A&& a) 		 		{ this->~Optional(); Storage::CopyFrom(data, (A&&) a); JASSERT(HasValue()); return *this; }
		
		JINLINE Optional& operator=(Optional& a)		{ this->~Optional(); Storage::CopyFrom(data, a.data); return *this; }
		JINLINE Optional& operator=(const Optional& a)	{ this->~Optional(); Storage::CopyFrom(data, a.data); return *this; }
		JINLINE Optional& operator=(Optional&& a)		{ this->~Optional(); Storage::MoveFrom(data, (StorageType&&) a.data); return *this; }
		
		
		template<typename... A>
		JINLINE T& Create(A&&... a)					{ JASSERT(!HasValue()); return Storage::CopyFrom(data, (A&&) a...); }
		template<typename... A>
		JINLINE void Replace(A&&... a)				{ JASSERT(HasValue()); Storage::Replace(data, (A&&) a...); }
		
		JINLINE bool HasValue() const				{ return Storage::HasValue(data); }
		JINLINE bool IsEmpty() const				{ return !Storage::HasValue(data); }
		JINLINE void SetEmpty() 					{ this->~Optional(); Storage::SetEmpty(data); }
		
		JINLINE T GetValueWithDefault(const T& a)	{ return HasValue() ? Storage::GetValue(data) : a; }
		
		JINLINE operator T&()						{ return Storage::GetValue(data); }
		JINLINE operator const T&()	const			{ return Storage::GetValue(data); }
		
	private:
		StorageType data;
	};
	
//============================================================================
} // namespace Javelin
//============================================================================
