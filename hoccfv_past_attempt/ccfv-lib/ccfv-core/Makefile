CPPFLAGS=-Wall --pedantic -O3 -std=c++11
LDFLAGS=

debug: CPPFLAGS+= -O0
debug: CPPFLAGS+= -g -g3 -gdwarf-2
debug: CPPFLAGS+= -DDEBUG
debug: LDFLAGS+= -g -g3 -gdwarf-2

example: example.o ccfv-core.o SAT.o
	g++ $(LDFLAGS) $(CPPFLAGS) -o example example.o ccfv-core.o SAT.o libcadical.a

example.o: example.cc ccfv-core.hh
	g++ $(CPPFLAGS) -c example.cc

SAT.o: SAT.cc SAT.hh
	g++ $(CPPFLAGS) -c SAT.cc

ccfv-core.o: ccfv-core.hh ccfv-core.cc
	g++ $(CPPFLAGS) -c ccfv-core.cc

debug: example

clean:
	rm -f example *~ *.o
