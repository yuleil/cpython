M=logging,json,decimal

PROG="import $M"
P=./python.exe

export PYTHONHASHSEED=0
#export PYTHONPROFILEIMPORTTIME=1

PYTHONSHAREDCODE=1 $P -c "import $M" >/dev/null 2>&1
time PYTHONVERBOSE2=0 PYTHONSHAREDCODE=2 $P -c "$PROG"
time PYTHONVERBOSE2=0 $P -c "$PROG"

echo

#PYTHONSHAREDCODE=1 $P -c "import $M" >/dev/null&& PYTHONVERBOSE2=1 PYTHONSHAREDCODE=2 $P -c "$PROG" 2>&1| grep get_code | awk '{s+=$3} END {print s}'
#PYTHONSHAREDCODE=1 $P -c "import $M" >/dev/null&& PYTHONVERBOSE2=1 PYTHONSHAREDCODE=2 $P -c "$PROG" 2>&1| grep find_spec | awk '{s+=$3} END {print s}'
echo
PYTHONVERBOSE2=1 $P -c "$PROG"  2>&1| grep get_code | awk '{s+=$3} END {print s}'
PYTHONVERBOSE2=1 $P -c "$PROG"  2>&1| grep find_spec | awk '{s+=$3} END {print s}'
