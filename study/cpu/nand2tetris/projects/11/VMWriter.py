class VMWriter:
    def __init__(self, output):
        self.fp = open(output+".vm", "w")

    def __del__(self):
        self.fp.close()
    
    # VM push 명령 기록
    # segment(CONSTANT, ARGUMENT, LOCAL, STATIC, THIS, THAT, POINTER, TEMP)
    def write_push(self, segment:str, index:int):
        cmd = "push {} {}\n".format(segment, index)
        self.fp.write(cmd)
    
    # VM pop 명령 기록
    # segment(ARGUMENT, LOCAL, STATIC, THIS, THAT, POINTER, TEMP)
    def write_pop(self, segment:str, index:int):
        cmd = "pop {} {}\n".format(segment, index)
        self.fp.write(cmd)

    # VM 산술 논리 명령 기록
    # cmd(ADD, SUB, NEG, EQ, GT, LT, AND, OR, NOT)
    def write_arithmetic(self, cmd:str):
        cmd += "\n"
        self.fp.write(cmd)

    # VM label 명령 기록
    def write_label(self):
        print
    
    # VM goto 명령 기록
    def write_goto(self):
        print
    
    # VM if-goto 명령 기록
    def write_if(self):
        print

    # vm call 명령 기록
    def write_call(self, name:str, n_vars:int):
        cmd = "{} {}\n".format(name, n_vars)
        self.fp.write(cmd)
    
    # vm function 명령 기록
    def write_function(self, name:str, n_vars:int):
        cmd = "{} {}\n".format(name, n_vars)
        self.fp.write(cmd)

    # vm return 명령 기록
    def write_return(self):
        cmd = "return\n"
        self.fp.write(cmd)
    