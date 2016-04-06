all:
	gcc -g `pkg-config --cflags gtk+-3.0` -o dmk gui.c `pkg-config --libs gtk+-3.0`

