all:
	ld -r -b binary -o toganet.o toganet.bin
	gcc -march=native -Ofast *.cpp ./nnue/*.cpp toganet.o -o toga3 -lm -lpthread -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2 -DUSE_SSE -msse -DUSE_AVX2 -mavx2

