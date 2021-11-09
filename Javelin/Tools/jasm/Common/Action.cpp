//============================================================================

#include "Javelin/Tools/jasm/Common/Action.h"

#include "Javelin/Assembler/BitUtility.h"
#include "Javelin/Assembler/JitLabelId.h"
#include "Javelin/Tools/jasm/AssemblerException.h"
#include "Javelin/Tools/jasm/CommandLine.h"
#include "Javelin/Tools/jasm/Log.h"

#include <assert.h>
#include <inttypes.h>
#include <memory.h>

//============================================================================

using namespace Javelin::Assembler;
using namespace Javelin::Assembler::Common;

//============================================================================

const AlwaysAlternateActionCondition AlwaysAlternateActionCondition::instance;
const NeverAlternateActionCondition NeverAlternateActionCondition::instance;

//============================================================================

bool AlternateActionCondition::Equals(const AlternateActionCondition *other) const
{
	return this == other;
}

void AlwaysAlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
	// Do nothing.
}

AndAlternateActionCondition::~AndAlternateActionCondition()
{
	for(const AlternateActionCondition *condition : conditionList)
	{
		condition->Release();
	}
}

bool AndAlternateActionCondition::Equals(const AlternateActionCondition *other) const
{
	if(typeid(*this) != typeid(*other)) return false;
	AndAlternateActionCondition *o = (AndAlternateActionCondition*) other;
	if(conditionList.size() != o->conditionList.size()) return false;
	for(size_t i = 0; i < conditionList.size(); ++i)
	{
		if(!conditionList[i]->Equals(o->conditionList[i])) return false;
	}
	return true;
}

AlternateActionCondition::Result AndAlternateActionCondition::IsValid(ActionContext &context, const Action *action) const
{
	bool isAlways = true;
	for(const AlternateActionCondition *condition : conditionList)
	{
		switch(condition->IsValid(context, action))
		{
		case Result::Never:
			return Result::Never;
		case Result::Maybe:
			isAlways = false;
			break;
		case Result::Always:
			break;
		}
	}
	return isAlways ? Result::Always : Result::Maybe;
}

std::string AndAlternateActionCondition::GetDescription() const
{
	std::string result = "AND(";
	bool first = true;
	for(const AlternateActionCondition *condition : conditionList)
	{
		if (first) first = false;
		else result.push_back(',');
		result.append(condition->GetDescription());
	}
	result.push_back(')');
	return result;
}

void AndAlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
	// Each WriteByteCode wraps the previous condition, so write conditions in
	// reverse order so that they are processed in the same order that they're
	// written.
	for(auto it = conditionList.crbegin(); it != conditionList.crend(); ++it)
	{
		(*it)->WriteByteCode(result, context, action);
	}
}

void NeverAlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action) const
{
	assert(!"NeverAlternateAction should never be serialized!");
}

void ImmediateAlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action, int opcode) const
{
	// Prefix with condition code
	std::vector<uint8_t> conditionCode;
	conditionCode.push_back((int) opcode);
	Action::WriteExpressionOffset(conditionCode, context, expressionIndex);
	
	if(result.size() >= 256) throw AssemblerException("Encoding Error: Alternate jump offset should be less than 256 bytes");
	conditionCode.push_back(result.size());
	result.insert(result.begin(), conditionCode.begin(), conditionCode.end());
}

void ImmediateAlternateActionCondition::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context, const Action &action, int opcode, int bitCount) const
{
    // Prefix with condition code
    std::vector<uint8_t> conditionCode;
    conditionCode.push_back((int) opcode);
    Action::WriteExpressionOffset(conditionCode, context, expressionIndex);
    conditionCode.push_back((int) bitCount);

    if(result.size() >= 256) throw AssemblerException("Encoding Error: Alternate jump offset should be less than 256 bytes");
    conditionCode.push_back(result.size());
    result.insert(result.begin(), conditionCode.begin(), conditionCode.end());
}

std::string ZeroAlternateActionCondition::GetDescription() const
{
	char buffer[24];
	sprintf(buffer, "Zero{%d}", expressionIndex);
	return buffer;
}

bool ImmediateAlternateActionCondition::Equals(const AlternateActionCondition *other) const
{
	if(typeid(*this) != typeid(*other)) return false;
	ImmediateAlternateActionCondition *o = (ImmediateAlternateActionCondition*) other;
	return expressionIndex == o->expressionIndex;
}

//============================================================================

std::vector<uint8_t>& Action::GetLiteralData()
{
    throw AssemblerException("Invalid GetLiteralData() call");
}

void Action::UpdateActionOffset(bool forwards, ActionOffset &offset) const
{
    ssize_t minimumLength = GetMinimumLength();
    ssize_t maximumLength = GetMaximumLength();
    if(!forwards)
    {
        minimumLength = -minimumLength;
        maximumLength = -maximumLength;
    }
    if(minimumLength != maximumLength)
    {
        offset.blockIndex++;
        offset.offsetIntoBlock = 0;
        offset.alignment = 0;
    }
    else
    {
        offset.offsetIntoBlock += minimumLength;
    }
    offset.totalMinimumOffset += minimumLength;
    offset.totalMaximumOffset += maximumLength;
}

// Returns true if any changes have occurred.
bool Action::ResolveRelativeAddresses(ActionContext &context)
{
	actionOffset = context.offset;
    UpdateActionOffset(context.forwards, context.offset);
	return false;
}

void Action::WriteSigned16(std::vector<uint8_t> &result, int16_t value)
{
	uint8_t data[sizeof(value)];
	memcpy(data, &value, sizeof(value));
	result.insert(result.end(), data, data+sizeof(value));
}

void Action::WriteUnsigned32(std::vector<uint8_t> &result, uint32_t value)
{
	uint8_t data[sizeof(value)];
	memcpy(data, &value, sizeof(value));
	result.insert(result.end(), data, data+sizeof(value));
}

void Action::WriteExpressionOffset(std::vector<uint8_t> &result,
								   const ActionWriteContext &context,
								   int expressionIndex,
								   int additionalOffset)
{
	assert(0 < expressionIndex && expressionIndex <= context.expressionInfo.size());
	int offset = context.expressionInfo[expressionIndex-1].offset
					+ context.appendAssemblerReferenceSize
					+ additionalOffset;
	assert(BitUtility::IsValidUnsignedImmediate(offset, 16));
	result.push_back(offset);
	result.push_back(offset >> 8);
}

//============================================================================

SetAssemblerVariableNameAction::SetAssemblerVariableNameAction(const std::string &aVariableName)
: variableName(aVariableName) { }

void SetAssemblerVariableNameAction::Dump() const
{
	printf("\nassemblerVariableName:%s\n", variableName.c_str());
}

LiteralAction::LiteralAction(const std::vector<uint8_t> &aBytes) : bytes(aBytes)
{
}

LiteralAction::~LiteralAction()
{	
}

void LiteralAction::Dump() const
{
	printf(" \"");
	bool first = true;
	for(uint8_t value : bytes)
	{
		printf(first ? "%02x" : " %02x", value);
		first = false;
	}
	printf("\"");
}

void LiteralAction::AppendBytes(const std::vector<uint8_t>& extraBytes)
{
	bytes.insert(bytes.end(), extraBytes.begin(), extraBytes.end());
}

void LiteralAction::AppendBytes(const uint8_t *data, size_t length)
{
	bytes.insert(bytes.end(), data, data+length);
}

void LiteralAction::AppendToLiteral(LiteralAction *literal) const
{
	literal->AppendBytes(bytes);
}

bool LiteralAction::Simplify(ListAction *parent, size_t index)
{
	if(!index || !parent) return false;

	Action *previousAction = parent->GetActionAtIndex(index-1);
	if(!previousAction->IsLiteralAction()) return false;

	LiteralAction *literalAction = (LiteralAction *) previousAction;
	literalAction->AppendBytes(bytes);
	
	parent->RemoveActionAtIndex(index);
	delete this;
	return true;
}

//============================================================================

void AlignedAction::Dump() const
{
	printf(", aligned(%d)", alignment);
}

bool AlignedAction::ResolveRelativeAddresses(ActionContext &context)
{
	if(context.forwards == false) return false;
	
	context.offset.alignment = alignment;
	context.offset.blockIndex++;
	context.offset.offsetIntoBlock = 0;
	return false;
}

void AlignedAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
	// No byte code emitted.
}

//============================================================================

void AlignAction::Dump() const
{
	printf(", align(%d)", alignment);
}

size_t AlignAction::GetMinimumLength() const
{
	if(isFixed) return fixedLength;
	return 0;
}

size_t AlignAction::GetMaximumLength() const
{
	if(isFixed) return fixedLength;
	return alignment-1;
}

bool AlignAction::ResolveRelativeAddresses(ActionContext &context)
{
	if(context.forwards && context.offset.alignment >= alignment)
	{
		isFixed = true;
		fixedLength = (int) (-context.offset.offsetIntoBlock & (alignment-1));
		return true;
	}
	else
	{
		return Action::ResolveRelativeAddresses(context);
	}
}

//============================================================================

void UnalignAction::Dump() const
{
	printf(", unalign(%d)", alignment);
}

size_t UnalignAction::GetMinimumLength() const
{
	if(isFixed) return fixedLength;
	return 0;
}

size_t UnalignAction::GetMaximumLength() const
{
	if(isFixed) return fixedLength;
	return 4;
}

bool UnalignAction::ResolveRelativeAddresses(ActionContext &context)
{
	if(context.forwards == false) return false;
	
	if(context.offset.alignment >= alignment)
	{
		isFixed = true;
		fixedLength = (context.offset.offsetIntoBlock & (alignment-1)) ? 0 : 1;
		return true;
	}
	else
	{
		Action::ResolveRelativeAddresses(context);
	}
	return false;
}

//============================================================================

void ExpressionAction::Dump() const
{
    printf(", #%d={%d}", numberOfBytes, expressionIndex);
}

//============================================================================

bool LabelAction::Simplify(ListAction *parent, size_t index)
{
	if(global) return false;
	if(hasReference) return false;
	
	if(Log::IsVerboseEnabled())
	{
		printf("\n");
		Log::Verbose("Removing locallabel (0 references):");
		Dump();
		printf("\n");
	}
	
	// Local variable that has no references. Remove it.
	parent->RemoveActionAtIndex(index);
	delete this;
	return true;
}

void NamedLabelAction::Dump() const
{
	printf("\nLabel(%s%s)", global ? "*" : "", value.c_str());
}

bool NamedLabelAction::ResolveRelativeAddresses(ActionContext &context)
{
	if(context.forwards) hasReference = global;
	
	if(!global)
	{
		if(context.namedReferenceSet.find(value) != context.namedReferenceSet.end())
		{
			hasReference = true;
		}
	}
	context.namedLabels[value] = context.offset;
	return Inherited::ResolveRelativeAddresses(context);
}

void NumericLabelAction::Dump() const
{
	printf("\nLabel(%s%" PRId64 ")", global ? "*" : "", value);
}

bool NumericLabelAction::ResolveRelativeAddresses(ActionContext &context)
{
	// First pass reset code.
	if(context.forwards) hasReference = global;
	
	if(!global)
	{
		if(context.numericReferenceSet.find(value) != context.numericReferenceSet.end())
		{
			hasReference = true;
		}
	}
	
	context.numericLabels[global ? value ^ -1 : value] = context.offset;
	return Inherited::ResolveRelativeAddresses(context);
}

void ExpressionLabelAction::Dump() const
{
	printf("\nLabel(*{%d})", expressionIndex);
}

bool ExpressionLabelAction::ResolveRelativeAddresses(ActionContext &context)
{
	context.expressionLabels[expressionIndex] = context.offset;
	return Inherited::ResolveRelativeAddresses(context);
}

//============================================================================

AlternateAction::AlternateAction()
{
}

AlternateAction::~AlternateAction()
{
	for(const Alternate &alternate : alternateList)
	{
		alternate.condition->Release();
		delete alternate.action;
	}
}

void AlternateAction::Dump() const
{
	printf(" (");

	bool isFirst = true;
	for(const Alternate &alternate : alternateList)
	{
		printf(isFirst ? "[%s]" : " | [%s]", alternate.condition->GetDescription().c_str());
		isFirst = false;
		alternate.action->Dump();
	}

	printf(")");
}

size_t AlternateAction::GetMinimumLength() const
{
	if(alternateList.size() == 0) return 0;
	
	size_t minimumLength = (size_t) -1;
	for(const Alternate &alternate : alternateList)
	{
		size_t length = alternate.action->GetMinimumLength();
		if(minimumLength > length) minimumLength = length;
	}
	return minimumLength;
}

size_t AlternateAction::GetMaximumLength() const
{
	size_t maximumLength = 0;
	for(const Alternate &alternate : alternateList)
	{
		size_t length = alternate.action->GetMaximumLength();
		if(maximumLength < length) maximumLength = length;
	}
	return maximumLength;
}

Action *AlternateAction::GetSingleAlternateAndClearAlternateList()
{
	assert(alternateList.size() == 1);
	Action *action = alternateList[0].action;
	alternateList[0].condition->Release();
	alternateList.clear();
	return action;
}

bool AlternateAction::IsLiteral() const
{
	return alternateList.size() == 1
			&& alternateList[0].action->IsLiteral();
}

void AlternateAction::AppendToLiteral(LiteralAction *literal) const
{
	for(const Alternate& alternate : alternateList)
	{
		alternate.action->AppendToLiteral(literal);
	}
}

void AlternateAction::Add(Action *action, const AlternateActionCondition *condition)
{
	for(const Alternate& alternate : alternateList)
	{
		if(alternate.condition->Equals(condition))
		{
			delete action;
			condition->Release();
			return;
		}
	}
	
	const Alternate alternate =
	{
		.condition = condition,
		.action = action,
	};
	alternateList.push_back(alternate);
}

bool AlternateAction::ResolveRelativeAddresses(ActionContext &context)
{
	bool changed = false;
	ActionOffset offset = context.offset;

	for(int i = 0; i < alternateList.size(); ++i)
	{
		Alternate &alternate = alternateList[i];
		
		context.offset = offset;
        
        bool resolveBeforeIsValid = alternate.condition->ShouldProcessIsValidAtEndOfAlternate() == context.forwards;

		if(resolveBeforeIsValid)
		{
			if(alternate.action->ResolveRelativeAddresses(context)) changed = true;
		}

		AlternateActionCondition::Result result = alternate.condition->IsValid(context, alternate.action);
		
		if(!resolveBeforeIsValid)
		{
			if(alternate.action->ResolveRelativeAddresses(context)) changed = true;
		}
		
		switch(result)
		{
		case AlternateActionCondition::Result::Never:
			// Remove this entry, and continue.
			alternate.condition->Release();
			delete alternate.action;
			alternateList.erase(alternateList.begin()+i);
			--i;
			changed = true;
			break;
		case AlternateActionCondition::Result::Maybe:
			break;
		case AlternateActionCondition::Result::Always:
			if(alternate.condition == &AlwaysAlternateActionCondition::instance) break;
			alternate.condition->Release();
			alternate.condition = &AlwaysAlternateActionCondition::instance;
			for(int j = i+1; j < alternateList.size(); ++j)
			{
				alternateList[j].condition->Release();
				delete alternateList[j].action;
			}
				
			alternateList.erase(alternateList.begin()+i+1, alternateList.end());
			changed = true;
			goto end;
		}
		
	}

end:
	context.offset = offset;
	Action::ResolveRelativeAddresses(context);
	
	return changed;
}

bool AlternateAction::Simplify(ListAction *parent, size_t index)
{
	bool result = false;
	for(size_t i = alternateList.size(); i != 0;)
	{
		--i;
		if(alternateList[i].action->Simplify(nullptr, 0)) result = true;
	}
	
	switch(alternateList.size())
	{
	case 0:
		parent->RemoveActionAtIndex(index);
		delete this;
		return true;
	case 1:
		if(alternateList[0].condition == &AlwaysAlternateActionCondition::instance)
		{
			parent->ReplaceActionAtIndex(index, alternateList[0].action);
			alternateList[0].condition->Release();
			alternateList.clear();
			delete this;
			return true;
		}
		break;
	}
	return result;
}

void AlternateAction::DelayAndConsolidate()
{
	for(const Alternate& alternate : alternateList)
	{
		alternate.action->DelayAndConsolidate();
	}
}

void AlternateAction::Group()
{
	for(const Alternate& alternate : alternateList)
	{
		alternate.action->Group();
	}
}

//============================================================================

ListAction::ListAction()
{
}

ListAction::~ListAction()
{
	for(Action *action : actionList)
	{
		delete action;
	}
}

void ListAction::Append(Action *action)
{
	if(action->IsLiteral()
	   && actionList.size() > 0
	   && actionList.back()->IsLiteralAction())
	{
		LiteralAction* lastLiteralAction = (LiteralAction*) actionList.back();
		action->AppendToLiteral(lastLiteralAction);
		delete action;
	}
	else
	{
		actionList.push_back(action);
	}
}

void ListAction::Dump() const
{
	printf("[");
	for(Action *action : actionList)
	{
		action->Dump();
	}
	printf("]");
}

size_t ListAction::GetMinimumLength() const
{
	size_t minimumLength = 0;
	for(Action *action : actionList)
	{
		minimumLength += action->GetMinimumLength();
	}
	return minimumLength;
}

size_t ListAction::GetMaximumLength() const
{
	size_t maximumLength = 0;
	for(Action *action : actionList)
	{
		maximumLength += action->GetMaximumLength();
	}
	return maximumLength;
}

bool ListAction::IsLiteral() const
{
	for(Action *action : actionList)
	{
		if(!action->IsLiteral()) return false;
	}
	return true;
}

void ListAction::ResolveRelativeAddresses()
{
	bool changes;
	do
	{
		changes = false;

		{
			ActionContext actionContext;
			actionContext.forwards = true;
			actionContext.offset.alignment = 0;
			actionContext.offset.blockIndex = 0;
			actionContext.offset.offsetIntoBlock = 0;
			actionContext.offset.totalMinimumOffset = 0;
			actionContext.offset.totalMaximumOffset = 0;
			if(ResolveRelativeAddresses(actionContext)) changes = true;
		}

		{
			ActionContext actionContext;
			actionContext.forwards = false;
			actionContext.offset.alignment = 0;
			actionContext.offset.blockIndex = 0;
			actionContext.offset.offsetIntoBlock = 0;
			actionContext.offset.totalMinimumOffset = 0;
			actionContext.offset.totalMaximumOffset = 0;
			if(ResolveRelativeAddresses(actionContext)) changes = true;
		}
		
		if(Simplify(nullptr, 0)) changes = true;
		
	} while(changes);
}

bool ListAction::Simplify(ListAction *parent, size_t index)
{
	bool changes = false;
	for(size_t i = actionList.size(); i != 0;)
	{
		--i;
		if(actionList[i]->Simplify(this, i)) changes = true;
	}

	if(!parent) return changes;

	parent->ReplaceActionAtIndexWithList(index, actionList);
	actionList.clear();
	delete this;
	return true;
}

void ListAction::DelayAndConsolidate()
{
	for(Action* action : actionList)
	{
		action->DelayAndConsolidate();
	}
	
	// Step through all actions, and find out if there are
	// actions that can be delayed past literals
	std::vector<Action *> delayList;
	bool needConsolidate = false;
	int delay = 0;
	int startDelayListIndex = 0;
	for(int i = 0; i < actionList.size(); ++i)
	{
		Action *action = actionList[i];
		if(action->IsLiteralAction())
		{
			if(delayList.size() == 0) continue;
			LiteralAction *literalAction = (LiteralAction *)action;
			if(!delayList[0]->CanDelay(delay + literalAction->GetNumberOfBytes()))
			{
				needConsolidate = true;
				DelayActions(delayList, startDelayListIndex, i, delay);
				delayList.clear();
				delay = 0;
			}
			else
			{
				delay += literalAction->GetNumberOfBytes();
			}
		}
		else if(action->CanDelay())
		{
			if(delayList.size() == 0) startDelayListIndex = i;
			delayList.push_back(action);
		}
		else
		{
			// Not literal, and cannot delay.
			if(delay != 0)
			{
				needConsolidate = true;
				DelayActions(delayList, startDelayListIndex, i, delay);
			}
			if(delayList.size())
			{
				delayList.clear();
				delay = 0;
			}
		}
	}
	if(delay != 0)
	{
		needConsolidate = true;
		DelayActions(delayList, startDelayListIndex, (int) actionList.size(), delay);
	}
	
	if(needConsolidate) ConsolidateLiteralActions();
}

void ListAction::Group()
{
	for(int i = 0; i < actionList.size(); ++i)
	{
		Action *action = actionList[i];
		if(!action->CanGroup()) continue;

		int next = i+1;
		for(int j = i+1; j < actionList.size(); ++j)
		{
			Action *other = actionList[j];
			if(other->GetMinimumLength() != 0) break;
			
			if(action->CanGroup(other))
			{
				if(j == next) ++next;
				else
				{
					actionList.erase(actionList.begin()+j);
					actionList.insert(actionList.begin()+next, other);
					++i;
				}
			}
		}
	}
}

void ListAction::DelayActions(const std::vector<Action *> &delayList, int startIndex, int toIndex, int delay)
{
	actionList.insert(actionList.begin()+toIndex, delayList.begin(), delayList.end());
	int index = startIndex;
	int delayListIndex = 0;
	while(delayListIndex < delayList.size())
	{
		Action *action = actionList[index];
		if(action == delayList[delayListIndex])
		{
			assert(action->CanDelay(delay));
			action->Delay(delay);
			actionList.erase(actionList.begin()+index);
			++delayListIndex;
			continue;
		}
		else if(action->IsLiteralAction())
		{
			delay -= action->GetMinimumLength();
		}

		++index;
	}
}

void ListAction::ConsolidateLiteralActions()
{
	for(size_t i = actionList.size(); i >= 2; --i)
	{
		Action *a2 = actionList[i-1];
		Action *a1 = actionList[i-2];
		if(a1->IsLiteralAction() && a2->IsLiteralAction())
		{
			a2->AppendToLiteral((LiteralAction *) a1);
			delete a2;
			actionList.erase(actionList.begin() + i-1);
		}
	}
}

void ListAction::ReplaceActionAtIndexWithList(size_t i, std::vector<Action*> &newActionList)
{
	actionList.erase(actionList.begin()+i);
	actionList.insert(actionList.begin()+i, newActionList.begin(), newActionList.end());
}

bool ListAction::ResolveRelativeAddresses(ActionContext &context)
{
	bool changed = false;
	if(context.forwards)
	{
		for(Action *action : actionList)
		{
			if(action->ResolveRelativeAddresses(context)) changed = true;
		}
	}
	else
	{
		for(size_t i = actionList.size(); i != 0;)
		{
			--i;
			if(actionList[i]->ResolveRelativeAddresses(context)) changed = true;
		}
	}
	return changed;
}

void ListAction::AppendToLiteral(LiteralAction *literal) const
{
	for(Action *action : actionList)
	{
		action->AppendToLiteral(literal);
	}
}

void ListAction::WriteByteCode(std::vector<uint8_t> &result, ActionWriteContext &context) const
{
    for(Action *action : actionList)
    {
        action->WriteByteCode(result, context);
        context.previousAction = action;
    }
}

//============================================================================
