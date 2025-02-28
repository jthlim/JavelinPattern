//============================================================================

#include "Javelin/Container/AutoPointer.h"
#include "Javelin/Container/EnumSet.h"
#include "Javelin/Pattern/Internal/PatternCompiler.h"
#include "Javelin/Pattern/Internal/PatternComponent.h"
#include "Javelin/Pattern/Internal/PatternInstruction.h"
#include "Javelin/Pattern/Internal/PatternProcessor.h"
#include "Javelin/Pattern/Internal/PatternProcessorType.h"
#include "Javelin/Pattern/Internal/PatternTokenizer.h"
#include "Javelin/Pattern/Internal/PatternTokenizerGlob.h"
#include "Javelin/Pattern/Pattern.h"
#include "Javelin/Stream/DataBlockWriter.h"
#include "Javelin/Stream/StandardWriter.h"
#include "Javelin/Type/Character.h"
#include "Javelin/Type/Exception.h"

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

static const int MAXIMUM_REPETITION_COUNT	= 1000;

//============================================================================

static PatternProcessorType GetProcessorTypeForOptions(int options)
{
	switch(options & Pattern::PREFER_MASK)
	{
	case Pattern::PREFER_NFA:				return PatternProcessorType::Nfa;
	case Pattern::PREFER_SCAN_AND_CAPTURE:	return PatternProcessorType::ScanAndCapture;
	case Pattern::PREFER_BACK_TRACKING:		return PatternProcessorType::BackTracking;
	case Pattern::PREFER_NO_SCAN:			return PatternProcessorType::NoScan;
	default:								return PatternProcessorType::Default;
	}
}

Compiler::Compiler(Utf8Pointer pattern, Utf8Pointer patternEnd, int patternOptions)
: pattern(pattern.GetCharPointer(), patternEnd.GetCharPointer()-pattern.GetCharPointer())
{
	tokenizer = (patternOptions & Pattern::GLOB_SYNTAX) ?
					(TokenizerBase*) new GlobTokenizer(pattern, patternEnd, (patternOptions & Pattern::UTF8) != 0) :
					(TokenizerBase*) new Tokenizer(pattern, patternEnd, (patternOptions & Pattern::UTF8) != 0);
}

Compiler::~Compiler()
{
	delete tokenizer;
}

void Compiler::Compile(int options)
{
	AutoPointer<IComponent> component = CompileCapture(options);
	JPATTERN_VERIFY(tokenizer->PeekCurrentTokenType() == TokenType::End, UnexpectedToken, tokenizer->GetExceptionData());

	ResolveRecurseComponents();
	
	if(options & Pattern::AUTO_CLUSTER)
	{
		numberOfCaptures = 0;
		for(CaptureComponent* capture : captureGroups)
		{
			if(capture->isRecurseTarget
			   || capture->isBackReferenceTarget
			   || capture->captureIndex == 0)
			{
				capture->captureIndex = numberOfCaptures++;
			}
			else
			{
				capture->emitSaveInstructions = false;
			}
		}
	}
	else
	{
		numberOfCaptures = captureGroups.GetCount();
	}
	
#if JDUMP_PATTERN_INFORMATION
	PrintInformation(StandardOutput, component);
#endif
	
	PatternProcessorType processorType = GetProcessorTypeForOptions(options);
	instructionList.Build(options, InstructionList::Forwards, component, *this, usesBacktrackingComponents, processorType);
	
	DataBlockWriter byteCodeWriter;
	instructionList.WriteByteCode(byteCodeWriter, pattern, GetNumberOfCaptures());

#if JDUMP_PATTERN_INFORMATION
	instructionList.Dump(StandardOutput);
#endif
	
	ByteCodeHeader* header = (ByteCodeHeader*) byteCodeWriter.GetData();
	if(header->RequiresReverseProgram() && header->flags.reverseProcessorType == PatternProcessorType::None)
	{
		InstructionList reverseProgram;
		reverseProgram.Build(options, InstructionList::Reverse, component, *this, usesBacktrackingComponents, processorType);
		reverseProgram.AppendByteCode(byteCodeWriter);

#if JDUMP_PATTERN_INFORMATION
		StandardOutput.PrintF("Reverse\n");
		reverseProgram.Dump(StandardOutput);
#endif
	}

	byteCode = (DataBlock&&) byteCodeWriter.GetBuffer();
	
#if JDUMP_PATTERN_INFORMATION
	StandardOutput.PrintF("ByteCode\n");
	StandardOutput.DumpCStructure(byteCode);
#endif
}

void Compiler::ResolveRecurseInstructions()
{
	for(RecurseComponent& component : recurseComponentList)
	{
		if(component.recurseInstructionList.IsEmpty()) continue;

		CaptureComponent* target = component.target;
		JPATTERN_VERIFY(target->firstInstruction != nullptr, InternalError, nullptr);
		
		for(RecurseInstruction* instruction : component.recurseInstructionList)
		{
			instruction->callTarget = target->firstInstruction;
		}
	}
}

void Compiler::ResolveRecurseComponents()
{
	for(RecurseComponent& component : recurseComponentList)
	{
		JPATTERN_VERIFY(component.captureIndex >= 0 && component.captureIndex < captureGroups.GetCount(), UnableToResolveRecurseTarget, nullptr);
		CaptureComponent* target = captureGroups[component.captureIndex];;
		component.target = target;
		target->isRecurseTarget = true;
	}
}

void Compiler::PrintInformation(ICharacterWriter& output, IComponent* headComponent) const
{
#if JDUMP_PATTERN_INFORMATION
	StandardOutput.PrintF("Information for pattern: %A\n", &pattern);
#endif
	
	headComponent->Dump(StandardOutput, 0);
	StandardOutput.PrintF("MinimumLength: %z\n", headComponent->GetMinimumLength());
	StandardOutput.PrintF("HasStartAnchor: %s\n", headComponent->HasStartAnchor() ? "true" : "false");
	StandardOutput.PrintF("HasEndAnchor: %s\n", headComponent->HasEndAnchor() ? "true" : "false");
}

IComponent*	Compiler::CompilePattern(int options)
{
	IComponent* component = CompileGroup(options);

	if(tokenizer->PeekCurrentTokenType() != TokenType::Alternate) return component;
	
	AutoPointer<AlternationComponent> alternation = new AlternationComponent;
	alternation->componentList.Append(component);
	do
	{
		tokenizer->ProcessTokens();
		alternation->componentList.Append(CompileGroup(options));
	} while(tokenizer->PeekCurrentTokenType() == TokenType::Alternate);

	alternation->Optimize();
	
	if(alternation->componentList.GetCount() == 1)
	{
		IComponent* singleChild = alternation->componentList[0];
		alternation->componentList.SetCount(0);
		return singleChild;
	}
	
	return alternation.RelinquishPointer();
}

IComponent* Compiler::CompileGroup(int &options)
{
	AutoPointer<ConcatenateComponent> group = new ConcatenateComponent;
	
	for(;;)
	{
		const Token& currentToken = tokenizer->PeekCurrentToken();
		switch(currentToken.type)
		{
		case TokenType::OptionChange:
			options = (options & ~currentToken.optionMask) | currentToken.optionSet;
			tokenizer->ProcessTokens();
			break;

		case TokenType::Accept:
		case TokenType::AnyByte:
		case TokenType::AtomicGroup:
		case TokenType::BackReference:
		case TokenType::CaptureStart:
		case TokenType::ClusterStart:
		case TokenType::Character:
		case TokenType::Conditional:
		case TokenType::EndOfInput:
		case TokenType::EndOfLine:
		case TokenType::Fail:
		case TokenType::StartOfSearch:
		case TokenType::NegativeLookAhead:
		case TokenType::NegativeLookBehind:
		case TokenType::NotRange:
		case TokenType::NotWhitespaceCharacter:
		case TokenType::NotWordBoundary:
		case TokenType::NotWordCharacter:
		case TokenType::PositiveLookAhead:
		case TokenType::PositiveLookBehind:
		case TokenType::Range:
		case TokenType::Recurse:
		case TokenType::RecurseRelative:
		case TokenType::ResetCapture:
		case TokenType::StartOfInput:
		case TokenType::StartOfLine:
		case TokenType::WhitespaceCharacter:
		case TokenType::Wildcard:
		case TokenType::WordBoundary:
		case TokenType::WordCharacter:
			{
				IComponent* component = CompileAtom(options);
				if(group->componentList.HasData() && group->componentList.Back()->IsCounter())
				{
					CounterComponent* previousComponent = (CounterComponent*) group->componentList.Back();
					if(CoalesceComponent(previousComponent, component))
					{
						delete component;
						break;
					}
				}
				
				group->componentList.Append(component);
				break;
			}
				
		default:
			switch(group->componentList.GetCount())
			{
			case 0:
				return new EmptyComponent;
				
			case 1:
				{
					IComponent* result = group->componentList[0];
					group->componentList.Reset();
					return result;
				}
				
			default:
				return group.RelinquishPointer();
			}
		}
	}
}

bool Compiler::CoalesceComponent(CounterComponent* previousComponent, IComponent* component)
{
	// Opportunity for coalescing!
	if(previousComponent->content->IsEqual(component))
	{
		previousComponent->minimum++;
		if(previousComponent->maximum != TypeData<uint32_t>::Maximum()) previousComponent->maximum++;
		return true;
	}
	
	if(!component->IsCounter()) return false;

	CounterComponent* newComponent = (CounterComponent*) component;
	if(!previousComponent->content->IsEqual(newComponent->content)) return false;
	if(previousComponent->mode == CounterComponent::Possessive || newComponent->mode == CounterComponent::Possessive) return false;
	// Mix of bounded/unbounded and minimal/maximal modes
	//
	// Unbounded/Unbounded        Bounded/Unbounded             Unbounded/Bounded           Bounded/Bounded
	// a{3,}a{10,}   -> a{13,}    a{3,5}a{10,}   -> a{13,}      a{10,}a{3,5}   -> a{13,}	a{3,5}a{10,12}   -> a{13,17}
	// a{3,}a{10,}?  -> a{13,}    a{3,5}a{10,}?  -> XXX		    a{10,}a{3,5)?  -> a{13,}    a{3,5}a{10,12}?  -> XXX
	// a{3,}?a{10,}  -> a{13,}    a{3,5}?a{10,}  -> XXX         a{10,}?a{3,5}  -> XXX       a{3,5}?a{10,12}  -> XXX
	// a{3,}?a{10,}? -> a{13,}?   a{3,5}?a{10,}? -> a{13,)?     a{10,}?a{3,5}? -> a{10,}?   a{3,5)?a{10,12}? -> a{13,17}?
	
	// Note that for the XXX cases we can still modify the minimum/maximum to generate better code.
	// eg.
	//		a{3,5}a{10,}? -> a{13,15}a{0,}? -> a{12,15}a{1,}?
	//      a{10,}?a{3,5} -> a{13,}?a{0,2} -> a{12,}?a{1,3}
	if(previousComponent->maximum == TypeData<uint32_t>::Maximum())
	{
		if(newComponent->maximum == TypeData<uint32_t>::Maximum())
		{
			// Unbounded/Unbounded.
			previousComponent->minimum += newComponent->minimum;
			if(newComponent->mode == CounterComponent::Maximal) previousComponent->mode = CounterComponent::Maximal;
			return true;
		}
		else
		{
			// Unbounded/Bounded
			if(previousComponent->mode == CounterComponent::Minimal && newComponent->mode == CounterComponent::Maximal)
			{
				previousComponent->minimum += newComponent->minimum;
				newComponent->maximum -= newComponent->minimum;
				newComponent->minimum = 0;
				if(previousComponent->minimum > 1)
				{
					previousComponent->minimum--;
					newComponent->minimum++;
					newComponent->maximum++;
				}
				return false;
			}
			else
			{
				previousComponent->minimum += newComponent->minimum;
				return true;
			}
		}
	}
	else
	{
		if(newComponent->maximum == TypeData<uint32_t>::Maximum())
		{
			// Bounded/Unbounded
			if(previousComponent->mode == newComponent->mode)
			{
				previousComponent->minimum += newComponent->minimum;
				previousComponent->maximum = TypeData<uint32_t>::Maximum();
				return true;
			}
			else
			{
				previousComponent->minimum += newComponent->minimum;
				previousComponent->maximum += newComponent->minimum;
				newComponent->minimum = 0;
				if(previousComponent->minimum > 1)
				{
					previousComponent->minimum--;
					previousComponent->maximum--;
					newComponent->minimum++;
				}
				return false;
			}
		}
		else
		{
			// Bounded/Bounded
			if(previousComponent->mode == newComponent->mode)
			{
				previousComponent->minimum += newComponent->minimum;
				previousComponent->maximum += newComponent->maximum;
				return true;
			}
			else
			{
				previousComponent->minimum += newComponent->minimum;
				previousComponent->maximum += newComponent->minimum;
				newComponent->maximum -= newComponent->minimum;
				newComponent->minimum = 0;
				if(previousComponent->minimum > 1)
				{
					previousComponent->minimum--;
					previousComponent->maximum--;
					newComponent->minimum++;
					newComponent->maximum++;
				}
				return false;
			}
		}
	}
}

IComponent* Compiler::CompileCapture(int options)
{
	uint32_t index = captureGroups.GetCount();
	captureGroups.Append(nullptr);
	if(index == 0)
	{
		outerCapture = new CaptureComponent(index, nullptr);
		outerCapture->content = CompilePattern(options);
		captureGroups[0] = outerCapture;
		return outerCapture;
	}
	else
	{
		IComponent* body = CompilePattern(options);
		CaptureComponent* result = new CaptureComponent(index, body);
		captureGroups[index] = result;
		return result;
	}
}

IComponent* Compiler::CompileAtom(int options)
{
	// This is only valid until tokenizer->ProcessTokens() is called
	const Token& currentToken = tokenizer->PeekCurrentToken();

	IComponent* component = nullptr;
	switch(currentToken.type)
	{
	case TokenType::AnyByte:
		component = new CharacterRangeListComponent(false, {0, 255});
		tokenizer->ProcessTokens();
		break;
	
	case TokenType::BackReference:
		{
			JPATTERN_VERIFY((size_t) currentToken.i < captureGroups.GetCount(), InvalidBackReference, tokenizer->GetExceptionData());
			usesBacktrackingComponents = true;
			CaptureComponent* captureComponent = captureGroups[currentToken.i];
			JPATTERN_VERIFY(captureComponent != nullptr, InvalidBackReference, tokenizer->GetExceptionData());
			captureComponent->isBackReferenceTarget = true;
			component = new BackReferenceComponent(captureComponent);
			tokenizer->ProcessTokens();
		}
		break;
			
	case TokenType::Character:
		if(options & Pattern::IGNORE_CASE)
		{
			if((options & Pattern::UTF8) && (options & Pattern::UNICODE_CASE))
			{
				CharacterRangeList list;
				list.Append(currentToken.c);
				CharacterRangeListComponent* characterRangeListComponent = new CharacterRangeListComponent(true, list.CreateUnicodeCaseInsensitive());
				component = characterRangeListComponent;
			}
			else
			{
				Character upper = currentToken.c.ToUpper();
				Character lower = currentToken.c.ToLower();
				
				CharacterRangeListComponent* characterRangeListComponent = new CharacterRangeListComponent(false);
				component = characterRangeListComponent;
				if(upper == lower)
				{
					characterRangeListComponent->characterRangeList.Append(currentToken.c);
				}
				else
				{
					characterRangeListComponent->characterRangeList.Append(upper);
					characterRangeListComponent->characterRangeList.Append(lower);
				}
			}
		}
		else
		{
			CharacterRangeListComponent* characterRangeListComponent = new CharacterRangeListComponent(false);
			characterRangeListComponent->characterRangeList.Append(currentToken.c);
			component = characterRangeListComponent;
		}
		tokenizer->ProcessTokens();
		break;
			
	case TokenType::ResetCapture:
		tokenizer->ProcessTokens();
		return new ResetCaptureComponent(outerCapture);
		
	case TokenType::StartOfInput:
		tokenizer->ProcessTokens();
		return new AssertComponent(AssertType::StartOfInput);
		
	case TokenType::EndOfInput:
		tokenizer->ProcessTokens();
		return new AssertComponent(AssertType::EndOfInput);
		
	case TokenType::StartOfSearch:
		tokenizer->ProcessTokens();
		return new AssertComponent(AssertType::StartOfSearch);
		
	case TokenType::StartOfLine:
		tokenizer->ProcessTokens();
		return new AssertComponent((options & Pattern::MULTILINE) ? AssertType::StartOfLine : AssertType::StartOfInput);
			
	case TokenType::EndOfLine:
		tokenizer->ProcessTokens();
		return new AssertComponent((options & Pattern::MULTILINE) ? AssertType::EndOfLine : AssertType::EndOfInput);
		
	case TokenType::Accept:
		tokenizer->ProcessTokens();
		return new TerminalComponent(true);
			
	case TokenType::Fail:
		tokenizer->ProcessTokens();
		return new TerminalComponent(false);
		
	case TokenType::WhitespaceCharacter:
		component = new CharacterRangeListComponent((options&Pattern::UTF8) != 0, CharacterRangeList::WHITESPACE_CHARACTERS);
		tokenizer->ProcessTokens();
		break;
			
	case TokenType::NotWhitespaceCharacter:
		component = new CharacterRangeListComponent((options&Pattern::UTF8) != 0, CharacterRangeList::WHITESPACE_CHARACTERS.CreateComplement());
		tokenizer->ProcessTokens();
		break;
			
	case TokenType::WordBoundary:
		tokenizer->ProcessTokens();
		return new AssertComponent(AssertType::WordBoundary);
		
	case TokenType::NotWordBoundary:
		tokenizer->ProcessTokens();
		return new AssertComponent(AssertType::NotWordBoundary);
		
	case TokenType::Wildcard:
		if(options & (Pattern::DOTALL | Pattern::GLOB_SYNTAX))
		{
			if(options & Pattern::UTF8)
			{
				component = new CharacterRangeListComponent(true, {0, Character::Maximum()});
			}
			else
			{				
				component = new CharacterRangeListComponent(false, {0, 255});
			}
		}
		else
		{
			CharacterRangeList rangeList;
			rangeList.Add(0, '\n'-1);
			rangeList.Add('\n'+1, Character::Maximum());
			component = new CharacterRangeListComponent((options&Pattern::UTF8) != 0, rangeList);
		}
		tokenizer->ProcessTokens();
		break;

	case TokenType::Range:
		if(options & Pattern::IGNORE_CASE)
		{
			if((options & Pattern::UTF8) && (options & Pattern::UNICODE_CASE))
			{
				component = new CharacterRangeListComponent(true, currentToken.rangeList.CreateUnicodeCaseInsensitive());
			}
			else
			{
				component = new CharacterRangeListComponent(false, currentToken.rangeList.CreateCaseInsensitive());
			}
		}
		else
		{
			component = new CharacterRangeListComponent((options&Pattern::UTF8) != 0, currentToken.rangeList);
		}
		tokenizer->ProcessTokens();
		break;

	case TokenType::RecurseRelative:
		{
			usesBacktrackingComponents = true;
			RecurseComponent* recurseComponent = new RecurseComponent(int32_t(currentToken.i + captureGroups.GetCount()));
			recurseComponentList.Append(recurseComponent);
			component = recurseComponent;
			tokenizer->ProcessTokens();
		}
		break;

	case TokenType::Recurse:
		{
			usesBacktrackingComponents = true;
			RecurseComponent* recurseComponent = new RecurseComponent(currentToken.i);
			recurseComponentList.Append(recurseComponent);
			component = recurseComponent;
			tokenizer->ProcessTokens();
		}
		break;
		
	case TokenType::NotRange:
		if(options & Pattern::IGNORE_CASE)
		{
			if((options & Pattern::UTF8) && (options & Pattern::UNICODE_CASE))
			{
				component = new CharacterRangeListComponent(true, currentToken.rangeList.CreateUnicodeCaseInsensitive().CreateComplement());
			}
			else
			{
				component = new CharacterRangeListComponent(false, currentToken.rangeList.CreateCaseInsensitive().CreateComplement());
			}
		}
		else
		{
			component = new CharacterRangeListComponent((options&Pattern::UTF8) != 0, currentToken.rangeList.CreateComplement());
		}
		tokenizer->ProcessTokens();
		break;
		
	case TokenType::WordCharacter:
		component = new CharacterRangeListComponent(false, CharacterRangeList::WORD_CHARACTERS);
		tokenizer->ProcessTokens();
		break;

	case TokenType::NotWordCharacter:
		component = new CharacterRangeListComponent((options&Pattern::UTF8) != 0, CharacterRangeList::NOT_WORD_CHARACTERS);
		tokenizer->ProcessTokens();
		break;

	case TokenType::CaptureStart:
		{
			tokenizer->ProcessTokens();
			component = CompileCapture(options);
			JPATTERN_VERIFY(tokenizer->PeekCurrentTokenType() == TokenType::CloseBracket, ExpectedCloseGroup, tokenizer->GetExceptionData());
			tokenizer->ProcessTokens();
			break;
		}
		
	case TokenType::ClusterStart:
		{
			int newOptions = (options & ~currentToken.optionMask) | currentToken.optionSet;
			tokenizer->ProcessTokens();
			component = CompilePattern(newOptions);
			JPATTERN_VERIFY(tokenizer->PeekCurrentTokenType() == TokenType::CloseBracket, ExpectedCloseGroup, tokenizer->GetExceptionData());
			tokenizer->ProcessTokens();
			break;
		}
			
	case TokenType::Conditional:
		{
			static constexpr EnumSet<TokenType, uint64_t> VALID_CONDITIONALS
			{
				TokenType::NegativeLookBehind,
				TokenType::NegativeLookAhead,
				TokenType::PositiveLookBehind,
				TokenType::PositiveLookAhead,
			};

			tokenizer->ProcessTokens();
			TokenType conditionType = tokenizer->PeekCurrentTokenType();
			JPATTERN_VERIFY(VALID_CONDITIONALS.Contains(conditionType), MalformedConditional, tokenizer->GetExceptionData());
			tokenizer->ProcessTokens();
			int localOptions = options;
			AutoPointer<IComponent> condition = CompileGroup(localOptions);
			JPATTERN_VERIFY(tokenizer->PeekCurrentTokenType() == TokenType::CloseBracket, MalformedConditional, tokenizer->GetExceptionData());
			tokenizer->ProcessTokens();
			localOptions = options;
			AutoPointer<IComponent> trueComponent = CompileGroup(localOptions);
			JPATTERN_VERIFY(tokenizer->PeekCurrentTokenType() == TokenType::Alternate, MalformedConditional, tokenizer->GetExceptionData());
			tokenizer->ProcessTokens();
			localOptions = options;
			AutoPointer<IComponent> falseComponent = CompileGroup(localOptions);
			JPATTERN_VERIFY(tokenizer->PeekCurrentTokenType() == TokenType::CloseBracket, MalformedConditional, tokenizer->GetExceptionData());
			tokenizer->ProcessTokens();
			component = new ConditionalComponent(conditionType, condition.RelinquishPointer(), falseComponent.RelinquishPointer(), trueComponent.RelinquishPointer());
			break;
		}

	case TokenType::PositiveLookAhead:
		{
			int newOptions = (options & ~currentToken.optionMask) | currentToken.optionSet;
			tokenizer->ProcessTokens();
			AutoPointer<IComponent> body = CompilePattern(newOptions);
			JPATTERN_VERIFY(tokenizer->PeekCurrentTokenType() == TokenType::CloseBracket, ExpectedCloseGroup, tokenizer->GetExceptionData());
			tokenizer->ProcessTokens();
			usesBacktrackingComponents = true;
			return new LookAheadComponent(body.RelinquishPointer(), true);
		}

	case TokenType::NegativeLookAhead:
		{
			int newOptions = (options & ~currentToken.optionMask) | currentToken.optionSet;
			tokenizer->ProcessTokens();
			AutoPointer<IComponent> body = CompilePattern(newOptions);
			JPATTERN_VERIFY(tokenizer->PeekCurrentTokenType() == TokenType::CloseBracket, ExpectedCloseGroup, tokenizer->GetExceptionData());
			tokenizer->ProcessTokens();
			usesBacktrackingComponents = true;
			return new LookAheadComponent(body.RelinquishPointer(), false);
		}
			
	case TokenType::PositiveLookBehind:
		{
			int newOptions = (options & ~currentToken.optionMask) | currentToken.optionSet;
			tokenizer->ProcessTokens();
			AutoPointer<IComponent> body = CompilePattern(newOptions);
			JPATTERN_VERIFY(tokenizer->PeekCurrentTokenType() == TokenType::CloseBracket, ExpectedCloseGroup, tokenizer->GetExceptionData());
			tokenizer->ProcessTokens();
			JPATTERN_VERIFY(body->IsFixedLength(), LookBehindNotConstantByteLength, tokenizer->GetExceptionData());
			usesBacktrackingComponents = true;
			return new LookBehindComponent(body.RelinquishPointer(), true);
		}
			
	case TokenType::NegativeLookBehind:
		{
			int newOptions = (options & ~currentToken.optionMask) | currentToken.optionSet;
			tokenizer->ProcessTokens();
			AutoPointer<IComponent> body = CompilePattern(newOptions);
			JPATTERN_VERIFY(tokenizer->PeekCurrentTokenType() == TokenType::CloseBracket, ExpectedCloseGroup, tokenizer->GetExceptionData());
			tokenizer->ProcessTokens();
			JPATTERN_VERIFY(body->IsFixedLength(), LookBehindNotConstantByteLength, tokenizer->GetExceptionData());
			usesBacktrackingComponents = true;
			return new LookBehindComponent(body.RelinquishPointer(), false);
		}
			
	case TokenType::AtomicGroup:
		{
			int newOptions = (options & ~currentToken.optionMask) | currentToken.optionSet;
			tokenizer->ProcessTokens();
			AutoPointer<IComponent> body = CompilePattern(newOptions);
			JPATTERN_VERIFY(tokenizer->PeekCurrentTokenType() == TokenType::CloseBracket, ExpectedCloseGroup, tokenizer->GetExceptionData());
			tokenizer->ProcessTokens();
			usesBacktrackingComponents = true;
			component = new CounterComponent(1, 1, CounterComponent::Possessive, body.RelinquishPointer());
			break;
		}
			
	default:
		JPATTERN_ERROR(InternalError, tokenizer->GetExceptionData());
	}
	
	if(component->IsCounter())
	{
		IComponent* result = CompileAtomQuantifier(component, options);
		if(result == component) return result;
		
		JASSERT(result->IsCounter());
		
		// We have a nested counter!
		// (a*)*  -> a*         (a*){3}  -> a*           (a?)*  -> a*
		// (a*?)* -> a*?        (a*?){3} -> a*?          (a??)* -> a*?
		// (a+)*  -> a+         (a+){3}  -> a{3,}        (a?)*? -> a*?
		// (a+?)* -> a+?        (a+?){3} -> a{3,}?       (a??)*? -> a*?

		
		CounterComponent* inner = (CounterComponent*) component;
		if(inner->minimum > 1) return result;
		
		CounterComponent* outer = (CounterComponent*) result;
		
		// Determine minimal or maximal
		if(outer->minimum != outer->maximum)
		{
			if(inner->maximum <= 1)
			{
				if(outer->mode == CounterComponent::Minimal) inner->mode = CounterComponent::Minimal;
			}
			else if(inner->mode != outer->mode)
			{
				return result;
			}
		}
		
		inner->minimum = inner->minimum * outer->minimum;
		inner->maximum = inner->maximum == TypeData<uint32_t>::Maximum() || outer->maximum == TypeData<uint32_t>::Maximum() ? TypeData<uint32_t>::Maximum() : inner->maximum * outer->maximum;

		outer->content = nullptr;
		delete result;
		return inner;
	}
	else
	{
		return CompileAtomQuantifier(component, options);
	}
}

IComponent* Compiler::CompileAtomQuantifier(IComponent* atom, int options)
{
	switch(tokenizer->PeekCurrentTokenType())
	{
	case TokenType::OneOrMoreMaximal:
		tokenizer->ProcessTokens();
		return new CounterComponent(1, TypeData<uint32_t>::Maximum(), (options & Pattern::UNGREEDY) ? CounterComponent::Minimal : CounterComponent::Maximal, atom);

	case TokenType::OneOrMoreMinimal:
		tokenizer->ProcessTokens();
		return new CounterComponent(1, TypeData<uint32_t>::Maximum(), (options & Pattern::UNGREEDY) ? CounterComponent::Maximal : CounterComponent::Minimal, atom);
			
	case TokenType::OneOrMorePossessive:
		tokenizer->ProcessTokens();
		usesBacktrackingComponents = true;
		return new CounterComponent(1, TypeData<uint32_t>::Maximum(), CounterComponent::Possessive, atom);

	case TokenType::ZeroOrMoreMaximal:
		tokenizer->ProcessTokens();
		return new CounterComponent(0, TypeData<uint32_t>::Maximum(), (options & Pattern::UNGREEDY) ? CounterComponent::Minimal : CounterComponent::Maximal, atom);

	case TokenType::ZeroOrMoreMinimal:
		tokenizer->ProcessTokens();
		return new CounterComponent(0, TypeData<uint32_t>::Maximum(), (options & Pattern::UNGREEDY) ? CounterComponent::Maximal : CounterComponent::Minimal, atom);
		
	case TokenType::ZeroOrMorePossessive:
		tokenizer->ProcessTokens();
		usesBacktrackingComponents = true;
		return new CounterComponent(0, TypeData<uint32_t>::Maximum(), CounterComponent::Possessive, atom);

	case TokenType::ZeroOrOneMaximal:
		tokenizer->ProcessTokens();
		return new CounterComponent(0, 1, (options & Pattern::UNGREEDY) ? CounterComponent::Minimal : CounterComponent::Maximal, atom);
		
	case TokenType::ZeroOrOneMinimal:
		tokenizer->ProcessTokens();
		return new CounterComponent(0, 1, (options & Pattern::UNGREEDY) ? CounterComponent::Maximal : CounterComponent::Minimal, atom);
		
	case TokenType::ZeroOrOnePossessive:
		tokenizer->ProcessTokens();
		usesBacktrackingComponents = true;
		return new CounterComponent(0, 1, CounterComponent::Possessive, atom);
		
	case TokenType::CounterStart:
		{
			tokenizer->ProcessTokens();
			
			// Cases can be:
			//
			// {x}
			// {min,max}
			// {min,max}?
			// {,max}
			// {,max}?
			// {min,}
			// {min,}?
			
			if(tokenizer->PeekCurrentTokenType() == TokenType::CounterValue)
			{
				int min = tokenizer->PeekCurrentToken().i;
				tokenizer->ProcessTokens();
				
				switch(tokenizer->PeekCurrentTokenType())
				{
				case TokenType::CounterEnd:
				case TokenType::CounterEndMinimal:
				case TokenType::CounterEndPossessive:
					// {x}
					tokenizer->ProcessTokens();
					if(min == 0)
					{
						delete atom;
						return new EmptyComponent;
					}
					else if(min == 1)
					{
						return atom;
					}
					else return new CounterComponent(min, min, CounterComponent::Minimal, atom);
						
				case TokenType::CounterSeparator:
					tokenizer->ProcessTokens();
					switch(tokenizer->PeekCurrentTokenType())
					{
					case TokenType::CounterValue:
						{
							int max = tokenizer->PeekCurrentToken().i;
							JPATTERN_VERIFY(min <= max, MinimumCountExceedsMaximumCount, tokenizer->GetExceptionData());
							JPATTERN_VERIFY(max <= MAXIMUM_REPETITION_COUNT, MaximumRepetitionCountExceeded, tokenizer->GetExceptionData());
							tokenizer->ProcessTokens();
							
							switch(tokenizer->PeekCurrentTokenType())
							{
							case TokenType::CounterEnd:
								// {min,max}
								tokenizer->ProcessTokens();
								return new CounterComponent(min, max, (options & Pattern::UNGREEDY) ? CounterComponent::Minimal : CounterComponent::Maximal, atom);
								
							case TokenType::CounterEndMinimal:
								// {min,max}?
								tokenizer->ProcessTokens();
								return new CounterComponent(min, max, (options & Pattern::UNGREEDY) ? CounterComponent::Maximal : CounterComponent::Minimal, atom);
								
							case TokenType::CounterEndPossessive:
								// {min,max}+
								tokenizer->ProcessTokens();
								usesBacktrackingComponents = true;
								return new CounterComponent(min, max, CounterComponent::Possessive, atom);
								
							default:
								JPATTERN_ERROR(UnableToParseRepetition, tokenizer->GetExceptionData());
							}
						}
						
					case TokenType::CounterEnd:
						// {min,}
						tokenizer->ProcessTokens();
						return new CounterComponent(min, TypeData<uint32_t>::Maximum(), (options & Pattern::UNGREEDY) ? CounterComponent::Minimal : CounterComponent::Maximal, atom);
						
					case TokenType::CounterEndMinimal:
						// {min,}?
						tokenizer->ProcessTokens();
						return new CounterComponent(min, TypeData<uint32_t>::Maximum(), (options & Pattern::UNGREEDY) ? CounterComponent::Maximal : CounterComponent::Minimal, atom);
						
					case TokenType::CounterEndPossessive:
						// {min,}+
						tokenizer->ProcessTokens();
						usesBacktrackingComponents = true;
						return new CounterComponent(min, TypeData<uint32_t>::Maximum(), CounterComponent::Possessive, atom);
						
					default:
						JPATTERN_ERROR(UnableToParseRepetition, tokenizer->GetExceptionData());
					}
						
				default:
					JPATTERN_ERROR(UnableToParseRepetition, tokenizer->GetExceptionData());
				}
			}
			else
			{
				JPATTERN_VERIFY(tokenizer->PeekCurrentTokenType() == TokenType::CounterSeparator, UnableToParseRepetition, tokenizer->GetExceptionData());
				tokenizer->ProcessTokens();
				
				JPATTERN_VERIFY(tokenizer->PeekCurrentTokenType() == TokenType::CounterValue, UnableToParseRepetition, tokenizer->GetExceptionData());
				int max = tokenizer->PeekCurrentToken().i;
				JPATTERN_VERIFY(max <= MAXIMUM_REPETITION_COUNT, MaximumRepetitionCountExceeded, tokenizer->GetExceptionData());
				tokenizer->ProcessTokens();
				
				switch(tokenizer->PeekCurrentTokenType())
				{
				case TokenType::CounterEnd:
					// {,max}
					tokenizer->ProcessTokens();
					return new CounterComponent(0, max, (options & Pattern::UNGREEDY) ? CounterComponent::Minimal : CounterComponent::Maximal, atom);
					
				case TokenType::CounterEndMinimal:
					// {,max}?
					tokenizer->ProcessTokens();
					return new CounterComponent(0, max, (options & Pattern::UNGREEDY) ? CounterComponent::Maximal : CounterComponent::Minimal, atom);
					
				case TokenType::CounterEndPossessive:
					// {,max}+
					tokenizer->ProcessTokens();
					usesBacktrackingComponents = true;
					return new CounterComponent(0, max, CounterComponent::Possessive, atom);
					
				default:
					JPATTERN_ERROR(UnableToParseRepetition, tokenizer->GetExceptionData());
				}
			}
		}

	default:
		return atom;
	}
}

//============================================================================
