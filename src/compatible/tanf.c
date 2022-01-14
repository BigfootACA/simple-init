/*
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
 * Optimized by Bruce D. Evans.
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

#include "compatible.h"

/* Small multiples of pi/2 rounded to double precision. */
static const double
t1pio2 = 1*M_PI_2, /* 0x3FF921FB, 0x54442D18 */
t2pio2 = 2*M_PI_2, /* 0x400921FB, 0x54442D18 */
t3pio2 = 3*M_PI_2, /* 0x4012D97C, 0x7F3321D2 */
t4pio2 = 4*M_PI_2; /* 0x401921FB, 0x54442D18 */

static const float atanhi[] = {
	4.6364760399e-01, /* atan(0.5)hi 0x3eed6338 */
	7.8539812565e-01, /* atan(1.0)hi 0x3f490fda */
	9.8279368877e-01, /* atan(1.5)hi 0x3f7b985e */
	1.5707962513e+00, /* atan(inf)hi 0x3fc90fda */
};

static const float atanlo[] = {
	5.0121582440e-09, /* atan(0.5)lo 0x31ac3769 */
	3.7748947079e-08, /* atan(1.0)lo 0x33222168 */
	3.4473217170e-08, /* atan(1.5)lo 0x33140fb4 */
	7.5497894159e-08, /* atan(inf)lo 0x33a22168 */
};

static const float aT[] = {
	3.3333328366e-01,
	-1.9999158382e-01,
	1.4253635705e-01,
	-1.0648017377e-01,
	6.1687607318e-02,
};

static const float
pi     = 3.1415927410e+00, /* 0x40490fdb */
pi_lo  = -8.7422776573e-08; /* 0xb3bbbd2e */

/* |tan(x)/x - t(x)| < 2**-25.5 (~[-2e-08, 2e-08]). */
static const double T[] = {
	0x15554d3418c99f.0p-54, /* 0.333331395030791399758 */
	0x1112fd38999f72.0p-55, /* 0.133392002712976742718 */
	0x1b54c91d865afe.0p-57, /* 0.0533812378445670393523 */
	0x191df3908c33ce.0p-58, /* 0.0245283181166547278873 */
	0x185dadfcecf44e.0p-61, /* 0.00297435743359967304927 */
	0x1362b9bf971bcd.0p-59, /* 0.00946564784943673166728 */
};

float comp__tandf(double x, int odd)
{
	double_t z,r,w,s,t,u;

	z = x*x;
	/*
	 * Split up the polynomial into small independent terms to give
	 * opportunities for parallel evaluation.  The chosen splitting is
	 * micro-optimized for Athlons (XP, X64).  It costs 2 multiplications
	 * relative to Horner's method on sequential machines.
	 *
	 * We add the small terms from lowest degree up for efficiency on
	 * non-sequential machines (the lowest degree terms tend to be ready
	 * earlier).  Apart from this, we don't care about order of
	 * operations, and don't need to to care since we have precision to
	 * spare.  However, the chosen splitting is good for accuracy too,
	 * and would give results as accurate as Horner's method if the
	 * small terms were added from highest degree down.
	 */
	r = T[4] + z*T[5];
	t = T[2] + z*T[3];
	w = z*z;
	s = z*x;
	u = T[0] + z*T[1];
	r = (x + s*u) + (s*w)*(t + w*r);
	return odd ? -1.0/r : r;
}

float comp_atanf(float x)
{
	float_t w,s1,s2,z;
	uint32_t ix,sign;
	int id;

	GET_FLOAT_WORD(ix, x);
	sign = ix>>31;
	ix &= 0x7fffffff;
	if (ix >= 0x4c800000) {  /* if |x| >= 2**26 */
		if (isnan(x))
			return x;
		z = atanhi[3] + 0x1p-120f;
		return sign ? -z : z;
	}
	if (ix < 0x3ee00000) {   /* |x| < 0.4375 */
		if (ix < 0x39800000) {  /* |x| < 2**-12 */
			if (ix < 0x00800000)
				/* raise underflow for subnormal x */
				FORCE_EVAL(x*x);
			return x;
		}
		id = -1;
	} else {
		x = fabsf(x);
		if (ix < 0x3f980000) {  /* |x| < 1.1875 */
			if (ix < 0x3f300000) {  /*  7/16 <= |x| < 11/16 */
				id = 0;
				x = (2.0f*x - 1.0f)/(2.0f + x);
			} else {                /* 11/16 <= |x| < 19/16 */
				id = 1;
				x = (x - 1.0f)/(x + 1.0f);
			}
		} else {
			if (ix < 0x401c0000) {  /* |x| < 2.4375 */
				id = 2;
				x = (x - 1.5f)/(1.0f + 1.5f*x);
			} else {                /* 2.4375 <= |x| < 2**26 */
				id = 3;
				x = -1.0f/x;
			}
		}
	}
	/* end of argument reduction */
	z = x*x;
	w = z*z;
	/* break sum from i=0 to 10 aT[i]z**(i+1) into odd and even poly */
	s1 = z*(aT[0]+w*(aT[2]+w*aT[4]));
	s2 = w*(aT[1]+w*aT[3]);
	if (id < 0)
		return x - x*(s1+s2);
	z = atanhi[id] - ((x*(s1+s2) - atanlo[id]) - x);
	return sign ? -z : z;
}

float comp_atan2f(float y, float x)
{
	float z;
	uint32_t m,ix,iy;

	if (isnan(x) || isnan(y))
		return x+y;
	GET_FLOAT_WORD(ix, x);
	GET_FLOAT_WORD(iy, y);
	if (ix == 0x3f800000)  /* x=1.0 */
		return atanf(y);
	m = ((iy>>31)&1) | ((ix>>30)&2);  /* 2*sign(x)+sign(y) */
	ix &= 0x7fffffff;
	iy &= 0x7fffffff;

	/* when y = 0 */
	if (iy == 0) {
		switch (m) {
			case 0:
			case 1: return y;   /* atan(+-0,+anything)=+-0 */
			case 2: return  pi; /* atan(+0,-anything) = pi */
			case 3: return -pi; /* atan(-0,-anything) =-pi */
		}
	}
	/* when x = 0 */
	if (ix == 0)
		return m&1 ? -pi/2 : pi/2;
	/* when x is INF */
	if (ix == 0x7f800000) {
		if (iy == 0x7f800000) {
			switch (m) {
				case 0: return  pi/4; /* atan(+INF,+INF) */
				case 1: return -pi/4; /* atan(-INF,+INF) */
				case 2: return 3*pi/4;  /*atan(+INF,-INF)*/
				case 3: return -3*pi/4; /*atan(-INF,-INF)*/
			}
		} else {
			switch (m) {
				case 0: return  0.0f;    /* atan(+...,+INF) */
				case 1: return -0.0f;    /* atan(-...,+INF) */
				case 2: return  pi; /* atan(+...,-INF) */
				case 3: return -pi; /* atan(-...,-INF) */
			}
		}
	}
	/* |y/x| > 0x1p26 */
	if (ix+(26<<23) < iy || iy == 0x7f800000)
		return m&1 ? -pi/2 : pi/2;

	/* z = atan(|y/x|) with correct underflow */
	if ((m&2) && iy+(26<<23) < ix)  /*|y/x| < 0x1p-26, x < 0 */
		z = 0.0;
	else
		z = atanf(fabsf(y/x));
	switch (m) {
		case 0: return z;              /* atan(+,+) */
		case 1: return -z;             /* atan(-,+) */
		case 2: return pi - (z-pi_lo); /* atan(+,-) */
		default: /* case 3 */
			return (z-pi_lo) - pi; /* atan(-,-) */
	}
}

float comp_tanf(float x)
{
	double y;
	uint32_t ix;
	unsigned n, sign;

	GET_FLOAT_WORD(ix, x);
	sign = ix >> 31;
	ix &= 0x7fffffff;

	if (ix <= 0x3f490fda) {  /* |x| ~<= pi/4 */
		if (ix < 0x39800000) {  /* |x| < 2**-12 */
			/* raise inexact if x!=0 and underflow if subnormal */
			FORCE_EVAL(ix < 0x00800000 ? x/0x1p120f : x+0x1p120f);
			return x;
		}
		return __tandf(x, 0);
	}
	if (ix <= 0x407b53d1) {  /* |x| ~<= 5*pi/4 */
		if (ix <= 0x4016cbe3)  /* |x| ~<= 3pi/4 */
			return __tandf((sign ? x+t1pio2 : x-t1pio2), 1);
		else
			return __tandf((sign ? x+t2pio2 : x-t2pio2), 0);
	}
	if (ix <= 0x40e231d5) {  /* |x| ~<= 9*pi/4 */
		if (ix <= 0x40afeddf)  /* |x| ~<= 7*pi/4 */
			return __tandf((sign ? x+t3pio2 : x-t3pio2), 1);
		else
			return __tandf((sign ? x+t4pio2 : x-t4pio2), 0);
	}

	/* tan(Inf or NaN) is NaN */
	if (ix >= 0x7f800000)
		return x - x;

	/* argument reduction */
	n = __rem_pio2f(x, &y);
	return __tandf(y, n&1);
}
