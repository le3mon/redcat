class Tokenizer:
    def __init__(self, input_path):
        self.fp = open(input_path, "r", encoding="utf-8")
        self.com_tog = 0
        self.lines = self.data_preproc(self.fp.read())
        self.line = ""
        self.token = None
        self.token_type = None
        self.str_tog = 0
        self.bar_tog = 0

        self.keyword_list = ("class", "constructor", "function", 
                            "method", "field", "static", "var", 
                            "int", "char", "boolean", "void", 
                            "true", "false", "null", "this", 
                            "let", "do", "if", "else", 
                            "while", "return")

        self.symbol_list = ("{", "}", "(", ")", "[", "]", 
               ".", ",", ";", "+", "-", 
               "*", "/", "&", "|", "<", 
               ">", "=", "~")
    
        # self.diff_symbol_list = ("<", ">")
        
    
    def __del__(self):
        self.fp.close()

    
    def is_comment(self, line:str):
        if line.startswith("//"):
            return True
        elif line.startswith("/**"):
            self.com_tog = 1
            if line.find("*/") >= 0:
                self.com_tog = 0
            return True
        elif line.startswith("\n"):
            return True
        elif line.find("*/") >= 0:
            self.com_tog = 0
            return True
        return False

    def data_preproc(self, pre_data:str):
        data = ""
        li = []
        for line in pre_data.split("\n"):
            line = line.lstrip()
            
            if self.is_comment(line) == True:
                continue

            if line == "" or line is None:
                continue
            
            
            if self.com_tog == 1:
                continue

            idx = line.find("//")
            if idx > 0:
                line = line[:idx] + "\n"
            else:
                line += "\n"

            data += line
        
        tmp = data.split("\n")
        for i in tmp:
            li.append(i.rstrip() + "\n")
        
        # print(li)

        return li

    def has_more_tokens(self):
        if self.line is not None:
            return True
        else:
            for l in self.lines:
                if l == "\n":
                    return False
                else:
                    self.line = l
                    self.lines = self.lines[1:]
                    return True
    
    def advance(self):
        if self.has_more_tokens():
            idx_space = self.line.find(" ")
            idx_next = self.line.find("\n")
            idx_seme = self.line.find(";")
            idx_par_op = self.line.find("(")
            idx_par_clo = self.line.find(")")
            idx_par_clo_2 = self.line.rfind(")")
            idx_bar_op = self.line.find("[")
            idx_bar_clo = self.line.find("]")
            idx_dot = self.line.find(".")
            idx_dq = self.line.find("\"")
            idx_commas = self.line.find(",")
            idx_tilde = self.line.find("~")
            idx_not = self.line.find("-")
            idx_brace_op = self.line.find("{")
            idx_brace_clo = self.line.find("}")
        

            # "(" 가 현재 라인의 가장 처음일 경우 처리
            if idx_par_op == 0:
                self.token = self.line[0]
                self.line = self.line[1:]
                return
            
            # ~가 시작이면 처리
            if idx_tilde == 0:
                self.token = self.line[0]
                self.line = self.line[1:]
                return

            # -가 시작이면 처리
            if idx_not == 0 and idx_space != 1:
                self.token = self.line[0]
                self.line = self.line[1:]
                return

            # " 가 현재 라인 가장 처음일 경우 처리
            if idx_dq == 0:
                self.token = self.line[0]
                self.line = self.line[1:] 
                self.str_tog = self.str_tog ^ 1 # 해당 값으로 현재 "가 열리고 닫혔는지 확인
                return
            
            # " 가 열린 상태라면 다음 "까지 문자열을 하나의 토큰으로 처리
            if self.str_tog == 1:
                self.token = self.line[:idx_dq]
                self.line = self.line[idx_dq:]
                return
            
            # . 이 현재 라인의 가장 처음일 경우 처리
            if idx_dot == 0:
                self.token = self.line[0]
                self.line = self.line[1:]
                return

            if idx_bar_op == 0:
                # print("[ 처리 - before line : " + self.line)
                self.token = self.line[0]
                self.line = self.line[1:]
                # print("token : " + self.token)
                # print("after line : '{}'\n\n".format(self.line[:-1]))
                return

            if idx_bar_clo == 0:
                # print("] 처리 - before line : " + self.line)
                if idx_space == 1:
                    self.token = self.line[0]
                    self.line = self.line[2:]
                else:
                    self.token = self.line[0]
                    self.line = self.line[1:]
                # print("token : " + self.token)
                # print("after line : '{}'\n\n".format(self.line[:-1]))
                # print(self.token)
                # print(self.line)
                self.bar_tog = 0
                return

            # if self.bar_tog == 1:
            #     self.token = self.line[:idx_bar_clo]
            #     self.line = self.line[idx_bar_clo:]
            #     print("bar tog : " + self.token)
            #     return
            
            if idx_par_clo == 0 and self.line.find(")", 1) == 1:
                self.token = self.line[0]
                self.line = self.line[1:]
                return

            # 기본적으로 공백을 기준으로 처리
            if idx_space > 0:
                idx = idx_space
            else:
                idx = idx_next

            if ((idx_dot < idx_par_op) and (idx_dot > 0) and
                ((idx > idx_par_op) or (idx == -1))):
                # print("A")
                if idx_seme < idx_brace_clo and idx_brace_op > 0:
                    self.token = self.line[idx_brace_op:idx_brace_op+1]
                    self.line = self.line[idx_brace_op+2:]
                    
                else:
                    self.token = self.line[:idx_dot]
                    self.line = self.line[idx_dot:]
                    
            
            elif idx_par_op < idx and idx_par_op > 0 and \
                (idx_par_op < idx_bar_op or idx_bar_op == -1):
                # print("B")
                self.token = self.line[:idx_par_op]
                self.line = self.line[idx_par_op:]
            
            elif idx_par_clo < idx and idx_par_clo > 0 and \
                (idx_par_clo < idx_bar_clo or idx_bar_clo == -1):
                # print("C")
                tok = self.line[:idx_par_clo]
                if tok[0] == "-":
                    self.token = self.line[0]
                    self.line = self.line[1:]
                    
                else:
                    self.token = self.line[:idx_par_clo]
                    self.line = self.line[idx_par_clo:]
                    
                # self.token = self.line[:idx_par_clo]
                # self.line = self.line[idx_par_clo:]
            
            elif ((idx_commas < idx) and (idx_commas > 0)):
                # print("D")
                self.token = self.line[:idx_commas]
                self.line = self.line[idx_commas:]
            
            elif idx_bar_op == 1 or (idx_space == 0 and idx_bar_op == 2):
                # print("bar op - before line : " + self.line)
                if idx_bar_op == 1:
                    self.token = self.line[0]
                    self.line = self.line[idx_bar_op:]
                elif idx_bar_op == 2:
                    self.token = self.line[1]
                    self.line = self.line[idx_bar_op:]
                # print("token : " + self.token)
                # print("after line : '{}'\n\n".format(self.line[:-1]))

            elif idx_bar_clo < idx and idx_bar_clo > 0:
                # print("bar clo - before line : " + self.line)
                self.token = self.line[:idx_bar_clo]
                self.line = self.line[idx_bar_clo:]
                # print("token : " + self.token)
                # print("after line : '{}'\n\n".format(self.line[:-1]))
            
            elif ((idx_seme < idx) and (idx_seme > 0)):
                if idx_seme < idx_brace_clo and idx_brace_op > 0:
                    self.token = self.line[idx_brace_op:idx_brace_op+1]
                    self.line = self.line[idx_brace_op+2:]
                    
                elif idx_par_clo == 0 and idx_par_clo_2 == 1:
                    self.token = self.line[0]
                    self.line = self.line[1:]                    
                    
                else:
                    self.token = self.line[:idx_seme]
                    self.line = self.line[idx_seme:]

            elif idx_next < 0:
                self.line = None
                self.advance()
            
            else:
                # print("마무리 처리 - before line : " + self.line)
                # )) 와 같은 경우 ')' 하나만 처리 할 수 있도록 로직 추가
                if idx_par_clo == 0 and idx_par_clo_2 == 1:
                    self.token = self.line[0]
                    self.line = self.line[1:]
                else:
                    self.token = self.line[:idx]
                    self.line = self.line[idx+1:]
                # print("token : " + self.token)
                # print("after line : '{}'\n\n".format(self.line[:-1]))
                    
            
            # a[i]와 같은 토큰이 나올 경우 a 만 따로 토큰으로 추출
            # idx_bar_op = self.token.find("[")
            # if idx_bar_op == 1:
            #     self.line = self.token[idx_bar_op:] + self.line
            #     self.token = self.token[:idx_bar_op]


    def set_token_type(self):
        if self.token in self.keyword_list:
            return "KEYWORD"
        elif self.token in self.symbol_list:
            return "SYMBOL"
        elif ((self.token != "\"") and (self.str_tog == 1)):
            return "STRING_CONST"
        elif (self.token[0].isdigit() == True):
            return "INT_CONST"
        else:
            return "IDENTIFIER"
    
    def get_token(self):
        return self.token

    def get_token_type(self):
        self.token_type = self.set_token_type()
        return self.token_type