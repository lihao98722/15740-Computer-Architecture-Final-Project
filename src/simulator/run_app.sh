cd ../app && make clean && make
cd ../simulator

PIN_ROOT=/afs/cs.cmu.edu/academic/class/15740-s17/public/pin-3.0
APP_DIR=../app/app

make clean
make

$PIN_ROOT/pin -t obj-intel64/Simulator.so -- $APP_DIR
