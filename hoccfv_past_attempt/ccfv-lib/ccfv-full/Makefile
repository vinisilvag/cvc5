CPPFLAGS=-Wall --pedantic -O3 -std=c++11
LDFLAGS=

debug: CPPFLAGS+= -O0
debug: CPPFLAGS+= -g -g3 -gdwarf-2
debug: CPPFLAGS+= -DDEBUG
debug: LDFLAGS+= -g -g3 -gdwarf-2

example: example.o ccfv-full.o
	g++ $(LDFLAGS) $(CPPFLAGS) -o example example.o ccfv-full.o ccfv-core.o term.o CC.o general.o h-util.o veriT-qsort.o SAT.o libcadical.a

example.o: example.cc ccfv-full.hh
	g++ $(CPPFLAGS) -c example.cc

ccfv-full.o: ccfv-full.cc ccfv-full.hh
	g++ $(CPPFLAGS) -c ccfv-full.cc

debug: example

clean:
	rm -f example *~ ccfv-full.o example.o

