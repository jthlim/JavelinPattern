//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"

//============================================================================

namespace Javelin
{
//============================================================================

	namespace Math
	{
		// These are here to make lambdas easier to read.
		template<typename T> T Add(T a, T b)		{ return a+b;	}
		template<typename T> T Subtract(T a, T b)	{ return a-b;	}
		template<typename T> T Multiple(T a, T b)	{ return a*b;	}
		template<typename T> T Divide(T a, T b)		{ return a/b;	}
		template<typename T> T Square(T a) 			{ return a*a;	}
		template<typename T> T Negate(T a) 			{ return -a; 	}
		template<typename T, T k> T Scale(T a) 		{ return k*a;	}
		
		
		// Templated math functions
		template<typename T> T Abs(T x);
		template<typename T> T ACos(T x);
		template<typename T> T ASin(T x);
		template<typename T> T ATan(T x);
		template<typename T> T ATan(T y, T x);
		template<typename T> T Ceil(T x);
		template<typename T> T CopySign(T magnitude, T sign);
		template<typename T> T Cos(T x);
		template<typename T> T Exp(T x);
		template<typename T> T Floor(T x);
		template<typename T> T Log(T x);
		template<typename T> T Log10(T x);
		template<typename T> T Mod(T x, T y);
		template<typename T> T Pow(T x, T y);
		template<typename T> T Pow10(T x);
		template<typename T> T Round(T x);
		template<typename T> T Sin(T x);
		template<typename T> T Sqrt(T x);
		template<typename T> T Tan(T x);
		template<typename T> bool ApproximateEquals(T x, T y, T ACCEPTABLE_ERROR = 0.001f);

		
		// Int specializations
		template<> JINLINE int Abs(int x)						{ return abs(x);			}
		template<> JEXPORT int Pow(int x, int y);
		template<> JEXPORT int Sqrt(int x);
		template<> JINLINE int CopySign(int magn, int sign)		{ int x = sign >> 31; return (Abs(magn) ^ x) - x; }

		// Unsigned specialization
		template<> JEXPORT unsigned int Log10(unsigned int x);
		
		// Float specializations
		template<> JINLINE float Abs(float x)					{ return fabsf(x);			}
		template<> JINLINE float ACos(float x)					{ return acosf(x);			}
		template<> JINLINE float ASin(float x)					{ return asinf(x);			}
		template<> JINLINE float ATan(float x)					{ return atanf(x);			}
		template<> JINLINE float ATan(float y, float x)			{ return atan2f(y, x);		}
		template<> JINLINE float Ceil(float x)					{ return ceilf(x);			}
		template<> JINLINE float CopySign(float m, float s)		{ return copysignf(m, s);	}
		template<> JINLINE float Cos(float x)					{ return cosf(x);			}
		template<> JINLINE float Exp(float x)					{ return expf(x);			}
		template<> JINLINE float Floor(float x)					{ return floorf(x);			}
		template<> JINLINE float Log(float x)					{ return logf(x);			}
		template<> JINLINE float Log10(float x)					{ return log10f(x);			}
		template<> JINLINE float Mod(float x, float y)			{ return fmodf(x, y);		}
		template<> JINLINE float Pow(float x, float y)			{ return powf(x, y);		}
		template<> JINLINE float Pow10(float x)					{ return powf(10, x);		}
#if defined(JCOMPILER_MSVC)
		template<> JINLINE float Round(float x)					{ return floorf(x+0.5f); 	}
#else
		template<> JINLINE float Round(float x)					{ return roundf(x);			}
#endif
		template<> JINLINE float Sin(float x)					{ return sinf(x);			}
		template<> JINLINE float Sqrt(float x)					{ return sqrtf(x);			}
		template<> JINLINE float Tan(float x)					{ return tanf(x);			}
		template<> JINLINE bool ApproximateEquals(float a, float b, float ACCEPTABLE_ERROR) { return Abs(a-b) < ACCEPTABLE_ERROR; }

		
		// Double specializations
		template<> JINLINE double Abs(double x)					{ return fabs(x);			}
		template<> JINLINE double ACos(double x)				{ return acos(x);			}
		template<> JINLINE double ASin(double x)				{ return asin(x);			}
		template<> JINLINE double ATan(double x)				{ return atan(x);			}
		template<> JINLINE double ATan(double y, double x)		{ return atan2(y, x);		}
		template<> JINLINE double Ceil(double x)				{ return ceil(x);			}
		template<> JINLINE double CopySign(double m, double s)	{ return copysign(m, s);	}
		template<> JINLINE double Cos(double x)					{ return cos(x);			}
		template<> JINLINE double Exp(double x)					{ return exp(x);			}
		template<> JINLINE double Floor(double x)				{ return floor(x);			}
		template<> JINLINE double Log(double x)					{ return log(x);			}
		template<> JINLINE double Log10(double x)				{ return log10(x);			}
		template<> JINLINE double Mod(double x, double y)		{ return fmod(x, y);		}
		template<> JINLINE double Pow(double x, double y)		{ return pow(x, y);			}
		template<> JINLINE double Pow10(double x)				{ return pow(10, x);		}
#if defined(JCOMPILER_MSVC)
		template<> JINLINE double Round(double x)				{ return floor(x+0.5f); 	}
#else
		template<> JINLINE double Round(double x)				{ return round(x);			}
#endif
		template<> JINLINE double Sin(double x)					{ return sin(x);			}
		template<> JINLINE double Sqrt(double x)				{ return sqrt(x);			}
		template<> JINLINE double Tan(double x)					{ return tan(x);			}
		template<> JINLINE bool ApproximateEquals(double a, double b, double ACCEPTABLE_ERROR) { return Abs(a-b) < ACCEPTABLE_ERROR; }

		
		// Long double specializations
		template<> JINLINE long double Abs(long double x)						{ return fabsl(x);			}
		template<> JINLINE long double ACos(long double x)						{ return acosl(x);			}
		template<> JINLINE long double ASin(long double x)						{ return asinl(x);			}
		template<> JINLINE long double ATan(long double x)						{ return atanl(x);			}
		template<> JINLINE long double ATan(long double y, long double x)		{ return atan2l(y, x);		}
		template<> JINLINE long double Ceil(long double x)						{ return ceill(x);			}
		template<> JINLINE long double CopySign(long double m, long double s)	{ return copysignl(m, s);	}
		template<> JINLINE long double Cos(long double x)						{ return cosl(x);			}
		template<> JINLINE long double Exp(long double x)						{ return expl(x);			}
		template<> JINLINE long double Floor(long double x)						{ return floorl(x);			}
		template<> JINLINE long double Log(long double x)						{ return logl(x);			}
		template<> JINLINE long double Log10(long double x)						{ return log10l(x);			}
		template<> JINLINE long double Mod(long double x, long double y)		{ return fmodl(x, y);		}
		template<> JINLINE long double Pow(long double x, long double y)		{ return powl(x, y);		}
		template<> JINLINE long double Pow10(long double x)						{ return powl(10, x);		}
#if defined(JCOMPILER_MSVC)
		template<> JINLINE long double Round(long double x)						{ return floorl(x+0.5f); 	}
#else
		template<> JINLINE long double Round(long double x)						{ return roundl(x);			}
#endif
		template<> JINLINE long double Sin(long double x)						{ return sinl(x);			}
		template<> JINLINE long double Sqrt(long double x)						{ return sqrtl(x);			}
		template<> JINLINE long double Tan(long double x)						{ return tanl(x);			}
		template<> JINLINE bool ApproximateEquals(long double a, long double b, long double ACCEPTABLE_ERROR) { return Abs(a-b) < ACCEPTABLE_ERROR; }
	};

//============================================================================
} // namespace Javelin
//============================================================================
