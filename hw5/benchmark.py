#!/usr/bin/env python3
from subprocess import Popen, PIPE, STDOUT, TimeoutExpired
import re
import argparse
import time

proc_name = 'adtTest'
proc_ext_avail = ['dlist', 'array', 'bst']
proc_ext_do = []
repeat = 5
magnititudes = [ int(n) for n in [1e4, 3e4, 1e5, 3e5, 1e6, 3e6, 1e7] ]
timeout = 300

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('dofile', help='the dofile name inside ./benchmarks to test.\
 __N__ inside files are substituted with different magnititudes.')
    parser.add_argument('-d', '--dlist', action="store_true", help='only perform tests on dlist')
    parser.add_argument('-a', '--array', action="store_true", help='only perform tests on array')
    parser.add_argument('-b', '--bst', action="store_true", help='only perform tests on BST')
    args = parser.parse_args()

    do_in = open('./tests/' + args.dofile, 'r').read()
    verdict = open('./benchmarks/' + args.dofile + '.log', 'w')

    proc_ext_do = []
    for ext in proc_ext_avail:
        if getattr(args, ext):
            proc_ext_do.append(ext)
    if not len(proc_ext_do):  # no ext provided mean all
        proc_ext_do = proc_ext_avail

    for ext in proc_ext_do:
        for N in magnititudes:
            print('Testing {}, preforming N={}:'.format(ext, N))

            for _ in range(repeat):
                print('  #{}... '.format(_), end='', flush=True)
                do_content = re.sub(r'__N__', str(N), do_in)
                ts = time.time()
                proc = Popen(['{}/{}.{}'.format('./', proc_name, ext)], stdin=PIPE, stdout=PIPE, stderr=STDOUT)

                verdict.write('======== {} :: {} :: {} ========\n'.format(ext, N, _))
                killed = 0

                try:
                    outs, errs = proc.communicate(do_content.encode(), timeout)
                    print('OK. {:.3f} ms'.format(time.time() - ts))
                except TimeoutExpired:
                    outs, errs = proc.communicate()
                    print('Killed... ')
                    killed = 1

                verdict.write(outs.decode('utf-8'))

                if killed:
                    verdict.write('\n##### KILLED after {} sec #####\n'.format(timeout))

if __name__ == '__main__':
    main()
