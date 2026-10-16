// MSL C's math/k_rem_pio2.c, Sun's fdlibm __kernel_rem_pio2: the remainder of a huge x divided by pi / 2, for
// __ieee754_rem_pio2. x is given as nx pieces of 24 bits (x[0] is the most significant, scaled by 2**e0, the exponent
// of its leading piece minus 23), and the quotient is divided out of 2 / pi, which ipio2 holds in 24-bit pieces.
// The product of x and the pieces of 2 / pi is taken in doubles, which are exact for 24-bit pieces (q), and split
// again into integer pieces (iq): the integer part gives the quotient's low three bits (n, the return value) and the
// fraction gives the remainder after a multiplication by pi / 2, held in as many doubles as prec says: 0 for a 24-bit
// result, 1 or 2 for a double (y[0] + y[1]), 3 for a triple (y[0] + y[1] + y[2]). If the fraction cancels, the loop
// starts over with more pieces of 2 / pi.
// fdlibm's notice is in ../fdlibm.h.
#include "../fdlibm.h"
#include "math.h"

// How many pieces of 2 / pi (minus one) it takes for each precision
static const int init_jk[] = { 2, 3, 4, 6 };

// pi / 2 in 24-bit pieces
static const double PIo2[] = {
    1.57079625129699707031e+00, 7.54978941586159635335e-08, 5.39030252995776476554e-15, 3.28200341580791294123e-22,
    1.27065575308067607349e-29, 1.22933308981111328932e-36, 2.73370053816464559624e-44, 2.16741683877804819444e-51,
};

static const double zero = 0.0;
static const double one = 1.0;
static const double two24 = 1.67772160000000000000e+07;
static const double twon24 = 5.96046447753906250000e-08;

int __kernel_rem_pio2(double *x, double *y, int e0, int nx, int prec, const int *ipio2) {
    int jz, jx, jv, jp, jk, carry, n, iq[20], i, j, k, m, q0, ih;
    double z, fw, f[20], fq[20], q[20];

    // jk + 1 products are taken. jv is the first piece of 2 / pi that matters for x (the one 3 bits below its exponent
    // e0, at least the first), and q0 the scale of what is left above the pieces that the products cover
    jp = jk = init_jk[prec];
    jx = nx - 1;
    jv = (e0 - 3) / 24;
    if (jv < 0) {
        jv = 0;
    }
    q0 = e0 - 24 * (jv + 1);

    // f is 2 / pi in doubles (zero before its start), at the places that the products below read
    j = jv - jx;
    m = jx + jk;
    for (i = 0; i <= m; i++, j++) {
        f[i] = (j < 0) ? zero : (double)ipio2[j];
    }

    // q is the products of x and f, one for each piece of the result
    for (i = 0; i <= jk; i++) {
        for (j = 0, fw = 0.0; j <= jx; j++) {
            fw += x[j] * f[jx + i - j];
        }
        q[i] = fw;
    }

    jz = jk;
recompute:
    // Split q into 24-bit integers iq, from the least significant piece up, with z the part above them
    for (i = 0, j = jz, z = q[jz]; j > 0; i++, j--) {
        fw = (double)((int)(twon24 * z));
        iq[i] = (int)(z - two24 * fw);
        z = q[j - 1] + fw;
    }

    // z is the integer part of the product above the pieces, and what is left of it, scaled, is the fraction
    z = scalbn(z, q0);
    z -= 8.0 * floor(z * 0.125);
    n = (int)z;
    z -= (double)n;
    ih = 0;
    if (q0 > 0) {
        // The integer part also takes bits of the last piece
        i = (iq[jz - 1] >> (24 - q0));
        n += i;
        iq[jz - 1] -= i << (24 - q0);
        ih = iq[jz - 1] >> (23 - q0);
    } else if (q0 == 0) {
        ih = iq[jz - 1] >> 23;
    } else if (z >= 0.5) {
        ih = 2;
    }

    // The fraction is at least 1 / 2: take it from 1 (negate the pieces), so that the result is the remainder
    // within [-pi / 4, pi / 4]
    if (ih > 0) {
        n += 1;
        carry = 0;
        for (i = 0; i < jz; i++) {
            j = iq[i];
            if (carry == 0) {
                if (j != 0) {
                    carry = 1;
                    iq[i] = 0x1000000 - j;
                }
            } else {
                iq[i] = 0xffffff - j;
            }
        }
        if (q0 > 0) {
            switch (q0) {
            case 1:
                iq[jz - 1] &= 0x7fffff;
                break;
            case 2:
                iq[jz - 1] &= 0x3fffff;
                break;
            }
        }
        if (ih == 2) {
            z = one - z;
            if (carry != 0) {
                z -= scalbn(one, q0);
            }
        }
    }

    // The fraction is zero: if the pieces down to jk are zero too, the bits are all gone, so recompute with more
    // pieces of 2 / pi
    if (z == zero) {
        j = 0;
        for (i = jz - 1; i >= jk; i--) {
            j |= iq[i];
        }
        if (j == 0) {
            // k more pieces are needed, as many as the zero ones at the end of iq
            for (k = 1; iq[jk - k] == 0; k++) {
            }
            for (i = jz + 1; i <= jz + k; i++) {
                f[jx + i] = (double)ipio2[jv + i];
                for (j = 0, fw = 0.0; j <= jx; j++) {
                    fw += x[j] * f[jx + i - j];
                }
                q[i] = fw;
            }
            jz += k;
            goto recompute;
        }
    }

    if (z == zero) {
        // Drop the pieces of zero at the end
        jz -= 1;
        q0 -= 24;
        while (iq[jz] == 0) {
            jz--;
            q0 -= 24;
        }
    } else {
        // Break z up into 24-bit pieces if it does not fit one
        z = scalbn(z, -q0);
        if (z >= two24) {
            fw = (double)((int)(twon24 * z));
            iq[jz] = (int)(z - two24 * fw);
            jz += 1;
            q0 += 24;
            iq[jz] = (int)fw;
        } else {
            iq[jz] = (int)z;
        }
    }

    // The pieces back to doubles, each weighted by its power of 2**-24
    fw = scalbn(one, q0);
    for (i = jz; i >= 0; i--) {
        q[i] = fw * (double)iq[i];
        fw *= twon24;
    }

    // fq is the product of the pieces of pi / 2 and q
    for (i = jz; i >= 0; i--) {
        for (fw = 0.0, k = 0; k <= jp && k <= jz - i; k++) {
            fw += PIo2[k] * q[i + k];
        }
        fq[jz - i] = fw;
    }

    // Put fq in the form that prec asks for
    switch (prec) {
    case 0:
        fw = 0.0;
        for (i = jz; i >= 0; i--) {
            fw += fq[i];
        }
        y[0] = (ih == 0) ? fw : -fw;
        break;
    case 1:
    case 2:
        fw = 0.0;
        for (i = jz; i >= 0; i--) {
            fw += fq[i];
        }
        y[0] = (ih == 0) ? fw : -fw;
        fw = fq[0] - fw;
        for (i = 1; i <= jz; i++) {
            fw += fq[i];
        }
        y[1] = (ih == 0) ? fw : -fw;
        break;
    case 3:
        // Add from the end, keeping each sum's rounding error in the next term
        for (i = jz; i > 0; i--) {
            fw = fq[i - 1] + fq[i];
            fq[i] += fq[i - 1] - fw;
            fq[i - 1] = fw;
        }
        for (i = jz; i > 1; i--) {
            fw = fq[i - 1] + fq[i];
            fq[i] += fq[i - 1] - fw;
            fq[i - 1] = fw;
        }
        for (fw = 0.0, i = jz; i >= 2; i--) {
            fw += fq[i];
        }
        if (ih == 0) {
            y[0] = fq[0];
            y[1] = fq[1];
            y[2] = fw;
        } else {
            y[0] = -fq[0];
            y[1] = -fq[1];
            y[2] = -fw;
        }
        break;
    }
    return n & 7;
}
