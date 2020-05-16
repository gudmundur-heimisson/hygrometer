run: scanner
	sudo ./scanner

scanner: scanner.c
	gcc -I/usr/include/glib-2.0 \
      -I/usr/lib/arm-linux-gnueabihf/glib-2.0/include \
      -I/usr/lib/x86_64-linux-gnu/glib-2.0/include \
      -I/usr/include/x86_64-linux-gnu/curl \
      -I/usr/include/arm-linux-gnueabihf/curl \
      -I/usr/include/bluetooth \
      -lglib-2.0 \
      -lgio-2.0 \
      -lgobject-2.0 \
      -lcurl \
      -o scanner \
      scanner.c

clean:
	rm -f scanner
