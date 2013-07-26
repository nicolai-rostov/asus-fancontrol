MODEL=ux31a
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin

bin:
	gcc $(MODEL).c -o fanctrl

clean:
	rm -f fanctrl

install:
	install --mode 755 fanctrl            $(BINDIR)/fanctrl
	install --mode 755 asus-fancontrol.sh $(BINDIR)/asus-fancontrol

uninstall:
	rm -f $(BINDIR)/fanctrl
	rm -f $(BINDIR)/asus-fancontrol
