#ifndef _BML_FAST_MATH_H_
#define _BML_FAST_MATH_H_

#include "vm_types.h"

/*! \addtogroup fast_math Fast 32-bit maths
 * @{
 * \brief Fast conversions.
 *
 * The VM creates its own environment, which is used to (un)serialize programs.
 * The map supports indexed access to keys and values and random access by key lookup.
 */

/*! \brief union to reinterpret bits in a word as an integer and a float. */
union _intfloat_conv {
	long i;	/*!< \brief bits as integer */
	float_t f;	/*!< \brief bits as floating point */
};

/*! _brief Public type for conversion union. */
typedef union _intfloat_conv _IFC;

#ifdef __32__
/* 32-bit floats and ints only ! */
/*! \brief Magic converting bias, hex version. */
#define BIAS_HEX ( ((23+127)<<23) + (1<<22) )
/*! \brief Converting bias as an long */
#define BIAS_INT ((long)BIAS_HEX)
/*! \brief Converting bias as a float */
#define BIAS_FLOAT ((float)12582912.0f)

/*! \brief Convert from float to long. */
static inline long f2i(float_t f) {
	_IFC c;
	c.f=f+BIAS_FLOAT;
	return c.i-BIAS_INT;
}

/*! \brief Convert from long to float. */
static inline float_t i2f(long i) {
	_IFC c;
	c.i=i+BIAS_INT;
	return c.f-BIAS_FLOAT;
}

#else

/*! \brief Convert from float to long. */
static inline long f2i(float_t f) {
	return (long)f;
}

/*! \brief Convert from long to float. */
static inline float_t i2f(long i) {
	return (float_t)i;
}

#endif


/*! @{ \brief Ease writing of arithmetic opcodes. */
#define fast_apply_bin_int_func(_ta,_a,_tb,_b,_op,_ret)	do {\
		_IFC _R;\
		switch(((((word_t)_ta)-1)<<1)|(((word_t)_tb)-1)) {\
		case 0:\
			_R.i = _op(*(long*)(&_a), *(long*)(&_b));\
			break;\
		case 1:\
			_R.i = _op(*(long*)(&_a), f2i(_b));\
			break;\
		case 2:\
			_R.i = _op(f2i(_a), *(long*)(&_b));\
			break;\
		case 3:\
			_R.i = _op(f2i(_a), f2i(_b));\
		};\
		_ret=_R.i;\
	} while(0)


#define fast_apply_bin_func(_ta,_a,_tb,_b,_opi,_opf,_ret,_ret_typ)	do {\
		_IFC _R,_X,_Y;\
		switch(((_ta==DataFloat)<<1)|(_tb==DataFloat)) {\
		case 0:\
			_R.i = _opi(*(long*)(&_a), *(long*)(&_b));\
			break;\
		case 1:\
			_X.i = (_b);\
			_R.f = _opf(i2f(*(long*)(&_a)), _X.f);\
			break;\
		case 2:\
			_X.i = (_a);\
			_R.f = _opf(_X.f, i2f(*(long*)(&_b)));\
			break;\
		case 3:\
			_X.i = (_a);\
			_Y.i = (_b);\
			_R.f = _opf(_X.f, _Y.f);\
		};\
		_ret_typ=(_ta==DataFloat||_tb==DataFloat)?DataFloat:DataInt;\
		_ret=_R.i;\
	} while(0)
/*@}*/

/*@}*/


#endif

