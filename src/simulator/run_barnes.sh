PIN_ROOT=/afs/cs.cmu.edu/academic/class/15740-s17/public/pin-3.0

# bechmark applications
BARNES_DIR=../splash2/codes/apps/barnes

make clean
make

$PIN_ROOT/pin -t obj-intel64/Simulator.so -- $BARNES_DIR/BARNES < $BARNES_DIR/input
