FLAGS = -Wall -Wextra -Wno-implicit-fallthrough -std=gnu2x -fPIC -O2
TESTFLAGS = -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=reallocarray -Wl,--wrap=free -Wl,--wrap=strdup -Wl,--wrap=strndup

test_run:
	rm -f *.o app
	gcc -Wall -Wextra -fsanitize=address -g output.c main.c -o app
	./app



run: main.o
	gcc $(FLAGS) main.o -o app ./app

main.o: main.c rstack.h
	gcc $(FLAGS) -c main.c
	
clean: 
	rm -f *.o app

example: 
	gcc $(TESTFLAGS) -g memory_tests.c main.c rstack_example.c -o cts 
# 	./cts two
# 	./cts three
# 	./cts four
# 	./cts five
		./cts memory



librstack.so: main.h main.c
	gcc -fPIC -shared main.c -o librstack.so

compile: librstack.so