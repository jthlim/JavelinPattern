//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Type/DataBlock.h"
#include "Javelin/Type/String.h"

//============================================================================

namespace Javelin
{
//============================================================================

	class ICharacterWriter;
	class Variant;

//============================================================================

	enum class VariantType
	{
		Empty,
		Boolean,
		Number,
		String,
		Object,
		DataBlock
	};
	
//============================================================================

	class JABSTRACT IVariantData
	{
	public:
		virtual ~IVariantData() { }

		virtual bool 	AsBool()	const;
		virtual int 	AsInteger() const;
		virtual double 	AsDouble() 	const;
		virtual String 	AsString()	const;
		virtual SmartPointer<DataBlock> AsData() const;
		
		virtual size_t GetHash() const = 0;

		virtual void	CloneTo(Variant& target) const = 0;
		virtual void	Release() { }
		
		virtual VariantType GetType() const = 0;
		virtual bool IsEqual(const IVariantData* data) const = 0;
		virtual bool IsLessThan(const IVariantData* data) const = 0;
		virtual bool IsLessThanOrEqual(const IVariantData* data) const = 0;

		virtual void JCALL Merge(const IVariantData* v);
		
		virtual 		Variant& JCALL operator[](int i);
		virtual const	Variant& JCALL operator[](int i) const;
		virtual 		Variant& JCALL operator[](size_t i);
		virtual const	Variant& JCALL operator[](size_t i) const;
		virtual 		Variant& JCALL operator[](const Variant& k);
		virtual const	Variant& JCALL operator[](const Variant& k) const;

		virtual size_t	JCALL GetLength() const;
		
		virtual void	JCALL WriteJson(ICharacterWriter& writer, bool pretty, size_t depth) const = 0;
		virtual void	JCALL WriteYaml(ICharacterWriter& writer, size_t depth, bool lastWasArray) const;
		virtual void	JCALL WriteMessagePack(IWriter& writer) const = 0;
		
		static const IVariantData* GetVariantData(const Variant& variant);	// Implemented in ObjectVariantData, where it's used for JSON pretty printing
		
		// Implemented in Variant.cpp for performance
		static void* operator new(size_t size, Variant& variant);
		static void operator delete(void*, Variant&)				{ }
		static void operator delete(void*)							{ }
	};

//============================================================================
} // namespace Javelin
//============================================================================
