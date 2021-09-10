#!python

"""
This script is used to get median value of results.

For simple run, run.sh might be better.
"""

import os
import re
from argparse import ArgumentParser
from statistics import median

from subprocess import run, PIPE, DEVNULL


def parse_shell_time(o):
    m = re.search(r'^real[ \t]+(\d+)m([\d\.]+)s', o, re.MULTILINE)
    assert m, repr(o)
    return float(m.group(1) * 60) + float(m.group(2))


ITERATION = 100

if __name__ == '__main__':
    assert os.getenv('PS1', '').startswith('(venv)'), 'not in venv'

    arg_p = ArgumentParser()
    arg_p.add_argument('--cds', default=None, action='store')
    arg_p.add_argument('--no-seed', dest='seed', default=True, action='store_false')

    args = arg_p.parse_args()

    run('./venv/bin/pip install -i https://mirrors.aliyun.com/pypi/simple/ numpy scipy pymongo',
        check=True, shell=True, stdout=DEVNULL, stderr=DEVNULL)

    results = []

    env = {}

    if not args.seed:
        env['PYTHONHASHSEED'] = '0'
    if args.cds:
        if args.cds == 'POC':
            args.cds = 'PYTHONSHAREDCODE'
        elif args.cds == 'HEAD':
            args.cds = 'PYCDSMODE'
            env['PYCDSARCHIVE'] = 'test.img'
        else:
            arg_p.error(args.cds)

    for pkg in ('', 'sys', 'logging,json,decimal', 'pymongo', 'numpy', 'scipy'):
        stmt = f'import {pkg}' if pkg else ''
        cmd = f"time ./venv/bin/python -c '{stmt}'"

        cds_env = {}
        if args.cds:
            cds_env[args.cds] = '1'
            run(cmd, check=True, shell=True, stdout=DEVNULL, stderr=DEVNULL,
                env={**env, **cds_env})
            cds_env[args.cds] = '2'

        times = []
        for _ in range(ITERATION):
            pass
            time_output = run(cmd, shell=True, stderr=PIPE, stdout=DEVNULL,
                              env={**env, **cds_env})
            assert time_output.returncode == 0, time_output
            time_output = time_output.stderr.decode()

            times.append(parse_shell_time(time_output))

        results.append(median(times))

    print(*results, sep='\t')
