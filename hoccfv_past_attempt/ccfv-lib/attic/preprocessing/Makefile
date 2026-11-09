CPPFLAGS=-Wall --pedantic -O3 -std=c++17
LDFLAGS=

debug: CPPFLAGS+= -O0
debug: CPPFLAGS+= -g -g3 -gdwarf-2
debug: CPPFLAGS+= -DDEBUG
debug: LDFLAGS+= -g -g3 -gdwarf-2

example: example.o ccfv_preproc.o
	g++ $(LDFLAGS) $(CPPFLAGS) -o example example.o ccfv_preproc.o CC.o term.o general.o h-util.o veriT-qsort.o

example.o: example.cc ccfv_preproc.hh
	g++ $(CPPFLAGS) -c example.cc

ccfv_preproc.o: ccfv_preproc.cc ccfv_preproc.hh
	g++ $(CPPFLAGS) -c ccfv_preproc.cc

debug: example

clean:
	rm -f example *~ ccfv_preproc.o example.o

