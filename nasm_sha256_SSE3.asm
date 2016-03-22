

%macro byte_swap32x4_SSE3 1
	pshufb %1, 0x32107654BA98FEDC
%endmacro
