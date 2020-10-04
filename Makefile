all: lsm tester


tester.exe: lsm_params_module.c main.c bloom_params_module.c
	g++ -g -o $@ $^

bloom.exe: bloom_params_module.c bloom_main.c
	g++ -g -o $@ $^

lsm.exe: lsm_simulation.c lsm_params_module.c lsm_main.c iteration.c
	g++ -g -O3 -o $@ $^

clean:
	rm *.exe
