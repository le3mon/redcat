from xml.etree.ElementTree import Element, SubElement, ElementTree
import binascii
class Compile:
    def __init__(self, file_name):
        self.fp = open(file_name, "wb")
        self.root = None # class
        self.class_vd = None #classVarDec
        self.sub_dec = None #SubroutineDec
        self.para_list = None
        self.sub_body = None #SubroutineBody
        self.var_dec = None
        self.statements = None #statements
        self.let = None
        self.do = None
        self.while_ = None
        self.while_stat = None
        self.expr = None
        self.expr_list = None
        self.term_list = [-1]
        self.term = None
        
        self.latest = None # 가장 최근 element
        self.prev_token = None
        # self.sub_rou = None
        # self.param = None
        # self.sub_var = None
        # self.root_state = None
        # self.let_state = None
        self.class_tog = -1
        self.para_tog = 0 # parameter toggle
         
        self.class_vd_keyword = ("static", "field")
        self.sub_dec_keyword = ("constructor", "function", "method")
        
    def _pretty_print(self, current, parent=None, index=-1, depth=0):
        for i, node in enumerate(current):
            self._pretty_print(node, current, i, depth + 1)
        if parent is not None:
            if index == 0:
                parent.text = '\n' + ("  " * depth)
                if current.text.startswith("\x0a") == False:
                    current.text = " {} ".format(current.text)
            else:
                if current.text is not None:
                    if current.text.startswith("\x0a") == False:
                        current.text = " {} ".format(current.text)
                else:
                    current.text = '\n'  + ('  ' * depth)
                
                parent[index - 1].tail = '\n' + ('  ' * depth)
            if index == len(parent) - 1:
                current.tail = '\n' + ('  ' * (depth - 1))     
    
    def __del__(self):
        self._pretty_print(self.root)
        tree = ElementTree(self.root)
        tree.write(self.fp, encoding="utf-8", short_empty_elements=False)
        self.fp.close()

    def append_element(self, token:str, t_type:str, tree):
        if t_type == "STRING_CONST":
            t_type = "stringConstant"
        elif t_type == "INT_CONST":
            t_type = "integerConstant"
        else:
            t_type = t_type.lower()

        elm = SubElement(tree, t_type)
        elm.text = token
        self.class_tog += 1
    
    def compile_class_var_dec(self, token, t_type):
        if token in self.class_vd_keyword:
            self.class_vd = Element("classVarDec")
            self.root.append(self.class_vd)
        
        self.append_element(token, t_type, self.class_vd)
        
        # 토큰이 ; 이면 해당 명령줄의 끝으로 보고 class_vd 초기화
        if token == ";":
            self.class_vd = None
            
    def compile_subroutine(self, token, t_type):
        # subroutineBody 처리
        if (((token == "{") and (self.sub_body is None)) or
            self.sub_body is not None):
            self.compile_subroutine_body(token, t_type)
        
        # parameterList 처리
        elif (((token == "(") and (self.para_list is None)) or
                self.para_list is not None):
            self.compile_parameter_list(token, t_type, self.sub_dec)
                
        # subroutineDec 처리
        elif token in self.sub_dec_keyword:
            self.sub_dec = Element("subroutineDec")
            self.root.append(self.sub_dec)
            self.append_element(token, t_type, self.sub_dec)
            self.latest = self.sub_dec
                
        elif (self.para_list is None) and (self.sub_dec is not None):
            self.append_element(token, t_type, self.sub_dec)
         
        
    def compile_parameter_list(self, token, t_type, elmnt):
        if token == "(":
            # ( 의 경우 parameter 서브가 아니라 이전 엘리먼트로 처리
            self.append_element(token, t_type, elmnt)
            
            # parameter element 생성
            self.para_list = Element("parameterList")
            elmnt.append(self.para_list)
            
        elif token == ")":
            # )도 (와 같은 곳에 처리
            self.append_element(token, t_type, elmnt)
            
            # 모든 인자를 추가했으니 생성한 element 초기화
            self.para_list = None
            
        else:
            self.append_element(token, t_type, self.para_list)
    
    def compile_subroutine_body(self, token, t_type):
        if self.statements is not None:
            if (token == "let") or (self.let is not None):
                self.compile_let(token, t_type)
            
            elif (token == "do") or (self.do is not None):
                self.compile_do(token, t_type)
            
            elif (token == "while") or (self.while_ is not None):
                self.compile_while(token, t_type)
        else:
            # subroutineBody 생성
            if (token == "{") and (self.sub_body is None):
                self.sub_body = Element("subroutineBody")
                self.sub_dec.append(self.sub_body)
                self.append_element(token, t_type, self.sub_body)
            
            # # VarDec 처리
            elif (token == "var") or (self.var_dec is not None):
                self.compile_var_dec(token, t_type)
            
            # # statements 생성
            elif self.statements is None:
                self.statements = Element("statements")
                self.sub_body.append(self.statements)
                self.compile_subroutine_body(token, t_type)
        
        # self.latest = self.sub_body
        # if token == "}":
        #     self.statements = None
        #     self.append_element(token, t_type, self.sub_body)
        #     self.sub_body = None
        
    def compile_var_dec(self, token, t_type):
        if (token == "var") and (self.var_dec is None):
            self.var_dec = Element("varDec")
            self.sub_body.append(self.var_dec)
        
        self.append_element(token, t_type, self.var_dec)
        
        if token == ";":
            self.var_dec = None
    
    def compile_expression(self, token, t_type):
        if token == "(" or token == "[" or token == "=":
            self.term = Element("term")
            self.expr.append(self.term)
            self.term_list.append(self.term)
        else:
            if token == "\"":
                return
            else:
                self.append_element(token, t_type, self.term_list[-1])
            
    
    def compile_let(self, token, t_type):
        if (token == "let") and (self.let is None):
            self.let = Element("letStatement")
            self.statements.append(self.let)
            self.append_element(token, t_type, self.let)
        
        elif token == "(":
            if self.term_list[-1] == -1:
                elem = self.let
            else:
                elem = self.term_list[-1]
            self.append_element(token, t_type, elem)
            self.expr_list = Element("expressionList")
            elem.append(self.expr_list)
            self.expr = Element("expression")
            self.expr_list.append(self.expr)
            self.compile_expression(token, t_type)
        
        elif token == ")" or token == ";" or token == "]":
            self.term_list.pop()
            if self.term_list[-1] == -1:
                elem = self.let
            else:
                elem = self.term_list[-1]
            self.append_element(token, t_type, elem)
            if token == ";":
                self.let = None
        
        elif self.term_list[-1] != -1:
            self.compile_expression(token, t_type)
        
        # term 엘리멘트도 없으며, token이 [ 또는 = 일 경우 엘리멘트 생성
        elif ((token == "[") or (token == "=")):
            self.append_element(token, t_type, self.let)
            if t_type == "SYMBOL":
                self.expr = Element("expression")
                self.let.append(self.expr)
                self.compile_expression(token, t_type)
        
        else:
            self.append_element(token, t_type, self.let)
    
    def compile_do(self, token, t_type):
        if (token == "do") and (self.do is None):
            self.do = Element("doStatement")
            self.statements.append(self.do)
            self.append_element(token, t_type, self.do)
        
        elif token == "(":
            if self.term_list[-1] == -1:
                elem = self.do
            else:
                elem = self.term_list[-1]
            self.append_element(token, t_type, elem)
            self.expr_list = Element("expressionList")
            elem.append(self.expr_list)
            self.expr = Element("expression")
            self.expr_list.append(self.expr)
            self.compile_expression(token, t_type)
        
        elif token == ")" or token == ";" or token == "]":
            top = self.term_list.pop()
            if top != -1:
                if self.term_list[-1] == -1:
                    elem = self.do
                else:
                    elem = self.term_list[-1]
                
                self.append_element(token, t_type, elem)
                
                if self.prev_token == "(":
                    self.expr_list.remove(self.expr)
                
            else:
                self.term_list.append(-1)
            
            if token == ";":
                self.do = None
        
        elif self.term_list[-1] != -1:
            self.compile_expression(token, t_type)
        
        # term 엘리멘트도 없으며, token이 [ 또는 = 일 경우 엘리멘트 생성
        elif ((token == "[") or (token == "=")):
            self.append_element(token, t_type, self.do)
            if t_type == "SYMBOL":
                self.expr = Element("expression")
                self.do.append(self.expr)
                self.compile_expression(token, t_type)
        
        else:
            self.append_element(token, t_type, self.do)
    
    def compile_while(self, token, t_type):
        if (token == "while") and (self.while_ is None):
            self.while_ = Element("whileStatement")
            self.statements.append(self.while_)
            self.append_element(token, t_type, self.while_)
            self.expr = None
        
        elif token == "}":
            self.while_stat = None
            self.while_ = None

        elif self.while_stat is None:
            if token == "(" and self.expr is None:
                self.append_element(token, t_type, self.while_)
                self.expr = Element("expression")
                self.while_.append(self.expr)
                self.term = Element("term")
                self.expr.append(self.term)
                self.append_element(token, t_type, self.term)
            
            if self.expr is not None:
                if t_type == "SYMBOL":
                    self.append_element(token, t_type, self.expr)
                else:
                    self.term = Element("term")
                    self.expr.append(self.term)
                    self.append_element(token, t_type, self.term)
                
            
            elif token == ")" or token == "]":
                self.append_element(token, t_type, self.while_)
                self.expr = None
            

            elif token == "}":
                self.append_element(token, t_type, self.while_)
                self.while_ = None
        
        elif token == "{" and self.while_stat is None:
            self.append_element(token, t_type, self.while_)

        else:
            self.append_element(token, t_type, self.while_)

    def compile_class(self, token, t_type):
        # class 컴파일
        if token == "class" or self.class_tog < 3:
            if token == "class":
                self.root = Element("class")
                self.class_tog = 0
                self.append_element(token, t_type, self.root)
            else:
                self.append_element(token, t_type, self.root)
        
        # ClassVarDec 컴파일
        # "static" or "field" keyword가 먼저 올 시 처리
        elif (token in self.class_vd_keyword or
            self.class_vd is not None):
            self.compile_class_var_dec(token, t_type)
        
        # Subroutine 컴파일
        elif (token in self.sub_dec_keyword or
            self.sub_dec is not None):
            self.compile_subroutine(token, t_type)
        
        self.prev_token = token