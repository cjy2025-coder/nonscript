section .rodata
int_fmt db "%lld", 0
str_fmt db "%s", 0
newline db 10, 0
scanf_fmt db "%lld", 0
str0 db "hello", 0
str1 db "hell", 0

section .bss
default rel
section .text
global main
extern printf
extern scanf
extern malloc

f:
  push rbp
  mov rbp, rsp
  sub rsp, 32          ; 预留 shadow space
  lea rdx, [str0]      ; 第二个参数：字符串
  lea rcx, [rel str_fmt] ; 第一个参数：格式串
  call printf
  add rsp, 32
  mov rsp, rbp
  pop rbp
  ret

main:
  push rbp
  mov rbp, rsp

  ; 调用 f
  sub rsp, 32
  call f
  add rsp, 32

  ; 输出 "hell"
  sub rsp, 32
  lea rdx, [str1]
  lea rcx, [rel str_fmt]
  call printf
  add rsp, 32

  ; 输出 43
  sub rsp, 32
  mov rdx, 43
  lea rcx, [rel int_fmt]
  call printf
  add rsp, 32

  xor eax, eax         ; 返回值 0
  mov rsp, rbp
  pop rbp
  ret