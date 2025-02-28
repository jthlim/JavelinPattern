//============================================================================
//
// Ownership Policies
//  - SmartPointerOwnershipPolicy_NonIntrusiveReferenceCounted
//  - SmartPointerOwnershipPolicy_ReferenceCountedSingleThreaded
//  - SmartPointerOwnershipPolicy_SmartObject
//      For classes derived from SmartObject
//  - SmartPointerOwnershipPolicy_SmartObjectPointerAlwaysValid
//      For classes derived from SmartObject where the pointer will never be nullptr
//  - SmartPointerOwnershipPolicy_SingleAssignSmartObject
//      For classes derived from SingleAssignSmartObject
//  - SmartPointerOwnershipPolicy_SingleAssignSmartObjectPointerAlwaysValid
//      For classes derived from SingleAssignSmartObject where the pointer will never be nullptr
//  - SmartPointerOwnershipPolicy_DestructiveCopy
//
//  - SmartPointerOwnershipPolicy_AutomaticReferenceCountedType
//  - SmartPointerOwnershipPolicy_SingleAssignReferenceCountedAlwaysValid
//
// SingleAssign Notes:
//  Single Assign is where there is only one possible assignment of the raw object to the smart pointer
//  ie. MySmartPointer p(new MyObject);
//
// Conversion Policies
//  - SmartPointerConversionPolicy_Enable
//  - SmartPointerConversionPolicy_Disable
//
// Checking Policies
//  - SmartPointerCheckingPolicy_AssertCheck
//  - SmartPointerCheckingPolicy_AssertNotNull
//  - SmartPointerCheckingPolicy_None
//
// Storage Policies
//  - SmartPointerStoragePolicy_Default
//  - SmartPointerStoragePolicy_NoDereference
//  - SmartPointerStoragePolicy_Array
//
//============================================================================

#pragma once
#include "Javelin/System/Assert.h"
#include "Javelin/Type/Atomic.h"
#include "Javelin/Template/Conversion.h"
#include "Javelin/Template/SelectType.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	class IReader;
	class IWriter;
	
//============================================================================
// Base class for intrusive reference counting
//============================================================================

	class SmartObject
	{
	public:
		bool HasUniqueReference() const	{ return referenceCount == 1; }

		void AddReference()	const		{ ++referenceCount; }		
		bool RemoveReference() const	{ return --referenceCount == 0; }
		
	protected:
		SmartObject()					: referenceCount(0)			{ }
		SmartObject(bool isStatic)		: referenceCount(isStatic)	{ }
		SmartObject(const SmartObject&) : referenceCount(0)			{ }

	private:
		mutable Atomic<unsigned> referenceCount;

		template<typename T> friend class SmartPointerOwnershipPolicy_SmartObject;
		template<typename T> friend class SmartPointerOwnershipPolicy_SmartObjectPointerAlwaysValid;
	};

	class SingleAssignSmartObject
	{
	public:
		bool HasUniqueReference() const	{ return referenceCount == 1; }

		void AddReference()	const		{ ++referenceCount; }		
		bool RemoveReference() const	{ return --referenceCount == 0; }

	protected:
		SingleAssignSmartObject()								: referenceCount(1)				{ }
		SingleAssignSmartObject(bool isStatic)					: referenceCount(1+isStatic)	{ }
		SingleAssignSmartObject(const SingleAssignSmartObject&) : referenceCount(1)				{ }

	private:
		mutable Atomic<unsigned> referenceCount;

		template<typename T> friend class SmartPointerOwnershipPolicy_SingleAssignSmartObject;
		template<typename T> friend class SmartPointerOwnershipPolicy_SingleAssignSmartObjectPointerAlwaysValid;
	};

//============================================================================
// Ownership Policies
//============================================================================

	// Non-intrusive reference counting
	template<typename T> class SmartPointerOwnershipPolicy_NonIntrusiveReferenceCounted
	{
	protected:
		SmartPointerOwnershipPolicy_NonIntrusiveReferenceCounted() : pCounter(new Atomic<unsigned>(1)) { }
		SmartPointerOwnershipPolicy_NonIntrusiveReferenceCounted(const SmartPointerOwnershipPolicy_NonIntrusiveReferenceCounted& a) : pCounter(a.pCounter) { }

		void OnInitialise(T*)	{ }
		void OnStoredTypeInitialise(T*)	{ pCounter = new Atomic<unsigned>(1); }

		T*	Clone(T *p)	const	{ ++*pCounter; return p; }
		int Release(T* p)		{ if(--*pCounter == 0) { delete pCounter; return 0; } else return 1; }

		enum { destructiveCopy = false };

	private:
		Atomic<unsigned>*	pCounter;
	};

	template<typename T> class SmartPointerOwnershipPolicy_ReferenceCountedSingleThreaded
	{
	protected:
		SmartPointerOwnershipPolicy_ReferenceCountedSingleThreaded() : pCounter(new int(1)) { }
		SmartPointerOwnershipPolicy_ReferenceCountedSingleThreaded(const SmartPointerOwnershipPolicy_ReferenceCountedSingleThreaded& a) : pCounter(a.pCounter) { }

		void OnInitialise(T*)	{ }
		void OnStoredTypeInitialise(T*)	{ pCounter = new int(1); }

		T*	Clone(T *p) const	{ ++*pCounter; return p; }
		int Release(T* p)		{ if(--*pCounter == 0) { delete pCounter; return 0; } else return 1; }

		enum { destructiveCopy = false };

	private:
		int*	pCounter;
	};

	// Intrusive reference counting
	template<typename T> class SmartPointerOwnershipPolicy_SmartObject
	{
	protected:
		SmartPointerOwnershipPolicy_SmartObject() { }
		SmartPointerOwnershipPolicy_SmartObject(const SmartPointerOwnershipPolicy_SmartObject&) { }

		void OnInitialise(T* p)				{ if(p) ++(p->referenceCount); }
		void OnStoredTypeInitialise(T* p)	{ if(p) ++(p->referenceCount); }

		T*	Clone(T* p) const	{
									if(p) ++(p->referenceCount);
									return p;
								}

		int Release(T* p)		{
									if(!p) return false;
									return --(p->referenceCount);
								}

		enum { destructiveCopy = false };
	};

	template<typename T> class SmartPointerOwnershipPolicy_SmartObjectPointerAlwaysValid
	{
	protected:
		SmartPointerOwnershipPolicy_SmartObjectPointerAlwaysValid() { }
		SmartPointerOwnershipPolicy_SmartObjectPointerAlwaysValid(const SmartPointerOwnershipPolicy_SmartObjectPointerAlwaysValid&) { }

		JINLINE void OnInitialise(T* p)				{ ++(p->referenceCount); }
		JINLINE void OnStoredTypeInitialise(T* p)	{ ++(p->referenceCount); }

		JINLINE T*	Clone(T* p) const		{ ++(p->referenceCount); return p; }
		JINLINE int Release(T* p)			{ return --(p->referenceCount); }

		enum { destructiveCopy = false };
	};

	template<typename T> class SmartPointerOwnershipPolicy_SingleAssignSmartObject
	{
	protected:
		SmartPointerOwnershipPolicy_SingleAssignSmartObject() { }
		SmartPointerOwnershipPolicy_SingleAssignSmartObject(const SmartPointerOwnershipPolicy_SingleAssignSmartObject&) { }
		
		JINLINE void OnInitialise(T* p)				{ if(p) ++(p->referenceCount); }
		JINLINE void OnStoredTypeInitialise(T* p)	{ JASSERT(p->referenceCount == 1); }
		
		JINLINE T*	Clone(T* p) const				{
														if(p) ++(p->referenceCount);
														return p;
													}
		
		JINLINE int Release(T* p)					{
														if(!p) return false;
														return --(p->referenceCount);
													}
		
		enum { destructiveCopy = false };
	};
	
	template<typename T> class SmartPointerOwnershipPolicy_SingleAssignSmartObjectPointerAlwaysValid
	{
	protected:
		SmartPointerOwnershipPolicy_SingleAssignSmartObjectPointerAlwaysValid() { }
		SmartPointerOwnershipPolicy_SingleAssignSmartObjectPointerAlwaysValid(const SmartPointerOwnershipPolicy_SingleAssignSmartObjectPointerAlwaysValid&) { }

		JINLINE void OnInitialise(T* p)				{ ++(p->referenceCount); }
		JINLINE void OnStoredTypeInitialise(T* p)	{ JASSERT(p->referenceCount == 1); }

		JINLINE T*	Clone(T* p) const				{ ++(p->referenceCount); return p; }
		JINLINE int Release(T* p)					{ return --(p->referenceCount); }

		enum { destructiveCopy = false };
	};

	template<typename T> class SmartPointerOwnershipPolicy_DestructiveCopy
	{
	protected:
		SmartPointerOwnershipPolicy_DestructiveCopy()												{ }
		SmartPointerOwnershipPolicy_DestructiveCopy(SmartPointerOwnershipPolicy_DestructiveCopy &a)	{ }

		void OnInitialise(T*)	{ }
		void OnStoredTypeInitialise(T*)	{ }

		T*	Clone(T* &p)		{ T* a(p); p = nullptr; return a; }
		int Release(T*)			{ return 0; }

		enum { destructiveCopy = true };
	};

	template<typename T, bool isSmartObject> class SmartPointerOwnershipPolicy_AutomaticReferenceCountedType : public SmartPointerOwnershipPolicy_SmartObject<T> { };
	template<typename T> class SmartPointerOwnershipPolicy_AutomaticReferenceCountedType<T, false> : public SmartPointerOwnershipPolicy_NonIntrusiveReferenceCounted<T> { };
	template<typename T, bool isSingleAssign> class SmartPointerOwnershipPolicy_SingleAssignAutomaticReferenceCountedType : public SmartPointerOwnershipPolicy_SingleAssignSmartObject<T> { };
	template<typename T> class SmartPointerOwnershipPolicy_SingleAssignAutomaticReferenceCountedType<T, false> : public SmartPointerOwnershipPolicy_AutomaticReferenceCountedType <T, Conversion<SmartObject, T>::EXISTS > { };
	template<typename T> class SmartPointerOwnershipPolicy_ReferenceCounted : public SmartPointerOwnershipPolicy_SingleAssignAutomaticReferenceCountedType<T, Conversion<SingleAssignSmartObject, T>::EXISTS > { };

	template<typename T, bool isSingleAssign> class SmartPointerOwnershipPolicy_SingleAssignReferenceCountedAlwaysValid : public SmartPointerOwnershipPolicy_SingleAssignSmartObjectPointerAlwaysValid<T> { };
	template<typename T> class SmartPointerOwnershipPolicy_SingleAssignReferenceCountedAlwaysValid<T, false> : public SmartPointerOwnershipPolicy_SmartObjectPointerAlwaysValid<T> { };
	template<typename T> class SmartPointerOwnershipPolicy_ReferenceCountedAlwaysValid : public SmartPointerOwnershipPolicy_SingleAssignReferenceCountedAlwaysValid<T, Conversion<SingleAssignSmartObject, T>::EXISTS > { };

//============================================================================
// Conversion Policies
//============================================================================

	class SmartPointerConversionPolicy_Enable
	{
	public:
		enum { allow = true };
	};

	class SmartPointerConversionPolicy_Disable
	{
	public:
		enum { allow = false };
	};

//============================================================================
// Checking Policies
//============================================================================

	template<typename T>
	class SmartPointerCheckingPolicy_AssertCheck
	{
	protected:
		static void OnDefault(T /*p*/)			{ }
		static void OnInitialise(T /*p*/)		{ }
		static void OnDereference(T p)			{ JASSERT(p != nullptr); }
	};

	template<typename T>
	class SmartPointerCheckingPolicy_AssertNotNull
	{
	protected:
		static void OnDefault(T p)				{ JASSERT(p != nullptr); }
		static void OnInitialise(T p)			{ JASSERT(p != nullptr); }
		static void OnDereference(T p)			{ JASSERT(p != nullptr); }
	};

	template<typename T>
	class SmartPointerCheckingPolicy_None
	{
	protected:
		SmartPointerCheckingPolicy_None() { }
		template<typename U> SmartPointerCheckingPolicy_None(const U& a) { }

		static void OnDefault(T p)				{ }
		static void OnInitialise(T p)			{ }
		static void OnDereference(T p)			{ }
	};

	//============================================================================
	// Storage Policies
	//============================================================================

	template<typename T> class SmartPointerStoragePolicy_Default
	{
	// Made public due to stupid compilers...
	public:
		typedef		  T*	StoredType;
		typedef T *const	ConstStoredType;
		typedef		  T*	PointerType;
		typedef const T*	ConstPointerType;
		typedef		  T&	ReferenceType;
		typedef const T&	ConstReferenceType;

	public:
		PointerType			operator->()		{ return p; }
		ConstPointerType	operator->() const	{ return p; }
		ReferenceType		operator*()			{ return *p; }
		ConstReferenceType	operator*()  const	{ return *p; }

		JINLINE friend PointerType	GetPointer(const SmartPointerStoragePolicy_Default& a)	{ return a.p; }
		JINLINE PointerType			GetPointer() const										{ return p; }

	protected:
		SmartPointerStoragePolicy_Default() : p(DefaultValue()) { }
		SmartPointerStoragePolicy_Default(const SmartPointerStoragePolicy_Default &a) : p(a.p) { }
		SmartPointerStoragePolicy_Default(const StoredType& aP) : p(aP) { }

		void				Destroy()		{ delete p; }
		static StoredType	DefaultValue()	{ return nullptr; }

		JINLINE StoredType&			GetPointerReference()			{ return p; }
		JINLINE ConstStoredType&	GetPointerReference()	const	{ return p; }

	private:
		StoredType		p;
	};

	template<typename T> class SmartPointerStoragePolicy_NoDereference : public SmartPointerStoragePolicy_Default<T>
	{
	private:
		typedef SmartPointerStoragePolicy_Default<T> Inherited;

		typename Inherited::PointerType			operator->();
		typename Inherited::ConstPointerType	operator->() const;
		typename Inherited::ReferenceType		operator*();
		typename Inherited::ConstReferenceType	operator*()  const;

	public:
		SmartPointerStoragePolicy_NoDereference() { }
		SmartPointerStoragePolicy_NoDereference(const SmartPointerStoragePolicy_NoDereference &a) : Inherited(a) { }
		SmartPointerStoragePolicy_NoDereference(const typename Inherited::StoredType& aP) : Inherited(aP) { }
	};

	template<typename T> class SmartPointerStoragePolicy_Array
	{
	// Made public due to stupid compilers...
	public:
		typedef		  T*	StoredType;
		typedef T *const	ConstStoredType;
		typedef		  T*	PointerType;
		typedef const T*	ConstPointerType;
		typedef		  T&	ReferenceType;
		typedef const T&	ConstReferenceType;

	public:
		PointerType			operator->()		{ return p; }
		ConstPointerType	operator->() const	{ return p; }
		ReferenceType		operator*()			{ return *p; }
		ConstReferenceType	operator*()  const	{ return *p; }

		JINLINE friend PointerType	GetPointer(const SmartPointerStoragePolicy_Array& a) { return a.p; }
		JINLINE PointerType			GetPointer() const									 { return p; }

	protected:
		SmartPointerStoragePolicy_Array() : p(DefaultValue()) { }
		SmartPointerStoragePolicy_Array(const SmartPointerStoragePolicy_Array & a) : p(a.p) { }
		SmartPointerStoragePolicy_Array(const StoredType& aP) : p(aP) { }

		void				Destroy()		{ delete [] p; }
		static StoredType	DefaultValue()	{ return nullptr; }

		JINLINE StoredType&			GetPointerReference()			{ return p; }
		JINLINE ConstStoredType&	GetPointerReference()	const	{ return p; }

	private:
		StoredType		p;
	};

//============================================================================

	class SmartPointerStreaming
	{
	protected:
		static bool ShouldWriteObject(IWriter& output, void* object);
	};
	
//============================================================================
// SmartPointer
//============================================================================

	template<typename T,
			 template<typename> class OwnershipPolicy = SmartPointerOwnershipPolicy_ReferenceCounted,
			 class ConversionPolicy = SmartPointerConversionPolicy_Disable,
			 template<typename> class StoragePolicy = SmartPointerStoragePolicy_Default,
			 template<typename> class CheckingPolicy = SmartPointerCheckingPolicy_AssertCheck>
	class SmartPointer : public StoragePolicy<T>,
						 public OwnershipPolicy<T>,
						 public CheckingPolicy<typename StoragePolicy<T>::PointerType>,
						 private SmartPointerStreaming
	{
	private:
		typedef typename StoragePolicy<T>::StoredType			StoredType;
		typedef typename StoragePolicy<T>::PointerType			PointerType;
		typedef typename StoragePolicy<T>::ConstPointerType		ConstPointerType;
		typedef typename StoragePolicy<T>::ReferenceType		ReferenceType;
		typedef typename StoragePolicy<T>::ConstReferenceType	ConstReferenceType;

		struct InvalidConditionalType { InvalidConditionalType(PointerType) { } };
		typedef typename SelectType<ConversionPolicy::allow, PointerType, InvalidConditionalType>::Result AutomaticConversionType;

		typedef bool (SmartPointer::*conditionalType)() const;
		typedef typename SelectType<ConversionPolicy::allow, InvalidConditionalType, conditionalType>::Result ConditionalType;

		typedef typename SelectType<OwnershipPolicy<T>::destructiveCopy, SmartPointer&, const SmartPointer&>::Result	CopyType;

	public:
		SmartPointer()							{
													OwnershipPolicy<T>::OnInitialise(StoragePolicy<T>::GetPointerReference());
													CheckingPolicy<typename StoragePolicy<T>::PointerType>::OnDefault( StoragePolicy<T>::GetPointer() );
												}

		SmartPointer(const StoredType& p)		: StoragePolicy<T>(p)
												{
													OwnershipPolicy<T>::OnStoredTypeInitialise(StoragePolicy<T>::GetPointerReference());
													CheckingPolicy<typename StoragePolicy<T>::PointerType>::OnInitialise(StoragePolicy<T>::GetPointerReference());
												}

		SmartPointer(CopyType p)				: StoragePolicy<T>(p), 
												  OwnershipPolicy<T>(p)
												{ 
													StoragePolicy<T>::GetPointerReference() = OwnershipPolicy<T>::Clone(p.GetPointerReference());
													CheckingPolicy<typename StoragePolicy<T>::PointerType>::OnInitialise(StoragePolicy<T>::GetPointerReference());
												}

		SmartPointer(SmartPointer&& p)			: StoragePolicy<T>(p),
												  OwnershipPolicy<T>(p)
												{
													StoragePolicy<T>::GetPointerReference() = OwnershipPolicy<T>::Clone(p.GetPointerReference());
													CheckingPolicy<typename StoragePolicy<T>::PointerType>::OnInitialise(StoragePolicy<T>::GetPointerReference());
												}
		
		
		~SmartPointer()							{ if(!OwnershipPolicy<T>::Release(StoragePolicy<T>::GetPointer())) StoragePolicy<T>::Destroy(); }


		SmartPointer& operator=(const StoredType& p)
												{
													if(!OwnershipPolicy<T>::Release(StoragePolicy<T>::GetPointer())) StoragePolicy<T>::Destroy();
													StoragePolicy<T>::GetPointerReference() = p;
													OwnershipPolicy<T>::OnStoredTypeInitialise(StoragePolicy<T>::GetPointerReference());
													CheckingPolicy<typename StoragePolicy<T>::PointerType>::OnInitialise(StoragePolicy<T>::GetPointerReference());
													return (*this);
												}
		SmartPointer& operator=(CopyType p)		{
													PointerType newPointer = p.Clone(p.GetPointerReference());
													if(!OwnershipPolicy<T>::Release(StoragePolicy<T>::GetPointer())) StoragePolicy<T>::Destroy();
													StoragePolicy<T>::operator=(p);
													OwnershipPolicy<T>::operator=(p);
													StoragePolicy<T>::GetPointerReference() = newPointer;
													CheckingPolicy<typename StoragePolicy<T>::PointerType>::OnInitialise(StoragePolicy<T>::GetPointerReference());
													return (*this);
												}

		PointerType operator->()				{ 
													CheckingPolicy<typename StoragePolicy<T>::PointerType>::OnDereference(StoragePolicy<T>::GetPointer()); 
													return StoragePolicy<T>::operator->();
												}
		ConstPointerType operator->() const		{
													CheckingPolicy<typename StoragePolicy<T>::PointerType>::OnDereference(StoragePolicy<T>::GetPointer()); 
													return StoragePolicy<T>::operator->();
												}
		ReferenceType operator*()				{ 
													CheckingPolicy<typename StoragePolicy<T>::PointerType>::OnDereference(StoragePolicy<T>::GetPointer()); 
													return StoragePolicy<T>::operator*();
												}
		ConstReferenceType operator*() const	{
													CheckingPolicy<typename StoragePolicy<T>::PointerType>::OnDereference(StoragePolicy<T>::GetPointer()); 
													return StoragePolicy<T>::operator*();
												}

		JINLINE bool operator!() const						{ return StoragePolicy<T>::GetPointer() == 0;	}
		JINLINE operator ConditionalType() const			{ return StoragePolicy<T>::GetPointer() != 0 ? &SmartPointer::operator! : 0; }
		JINLINE operator AutomaticConversionType() const	{ return StoragePolicy<T>::GetPointer();		}
		
		JINLINE bool JCALL operator==(const SmartPointer& a) const	{ return StoragePolicy<T>::GetPointer() == a.StoragePolicy<T>::GetPointer(); }
		JINLINE bool JCALL operator!=(const SmartPointer& a) const	{ return StoragePolicy<T>::GetPointer() != a.StoragePolicy<T>::GetPointer(); }
		
		template<typename U>
		JINLINE bool JCALL operator==(U* p) const				{ return StoragePolicy<T>::GetPointer() == p; }
		template<typename U>
		JINLINE bool JCALL operator!=(U* p) const				{ return StoragePolicy<T>::GetPointer() == p; }
		
		JINLINE friend IWriter& operator<<(IWriter& output, const SmartPointer& p) { if(ShouldWriteObject(output, p.StoragePolicy<T>::GetPointer())) output << *p.StoragePolicy<T>::GetPointer(); return output; }

	protected:
		JINLINE SmartPointer(const NoInitialize*, const StoredType& p) : StoragePolicy<T>(p) { }
	};

//============================================================================
} // namespace Javelin
//============================================================================
