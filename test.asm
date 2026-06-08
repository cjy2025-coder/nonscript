section .rodata
int_fmt db "%lld", 0
str_fmt db "%s", 0
newline db 10, 0
scanf_fmt db "%lld", 0
str0 db "1", 0
str1 db "1", 0
str2 db "2", 0
str3 db "!1", 0

section .bss

default rel
section .text
global main
extern printf
extern scanf
extern malloc
extern calloc

main:
  push rbp
  mov rbp, rsp
  sub rsp, 112
  lea rax, [str0]
  mov [rbp-8], rax
  mov rax, [rbp-8]
  mov [rbp-24], rax
  lea rax, [str1]
  mov [rbp-32], rax
  mov rax, [rbp-24]
  cmp rax, [rbp-32]
  setne al
  movzx rax, al
  mov [rbp-40], rax
  mov rax, [rbp-40]
  cmp rax, 0
  jne .L2
  mov rax, 1
  mov [rbp-48], rax
  sub rsp, 32
  mov rdx, [rbp-48]
  lea rcx, [rel int_fmt]
  call printf
  add rsp, 32
  jmp .L1
.L2:
  lea rax, [str2]
  mov [rbp-72], rax
  mov rax, [rbp-24]
  cmp rax, [rbp-72]
  setne al
  movzx rax, al
  mov [rbp-88], rax
  mov rax, [rbp-88]
  cmp rax, 0
  jne .L3
  jmp .L1
.L3:
  lea rax, [str3]
  mov [rbp-96], rax
  sub rsp, 32
  mov rdx, [rbp-96]
  lea rcx, [rel str_fmt]
  call printf
  add rsp, 32
.L1:
  mov rsp, rbp
  pop rbp
  ret

