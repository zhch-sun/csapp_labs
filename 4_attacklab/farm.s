
farm.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <start_farm>:
   0:	b8 01 00 00 00       	mov    $0x1,%eax
   5:	c3                   	retq   

0000000000000006 <getval_142>:
   6:	b8 fb 78 90 90       	mov    $0x909078fb,%eax
   b:	c3                   	retq   

000000000000000c <addval_273>:
   c:	8d 87 48 89 c7 c3    	lea    -0x3c3876b8(%rdi),%eax
  12:	c3                   	retq   

0000000000000013 <addval_219>:
  13:	8d 87 51 73 58 90    	lea    -0x6fa78caf(%rdi),%eax
  19:	c3                   	retq   

000000000000001a <setval_237>:
  1a:	c7 07 48 89 c7 c7    	movl   $0xc7c78948,(%rdi)
  20:	c3                   	retq   

0000000000000021 <setval_424>:
  21:	c7 07 54 c2 58 92    	movl   $0x9258c254,(%rdi)
  27:	c3                   	retq   

0000000000000028 <setval_470>:
  28:	c7 07 63 48 8d c7    	movl   $0xc78d4863,(%rdi)
  2e:	c3                   	retq   

000000000000002f <setval_426>:
  2f:	c7 07 48 89 c7 90    	movl   $0x90c78948,(%rdi)
  35:	c3                   	retq   

0000000000000036 <getval_280>:
  36:	b8 29 58 90 c3       	mov    $0xc3905829,%eax
  3b:	c3                   	retq   

000000000000003c <mid_farm>:
  3c:	b8 01 00 00 00       	mov    $0x1,%eax
  41:	c3                   	retq   
