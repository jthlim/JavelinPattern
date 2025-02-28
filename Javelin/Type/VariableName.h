//===========================================================================

#pragma once
#include "Javelin/Container/Table.h"
#include "Javelin/Type/String.h"

//===========================================================================

namespace Javelin
{
//===========================================================================

	class VariableName : public Table<String>
	{
	public:
		class CamelCase { };
		constexpr static const CamelCase* CAMEL_CASE = nullptr;
		
		class Underscore { };
		constexpr static const Underscore* UNDERSCORE = nullptr;
		
		VariableName(const String& s, const CamelCase*)		{ InitWithCamelCase(s); 			}
		VariableName(const String& s, const Underscore*)	{ InitWithSeparator(s, '_'); 		}
		VariableName(const String& s, char separator)		{ InitWithSeparator(s, separator); 	}
		
		JEXPORT String JCALL AsLowerCamelCase() const;
		JEXPORT String JCALL AsUpperCamelCase() const;
		JINLINE String JCALL AsLowerUnderscore() const		{ return AsLowerWithSeparator('_'); }
		JINLINE String JCALL AsUpperUnderscore() const		{ return AsUpperWithSeparator('_'); }
		JEXPORT String JCALL AsLowerWithSeparator(char c) const;
		JEXPORT String JCALL AsUpperWithSeparator(char c) const;
		
	private:
		void AddPart(const Utf8Pointer& pStart, const Utf8Pointer& pEnd);
		
		size_t GetPartLength() const;
		
		void InitWithCamelCase(const String& s);
		void InitWithSeparator(const String& s, char separator);
	};
	
//===========================================================================
} // namespace Javelin
//===========================================================================
