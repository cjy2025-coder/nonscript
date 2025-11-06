section .rodata
section .text
global _start

_start:
push rbp
mov rbp, rsp
sub rsp, 64
mov qword [rbp-32], 1
mov qword [rbp-40], 2
mov qword [rbp-48], 3
mov rax, [rbp-32]
mov rbx, [rbp-56]
add rax, rbx
mov [rbp-64], rax
mov rax, [rbp-64]
mov [rbp-8], rax
mov rsp, rbp
pop rbp
mov rax, 60
xor rdi, rdi
syscall
