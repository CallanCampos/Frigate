#ifndef MSL_ANSI_FP_H
#define MSL_ANSI_FP_H

// Private to MSL: the decimal form of a floating-point number, which ansi_fp.c converts to and from and which the
// printf and scanf code format and parse. A number is the digits of sig.text (as values 0 to 9 while it is being
// worked on, as characters once __num2dec is done) times a power of ten: exp is the exponent of the first digit.

#define SIGDIGLEN 32

// What decform::style asks for
#define FLOATDECIMAL 0
#define FIXEDDECIMAL 1

typedef struct decimal {
    char sign; // 1 for a negative number
    short exp;
    struct {
        unsigned char length;
        unsigned char text[SIGDIGLEN]; // a lone 'I' or 'N' stands for an infinity or a NaN
        unsigned char unused;
    } sig;
} decimal;

typedef struct decform {
    char style;
    short digits; // the number of digits that __num2dec keeps
} decform;

void __timesdec(decimal *result, const decimal *x, const decimal *y);
void __str2dec(decimal *d, const char *s, short exp);
void __two_exp(decimal *result, int exp);
int __equals_dec(const decimal *x, const decimal *y);
int __less_dec(const decimal *x, const decimal *y);
void __minus_dec(decimal *result, const decimal *x, const decimal *y);
void __ull2dec(decimal *result, unsigned long long val);
void __num2dec_internal(decimal *d, double x);
void __num2dec(const decform *form, double x, decimal *d);
double __dec2num(const decimal *d);

#endif // MSL_ANSI_FP_H
