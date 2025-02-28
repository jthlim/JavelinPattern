//============================================================================

#include "Javelin/Pattern/Internal/PatternProcessor.h"
#if defined(JABI_AMD64_SYSTEM_V)

//============================================================================

namespace Javelin::PatternInternal
{
//============================================================================

	namespace Amd64FindMethods
	{
		void* InternalAvxFindNibbleMask();
		void* InternalAvxFindPairNibbleMask();
		void* InternalAvxFindPairNibbleMaskPath();
		void* InternalAvxFindTripletNibbleMask();
		void* InternalAvxFindTripletNibbleMaskPath();
		void* InternalAvxFindQuadNibbleMask();
		void* InternalAvxFindEitherOf2();
		void* InternalAvxFindEitherOf3();
		void* InternalAvxFindPair();
		void* InternalAvxFindPair2();
		void* InternalAvxFindPairPath2();
		void* InternalAvxFindByteRange();
		void* InternalAvxFindByteRangePair();
		void* InternalAvxFindTriplet();

		void* InternalAvx2FindNibbleMask();
		void* InternalAvx2FindPairNibbleMask();
		void* InternalAvx2FindPairNibbleMaskPath();
		void* InternalAvx2FindTripletNibbleMaskPath();
		void* InternalAvx2FindByte();
		void* InternalAvx2FindEitherOf2();
		void* InternalAvx2FindEitherOf3();
		void* InternalAvx2FindPair();
		void* InternalAvx2FindPair2();
		void* InternalAvx2FindPairPath2();
		void* InternalAvx2FindByteRange();
		void* InternalAvx2FindTriplet();

        void* InternalAvx512FindNibbleMask();
        void* InternalAvx512FindPairNibbleMask();
        void* InternalAvx512FindPairNibbleMaskPath();
        void* InternalAvx512FindByte();
        void* InternalAvx512FindEitherOf2();
        void* InternalAvx512FindEitherOf3();
        void* InternalAvx512FindPair();
        void* InternalAvx512FindPair2();
        void* InternalAvx512FindPairPath2();
        void* InternalAvx512FindByteRange();

        void* InternalFindByte();
		void* InternalFindEitherOf2();
		void* InternalFindEitherOf3();
		void* InternalFindEitherOf4();
		void* InternalFindEitherOf5();
		void* InternalFindEitherOf6();
		void* InternalFindEitherOf7();
		void* InternalFindEitherOf8();
		void* InternalFindPair();
		void* InternalFindPair2();
		void* InternalFindPair3();
		void* InternalFindPair4();
		void* InternalFindTriplet();
		void* InternalFindTriplet2();
		void* InternalFindByteRange();
		void* InternalFindByteRangePair();
		void* InternalFindShiftOr();

		void* InternalFindByteReverse();
	}
	
//============================================================================
}

//============================================================================
#endif
//============================================================================
