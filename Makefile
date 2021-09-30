# example: 
# ./bin2coe --in ../vortex_hbm/fibonacci.bin --offset=131072 --depth=262144 --word=4 --default="DEADBEEF"
# ./bin2coe --in ../vortex_hbm/fibonacci.bin --offset=8192 --depth=16384 --word=64 --default="DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF"

default: bin2coe

bin2coe: main.cpp
	g++ main.cpp -lboost_program_options -o bin2coe

clean:
	rm -f bin2coe