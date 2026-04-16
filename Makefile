FLAGS = -Wall -Wextra -Wno-implicit-fallthrough -std=gnu2x -fPIC -O2
MEMTESTFLAGS = -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=reallocarray -Wl,--wrap=free -Wl,--wrap=strdup -Wl,--wrap=strndup


clean: 
	rm -f *.o app

example: memory_tests_compile
	gcc $(MEMTESTFLAGS) -g main.c garbage_collector.c rstack_example.c memory_tests.o -o cts 
	./cts two
	./cts three
	./cts four
	./cts five
	./cts memory

#compilation of memory_tests to object files
memory_tests_compile: 
	gcc -std=gnu2x -fPIC -c -g memory_tests.c

format:
	clang-format -i garbage_collector.c garbage_collector.h main.c main.h 

compile:
	gcc -fPIC -shared -g main.c garbage_collector.c -o librstack.so