lib:
	gcc -g -Wall -std=gnu99 -I/usr/local/include/traceevent -I/usr/local/include/tracefs -I/usr/local/include/trace-cmd -ltraceevent -ltracefs -ltracecmd -c -o mylib.o stream.c
	ar rcs libbinparse.a mylib.o

clean:
	rm bin_parser.out
