; nasm -f macho64 -g nasm_sha256.asm && ld -macosx_version_min 10.10 -lSystem -o nasm_sha256 nasm_sha256.o
section .data align=16
	k: dd 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
	initial_state:  dd 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
	padding: db 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	ctx_data: dd 0x00000000, 0x11111111, 0x22222222, 0x33333333, 0x44444444, 0x55555555, 0x66666666, 0x77777777, 0x88888888, 0x99999999, 0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF
section .bss align=16
	w: resd 64
	ctx_state: resd 8

section .text

extern _printf
extern _memcpy
extern _memset
extern _bin2hex

global _asm_sha256d_scan
global _asm_sha256_hash

%define ctx_state_offset(position) position * 4 
%define ctx_data_offset(position) 32 + (position * 4)
%define SHA256_LENGTH 32
%define SHA256_CTX_LENGTH 192

; [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17]

; [0, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8]
; [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17]

; Performs a byte swap of 4 32 bit values
; byte_swap32x4(dst&src, tmp_xmm1, tmp_xmm2)
%macro byte_swap32x4_SSE2 3
	movdqa %2, %1
	movdqa %3, %1
	pxor    %1, %1
	punpckhbw %2, %1 ; interleave '0' with bytes of original
	punpcklbw %3, %1 ;  so they become words 
	pshuflw %2, %2, 0b00_01_10_11 ; swap the words by shuffling
	pshufhw %2, %2, 0b00_01_10_11 
	pshuflw %3, %3, 0b00_01_10_11
	pshufhw %3, %3, 0b00_01_10_11
	packuswb %3, %2 ; pack/de-interleave, ie make the words back into bytes.
	movdqa %1, %3
%endmacro

; XMM rotate(dst&src, aux, quantity)
%macro xmm_rotate 3
	movdqa %2, %1
	psrld  %1, %3
	pslld  %2, (32 - %3)
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
	xor %2, %3
	and %1, %2
	xor %1, %3
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
	movdqa %2, %1
	xmm_rotate %2, %3, 13
	movdqa %3, %1
	xmm_rotate %3, %4, 22
	xmm_rotate %1, %4, 2
	pxor %1, %2
	pxor %1, %3
%endmacro

%macro xmm_S1 4
	movdqa %2, %1
	xmm_rotate %2, %3, 6
	movdqa %3, %1
	xmm_rotate %3, %4, 11
	xmm_rotate %1, %4, 25
	pxor %1, %2
	pxor %1, %3
%endmacro

%macro xmm_s0 4
	movdqa %2, %1
	xmm_rotate %2, %3, 7
	movdqa %3, %1
	xmm_rotate %3, %4, 18
	; LOOK INTO ARITMETHIC AND LOGICAL SHIFTS IF THEY BEHAVE THE SAME IN THE C CODE
	psrld %1, 3
	pxor %1, %2
	pxor %1, %3
%endmacro

%macro xmm_s1 4
	movdqa %2, %1
	xmm_rotate %2, %3, 17
	movdqa %3, %1
	xmm_rotate %3, %4, 19
	; LOOK INTO ARITMETHIC AND LOGICAL SHIFTS IF THEY BEHAVE THE SAME IN THE C CODE
	psrld %1, 10
	pxor %1, %2
	pxor %1, %3
%endmacro

; unrolled_w_extend(rax, xmm0, xmm1, xmm2, xmm3, tmp_xmm4, tmp_xmm5, tmp_xmm6, tmp2_xmm7
; xmm0 .. xmm3 must contain 16 W's at any giving point of the excecution
; as the procedure works with 4 32 ints at the time, we only need 16 unrolls
%macro unrolled_w_extend 8
	; Sum the w[-7]'s to the w[-16]'s
	; Get the w[-7]'s altogether
	movdqa %5, %3
	psrldq %5, 4
	movdqa %6, %4
	pslldq %6, 12
	por %5, %6
	; xmm4 now has w[-7] + w[-16]
	paddd %5, %1

	; Get the 15's
	movdqa %6, %1
	movdqa %7, %2
	pslldq %7, 12
	psrldq %6, 4
	por %6, %7

	; Push the 4 first ints and shift
	; movdqa [%1], %1
	; add %1, 16d ; push the mem pointer to the next place in memory
	movdqa %1, %2
	movdqa %2, %3
	movdqa %3, %4
	movdqa %4, %5
	; Apply s0 to each 15's
	xmm_s0 %6, %5, %7, %8
	paddd %4, %6
	; Apply S1 to the two 2's we have
	movdqa %5, %3
	;pand xmm4, 0000000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	psrldq %5, 8
	xmm_s1 %5, %6, %7, %8
	; Adding the two 2's to xmm3 will result in the higher double word
	; computed, we then can use that higher doubler word to compute the
	; last two 2's left and get a complete 128 extension
	paddd %4, %5

	; Get the new w[-2]'s that we have just generated
	movdqa %5, %4
	pslldq %5, 8
	; Coping xmm3 altogether isn't better? then shifting
	; LOOK IF THIS DOUBLE WORDS HAVE THE OTHER DOUBLE WORDS IN 0 WHEN WE ADD THEM
	xmm_s1 %5, %6, %7, %8
	paddd %4, %5
%endmacro


; _main:
; push rbp
; mov rbp, rsp

; ; lea rdi, [rel hello_world]
; ; call _printf

; lea rdi, [rel ctx_state]
; call _sha256_init

; lea rdi, [rel initial_state]

; call _sha256_transform
; ; lea rdi, [rel sha256_init_text]
; ; lea rsi, [rel w]
; ; mov esi, [rsi]
; ; lea rdx, [rel w + 4]
; ; mov edx, [rdx]
; ; lea rcx, [rel w + 8]
; ; mov ecx, [rcx]
; ; lea r8, [rel w + 12]
; ; mov r8d, [r8]
; ; call _printf
; ;mov rax, 0x2000004      ; System call write = 4
; ;mov rdi, 1              ; Write to standard out = 1
; ;mov rsi, hello_world    ; The address of hello_world string
; ;mov rdx, 14             ; The size to write
; 		                ; Invoke the kernel
; mov rax, 0x2000001      ; System call number for exit = 1
; mov rdi, 0              ; Exit success = 0

; pop rbp
; syscall                 ; Invoke the kernel


; Recieves a ctx struct in RDI
_asm_sha256_transform:
push rbp
mov rbp, rsp
push rbx
push r12
push r13
push r14
push r15

; Copies 16 32bits uint and transforms them into big endian
; lea rdi, [rel ctx_data]
movdqu xmm0, [rdi + ctx_data_offset(0)]
movdqu xmm1, [rdi + ctx_data_offset(4)]
movdqu xmm2, [rdi + ctx_data_offset(8)]
movdqu xmm3, [rdi + ctx_data_offset(12)]

byte_swap32x4_SSE2 xmm0, xmm4, xmm5 
byte_swap32x4_SSE2 xmm1, xmm4, xmm5
byte_swap32x4_SSE2 xmm2, xmm4, xmm5
byte_swap32x4_SSE2 xmm3, xmm4, xmm5

; Copy state to xmm4, xmm5 (ls)
; xmm4 and xmm5 become ls
movdqu xmm4, [rdi + ctx_state_offset(0)]
movdqu xmm5, [rdi + ctx_state_offset(4)]

; Main loop unrolled

movd r8d, xmm5
psrldq xmm5, 4

movd r9d, xmm5
psrldq xmm5, 4

movd r10d, xmm5
psrldq xmm5, 4

movd r11d, xmm5
psrldq xmm5, 4

movd r12d, xmm4
psrldq xmm4, 4

movd r13d, xmm4
psrldq xmm4, 4

movd r14d, xmm4
psrldq xmm4, 4

movd r15d, xmm4
psrldq xmm4, 4

%assign i 0
%rep 16
	; Correct memory reading
	movdqa xmm4, [rel k + (i * 16)]
	paddd xmm4, xmm0
	%assign i i+1
	%rep 4
		; rcx => Holds t1
		; rbx
		; rax
		; rdx
		; rdi/
		; rsi

		; r12d => ls[0]
		; r13d => ls[1]
		; r14d => ls[2]
		; r15d => ls[3]
		; r8d => ls[4]
		; r9d => ls[5]
		; r10d => ls[6]
		; r11d => ls[7]

		; ECX will hold t1

		; Starts with r[7] in ecx
		mov ecx, r11d
		; Put k[i] + w[i] in eax
		movd eax, xmm4
		psrldq xmm4, 4
		; Ads k[i] to ecx
		; add ecx, eax
		; Put w[i] in eax
		; movd eax, xmm0
		; psrldq xmm0, 4
		; Adds k[i] + w[i] to ecx
		add ecx, eax
		; Mov l[4]
		mov edx, r8d 
		; Do S1
		S1 edx, eax, esi
		; Adds S1 to ecx
		add ecx, edx
		; Do CH
		mov edx, r8d
		mov esi, r9d
		mov eax, r10d
		CH edx, esi, eax
		; Adds the result of Ch to ecx and obtains t1
		add ecx, edx

		; t1 completed, move the last 8 bytes
		mov r11d, r10d
		mov r10d, r9d
		mov r9d, r8d
		mov r8d, r15d
		; ls[4] = ls[3] + t1
		add r8d, ecx

		; Now to the other half part
		; Mov each integer
		mov r15d, r14d
		mov r14d, r13d
		mov r13d, r12d
		; r12d will hold t2
		S0 r12d, eax, edx
		mov ebx, r13d
		mov eax, r14d
		mov edx, r15d
		Maj ebx, eax, edx, esi
		add r12d, ebx
		; We have now t2_1 in r12d
		; Add t1_1 & t2_1
		add r12d, ecx
	%endrep

	; W extension
	; Each W extension generates 4 new w's
	; We have conserverd xmm0, xmm1, xmm2 and xmm3
	; from the begging of the function to use it here
	unrolled_w_extend xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7
%endrep

; Rebuild to xmm0
movd xmm0, r12d
movd xmm1, r13d
pslldq xmm1, 4
por xmm0, xmm1
movd xmm1, r14d
pslldq xmm1, 8
por xmm0, xmm1
movd xmm1, r15d
pslldq xmm1, 12
por xmm0, xmm1

; Rebuild to xmm1
movd xmm1, r8d
movd xmm2, r9d
pslldq xmm2, 4
por xmm1, xmm2
movd xmm2, r10d
pslldq xmm2, 8
por xmm1, xmm2
movd xmm2, r11d
pslldq xmm2, 12
por xmm1, xmm2

; Get the state data again
movdqu xmm4, [rdi + ctx_state_offset(0)]
movdqu xmm5, [rdi + ctx_state_offset(4)]
; Sum the local with the ctx state data
paddd xmm0, xmm4
paddd xmm1, xmm5

; Store the new ctx data
movdqu [rdi + ctx_state_offset(0)], xmm0
movdqu [rdi + ctx_state_offset(4)], xmm1

pop r15
pop r14
pop r13
pop r12
pop rbx
pop rbp
ret

_asm_sha256_init:
; IF NOT USING THE REGISTERS, DO NOT PUSH THEM
lea rax, [rel initial_state]
movdqu xmm0, [rax]
movdqu xmm1, [rax + 16]
movdqu [rdi + ctx_state_offset(0)], xmm0
movdqu [rdi + ctx_state_offset(4)], xmm1
ret

; asm_sha256 -> sha256_ctx *context, const uint8_t *data, const size_t length
_asm_sha256:
push rbp
mov rbp, rsp
push rbx
push r12
push r13
push r14
push r15
sub rsp, 8d

; rbx => sha256_ctx *context
; r12 => uint8_t *data
; r13 => size_t length
; r14 => bytes_left
; r15 => bytes_hashed

mov rbx, rdi
mov r12, rsi
mov r13, rdx
; Set bytes_left and bytes_hashed to 0
mov r14, 0d
mov r15, 0d

.hashedCicle:
; See if the bytes
cmp r15, r13
; If the bytes_hashed are >= to length, we have finished
jnl .end
; bytes_left = length - bytes_hashed;
mov r14, r13
sub r14, r15
;bytes_left < 56
cmp r14, 56d
jnl .bytesLeftBetween57And64
; memcpy((uint8_t *) context->data, data + bytes_hashed, bytes_left);
lea rdi, [rbx + ctx_data_offset(0)]
lea rsi, [r12 + r15]
; bytes_hashed += bytes_left;
add r15, r14
mov rdx, r14
call _memcpy

lea rdi, [rbx + ctx_data_offset(0) + r14]
; Add bytes left
;add rdi, r14
lea rsi, [rel padding]
mov rdx, 56d
sub rdx, r14
call _memcpy
lea rdi, [rbx + ctx_data_offset(0) + 56]
; be_bit_length
mov rax, r15
mov rcx, 8d
mul rcx
bswap rax
mov [rbp - 8], rax
lea rsi, [rbp-8]
mov rdx, 8d
call _memcpy

mov rdi, rbx
call _asm_sha256_transform
jmp .hashedCicle

.bytesLeftBetween57And64:
cmp r14, 64d
jnle .bytesLeftGreaterThan64
; memcpy((uint8_t *) context->data, data + bytes_hashed, bytes_left);
lea rdi, [rbx + ctx_data_offset(0)]
lea rsi, [r12 + r15]
; bytes_hashed += bytes_left;
add r15, r14
mov rdx, r14
call _memcpy

lea rdi, [rbx + ctx_data_offset(0) + r14]
lea rsi, [rel padding]
mov rdx, 64d
sub rdx, r14
call _memcpy

mov rdi, rbx
call _asm_sha256_transform

cmp r14, 64d
je .justPadd
lea rdi, [rbx + ctx_data_offset(0)]
mov rsi, 0d
mov rdx, 56d
call _memset
jmp .writeLength

.justPadd:
lea rdi, [rbx + ctx_data_offset(0)]
lea rsi, [rel padding]
mov rdx, 56d
call _memcpy

.writeLength:
lea rdi, [rbx + ctx_data_offset(0) + 56]
; be_bit_length
mov rax, r15
mov rcx, 8d
mul rcx
bswap rax
mov [rbp - 8], rax
lea rsi, [rbp-8]
mov rdx, 8d
call _memcpy

mov rdi, rbx
call _asm_sha256_transform
jmp .hashedCicle

.bytesLeftGreaterThan64:
; memcpy((uint8_t *) context->data, data + bytes_hashed, 64);
lea rdi, [rbx + ctx_data_offset(0)]
lea rsi, [r12 + r15]
; bytes_hashed += bytes_left;
add r15, 64d
mov rdx, 64d
call _memcpy

mov rdi, rbx
call _asm_sha256_transform
jmp .hashedCicle

.end:
; Convert to big endian
movdqu xmm0, [rbx + ctx_state_offset(0)]
movdqu xmm1, [rbx + ctx_state_offset(4)]
byte_swap32x4_SSE2 xmm0, xmm2, xmm3
byte_swap32x4_SSE2 xmm1, xmm2, xmm3
movdqu [rbx + ctx_state_offset(0)], xmm0
movdqu [rbx + ctx_state_offset(4)], xmm1

add rsp, 8d
pop r15
pop r14
pop r13
pop r12	
pop rbx
pop rbp
ret

;(const uint8_t *data, const size_t length, char *hash)
_asm_sha256_hash:
push rbp
mov rbp, rsp
push rbx
push r12
push r13
; Create local ctx context
sub rsp, SHA256_CTX_LENGTH
; Align stack
sub rsp, 8d

; RBX -> data
mov rbx, rdi
; R12 -> length
mov r12, rsi
; R13 -> hash
mov r13, rdx

lea rdi, [rbp - SHA256_CTX_LENGTH]
call _asm_sha256_init

; sha256_ctx *context, const uint8_t *data, const size_t length
lea rdi, [rbp - SHA256_CTX_LENGTH]
mov rsi, rbx
mov rdx, r12
call _asm_sha256

; Call _bin2hex
mov rdi, r13
lea rsi, [rbp - SHA256_CTX_LENGTH]
mov rdx, SHA256_LENGTH
call _bin2hex

add rsp, SHA256_CTX_LENGTH + 8
pop r13
pop r12
pop rbx
pop rbp
ret

; sha256d_scan(uint32_t *fst_state, uint32_t *snd_state, const uint32_t *data, uint32_t *lw)
_asm_sha256d_scan:
push rbx
push r12
push r13
push r14
sub rsp, 8
mov rbx, rdi
mov r12, rsi
mov r13, rdx
mov r14, rcx
call _asm_sha256_init
; Convert 64 bytes of data to big endian
movdqu xmm0, [r13]
movdqu xmm1, [r13 + 16]
movdqu xmm2, [r13 + 32]
movdqu xmm3, [r13 + 48]
byte_swap32x4_SSE2 xmm0, xmm4, xmm5
byte_swap32x4_SSE2 xmm1, xmm4, xmm5
byte_swap32x4_SSE2 xmm2, xmm4, xmm5
byte_swap32x4_SSE2 xmm3, xmm4, xmm5
movdqu [r14], xmm0
movdqu [r14 + 16], xmm1
movdqu [r14 + 32], xmm2
movdqu [r14 + 48], xmm3
; Calls transform
mov rdi, rbx
mov rsi, r14
call _asm_sha256_transform_scan
; Convert 16 bytes of data to big endian
movdqu xmm0, [r13 + 64]
movdqu xmm1, [r13 + 80]
movdqu xmm2, [r13 + 96]
movdqu xmm3, [r13 + 112]
byte_swap32x4_SSE2 xmm0, xmm4, xmm5
movdqu [r14], xmm0
movdqu [r14 + 16], xmm1
movdqu [r14 + 32], xmm2
movdqu [r14 + 48], xmm3
; Calls transform
mov rdi, rbx
mov rsi, r14
call _asm_sha256_transform_scan
; Converts to big endian the resulting hash
; movdqu xmm0, [rbx]
; movdqu xmm1, [rbx + 16]
; byte_swap32x4_SSE2 xmm0, xmm2, xmm3
; byte_swap32x4_SSE2 xmm1, xmm2, xmm3
; movdqu [rbx], xmm0
; movdqu [rbx + 16], xmm1

; Hashing the previous hash

; Inicializing the second state
mov rdi, r12
call _asm_sha256_init
; Convert 32 bytes of data to big endian
mov rdi, r12
mov rsi, rbx
call _asm_sha256_transform_scan
; Converts to big endian the resulting hash
movdqu xmm0, [r12]
movdqu xmm1, [r12 + 16]
byte_swap32x4_SSE2 xmm0, xmm2, xmm3
byte_swap32x4_SSE2 xmm1, xmm2, xmm3
movdqu [r12], xmm0
movdqu [r12 + 16], xmm1
add rsp, 8d
pop r14
pop r13
pop r12
pop rbx
ret

; _asm_sha256_transform_scan(uint32_t *state, const uint32_t *data)
_asm_sha256_transform_scan:
push rbp
mov rbp, rsp
push rbx
push r12
push r13
push r14
push r15

; Copies 16 32bits uint and transforms them into big endian
; lea rdi, [rel ctx_data]
movdqu xmm0, [rsi]
movdqu xmm1, [rsi + 16]
movdqu xmm2, [rsi + 32]
movdqu xmm3, [rsi + 48]

; Copy state to xmm4, xmm5 (ls)
; xmm4 and xmm5 become ls
movdqu xmm4, [rdi]
movdqu xmm5, [rdi + 16]

; Main loop unrolled

movd r8d, xmm5
psrldq xmm5, 4

movd r9d, xmm5
psrldq xmm5, 4

movd r10d, xmm5
psrldq xmm5, 4

movd r11d, xmm5
psrldq xmm5, 4

movd r12d, xmm4
psrldq xmm4, 4

movd r13d, xmm4
psrldq xmm4, 4

movd r14d, xmm4
psrldq xmm4, 4

movd r15d, xmm4
psrldq xmm4, 4

%assign i 0
%rep 16
	; Correct memory reading
	movdqa xmm4, [rel k + (i * 16)]
	paddd xmm4, xmm0
	%assign i i+1
	%rep 4
		; rcx => Holds t1
		; rbx
		; rax
		; rdx
		; rdi/
		; rsi

		; r12d => ls[0]
		; r13d => ls[1]
		; r14d => ls[2]
		; r15d => ls[3]
		; r8d => ls[4]
		; r9d => ls[5]
		; r10d => ls[6]
		; r11d => ls[7]

		; ECX will hold t1

		; Starts with r[7] in ecx
		mov ecx, r11d
		; Put k[i] + w[i] in eax
		movd eax, xmm4
		psrldq xmm4, 4
		; Ads k[i] to ecx
		; add ecx, eax
		; Put w[i] in eax
		; movd eax, xmm0
		; psrldq xmm0, 4
		; Adds k[i] + w[i] to ecx
		add ecx, eax
		; Mov l[4]
		mov edx, r8d 
		; Do S1
		S1 edx, eax, esi
		; Adds S1 to ecx
		add ecx, edx
		; Do CH
		mov edx, r8d
		mov esi, r9d
		mov eax, r10d
		CH edx, esi, eax
		; Adds the result of Ch to ecx and obtains t1
		add ecx, edx

		; t1 completed, move the last 8 bytes
		mov r11d, r10d
		mov r10d, r9d
		mov r9d, r8d
		mov r8d, r15d
		; ls[4] = ls[3] + t1
		add r8d, ecx

		; Now to the other half part
		; Mov each integer
		mov r15d, r14d
		mov r14d, r13d
		mov r13d, r12d
		; r12d will hold t2
		S0 r12d, eax, edx
		mov ebx, r13d
		mov eax, r14d
		mov edx, r15d
		Maj ebx, eax, edx, esi
		add r12d, ebx
		; We have now t2_1 in r12d
		; Add t1_1 & t2_1
		add r12d, ecx
	%endrep

	; W extension
	; Each W extension generates 4 new w's
	; We have conserverd xmm0, xmm1, xmm2 and xmm3
	; from the begging of the function to use it here
	unrolled_w_extend xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7
%endrep

; Rebuild to xmm0
movd xmm0, r12d
movd xmm1, r13d
pslldq xmm1, 4
por xmm0, xmm1
movd xmm1, r14d
pslldq xmm1, 8
por xmm0, xmm1
movd xmm1, r15d
pslldq xmm1, 12
por xmm0, xmm1

; Rebuild to xmm1
movd xmm1, r8d
movd xmm2, r9d
pslldq xmm2, 4
por xmm1, xmm2
movd xmm2, r10d
pslldq xmm2, 8
por xmm1, xmm2
movd xmm2, r11d
pslldq xmm2, 12
por xmm1, xmm2

; Get the state data again
movdqu xmm4, [rdi]
movdqu xmm5, [rdi + 16]
; Sum the local with the ctx state data
paddd xmm0, xmm4
paddd xmm1, xmm5

; Store the new ctx data
movdqu [rdi], xmm0
movdqu [rdi + 16], xmm1

pop r15
pop r14
pop r13
pop r12
pop rbx
pop rbp
ret
