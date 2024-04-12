    org 07c00h    ; 告诉编译器，将这个程序加载到内存地址 0x7c00 开始。
    mov ax, cs    ; 将当前代码段的段地址（即 cs 寄存器中的值）放入 ax 寄存器中。
    mov ds, ax    ; 将 ax 寄存器中的值（即当前代码段的段地址）放入数据段寄存器 ds 中。
    mov es, ax    ; 将 ax 寄存器中的值（即当前代码段的段地址）放入附加段寄存器 es 中。
    call DispStr  ; 调用 DispStr 子程序。
    jmp $         ; 死循环
    
DispStr:
    mov ax, BootMessage  ; 将消息字符串 BootMessage 的地址放入 ax 寄存器中。
    mov bp, ax           ; 将 ax 寄存器中的值（即 BootMessage 的地址）放入基址寄存器 bp 中。
    mov cx, 10           ; 将计数器寄存器 cx 的值设为 10
    mov ax, 01301h       ; 将显示字符串的BIOS中断号 0x13 和显示模式 0x01 放入 ax 寄存器中。
    mov bx, 000ch        ; 将显示页面号 0x000c 放入 bx 寄存器中。
    mov dl, 0            ; 将显示颜色的 dl 寄存器的值设为 0
    int 10h              ; 调用BIOS中断 0x10，显示字符串
    ret                  ; 返回到调用 DispStr 子程序的位置
    
BootMessage: db "Hello, OS!" ; 定义消息字符串 BootMessage，内容为“Hello, OS!”。
times 510-($-$$) db 0    ; 在 BootMessage 之后填充 0，使整个程序长度为 512 字节。
dw 0xaa55                ; 在程序末尾添加引导扇区标识符 0xaa55，用于让BIOS识别该程序为可引导扇区
