create_library: mysocket.o
	ar rcs libmsocket.a mysocket.o

mysocket.o: mysocket.h
	gcc -c mysocket.c

clean:
	rm -rf *.o *.a