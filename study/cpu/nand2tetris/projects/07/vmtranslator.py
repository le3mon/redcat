import sys
import vmparser as p
import vmcodewriter as c

vm_file = sys.argv[1]
if len(sys.argv) != 2:
    print("vmtranslator.py [vm file]")
    sys.exit()

idx = vm_file.rfind("\\")
if idx == -1:
    fn = vm_file[:-3]
else:
    fn = vm_file[idx+1:-3]
out_fn = fn + ".asm"

f = open(vm_file, "r")
w = open(out_fn, "w")

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
    
    if ((command_type == p.C_PUSH) or (command_type == p.C_POP) or (command_type == p.C_FUNCTION) or (command_type == p.C_CALL)):
        arg2 = p.get_arg2(command)
    
    if ((command_type == p.C_PUSH) or (command_type == p.C_POP)):
        c.write_push_pop(command_type, arg1, arg2, w)
    
    if command_type == p.C_ARITHMETIC:
        c.write_arithemetic(arg1, w)

c.write_end(w)

w.close()
f.close()