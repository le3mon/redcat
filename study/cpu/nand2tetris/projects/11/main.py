import sys, glob
import JackTokenizer as tok
import Compileation_Engine as com_eng

jack_dir = sys.argv[1]
# jack_dir = "C:\\Users\\c3054\\Desktop\\git\\redcat\\study\\cpu\\nand2tetris\\projects\\11\\Seven"
if len(sys.argv) != 2:
    print("main.py [jack directory]")
    sys.exit()

file_list = glob.glob(jack_dir+"\*.jack")

for input_name in sorted(file_list):
    out_name = input_name[:-5]
    
    com = com_eng.Compile(input_name, out_name)
    com.compile_class()