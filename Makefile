# example: 
# ./bin2coe --binary ../vortex_hbm/fibonacci.bin --binaddr=131072 --depth=262144 --wordsize=4 --default="DEADBEEF"
# ./bin2coe --binary ../vortex_hbm/fibonacci.bin --binaddr=8192 --depth=16384 --wordsize=64 --default="DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF"

default: bin2coe

bin2coe: main.cpp
	g++ -g -O0 main.cpp -lboost_program_options -o bin2coe

test:
	./bin2coe --binary sample.bin --data sample.dat --binaddr=8192 --depth=16384 --wordsize=64 --default="DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF"

clean:
	rm -f bin2coe