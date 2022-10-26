#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace llvm;


enum Token {
    tok_eof = -1,
    tok_def = -2,
    tok_extern = -3,
    tok_identifier = -4,
    tok_number = -5,
    tok_comma = -6
};

static std::string IdentifierStr;
static double NumVal;

static int gettok() {
    static int LastChar = ' ';

    while (isspace(LastChar))
        LastChar = getchar();
    
    if (isalpha(LastChar)) {
        IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar())))
            IdentifierStr += LastChar;
        
        if (IdentifierStr == "def")
            return tok_def;
        
        if (IdentifierStr == "extern")
            return tok_extern;
        
        return tok_identifier;
    }

    if (isdigit(LastChar) || LastChar == '.') {
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), nullptr);
        return tok_number;
    }

    if (LastChar == '#') {
        do
            LastChar = getchar();
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF)
            return gettok();
    }

    if (LastChar == EOF)
        return tok_eof;
    
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}

namespace {

class ExprAST {
public:
    virtual ~ExprAST() = default;

    virtual Value *codegen() = 0;
};

class NumberExprAST : public ExprAST {
    double Val;

public:
    NumberExprAST(double Val) : Val(Val) {}
    Value *codegen() override;
};

class VariableExprAST : public ExprAST {
    std::string Name;

public:
    VariableExprAST(const std::string &Name) : Name(Name) {}
    Value *codegen() override;
};

class BinaryExprAST : public ExprAST {
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;

public:
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS, 
    std::unique_ptr<ExprAST> RHS) : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
    Value *codegen() override;
};

class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

public:
    CallExprAST(const std::string &Callee, 
    std::vector<std::unique_ptr<ExprAST>> Args) : Callee(Callee), Args(std::move(Args)) {}
    Value *codegen() override;
};

class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;

public:
    PrototypeAST(const std::string &Name, 
    std::vector<std::string> Args) : Name(Name), Args(std::move(Args)) {}

    Function *codegen();
    const std::string &getName() const { return Name; }
};

class FunctionAST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto, 
    std::unique_ptr<ExprAST> Body) : Proto(std::move(Proto)), Body(std::move(Body)) {}
    Function *codegen();
};
}

static int CurTok;
static int getNextToken() { return CurTok = gettok(); }

static std::map<char, int> BinopPrecedence;

static int GetTokPrecedence() {
    if (!isascii(CurTok))
        return -1;
    
    int TokPrec = BinopPrecedence[CurTok];
    if (TokPrec <= 0)
        return -1;
    return TokPrec;
}

std::unique_ptr<ExprAST> LogError(const char *Str) {
    fprintf(stderr, "Error : %s\n", Str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
    LogError(Str);
    return nullptr;
}

static std::unique_ptr<ExprAST> ParseExpression();

static std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto Result = std::make_unique<NumberExprAST>(NumVal);
    getNextToken();
    return std::move(Result);
}

static std::unique_ptr<ExprAST> ParseParenExpr() {
    getNextToken();
    auto V = ParseExpression();
    if (!V)
        return nullptr;
    
    if (CurTok != ')')
        return LogError("expected ')'");
    getNextToken();
    return V;
}

static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    std::string IdName = IdentifierStr;

    getNextToken();

    if (CurTok != '(')
        return std::make_unique<VariableExprAST>(IdName);
    
    getNextToken();
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (CurTok != ')') {
        while (true) {
            if (auto Arg = ParseExpression())
                Args.push_back(std::move(Arg));
            else
                return nullptr;
            
            if (CurTok == ')')
                break;
            
            if (CurTok != ',')
                return LogError("Expected ')' or ',' in argument list");
            getNextToken();
        }
    }

    getNextToken();

    return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

static std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurTok) {
    case tok_identifier:
        return ParseIdentifierExpr();
    case tok_number:
        return ParseNumberExpr();
    case '(':
        return ParseParenExpr();
    default:
        return LogError("unknown token when expecting an expression");
    }
}

static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS) {
    while (true) {
        int TokPrec = GetTokPrecedence();

        if (TokPrec < ExprPrec)
            return LHS;
        
        int BinOp = CurTok;
        getNextToken();

        auto RHS = ParsePrimary();
        if (!RHS)
            return nullptr;
        
        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec) {
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
            if (!RHS)
                return nullptr;            
        }

        LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
    }
}

static std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();
    if (!LHS)
        return nullptr;
    
    return ParseBinOpRHS(0, std::move(LHS));
}

static std::unique_ptr<PrototypeAST> ParsePrototype() {
    if (CurTok != tok_identifier)
        return LogErrorP("Expected function name in prototype");
    
    std::string FnName = IdentifierStr;
    getNextToken();

    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");
    
    std::vector<std::string> ArgNames;
    static int b_tok = 0;
    while (getNextToken() == tok_identifier) {
        b_tok = tok_identifier;
        ArgNames.push_back(IdentifierStr);
        getNextToken(); // ',' 처리
        
        if (CurTok == ')')
            break;
        
        if (CurTok != ',')
            return LogErrorP("Expected ',' in prototype");

        b_tok = tok_comma;
    }

    if (b_tok == tok_comma) // ')' 이전 토큰이 ',' 일 경우 에러 처리
        return LogErrorP("Identifier is required between ')' and ',' in prototype");
        
    if (CurTok != ')')
        return LogErrorP("Expected ')' in prototype");
    
    getNextToken();

    return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

static std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken();
    auto Proto = ParsePrototype();
    if (!Proto)
        return nullptr;
    
    if (auto E = ParseExpression())
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    return nullptr;
}

static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    if (auto E = ParseExpression()) {
        auto Proto = std::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>());
        
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

static std::unique_ptr<PrototypeAST> ParseExtern() {
    getNextToken();
    return ParsePrototype();
}

static std::unique_ptr<LLVMContext> TheContext;
static std::unique_ptr<Module> TheModule;
static std::unique_ptr<IRBuilder<>> Builder;
static std::map<std::string, Value *> NamedValues;

Value *LogErrorV(const char *Str) {
    LogError(Str);
    return nullptr;
}

Value *NumberExprAST::codegen() {
    // fprintf(stderr, "NumberExprAST codegen\n");
    fprintf(stderr, "NumberExprAST - num : %lf\n", Val);
    return ConstantFP::get(*TheContext, APFloat(Val));
}

Value *VariableExprAST::codegen() {
    // fprintf(stderr, "VariableExprAST codegen\n");
    Value *V = NamedValues[Name];
    fprintf(stderr, "VariableExprAST - variable name : %s\n", Name.c_str());
    if (!V)
        return LogErrorV("Unknown variable name");
    return V;
}

Value *BinaryExprAST::codegen() {
    // fprintf(stderr, "BinaryExprAST codegen\n");
    Value *L = LHS->codegen();
    Value *R = RHS->codegen();
    if (!L || !R)
        return nullptr;
    
    switch (Op) {
    case '+':
        return Builder->CreateFAdd(L, R, "addtmp");
    case '-':
        return Builder->CreateFSub(L, R, "subtmp");
    case '*':
        return Builder->CreateFMul(L, R, "multmp");
    case '<':
        L = Builder->CreateFCmpULT(L, R, "cmptmp");
        return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
    default:
        return LogErrorV("invalid binary operator");
    }
}

Value *CallExprAST::codegen() {
    // fprintf(stderr, "CallExprAST codegen\n");
    //호출되는 함수가 모듈에 저장되있는지 확인
    Function *CalleeF = TheModule->getFunction(Callee);
    fprintf(stderr, "CallExprAST - callee name : %s\n", Callee.c_str());
    if (!CalleeF)
        return LogErrorV("Unknown function referenced");
    
    if (CalleeF->arg_size() != Args.size())
        return LogErrorV("Incorrect # arguments passed");
    
    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) {
        ArgsV.push_back(Args[i]->codegen());
        if (!ArgsV.back())
            return nullptr;
    }

    return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Function *PrototypeAST::codegen() {
    // fprintf(stderr, "PrototypeAST codegen\n");
    // 함수 유형을 double(double, double) 등으로 만듭니다.
    std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(*TheContext));

    FunctionType *FT = FunctionType::get(Type::getDoubleTy(*TheContext), Doubles, false);

    // 새 함수를 만들고 모듈에 연결
    Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

    unsigned Idx = 0;
    
    for (auto &Arg : F->args()) {
        // fprintf(stderr, "arg name : %s\n", Args[Idx].c_str());
        Arg.setName(Args[Idx++]);
    }
    
    // fprintf(stderr, "PrototypeAST - Function : %p\n", F);
    
    return F;
}

Function *FunctionAST::codegen() {
    // 먼저 이전 'extern' 선언에서 기존 함수를 확인합니다.
    Function *TheFunction = TheModule->getFunction(Proto->getName());
    
    // 테스트용 코드
    // fprintf(stderr, "Function name : %s, %p\n", Proto->getName().c_str(), TheFunction);

    if (!TheFunction)
        TheFunction = Proto->codegen();
    
    if (!TheFunction)
        return nullptr;

    // 삽입을 시작할 새 기본 블록을 작성하십시오.
    BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
    Builder->SetInsertPoint(BB); //생성된 명령어를 지정된 블록의 끝에 추가하도록 지정

    // NamedValues 맵에 함수 인수를 기록합니다.
    NamedValues.clear();
    for (auto &Arg : TheFunction->args()) {
        NamedValues[std::string(Arg.getName())] = &Arg;
    }
    
    // 테스트용 map 출력 코드
    // for (std::map<std::string, Value *>::iterator itr = NamedValues.begin(); itr != NamedValues.end(); ++itr) {
    //     fprintf(stderr, "named values %s\n", itr->first.c_str());
    // }
    
    if (Value *RetVal = Body->codegen()) {
        Builder->CreateRet(RetVal); // 'ret void' 명령어 생성을 위해 사용

        verifyFunction(*TheFunction); //함수의 오류가 있는지 확인

        return TheFunction;
    }

    TheFunction->eraseFromParent();
    return nullptr;
}

static void InitializeModule() {
    TheContext = std::make_unique<LLVMContext>();

    TheModule = std::make_unique<Module>("my cool jit", *TheContext);

    Builder = std::make_unique<IRBuilder<>>(*TheContext);
}

static void HandleDefinition() {
    if (auto FnAST = ParseDefinition()) {
        if (auto *FnIR = FnAST->codegen()) {
            fprintf(stderr, "Read function definition:");
            FnIR->print(errs());
            fprintf(stderr, "\n");
        }
    } else {
        getNextToken();
    }
}

static void HandleExtern() {
    if (auto ProtoAST = ParseExtern()) {
        if (auto *FnIR = ProtoAST->codegen()) {
            fprintf(stderr, "Read extern: ");
            FnIR->print(errs());
            fprintf(stderr, "\n");
        }
    } else {
        getNextToken();
    }
}

static void HandleTopLevelExpression() {
    if (auto FnAST = ParseTopLevelExpr()) {
        if (auto *FnIR = FnAST->codegen()) {
            fprintf(stderr, "Read top-level expression: ");
            FnIR->print(errs());
            fprintf(stderr, "\n");

            FnIR->eraseFromParent();
        }
    } else {
        getNextToken();
    }
}

static void MainLoop() {
    while (true) {
        fprintf(stderr, "ready> ");
        switch (CurTok) {
        case tok_eof:
            return;
        case ';':
            getNextToken();
            break;
        case tok_def:
            HandleDefinition();
            break;
        case tok_extern:
            HandleExtern();
            break;
        default:
            HandleTopLevelExpression();
            break;
        }
    }
}

int main() {
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40;

    fprintf(stderr, "ready> ");
    getNextToken();

    InitializeModule();

    MainLoop();

    TheModule->print(errs(), nullptr);

    return 0;
}