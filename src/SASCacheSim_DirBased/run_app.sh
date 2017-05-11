cd ../App && make clean && make
cd ../SASCacheSim_DirBased

PIN_ROOT=/afs/cs.cmu.edu/academic/class/15740-s17/public/pin-3.0
APP_DIR=../App/app

make clean
make

$PIN_ROOT/pin -t obj-intel64/SASCache.so -- $APP_DIR
