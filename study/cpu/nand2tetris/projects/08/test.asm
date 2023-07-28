// push constant 10
@10
D=A
@SP
A=M
M=D
@SP
M=M+1

@SP
M=M-1
A=M
D=M

@THIS
M=D

// label LOOP_START
(LOOP_START)



@SP //pop sp
M=M-1
A=M
D=M
@LOOP_START
D;JNE


// LCL 임시 변수에 저장
@LCL
D=M
@R14
M=D

// return addr 임시 저장
@5
D=A
@R14
A=M-D
D=M
@R15
M=D

@Sys.init$ret.0
D=A
@SP
A=M
M=D
@SP
M=M+1

(Main.fibonacci)
0;JMP
(Sys.init$ret.0)