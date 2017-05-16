PIN_ROOT=/afs/cs.cmu.edu/academic/class/15740-s17/public/pin-3.0

# bechmark applications
OCEANS_DIR=../splash2/codes/apps/ocean/non_contiguous_partitions

cd $OCEANS_DIR
make clean
make

cd ../../../../../simulator
make clean
make

$PIN_ROOT/pin -t obj-intel64/Simulator.so -- $OCEANS_DIR/OCEAN -n34 -p4 -e1e-7 -r20000.0 -t28800.0
