import sys

A_INSTRUCTION = 1
C_INSTRUCTION = 2
L_INSTRUCTION = 3

def hasMoreLines(cmd:str):
    if cmd == "":
        return False
    return True

def isCommentOrNewline(cmd:str):
    if cmd == "\n":
        return True
    elif cmd.startswith("//"):
        return True

def instructionType(cmd:str):
    if cmd.startswith("@"):
        return A_INSTRUCTION
    elif cmd.find("="):
        return C_INSTRUCTION
    return L_INSTRUCTION

def getSymbol(cmd:str, ins_type):
    if ins_type == A_INSTRUCTION:
        sym = cmd[1:].replace("\n", "")
        if sym.isdigit():
            return sym

def decodingCCommand(cmd:str, ins:dict):
    dest_off_1 = cmd.find("=")
    dest_off_2 = cmd.find(";")
    
    if dest_off_1:
        dest = cmd[:dest_off_1]
    elif dest_off_2:
        dest = cmd[:dest_off_2]
    


asm_file = sys.argv[1]
if len(sys.argv) != 2:
    print("hackass.py [assembler file]")
    sys.exit()

f = open(asm_file, 'r')
symbol_list = {}
while True:
    instruction = {"dest":"", "comp":"", "jump":""}
    command = f.readline()
    if hasMoreLines(command) == False:
        break
    
    if isCommentOrNewline(command) == True:
        continue

    ins_type = instructionType(command)
    if ((ins_type == A_INSTRUCTION) or (ins_type == L_INSTRUCTION)):
        symbol = getSymbol(command, ins_type)
    
    elif ins_type == C_INSTRUCTION:
        decodingCCommand(command, instruction)

    



f.close()