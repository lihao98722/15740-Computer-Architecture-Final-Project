PIN_ROOT=/afs/cs.cmu.edu/academic/class/15740-s17/public/pin-3.0

# bechmark applications
RADIX_DIR=../splash2/codes/kernels/radix

make clean
make

$PIN_ROOT/pin -t obj-intel64/Simulator.so -- $RADIX_DIR/RADIX
