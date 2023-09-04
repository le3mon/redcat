import binascii
import JackTokenizer
class Compile:
    def __init__(self, input_path, output_path):
        self.fp = open(output_path, "w")

        self.tokenizer = JackTokenizer.Tokenizer(input_path)
        self.indent = "  "
        self.indent_idx = 0
        self.prev_token = ""
        self.prev_type = ""
    
    def __del__(self):
        self.fp.close()

    def compile_class(self):
        self.write_compile_open("class", 1)

        self.tokenizer.advance()
        self.write_token_and_type()
        self.tokenizer.advance()
        self.write_token_and_type()
        self.tokenizer.advance()
        self.write_token_and_type()

        while(True):
            self.tokenizer.advance()
            if self.tokenizer.get_token() == "static" or \
                self.tokenizer.get_token() == "field":
                self.compile_class_var_dec()
            
            elif self.tokenizer.get_token() == "constructor" or \
                self.tokenizer.get_token() == "method" or \
                self.tokenizer.get_token() == "function":
                self.compile_subroutine()

            else:
                break
        
        self.write_token_and_type()
        
        self.write_compile_close("class", -1)
            
    def compile_class_var_dec(self):
        self.write_compile_open("classVarDec", 1)

        self.write_token_and_type()
        while(True):
            self.tokenizer.advance()
            self.write_token_and_type()
            if self.tokenizer.get_token() == ";":
                break

        self.write_compile_close("classVarDec", -1)

    def compile_subroutine(self):
        self.write_compile_open("subroutineDec", 1)        

        self.write_token_and_type()
        while(True):
            self.tokenizer.advance()
            self.write_token_and_type()
            if self.tokenizer.get_token() == "(":
                self.compile_parameter()
                self.write_token_and_type()
                self.tokenizer.advance()
                break
            elif self.tokenizer.get_token() == "{":
                break

        
        if self.tokenizer.get_token() == "{":
            self.compile_subroutine_body()
            
        
        self.write_compile_close("subroutineDec", -1)

    def compile_parameter(self):
        self.write_compile_open("parameterList", 1)

        while(True):
            self.tokenizer.advance()
            if self.tokenizer.get_token() == ")":
                break
            else:
                self.write_token_and_type()


        self.write_compile_close("parameterList", -1)

    def compile_subroutine_body(self):
        self.write_compile_open("subroutineBody", 1)

        self.write_token_and_type()

        while(True):
            self.tokenizer.advance()
            if self.tokenizer.get_token() == "var":
                self.compile_var_dec()
            else:
                break
        
        self.compile_statements()
        
        # } 출력
        self.write_token_and_type()
        
        self.write_compile_close("subroutineBody", -1)
    
    def compile_var_dec(self):
        self.write_compile_open("varDec", 1)

        self.write_token_and_type()
        while True:
            self.tokenizer.advance()
            self.write_token_and_type()
            if self.tokenizer.get_token() == ";":
                break

        self.write_compile_close("varDec", -1)

    def compile_statements(self):
        self.write_compile_open("statements", 1)

        
        while True:
            if self.tokenizer.get_token() == "let":
                self.compile_let()

            elif self.tokenizer.get_token() == "while":
                self.compile_while()

            elif self.tokenizer.get_token() == "do":
                self.compile_do()
            
            elif self.tokenizer.get_token() == "if":
                self.compile_if()
                if self.tokenizer.get_token() != "}":
                    continue
            
            elif self.tokenizer.get_token() == "return":
                self.compile_return()
            
            elif self.tokenizer.get_token() == "}":
                break
            
            self.tokenizer.advance()

        self.write_compile_close("statements", -1)

    def compile_let(self):
        self.write_compile_open("letStatement", 1)

        # let 출력
        self.write_token_and_type()

        
        # 식별자면 출력하고 ( 이면 expression 컴파일 함수 호출
        while(True):    
            self.tokenizer.advance()
            if self.tokenizer.get_token() == "=":
                break
            elif self.tokenizer.get_token_type() == "IDENTIFIER":
                self.write_token_and_type()
            elif self.tokenizer.get_token_type() == "SYMBOL":
                self.write_token_and_type()
                self.compile_expression()
                self.write_token_and_type()
        
        # "=" 출력
        self.write_token_and_type()
        
        self.compile_expression()
        self.write_token_and_type()
        # 
        
        self.write_compile_close("letStatement", -1)

    def compile_while(self):
        self.write_compile_open("whileStatement", 1)
        
        # while 출력
        self.write_token_and_type()
        
        # ( 출력 및 expression 처리
        self.tokenizer.advance()
        self.write_token_and_type()
        self.compile_expression()
        self.write_token_and_type()
        
        # { statements } 처리
        self.tokenizer.advance()
        self.write_token_and_type()
        self.compile_statements()
        
        # } 출력
        self.write_token_and_type()
        
        self.write_compile_close("whileStatement", -1)

    def compile_do(self):
        self.write_compile_open("doStatement", 1)
        
        # do 출력
        self.write_token_and_type()
        
        while(True):    
            self.tokenizer.advance()
            self.write_token_and_type()
                
            if self.tokenizer.get_token() == "(":
                self.compile_expression_list()
            
            elif self.tokenizer.get_token() == ";":
                break
        
        self.write_compile_close("doStatement", -1)

    def compile_if(self):
        self.write_compile_open("ifStatement", 1)
        
        # if 출력
        self.write_token_and_type()
        
        # ( 출력 및 expression 처리
        self.tokenizer.advance()
        self.write_token_and_type()
        self.compile_expression()
        self.write_token_and_type()
        
        # { statements } 처리
        self.tokenizer.advance()
        self.write_token_and_type()
        self.compile_statements()
        self.write_token_and_type()
        
        self.tokenizer.advance()
        if self.tokenizer.get_token() == "else":
            # else, { 출력
            self.write_token_and_type()
            self.tokenizer.advance()
            self.write_token_and_type()
            
            self.compile_statements()
            self.write_token_and_type()
        
        
        self.write_compile_close("ifStatement", -1)

    def compile_expression(self, prev = 0):
        self.write_compile_open("expression", 1)

        self.write_compile_open("term", 1)
        
        sub_tog = 0
        roop = 0
        unary_tog = 0
        while(True):
            roop += 1
            if prev == 0:
                self.tokenizer.advance()
            else:
                prev = 0
            
            if self.tokenizer.get_token() == "\"":
                continue
            
            elif self.tokenizer.get_token() == ",":
                break
            
            elif self.tokenizer.get_token() == ")" or \
                self.tokenizer.get_token() == "]":
                if unary_tog == 1:
                    unary_tog = 0
                    self.write_compile_close("term", -1)
                break
            
            elif self.tokenizer.get_token() == ";":
                break
            
            elif self.tokenizer.get_token() == ">" or \
                self.tokenizer.get_token() == "<" or \
                self.tokenizer.get_token() == "/" or \
                self.tokenizer.get_token() == "*" or \
                self.tokenizer.get_token() == "|" or \
                self.tokenizer.get_token() == "&" or \
                self.tokenizer.get_token() == "=" or \
                (self.tokenizer.get_token() == "-" and self.prev_token != "(") or \
                self.tokenizer.get_token() == "+":
                
                self.write_compile_close("term", -1)
                self.write_token_and_type()
                self.write_compile_open("term", 1)
                continue
            
            elif (self.tokenizer.get_token_type() == "IDENTIFIER" or 
                self.tokenizer.get_token_type() == "INT_CONST") and \
                roop == 2 and \
                self.prev_token in self.tokenizer.unary_list:
                self.write_compile_open("term", 1)
                self.write_token_and_type()
                self.write_compile_close("term", -1)
                continue
            
            elif self.tokenizer.get_token() == "(" and self.prev_token == "~":
                self.write_compile_open("term", 1)
                unary_tog = 1

            self.write_token_and_type()
            
            if self.tokenizer.get_token() == "(":
                if sub_tog == 1:
                    self.compile_expression_list()
                else:
                    self.compile_expression()
                    self.write_token_and_type()
                
            elif self.tokenizer.get_token() == "[":
                self.compile_expression()
                self.write_token_and_type()
                
            elif self.tokenizer.get_token() == ".":
                sub_tog = 1

        self.write_compile_close("term", -1)

        self.write_compile_close("expression", -1)

    def compile_expression_list(self):
        self.write_compile_open("expressionList", 1)
        
        self.tokenizer.advance()
        
        # () 괄호 안에 아무 값도 없을 경우
        # expressionList 태그를 열고 닫아야 하는데 이를 위한 처리
        if self.prev_token == "(" and self.tokenizer.get_token() == ")":
            a = 0 #dummy code
        else:
            self.compile_expression(1)
        
        if self.tokenizer.get_token() == ",":    
            while(True):
                if self.tokenizer.get_token() == ")":
                    break
                else:
                    self.write_token_and_type()
                    self.tokenizer.advance()
                    self.compile_expression(1)
            
        
        self.write_compile_close("expressionList", -1)
        self.write_token_and_type()

    def compile_return(self):
        self.write_compile_open("returnStatement", 1)

        # return 출력
        self.write_token_and_type()
        
        while(True):    
            self.tokenizer.advance()
            
            if self.tokenizer.get_token() == ";":
                self.write_token_and_type()
                break
            
            elif self.tokenizer.get_token() == "(":
                self.compile_expression_list()
            
            else:
                self.compile_expression(1)
                if self.tokenizer.get_token() == ";":
                    self.write_token_and_type()
                    break

        self.write_compile_close("returnStatement", -1)

    def write_token_and_type(self):
        indent = self.indent * self.indent_idx
        token = self.tokenizer.get_token()
        self.prev_token = self.tokenizer.get_token()
        token_type = self.tokenizer.get_token_type()
        self.prev_type = self.tokenizer.get_token_type()
        if token_type == "STRING_CONST":
            token_type = "stringConstant"
        elif token_type == "INT_CONST":
            token_type = "integerConstant"
        else:
            token_type = token_type.lower()
        
        if token == "<":
            token = "&lt;"
        elif token == ">":
            token = "&gt;"
        elif token == "&":
            token = "&amp;"
            
        self.fp.write("{}<{}> {} </{}>\n".format(
            indent, token_type, token, token_type))
    
    
    # 각각 컴파일 처리 시 시작 태그와 끝 태그 출력 함수
    # tag는 출력할 때 필요한 문자열, ind는 indent_idx에 연산할 값
    def write_compile_open(self, tag, ind = 0):
        self.fp.write("{}<{}>\n".format(self.indent * self.indent_idx, tag))
        self.indent_idx += ind
        
    
    def write_compile_close(self, tag, ind = 0):
        self.indent_idx += ind
        self.fp.write("{}</{}>\n".format(self.indent * self.indent_idx, tag))
    
    
    # for debugging
    def print_token_and_type(self):
        print("Token is : {}".format(self.tokenizer.get_token()))
        print("Token type is : {}\n".format(self.tokenizer.get_token_type()))
    