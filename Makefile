
nanoc: nanoc.c
	$(CC) -o nanoc nanoc.c
 
.PHONY: clean
clean:
	rm -fr nanoc a.out
