	.section	__TEXT,__text,regular,pure_instructions
	.macosx_version_min 10, 10
	.globl	_main
	.align	4, 0x90
_main:                                  ## @main
## BB#0:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$8, %esp
	movl	$L_.str, (%esp)
	calll	_puts
	xorl	%eax, %eax
	addl	$8, %esp
	popl	%ebp
	retl

_transform:
	

	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 ## @.str
	.asciz	"Hello world!\n"


.subsections_via_symbols
