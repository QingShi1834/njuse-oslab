SECTION .data
msg1        db      'Please enter the 被除数: ', 0h      ; 用于提示用户输入
msg2        db      'Please enter the 除数: ', 0h        ; 用于与用户打招呼
msg3        db      '商是:',0h
msg4        db      '余数是:',0h
msg5	    db	     '除数是0，发生错误，请重试',0h

j           db      0
alen        db      0
blen        db      0
clen        db      0
num         db      0       ; 在pring_array中作为中介用于输出

SECTION .bss
beiChuShuStr:     resb    255                                 ; 预留255字节空间来保存用户输入
ChuShuStr:        resb    255
c:                resb    255
d:                resb    255
e:                resb    255  
i resb 1
temp resb 1
SECTION .text
global  _start
global main
 
_start:

    mov     eax, msg1       ; 将提示字符串移入EAX
    mov     ecx, beiChuShuStr     ; 缓冲变量地址 (buffer)
    call    getStr ; the args are eax and ecx

    mov     eax, msg2       
    mov     ecx, ChuShuStr
    call    getStr

    mov     eax, beiChuShuStr     ; 将用户输入移入EAX（注意：输入包含换行符）
    ;call    sprintLF          ; 调用打印函数
 
    call    cleanRigister

    cmp     byte[ChuShuStr],48
    je      errorQuit

    mov     eax, beiChuShuStr
    call    strlen
    mov     byte[alen], al
    dec     byte[alen]

    mov     eax, ChuShuStr
    call    strlen
    mov     byte[blen], al
    dec     byte[blen]                  ; 去掉最后一个换行符的计数

    mov     byte[i],0
    call    cleanRigister

convert_a:
    mov     ebx, beiChuShuStr
    ; edx 已经被清空了
    push    edx 
    xor     edx,edx
    mov     dl, byte[i]                 ; 向edx中放入内容i
    add     ebx, edx                    ; ebx 存储了a[i]的地址
    pop     edx
    mov     al,byte[ebx]                ; al中存储了ebx的地址所存储的内容 即 a[i]
    sub     al,48                       ; a[i] - '0'
    xor     edx,edx
    mov     dl, byte[alen]              ; dl中存储了alen中的值
    
    sub     dl, byte[i]                 ; dl = alen - i
    sub     dl, 1                       ; dl = alen - i - 1
                                        ; edx中包含了dl
    add     edx, d                      ; edx中存储了d + alen - i - 1的地址
    mov     byte[edx], al               ; d + edx 因为地址是32位的
    inc     byte[i]
    mov     cl, byte[i]
    cmp     cl,byte[alen]
    jne     convert_a

    call    cleanRigister
    mov     byte[i],0



convert_b:
    mov     ebx, ChuShuStr
    ; edx 已经被清空了
    push    edx 
    xor     edx,edx
    mov     dl, byte[i]                 ; 向ebx中放入内容i
    add     ebx, edx                    ; ebx 存储了b[i]的地址
    pop     edx
    mov     al,byte[ebx]                ; al中存储了ebx的地址所存储的内容 即 b[i]
    sub     al,48                       ; b[i] - '0'
    xor     edx,edx
    mov     dl, byte[blen]
    sub     dl, byte[i]
    sub     dl, 1                       ; dl = blen - i - 1
    
    add     edx, e
    mov     byte[edx], al               ; e + edx 因为地址是32位的
    inc     byte[i]
    mov     cl, [i]
    cmp     cl,[blen]
    jne     convert_b

    call    cleanRigister
    mov     byte[i],0

;-------------------------- 为进入divide的for循环做准备

    call    cleanRigister
    
    mov     al,[alen]
    sub     al,[blen]
    add     al,1
    mov     byte[clen],al           ; clen = alen - blen + 1



divide:
    call    cleanRigister
    mov     al,byte[clen]
    sub     al,1
    mov     byte[i],al
    loop_divide:
        cmp     byte[i],0
        jl      out_divide_loop
        call    cleanRigister
        jmp     equal
    equal:

        mov     al,byte[i]              ; eax中存i的值
        add     al,byte[blen]           ; eax 中存i + ben的值
        add     eax,d                   ; eax中存d[i + blen]的地址
        cmp     byte[eax],0
        jne     in_while                ; 不等于时跳转
        
        
        xor     eax,eax
        mov     al,[blen]
        sub     al,1 ; al中存blen-1
        mov     byte[j],al                  ; j = blen - 1
        xor     eax,eax
        cmp     byte[j],0
        jge     loop_equal              ; 大于或等于跳转
        jmp     in_while                ; 默认进入while循环
        loop_equal:
            push    ebx
            push    ecx
            push    edx
            call    cleanRigister
            mov     bl,byte[i]
            add     bl,byte[j]              ; bl = i + j
            add     ebx,d                   ; ebx 中存d[i + j]的地址
            mov     cl,byte[j]              ; ecx中存j的值
            add     ecx,e                   ; ecx 中存e[j] 的地址
            mov     edx,[ecx]               ; edx 中存e[j]的值
            ; if (d[i + j] > e[j]) return true
            cmp     [ebx],edx
            jg      in_while                ; d[i + j] > e[j] 则进入while循环
            ; if (d[i + j] < e[j]) return false
            cmp     [ebx],edx
            jl      out_while               ; d[i + j] < e[j] 则不进入while循环
            call    cleanRigister
            pop     edx
            pop     ecx
            pop     ebx
            dec     byte[j]
            cmp     byte[j],0

            jge     loop_equal              ; 大于或等于跳转
            jmp     in_while                ; 默认进入while循环

    in_while:
        xor     eax,eax
        mov     byte[j],0           ; for (j = 0; j < blen; j++)
    in_while_loop:
        call    cleanRigister
        mov     al,[blen] ;
        cmp     byte[j],al
        jae     out_while_for ; 跳出for循环，进行c[i]++
        mov     bl,byte[i]
        add     bl,byte[j]          ; bl = i + j
        add     ebx,d               ; ebx 中存d[i + j]的地址
        mov     cl,byte[j]          ; ecx中存j的值
        add     ecx,e               ; ecx 中存e[j] 的地址
        mov     dl,[ecx]           ; edx 中存e[j]的值
        ; 要用dl存储
        ; 此处：用sub命令可能会出问题
        sub     byte[ebx],dl
        inc     byte[j]             ; j++
        cmp     byte[ebx],0
        jge     in_while_loop       ; 有符号数的比较
        


        add     byte[ebx],10
        add     ebx,1               ; ebx 存d[i + j + 1]的地址
        dec     byte[ebx]               ; d[i + j + 1]--
        ; 需要转换成字符或者完善成打印出整数数组
        jmp     in_while_loop
    out_while_for:
        ; c[i] ++
        push    eax
        xor     eax,eax
        mov     al,byte[i]
        add     eax,c               ; eax中存的是c[i]的地址
        inc     byte[eax]               ; c[i]++
        pop     eax
        jmp     loop_equal          ; 继续进入while中判断equal是否成立

            ; 应当还少了一层while嵌套
    out_while:
        ; 完成循环所需要的外部修改
        dec     byte[i]
        jmp     loop_divide
        call    quit


        ; mov     esi,0
        ; mov     ecx,255
        ; add     esi,d
        ; add     esi,[alen]
        ; sub     esi,1

out_divide_loop:

    mov     eax,msg3
    call    sprint
    mov     esi,c
    call    print_array
    call    cleanRigister
    mov     eax,msg4
    call    sprint
    xor     esi,esi  
    mov     esi,d
    call    print_array
    call    quit


getStr:
    call    sprint          ; 打印提示字符串
    mov     edx, 255        ; 要读取的字符串字节数
    mov     ebx, 0          ; write to the STDIN file
    mov     eax, 3          ; 调用SYS_READ (OPCODE为3)
    int     80h
    ret

cleanRigister:
    xor     eax,eax
    xor     ebx,ebx
    xor     ecx,ecx
    xor     edx,edx
    ret
    
; esi: 存储数组的下标
; i:循环计数器
print_array:
    call cleanRigister
    mov ebx, 1           ; 文件描述符stdout
    mov byte[i], 0
    mov byte[i], byte(100)
    add esi,[i]        
    ; 调用时，esi传入了起始地址，从后往前输出

skip_zero:

    ; 取出数组元素，将其转换为ASCII码并输出
    mov al, byte[esi]       ; 取出d[i]
    cmp al, 0
    jne loop_array
    dec esi
    dec byte[i]
    cmp byte[i],0
    jge skip_zero

loop_array:

    ; 取出数组元素，将其转换为ASCII码并输出
    xor eax,eax
    mov al, [esi]       ; 取出d[i]
    add al, 48         ; 转换为ASCII码
    ;mov byte[num], al       ; 存储ASCII码形式的数字
    xor ecx, ecx
    mov byte[num], al         ; 存储数字to ecx
    mov ecx, num
    mov edx, 1          ; get the length to edx
    mov eax, 4          ; 使用write系统调用
    mov ebx, 1          ; 文件描述符stdout
    int 0x80            ; 执行系统调用
    ; 系统调用后会修改eax的内容
    ; 更新循环计数器和数组指针
    dec esi
    dec byte[i]
    ; 0 - 1 = -1 = 255 ?
    cmp byte[i],0
    ; 判断循环是否结束
    jge loop_array

    xor eax,eax
    xor ebx,ebx
    xor ecx,ecx
    xor edx,edx
    mov byte[num], 0Ah         ; 存储数字to ecx
    mov ecx, num
    mov edx, 1          ; get the length to edx
    mov eax, 4          ; 使用write系统调用
    mov ebx, 1          ; 文件描述符stdout
    int 0x80            ; 执行系统调用
    call cleanRigister
    mov byte[num],0
    ret
    
    ;------------------------------------------
; int strlen(String message)  返回值保存在EAX中
; String length calculation function
strlen:					   ; 返回值保存在EAX中
    push    ebx  		   ; 将EBX中的值保存于栈上，因为strlen会使用该寄存器			
    mov     ebx, eax  	   ; 将EAX中msg的地址移EBX（现在二者指向内存中同一处）
 
nextchar:				   ; 与 Lesson3 中相同
    cmp     byte [eax], 0  ; 比较当前EAX地址处的内容是否为字符串结尾'\n' -- 出现了段错误
    ; 如果判断是否结尾是0，就不会有段错误
    je      finished       ; ZF为1，跳出循环到finished
    inc     eax            ; ZF不为1，EAX中的地址递增
    jmp     nextchar       ; 继续循环
 
finished:
    sub     eax, ebx       ; EBX - EAX，长度保存在EAX中
    pop     ebx            ; 将栈上之前保存的值pop回EBX
    ret                    ; 返回函数调用处
 
 
;------------------------------------------
; void sprint(String message)
; String printing function
sprint:
    push    edx			; 将EDX中的值保存于栈上
    push    ecx			; 将ECX中的值保存于栈上
    push    ebx			; 将EBX中的值保存于栈上
    push    eax			; 将EAX中的值保存于栈上，即参数string
    call    strlen		; 计算EAX中字符串长度，保存在EAX中
 
    mov     edx, eax	; 将长度移入到EDX
    pop     eax			; 恢复EAX值，即参数string
 
    mov     ecx, eax	; 将待打印string移入ECX
    mov     ebx, 1		; 表示写入到标准输出STDOUT
    mov     eax, 4		; 调用SYS_WRITE（操作码是4）
    int     80h
 
    pop     ebx			; 恢复原来EBX中的值
    pop     ecx			; 恢复原来ECX中的值
    pop     edx			; 恢复原来EDX中的值
    ret
    
    
;------------------------------------------
; void sprintLF(String message)
; String printing with line feed function
sprintLF:
    call    sprint
 
    push    eax         ; 将EAX中的值保存于栈上，即参数string
    mov     eax, 0Ah    ; 将换行符0Ah移入EAX
    push    eax         ; 将换行符0Ah入栈，这样可以获取其地址
    mov     eax, esp    ; 将当前栈指针ESP中的地址（指向0Ah）移入EAX
    call    sprint      ; 调用sprint打印换行符
    pop     eax         ; 换行符退栈
    pop     eax         ; 恢复调用该函数前EAX中的值
    ret                 ; 返回调用处
 
 
;------------------------------------------
; void quit()
; Exit program and restore resources
quit:
    mov     ebx, 0		; exit时返回状态0 - 'No Errors'
    mov     eax, 1		; 调用SYS_EXIT（OPCODE是1）
    int     80h
    ret

errorQuit:
    mov     eax, msg5
    call    sprintLF
    call    quit
    ret

