CC_DIR := cc-library
TERMS_DIR := smt-symbolic-module
SAT_DIR := cadical
CORE_DIR := ccfv-core
FULL_DIR := ccfv-full

INCL_DIR := ./include
BUILD_DIR := ./build
BUILD_DIR_CORE := $(BUILD_DIR)/core
BUILD_DIR_FULL := $(BUILD_DIR)/full
CC_BUILD_DIR := ../$(CC_DIR)/build/src
CC_INCL_DIR := ../$(CC_DIR)/src
CC_UTIL_INCL_DIR := ../$(CC_DIR)/src/util
TERMS_BUILD_DIR := ../$(TERMS_DIR)/build
TERMS_INCL_DIR := ../$(TERMS_DIR)/include

CPPFLAGS=-Wall --pedantic -O3 -std=c++11
LDFLAGS=

# example: $(BUILD_DIR)/example.o $(BUILD_DIR)/ccfv-full.o
# 	g++ $(LDFLAGS) $(CPPFLAGS) -o example $< $(TERMS_BUILD_DIR)/term.o $(CC_BUILD_DIR)/CC.c.o $(CC_BUILD_DIR)/util/general.c.o $(CC_BUILD_DIR)/util/h-util.c.o $(CC_BUILD_DIR)/util/veriT-qsort.c.o $(BUILD_DIR)/libcadical.a

example: $(FULL_DIR)/example.cc $(FULL_DIR)/ccfv-full.hh $(FULL_DIR)/ccfv-full.cc $(CORE_DIR)/ccfv-core.cc $(CORE_DIR)/ccfv-core.hh $(CORE_DIR)/SAT.cc $(CORE_DIR)/SAT.hh
	cd ../$(TERMS_DIR) && $(MAKE)
	cd ../$(CC_DIR) && DEBUG=1 $(MAKE)
	mkdir -p $(BUILD_DIR)
ifeq ($(wildcard $(BUILD_DIR)/libcadical.a),)
	cd ../$(SAT_DIR) && ./configure && $(MAKE)
	cp ../$(SAT_DIR)/build/libcadical.a $(BUILD_DIR)
endif
	mkdir -p $(BUILD_DIR_CORE)
	mkdir -p $(INCL_DIR)
	cp ../$(SAT_DIR)/src/cadical.hpp $(INCL_DIR)
	g++ $(CPPFLAGS) -c $(CORE_DIR)/SAT.cc -o $(BUILD_DIR)/SAT.o -I $(INCL_DIR)
	g++ $(CPPFLAGS) -c $(CORE_DIR)/ccfv-core.cc -o $(BUILD_DIR)/ccfv-core.o -I $(INCL_DIR)
	g++ $(CPPFLAGS) -c $(FULL_DIR)/ccfv-full.cc -o $(BUILD_DIR)/ccfv-full.o -I $(TERMS_INCL_DIR) -I $(CC_INCL_DIR) -I $(CC_UTIL_INCL_DIR) -I $(CORE_DIR) -I $(INCL_DIR)
	g++ $(CPPFLAGS) -c $(FULL_DIR)/example.cc -o $(BUILD_DIR)/example.o -I $(TERMS_INCL_DIR) -I $(CC_INCL_DIR) -I $(CC_UTIL_INCL_DIR) -I $(CORE_DIR) -I $(INCL_DIR)
	g++ $(LDFLAGS) $(CPPFLAGS) -o example $(BUILD_DIR)/example.o $(TERMS_BUILD_DIR)/src/term.o $(CC_BUILD_DIR)/CC.c.o $(CC_BUILD_DIR)/util/general.c.o $(CC_BUILD_DIR)/util/h-util.c.o $(CC_BUILD_DIR)/util/veriT-qsort.c.o $(BUILD_DIR)/libcadical.a

# $(BUILD_DIR)/ccfv-full.o: $(FULL_DIR)/ccfv-full.cc $(FULL_DIR)/ccfv-full.hh
# 	g++ $(CPPFLAGS) -c $< -o $@ -I $(TERMS_INCL_DIR) -I $(CC_INCL_DIR) -I $(CC_UTIL_INCL_DIR) -I $(CORE_DIR) -I $(INCL_DIR)

# $(BUILD_DIR)/ccfv-core.o: $(CORE_DIR)/ccfv-core.cc
# 	g++ $(CPPFLAGS) -c $< -o $@ -I $(INCL_DIR)

# $(Build_DIR)/SAT.o: $(CORE_DIR)/SAT.cc
# 	g++ $(CPPFLAGS) -c $(CORE_DIR)/SAT.cc -o $(BUILD_DIR)/SAT.o -I $(INCL_DIR)

# .PHONY: modules
# modules:
# 	cd ../$(TERMS_DIR) && $(MAKE)
# 	cd ../$(CC_DIR) && DEBUG=1 $(MAKE)
# 	mkdir -p $(BUILD_DIR)
# ifeq ($(wildcard $(BUILD_DIR)/libcadical.a),)
# 	cd ../$(SAT_DIR) && ./configure && $(MAKE)
# 	cp ../$(SAT_DIR)/build/libcadical.a $(BUILD_DIR)
# endif
# 	mkdir -p $(BUILD_DIR_CORE)
# 	mkdir -p $(INCL_DIR)
# 	cp ../$(SAT_DIR)/src/cadical.hpp $(INCL_DIR)

debug: CPPFLAGS+= -O0
debug: CPPFLAGS+= -g -g3 -gdwarf-2
debug: CPPFLAGS+= -DDEBUG
debug: LDFLAGS+= -g -g3 -gdwarf-2


.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -f example

.PHONY: lclean
lclean:
	rm -rf $(BUILD_DIR)/*.o
	rm -f example
