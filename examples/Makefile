obj := mtex_expr.o image.o image_tga.o

CFLAGS := -std=c89 -pedantic -Wall -g

ex1: $(obj)
	$(CC) -o $@ $(obj) -lglut -lmtexp

.PHONY: clean
clean:
	rm -f $(obj) ex1
