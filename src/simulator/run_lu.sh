PIN_ROOT=/afs/cs.cmu.edu/academic/class/15740-s17/public/pin-3.0

# bechmark applications
LU_DIR=../splash2/codes/kernels/lu/non_contiguous_blocks
cd $LU_DIR 
make clean
make

cd ../../../../../simulator
make clean
make

$PIN_ROOT/pin -t obj-intel64/Simulator.so -- $LU_DIR/LU

