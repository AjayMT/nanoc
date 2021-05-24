
nanoc: nanoc.c
	$(CC) -o nanoc nanoc.c

archive.a: test/archive.c
	i386-elf-gcc -c test/archive.c -o archive.o
	i386-elf-ar r archive.a archive.o
 
.PHONY: clean
clean:
	rm -fr nanoc a.out archive.o archive.a
