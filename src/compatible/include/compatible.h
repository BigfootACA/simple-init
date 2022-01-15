/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _COMPATIBLE_H
#define _COMPATIBLE_H
#include<math.h>
#include<float.h>
#include<stddef.h>
#include<stdint.h>

/* Support non-nearest rounding mode.  */
#define WANT_ROUNDING 1
/* Support signaling NaNs.  */
#define WANT_SNAN 0

#define issignalingf_inline(x) 0
#define issignaling_inline(x) 0

#define EXP_TABLE_BITS 7
#define EXP_POLY_ORDER 5
#define EXP_USE_TOINT_NARROW 0
#define EXP2_POLY_ORDER 5
extern const struct exp_data {
	double invln2N;
	double shift;
	double negln2hiN;
	double negln2loN;
	double poly[4];/* Last four coefficients.  */
	double exp2_shift;
	double exp2_poly[EXP2_POLY_ORDER];
	uint64_t tab[2*(1<<EXP_TABLE_BITS)];
} __exp_data;

#define POW_LOG_TABLE_BITS 7
#define POW_LOG_POLY_ORDER 8
extern const struct pow_log_data{
	double ln2hi;
	double ln2lo;
	double poly[POW_LOG_POLY_ORDER-1];/* First coefficient is 1.  */
	/* Note: the pad field is unused,but allows slightly faster indexing.  */
	struct {double invc,pad,logc,logctail;}tab[1<<POW_LOG_TABLE_BITS];
} __pow_log_data;

#ifdef ENABLE_UEFI
extern int efi_status_to_errno(EFI_STATUS st);
#endif
extern char*comp_strdup(const char*s);
extern char*comp_strndup(const char*s,size_t n);
extern size_t comp_strnlen(const char*s,size_t n);
extern float comp__cosdf(double x);
extern float comp__sindf(double x);
extern float comp__tandf(double x,int odd);
extern int comp__rem_pio2f(float x,double*y);
extern int comp__rem_pio2_large(double*x,double*y,int e0,int nx,int prec);
extern float comp_acosf(float x);
extern float comp_atan2f(float y,float x);
extern float comp_ceilf(float x);
extern float comp_cosf(float x);
extern float comp_fabsf(float x);
extern float comp_floorf(float x);
extern float comp_fmodf(float x,float y);
extern float comp_sinf(float x);
extern double comp_sqrt(double x);
extern float comp_sqrtf(float x);
extern float comp_tanf(float x);
extern float comp_atanf(float x);
extern double comp_fabs(double x);
extern double comp_sqrt(double x);
extern double comp_pow(double x,double y);
extern double comp_scalbn(double x,int n);
extern double comp_floor(double x);
#define strdup comp_strdup
#define strndup comp_strndup
#define strnlen comp_strnlen
#define __cosdf comp__cosdf
#define __sindf comp__sindf
#define __tandf comp__tandf
#define __rem_pio2f comp__rem_pio2f
#define __rem_pio2_large comp__rem_pio2_large
#define __math_xflow comp__math_xflow
#define __math_xflowf comp__math_xflowf
#define __math_uflow comp__math_uflow
#define __math_uflowf comp__math_uflowf
#define __math_oflow comp__math_oflow
#define __math_oflowf comp__math_oflowf
#define __math_invalid comp__math_invalid
#define __math_invalidf comp__math_invalidf
#define tanf comp_tanf
#define atanf comp_atanf
#define atan2f comp_atan2f
#define ceilf comp_ceilf
#define acosf comp_acosf
#define cosf comp_cosf
#define fabs comp_fabs
#define fabsf comp_fabsf
#define floor comp_floor
#define floorf comp_floorf
#define fmodf comp_fmodf
#define sinf comp_sinf
#define sqrt comp_sqrt
#define sqrtf comp_sqrtf
#define pow comp_pow
#define scalbn comp_scalbn

#define asuint(f) ((union{float _f;uint32_t _i;}){f})._i
#define asfloat(i) ((union{uint32_t _i;float _f;}){i})._f
#define asuint64(f) ((union{double _f;uint64_t _i;}){f})._i
#define asdouble(i) ((union{uint64_t _i;double _f;}){i})._f
#define GET_FLOAT_WORD(w,d) do{(w)=asuint(d);}while(0)
#define SET_FLOAT_WORD(d,w) do{(d)=asfloat(w);}while(0)

#if !defined(__DEFINED_float_t)
typedef float float_t;
#define __DEFINED_float_t
#endif

#if !defined(__DEFINED_double_t)
typedef double double_t;
#define __DEFINED_double_t
#endif

#define FORCE_EVAL(x) do{\
        if(sizeof(x)==sizeof(float))fp_force_evalf(x);\
        else if(sizeof(x)==sizeof(double))fp_force_eval(x);\
        else fp_force_evall(x);\
}while(0)

#ifndef fp_force_evall
#define fp_force_evall fp_force_evall
static inline void fp_force_evall(long double x){volatile long double y;y=x;(void)y;}
#endif

#ifndef fp_force_evalf
#define fp_force_evalf fp_force_evalf
static inline void fp_force_evalf(float x){volatile float y;y=x;(void)y;}
#endif

#ifndef fp_force_eval
#define fp_force_eval fp_force_eval
static inline void fp_force_eval(double x){volatile double y;y=x;(void)y;}
#endif

#ifndef fp_barrierf
#define fp_barrierf fp_barrierf
static inline float fp_barrierf(float x){volatile float y=x;return y;}
#endif

#ifndef fp_barrier
#define fp_barrier fp_barrier
static inline double fp_barrier(double x){volatile double y=x;return y;}
#endif

#ifndef fp_barrierl
#define fp_barrierl fp_barrierl
static inline long double fp_barrierl(long double x){volatile long double y=x;return y;}
#endif

static inline float eval_as_float(float x){float y=x;return y;}
static inline double eval_as_double(double x){double y=x;return y;}
static inline double comp__math_invalid(double x){return(x-x)/(x-x);}
static inline float comp__math_invalidf(float x){return(x-x)/(x-x);}
static inline double comp__math_xflow(uint32_t sign,double y){return eval_as_double(fp_barrier(sign?-y:y)*y);}
static inline float comp__math_xflowf(uint32_t sign,float y){return eval_as_float(fp_barrierf(sign?-y:y)*y);}
static inline double comp__math_uflow(uint32_t sign){return comp__math_xflow(sign,0x1p-767);}
static inline float comp__math_uflowf(uint32_t sign){return comp__math_xflowf(sign,0x1p-95f);}
static inline float comp__math_oflowf(uint32_t sign){return comp__math_xflowf(sign,0x1p97f);}
static inline double comp__math_oflow(uint32_t sign){return comp__math_xflow(sign,0x1p769);}

#ifndef predict_true
#ifdef __GNUC__
#define predict_true(x) __builtin_expect(!!(x),1)
#define predict_false(x) __builtin_expect(x,0)
#else
#define predict_true(x) (x)
#define predict_false(x) (x)
#endif
#endif

#endif
