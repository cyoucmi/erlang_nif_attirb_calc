FLAGS = -g -std=c99

## TODO:
ERL_ROOT = /usr/local/lib/erlang/erts-5.10.4

ECC = erlc

OUTDIR = ./
RCS = $(wildcard *.erl)
OBJS = $(patsubst %.erl,$(OUTDIR)/%.beam,$(RCS))

all:attrib_nif.so $(OBJS)

attrib_nif.so:attrib.c attrib.h header.h attrib_nif.c
	gcc -o $@ $^ --shared -fpic -D ERL_NIF  $(FLAGS) -Wall -I $(ERL_ROOT)/emulator/beam -I $(ERL_ROOT)/include
	
%.beam:%.erl
	$(ECC) $^

clean: 
	rm  attrib_nif.so *.beam
