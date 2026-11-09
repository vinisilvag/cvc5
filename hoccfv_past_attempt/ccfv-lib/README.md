# CCFV library

Higher order CCFV implementation

# Compiling (to update)

Compiling requires the presence of:

 - Congruence Closure ( https://gitlab.uliege.be/smt-modules/cc-library )
 - SMT symbolic module for parsing and terms ( https://gitlab.uliege.be/smt-modules/smt-symbolic-module )
 - the SAT solver Cadical ( https://github.com/arminbiere/cadical ), or another one, but then the makefile should be adapted

in the same directory as ccfv-lib.

# Current road map

- Adapt interface of CCFV-full to take two vectors of terms (one at a time): one
  for E, one for L
- Use CC to store the literals in E
- Create a backtracking point, add *the terms* of L (i.e., under the
  equalities/disequalities) to CC
  - Keep track of the variables that occur in L, so that querying them in CC
    will give what they have been assigned to
- Detect trivial assignments (until fix point)
  - when doing so, add the respective equalities to CC
  - keep track of the original variables that remain after this process (they'll
    be the ones to query the ccfv-core for a substitution later on)
- Process ground literals in L to discard the entailed ones and finish with a
  failure if any non-entailed
  - this will require being able to determine groundness modulo assignments to
    variables. So need to consider that the variable may be the representative
    of its class, even if there is a ground term there as well
- Do flattening, which will only impact core
  - The actual new literals should be done only in the representation for the
    ccfv-core
- call ccfv-core's solve()    
- for the variables that were saved after the trivial assignments, query them
  from ccfv-core for solutions (create backtracking point in CC before this)
  - add the respective equalities to CC     
- from CC, get solution as some ground term from the classes of the variables.
