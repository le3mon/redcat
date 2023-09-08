class VMWriter:
    def __init__(self, output):
        self.fp = open(output+".vm", "w")

    def __del__(self):
        self.fp.close()
    
    # VM push 명령 기록
    def write_push(self):
        print
    
    # VM pop 명령 기록
    def write_pop(self):
        print

    # VM 산술 논리 명령 기록
    def write_arithmetic(self):
        print

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
        print
    