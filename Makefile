PREFIX := /usr/local

obj :=

opt := -g
inc_flags := -Isrc

CFLAGS := $(opt) -std=c89 -pedantic -Wall -fPIC $(inc_flags) -DDEBUG

include src/Makefile-part

libmtexp.so.0.1.0: $(obj)
	$(CC) -shared -Wl,-soname,libmtexp.so.0 -o $@ $(obj)

libmtexp.a: $(obj)
	$(AR) rcs $@ $(obj)

include $(obj:.o=.d)

%.d: %.c
	@set -e; rm -f $@; $(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; rm -f $@.$$$$

.PHONY: clean
clean:
	$(RM) $(obj)

.PHONY: cleandep
cleandep:
	$(RM) src/*.d src/*.d.*

.PHONY: install
install:
	install src/mtexp.h $(PREFIX)/include/
	install libmtexp.* $(PREFIX)/lib/
	cd $(PREFIX)/lib; ln -s libmtexp.so.0.1.0 libmtexp.so
	ldconfig

.PHONY: remove
remove:
	rm $(PREFIX)/include/mtexp.h
	rm $(PREFIX)/lib/libmtexp.*
