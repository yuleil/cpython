M="logging,json,decimal"

PROG="import $M"
P=./python.exe

export PYTHONHASHSEED=0
PYTHONSHAREDCODE=1 $P -c "import $M"
time PYTHONVERBOSE2=0 PYTHONSHAREDCODE=2 $P -c "$PROG"
time PYTHONVERBOSE2=1 $P -c "$PROG"

