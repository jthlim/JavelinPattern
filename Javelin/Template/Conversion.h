//============================================================================

#pragma once

//============================================================================

namespace Javelin
{
//============================================================================

	template<typename TO, typename FROM> class Conversion
	{
	private:
		typedef char Small;
		struct Big { char dummy[2]; };

		static Small Test(const TO&);
		static Big   Test(...);

		static FROM& MakeFROM();

	public:
		enum { EXISTS = sizeof(Test(MakeFROM())) == sizeof(Small) };
		enum { SAME_TYPE = false };
	};

	template<typename T> class Conversion<T, T>
	{
	public:
		enum { EXISTS = true };
		enum { SAME_TYPE = true };
	};

//============================================================================
} // namespace Javelin
//============================================================================
