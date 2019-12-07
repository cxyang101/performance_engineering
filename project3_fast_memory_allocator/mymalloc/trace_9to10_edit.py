import re

def create_9to10_v0():
    with open('traces/trace_c9_v0', 'r') as old, open('extra_traces/trace_c10_v0', 'w+') as tr:
        for line in old:
            if line.startswith('a 0'):
                tokens = re.split(r'\s', line)
                tr.write('a 0 614784\n')
            elif line.startswith('r 0'):
                tokens = re.split(r'\s', line)
                new_size = 614784 + 512 - int(tokens[2])
                tr.write('r 0 {}\n'.format(new_size))
            else:
                tr.write(line)


def create_9to10_v1():
    with open('additional_traces/trace_c9_v1', 'r') as old, open('extra_traces/trace_c10_v1', 'w+') as tr:
        for line in old:
            if line.startswith('a 0'):
                tokens = re.split(r'\s', line)
                tr.write('a 0 28087\n')
            elif line.startswith('r 0'):
                tokens = re.split(r'\s', line)
                new_size = 28087 + 4092 - int(tokens[2])
                tr.write('r 0 {}\n'.format(new_size))
            else:
                tr.write(line)


if __name__ == '__main__':
    create_9to10_v0()
    create_9to10_v1()
