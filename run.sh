set -e

M=numpy
V=venv
P=./python.exe
O=tmp.log
IMG=test.img

conda deactivate >/dev/null 2>&1 || true
rm -rf $V && $P -m venv $V && source $V/bin/activate
P=python
pip install -qq numpy

export PYTHONPROFILEIMPORTTIME=x
export PYCDSVERBOSE=2
export PYTHONDONTWRITEBYTECODE=1
find . -depth -name '__pycache__' -exec rm -rf {} ';'
find . -name '*.py[co]' -exec rm -f {} ';'

function foo() {
  # time output differently on Linux and Macos, do not parse.
  time ($P -c "import $M" >$O 2>&1)
#  $P -c "import $M" >$O 2>&1
  # total import time of all packages
  ALL=$(grep 'import time' < $O | awk '{if(NR>1) s+=$3} END {print s}')
  # total import time of top-level packages (cumulative)
  TOP=$(grep '\d | [^ ]' < $O | grep 'import time' | awk '{if(NR>1) s+=$5} END {print s}')
  echo "$ALL $TOP"
}

PYCDSMODE=1 PYCDSARCHIVE=$IMG foo > /dev/null 2>&1

# no CDS, no PYC
PYCDSMODE=0 foo
# CDS, no PYC
PYCDSMODE=2 PYCDSARCHIVE=$IMG foo

export PYTHONDONTWRITEBYTECODE=
foo > /dev/null 2>&1
# no CDS, PYC
PYCDSMODE=0 foo
# CDS, PYC
PYCDSMODE=2 PYCDSARCHIVE=$IMG foo