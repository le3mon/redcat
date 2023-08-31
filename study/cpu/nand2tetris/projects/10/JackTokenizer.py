class Tokenizer:
    def __init__(self, input_path):
        self.fp = open(input_path, "r")
        self.data = self.data_preproc(self.fp.read())
        self.lines = []
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

    def __del__(self):
        self.fp.close()

    
    def is_comment(self, line:str):
        if line.startswith("//"):
            return True
        elif line.startswith("/**"):
            return True
        elif line.startswith("\n"):
            return True
        elif line.find("*/") > 0:
            return True
        return False

    def data_preproc(self, pre_data:str):
        data = ""
        for line in pre_data.split("\n"):
            line = line.lstrip()
            if self.is_comment(line) == True:
                continue
                
            if line == "":
                continue
        
            idx = line.find("//")
            if idx > 0:
                line = line[:idx] + "\n"
            else:
                line += "\n"
            
            data += line
            # self.lines.append(line)
        
        self.lines = data.split("\n")
        print(data.split("\n"))
        return data

    def has_more_tokens(self):
        print(self.lines)
    
    def advance(self):
        print