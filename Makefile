all: lsm.exe tester.exe

CFLAGS=\
	   	-g\
		-O3\
#		-fsanitize=address\

tester.exe: lsm_params_module.c main.c bloom_params_module.c
	g++ -g $(CFLAGS) -o $@ $^

bloom.exe: bloom_params_module.c bloom_main.c
	g++ -g $(CFLAGS) -o $@ $^

lsm.exe: lsm_simulation.c lsm_params_module.c lsm_main.c iteration.c lsm_last_compaction.c lsm_last_gc.cpp
	g++ -g $(CFLAGS) -o $@ $^

clean:
	rm *.exe
