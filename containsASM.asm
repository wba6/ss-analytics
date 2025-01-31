; -----------------------------------------
; File: contains_linux64.asm
; A function 'contains(const char* s1, const char* s2)'
; that returns 1 if s1 contains s2, otherwise 0.
; System V AMD64 ABI (Linux 64-bit)
; -----------------------------------------

global containsASM

section .text

containsASM:
    ; Prologue
    push    rbp
    mov     rbp, rsp

    ; Move s1 and s2 into some scratch registers:
    ; s1 is in RDI, s2 is in RSI
    mov     rdx, rdi    ; s1
    mov     rcx, rsi    ; s2

    ; Quick check: if s2 is empty, return 1
    cmp     byte [rcx], 0
    je      .return_one

.outer_loop:
    ; Check if we've reached the end of s1
    cmp     byte [rdx], 0
    je      .return_zero

    ; Save the current pointers on the stack
    push    rdx
    push    rcx

.inner_loop:
    mov     al, [rdx]
    mov     bl, [rcx]

    ; If we reached the end of s2 => full match
    cmp     bl, 0
    je      .found_substring

    ; If mismatch or s1 ended
    cmp     al, 0
    je      .not_matched
    cmp     al, bl
    jne     .not_matched

    ; Characters matched, keep checking
    inc     rdx
    inc     rcx
    jmp     .inner_loop

.found_substring:
    ; Found the entire s2 in s1
    pop     rcx
    pop     rdx
    jmp     .return_one

.not_matched:
    ; Mismatch; restore pointers, move s1 forward by 1 and try again
    pop     rcx
    pop     rdx
    inc     rdx
    jmp     .outer_loop

.return_one:
    mov     rax, 1
    jmp     .end_contains

.return_zero:
    mov     rax, 0

.end_contains:
    pop     rbp
    ret
