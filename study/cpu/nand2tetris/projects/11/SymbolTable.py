class SymbolTable:
    class_table = {}
    kind = {
        "FIELD" : 0,
        "STATIC" : 0,
        "ARG" : 0,
        "VAR" : 0
    }

    def __init__(self):
        self.subroutine_table = {}
    
    def __del__(self):
        print
    
    # 심볼 테이블 비우고 4개 인덱스 0으로 재설정
    # 서브루틴 선언을 컴파일하기 시작할 때 호출
    def reset(self):
        # self.kind["ARG"] = 0
        # self.kind["VAR"] = 0
        self.subroutine_table = {}
        for ty in self.kind:
            self.kind[ty] = 0
    

    # 주어진 이름, 타입, 종류(STATIC, FIELD, ARG, VAR)에
    # 새 변수를 정의(테이블에 추가)한다.
    # 변수에 해당 kind의 인덱스 값을 할당하고, 인덱스에 1 더함
    def define(self, name:str, type:str, kind):
        idx = 0
        if kind == "ARG" or kind == "VAR":
            if kind == "ARG":
                idx = self.kind["ARG"]
                self.kind["ARG"] += 1
            elif kind == "VAR":
                idx = self.kind["VAR"]
                self.kind["VAR"] += 1
            self.subroutine_table[name] = (type, kind, idx)

        elif kind == "FIELD" or kind == "STATIC":
            if kind == "FIELD":
                idx = self.kind["FIELD"]
                self.kind["FIELD"] += 1
            elif kind == "STATIC":
                idx = self.kind["STATIC"]
                self.kind["STATIC"] += 1
            self.class_table[name] = (type, kind, idx)



    # 테이블에서 이미 정의된, 특정 kind의 변수 개수를 반환한다.
    def var_count(self, kind):
        return self.kind[kind]
    
    # 해당 이름 식별자의 kind 반환, 그런 식별자가 없다면 NONE을 반환
    def kind_of(self, name:str):
        kind = None
        if name in self.subroutine_table.keys():
            kind = self.subroutine_table[name][1]
        elif name in self.class_table.keys():
            kind = self.class_table[name][1]
        else:
            kind = None
        return kind
    
    # 해당 이름 변수의 type 반환
    def type_of(self, name:str):
        t_type = None
        if name in self.subroutine_table.keys():
            t_type = self.subroutine_table[name][0]
        elif name in self.class_table.keys():
            t_type = self.class_table[name][0]
        else:
            t_type = None
        return t_type
    
    # 해당 이름 변수의 index 반환
    def index_of(self, name:str):
        idx = None
        if name in self.subroutine_table.keys():
            idx = self.subroutine_table[name][2]
        elif name in self.class_table.keys():
            idx = self.class_table[name][2]
        else:
            idx = None
        return idx
    
    def print_table(self):
        print("============ CLASS ============")
        print(self.class_table)
        print("============ SUBROUTINE ============")
        print(self.subroutine_table)
        print("\n\n")
        