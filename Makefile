.PHONY: clean
GNU = -std=gnu2x
CFLAGS = $(GNU) -Wall -Wextra -Wno-implicit-fallthrough -fPIC -O2
CMEMTESTFLAGS = -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=reallocarray -Wl,--wrap=free -Wl,--wrap=strdup -Wl,--wrap=strndup

example: main.o garbage_collector.o memory_tests.o rstack_example.c
	gcc $(CMEMTESTFLAGS) -g main.o garbage_collector.o rstack_example.c memory_tests.o -o cts 
	./cts two
	./cts three
	./cts four
	./cts five
	./cts memory

clean: 
	rm -f *.o *.out app tests librstack.so memory_tests.o

format:
	clang-format -i garbage_collector.c garbage_collector.h main.c main.h 

librstack.so: main.o garbage_collector.o memory_tests.o
	gcc $(CMEMTESTFLAGS) -shared main.o garbage_collector.o memory_tests.o -o librstack.so

memory_tests.o: memory_tests.h
	gcc $(GNU) -fPIC -c memory_tests.c -o memory_tests.o

main.o: main.c main.h rstack.h garbage_collector.h
	gcc $(CFLAGS) -c main.c

garbage_collector.o: garbage_collector.c garbage_collector.h main.h rstack.h
	gcc $(CFLAGS) -c garbage_collector.c