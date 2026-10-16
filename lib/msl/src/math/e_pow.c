// MSL C's math/e_pow.c, Sun's fdlibm __ieee754_pow: x raised to the power y, with the special cases of the C standard
// and the result of exp(y * log(x)) taken in extra precision: log2(x) is found as a head and a tail (n + the table's
// dp_h and dp_l + the polynomial of the series in (x - 1) / (x + 1)), multiplied by y, and 2 raised to that is
// found by the polynomial of exp.
// fdlibm's notice is in ../fdlibm.h.
#include "../fdlibm.h"
#include <errno.h>
#include <math.h>

static const double bp[] = { 1.0, 1.5 };
static const double dp_h[] = { 0.0, 5.84962487220764160156e-01 }; // log2(1.5), high part
static const double dp_l[] = { 0.0, 1.35003920212974897128e-08 }; // log2(1.5), low part

static const double zero = 0.0;
static const double one = 1.0;
static const double two53 = 9007199254740992.0;
static const double huge = 1.0e300;
static const double tiny = 1.0e-300;

// The coefficients of the series of log((1 + s) / (1 - s)) in s * s
static const double L1 = 5.99999999999994648725e-01;
static const double L2 = 4.28571428578550184252e-01;
static const double L3 = 3.33333329818377432918e-01;
static const double L4 = 2.72728123808534006489e-01;
static const double L5 = 2.30660745775561754067e-01;
static const double L6 = 2.06975017800338417784e-01;

// The coefficients of the polynomial of exp
static const double P1 = 1.66666666666666019037e-01;
static const double P2 = -2.77777777770155933842e-03;
static const double P3 = 6.61375632143793436117e-05;
static const double P4 = -1.65339022054652515390e-06;
static const double P5 = 4.13813679705723846039e-08;

static const double lg2 = 6.93147180559945286227e-01;
static const double lg2_h = 6.93147182464599609375e-01;
static const double lg2_l = -1.90465429995776804525e-09;
static const double ovt = 8.00856625953729410197e-17; // -(1024 - log2(ovfl + .5ulp))
static const double cp = 9.61796693925975554329e-01;  // 2 / (3 * ln(2))
static const double cp_h = 9.61796700954437255859e-01;
static const double cp_l = -7.02846165095275826516e-09;
static const double ivln2 = 1.44269504088896338700e+00; // 1 / ln(2)
static const double ivln2_h = 1.44269502162933349609e+00;
static const double ivln2_l = 1.92596299112661746887e-08;

double __ieee754_pow(double x, double y) {
    double z, ax, z_h, z_l, p_h, p_l;
    double y1, t1, t2, r, s, t, u, v, w;
    double ss, s2, s_h, s_l, t_h, t_l;
    int i, j, k, yisint, n, xpos;
    int hx, hy, ix, iy;
    unsigned lx, ly;

    hx = __HI(x);
    lx = __LO(x);
    hy = __HI(y);
    ly = __LO(y);
    ix = hx & 0x7fffffff;
    iy = hy & 0x7fffffff;

    // x ** 0 is 1
    if ((iy | ly) == 0) {
        return one;
    }

    // A NaN gives a NaN
    if (ix > 0x7ff00000 || ((ix == 0x7ff00000) && (lx != 0)) || iy > 0x7ff00000 || ((iy == 0x7ff00000) && (ly != 0))) {
        return x + y;
    }

    // Whether y is an odd integer (1), an even integer (2) or neither (0), when x is negative
    yisint = 0;
    if (hx < 0) {
        if (iy >= 0x43400000) {
            yisint = 2;
        } else if (iy >= 0x3ff00000) {
            k = (iy >> 20) - 0x3ff;
            if (k > 20) {
                j = ly >> (52 - k);
                if ((j << (52 - k)) == ly) {
                    yisint = 2 - (j & 1);
                }
            } else if (ly == 0) {
                j = iy >> (20 - k);
                if ((j << (20 - k)) == iy) {
                    yisint = 2 - (j & 1);
                }
            }
        }
    }

    // The special values of y
    if (ly == 0) {
        if (iy == 0x7ff00000) {
            // y is an infinity
            if (((ix - 0x3ff00000) | lx) == 0) {
                return y - y;
            } else if (ix >= 0x3ff00000) {
                return (hy >= 0) ? y : zero;
            } else {
                return (hy < 0) ? -y : zero;
            }
        }
        if (iy == 0x3ff00000) {
            // y is 1 or -1
            if (hy < 0) {
                return one / x;
            } else {
                return x;
            }
        }
        if (hy == 0x40000000) {
            // y is 2
            return x * x;
        }
        if (hy == 0x3fe00000) {
            // y is 1/2
            if (hx >= 0) {
                return sqrt(x);
            }
        }
    }

    ax = fabs(x);

    // The special values of x: 0, 1 and the infinity, with either sign
    if (lx == 0) {
        if (ix == 0x7ff00000 || ix == 0 || ix == 0x3ff00000) {
            z = ax;
            if (hy < 0) {
                z = one / z;
            }
            if (hx < 0) {
                if (((ix - 0x3ff00000) | yisint) == 0) {
                    z = (z - z) / (z - z);
                } else if (yisint == 1) {
                    z = -z;
                }
            }
            return z;
        }
    }

    // 1 if x is positive or 0 (its sign bit is clear), else 0
    xpos = (hx >> 31) + 1;

    // A negative number to a power that is not an integer is a domain error
    if ((xpos | yisint) == 0) {
        errno = EDOM;
        return NAN;
    }

#ifdef BUGFIX
    s = one;
    if ((xpos | (yisint - 1)) == 0) {
        s = -one;
    }
#endif

    if (iy > 0x41e00000) {
        // |y| is over 2 ** 31
        if (iy > 0x43f00000) {
            // |y| is over 2 ** 64, so the result over- or underflows unless x is a 1
            if (ix <= 0x3fefffff) {
                return (hy < 0) ? huge * huge : tiny * tiny;
            }
            if (ix >= 0x3ff00000) {
                return (hy > 0) ? huge * huge : tiny * tiny;
            }
        }
        // Over- or underflow if x is not close to 1
        // BUG: the sign of the result is not applied, so a negative x to an odd power over 2 ** 31 gives +inf or +0
        // where -inf or -0 is due. fdlibm's returns multiply by the sign
#ifdef BUGFIX
        if (ix < 0x3fefffff) {
            return (hy < 0) ? s * huge * huge : s * tiny * tiny;
        }
        if (ix > 0x3ff00000) {
            return (hy > 0) ? s * huge * huge : s * tiny * tiny;
        }
#else
        if (ix < 0x3fefffff) {
            return (hy < 0) ? huge * huge : tiny * tiny;
        }
        if (ix > 0x3ff00000) {
            return (hy > 0) ? huge * huge : tiny * tiny;
        }
#endif

        // |1 - x| is at most 2 ** -20 here, so log(x) is the series x - x ** 2 / 2 + x ** 3 / 3 - x ** 4 / 4
        // BUG: x is used where |x| is due, so a negative x near -1 gets t near -2 and a meaningless logarithm
#ifdef BUGFIX
        t = ax - one;
#else
        t = x - one;
#endif
        w = (t * t) * (0.5 - t * (0.3333333333333333333333 - t * 0.25));
        u = ivln2_h * t;
        v = t * ivln2_l - w * ivln2;
        t1 = u + v;
        __LO(t1) = 0;
        t2 = v - (t1 - u);
    } else {
        n = 0;
        // A subnormal x is scaled up first
        if (ix < 0x00100000) {
            ax *= two53;
            n -= 53;
            ix = __HI(ax);
        }
        n += (ix >> 20) - 0x3ff;
        j = ix & 0x000fffff;
        // Where x lies: below sqrt(3/2) (k is 0), below sqrt(3) (1), or above (0, with n one more)
        ix = j | 0x3ff00000;
        if (j <= 0x3988e) {
            k = 0;
        } else if (j < 0xbb67a) {
            k = 1;
        } else {
            k = 0;
            n += 1;
            ix -= 0x00100000;
        }
        __HI(ax) = ix;

        // ss = s_h + s_l is (x - 1) / (x + 1) or (x - 1.5) / (x + 1.5)
        u = ax - bp[k];
        v = one / (ax + bp[k]);
        ss = u * v;
        s_h = ss;
        __LO(s_h) = 0;
        // t_h is ax + bp[k], high part
        t_h = zero;
        __HI(t_h) = ((ix >> 1) | 0x20000000) + 0x00080000 + (k << 18);
        t_l = ax - (t_h - bp[k]);
        s_l = v * ((u - s_h * t_h) - s_h * t_l);

        // log(ax)
        s2 = ss * ss;
        r = s2 * s2 * (L1 + s2 * (L2 + s2 * (L3 + s2 * (L4 + s2 * (L5 + s2 * L6)))));
        r += s_l * (s_h + ss);
        s2 = s_h * s_h;
        t_h = 3.0 + s2 + r;
        __LO(t_h) = 0;
        t_l = r - ((t_h - 3.0) - s2);
        // u + v is ss * (1 + ...)
        u = s_h * t_h;
        v = s_l * t_h + t_l * ss;
        // p_h + p_l is 2 / (3 * ln(2)) * (ss + ...)
        p_h = u + v;
        __LO(p_h) = 0;
        p_l = v - (p_h - u);
        z_h = cp_h * p_h;
        z_l = cp_l * p_h + p_l * cp + dp_l[k];
        // log2(ax) is n + dp_h + z_h + z_l
        t = (double)n;
        t1 = (((z_h + z_l) + dp_h[k]) + t);
        __LO(t1) = 0;
        t2 = z_l - (((t1 - t) - dp_h[k]) - z_h);
    }

#ifndef BUGFIX
    // The sign of the result is -1 for a negative number to an odd power
    s = one;
    if ((xpos | (yisint - 1)) == 0) {
        s = -one;
    }
#endif

    // y * log2(x) = (y1 + y2) * (t1 + t2), as p_h + p_l
    y1 = y;
    __LO(y1) = 0;
    p_l = (y - y1) * t1 + y * t2;
    p_h = y1 * t1;
    z = p_l + p_h;
    j = __HI(z);
    i = __LO(z);
    if (j >= 0x40900000) {
        // z is 1024 or more
        if (((j - 0x40900000) | i) != 0) {
            return s * huge * huge;
        } else {
            if (p_l + ovt > z - p_h) {
                return s * huge * huge;
            }
        }
    } else if ((j & 0x7fffffff) >= 0x4090cc00) {
        // z is -1075 or less
        if (((j - 0xc090cc00) | i) != 0) {
            return s * tiny * tiny;
        } else {
            if (p_l <= z - p_h) {
                return s * tiny * tiny;
            }
        }
    }

    // 2 ** z, as 2 ** n * exp(r) for the integer n nearest z and the rest r
    i = j & 0x7fffffff;
    k = (i >> 20) - 0x3ff;
    n = 0;
    if (i > 0x3fe00000) {
        n = j + (0x00100000 >> (k + 1));
        k = ((n & 0x7fffffff) >> 20) - 0x3ff;
        t = zero;
        __HI(t) = n & ~(0x000fffff >> k);
        n = ((n & 0x000fffff) | 0x00100000) >> (20 - k);
        if (j < 0) {
            n = -n;
        }
        p_h -= t;
    }
    t = p_l + p_h;
    __LO(t) = 0;
    u = t * lg2_h;
    v = (p_l - (t - p_h)) * lg2 + t * lg2_l;
    z = u + v;
    w = v - (z - u);
    t = z * z;
    t1 = z - t * (P1 + t * (P2 + t * (P3 + t * (P4 + t * P5))));
    r = (z * t1) / (t1 - 2.0) - (w + z * w);
    z = one - (r - z);
    j = __HI(z);
    j += (n << 20);
    if ((j >> 20) <= 0) {
        z = scalbn(z, n);
    } else {
        __HI(z) += (n << 20);
    }
    return s * z;
}
