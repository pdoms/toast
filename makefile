CC=gcc
CFLAGS=-Wall -Wextra

toaster: toaster.c
	$(CC) toaster.c $(CFLAGS) -o toaster 

clean:
	rm -rf toaster


