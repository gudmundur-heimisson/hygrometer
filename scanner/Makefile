run: scanner
	sudo ./scanner

scanner: scanner.c
	gcc -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -lglib-2.0 -lgio-2.0 -o scanner scanner.c

clean:
	rm -f scanner
