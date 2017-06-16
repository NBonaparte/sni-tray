sni-tray: gdbus.c icons.c
	$(CC) -o $@ $^ -Wall -pedantic `pkg-config --cflags --libs gio-2.0`

clean:
	rm sni-tray
