LDFLAGS = lib/libtraceevent/lib/libtraceevent.a lib/libtracefs/lib/tracefs/libtracefs.a lib/trace-cmd/lib/trace-cmd/libtracecmd.a 
CFLAGS = -Ilib/libtraceevent/src/ -Ilib/libtracefs/include -Ilib/trace-cmd/include/trace-cmd

LDFLAGS_DYN = lib/libtraceevent/lib/libtraceevent.so lib/libtracefs/lib/tracefs/libtracefs.so lib/trace-cmd/lib/trace-cmd/libtracecmd.so 
LDFLAGS_DIR = -Llib/libtraceevent/lib -Llib/libtracefs/lib/tracefs -Llib/trace-cmd/lib/trace-cmd 

obj/%.o: src/%.c
	gcc -c ${CFLAGS} -o $@ $^ 

out/binparser.o: obj/stream.o obj/record.o
	gcc -static ${LDFLAGS} -o $@ $^

out/binparser.a: out/binparser.o
	ar rcs $@ $^

libstatic: out/binparser.a

# What about record.c?
libdyn:
	gcc -g -Wall -std=gnu99 -I/usr/local/include/traceevent -I/usr/local/include/tracefs -I/usr/local/include/trace-cmd -ltraceevent -ltracefs -ltracecmd -c -o mylib.o stream.c
	ar rcs libbinparse.a mylib.o

# What about record.c?
libdyn-test:
	gcc -g -Wall -std=gnu99 ${CFLAGS} ${LDFLAGS_DIR} -c -o mylib.o stream.c
	ar rcs libbinparse.a mylib.o

libstatic-test:
	gcc -g -Wall -std=gnu99 lib/libtraceevent/lib/libtraceevent.a lib/libtracefs/lib/tracefs/libtracefs.a lib/trace-cmd/lib/trace-cmd/libtracecmd.a -Ilib/libtraceevent/src -Ilib/libtracefs/include -Ilib/trace-cmd/include/trace-cmd -c -o mylib.o stream.c
	ar rcs libbinparse.a mylib.o

clean:
	rm obj/*
	rm out/*
	rm *.o
	rm *.a
