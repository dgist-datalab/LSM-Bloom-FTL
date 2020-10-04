all: simdcomp

simdcomp: simd_comp.c
	gcc -fPIC -g -o compress simd_comp.c -lsimdcomp

clean:
	rm -f test
	rm -f compress
