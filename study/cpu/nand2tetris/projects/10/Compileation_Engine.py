import binascii
import JackTokenizer
class Compile:
    def __init__(self, input_path, output_path):
        self.w = open(output_path, "w")
        self.tokenizer = JackTokenizer.Tokenizer(input_path)
        self.indent = "  "
        self.indent_idx = 0
    
    def __del__(self):
        self.w.close()

    def compile_class(self):
        self.w.write("<class>\n")
        self.indent_idx += 1

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

    def compile_class_var_dec(self):
        self.w.write("{}<classVarDec>\n".format(self.indent * self.indent_idx))
        self.indent_idx += 1

        self.write_token_and_type()
        while(True):
            self.tokenizer.advance()
            self.write_token_and_type()
            if self.tokenizer.get_token() == ";":
                break

        self.indent_idx -= 1
        self.w.write("{}</classVarDec>\n".format(self.indent * self.indent_idx))

    def compile_subroutine(self):
        self.w.write("{}<subroutineDec>\n".format(self.indent * self.indent_idx))
        self.indent_idx += 1

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
            
        
        self.indent_idx -= 1
        self.w.write("{}</subroutineDec>\n".format(self.indent * self.indent_idx))

    def compile_parameter(self):
        self.w.write("{}<parameterList>\n".format(self.indent * self.indent_idx))
        self.indent_idx += 1

        while(True):
            self.tokenizer.advance()
            if self.tokenizer.get_token() == ")":
                break
            else:
                self.write_token_and_type()

        self.indent_idx -= 1
        self.w.write("{}</parameterList>\n".format(self.indent * self.indent_idx))

    def compile_subroutine_body(self):
        self.w.write("{}<subroutineBody>\n".format(self.indent * self.indent_idx))
        self.indent_idx += 1

        self.write_token_and_type()

        while(True):
            self.tokenizer.advance()
            if self.tokenizer.get_token() == "var":
                self.compile_var_dec()
            else:
                break
        
        self.compile_statements()

        self.indent_idx -= 1
        self.w.write("{}</subroutineBody>\n".format(self.indent * self.indent_idx))
    
    def compile_var_dec(self):
        self.w.write("{}<varDec>\n".format(self.indent * self.indent_idx))
        self.indent_idx += 1

        self.write_token_and_type()
        while True:
            self.tokenizer.advance()
            self.write_token_and_type()
            if self.tokenizer.get_token() == ";":
                break

        self.indent_idx -= 1
        self.w.write("{}</varDec>\n".format(self.indent * self.indent_idx))

    def compile_statements(self):
        self.w.write("{}<statements>\n".format(self.indent * self.indent_idx))
        self.indent_idx += 1

        self.tokenizer.advance()
        while True:
            self.tokenizer.advance()
            if self.tokenizer.get_token() == "let":
                self.compile_let()

            elif self.tokenizer.get_token() == "return":
                self.compile_return()
                break

        self.indent_idx -= 1
        self.w.write("{}</statements>\n".format(self.indent * self.indent_idx))

    def compile_let(self):
        self.w.write("{}<letStatement>\n".format(self.indent * self.indent_idx))
        self.indent_idx += 1

        

        self.indent_idx -= 1
        self.w.write("{}</letStatement>\n".format(self.indent * self.indent_idx))

    def compile_return(self):
        self.w.write("{}<returnStatement>\n".format(self.indent * self.indent_idx))
        self.indent_idx += 1

        self.indent_idx -= 1
        self.w.write("{}</returnStatement>\n".format(self.indent * self.indent_idx))

    def write_token_and_type(self):
        indent = self.indent * self.indent_idx
        token = self.tokenizer.get_token()
        token_type = self.tokenizer.get_token_type()
        if token_type == "STRING_CONST":
            token_type = "stringConstant"
        elif token_type == "INT_CONST":
            token_type = "integerConstant"
        else:
            token_type = token_type.lower()
        
        self.w.write("{}<{}> {} </{}>\n".format(indent, token_type, token, token_type))
        
