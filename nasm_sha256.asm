; nasm -f macho64 -g nasm_sha256.asm && ld -macosx_version_min 10.10 -lSystem -o nasm_sha256 nasm_sha256.o
section .data
	hello_world     db      "Hello World!", 0x0a, 0x0
	sha256_init_text     db      "1: 0x%08x 2: 0x%08x 3: 0x%08x 4: 0x%08x", 0x0a, 0x0
	initial_state:  dq 0xbb67ae85, 0x6a09e667, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
	k: dq 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2

section .bss
	w: resd 64

.section __TEXT,__text

extern _printf

global _main

%define ctx_state_offset(position) position * 4 
%define ctx_data_offset(position) 256 + (position * 4)

; Performs a byte swap of 4 32 bit values
; byte_swap32x4(dst&src, tmp_xmm1, tmp_xmm2)
%macro byte_swap32x4_SSE2 3
	movdqu %2, %1
	movdqu %3, %1
	pxor    %1, %1
	punpckhbw %2, %1 ; interleave '0' with bytes of original
	punpcklbw %3, %1 ;  so they become words 
	pshuflw %2, %2, 0b00_01_10_11 ; swap the words by shuffling
	pshufhw %2, %2, 0b00_01_10_11 
	pshuflw %3, %3, 0b00_01_10_11
	pshufhw %3, %3, 0b00_01_10_11
	packuswb %3, %2 ; pack/de-interleave, ie make the words back into bytes.
	movdqu %1, %3
%endmacro

; XMM rotate(dst&src, aux, quantity)
%macro xmm_rotate 3
	movdqu %2, %1
	pslld  %1, %3
	psrld  %2, (32 - %3)
	por %1, %2
%endmacro 

; S0 in 64bit registers
; S0(src&dst, tmp1, tmp2)
%macro S0 3
	mov %2, %1
	mov %3, %1
	ror %1, 2
	ror %2, 13
	ror %3, 22
	xor %1, %2
	xor %1, %3
%endmacro

; S1 in 64bit registers
; S1(src&dst, tmp1, tmp2)
%macro S1 3
	mov %2, %1
	mov %3, %1
	ror %1, 6
	ror %2, 11
	ror %3, 25
	xor %1, %2
	xor %1, %3
%endmacro

; Remember that those modify the other registers
; Ch(x&dst, y, z)
%macro CH 3
	mov %4, 3
	xor %2, 3
	and %1, %2
	xor %1, 3
%endmacro

; Maj(x&dst, y, z, tmp)
%macro Maj 4
	mov %4, %3
	and %4, %2
	or %3, %2
	and %1, %3
	or %1, %4
%endmacro

%macro xmm_S0 4
	movdqu %2, %1
	xmm_rotate %2, %3, 13
	movdqu %3, %1
	xmm_rotate %3, %4, 22
	xmm_rotate %1, %4, 2
	pxor %1, %2
	pxor %1, %3
%endmacro

%macro xmm_S1 4
	movdqu %2, %1
	xmm_rotate %2, %3, 6
	movdqu %3, %1
	xmm_rotate %3, %4, 11
	xmm_rotate %1, %4, 25
	pxor %1, %2
	pxor %1, %3
%endmacro

%macro xmm_s0 4
	movdqu %2, %1
	xmm_rotate %2, %3, 7
	movdqu %3, %1
	xmm_rotate %3, %4, 18
	; LOOK INTO ARITMETHIC AND LOGICAL SHIFTS IF THEY BEHAVE THE SAME IN THE C CODE
	psrld %1, 3
	pxor %1, %2
	pxor %1, %3
%endmacro

%macro xmm_s1 4
	movdqu %2, %1
	xmm_rotate %2, %3, 17
	movdqu %3, %1
	xmm_rotate %3, %4, 19
	; LOOK INTO ARITMETHIC AND LOGICAL SHIFTS IF THEY BEHAVE THE SAME IN THE C CODE
	psrld %1, 10
	pxor %1, %2
	pxor %1, %3
%endmacro

; unrolled_w_extend(memory ptr), xmm0, xmm1, xmm2, xmm3 must contain W at a giving point of the excecution
; as the procedure works with 4 32 ints at the time, we only need 16 unrolls
%macro unrolled_w_extend 1
	; Sum the w[-7]'s to the w[-16]'s
	; Get the w[-7]'s altogether
	movdqu xmm4, xmm2
	pslldq xmm4, 32
	movdqu xmm5, xmm3
	psrldq xmm5, 96
	por xmm4, xmm5
	; xmm4 now has w[-7] + w[-16]
	paddd xmm4, xmm0 
	; Get the 15's
	movdqu xmm5, xmm0
	movdqu xmm6, xmm1
	psrldq xmm6, 96
	por xmm5, xmm6
	; Push the 4 first ints and shift
	movdqu [%1], xmm0
	add %1, 16d ; push the mem pointer to the next place in memory
	movdqu xmm0, xmm1
	movdqu xmm1, xmm2
	movdqu xmm2, xmm3
	movdqu xmm3, xmm4
	; Apply s0 to each 15's
	xmm_s0 xmm5, xmm4, xmm6, xmm7
	paddd xmm3, xmm5 ; LOOK IF PADDD IS THE ONE WE NEED
	; Apply S1 to the two 2's we have
	movdqu xmm4, xmm2
	;pand xmm4, 0000000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	pslldq xmm4, 64
	xmm_s1 xmm4, xmm5, xmm6, xmm7
	; Adding the two 2's to xmm3 will result in the higher double word
	; computed, we then can use that higher doubler word to compute the
	; last two 2's left and get a complete 128 extension
	paddd xmm3, xmm4

	; Get the new w[-2]'s that we have just generated
	movdqu xmm4, xmm3
	psrldq xmm4, 64
	; Coping xmm3 altogether isn't better? then shifting
	; LOOK IF THIS DOUBLE WORDS HAVE THE OTHER DOUBLE WORDS IN 0 WHEN WE ADD THEM
	xmm_s1 xmm4, xmm5, xmm6, xmm7
	paddd xmm3, xmm4
%endmacro


_main:
push rbp
mov rbp, rsp

;sub rsp, 8
; lea rdi, [rel hello_world]
; call _printf

lea rdi, [rel w]
call _sha256_init

; lea rdi, [rel sha256_init_text]
; lea rsi, [rel w]
; mov esi, [rsi]
; lea rdx, [rel w + 4]
; mov edx, [rdx]
; lea rcx, [rel w + 8]
; mov ecx, [rcx]
; lea r8, [rel w + 12]
; mov r8d, [r8]
; call _printf
;mov rax, 0x2000004      ; System call write = 4
;mov rdi, 1              ; Write to standard out = 1
;mov rsi, hello_world    ; The address of hello_world string
;mov rdx, 14             ; The size to write
		                ; Invoke the kernel
mov rax, 0x2000001      ; System call number for exit = 1
mov rdi, 0              ; Exit success = 0

pop rbp
syscall                 ; Invoke the kernel


; Recieves a ctx struct in RDI
_sha256_transform:
push rbp
mov rbp, rsp
push rbx
push r12
push r13
push r14
push r15

; Copies 16 32bits uint and transforms them into big endian
movdqu xmm0, [rdi + ctx_data_offset(0)]
movdqu xmm1, [rdi + ctx_data_offset(16)]
movdqu xmm2, [rdi + ctx_data_offset(32)]
movdqu xmm3, [rdi + ctx_data_offset(48)]

byte_swap32x4_SSE2 xmm0, xmm4, xmm5 
byte_swap32x4_SSE2 xmm1, xmm4, xmm5
byte_swap32x4_SSE2 xmm2, xmm4, xmm5
byte_swap32x4_SSE2 xmm3, xmm4, xmm5

; Copy the address of w into rax
mov rax, w

; W extension
%rep 16
	unrolled_w_extend rax
%endrep
; See if I need to move to memory all the xmm1, xmm2 and xmm3 regs

; Copy state to xmm0, xmm1 (ls)
; xmm0 and xmm1 become ls
movdqu xmm0, [rdi + ctx_state_offset(0)]
movdqu xmm1, [rdi + ctx_state_offset(4)]

; Main loop unrolled
lea rax, [rel w]
movdqu xmm2, [rax]
lea rax, [rel k]
movdqu xmm3, [rax]

; Calculate t1

; Calculate t2

; Copy the result to the local state
pslldq xmm1, 32
movdqu xmm4, xmm0
psrldq xmm4, 96
; Sum t1
por xmm1, xmm4

pslldq xmm0, 32

; Copy local state to state
; Adds the result to the state
movdqu xmm2, [rdi + ctx_data_offset(0)]
movdqu xmm3, [rdi + ctx_data_offset(4)]
paddd xmm0, xmm2
paddd xmm1, xmm3
movdqu [rdi + ctx_data_offset(0)], xmm0
movdqu [rdi + ctx_data_offset(4)], xmm1

pop r15
pop r14
pop r13
pop r12
pop rbx
pop rbp
ret

_sha256_init:
; IF NOT USING THE REGISTERS, DO NOT PUSH THEM
push rbp
mov rbp, rsp
push rbx
push r12
push r13
push r14
push r15

lea rax, [rel initial_state]
movdqu xmm0, [rax]
movdqu xmm1, [rax + 16]
movdqu [rdi + ctx_state_offset(0)], xmm0
movdqu [rdi + ctx_state_offset(4)], xmm1

pop r15
pop r14
pop r13
pop r12
pop rbx
pop rbp
ret
