import sys
import re

A_INSTRUCTION = 1
C_INSTRUCTION = 2
L_INSTRUCTION = 3

global base_addr
base_addr = 16
comp_list = {
    "0":"0101010", "1":"0111111", "-1":"0111010",
    "D":"0001100", "A":"0110000", "!D":"0001101",
    "!A":"0110001", "-D":"0001111", "-A":"0110011",
    "D+1":"0011111", "A+1":"0110111", "D-1":"0001110",
    "A-1":"0110010", "D+A":"0000010", "D-A":"0010011",
    "A-D":"0000111", "D&A":"0000000", "D|A":"0010101",
    "M":"1110000", "!M":"1110001", "-M":"1110011",
    "M+1":"1110111", "M-1":"1110010", "D+M":"1000010",
    "D-M":"1010011", "M-D":"1000111", "D&M":"1000000",
    "D|M":"1010101"
}

dest_list = {
    "M":"001", "D":"010", "DM":"011", "MD":"011",
    "A":"100", "AM":"101", "MA":"101", "DA":"110",
    "AD":"110", "ADM":"111"
}

jump_list = {
    "JGT":"001", "JEQ":"010", "JGE":"011",
    "JLT":"100", "JNE":"101", "JLE":"110", "JMP":"111"
}

default_symbol_list = {
    "R0":"0", "R1":"1", "R2":"2", "R3":"3", "R4":"4",
    "R5":"5", "R6":"6", "R7":"7", "R8":"8", "R9":"9",
    "R10":"10", "R11":"11", "R12":"12", "R13":"13", "R14":"14",
    "R15":"15", "SP":"0", "LCL":"1", "ARG":"2", "THIS":"3", "THAT":"4",
    "SCREEN":"16384", "KBD":"24576"
}

def has_more_lines(cmd:str):
    if cmd == "":
        return False
    return True

def is_comment_or_newline(cmd:str):
    if cmd == "\n":
        return True
    elif cmd.startswith("//"):
        return True

def instruction_type(cmd:str):
    if cmd.startswith("@"):
        return A_INSTRUCTION
    elif cmd.find("="):
        return C_INSTRUCTION
    return L_INSTRUCTION

def get_symbol(cmd:str, ins_type):
    if ins_type == A_INSTRUCTION:
        sym = cmd[1:].replace("\n", "")
        return sym

def decoding_c_command(cmd:str, ins:dict):
    # 공백제거, 뒤에 추가로 달린 주석 제거
    off_1 = cmd.find("=")
    off_2 = cmd.find(";")

    # case 1 : D = C ; J, case 2 : D = C, case 3 : D ; J
    # = 처리
    if off_1:
        dest = cmd[:off_1]
        comp = cmd[off_1+1:].replace("\n", "")
        ins["dest"] = dest
        ins["comp"] = comp

        # = 이후 ; 나올 시 처리
        if off_2 > 0:
            jump = cmd[off_2+1:].replace("\n", "")
            comp = cmd[off_1+1:off_2]
            ins["comp"] = comp
            ins["jump"] = jump

    # ; 처리
    elif off_2:
        comp = cmd[:off_2]
        jump = cmd[off_1+1:].replace("\n", "")
        ins["comp"] = comp
        ins["jump"] = jump

def set_symbol(sym:str, sym_list:dict, jum_list:dict):
    # 두 딕셔너리에서 확인
    if ((sym_list.get(sym) != None) or (jum_list.get(sym) != None) or (default_symbol_list.get(sym) != None) or (sym[0].isdigit() == True)):
        return
    
    # 없으면 점프 주소인지 확인, 있으면 해당 주소 저장 없으면 심볼 리스트에 저장
    x = "(" + sym + ")"
    i = 0
    with open(asm_file, "r") as fp:
        while True:
            y = fp.readline()
            if not y:
                break
            y = y.replace("\n", "")
            if x == y:
                jum_list[sym] = str(i)
                return
            
            if y[0] != "(":
                i += 1
    
    global base_addr
    
    sym_list[sym] = str(base_addr)
    base_addr += 1
    print(sym_list)

def decoding(cmd:str, ins_type:int, sym:str, ins:dict, sym_list:dict, jum_list:dict, fp):
    if ins_type == A_INSTRUCTION:
        # 그냥 숫자일 경우 이진수 그대로 저장
        if sym[0].isdigit():
            bin = str(format(int(sym), "b")).zfill(16)
        
        # 저장된 심볼이 있을 경우 해당 값을 저장
        else:
            addr_d = default_symbol_list.get(sym)
            addr_s = sym_list.get(sym)
            addr_j = jum_list.get(sym)
            if addr_d != None:
                bin = str(format(int(addr_d), "b")).zfill(16)
            elif addr_s != None:
                bin = str(format(int(addr_s), "b")).zfill(16)
            elif addr_j != None:
                bin = str(format(int(addr_j), "b")).zfill(16)
        fp.write(bin+"\n")

    elif ins_type == C_INSTRUCTION:
        # comp는 무조건 존재
        comp = comp_list.get(ins["comp"])
        if comp == None:
            comp = "0000000"

        # dest, jump는 없을 경우 0으로 채움
        dest = dest_list.get(ins["dest"])
        if dest == None:
            dest = "000"

        jump = jump_list.get(ins["jump"])
        if jump == None:
            jump = "000"
        
        c_com = "111" + comp + dest + jump
        # print(c_com+"\n\n")
        fp.write(c_com+"\n")
    
    # elif ins_type == L_INSTRUCTION:
        

asm_file = sys.argv[1]
if len(sys.argv) != 2:
    print("hackass.py [assembler file]")
    sys.exit()

f = open(asm_file, "r")
w = open("out.hack", "w")
symbol_list = {}
jump_symbol_list = {}

while True:
    instruction = {"dest":"", "comp":"", "jump":""}
    symbol = ""
    command = f.readline().replace(" ", "")
    command = re.sub(r"\/\/.+", "", command)
    if not command:
        break

    if command[0] == "(":
        continue

    if has_more_lines(command) == False:
        break
    
    if is_comment_or_newline(command) == True:
        continue

    ins_type = instruction_type(command)
    if ((ins_type == A_INSTRUCTION) or (ins_type == L_INSTRUCTION)):
        symbol = get_symbol(command, ins_type)
        set_symbol(symbol, symbol_list, jump_symbol_list)
        
    elif ins_type == C_INSTRUCTION:
        decoding_c_command(command, instruction)
    
    if ins_type:
        decoding(command, ins_type, symbol, instruction, symbol_list, jump_symbol_list, w)


w.close()
f.close()