//============================================================================

#include "Javelin/Type/SmartPointer.h"
#include "Javelin/Stream/IReader.h"
#include "Javelin/Stream/IWriter.h"

//============================================================================

using namespace Javelin;

//============================================================================

bool SmartPointerStreaming::ShouldWriteObject(IWriter& output, void* object)
{
	return output.WriteObjectReferenceId(object);
}

//============================================================================
