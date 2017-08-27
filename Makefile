sni-tray: libgwater/xcb/libgwater-xcb.c gdbus.c icons.c
	$(CC) -g -o $@ $^ -Wall -lxcb -lxcb-randr -lxcb-util `pkg-config --cflags --libs gio-2.0`
test-window: draw.c
	$(CC) -g -o $@ $^ -Wall -lxcb -lxcb-randr -lxcb-util `pkg-config --cflags --libs cairo gdk-pixbuf-2.0`
test-water: libgwater/xcb/libgwater-xcb.c draw.c
	$(CC) -g -o $@ $^ -Wall -lxcb -lxcb-randr -lxcb-util -lxcb-ewmh -lxcb-icccm `pkg-config --cflags --libs cairo gdk-pixbuf-2.0`
test-full: libgwater/xcb/libgwater-xcb.c draw.c gdbus.c icons.c
	$(CC) -g -o $@ $^ -Wall -lxcb -lxcb-randr -lxcb-util -lxcb-ewmh -lxcb-icccm `pkg-config --cflags --libs cairo gdk-pixbuf-2.0 gio-2.0`
clean:
	rm sni-tray test-window test-water
