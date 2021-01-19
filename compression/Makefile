all: compress

compress: simd_comp.c
	gcc -fPIC -g -O3 -o compress simd_comp.c -lsimdcomp

debug: simd_comp.c
	gcc -fPIC 	-fsanitize=address -g -o compress simd_comp.c -lsimdcomp

clean:
	rm -f test
	rm -f compress
	rm -f *.exe
