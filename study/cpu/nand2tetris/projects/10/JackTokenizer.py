global line, token, str_tog, bar_tog
line = ""
token = ""
str_tog = 0
bar_tog = 0
keyword_list = ("class", "constructor", "function", 
                "method", "field", "static", "var", 
                "int", "char", "boolean", "void", 
                "true", "false", "null", "this", 
                "let", "do", "if", "else", 
                "while", "return")
symbol_list = ("{", "}", "(", ")", "[", "]", 
               ".", ",", ";", "+", "-", 
               "*", "/", "&", "|", "<", 
               ">", "=", "~")

def is_comment_or_newline(line:str):
    if line == "\n":
        return True
    elif line.startswith("//"):
        return True
    elif line.startswith("/**"):
        return True
    return False

def set_new_line(f):
    global line
    while True:
        l = f.readline()

        if l.startswith(" "):
            l = l.lstrip()
        
        # 공백, 주석, 줄넘김 처리
        if is_comment_or_newline(l):
            continue
        
        if not l:
            return False
        
        else:
            #
            # 스페이스 여러 개 이상인 경우 처리하는 로직 추가

            # 주석 제거
            idx = l.find("//")
            if idx > 0:
                line = l[:idx] + "\n"
            else:
                line = l
            
            line = line.lstrip()
            
            return True

def has_more_token(f):
    global line
    if line == "":
        if set_new_line(f) == False:
            return False
        else:
            return True
    else:
        return True

def advance(): # idx 명칭 변경 필요
    global line, token, str_tog, bar_tog
    
    idx_space = line.find(" ")
    idx_next = line.find("\n")
    idx_seme = line.find(";")
    idx_par_op = line.find("(")
    idx_par_clo = line.find(")")
    idx_bar_op = line.find("[")
    idx_bar_clo = line.find("]")
    idx_dot = line.find(".")
    idx_dq = line.find("\"")
    idx_commas = line.find(",")
    
    
    # "(" 가 현재 라인의 가장 처음일 경우 처리
    if idx_par_op == 0:
        token = line[0]
        line = line[1:]
        return token
    
    # " 가 현재 라인 가장 처음일 경우 처리
    if idx_dq == 0:
        token = line[0]
        line = line[1:] 
        str_tog = str_tog ^ 1 # 해당 값으로 현재 "가 열리고 닫혔는지 확인
        return token
    
    # " 가 열린 상태라면 다음 "까지 문자열을 하나의 토큰으로 처리
    if str_tog == 1:
        token = line[:idx_dq]
        line = line[idx_dq:]
        return token
    
    # . 이 현재 라인의 가장 처음일 경우 처리
    if idx_dot == 0:
        token = line[0]
        line = line[1:]
        return token

    if idx_bar_op == 0:
        token = line[0]
        line = line[1:]
        bar_tog = bar_tog ^ 1
        return token
    
    if idx_bar_clo == 0:
        token = line[0]
        line = line[1:]
        bar_tog = bar_tog ^ 1
        return token
    
    if bar_tog == 1:
        token = line[:idx_bar_clo]
        line = line[idx_bar_clo:]
        return token
    
    # 기본적으로 공백을 기준으로 처리
    if idx_space >= 0:
        idx = idx_space
    else:
        idx = idx_next
    
    if ((idx_dot < idx_par_op) and (idx_dot > 0) and
        ((idx > idx_par_op) or (idx == -1))):
        token = line[:idx_dot]
        line = line[idx_dot:]
    
    elif ((idx_par_op < idx) and (idx_par_op > 0)):
        token = line[:idx_par_op]
        line = line[idx_par_op:]
    
    elif ((idx_par_clo < idx) and (idx_par_clo > 0)):
        token = line[:idx_par_clo]
        line = line[idx_par_clo:]
    
    elif ((idx_commas < idx) and (idx_commas > 0)):
        token = line[:idx_commas]
        line = line[idx_commas:]
    
    elif ((idx_seme < idx) and (idx_seme > 0)):
        token = line[:idx_seme]
        line = line[idx_seme:]

    elif idx_next < 0:
        return False
    
    else:
        token = line[:idx]
        line = line[idx+1:]
    
    # a[i]와 같은 토큰이 나올 경우 a 만 따로 토큰으로 추출
    token_bar_op = token.find("[")
    if token_bar_op > 0:
        line = token[token_bar_op:] + line
        token = token[:token_bar_op]
        
    return token

def token_type():
    global token, str_tog
    if token in keyword_list:
        return "KEYWORD"
    elif token in symbol_list:
        return "SYMBOL"
    elif ((token != "\"") and (str_tog == 1)):
        return "STRING_CONST"
    elif (token[0].isdigit() == True):
        return "INT_CONST"
    else:
        return "IDENTIFIER"