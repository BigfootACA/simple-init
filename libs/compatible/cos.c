/*
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */
/* origin: FreeBSD /usr/src/lib/msun/src/s_cos.c */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */
/* cos(x)
 * Return cosine function of x.
 *
 * kernel function:
 *      __sin           ... sine function on [-pi/4,pi/4]
 *      __cos           ... cosine function on [-pi/4,pi/4]
 *      __rem_pio2      ... argument reduction routine
 *
 * Method.
 *      Let S,C and T denote the sin, cos and tan respectively on
 *      [-PI/4, +PI/4]. Reduce the argument x to y1+y2 = x-k*pi/2
 *      in [-pi/4 , +pi/4], and let n = k mod 4.
 *      We have
 *
 *          n        sin(x)      cos(x)        tan(x)
 *     ----------------------------------------------------------
 *          0          S           C             T
 *          1          C          -S            -1/T
 *          2         -S          -C             T
 *          3         -C           S            -1/T
 *     ----------------------------------------------------------
 *
 * Special cases:
 *      Let trig be any of sin, cos, or tan.
 *      trig(+-INF)  is NaN, with signals;
 *      trig(NaN)    is that NaN;
 *
 * Accuracy:
 *      TRIG(x) returns trig(x) nearly rounded
 */
/* origin: FreeBSD /usr/src/lib/msun/src/k_cos.c */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */
/*
 * __cos( x,  y )
 * kernel cos function on [-pi/4, pi/4], pi/4 ~ 0.785398164
 * Input x is assumed to be bounded by ~pi/4 in magnitude.
 * Input y is the tail of x.
 *
 * Algorithm
 *      1. Since cos(-x) = cos(x), we need only to consider positive x.
 *      2. if x < 2^-27 (hx<0x3e400000 0), return 1 with inexact if x!=0.
 *      3. cos(x) is approximated by a polynomial of degree 14 on
 *         [0,pi/4]
 *                                       4            14
 *              cos(x) ~ 1 - x*x/2 + C1*x + ... + C6*x
 *         where the remez error is
 *
 *      |              2     4     6     8     10    12     14 |     -58
 *      |cos(x)-(1-.5*x +C1*x +C2*x +C3*x +C4*x +C5*x  +C6*x  )| <= 2
 *      |                                                      |
 *
 *                     4     6     8     10    12     14
 *      4. let r = C1*x +C2*x +C3*x +C4*x +C5*x  +C6*x  , then
 *             cos(x) ~ 1 - x*x/2 + r
 *         since cos(x+y) ~ cos(x) - sin(x)*y
 *                        ~ cos(x) - x*y,
 *         a correction term is necessary in cos(x) and hence
 *              cos(x+y) = 1 - (x*x/2 - (r - x*y))
 *         For better accuracy, rearrange to
 *              cos(x+y) ~ w + (tmp + (r-x*y))
 *         where w = 1 - x*x/2 and tmp is a tiny correction term
 *         (1 - x*x/2 == w + tmp exactly in infinite precision).
 *         The exactness of w + tmp in infinite precision depends on w
 *         and tmp having the same precision as x.  If they have extra
 *         precision due to compiler bugs, then the extra precision is
 *         only good provided it is retained in all terms of the final
 *         expression for cos().  Retention happens in all cases tested
 *         under FreeBSD, so don't pessimize things by forcibly clipping
 *         any extra precision in w.
 */

#include "math.h"
#include "limits.h"

static const float
pio2_hi = 1.5707962513e+00, /* 0x3fc90fda */
pio2_lo = 7.5497894159e-08, /* 0x33a22168 */
pS0 =  1.6666586697e-01,
pS1 = -4.2743422091e-02,
pS2 = -8.6563630030e-03,
qS1 = -7.0662963390e-01;
static const double
/* Small multiples of pi/2 rounded to double precision. */
c1pio2 = 1*M_PI_2, /* 0x3FF921FB, 0x54442D18 */
c2pio2 = 2*M_PI_2, /* 0x400921FB, 0x54442D18 */
c3pio2 = 3*M_PI_2, /* 0x4012D97C, 0x7F3321D2 */
c4pio2 = 4*M_PI_2; /* 0x401921FB, 0x54442D18 */

/* |cos(x) - c(x)| < 2**-34.1 (~[-5.37e-11, 5.295e-11]). */
static const double
C0  = -0x1ffffffd0c5e81.0p-54, /* -0.499999997251031003120 */
C1  =  0x155553e1053a42.0p-57, /*  0.0416666233237390631894 */
C2  = -0x16c087e80f1e27.0p-62, /* -0.00138867637746099294692 */
C3  =  0x199342e0ee5069.0p-68; /*  0.0000243904487962774090654 */

static const double
_C1  =  4.16666666666666019037e-02, /* 0x3FA55555, 0x5555554C */
_C2  = -1.38888888888741095749e-03, /* 0xBF56C16C, 0x16C15177 */
_C3  =  2.48015872894767294178e-05, /* 0x3EFA01A0, 0x19CB1590 */
_C4  = -2.75573143513906633035e-07, /* 0xBE927E4F, 0x809C52AD */
_C5  =  2.08757232129817482790e-09, /* 0x3E21EE9E, 0xBDB4B1C4 */
_C6  = -1.13596475577881948265e-11; /* 0xBDA8FAE9, 0xBE8838D4 */

static float R(float z)
{
	float_t p, q;
	p = z*(pS0+z*(pS1+z*pS2));
	q = 1.0f+z*qS1;
	return p/q;
}

weak_decl float __cosdf(double x)
{
	double_t r, w, z;

	/* Try to optimize for parallel evaluation as in __tandf.c. */
	z = x*x;
	w = z*z;
	r = C2+z*C3;
	return ((1.0+z*C0) + w*C1) + (w*z)*r;
}

weak_decl float acosf(float x)
{
	float z,w,s,c,df;
	uint32_t hx,ix;

	GET_FLOAT_WORD(hx, x);
	ix = hx & 0x7fffffff;
	/* |x| >= 1 or nan */
	if (ix >= 0x3f800000) {
		if (ix == 0x3f800000) {
			if (hx >> 31)
				return 2*pio2_hi + 0x1p-120f;
			return 0;
		}
		return 0/(x-x);
	}
	/* |x| < 0.5 */
	if (ix < 0x3f000000) {
		if (ix <= 0x32800000) /* |x| < 2**-26 */
			return pio2_hi + 0x1p-120f;
		return pio2_hi - (x - (pio2_lo-x*R(x*x)));
	}
	/* x < -0.5 */
	if (hx >> 31) {
		z = (1+x)*0.5f;
		s = sqrtf(z);
		w = R(z)*s-pio2_lo;
		return 2*(pio2_hi - (s+w));
	}
	/* x > 0.5 */
	z = (1-x)*0.5f;
	s = sqrtf(z);
	GET_FLOAT_WORD(hx,s);
	SET_FLOAT_WORD(df,hx&0xfffff000);
	c = (z-df*df)/(s+df);
	w = R(z)*s+c;
	return 2*(df+w);
}

weak_decl float cosf(float x)
{
	double y;
	uint32_t ix;
	unsigned n, sign;

	GET_FLOAT_WORD(ix, x);
	sign = ix >> 31;
	ix &= 0x7fffffff;

	if (ix <= 0x3f490fda) {  /* |x| ~<= pi/4 */
		if (ix < 0x39800000) {  /* |x| < 2**-12 */
			/* raise inexact if x != 0 */
			FORCE_EVAL(x + 0x1p120f);
			return 1.0f;
		}
		return __cosdf(x);
	}
	if (ix <= 0x407b53d1) {  /* |x| ~<= 5*pi/4 */
		if (ix > 0x4016cbe3)  /* |x|  ~> 3*pi/4 */
			return -__cosdf(sign ? x+c2pio2 : x-c2pio2);
		else {
			if (sign)
				return __sindf(x + c1pio2);
			else
				return __sindf(c1pio2 - x);
		}
	}
	if (ix <= 0x40e231d5) {  /* |x| ~<= 9*pi/4 */
		if (ix > 0x40afeddf)  /* |x| ~> 7*pi/4 */
			return __cosdf(sign ? x+c4pio2 : x-c4pio2);
		else {
			if (sign)
				return __sindf(-x - c3pio2);
			else
				return __sindf(x - c3pio2);
		}
	}

	/* cos(Inf or NaN) is NaN */
	if (ix >= 0x7f800000)
		return x-x;

	/* general argument reduction needed */
	n = __rem_pio2f(x,&y);
	switch (n&3) {
		case 0: return  __cosdf(y);
		case 1: return  __sindf(-y);
		case 2: return -__cosdf(y);
		default:
			return  __sindf(y);
	}
}

weak_decl double cos(double x)
{
	double y[2];
	uint32_t ix;
	unsigned n;

	GET_HIGH_WORD(ix, x);
	ix &= 0x7fffffff;

	/* |x| ~< pi/4 */
	if (ix <= 0x3fe921fb) {
		if (ix < 0x3e46a09e) {  /* |x| < 2**-27 * sqrt(2) */
			/* raise inexact if x!=0 */
			FORCE_EVAL(x + 0x1p120f);
			return 1.0;
		}
		return __cos(x, 0);
	}

	/* cos(Inf or NaN) is NaN */
	if (ix >= 0x7ff00000)
		return x-x;

	/* argument reduction */
	n = __rem_pio2(x, y);
	switch (n&3) {
	case 0: return  __cos(y[0], y[1]);
	case 1: return -__sin(y[0], y[1], 1);
	case 2: return -__cos(y[0], y[1]);
	default:
		return  __sin(y[0], y[1], 1);
	}
}

/* cosh(x) = (exp(x) + 1/exp(x))/2
 *         = 1 + 0.5*(exp(x)-1)*(exp(x)-1)/exp(x)
 *         = 1 + x*x/2 + o(x^4)
 */
weak_decl double cosh(double x)
{
	union {double f; uint64_t i;} u = {.f = x};
	uint32_t w;
	double t;

	/* |x| */
	u.i &= (uint64_t)-1/2;
	x = u.f;
	w = u.i >> 32;

	/* |x| < log(2) */
	if (w < 0x3fe62e42) {
		if (w < 0x3ff00000 - (26<<20)) {
			/* raise inexact if x!=0 */
			FORCE_EVAL(x + 0x1p120f);
			return 1;
		}
		t = expm1(x);
		return 1 + t*t/(2*(1+t));
	}

	/* |x| < log(DBL_MAX) */
	if (w < 0x40862e42) {
		t = exp(x);
		/* note: if x>log(0x1p26) then the 1/t is not needed */
		return 0.5*(t + 1/t);
	}

	/* |x| > log(DBL_MAX) or nan */
	/* note: the result is stored to handle overflow */
	t = __expo2(x, 1.0);
	return t;
}

weak_decl double __cos(double x, double y)
{
	double_t hz,z,r,w;

	z  = x*x;
	w  = z*z;
	r  = z*(_C1+z*(_C2+z*_C3)) + w*w*(_C4+z*(_C5+z*_C6));
	hz = 0.5*z;
	w  = 1.0-hz;
	return w + (((1.0-w)-hz) + (z*r-x*y));
}
