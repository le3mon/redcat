import re

C_ARITHMETIC = 1
C_PUSH = 2
C_POP = 3
C_LABEL = 4
C_GOTO = 5
C_IF = 6
C_FUNCTION = 7
C_RETURN = 8
C_CALL = 9

arithmetic_list = ("add", "sub", "neg", "eq", "gt", "lt", "and", "or", "not")

def has_more_lines(line:str):
    # 공백과 주석 제거 후 반환
    if not line:
        return False
    return True

def is_comment_or_newline(line:str):
    if line == "\n":
        return True
    elif line.startswith("//"):
        return True
    return False

def advance(line:str):
    cmd = line.replace("\n", "")
    cmd = re.sub(r"\/\/.+", "", cmd)
    return cmd

def get_command_type(cmd:str):
    if cmd.startswith(arithmetic_list):
        return C_ARITHMETIC
    elif cmd.startswith("push"):
        return C_PUSH
    elif cmd.startswith("pop"):
        return C_POP
    elif cmd.startswith("label"):
        return C_LABEL
    elif cmd.startswith("goto"):
        return C_GOTO
    elif cmd.startswith("if"):
        return C_IF
    elif cmd.startswith("function"):
        return C_FUNCTION
    elif cmd.startswith("return"):
        return C_RETURN
    elif cmd.startswith("call"):
        return C_CALL

def get_arg1(cmd:str, type:int):
    if type == C_ARITHMETIC:
        for i in arithmetic_list:
            if cmd.startswith(i):
                return i
    else:
        cmds = cmd.split(" ")
        return cmds[1]

def get_arg2(cmd:str):
    cmds = cmd.split(" ")
    return cmds[2]