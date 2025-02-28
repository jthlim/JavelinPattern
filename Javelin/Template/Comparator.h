//============================================================================

#pragma once

//============================================================================

namespace Javelin
{
//============================================================================

	template<typename T> struct IComparator
	{
		virtual bool IsEqual(const T& a, const T& b)	= 0;
		virtual bool IsOrdered(const T& a, const T& b)	= 0;
	};

//============================================================================

	struct Equal
	{
		template<typename A, typename B>
			static bool IsEqual(A&& a, B&& b)				{ return a == b;	}
	};

//============================================================================

	struct Less : public Equal
	{
		template<typename A, typename B>
			static bool IsOrdered(A&& a, B&& b)	{ return a < b;		}
	};

	struct Greater : public Equal
	{
		template<typename A, typename B>
			static bool IsOrdered(A&& a, B&& b)	{ return a > b;		}
	};

//============================================================================

	struct LessOrEqual : public Equal
	{
		template<typename A, typename B>
			static bool IsOrdered(A&& a, B&& b)	{ return a <= b;	}
	};

	struct GreaterOrEqual : public Equal
	{
		template<typename A, typename B>
			static bool IsOrdered(A&& a, B&& b)	{ return a >= b;	}
	};

//============================================================================

	// Priroity: 1 = first, 2 = second, 3 = third
	//			-1 = last, -2 = second to last, etc.
	struct PriorityComparator : public Equal
	{
		template<typename A, typename B>
			static bool IsOrdered(A&& a, B&& b)
		{
			if(a >= 0)
			{
				if(b < 0) return true;
			}
			else
			{
				if(b >= 0) return false;
			}
			return a <= b;
		}
	};
	
//============================================================================

	template<typename CLASS, typename T, typename C=Less> struct MemberAcccessorComparator
	{
		const C c;
		T CLASS::*e;

		MemberAcccessorComparator(CLASS T::*ae, const C& aC() = C()) : c(aC), e(ae) { }

		bool IsOrdered(const CLASS& a, const CLASS& b) 	const	{ return c.IsOrdered(a.*e, b.*e);	}
		bool IsOrdered(const CLASS& a, const T& b)		const	{ return c.IsOrdered(a.*e, b);		}
		bool IsOrdered(const T& a, const CLASS& b)		const	{ return c.IsOrdered(a, b.*e);		}
		
		bool IsEqual(const CLASS& a, const CLASS& b)	const	{ return c.IsEqual(a.*e, b.*e);		}
		bool IsEqual(const CLASS& a, const T& b)		const	{ return c.IsEqual(a.*e, b);		}
		bool IsEqual(const T& a, const CLASS& b)		const	{ return c.IsEqual(a, b.*e);		}
	};

	template<typename CLASS, typename T, typename C=Less> struct MemberFunctionComparator
	{
		const C c;
		T (CLASS::*e)();

		MemberFunctionComparator(T (CLASS::*ae)(), const C& aC() = C()) : c(aC), e(ae) { }

		bool IsOrdered(const CLASS& a, const CLASS& b) 	const	{ return c.IsOrdered((a.*e)(), (b.*e)());	}
		bool IsOrdered(const CLASS& a, const T& b)		const	{ return c.IsOrdered((a.*e)(), b);			}
		bool IsOrdered(const T& a, const CLASS& b)		const	{ return c.IsOrdered(a, (b.*e)());			}
		
		bool IsEqual(const CLASS& a, const CLASS& b)	const	{ return c.IsEqual((a.*e)(), (b.*e)());		}
		bool IsEqual(const CLASS& a, const T& b)		const	{ return c.IsEqual((a.*e)(), b);			}
		bool IsEqual(const T& a, const CLASS& b)		const	{ return c.IsEqual(a, (b.*e)());			}
	};

	template<typename CLASS, typename T, T CLASS::*M, typename C=Less> struct MemberComparator
	{
		const C c;
		
		MemberComparator(const C& aC = C()) : c(aC)	{ }
		
		bool IsOrdered(const CLASS& a, const CLASS& b) 	const	{ return c.IsOrdered(a.*M, b.*M);	}
		bool IsOrdered(const CLASS& a, const T& b)		const	{ return c.IsOrdered(a.*M, b);		}
		bool IsOrdered(const T& a, const CLASS& b)		const	{ return c.IsOrdered(a, b.*M);		}
		
		bool IsEqual(const CLASS& a, const CLASS& b)	const	{ return c.IsEqual(a.*M, b.*M);		}
		bool IsEqual(const CLASS& a, const T& b)		const	{ return c.IsEqual(a.*M, b);		}
		bool IsEqual(const T& a, const CLASS& b)		const	{ return c.IsEqual(a, b.*M);		}
	};

//============================================================================

	template<typename FirstComparator, typename SecondComparator> struct ChainComparator
	{
		template<typename A, typename B>
			bool IsOrdered(A&& a, B&& b)	const	
		{ 
			if(FirstComparator::IsEqual(a, b)) return SecondComparator::IsOrdered(a, b);
			else return FirstComparator::IsOrdered(a, b);
		}
		
		template<typename A, typename B>
			bool IsEqual(A&& a, B&& b) const		{ return FirstComparator::IsEqual(a, b) && SecondComparator::IsEqual(a, b);	}
	};

//============================================================================
} // namespace Javelin

//============================================================================
