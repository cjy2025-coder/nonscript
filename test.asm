section .rodata
int_fmt db "%lld", 0
str_fmt db "%s", 0
newline db 10, 0
scanf_fmt db "%lld", 0
str0 db "hello, i am ", 0
str1 db `\n`, 0
str2 db "Tom", 0

section .bss

default rel
section .text
global main
extern printf
extern scanf
extern malloc
extern calloc

say:
  push rbp
  mov rbp, rsp
  sub rsp, 64
  mov [rbp-8], rcx
  lea rax, [str0]
  mov [rbp-16], rax
  sub rsp, 32
  mov rdx, [rbp-16]
  lea rcx, [rel str_fmt]
  call printf
  add rsp, 32
  mov rax, [rbp-8]
  mov rdx, [rax + 0]
  mov [rbp-40], rdx
  sub rsp, 32
  mov rdx, [rbp-40]
  lea rcx, [rel str_fmt]
  call printf
  add rsp, 32
  lea rax, [str1]
  mov [rbp-56], rax
  sub rsp, 32
  mov rdx, [rbp-56]
  lea rcx, [rel str_fmt]
  call printf
  add rsp, 32
  mov rsp, rbp
  pop rbp
  ret

setName:
  push rbp
  mov rbp, rsp
  sub rsp, 32
  mov [rbp-8], rcx
  mov [rbp-16], rdx
  mov rax, [rbp-8]
  mov rdx, [rax + 0]
  mov [rbp-24], rdx
  mov rax, [rbp-8]
  mov rdx, [rbp-16]
  mov [rax + 0], rdx
  mov rsp, rbp
  pop rbp
  ret

main:
  push rbp
  mov rbp, rsp
  sub rsp, 64
  mov rdx, 64
  mov rcx, 1
  sub rsp, 32
  call calloc
  add rsp, 32
  mov [rbp-8], rax
  mov rax, [rbp-8]
  mov [rbp-24], rax
  lea rax, [str2]
  mov [rbp-32], rax
  sub rsp, 32
  mov rcx, [rbp-24]
  mov rdx, [rbp-32]
  call setName
  add rsp, 32
  mov [rbp-48], rax
  sub rsp, 32
  mov rcx, [rbp-24]
  call say
  add rsp, 32
  mov [rbp-56], rax
  mov rsp, rbp
  pop rbp
  ret

