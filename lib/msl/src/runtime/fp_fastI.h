#ifndef POKEBW2_MSL_RUNTIME_FP_FASTI_H
#define POKEBW2_MSL_RUNTIME_FP_FASTI_H

// The routines of fp/fp_fastI.c, MSL's floating-point and integer runtime, which MWCC's code calls for arithmetic that
// the processor has no instruction for. The names are the symbols' (swan's AEABI names, or the runtime's own); the
// comments give the runtime's name where it differs.

// Double arithmetic
double __aeabi_dadd(double a, double b); // _dadd
double doubleSub(double a, double b);    // _dsub
double _drsb(double a, double b);        // b - a
double __aeabi_dmul(double a, double b); // _dmul
double __aeabi_ddiv(double a, double b); // _ddiv
double _drdiv(double a, double b);       // b / a

// Double comparison, which leaves its answer in the flags as well
int __aeabi_dcmpge(double a, double b);   // _dgeq
int __aeabi_dcmpgt(double a, double b);   // _dgr
int __aeabi_dcmple(double a, double b);   // _dleq
int __aeabi_dcmplt(double a, double b);   // _dls
int __aeabi_dcmpeq(double a, double b);   // _deq
int doubleIsNotEqual(double a, double b); // _dneq

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
float __aeabi_d2f(double x);         // _d2f
double __aeabi_f2d(float x);         // _f2d
double __aeabi_i2d(int x);           // _dflt
double __aeabi_ui2d(unsigned int x); // _dfltu
float __aeabi_i2f(int x);            // _fflt
float _ffltu(unsigned int x);
int _ffix(float x);
unsigned int __aeabi_f2uiz(float x); // _ffixu
int __aeabi_d2iz(double x);          // _dfix
unsigned long long __aeabi_d2ulz(double x);

// 64-bit integers
long long _ull_mul(long long a, long long b);   // _ll_mul
long long __aeabi_llsl(long long a, int shift); // _ll_shl
long long _ll_sdiv(long long a, long long b);
long long mod64Signed(long long a, long long b);                                 // the signed remainder
unsigned long long __aeabi_uldivmod(unsigned long long a, unsigned long long b); // _ll_udiv
unsigned long long _ull_mod(unsigned long long a, unsigned long long b);

// 32-bit division
int _s32_div_f(int a, int b);
unsigned int __aeabi_uidivmod(unsigned int a, unsigned int b); // _u32_div_f

// Others
void func_0208e130(void); // dead code after _fadd
void func_0208ecdc(void); // _fp_init

#endif // POKEBW2_MSL_RUNTIME_FP_FASTI_H
