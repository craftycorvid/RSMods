IFNDEF RAX
.MODEL FLAT, C
ENDIF
 
.CODE
 
NtProtectVirtualMemory PROC
    mov eax, 50h
    call KiSystemCall
    ret
NtProtectVirtualMemory ENDP

NtQuerySystemInformation PROC
    mov eax, 36h
    call KiSystemCall
    ret
NtQuerySystemInformation ENDP
 
NtQueryVirtualMemory PROC
    mov eax, 23h
    call KiSystemCall
    ret
NtQueryVirtualMemory ENDP
 
KiSystemCall PROC
IFDEF RAX
    mov r10,rcx
    syscall
    ret
ELSE
    db 234
    dd exit
    dw 51
exit:
    inc ecx 
    jmp dword ptr [edi+248]
ENDIF
KiSystemCall ENDP
 
END