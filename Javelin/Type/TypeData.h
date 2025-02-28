//============================================================================

#pragma once
#include "Javelin/JavelinBase.h"
#include <float.h>
#include <limits.h>

#if !defined(FLT_MAX)
	#define FLT_MAX 3.402823466e+38
#endif
#if !defined(DBL_MAX)
	#define DBL_MAX 1.7976931348623158e+308
#endif

//============================================================================

namespace Javelin
{
//============================================================================
	
	// Generic interface
	template<typename T> struct TypeData
	{
//		static constexpr T Minimum()			{ return 0;	}
//		static T Maximum();
//
//		// Allows other TypeDatas to override what "Zero" means (eg. for intervals)
//		static constexpr T Zero()				{ return 0; }
//
//		static constexpr T Identity()			{ return 1;	}

		enum { IS_INTEGRAL			= false };
		enum { IS_FLOAT				= false };
		enum { IS_POD				= false };
		enum { IS_BITWISE_COPY_SAFE	= false };
		enum { IS_POINTER			= false };

		typedef T NonPointerType;
		typedef T NonReferenceType;
		typedef T NonConstType;
		typedef T StorageType;			// This will take a const Point3 &p and return Point3. ie. NonConst, NonReference
		
		// ParameterUpcast is the type that the C compiler will use when passing parameters through "..."
		typedef T ParameterUpcast;

		// MathematicalUpcast is the type that the C compiler will use when performing basic operations
		typedef T MathematicalUpcast;
		typedef T DeltaType;

		// MathematicalFloatUpcast is the type that the C compiler will need/use when performing <cmath> functions
		typedef float MathematicalFloatUpcast;

		// ParameterType is the type most appropriate for passing this data around
		typedef const T& ParameterType;
		typedef T&& MoveType;
		typedef T& ReferenceType;
	};

//============================================================================

	template<typename T> struct TypeData<T*>
	{
		static constexpr T* Zero()				{ return 0; }

		enum { IS_FLOAT				= false };
		enum { IS_POD				= true  };
		enum { IS_BITWISE_COPY_SAFE	= true  };
		enum { IS_POINTER			= true  };

		typedef T NonPointerType;
		typedef T* NonReferenceType;
		typedef T* NonConstType;
		typedef T* StorageType;
		
		typedef T* ParameterUpcast;
		typedef T* ParameterType;
		typedef T*&& MoveType;
		typedef T*& ReferenceType;
		
		typedef void MathematicalUpcast;
		typedef size_t DeltaType;
	};

//============================================================================
	
	template<typename T> struct TypeData<T&>
	{
		enum { IS_FLOAT				= false };
		enum { IS_POD				= true  };
		enum { IS_BITWISE_COPY_SAFE	= true  };
		enum { IS_POINTER			= false };
		
		typedef T& NonPointerType;
		typedef T NonReferenceType;
		typedef T& NonConstType;
		typedef T StorageType;
		
		typedef T& ParameterUpcast;
		typedef T& ParameterType;
		typedef T&& MoveType;
		typedef T& ReferenceType;
		
		typedef void MathematicalUpcast;
	};
	
//============================================================================
	
	template<typename T> struct TypeData<const T&>
	{
		enum { IS_FLOAT				= false };
		enum { IS_POD				= true  };
		enum { IS_BITWISE_COPY_SAFE	= true  };
		enum { IS_POINTER			= false };
		
		typedef const T& NonPointerType;
		typedef const T NonReferenceType;
		typedef T NonConstType;
		typedef T StorageType;
		
		typedef const T& ParameterUpcast;
		typedef const T& ParameterType;
		typedef T&& MoveType;
		typedef const T& ReferenceType;
		
		typedef void MathematicalUpcast;
	};
	
//============================================================================

	template<typename T> struct TypeData<const T>
	{
		static constexpr T Minimum()		{ return TypeData<T>::Minimum();	}
		static constexpr T Maximum()		{ return TypeData<T>::Minimum();	}
		static constexpr T Zero()			{ return TypeData<T>::Zero();		}
		static constexpr T Identity()		{ return TypeData<T>::Identity();	}

		enum { IS_INTEGRAL			= TypeData<T>::IS_INTEGRAL };
		enum { IS_FLOAT				= TypeData<T>::IS_FLOAT };
		enum { IS_POD				= TypeData<T>::IS_POD };
		enum { IS_BITWISE_COPY_SAFE	= TypeData<T>::IS_BITWISE_COPY_SAFE };
		enum { IS_POINTER			= TypeData<T>::IS_POINTER };

		typedef const T NonPointerType;
		typedef const T NonReferenceType;
		typedef T NonConstType;
		typedef T StorageType;
		
		typedef const typename TypeData<T>::ParameterUpcast ParameterUpcast;
		typedef const typename TypeData<T>::MathematicalUpcast MathematicalUpcast;
		typedef const typename TypeData<T>::DeltaType DeltaType;
		typedef const typename TypeData<T>::MathematicalFloatUpcast MathematicalFloatUpcast;
		typedef const typename TypeData<T>::ParameterType ParameterType;
		typedef T&& MoveType;
		typedef const T& ReferenceType;
	};

//---------------------------------------------------------------------------

	template<> struct TypeData<bool>
	{
		enum { IS_INTEGRAL			= false };
		enum { IS_FLOAT				= false };
		enum { IS_POD				= true  };
		enum { IS_BITWISE_COPY_SAFE	= true  };
		enum { IS_POINTER			= false };
		
		typedef bool NonPointerType;
		typedef bool NonReferenceType;
		typedef bool NonConstType;
		typedef int ParameterUpcast;
		typedef int MathematicalUpcast;
		typedef int DeltaType;
		typedef float MathematicalFloatUpcast;
		typedef bool ParameterType;
		typedef bool&& MoveType;
		typedef bool& ReferenceType;
	};
	
//---------------------------------------------------------------------------
	
	template<> struct TypeData<char>
	{
		static constexpr char Zero()		{ return 0;	}
		static constexpr char Identity()	{ return 1;	}
		
		enum { IS_INTEGRAL			= true };
		enum { IS_FLOAT				= false };
		enum { IS_POD				= true };
		enum { IS_BITWISE_COPY_SAFE	= true };
		enum { IS_POINTER			= false };
		
		typedef char NonPointerType;
		typedef char NonReferenceType;
		typedef char NonConstType;
		typedef int ParameterUpcast;
		typedef int MathematicalUpcast;
		typedef int DeltaType;
		typedef float MathematicalFloatUpcast;
		typedef char ParameterType;
		typedef char&& MoveType;
		typedef char& ReferenceType;
	};

	template<> struct TypeData<signed char>
	{
		static constexpr signed char Minimum()	{ return SCHAR_MIN;	}
		static constexpr signed char Maximum()	{ return SCHAR_MAX;	}
		static constexpr signed char Zero()		{ return 0;			}
		static constexpr signed char Identity()	{ return 1;			}

		enum { IS_INTEGRAL			= true };
		enum { IS_FLOAT				= false };
		enum { IS_POD				= true };
		enum { IS_BITWISE_COPY_SAFE	= true };
		enum { IS_POINTER			= false };

		typedef signed char NonPointerType;
		typedef signed char NonReferenceType;
		typedef signed char NonConstType;
		typedef int ParameterUpcast;
		typedef int MathematicalUpcast;
		typedef int DeltaType;
		typedef float MathematicalFloatUpcast;
		typedef signed char ParameterType;
		typedef signed char&& MoveType;
		typedef signed char& ReferenceType;
	};

	template<> struct TypeData<unsigned char>
	{
		static constexpr unsigned char Minimum()	{ return 0;			}
		static constexpr unsigned char Maximum()	{ return UCHAR_MAX;	}
		static constexpr unsigned char Zero()		{ return 0;			}
		static constexpr unsigned char Identity()	{ return 1;			}

		enum { IS_INTEGRAL			= true };
		enum { IS_FLOAT				= false };
		enum { IS_POD				= true };
		enum { IS_BITWISE_COPY_SAFE	= true };
		enum { IS_POINTER			= false };

		typedef unsigned char NonPointerType;
		typedef unsigned char NonReferenceType;
		typedef unsigned char NonConstType;
		typedef unsigned char StorageType;
		typedef unsigned int ParameterUpcast;
		typedef unsigned int MathematicalUpcast;
		typedef unsigned int DeltaType;
		typedef float MathematicalFloatUpcast;
		typedef unsigned char ParameterType;
		typedef unsigned char&& MoveType;
		typedef unsigned char& ReferenceType;
	};

//---------------------------------------------------------------------------

	template<> struct TypeData<signed short>
	{
		static constexpr signed short Minimum()     { return SHRT_MIN; 	}
		static constexpr signed short Maximum()     { return SHRT_MAX; 	}
		static constexpr signed short Zero()		{ return 0; 		}
		static constexpr signed short Identity()	{ return 1; 		}

		enum { IS_INTEGRAL			= true };
		enum { IS_FLOAT				= false };
		enum { IS_POD				= true };
		enum { IS_BITWISE_COPY_SAFE	= true };
		enum { IS_POINTER			= false };

		typedef signed short NonPointerType;
		typedef signed short NonReferenceType;
		typedef signed short NonConstType;
		typedef signed short StorageType;
		typedef int ParameterUpcast;
		typedef int MathematicalUpcast;
		typedef int DeltaType;
		typedef float MathematicalFloatUpcast;
		typedef signed short ParameterType;
		typedef signed short&& MoveType;
		typedef signed short& ReferenceType;
	};

	template<> struct TypeData<unsigned short>
	{
		static constexpr unsigned short Minimum()		{ return 0;			}
		static constexpr unsigned short Maximum()		{ return USHRT_MAX;	}
		static constexpr unsigned short Zero()          { return 0;			}
		static constexpr unsigned short Identity()      { return 1;			}

		enum { IS_INTEGRAL			= true };
		enum { IS_FLOAT				= false };
		enum { IS_POD				= true };
		enum { IS_BITWISE_COPY_SAFE	= true };
		enum { IS_POINTER			= false };

		typedef unsigned short NonPointerType;
		typedef unsigned short NonReferenceType;
		typedef unsigned short NonConstType;
		typedef unsigned short StorageType;
		typedef unsigned int ParameterUpcast;
		typedef unsigned int MathematicalUpcast;
		typedef unsigned int DeltaType;
		typedef float MathematicalFloatUpcast;
		typedef unsigned short ParameterType;
		typedef unsigned short&& MoveType;
		typedef unsigned short& ReferenceType;
	};

//---------------------------------------------------------------------------

	template<> struct TypeData<signed int>
	{
		static constexpr signed int Minimum()		{ return INT_MIN;		}
		static constexpr signed int Maximum()		{ return INT_MAX;		}
		static constexpr signed int Zero()          { return 0;				}
		static constexpr signed int Identity()      { return 1;				}

		enum { IS_INTEGRAL			= true };
		enum { IS_FLOAT				= false };
		enum { IS_POD				= true };
		enum { IS_BITWISE_COPY_SAFE	= true };
		enum { IS_POINTER			= false };

		typedef signed int NonPointerType;
		typedef signed int NonReferenceType;
		typedef signed int NonConstType;
		typedef signed int StorageType;
		typedef int ParameterUpcast;
		typedef int MathematicalUpcast;
		typedef int DeltaType;
		typedef float MathematicalFloatUpcast;
		typedef signed int ParameterType;
		typedef signed int&& MoveType;
		typedef signed int& ReferenceType;
	};

	template<> struct TypeData<unsigned int>
	{
		static constexpr unsigned int Minimum()     { return 0;				}
		static constexpr unsigned int Maximum()     { return UINT_MAX;		}
		static constexpr unsigned int Zero()		{ return 0;				}
		static constexpr unsigned int Identity()	{ return 1;				}

		enum { IS_INTEGRAL			= true };
		enum { IS_FLOAT				= false };
		enum { IS_POD				= true };
		enum { IS_BITWISE_COPY_SAFE	= true };
		enum { IS_POINTER			= false };

		typedef unsigned int NonPointerType;
		typedef unsigned int NonReferenceType;
		typedef unsigned int NonConstType;
		typedef unsigned int StorageType;
		typedef unsigned int ParameterUpcast;
		typedef unsigned int MathematicalUpcast;
		typedef unsigned int DeltaType;
		typedef float MathematicalFloatUpcast;
		typedef unsigned int ParameterType;
		typedef unsigned int&& MoveType;
		typedef unsigned int& ReferenceType;
	};

//---------------------------------------------------------------------------

	template<> struct TypeData<signed long>
	{
		static constexpr signed long Minimum()		{ return LONG_MIN;		}
		static constexpr signed long Maximum()		{ return LONG_MAX;		}
		static constexpr signed long Zero()			{ return 0;				}
		static constexpr signed long Identity()		{ return 1;				}

		enum { IS_INTEGRAL			= true };
		enum { IS_FLOAT				= false };
		enum { IS_POD				= true };
		enum { IS_BITWISE_COPY_SAFE	= true };
		enum { IS_POINTER			= false };

		typedef signed long NonPointerType;
		typedef signed long NonReferenceType;
		typedef signed long NonConstType;
		typedef signed long StorageType;
		typedef long ParameterUpcast;
		typedef long MathematicalUpcast;
		typedef long DeltaType;
		typedef float MathematicalFloatUpcast;
		typedef signed long ParameterType;
		typedef signed long&& MoveType;
		typedef signed long& ReferenceType;
	};

	template<> struct TypeData<unsigned long>
	{
		static constexpr unsigned long Minimum()	{ return 0;				}
		static constexpr unsigned long Maximum()	{ return ULONG_MAX;		}
		static constexpr unsigned long Zero()		{ return 0;				}
		static constexpr unsigned long Identity()	{ return 1;				}

		enum { IS_INTEGRAL			= true };
		enum { IS_FLOAT				= false };
		enum { IS_POD				= true };
		enum { IS_BITWISE_COPY_SAFE	= true };
		enum { IS_POINTER			= false };

		typedef unsigned long NonPointerType;
		typedef unsigned long NonReferenceType;
		typedef unsigned long NonConstType;
		typedef unsigned long StorageType;
		typedef unsigned long ParameterUpcast;
		typedef unsigned long MathematicalUpcast;
		typedef unsigned long DeltaType;
		typedef float MathematicalFloatUpcast;
		typedef unsigned long ParameterType;
		typedef unsigned long&& MoveType;
		typedef unsigned long& ReferenceType;
	};

//---------------------------------------------------------------------------

	template<> struct TypeData<signed long long>
	{
		static constexpr signed long long Minimum()		{ return LLONG_MIN;	}
		static constexpr signed long long Maximum()		{ return LLONG_MAX;	}
		static constexpr signed long long Zero()		{ return 0;			}
		static constexpr signed long long Identity()	{ return 1;			}

		enum { IS_INTEGRAL			= true };
		enum { IS_FLOAT				= false };
		enum { IS_POD				= true };
		enum { IS_BITWISE_COPY_SAFE	= true };
		enum { IS_POINTER			= false };

		typedef signed long long NonPointerType;
		typedef signed long long NonReferenceType;
		typedef signed long long NonConstType;
		typedef signed long long StorageType;
		typedef long long ParameterUpcast;
		typedef long long MathematicalUpcast;
		typedef long long DeltaType;
		typedef double MathematicalFloatUpcast;
		typedef signed long long ParameterType;
		typedef signed long long&& MoveType;
		typedef signed long long& ReferenceType;
	};

	template<> struct TypeData<unsigned long long>
	{
		static constexpr unsigned long long Minimum()	{ return 0;				}
		static constexpr unsigned long long Maximum()	{ return ULLONG_MAX;	}
		static constexpr unsigned long long Zero()		{ return 0;				}
		static constexpr unsigned long long Identity()	{ return 1;				}

		enum { IS_INTEGRAL			= true  };
		enum { IS_FLOAT				= false };
		enum { IS_POD				= true  };
		enum { IS_BITWISE_COPY_SAFE	= true  };
		enum { IS_POINTER			= false };

		typedef unsigned long long NonPointerType;
		typedef unsigned long long NonReferenceType;
		typedef unsigned long long NonConstType;
		typedef unsigned long long StorageType;
		typedef unsigned long long ParameterUpcast;
		typedef unsigned long long MathematicalUpcast;
		typedef unsigned long long DeltaType;
		typedef double MathematicalFloatUpcast;
		typedef unsigned long long ParameterType;
		typedef unsigned long long&& MoveType;
		typedef unsigned long long& ReferenceType;
	};

//---------------------------------------------------------------------------

	template<> struct TypeData<float>
	{
		static constexpr float Minimum()		{ return -FLT_MAX;	}
		static constexpr float Maximum()		{ return FLT_MAX;	}
		static constexpr float Zero()			{ return 0.0f;		}
		static constexpr float Identity()		{ return 1.0f;		}

		enum { IS_INTEGRAL			= false };
		enum { IS_FLOAT				= true  };
		enum { IS_POD				= true  };
		enum { IS_BITWISE_COPY_SAFE	= true  };
		enum { IS_POINTER			= false };

		typedef float	NonPointerType;
		typedef float	NonReferenceType;
		typedef float	NonConstType;
		typedef float	StorageType;
		typedef double	ParameterUpcast;
		typedef float	MathematicalUpcast;
		typedef float	DeltaType;
		typedef float	MathematicalFloatUpcast;
		typedef float	ParameterType;
		typedef float&&	MoveType;
		typedef float&	ReferenceType;
	};

//---------------------------------------------------------------------------

	template<> struct TypeData<double>
	{
		static constexpr double Minimum()		{ return -DBL_MAX;	}
		static constexpr double Maximum()		{ return DBL_MAX;	}
		static constexpr double Zero()			{ return 0.0;		}
		static constexpr double Identity()		{ return 1.0;		}

		enum { IS_INTEGRAL			= false };
		enum { IS_FLOAT				= true  };
		enum { IS_POD				= true  };
		enum { IS_BITWISE_COPY_SAFE	= true  };
		enum { IS_POINTER			= false };

		typedef double	NonPointerType;
		typedef double	NonReferenceType;
		typedef double	NonConstType;
		typedef double	StorageType;
		typedef double	ParameterUpcast;
		typedef double	MathematicalUpcast;
		typedef double	DeltaType;
		typedef double	MathematicalFloatUpcast;
		typedef double	ParameterType;
		typedef double&& MoveType;
		typedef double& ReferenceType;
	};
	
//---------------------------------------------------------------------------
	
	template<> struct TypeData<long double>
	{
		static constexpr long double Minimum()		{ return -LDBL_MAX;	}
		static constexpr long double Maximum()		{ return LDBL_MAX;	}
		static constexpr long double Zero()			{ return 0.0;		}
		static constexpr long double Identity()		{ return 1.0;		}
		
		enum { IS_INTEGRAL			= false };
		enum { IS_FLOAT				= true  };
		enum { IS_POD				= true  };
		enum { IS_BITWISE_COPY_SAFE	= true  };
		enum { IS_POINTER			= false };
		
		typedef long double	NonPointerType;
		typedef long double	NonReferenceType;
		typedef long double	NonConstType;
		typedef long double	StorageType;
		typedef long double	ParameterUpcast;
		typedef long double	MathematicalUpcast;
		typedef long double	DeltaType;
		typedef long double	MathematicalFloatUpcast;
		typedef long double	ParameterType;
		typedef long double&& MoveType;
		typedef long double& ReferenceType;
	};
	
//============================================================================
} // namespace Javelin
//============================================================================
