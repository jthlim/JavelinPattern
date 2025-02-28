//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include "Javelin/Container/Optional.h"
#include "Javelin/Math/BitUtility.h"
#include "Javelin/System/Assert.h"
#include "Javelin/Template/Sequence.h"
#include "Javelin/Type/Interval.h"

//============================================================================

namespace Javelin
{
//============================================================================

	namespace Private
	{
		class BitTableBase
		{
		protected:
			static void ClearAllBitsInTable(unsigned* data, size_t numberOfWords);
			static void SetAllBitsInTable(unsigned* data, size_t numberOfWords);
			static void AndAllBitsInTable(unsigned* data, const unsigned* other, size_t numberOfWords);
			static void OrAllBitsInTable(unsigned* data, const unsigned* other, size_t numberOfWords);
			static void NotAllBitsInTable(unsigned* out, const unsigned* in, size_t numberOfWords);
			static bool HasAllBitSetInTable(const unsigned* data, size_t numberOfBits);
			static bool HasAnyBitSetInTable(const unsigned* data, size_t numberOfWords);
			static Optional<Interval<size_t>> GetContiguousRange(const unsigned* data, size_t numberOfWords);
			static size_t CountBitsInTable(const unsigned* data, size_t numberOfWords);
			static size_t CountTrailingZeros(const unsigned* data, size_t numberOfWords);
		};
		
		template<size_t N, bool...> struct GetBitHelper;
		
		template<size_t N, bool b1, bool b2, bool ...bs> struct GetBitHelper<N, b1, b2, bs...>
		{
			static constexpr bool GetBit() { return N == 0 ? b1 : N >= 2 ? GetBitHelper<N-2, bs...>::GetBit() : GetBitHelper<N-1, b2, bs...>::GetBit(); }
		};
		
		template<size_t N, bool b> struct GetBitHelper<N, b>
		{
			static constexpr bool GetBit() { return b && N == 0; }
		};
		
		template<size_t N> struct GetBitHelper<N>
		{
			static constexpr bool GetBit() { return false; }
		};
		
		template<size_t N, bool ...bs> struct GetBitWordHelper
		{
			static constexpr unsigned GetWord()
			{
				return GetBitHelper<N*32, bs...>::GetBit()
				| GetBitHelper<N*32+1, bs...>::GetBit() << 1
				| GetBitHelper<N*32+2, bs...>::GetBit() << 2
				| GetBitHelper<N*32+3, bs...>::GetBit() << 3
				| GetBitHelper<N*32+4, bs...>::GetBit() << 4
				| GetBitHelper<N*32+5, bs...>::GetBit() << 5
				| GetBitHelper<N*32+6, bs...>::GetBit() << 6
				| GetBitHelper<N*32+7, bs...>::GetBit() << 7
				| GetBitHelper<N*32+8, bs...>::GetBit() << 8
				| GetBitHelper<N*32+9, bs...>::GetBit() << 9
				| GetBitHelper<N*32+10, bs...>::GetBit() << 10
				| GetBitHelper<N*32+11, bs...>::GetBit() << 11
				| GetBitHelper<N*32+12, bs...>::GetBit() << 12
				| GetBitHelper<N*32+13, bs...>::GetBit() << 13
				| GetBitHelper<N*32+14, bs...>::GetBit() << 14
				| GetBitHelper<N*32+15, bs...>::GetBit() << 15
				| GetBitHelper<N*32+16, bs...>::GetBit() << 16
				| GetBitHelper<N*32+17, bs...>::GetBit() << 17
				| GetBitHelper<N*32+18, bs...>::GetBit() << 18
				| GetBitHelper<N*32+19, bs...>::GetBit() << 19
				| GetBitHelper<N*32+20, bs...>::GetBit() << 20
				| GetBitHelper<N*32+21, bs...>::GetBit() << 21
				| GetBitHelper<N*32+22, bs...>::GetBit() << 22
				| GetBitHelper<N*32+23, bs...>::GetBit() << 23
				| GetBitHelper<N*32+24, bs...>::GetBit() << 24
				| GetBitHelper<N*32+25, bs...>::GetBit() << 25
				| GetBitHelper<N*32+26, bs...>::GetBit() << 26
				| GetBitHelper<N*32+27, bs...>::GetBit() << 27
				| GetBitHelper<N*32+28, bs...>::GetBit() << 28
				| GetBitHelper<N*32+29, bs...>::GetBit() << 29
				| GetBitHelper<N*32+30, bs...>::GetBit() << 30
				| GetBitHelper<N*32+31, bs...>::GetBit() << 31;
			}
		};
		
	}

//============================================================================
	
	template<bool... bs> struct BitTableInitializer
	{
		template<size_t N> static constexpr unsigned GetWord() { return Private::GetBitWordHelper<N, bs...>::GetWord(); }
	};
	
//============================================================================

	template<size_t N> class StaticBitTable : private Private::BitTableBase
	{
	public:
		// Default constructor zero-initializes all bits
		constexpr StaticBitTable() : StaticBitTable(typename CreateSequence<GetNumberOfWords()>::Type())	{ }
		
		StaticBitTable(const NoInitialize*) { }
		
		// BitTableInitializer constructor can compile time setup BitTable
		template<bool... bs> constexpr StaticBitTable(BitTableInitializer<bs...> bf) : StaticBitTable(BitTableInitializer<bs...>(), typename CreateSequence<GetNumberOfWords()>::Type()) { }
		
		size_t GetCount() const								{ return N; }
		
		void SetBit(size_t bitIndex)						{ JASSERT(bitIndex < N); BitUtility::SetBit(data, bitIndex);	}
		void ClearBit(size_t bitIndex)						{ JASSERT(bitIndex < N); BitUtility::ClearBit(data, bitIndex);	}

		// Will clear based on words, ie. a few extra bits could be zeroed.
		void ClearFirstNBits(size_t n)						{ JASSERT(n <= N); Private::BitTableBase::ClearAllBitsInTable(data, GetNumberOfWordsForBits(n)); }
		
		void SetAllBits()									{ Private::BitTableBase::SetAllBitsInTable(data, GetNumberOfWords()); 	}
		void ClearAllBits()									{ Private::BitTableBase::ClearAllBitsInTable(data, GetNumberOfWords()); }
		
		bool HasAnyBitSet() const							{ return Private::BitTableBase::HasAnyBitSetInTable(data, GetNumberOfWords()); }
		bool HasAllBitsSet() const							{ return Private::BitTableBase::HasAllBitSetInTable(data, N); }
		bool HasAllBitsClear() const						{ return !HasAnyBitSet(); }
		
		// returns minimium i for which bitTable[i] == true
		size_t CountTrailingZeros() const					{ return Private::BitTableBase::CountTrailingZeros(data, GetNumberOfWords()); }
		
		bool IsContiguous() const 							{ return Private::BitTableBase::GetContiguousRange(data, GetNumberOfWords()).HasValue(); }
		Optional<Interval<size_t>> GetContiguousRange() const { return Private::BitTableBase::GetContiguousRange(data, GetNumberOfWords()); }
		
		// Count the number of 1 bits
		size_t GetPopulationCount() const					{ return Private::BitTableBase::CountBitsInTable(data, GetNumberOfWords()); }
		
		bool operator[](size_t bitIndex) const				{ JASSERT(bitIndex < N); return BitUtility::IsBitSet(data, bitIndex); 	}
		BitUtility::BitProxy operator[](size_t bitIndex)	{ JASSERT(bitIndex < N); return BitUtility::BitProxy(data, bitIndex);	}
		
		bool operator==(const StaticBitTable& a) const		{ return memcmp(data, a.data, sizeof(data)) == 0; }
		
		StaticBitTable& operator|=(const StaticBitTable& a) { Private::BitTableBase::OrAllBitsInTable(data, a.data, GetNumberOfWords()); return *this; }
		StaticBitTable& operator&=(const StaticBitTable& a) { Private::BitTableBase::AndAllBitsInTable(data, a.data, GetNumberOfWords()); return *this; }
		
		StaticBitTable operator|(const StaticBitTable& a) const { StaticBitTable r{*this}; r |= a; return r; }
		StaticBitTable operator&(const StaticBitTable& a) const { StaticBitTable r{*this}; r &= a; return r; }
		StaticBitTable operator~() const 						{ StaticBitTable r{NO_INITIALIZE}; Private::BitTableBase::NotAllBitsInTable(r.data, data, GetNumberOfWords()); return r; }
		
	private:
		template<int ...S> constexpr StaticBitTable(Sequence<S...>)
		: data{ S * 0 ... } { }
		
		template<bool... bs, int ...S> constexpr StaticBitTable(BitTableInitializer<bs...>, Sequence<S...>)
		: data{ BitTableInitializer<bs...>::template GetWord<S>()... } { }

		static constexpr size_t GetNumberOfWords() 					{ return (N+31)/32; }
		static constexpr size_t GetNumberOfWordsForBits(size_t n) 	{ return (n+31)/32; }
		
		unsigned	data[GetNumberOfWords()];
	};
	
//============================================================================
	
	class BitTable : private Private::BitTableBase
	{
	public:
		BitTable();
		BitTable(size_t numberOfBits);
		BitTable(const BitTable &a);
		BitTable(BitTable&& a);
		~BitTable();
		
		BitTable& operator=(const BitTable& a)				{ this->~BitTable(); new(Placement(this)) BitTable(a); return *this; }
		BitTable& operator=(BitTable&& a)					{ this->~BitTable(); new(Placement(this)) BitTable((BitTable&&) a); return *this; }
		
		size_t GetCount() const								{ return numberOfBits; }
		void SetCount(size_t numberOfBits);
		
		void SetBit(size_t bitIndex)						{ JASSERT(bitIndex < numberOfBits); BitUtility::SetBit(data, bitIndex); }
		void ClearBit(size_t bitIndex)						{ JASSERT(bitIndex < numberOfBits); BitUtility::ClearBit(data, bitIndex); }
		
		void SetAllBits()									{ Private::BitTableBase::SetAllBitsInTable(data, GetNumberOfWords(numberOfBits)); 	}
		void ClearAllBits()									{ Private::BitTableBase::ClearAllBitsInTable(data, GetNumberOfWords(numberOfBits)); }

		bool HasAnyBitSet() const							{ return Private::BitTableBase::HasAnyBitSetInTable(data, capacityInWords); }
		bool HasAllBitsSet() const							{ return Private::BitTableBase::HasAllBitSetInTable(data, numberOfBits); 	}
		bool HasAllBitsClear() const						{ return !HasAnyBitSet(); }

		bool operator[](size_t bitIndex) const				{ JASSERT(bitIndex < numberOfBits); return BitUtility::IsBitSet(data, bitIndex); 	}
		BitUtility::BitProxy operator[](size_t bitIndex)	{ JASSERT(bitIndex < numberOfBits); return BitUtility::BitProxy(data, bitIndex);	}
		
	private:
		static constexpr size_t GetNumberOfWords(size_t numberOfBits) { return (numberOfBits+31)/32; }
		
		size_t 		numberOfBits;
		size_t		capacityInWords;
		unsigned*	data;
	};
	
//============================================================================
}
//============================================================================
