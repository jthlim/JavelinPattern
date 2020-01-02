//============================================================================

#pragma once

//============================================================================

#if defined(__amd64__)
  #include "Javelin/Assembler/x64/Assembler.h"
#elif defined(__arm64__)
  #include "Javelin/Assembler/arm64/Assembler.h"
#else
  #error "Unsupported architecture""
#endif

//============================================================================
