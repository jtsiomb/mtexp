obj := mtex_expr.o image.o image_tga.o

CFLAGS := -std=c89 -pedantic -Wall -g
LDFLAGS := -lGL -lGLU -lglut ../libmtexp.a

ex1: $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) ex1
