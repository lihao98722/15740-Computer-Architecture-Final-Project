PIN_ROOT=/afs/cs.cmu.edu/academic/class/15740-s17/public/pin-3.0

# bechmark applications
FMM_DIR=../splash2/codes/apps/fmm

cd $FMM_DIR
make clean
make

cd ../../../../simulator
make clean
make

$PIN_ROOT/pin -t obj-intel64/Simulator.so -- $FMM_DIR/FMM < $FMM_DIR/inputs/input.256

