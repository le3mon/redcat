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


// (RESET)
//     @SCREEN
//     D=M
//     @R0
//     M=D

(LOOP)
    @SCREEN
    D=A
    
    @n
    A=D+M
    M=-1


    @R0
    A=D+M
    M=-1

    // @64
    // A=D+M
    // M=-1

    @n
    M=M+1

    @LOOP
    0;JMP