nanoc: nanoc.c
	cc -o nanoc nanoc.c
 
.PHONY: clean
clean:
	rm -fr nanoc
