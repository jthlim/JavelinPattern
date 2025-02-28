//============================================================================

#pragma once
#include "Javelin/Type/VariantData/IVariantData.h"
#include "Javelin/Type/DataBlock.h"
#include "Javelin/Type/String.h"
#include "Javelin/Type/TypeData.h"

//============================================================================

namespace Javelin
{
//============================================================================
	
	class ICharacterReader;
	class IReader;
	
//============================================================================
	
	class Variant
	{
	public:
		Variant();
		Variant(bool a);
		Variant(int a);
		explicit Variant(size_t a);
		Variant(double a);
		Variant(const String& a);
		Variant(const Variant& a);
		Variant(Variant&& a) JNOTHROW;
		Variant(const void* data, size_t size);
		Variant(IReader& data, size_t size);
		Variant(const SmartPointer<DataBlock>& dataBlock);
		~Variant();

		JEXPORT static Variant JCALL CreateFromBinaryPlist(IReader& input);
		JEXPORT static Variant JCALL CreateFromBinaryPlist(const DataBlock& dataBlock);
		
		// Implemented in VariantJson.cpp
		JEXPORT static Variant JCALL CreateFromJson(ICharacterReader& input);
		JEXPORT static Variant JCALL CreateFromJson(const String& input);
		JEXPORT void JCALL WriteJson(ICharacterWriter& output, bool pretty=false, size_t depth=0) const;
		JEXPORT String JCALL AsJson(bool pretty=false) const;
		
		// Implemented in VariantYaml.cpp
		JEXPORT static Variant JCALL CreateFromYaml(ICharacterReader& input);
		JEXPORT static Variant JCALL CreateFromYaml(const String& input);
		JEXPORT void JCALL WriteYaml(ICharacterWriter& output) const;
		JEXPORT String JCALL AsYaml() const;
		
		// Implemented in VariantMessagePack.cpp
		JEXPORT static Variant JCALL CreateFromMessagePack(IReader& input);
		JEXPORT void JCALL WriteMessagePack(IWriter& output) const;
		
		JEXPORT static Variant JCALL CreateObject();

		bool 	AsBool()	const											{ return GetData()->AsBool();		}
		int		AsInteger() const											{ return GetData()->AsInteger();	}
		double 	AsDouble()	const											{ return GetData()->AsDouble();		}
		String 	AsString()	const											{ return GetData()->AsString();		}
		SmartPointer<DataBlock> AsData() const								{ return GetData()->AsData();		}
		size_t	GetHash() const												{ return GetData()->GetHash();		}
		
		friend size_t GetHash(const Variant& v)								{ return v.GetHash(); }

		JEXPORT Variant& JCALL operator=(bool a);
		JEXPORT Variant& JCALL operator=(int a);
		JEXPORT Variant& JCALL operator=(double a);
		JEXPORT Variant& JCALL operator=(const String& a);
		JEXPORT Variant& JCALL operator=(const Variant& a);
		JEXPORT Variant& JCALL operator=(Variant&& a) JNOTHROW;
		
		JEXPORT double JCALL operator+(const Variant& v) const;
		JEXPORT double JCALL operator-(const Variant& v) const;
		JEXPORT double JCALL operator*(const Variant& v) const;
		JEXPORT double JCALL operator/(const Variant& v) const;
		JEXPORT Variant& JCALL operator+=(const Variant& v);
		JEXPORT Variant& JCALL operator-=(const Variant& v);
		JEXPORT Variant& JCALL operator*=(const Variant& v);
		JEXPORT Variant& JCALL operator/=(const Variant& v);
		
		// Recursively merge another variant into this one.
		JEXPORT void	JCALL Merge(const Variant& v);
		
		JINLINE 		Variant& JCALL operator[](int i)					{ return GetData()->operator[](i);	}
		JINLINE const	Variant& JCALL operator[](int i) const				{ return GetData()->operator[](i);	}
		JINLINE 		Variant& JCALL operator[](size_t i)					{ return GetData()->operator[](i);	}
		JINLINE const	Variant& JCALL operator[](size_t i) const			{ return GetData()->operator[](i);	}
		JINLINE 		Variant& JCALL operator[](const Variant& k)			{ return GetData()->operator[](k);	}
		JINLINE const	Variant& JCALL operator[](const Variant& k) const	{ return GetData()->operator[](k);	}

		JINLINE			Variant& JCALL operator[](bool b)					{ return (*this)[Variant(b)]; 		}
		JINLINE	const	Variant& JCALL operator[](bool b) const				{ return (*this)[Variant(b)]; 		}
		JINLINE			Variant& JCALL operator[](double d)					{ return (*this)[Variant(d)]; 		}
		JINLINE	const	Variant& JCALL operator[](double d) const			{ return (*this)[Variant(d)]; 		}
		JINLINE			Variant& JCALL operator[](const String& s)			{ return (*this)[Variant(s)];		}
		JINLINE	const	Variant& JCALL operator[](const String& s) const	{ return (*this)[Variant(s)];		}
		JINLINE			Variant& JCALL operator[](const char* s) = delete;
		JINLINE	const	Variant& JCALL operator[](const char* s) const = delete;

		// This is a strict equals. ie. 0 != false, 5 != "5"
		// Objects are equal if they point to the same object!
		JEXPORT bool JCALL operator==(const Variant& a)	const;
		JEXPORT bool JCALL operator==(int a) const;
		JEXPORT bool JCALL operator==(size_t a) const;
		JEXPORT bool JCALL operator==(double a) const;
		JINLINE bool JCALL operator!=(const Variant& a)	const	{ return !(*this == a); }
		JINLINE bool JCALL operator!=(int a)	const			{ return !(*this == a); }
		JINLINE bool JCALL operator!=(size_t a)	const			{ return !(*this == a); }
		JINLINE bool JCALL operator!=(double a)	const			{ return !(*this == a); }
		JEXPORT bool JCALL operator<(const Variant& a)	const;
		JEXPORT bool JCALL operator<=(const Variant& a)	const;
		JINLINE bool JCALL operator>(const Variant& a)	const	{ return a < *this; }
		JINLINE bool JCALL operator>=(const Variant& a)	const	{ return a <= *this; }
		
		JINLINE size_t JCALL GetLength() const			{ return GetData()->GetLength(); }
			
		JINLINE bool JCALL IsEmpty() const				{ return GetVariantDataVTbl() == EMPTY.GetVariantDataVTbl();	}
		JINLINE bool JCALL IsNumber() const				{ return GetVariantDataVTbl() == ZERO.GetVariantDataVTbl();		}
		JINLINE bool JCALL IsString() const				{ return GetType() == VariantType::String; 						}
		JINLINE bool JCALL IsObject() const				{ return GetType() == VariantType::Object; 						}
			
		template<typename T> JEXPORT T JCALL GetValueWithDefault(const String& keyName, typename TypeData<T>::ParameterType defaultValue=T()) const;
			
		static const Variant EMPTY;
		static const Variant ZERO;
		static const Variant ONE;
			
	private:
		Variant(const char* a) = delete;
			
		void Destroy();
			
		JINLINE VariantType GetType() const				{ return GetData()->GetType(); }
			
		JINLINE IVariantData* GetData() 				{ return reinterpret_cast<IVariantData*>(&variantData); }
		JINLINE const IVariantData* GetData() const 	{ return reinterpret_cast<const IVariantData*>(&variantData); }

		JINLINE const void*	GetVariantDataVTbl() const	{ return variantData.vtbl; }
		
		struct VariantDataStore
		{
			void*	vtbl;
			union
			{
				double	doubleValue;
				void*	pointerValue;
			};
		};
		VariantDataStore	variantData;
		
		friend class IVariantData;
	
		friend class ObjectVariantData;
		JEXPORT void JCALL WriteYaml(ICharacterWriter& output, size_t depth, bool lastWasArray) const;
																		  
		JEXPORT int		JCALL GetIntWithDefault(const String& keyName, int defaultValue) const;
		JEXPORT double	JCALL GetDoubleWithDefault(const String& keyName, double defaultValue) const;
		JEXPORT String	JCALL GetStringWithDefault(const String& keyName, const String& defaultValue) const;
	};

//============================================================================
			
	template<> JINLINE int JCALL Variant::GetValueWithDefault<int>(const String& keyName, int defaultValue) 				const		{ return GetIntWithDefault(keyName, defaultValue);		}
	template<> JINLINE double JCALL Variant::GetValueWithDefault<double>(const String& keyName, double defaultValue)		const		{ return GetDoubleWithDefault(keyName, defaultValue); 	}
	template<> JINLINE String JCALL Variant::GetValueWithDefault<String>(const String& keyName, const String& defaultValue)	const		{ return GetStringWithDefault(keyName, defaultValue);	}

//============================================================================
	
	JINLINE String JCALL ToString(const Variant& a) { return a.AsYaml(); }
	
//============================================================================

	template<> struct TypeData<Variant>
	{
		enum { IS_POD				= false };
		enum { IS_BITWISE_COPY_SAFE	= true	};
		enum { IS_POINTER			= false };
		
		typedef Variant NonPointerType;
		typedef Variant NonReferenceType;
		typedef Variant NonConstType;
		typedef Variant StorageType;
		
		typedef Variant ParameterUpcast;
		typedef const Variant& ParameterType;
		typedef Variant&& MoveType;
		typedef Variant& ReferenceType;
		
		typedef Variant MathematicalUpcast;
	};
	
//============================================================================
} // namespace Javelin
//============================================================================
