// This file is part of www.nand2tetris.org
// and the book "The Elements of Computing Systems"
// by Nisan and Schocken, MIT Press.
// File name: projects/04/Fill.asm

// Runs an infinite loop that listens to the keyboard input.
// When a key is pressed (any key), the program blackens the screen,
// i.e. writes "black" in every pixel;
// the screen should remain fully black as long as the key is pressed. 
// When no key is pressed, the program clears the screen, i.e. writes
// "white" in every pixel;
// the screen should remain fully clear as long as no key is pressed.

// Put your code here.
    @n
    M=0

    @32
    D=A
    @R0
    M=D

    // 512*256/16
    @8192
    D=A
    @R1
    M=D

(LOOP)
    // n이 최대치(8192)이면 0으로 초기화
    @n
    D=M

    @R1
    D=M-D
    @RESET
    D;JEQ


    // 키 입력에 따른 분기문
    @24576
    D=M
    @FILLW
    D;JEQ

    @FILLB
    0;JMP

    @LOOP
    0;JMP

(RESET)
    @n
    M=0
    @LOOP
    0;JMP

(FILLB)
    @SCREEN
    D=A
    
    @n
    A=D+M
    M=-1

    @n
    M=M+1
    D=M

    @LOOP
    0;JMP

(FILLW)
    @SCREEN
    D=A
    
    @n
    A=D+M
    M=0

    @n
    M=M+1
    D=M

    @LOOP
    0;JMP