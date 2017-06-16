sni-tray: gdbus.c icons.c
	$(CC) -g -o $@ $^ -Wall `pkg-config --cflags --libs gio-2.0`

clean:
	rm sni-tray
