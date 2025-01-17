CC=gcc
CFLAGS=-Wall -Wextra

example: example.c
	cp ../toast.h .
	$(CC) example.c $(CFLAGS) -o example

harness: 
	cp ../toast.h .
	cp ../toaster.c .
	$(CC) toaster.c $(CFLAGS) -o toaster
	./toaster

clean:
	rm -rf toast.h
	rm -rf example
	rm -rf toaster.c
	rm -rf toaster
