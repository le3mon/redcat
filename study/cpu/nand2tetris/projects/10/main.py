import sys, glob
import JackTokenizer as tok
import Compileation_Engine as com_eng

jack_dir = sys.argv[1]
if len(sys.argv) != 2:
    print("main.py [jack directory]")
    sys.exit()

file_list = glob.glob(jack_dir+"\*.jack")

for input_name in sorted(file_list):
    # f = open(file_name, "r")
    out_name = input_name[:-5] + "_out.xml"
    # w = open(out_name, "w")
    
    com = com_eng.Compile(input_name, out_name)
    # i = 0
    # while True:
    #     if tok.has_more_token(f) != True:
    #         break
        
    #     token = tok.advance()
    #     if token == "\n" or token == "":
    #         continue
        
    #     token_type = tok.token_type()
    
    #     print("token : {}, type : {}".format(token, token_type))
        
    #     com.compile_class(token, token_type)
    #     i += 1
    #     # if i == 100:
    #     #     break
    # del(com)