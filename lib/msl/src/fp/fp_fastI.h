#ifndef POKEBW2_MSL_RUNTIME_FP_FASTI_H
#define POKEBW2_MSL_RUNTIME_FP_FASTI_H

// The routines of fp/fp_fastI.s, MSL's floating-point and integer runtime, which MWCC's code calls for arithmetic that
// the processor has no instruction for. The names are the runtime's own, which the compiler's calls use and pret's
// pokediamond labels; the comments give swan's name where it differs.

// Double arithmetic
double _dadd(double a, double b);  // swan's __aeabi_dadd
double _dsub(double a, double b);  // swan's doubleSub
double _drsb(double a, double b);  // b - a
double _dmul(double a, double b);  // swan's __aeabi_dmul
double _ddiv(double a, double b);  // swan's __aeabi_ddiv
double _drdiv(double a, double b); // b / a

// Double comparison, which leaves its answer in the flags as well
int _dgeq(double a, double b); // swan's __aeabi_dcmpge
int _dgr(double a, double b);  // swan's __aeabi_dcmpgt
int _dleq(double a, double b); // swan's __aeabi_dcmple
int _dls(double a, double b);  // swan's __aeabi_dcmplt
int _deq(double a, double b);  // swan's __aeabi_dcmpeq
int _dneq(double a, double b); // swan's doubleIsNotEqual

// Float arithmetic
float _fadd(float a, float b);
float _fsub(float a, float b);
float _frsb(float a, float b); // b - a
float _fmul(float a, float b);
float _fdiv(float a, float b);
float _frdiv(float a, float b); // b / a

// Float comparison
int _fgeq(float a, float b);
int _fgr(float a, float b);
int _fleq(float a, float b);
int _fls(float a, float b);

// Conversions
float _d2f(double x);          // swan's __aeabi_d2f
double _f2d(float x);          // swan's __aeabi_f2d
double _dflt(int x);           // swan's __aeabi_i2d
double _dfltu(unsigned int x); // swan's __aeabi_ui2d
float _fflt(int x);            // swan's __aeabi_i2f
float _ffltu(unsigned int x);
int _ffix(float x);
unsigned int _ffixu(float x);             // swan's __aeabi_f2uiz
int _dfix(double x);                      // swan's __aeabi_d2iz
unsigned long long _ll_ufrom_d(double x); // swan's __aeabi_d2ulz

// 64-bit integers
long long _ull_mul(long long a, long long b); // and _ll_mul: the compiler calls it by both
long long _ll_shl(long long a, int shift);    // swan's __aeabi_llsl
long long _ll_sdiv(long long a, long long b);
long long _ll_mod(long long a, long long b);                             // the signed remainder (swan's mod64Signed)
unsigned long long _ll_udiv(unsigned long long a, unsigned long long b); // swan's __aeabi_uldivmod
unsigned long long _ull_mod(unsigned long long a, unsigned long long b);

// 32-bit division
int _s32_div_f(int a, int b);
unsigned int _u32_div_f(unsigned int a, unsigned int b); // swan's __aeabi_uidivmod

// Others
void func_0208e130(void); // dead code after _fadd
void _fp_init(void);

#endif // POKEBW2_MSL_RUNTIME_FP_FASTI_H
