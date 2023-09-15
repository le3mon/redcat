import JackTokenizer
import SymbolTable
import VMWriter

class Compile:
    op_list = (">", "<", "/", "*", "|", "&", "=", "+", "-")
    operator = {
        "+": "ADD",
        "-": "SUB",
        "=": "EQ",
        ">": "GT",
        "<": "LT",
        "&": "AND",
        "|": "OR",
    }
    un_operator = {
        "-": "NEG",
        "~": "NOT"
    }
    unary_list = ("-", "~")
    kind_list = {
        'ARG': 'ARG',
        'STATIC': 'STATIC',
        'VAR': 'LOCAL',
        'FIELD': 'THIS'
  }

    def __init__(self, input_path, output_path):
        self.fp = open(output_path, "w")
        self.indent = "  "
        self.indent_idx = 0
        self.prev_token = ""
        self.prev_type = ""
        self.class_name = ""
        self.subroutine_name = ""
        self.subroutine_type = ""
        self.var_name = ""
        self.expr_list = []
        self.expr = ""
        self.while_idx = 0
        self.if_idx = 0

        self.tokenizer = JackTokenizer.Tokenizer(input_path)
        self.symbol_table = SymbolTable.SymbolTable()
        self.writer = VMWriter.VMWriter(output_path)
    
    def __del__(self):
        self.fp.close()
        del(self.writer)

    def compile_class(self):
        self.tokenizer.advance()
        self.tokenizer.advance() # class 처리
        self.class_name = self.tokenizer.get_token() # class 식별자 저장
        self.tokenizer.advance() # class 식별자 처리
        
        # 서브루틴 컴파일 전 심볼 테이블 초기화
        self.symbol_table.reset()

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
            
    def compile_class_var_dec(self):
        kind = self.tokenizer.get_token() # fiedld or static 저장
        # self.tokenizer.advance() # field or static 처리
        
        token_type = ""
        while(True):
            self.tokenizer.advance()
            # if self.tokenizer.get_token_type() == "KEYWORD":
            #     token_type = self.tokenizer.get_token() # 타입 처리(ex:int)
            #     print(token_type)
            if token_type == "":
                token_type = self.tokenizer.get_token()

            elif self.tokenizer.get_token_type() == "IDENTIFIER":
                name = self.tokenizer.get_token()
                self.symbol_table.define(name, token_type, kind.upper())

            if self.tokenizer.get_token() == ";":
                # self.tokenizer.advance() # ; 처리
                break
            

    def compile_subroutine(self):
        self.subroutine_type = self.tokenizer.get_token()
        self.tokenizer.advance() # constructor / method / function 처리

        self.tokenizer.advance() # void or type(int, char, ...) 처리
        
        self.subroutine_name = self.tokenizer.get_token()

        if self.subroutine_type == "method":
            self.symbol_table.define("instance", self.class_name, "ARG")
        
        
        while(True):
            self.tokenizer.advance()
            if self.tokenizer.get_token() == "(":
                self.compile_parameter()
                self.tokenizer.advance()
                break
            elif self.tokenizer.get_token() == "{":
                break

        if self.tokenizer.get_token() == "{":
            self.compile_subroutine_body()


    def compile_parameter(self):
        while(True):
            self.tokenizer.advance()

            if self.tokenizer.get_token() == ")":
                break
            
            elif self.tokenizer.get_token_type() == "KEYWORD":
                token_type = self.tokenizer.get_token()
            
            elif self.tokenizer.get_token_type() == "IDENTIFIER":
                name = self.tokenizer.get_token()
                self.symbol_table.define(name, token_type, "ARG")


    def compile_subroutine_body(self):
        while(True):
            self.tokenizer.advance()
            if self.tokenizer.get_token() == "var":
                self.compile_var_dec()
            else:
                break

        # vm function 기록
        name = "function {}.{}".format(self.class_name, self.subroutine_name) 
        self.writer.write_function(name, self.symbol_table.var_count("VAR"))
        
        if self.subroutine_type == "constructor":
            field_count = self.symbol_table.var_count("FIELD")
            self.writer.write_push("CONST", field_count)
            self.writer.write_call("Memory.alloc", 1)
            self.writer.write_pop("POINTER", 0)
        
        elif self.subroutine_type == "method":
            self.writer.write_push("ARG", 0)
            self.writer.write_pop("POINTER", 0)

        self.compile_statements()
    
    def compile_var_dec(self):
        token_type = None
        while True:
            self.tokenizer.advance()
            self.write_token_and_type()
            if self.tokenizer.get_token() == ";":
                break

            elif self.tokenizer.get_token() == "var" or \
                self.tokenizer.get_token() == ",":
                continue

            elif token_type == None:
                token_type = self.tokenizer.get_token()
            
            else:
                name = self.tokenizer.get_token()
                self.symbol_table.define(name, token_type, "VAR")

    def compile_statements(self):
        while True:
            if self.tokenizer.get_token() == "let":
                self.compile_let()
                continue

            elif self.tokenizer.get_token() == "while":
                self.compile_while()

            elif self.tokenizer.get_token() == "do":
                self.compile_do()
                continue
            
            elif self.tokenizer.get_token() == "if":
                self.compile_if()
                continue
            
            elif self.tokenizer.get_token() == "return":
                self.compile_return()
            
            elif self.tokenizer.get_token() == "}":
                break
            
            self.tokenizer.advance()

    def compile_let(self):
        # 식별자면 출력하고 ( 이면 expression 컴파일 함수 호출
        self.tokenizer.advance() # let 처리

        var_name = self.tokenizer.get_token()
        self.var_name = var_name
        var_kind = self.kind_list[self.symbol_table.kind_of(var_name)]
        var_idx = self.symbol_table.index_of(var_name)
        self.tokenizer.advance() # var_name 처리

        if self.tokenizer.get_token() == "[":
            self.tokenizer.advance() # [ 처리
            self.compile_expression()
            self.tokenizer.advance() # ] 처리
            
            self.writer.write_push(var_kind, var_idx)

            self.writer.write_arithmetic("ADD")

            self.tokenizer.advance() # = 처리
            self.compile_expression()
            self.tokenizer.advance() # ; 처리

            self.writer.write_pop("TEMP", 0)
            self.writer.write_pop("POINTER", 1)
            self.writer.write_push("TEMP", 0)
            self.writer.write_pop("THAT", 0)
        else:
            self.tokenizer.advance() # = 처리
            self.compile_expression()
            self.tokenizer.advance() # ; 처리
        
            self.writer.write_pop(var_kind, var_idx)

    def compile_while(self):
        self.tokenizer.advance() # while 처리
        while_idx = self.while_idx
        self.while_idx += 1
        
        self.writer.write_label("WHILE_EXP{}".format(while_idx))
        
        self.tokenizer.advance() # ( 처리
        
        self.compile_expression()
        self.writer.write_arithmetic("NOT")
        self.tokenizer.advance() # ) 처리

        self.tokenizer.advance() # { 처리
        self.writer.write_if("WHILE_END{}".format(while_idx))
        self.compile_statements()
        self.writer.write_goto("WHILE_EXP{}".format(while_idx))
        self.writer.write_label("WHILE_END{}".format(while_idx))

        
    def compile_do(self):
        self.tokenizer.advance() # do 처리

        self.compile_subroutine_call()
        self.writer.write_pop("TEMP", 0)
        self.tokenizer.advance() # ; 처리

    def compile_if(self):
        self.tokenizer.advance() # if 처리
        if_idx = self.if_idx
        self.if_idx += 1

        self.tokenizer.advance() # ( 처리
        self.compile_expression()
        if self.tokenizer.get_token() == ")":
            self.tokenizer.advance()  # ) 처리
        self.tokenizer.advance() # { 처리

        self.writer.write_if("IF_TRUE{}".format(if_idx))
        self.writer.write_goto("IF_FALSE{}".format(if_idx))
        self.writer.write_label("IF_TRUE{}".format(if_idx))
        self.compile_statements()

        self.tokenizer.advance() # } 처리    

        if self.tokenizer.get_token() == "else":
            self.writer.write_goto("IF_END{}".format(if_idx))
            self.writer.write_label("IF_FALSE{}".format(if_idx))
            self.tokenizer.advance() # else 처리
            self.tokenizer.advance() # { 처리
            self.compile_statements()
            self.tokenizer.advance() # } 처리
            self.writer.write_label("IF_END{}".format(if_idx))
        else:
            self.writer.write_label("IF_FALSE{}".format(if_idx))

    def compile_expression(self, prev = 0):
        self.compile_term()
        
        while self.tokenizer.get_token() in self.op_list:
            op = self.tokenizer.get_token()
            self.tokenizer.advance() # op 처리
            self.compile_term()
            
            if op in self.operator.keys():
                self.writer.write_arithmetic(self.operator[op])
            elif op == "*":
                self.writer.write_call("Math.multiply", 2)
            elif op == "/":
                self.writer.write_call("Math.divide", 2)
            

    def compile_term(self):
        if self.is_unop():
            op = self.tokenizer.get_token()[0]
            self.tokenizer.advance() # unop 처리
            self.compile_term()
            self.writer.write_arithmetic(self.un_operator[op])
        
        elif self.tokenizer.get_token() == "(":
            self.tokenizer.advance()
            self.compile_expression()
            self.tokenizer.advance()
        
        elif self.tokenizer.get_token_type() == "INT_CONST":
            self.writer.write_push("CONST", self.tokenizer.get_token())
            self.tokenizer.advance()
        
        elif self.tokenizer.get_token_type() == "STRING_CONST":
            self.compile_string()
            self.tokenizer.advance() # " 처리
        
        elif self.tokenizer.get_token_type() == "KEYWORD":
            self.compile_keyword()
            self.tokenizer.advance()
        
        else:
            if self.is_array():
                var_name = self.tokenizer.get_token()
                self.tokenizer.advance() # var name 처리
                self.tokenizer.advance() # [ 처리
                self.compile_expression()
                self.tokenizer.advance() # ] 처리

                arr_kind = self.symbol_table.kind_of(var_name)
                arr_idx = self.symbol_table.index_of(var_name)
                self.writer.write_push(self.kind_list[arr_kind], arr_idx)

                self.writer.write_arithmetic("ADD")
                self.writer.write_pop("POINTER", 1)
                self.writer.write_push("THAT", 0)
            
            elif self.is_subroutine_call():
                self.compile_subroutine_call()
            
            else:
                if self.tokenizer.get_token() == "\"":
                    self.tokenizer.advance()
                    self.compile_term()
                else:
                    var = self.tokenizer.get_token()
                    self.var_name = var
                    var_kind = self.kind_list[self.symbol_table.kind_of(var)]
                    var_idx = self.symbol_table.index_of(var)
                    # print(var)
                    # print(var_kind)
                    # print(var_idx)
                    # print("\n\n")
                    self.writer.write_push(var_kind, var_idx)
                    self.tokenizer.advance()

    def compile_string(self):
        string = self.tokenizer.get_token()
        self.tokenizer.advance() # 문자열 처리
        self.writer.write_push("CONST", len(string))
        self.writer.write_call("String.new", 1)

        for i in string:
            self.writer.write_push("CONST", ord(i))
            self.writer.write_call("String.appendChar", 2)
    
    def compile_keyword(self):
        keyword = self.tokenizer.get_token()

        if keyword == "this":
            self.writer.write_push("POINTER", 0)
        else:
            self.writer.write_push("CONST", 0)

            if keyword == "true":
                self.writer.write_arithmetic("NOT")

    def compile_subroutine_call(self):
        identifier = self.tokenizer.get_token()
        func_name = identifier
        num_arg = 0
        self.tokenizer.advance() # function name 처리
        
        if self.tokenizer.get_token() == ".":
            self.tokenizer.advance() #. 처리
            subroutine_name = self.tokenizer.get_token()
            iden_type = self.symbol_table.type_of(identifier)
            if iden_type == None:
                class_name = identifier
                func_name = "{}.{}".format(class_name, subroutine_name)
            else:
                inst_kind = self.symbol_table.kind_of(identifier)
                inst_idx = self.symbol_table.index_of(identifier)

                self.writer.write_push(self.kind_list[inst_kind], inst_idx)

                func_name = "{}.{}".format(iden_type, subroutine_name)
                num_arg += 1
            self.tokenizer.advance() # subroutine_name 처리

        elif self.tokenizer.get_token() == "(":
            subroutine_name = identifier
            func_name = "{}.{}".format(self.class_name, subroutine_name)
            num_arg += 1
            self.writer.write_push("POINTER", 0)
        
        num_arg += self.compile_expression_list()
        
        self.writer.write_call(func_name, num_arg)


    def compile_expression_list(self):
        num_args = 0
        self.tokenizer.advance() # ( 처리

        if self.tokenizer.get_token() != ")":
            num_args += 1
            self.compile_expression()
            # self.tokenizer.advance() # ) 처리
        
        while self.tokenizer.get_token() != ")":
            if self.tokenizer.get_token() == ",":
                num_args += 1
                self.tokenizer.advance() # , 처리
            self.compile_expression()
        
        self.tokenizer.advance() # ) 처리

        return num_args

    def compile_return(self):
        self.tokenizer.advance()
        if self.tokenizer.get_token() != ";":
            self.compile_expression()
        else:
            self.writer.write_push("CONST", 0)
        
        self.if_idx = 0
        self.while_idx = 0

        self.writer.write_return()

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
    
    def is_unop(self):
        try:
            op = self.tokenizer.get_token()[0]
            if op in self.unary_list and \
                (self.tokenizer.line[0].isdigit() or \
                self.tokenizer.line[0].isalpha() or \
                self.tokenizer.line[0].find("(") == 0):
                return True
            else:
                return False
        except IndexError:
            return False

    def is_subroutine_call(self):
        try:
            if self.tokenizer.line[0] == "." and \
                self.tokenizer.line.find("(") > 0:
                return True
        except IndexError:
            return False
    
    def is_array(self):
        try:
            if self.tokenizer.line[0] == "[":
                return True
            else:
                return False
        except IndexError:
            return False
