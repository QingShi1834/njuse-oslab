
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
_NR_myPrintf		equ 1
_NR_P 				equ 2
_NR_V				equ 3
_NR_sleep			equ 4
INT_VECTOR_SYS_CALL equ 0x90

; 导出符号
global	get_ticks
global  myPrintf
global	P
global  V
global  sleep

extern string
bits 32
[section .text]

; ====================================================================
;                              get_ticks
; ====================================================================
get_ticks:
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL
	ret

myPrintf:
	push ebx
	push ecx

	mov	eax, _NR_myPrintf
	mov ebx,[esp + 12]
	mov ecx,[esp + 16]
	int	INT_VECTOR_SYS_CALL
	pop ecx
	pop ebx
	ret

P:
	mov	eax,_NR_P
	mov ebx,[esp + 4]
	int INT_VECTOR_SYS_CALL
	ret

V:
	mov	eax,_NR_V
	mov ebx,[esp + 4]
	int INT_VECTOR_SYS_CALL
	ret

sleep:
	mov	eax,_NR_sleep
	mov ebx,[esp + 4]
	int INT_VECTOR_SYS_CALL
	ret
