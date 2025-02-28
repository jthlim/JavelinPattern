//==========================================================================

#include "Javelin/Pattern/Internal/PatternTypes.h"
#include "Javelin/Stream/ICharacterWriter.h"
#include "Javelin/Template/Utility.h"
#include "Javelin/Type/Character.h"

//==========================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//==========================================================================

bool CharacterRangeList::IsSingleByteUtf8() const
{
	return Back().max < 128;
}

uint32_t CharacterRangeList::GetNumberOfMatchingBytes() const
{
	uint32_t result = 0;
	for(const auto& it : *this)
	{
		result += it.GetSize()+1;
	}
	return result;
}

bool CharacterRangeList::Contains(Character c) const
{
	// TODO: Use binary search here.
	for(const auto& it : *this)
	{
		if(it.Contains(c)) return true;
	}
	return false;
}

void CharacterRangeList::Add(CharacterRange interval)
{
	// Check if it's already in the list.
	for(const auto& it : *this)
	{
		if(it.Contains(interval)) return;
	}
	
	int mergeIndex = -1;
	for(size_t i = GetCount(); i > 0; --i)
	{
		CharacterRange& range = (*this)[i-1];
		
		CharacterRange checkInterval;
		checkInterval.min = (range.min < interval.min) ? interval.min-1 : interval.min;
		checkInterval.max = (range.max > interval.max) ? interval.max+1 : interval.max;

		if(range % checkInterval)
		{
			if(mergeIndex < 0)
			{
				range |= checkInterval;
				mergeIndex = (int) i-1;
				interval = range;
			}
			else
			{
				CharacterRange& merge = (*this)[mergeIndex];
				merge |= range;
				RemoveIndex(i-1);
				--mergeIndex;
			}
		}
	}
	if(mergeIndex < 0) Table::Append(interval);
}

void CharacterRangeList::Sort()
{
	QuickSort<MemberComparator<Interval<Character, true>, Character, &CharacterRange::min>>();
}

void CharacterRangeList::Dump(ICharacterWriter& output)
{
	output.WriteByte('{');
	for(const auto& it : *this)
	{
		output.PrintF(" [%u, %u]", (unsigned) it.min, (unsigned) it.max);
	}
	output.PrintF(" }");
}

CharacterRangeList CharacterRangeList::CreateComplement() const
{
	CharacterRangeList result;
	if(IsEmpty())
	{
		result.Append(0, Character::Maximum());
	}
	else
	{
		if((*this)[0].min != 0)
		{
			result.Append(0, (*this)[0].min-1);
		}
		for(size_t i = 1; i < GetCount(); ++i)
		{
			result.Append((*this)[i-1].max+1, (*this)[i].min-1);
		}
		const CharacterRange& last = Back();
		if(last.max != Character::Maximum())
		{
			result.Append(last.max+1, Character::Maximum());
		}
	}
	return result;
}

CharacterRangeList CharacterRangeList::CreateAsciiRange() const
{
    static constexpr CharacterRange ASCII_RANGE{0,255};

    CharacterRangeList result;
	
	for(const CharacterRange& range : *this)
	{
		CharacterRange intersection = range & ASCII_RANGE;
		if(intersection.IsValid())
		{
			result.Append(intersection);
		}
	}
	return result;
}

CharacterRangeList CharacterRangeList::CreateCaseInsensitive() const
{
	static constexpr CharacterRange UPPER{'A', 'Z'};
	static constexpr CharacterRange LOWER{'a', 'z'};
	
	CharacterRangeList result;
	
	for(const CharacterRange& range : *this)
	{
		result.Add(range);
		
		CharacterRange upperOverlap = range & UPPER;
		if(upperOverlap.IsValid())
		{
			result.Add(upperOverlap + ('a' - 'A'));
		}
		
		CharacterRange lowerOverlap = range & LOWER;
		if(lowerOverlap.IsValid())
		{
			result.Add(lowerOverlap + ('A' - 'a'));
		}
	}
	result.Sort();
	
	return result;
}

CharacterRangeList CharacterRangeList::CreateUnicodeCaseInsensitive() const
{
	CharacterRangeList result;
	const CaseConversionData* conversionData = CaseConversionData::DATA;
	const CaseConversionData* conversionDataEnd = CaseConversionData::DATA + CaseConversionData::COUNT;
	
	// Ranges are sorted, so this will process increasing ranges.
	for(const CharacterRange& range : *this)
	{
		result.Add(range);

		if(range.max < conversionData->range.min) continue;
		while(range.min < conversionData->range.max)
		{
			if(conversionData == conversionDataEnd-1) goto Next;
			++conversionData;
		}
		
		{
			const CaseConversionData* p = conversionData;
			while(p->range.min <= range.max && p < conversionDataEnd)
			{
				CharacterRange overlap = range & p->range;
				if(overlap.IsValid())
				{
					switch(p->delta)
					{
					case CaseConversionData::MERGE_ALL:
						if(overlap != p->range)
						{
							result.Add(p->range);
						}
						break;
					
					case CaseConversionData::MERGE_EVEN_ODD:
						if(overlap != p->range)
						{
							overlap.min &= -2;
							overlap.max |= 1;
							result.Add(overlap);
						}
						break;
					
					case CaseConversionData::MERGE_ODD_EVEN:
						if(overlap != p->range)
						{
							overlap.min = ((overlap.min + 1) & -2) - 1;
							overlap.max = ((overlap.max - 1) | 1) + 1;
							result.Add(overlap);
						}
						break;

					default:
						if((p->delta & CaseConversionData::MERGE_FIXED_MASK) == CaseConversionData::MERGE_FIXED)
						{
							result.Add(p->delta & ~CaseConversionData::MERGE_FIXED_MASK);
						}
						else
						{
							result.Add(overlap + p->delta);
						}
						break;
					}
				}
				
				++p;
			}
		}
		
	Next:
		;
	}
	result.Sort();
	
	
	return result;
}

void CharacterRange::TrimRelevancyInterval(CharacterRange& range) const
{
	if(range.min < min)
	{
		if(range.max >= min) range.max = min-1;
	}
	else if(range.min <= max)
	{
		if(range.max > max) range.max = max;
	}
}

void CharacterRangeList::TrimRelevancyInterval(CharacterRange& range) const
{
	for(const CharacterRange& r : *this)
	{
		r.TrimRelevancyInterval(range);
	}
	if(range.min < 256)
	{
		if(range.max > 255) range.max = 255;
	}
}

//==========================================================================

void Token::operator=(Token &&a)
{
	type		= a.type;
	c			= a.c;
	i			= a.i;
	stringData	= Move(a.stringData);
	rangeList	= Move(a.rangeList);
}

//==========================================================================

String Token::GetString() const
{
	return String(stringData.GetData(), stringData.GetCount());
}

//==========================================================================
