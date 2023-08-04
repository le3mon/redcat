C_PUSH = 2
C_POP = 3
TEMP_BASE = 5
global cond_idx, call_idx
global ret_sym
global static_call_num, static_base
cond_idx = 0
call_idx = 0
static_call_num = 0
ret_sym = ""
static_base = 16

seg_list = ["LCL", "ARG", "THIS", "THAT"]

def write_bootstrap(fp):
    code = "@261\nD=A\n@SP\nM=D\n"
    # code = "@256\nD=A\n@SP\nM=D\n"
    code += "@Sys.init\n0;JMP\n"
    fp.write(code)
    # write_call("Sys.init", "0", fp)
    # fp.write(code)

def static_control():
    global static_call_num, static_base
    if static_call_num > 0:
        static_base += 1 + static_call_num
    
    static_call_num = 0

def write_push_pop(type:int, segment:str, index:str, fp):
    global static_call_num, static_base
    if type == C_PUSH:
        code = "@{}\nD=A\n".format(index)
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
            code = "@{}\nD=M\n".format(str(i))
        elif segment == "static":
            i = int(index) + static_base
            code = "@{}\nD=M\n".format(str(i))
            static_call_num += 1
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
            code = "@{}\nD=A\n".format(str(i))
        elif segment == "static":
            i = int(index) + static_base
            code = "@{}\nD=A\n".format(str(i))
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
    global cond_idx
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
            cond = "COND"+str(cond_idx)
            cond_idx += 1
            code += "D=D-M\n"
            code += "@{}.t\n".format(cond)
            code += "D;"+jump
            code += "@SP\nA=M\nM=0\n@SP\nM=M+1\n"
            code += "@{}.g\n".format(cond)
            code += "0;JMP\n"
            code += "({}.t)\n".format(cond)
            code += "@SP\nA=M\nM=-1\n@SP\nM=M+1\n"
            code += "({}.g)\n".format(cond)
        
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

def write_label(cmd:str, fp):
    code = "({})\n".format(cmd)
    fp.write(code)

def write_if(cmd:str, fp):
    code = "@SP\nM=M-1\nA=M\nD=M\n"
    code += "@{}\n".format(cmd)
    code += "D;JNE\n"
    fp.write(code)

def write_goto(cmd:str, fp):
    code = "@{}\n".format(cmd)
    code += "0;JMP\n"
    fp.write(code)

def write_call(cmd:str, index:str, fp):
    global call_idx, ret_sym
    
    r_s = "{}$ret.{}".format(ret_sym, call_idx)
    call_idx += 1
    # push return address
    code = "@{}\nD=A\n".format(r_s)
    code += "@SP\nA=M\nM=D\n@SP\nM=M+1\n"

    # push lcl ~ that
    for i in range(0, 4):
        code += "@{}\nD=M\n".format(seg_list[i])
        code += "@SP\nA=M\nM=D\n@SP\nM=M+1\n"

    # ARG = SP - 5 - nArgs
    code += "@{}\nD=A\n@SP\nD=M-D\n@ARG\nM=D\n".format(5+int(index))
    code += "@SP\nD=M\n@LCL\nM=D\n"
    
    # goto function
    code += "@{}\n0;JMP\n".format(cmd)

    # set return address
    code += "({})\n".format(r_s)

    fp.write(code)

def write_function(cmd:str, index:str, fp):
    global call_idx, ret_sym
    call_idx = 0
    ret_sym = cmd
    code = "({})\n".format(cmd)
    # push 0 * 인자 갯수
    for i in range(int(index)):
        code += "@SP\nA=M\nM=0\n@SP\nM=M+1\n"
    fp.write(code)

def write_return(fp):
    # LCL 임시 변수 저장
    code = "@LCL\nD=M\n@R14\nM=D\n"
    
    # return addr 임시 변수에 저장
    code += "@5\nD=A\n@R14\nA=M-D\nD=M\n@R15\nM=D\n"
    
    # return 값 ARG*에 저장
    code += "@SP\nM=M-1\nA=M\nD=M\n@ARG\nA=M\nM=D\n"

    # SP = ARG + 1
    code += "@ARG\nD=M\n@SP\nM=D+1\n"

    # THAT ~ LCL 복구
    for i in range(1, 5):
        code += "@{}\nD=A\n@R14\nA=M-D\nD=M\n".format(i)
        code += "@{}\nM=D\n".format(seg_list[5-i-1])
    
    # goto return addr
    code += "@R15\nA=M\n0;JMP\n"
    fp.write(code)

def write_end(fp):
    code = "(END)\n@END\n0;JMP\n"
    fp.write(code)