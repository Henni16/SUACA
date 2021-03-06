#include "iacaMarks.h"
#include "stdio.h"

int main(int argc, char** argv) {

  asm(".byte 0x0F, 0x0B;"
      "movl $111, %ebx;"
  ".byte 0x64, 0x67, 0x90;"
  //".intel_syntax noprefix;"
//
//  "add RBX, 6;"
//  "add RAX, RBX;"
//  "add RAX, RBX;"
//  "add RAX, RBX;"
//  "add RAX, RBX;"
//  "add RBX, RBX;"
//  
  "vmovsd (%r14,%r15,8), %xmm2;"
  "vaddsd 16(%r14,%r15,8), %xmm2, %xmm3;"
  "vaddsd 8(%rax,%r15,8), %xmm3, %xmm4;"
  "vaddsd 8(%rdx,%r15,8), %xmm4, %xmm5;"
  "vmulsd %xmm5, %xmm1, %xmm6;"
  "vmovsd %xmm6, 8(%r12,%r15,8);"
  "incq %r15;"
  "cmpq %r13, %r15;"


  //".att_syntax prefix;"
  "movl $222, %ebx;"
  ".byte 0x64, 0x67, 0x90;"
  ".byte 0x0F, 0x0B;"
  );

  //IACA_START
  int c = 0;
  while (c<5) {
    //IACA_START
    c++;
  }
  //IACA_END
  //IACA_START
  int a = 1;
  int b;
  if (a < 4) {
    b = 1;
    b++;
    //IACA_END
  }
  else {
    b = 2;
  }
  a++;
  //IACA_END
  printf("%i\n", a+b);
  return 0;
}
