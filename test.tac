===== Function: main =====
  LDC t1, "1"
  MOV a, t1
  LDC t2, "1"
  CMP_NE t3, a, t2
  JNE, t3, .L2
  LDC t4, 1
  PRINT, t4, int
  JMP, .L1
.L2:
  LDC t6, "2"
  CMP_NE t7, a, t6
  JNE, t7, .L3
  JMP, .L1
.L3:
  LDC t8, "!1"
  PRINT, t8, str
.L1:
  RET
