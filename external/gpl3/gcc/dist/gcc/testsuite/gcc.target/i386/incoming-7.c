/* PR target/40838 */
/* { dg-do compile { target { { ! *-*-darwin* } && ilp32 } } } */
/* { dg-options "-w -mstackrealign -O2 -msse2 -mpreferred-stack-boundary=4" } */
/* { dg-require-effective-target sse2 } */

typedef int v4si __attribute__ ((vector_size (16)));

extern v4si y(v4si, v4si, v4si, v4si, v4si);

extern v4si s1, s2;

v4si x(void)
{
  return y(s1, s2, s1, s2, s2);
}

/* { dg-final { scan-assembler "andl\[\\t \]*\\$-16,\[\\t \]*%esp" } } */
