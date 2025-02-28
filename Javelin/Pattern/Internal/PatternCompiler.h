//============================================================================

#pragma once
#include "Javelin/Type/String.h"
#include "Javelin/Type/DataBlock.h"
#include "Javelin/Container/Table.h"
#include "Javelin/Pattern/Internal/PatternComponent.h"
#include "Javelin/Pattern/Internal/PatternInstructionList.h"
#include "Javelin/Pattern/Internal/PatternTokenizerBase.h"

//============================================================================

#define JDUMP_PATTERN_INFORMATION	0

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================

	class CaptureComponent;
	class CounterComponent;
	class IComponent;
	
	class Compiler
	{
	public:
		Compiler(Utf8Pointer pattern, Utf8Pointer patternEnd, int patternOptions);
		~Compiler();

		void Compile(int options);

		void 				ResolveRecurseInstructions();
		uint32_t 			GetNumberOfCaptures() const		{ return numberOfCaptures;	}
		const DataBlock&	GetByteCode() const				{ return byteCode;			}
		
	private:
		IComponent* CompileCapture(int options);
		IComponent*	CompilePattern(int options);
		IComponent* CompileGroup(int &options);
		IComponent* CompileAtom(int options);
		IComponent* CompileAtomQuantifier(IComponent* atom, int options);
	
		DataBlock					byteCode;
		String						pattern;
		bool						usesBacktrackingComponents = false;
		uint32_t					numberOfCaptures;
		Table<CaptureComponent*>	captureGroups;
		CaptureComponent*			outerCapture = nullptr;
		TokenizerBase*				tokenizer;

		typedef IntrusiveList<RecurseComponent, &RecurseComponent::listNode> RecurseComponentLinkedList;
		RecurseComponentLinkedList	recurseComponentList;

		InstructionList				instructionList;

		void PrintInformation(ICharacterWriter& output, IComponent* headComponent) const;
		
		// Returns true if coalescing has occured and the component is not required.
		static bool CoalesceComponent(CounterComponent* previousComponent, IComponent* component);
		void ResolveRecurseComponents();
	};

//============================================================================
} // namespace Javelin::PatternInternal
//============================================================================
