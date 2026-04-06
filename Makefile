FLAGS = -Wall -Wextra -Wno-implicit-fallthrough -std=gnu23 -fPIC -O2 

run: main.o
	gcc $(FLAGS) main.o -o app

main.o: main.c rstack.h
	gcc $(FLAGS) -c main.c
	
clean: 
	rm -f *.o app
