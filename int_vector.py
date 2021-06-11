import json
import os

f = open('compile_commands.json', 'r')

contents = f.read()
compile_commands = json.loads(contents)

output = open('gather_csv.sh', 'w')

for command_tuple in compile_commands:
    comp = command_tuple['command']
    comp = 'build/bin/clang -Xclang -int-vector-dump -I/home/eumakri/Documents/llvm-project/build/include ' + ' '.join(comp.split()[1:]) + ' > ' + command_tuple['file'] + '.csv\n'
    output.write(comp)



