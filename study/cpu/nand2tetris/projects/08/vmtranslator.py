import sys, glob
import vmparser as p
import vmcodewriter as c
import os

def vm_trans(f, w):
    while True:
        line = f.readline()
        if (is_line := p.has_more_lines(line)) == False:
            break
        
        if p.is_comment_or_newline(line) == True:
            continue
        
        command = p.advance(line)

        command_type = p.get_command_type(command)
        
        if command_type != p.C_RETURN:
            arg1 = p.get_arg1(command, command_type)
        
        if ((command_type == p.C_PUSH) or (command_type == p.C_POP)
            or (command_type == p.C_FUNCTION) or (command_type == p.C_CALL)):
            arg2 = p.get_arg2(command)
        
        if ((command_type == p.C_PUSH) or (command_type == p.C_POP)):
            c.write_push_pop(command_type, arg1, arg2, w)
        
        elif command_type == p.C_ARITHMETIC:
            c.write_arithemetic(arg1, w)
        
        elif command_type == p.C_LABEL:
            c.write_label(arg1, w)
        
        elif command_type == p.C_IF:
            c.write_if(arg1, w)
        
        elif command_type == p.C_GOTO:
            c.write_goto(arg1, w)
        
        elif command_type == p.C_RETURN:
            c.write_return(w)
        
        elif command_type == p.C_CALL:
            c.write_call(arg1, arg2, w)
        
        elif command_type == p.C_FUNCTION:
            c.write_function(arg1, arg2, w)
    c.static_control()

vm_dir = sys.argv[1]
if len(sys.argv) != 2:
    print("vmtranslator.py [vm directory]")
    sys.exit()

file_list = glob.glob(vm_dir+"\*.vm")

idx = vm_dir.find("\\")
out_name = vm_dir[idx:].replace("\\", "")+".asm"

out_path = "{}\\{}".format(vm_dir, out_name)
w = open(out_path, "w")

c.write_bootstrap(w)

for file_name in sorted(file_list):
    if file_name.endswith("Sys.vm"):
        continue
    f = open(file_name, "r")
    vm_trans(f, w)
    f.close()


path = "{}\\Sys.vm".format(vm_dir)
if os.path.isfile(path):
    f = open(path, "r")
    if f:
        vm_trans(f, w)
        f.close()
else :
    c.write_end(w)

w.close()