GCC=gcc
CFLAGS=-static -Ofast -fomit-frame-pointer -DIS_64BIT
WIN=x86_64-w64-mingw32-g++
WINFLAGS=-static -Wall -fstrict-aliasing -fno-exceptions -fno-rtti  -Wno-unused-variable -Wno-unused-result -Wno-unused-but-set-variable -Wno-maybe-uninitialized -fPIC -fomit-frame-pointer

all:
	ld -r -b binary -o toganet.o toganet.bin
	$(GCC) -march=native $(CFLAGS) *.cpp ./nnue/*.cpp toganet.o -o toga3 -lm -lpthread -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2 -DUSE_SSE -msse -DUSE_AVX2 -mavx2
	$(GCC) $(CFLAGS) *.cpp ./nnue/*.cpp toganet.o -o toga3_avx2 -lm -lpthread -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2 -DUSE_SSE -msse -DUSE_AVX2 -mavx2
	$(GCC) $(CFLAGS) *.cpp ./nnue/*.cpp toganet.o -o toga3_sse -lm -lpthread -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2 -DUSE_SSE -msse
	$(GCC) $(CFLAGS) *.cpp ./nnue/*.cpp toganet.o -o toga3_basic -lm -lpthread
	$(WIN) $(WINFLAGS) -Ofast *.cpp ./nnue/*.cpp toganet.o -o toga3_avx2.exe -lm -lpthread -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2 -DUSE_SSE -msse -DUSE_AVX2 -mavx2

