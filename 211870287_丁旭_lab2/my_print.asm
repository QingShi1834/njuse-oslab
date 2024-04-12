; 参考代码：https://blog.csdn.net/bedisdover/article/details/51287555
global print
global print_red
global my_print

section .data
color_red	db 1Bh,'[31m',0
.len		equ $-color_red
color_default	db 1Bh,'[37m',0
.len		equ $-color_default 

section .text

print:
	mov eax, 4
	mov ebx, 1
	mov ecx, color_default
	mov edx, color_default.len
	int 80h
	
	mov ecx, [esp+4] ; ecx = str
	mov eax, [esp+4]
	call strlen
	mov edx, eax ; edx = len
	mov eax, 4
	mov ebx, 1
	int 80h

	ret

print_red:
	mov eax, 4
	mov ebx, 1
	mov ecx, color_red
	mov edx, color_red.len
	int 80h
	
	mov ecx, [esp+4]
	mov eax, [esp+4]
	call strlen
	mov edx, eax
	mov eax, 4
	mov ebx, 1
	int 80h

	mov eax, 4
	mov ebx, 1
	mov ecx, color_default
	mov edx, color_default.len
	int 80h

	ret

;; strlen(str: eax) -> len: eax
strlen:                     
    push ebx
    mov ebx, eax
 
	.nextchar:
        cmp byte [eax], 0
        jz .finished
        inc eax
        jmp .nextchar
        
	.finished:
        sub eax, ebx
        pop ebx 
        ret

my_print:
	push    ebp
	mov     edx, [esp+12]
	mov     ecx, [esp+8]
	mov     ebx, 1
	mov     eax, 4
	int     80h
	pop     ebp
	ret
