// MSL C's math/e_rem_pio2.c, Sun's fdlibm __ieee754_rem_pio2: the remainder of x divided by pi / 2, as the quotient's
// low bits n (the return value) and the remainder y[0] + y[1] (a head and a tail) in [-pi / 4, pi / 4]. It is done in
// one of three ways by the size of x: x within pi / 4 is its own remainder; x up to 2**19 * pi / 2 takes n from a
// multiplication by 2 / pi and subtracts n * pi / 2 held in up to three pieces (pio2_1 to pio2_3, with their tails)
// until enough bits are left; a larger x is split into 24-bit pieces for __kernel_rem_pio2, which divides it using
// the bits of 2 / pi.
// fdlibm's notice is in ../fdlibm.h.
#include "../fdlibm.h"
#include "math.h"

// The high word of n * pi / 2 for n = 1 to 32, to tell when a subtraction of n * pi / 2 cancels most of its bits
static const int npio2_hw[] = {
    0x3ff921fb, 0x400921fb, 0x4012d97c, 0x401921fb, 0x401f6a7a, 0x4022d97c, 0x4025fdbb, 0x402921fb,
    0x402c463a, 0x402f6a7a, 0x4031475c, 0x4032d97c, 0x40346b9c, 0x4035fdbb, 0x40378fdb, 0x403921fb,
    0x403ab41b, 0x403c463a, 0x403dd85a, 0x403f6a7a, 0x40407e4c, 0x4041475c, 0x4042106c, 0x4042d97c,
    0x4043a28c, 0x40446b9c, 0x404534ac, 0x4045fdbb, 0x4046c6cb, 0x40478fdb, 0x404858eb, 0x404921fb,
};

// 2 / pi in 24-bit pieces, 66 of them
static const int two_over_pi[] = {
    0xa2f983, 0x6e4e44, 0x1529fc, 0x2757d1, 0xf534dd, 0xc0db62, 0x95993c, 0x439041, 0xfe5163, 0xabdebb, 0xc561b7,
    0x246e3a, 0x424dd2, 0xe00649, 0x2eea09, 0xd1921c, 0xfe1deb, 0x1cb129, 0xa73ee8, 0x8235f5, 0x2ebb44, 0x84e99c,
    0x7026b4, 0x5f7e41, 0x3991d6, 0x398353, 0x39f49c, 0x845f8b, 0xbdf928, 0x3b1ff8, 0x97ffde, 0x05980f, 0xef2f11,
    0x8b5a0a, 0x6d1f6d, 0x367ecf, 0x27cb09, 0xb74f46, 0x3f669e, 0x5fea2d, 0x7527ba, 0xc7ebe5, 0xf17b3d, 0x0739f7,
    0x8a5292, 0xea6bfb, 0x5fb11f, 0x8d5d08, 0x560330, 0x46fc7b, 0x6babf0, 0xcfbc20, 0x9af436, 0x1da9e3, 0x91615e,
    0xe61b08, 0x659985, 0x5f14a0, 0x68408d, 0xffd880, 0x4d7327, 0x310606, 0x1556ca, 0x73a8c9, 0x60e27b, 0xc08c6b,
};

static const double zero = 0.0;
static const double half = 5.00000000000000000000e-01;
static const double two24 = 1.67772160000000000000e+07;
static const double invpio2 = 6.36619772367581382433e-01; // 2 / pi
// pi / 2 in three pieces of 33 bits each, and the tails that follow each of them
static const double pio2_1 = 1.57079632673412561417e+00;
static const double pio2_1t = 6.07710050650619224932e-11;
static const double pio2_2 = 6.07710050630396597660e-11;
static const double pio2_2t = 2.02226624879595063154e-21;
static const double pio2_3 = 2.02226624871116645580e-21;
static const double pio2_3t = 8.47842766036889956997e-32;

int __ieee754_rem_pio2(double x, double *y) {
    double z, w, t, r, fn;
    double tx[3];
    int e0, i, j, nx, n, ix, hx;

    hx = __HI(x);
    ix = hx & 0x7fffffff;

    // x is within pi / 4, so it is its own remainder
    if (ix <= 0x3fe921fb) {
        y[0] = x;
        y[1] = 0;
        return 0;
    }

    // x is within 3 * pi / 4, so n is 1 or -1
    if (ix < 0x4002d97c) {
        if (hx > 0) {
            z = x - pio2_1;
            if (ix != 0x3ff921fb) {
                // pio2_1 does not make up most of the bits of x
                y[0] = z - pio2_1t;
                y[1] = (z - y[0]) - pio2_1t;
            } else {
                // x is close to pi / 2: one more piece
                z -= pio2_2;
                y[0] = z - pio2_2t;
                y[1] = (z - y[0]) - pio2_2t;
            }
            return 1;
        } else {
            z = x + pio2_1;
            if (ix != 0x3ff921fb) {
                y[0] = z + pio2_1t;
                y[1] = (z - y[0]) + pio2_1t;
            } else {
                z += pio2_2;
                y[0] = z + pio2_2t;
                y[1] = (z - y[0]) + pio2_2t;
            }
            return -1;
        }
    }

    // x is up to 2**19 * pi / 2
    if (ix <= 0x413921fb) {
        t = fabs(x);
        n = (int)(t * invpio2 + half);
        fn = (double)n;
        r = t - fn * pio2_1;
        w = fn * pio2_1t;
        if (n < 32 && ix != npio2_hw[n - 1]) {
            // The first piece leaves enough bits
            y[0] = r - w;
        } else {
            // Take more pieces while the subtraction cancels the leading bits
            j = ix >> 20;
            y[0] = r - w;
            i = j - ((__HI(y[0]) >> 20) & 0x7ff);
            if (i > 16) {
                t = r;
                w = fn * pio2_2;
                r = t - w;
                w = fn * pio2_2t - ((t - r) - w);
                y[0] = r - w;
                i = j - ((__HI(y[0]) >> 20) & 0x7ff);
                if (i > 49) {
                    t = r;
                    w = fn * pio2_3;
                    r = t - w;
                    w = fn * pio2_3t - ((t - r) - w);
                    y[0] = r - w;
                }
            }
        }
        y[1] = (r - y[0]) - w;
        if (hx < 0) {
            y[0] = -y[0];
            y[1] = -y[1];
            return -n;
        }
        return n;
    }

    // x is an infinity or a NaN
    if (ix >= 0x7ff00000) {
        y[0] = y[1] = x - x;
        return 0;
    }

    // x is huge: z is |x| scaled down to a 24-bit exponent range, split in three 24-bit pieces
    __LO(z) = __LO(x);
    e0 = (ix >> 20) - 1046;
    __HI(z) = ix - (e0 << 20);
    for (i = 0; i < 2; i++) {
        tx[i] = (double)((int)z);
        z = (z - tx[i]) * two24;
    }
    tx[2] = z;
    nx = 3;
    while (tx[nx - 1] == zero) {
        nx--;
    }
    n = __kernel_rem_pio2(tx, y, e0, nx, 2, two_over_pi);
    if (hx < 0) {
        y[0] = -y[0];
        y[1] = -y[1];
        return -n;
    }
    return n;
}
