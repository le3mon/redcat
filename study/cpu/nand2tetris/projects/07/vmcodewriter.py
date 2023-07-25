C_PUSH = 2
C_POP = 3
TEMP_BASE = 5
STATIC_BASE = 16
global idx
idx = 0

def write_push_pop(type:int, segment:str, index:int, fp):
    if type == C_PUSH:
        code = "@"+index+"\nD=A\n"
        # if segment == "constant":
        if segment == "local":
            code += "@LCL\nA=M+D\nD=M\n"
        elif segment == "argument":
            code += "@ARG\nA=M+D\nD=M\n"
        elif segment == "this":
            code += "@THIS\nA=M+D\nD=M\n"
        elif segment == "that":
            code += "@THAT\nA=M+D\nD=M\n"
        elif segment == "temp":
            i = int(index) + TEMP_BASE
            code = "@"+str(i)+"\nD=M\n"
        elif segment == "static":
            i = int(index) + STATIC_BASE
            code = "@"+str(i)+"\nD=M\n"
        elif segment == "pointer":
            if index == "0":
                code = "@THIS\nD=M\n"
            elif index == "1":
                code = "@THAT\nD=M\n"
        code += "@SP\nA=M\nM=D\n@SP\nM=M+1\n"
            
    elif type == C_POP:
        code = "@"+index+"\nD=A\n"
        if segment == "local":
            code += "@LCL\nD=M+D\n"
        elif segment == "argument":
            code += "@ARG\nD=M+D\n"
        elif segment == "this":
            code += "@THIS\nD=M+D\n"
        elif segment == "that":
            code += "@THAT\nD=M+D\n"
        elif segment == "temp":
            i = int(index) + TEMP_BASE
            code = "@"+str(i)+"\nD=A\n"
        elif segment == "static":
            i = int(index) + STATIC_BASE
            code = "@"+str(i)+"\nD=A\n"
        # 우선 r14 레지스터를 임시로 사용하지만 문제 발생 시 변경 필요
        code += "@R14\nM=D\n"
        code += "@SP\nM=M-1\nA=M\nD=M\n"
        code += "@R14\nA=M\nM=D\n"
        if segment == "pointer":
            code = "@SP\nM=M-1\nA=M\nD=M\n"
            if index == "0":
                code += "@THIS\n"
            elif index == "1":
                code += "@THAT\n"
            code += "M=D\n"

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