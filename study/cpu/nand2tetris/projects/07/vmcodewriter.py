C_PUSH = 2
C_POP = 3
global idx
idx = 0
def write_push_pop(type:int, segment:str, index:int, fp):
    if type == C_PUSH:
        if segment == "constant":
            # 상수 처리 코드
            code = "@"+index+"\nD=A\n"
            
            # 스택에 push 코드
            code += "@SP\nA=M\nM=D\n@SP\nM=M+1\n"
            
            fp.write(code)

def write_arithemetic(cmd:str, fp):
    global idx
    if ((cmd == "neg") or (cmd == "not")):
        code = "@SP\nM=M-1\nA=M\nD=M\n"
        if cmd == "neg":
            code += "M=-D\n"
        elif cmd == "not":
            code += "M=!D\n"
        code += "@SP\nM=M+1\n"
    else :
        code = "@SP\nM=M-1\nA=M\nD=M\n@SP\nM=M-1\nA=M\n"
        if ((cmd == "eq") or (cmd == "gt") or (cmd == "lt")):
            if cmd=="eq":
                jump = "JEQ\n"
            elif cmd=="gt":
                jump = "JLT\n"
            elif cmd=="lt":
                jump = "JGT\n"
            cond = "COND"+str(idx)
            idx += 1
            code += "D=D-M\n"
            code += "@"+cond+".t\n"
            code += "D;"+jump
            code += "@SP\nA=M\nM=0\n@SP\nM=M+1\n"
            code += "@"+cond+".g\n"
            code += "0;JMP\n"
            code += "("+cond+".t)\n"
            code += "@SP\nA=M\nM=-1\n@SP\nM=M+1\n"
            code += "("+cond+".g)\n"
        
        elif ((cmd == "add") or (cmd == "sub")):
            if cmd=="add":
                code += "D=D+M\n"
            if cmd=="sub":
                code += "D=M-D\n"
            code += "@SP\nA=M\nM=D\n@SP\nM=M+1\n"
    
        elif ((cmd == "and") or (cmd == "or")):
            if cmd == "and":
                code += "D=D&M\n"
            elif cmd == "or":
                code += "D=D|M\n"
            code += "@SP\nA=M\nM=D\n@SP\nM=M+1\n"
        
    fp.write(code)

def write_end(fp):
    code = "(END)\n@END\n0;JMP\n"
    fp.write(code)