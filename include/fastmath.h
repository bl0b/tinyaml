#ifndef _BML_FAST_MATH_H_
#define _BML_FAST_MATH_H_

/* 32-bit floats and ints only ! */
#define BIAS_HEX ( ((23+127)<<23) + (1<<22) )
#define BIAS_INT ((long int)BIAS_HEX)
#define BIAS_FLOAT ((float)12582912.0f)


typedef union _intfloat_conv {
	long int i;
	float f;
} _IFC;

static inline long int f2i(float f) {
	_IFC c;
	c.f=f+BIAS_FLOAT;
	return c.i-BIAS_INT;
}

static inline float i2f(long int i) {
	_IFC c;
	c.i=i+BIAS_INT;
	return c.f-BIAS_FLOAT;
}



#define fast_apply_binop(_ta,_a,_tb,_b,_op,_ret,_ret_typ)	do {\
		_IFC _R;\
		switch((((word_t)_ta)<<1)|((word_t)_tb)) {\
		case 0:\
			printf("int int\n");\
			_R.i = (_a) _op (_b);\
			break;\
		case 1:\
			printf("int float\n");\
			_R.f = i2f(_a) _op (_b);\
			break;\
		case 2:\
			printf("float int\n");\
			_R.f = (_a) _op i2f(_b);\
			break;\
		case 3:\
			printf("float float\n");\
			_R.f = (_a) _op (_b);\
		};\
		_ret_typ=(_ta|_tb);\
		_ret=_R.i;\
	} while(0)


#define fast_apply_bin_int_func(_ta,_a,_tb,_b,_op,_ret)	do {\
		_IFC _R;\
		switch((((word_t)_ta)<<1)|((word_t)_tb)) {\
		case 0:\
			_R.i = _op((_a), (_b));\
			break;\
		case 1:\
			_R.i = _op((_a), f2i(_b));\
			break;\
		case 2:\
			_R.i = _op(f2i(_a), (_b));\
			break;\
		case 3:\
			_R.i = _op(f2i(_a), f2i(_b));\
		};\
		_ret=_R.i;\
	} while(0)


#define fast_apply_bin_func(_ta,_a,_tb,_b,_opi,_opf,_ret,_ret_typ)	do {\
		_IFC _R,_X,_Y;\
		switch((((word_t)_ta)<<1)|((word_t)_tb)) {\
		case 0:\
			_R.i = _opi((_a), (_b));\
			break;\
		case 1:\
			_X.i = (_b);\
			_R.f = _opf(i2f(_a), _X.f);\
			break;\
		case 2:\
			_X.i = (_a);\
			_R.f = _opf(_X.f, i2f(_b));\
			break;\
		case 3:\
			_X.i = (_a);\
			_Y.i = (_b);\
			_R.f = _opf(_X.f, _Y.f);\
		};\
		_ret_typ=(_ta|_tb);\
		_ret=_R.i;\
	} while(0)



#endif

