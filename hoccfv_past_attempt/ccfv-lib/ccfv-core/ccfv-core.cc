/*
  --------------------------------------------------------------
  CCFV computation module
  --------------------------------------------------------------
*/

#include <iostream>
#include <algorithm>
#include <cassert>
#include <ostream>

#include "ccfv-core.hh"

using namespace CCFV_SAT;

/**
   \author Pascal Fontaine & Sophie Tourret
   \brief adhoc class to compute sets of clauses, from 3-DNFs */
class CCCFV_prop_constraint {
public:
  CCCFV_prop_constraint(void);
  void add(Tvar Q);
  void add(Tvar p1, Tvar p2);
  void add(Tvar p1, Tvar p2, Tvar p3);
  
  /* the following functions send clauses to the SAT solver */
  void flush(Csolver &SAT_solver);
  void flush_clause(Csolver &SAT_solver, Tlit l1, Tlit l2);
  void flush_clause(Csolver &SAT_solver, Tlit l1, Tlit l2, Tlit l3);
  void flush_mex(Csolver &SAT_solver, Tvar p1, Tvar p2);

  /* printer */
  void printer(void);

private:
  std::vector<std::vector<Tlit>> cubes;
};

CCCFV_prop_constraint::CCCFV_prop_constraint(void)
{
  ; /* Nothing to do (for now) */
}

void CCCFV_prop_constraint::printer(void)
{
  for (auto &cube: cubes)
  {
    for (auto &lit: cube)
      std::cout << lit_DIMACS(lit) << " ";
    std::cout << "0\n";
  }
}

void
CCCFV_prop_constraint::add(Tlit l1)
{
  if (l1 == lit_false)
    return;
  cubes.resize(cubes.size() + 1);
  if (l1 != lit_true) cubes.back().push_back(l1);
}

void
CCCFV_prop_constraint::add(Tlit l1, Tlit l2)
{
  if (l1 == lit_false || l2 == lit_false)
    return;
  cubes.resize(cubes.size() + 1);
  if (l1 != lit_true) cubes.back().push_back(l1);
  if (l2 != lit_true) cubes.back().push_back(l2);
}

void
CCCFV_prop_constraint::add(Tlit l1, Tlit l2, Tlit l3)
{
  if (l1 == lit_false || l2 == lit_false || l3 == lit_false)
    return;
  cubes.resize(cubes.size() + 1);
  if (l1 != lit_true) cubes.back().push_back(l1);
  if (l2 != lit_true) cubes.back().push_back(l2);
  if (l3 != lit_true) cubes.back().push_back(l3);
}

/* the following functions send clauses to the SAT solver */
void
CCCFV_prop_constraint::flush(Csolver &SAT_solver)
{
  unsigned i = 0;
  /* if one cube is empty, clause is true, nothing to add */
  for (i = 0; i < cubes.size(); i++)
    if (cubes[i].size() == 0) return;
  /* remove all singletons with var 0 (must be false)
     in principle this is useless */
  i = 0;
  for (unsigned j = 0; j < cubes.size(); j++)
    if (cubes[j].size() >= 1 || lit_var(cubes[j][0]) != 0)
      cubes[i++] = cubes[j];
  cubes.resize(i);

  /* generate all clauses in the CNF,
     basically enumerating all possibilities to pick one literal in each clause,
     that is, counting... */
  std::vector<unsigned> index(cubes.size(), 0);
  do
  {
    /* print current constraint */
    for (i = 0; i < cubes.size(); i++)
      std::cout << (i?" v ":"") << lit_DIMACS(cubes[i][index[i]]);
    std::cout << std::endl;
    /* add current constraint to solver */
    for (i = 0; i < cubes.size(); i++)
      SAT_solver.add(cubes[i][index[i]]);
    SAT_solver.add(0);
    /* go to next one */
    for (i = 0; i < cubes.size(); i++)
      if (index[i] + 1 < cubes[i].size())
        {
          index[i] += 1;
          break;
        }
      else
        index[i] = 0;
  }
  while (i < cubes.size());
}

void
CCCFV_prop_constraint::flush_clause(Csolver &SAT_solver, Tlit l1, Tlit l2)
{
  std::cout << lit_DIMACS(l1) << " v " << lit_DIMACS(l2) << std::endl;
  SAT_solver.add_clause(l1, l2);
}

void
CCCFV_prop_constraint::flush_clause(Csolver &SAT_solver, Tlit l1, Tlit l2, Tlit l3)
{
  std::cout << lit_DIMACS(l1) << " v " << lit_DIMACS(l2) << " v " << lit_DIMACS(l3) << std::endl;
  SAT_solver.add_clause(l1, l2, l3);
}

void
CCCFV_prop_constraint::flush_mex(Csolver &SAT_solver, Tvar p1, Tvar p2)
{
  std::cout << "-" << p1 << " v " << "-" << p2 << std::endl;
  SAT_solver.add_clause(var_to_lit(p1, false), var_to_lit(p2, false));
}

CCCFV_solver::CCCFV_solver(void)
{
  P_Q_to_x.emplace_back(0,0);
}

void
CCCFV_solver::class_is_var(TCCFV_class x)
{
  expand(x);
  if (!is_var[x])
  {
    is_var[x] = true;
    vars.push_back(x);
  }
}

void
CCCFV_solver::class_sig(TCCFV_class c0, TCCFV_class c1, TCCFV_class c2)
{
  expand(c0);
  expand(c1);
  expand(c2);
  Epos.emplace_back(c0, c1, c2);
}

void
CCCFV_solver::class_sig(TCCFV_class c, unsigned n, TCCFV_class u0, TCCFV_class * u)
{
  /* for first-order only, not dealt with now */
  /* TODO3*/
}

void
CCCFV_solver::class_neq(TCCFV_class c0, TCCFV_class c1)
{
  expand(c0);
  expand(c1);
  Eneg.emplace_back(c0, c1);
}

void
CCCFV_solver::constraint_pos(TCCFV_class u0, TCCFV_class u1, TCCFV_class u2)
{
  expand(u0);
  expand(u1);
  expand(u2);
  Lpos.emplace_back(u0, u1, u2);
}

void
CCCFV_solver::constraint_neg(TCCFV_class u0, TCCFV_class u1)
{
  assert(is_var[u0]); /*in disequality constraints, the first parameter must be a variable
    as a consequence, the following line is useless */
  expand(u0);
  expand(u1);
  Lneg.emplace_back(u0, u1);
}

void
CCCFV_solver::set_class_type(TCCFV_class c, TCCFV_type type)
{
  class_type[c] = type;
}

void 
CCCFV_solver::Cycle_Detection(void)
{
  index.resize(0);
  low.resize(0);
  onstack.resize(0);
  index.resize(is_var.size(), 0);
  low.resize(is_var.size(), 0);
  onstack.resize(is_var.size(), false);
  counter = 1;
  S_stack.resize(0);

  for (auto &x : vars)
    for (unsigned i = 0; i < 2; i++)
      if (child[i][x])
      child[i][x] = FFind(child[i][x]);
    
  for (auto &x : vars)
    if (index[FFind(x)] == 0)
      strongconnect(FFind(x));
}

void
CCCFV_solver::strongconnect(TCCFV_class x)
{
  index[x] = counter;
  low[x] = counter;
  onstack[x] = true;
  S_stack.push_back(x);
  counter++;
  for (unsigned i = 0; i < 2; i++)
    if (child[i][x])
    {
      TCCFV_class y = child[i][x];
      if (!index[y])
      {
        strongconnect(y);
        low[x] = std::min(low[x], low[y]);
      }
      else if (onstack[y])
        low[x] = std::min(low[x], low[y]);
    }

  if (low[x] == index[x])
  {
    bool flag = floatable[x] && child[0][x] != x && child[1][x] != x;
    TCCFV_class y = S_stack.back();
    S_stack.pop_back();
    while (y != x)
    {
      flag = false;
      onstack[y] = false;
      floatable[y] = false;
      y = S_stack.back();
      S_stack.pop_back();
    }
    onstack[x] = false;
    floatable[x] = flag;
  }
}

bool
CCCFV_solver::solve(void)
{
  if (solution_count) {
    // adds negation of previously found model, if any, in SAT solver
    for (auto &v : SAT_solver.model)
      SAT_solver.add(var_to_lit(v, false));
    SAT_solver.add(0);
  } else {
    std::cerr << "Floater equivalences computation" << std::endl;
    FCompute();
    std::cerr << "Ground and type constraints computation" << std::endl;
    Ground_or_Type_Constraints();
    std::cerr << "Cycle detection" << std::endl;
    Cycle_Detection();
    std::cerr << "Constraints generation" << std::endl;
    Generate_Constraints();
  }
  std::cerr << "SAT resolution" << std::endl;
  debug_CNF();

  if (SAT_solver.solve() != 10) return false;

  std::cerr << "SAT model:" << std::endl;
  debug_model();
  std::cerr << "Solution reconstruction" << std::endl;
  Substitution_Reconstruction();
  std::cerr << "before normalization" << std::endl;
  debug_solution();
  Substitution_Normalize();
  std::cerr << "after normalization" << std::endl;
  debug_solution();
  solution_count++;
  return true;
}

void
CCCFV_solver::Generate_Constraints(void)
{
  std::cerr << "negative core constraints" << std::endl;
  /* negative core constraints */
  for (auto &lneg : Lneg)
  {
    CCCFV_prop_constraint constraint;
    if (!is_var[lneg.c[1]])
      /* /!\ tests only disequalities included in E, not entailment by E /!\ */
      for (auto &eneg : Eneg)
      {
        if (eneg.c[0] == lneg.c[1])
          constraint.add(var_to_lit(get_P(lneg.c[0], eneg.c[1])));
        else if (eneg.c[1] == lneg.c[1])
          constraint.add(var_to_lit(get_P(lneg.c[0], eneg.c[0])));
      }
    else
      for (auto &eneg : Eneg)
      {
        constraint.add(var_to_lit(get_P(lneg.c[0], eneg.c[0])), var_to_lit(get_P(lneg.c[1], eneg.c[1])));
        constraint.add(var_to_lit(get_P(lneg.c[0], eneg.c[1])), var_to_lit(get_P(lneg.c[1], eneg.c[0])));
      }
    constraint.flush(SAT_solver);
  }

  std::cerr << "positive core constraints" << std::endl;
  /* positive core constraints */
  for (auto &lpos : Lpos)
  {
    CCCFV_prop_constraint constraint;
    if (is_var[lpos.c[0]])
    {
      if (floatable[FFind(lpos.c[0])])
        constraint.add(var_to_lit(get_Q(FFind(lpos.c[0]))));
      for (auto &epos : Epos)
        constraint.add(var_to_lit(get_P(lpos.c[0], epos.c[0])),
                       build_P(lpos.c[1], epos.c[1]),
                       build_P(lpos.c[2], epos.c[2]));
    }
    else
      for (auto &epos : Epos) /* Here we could optimize and directly get the right epos */
        if (epos.c[0] == lpos.c[0])
          constraint.add(build_P(lpos.c[1], epos.c[1]),
                         build_P(lpos.c[2], epos.c[2]));
    constraint.flush(SAT_solver);
  }

  std::cerr << "floater injectivity constraints" << std::endl;
  /* floater injectivity */
  std::vector<TCCFV_class> V;
  std::vector<std::vector<TCCFV_class>> children[2];
  children[0].resize(is_var.size());
  children[1].resize(is_var.size());
  for (auto &lpos : Lpos)
    if (is_var[lpos.c[0]])
    {
      V.push_back(FFind(lpos.c[0]));
      if (is_var[lpos.c[1]]) children[0][FFind(lpos.c[0])].push_back(lpos.c[1]);
      if (is_var[lpos.c[2]]) children[1][FFind(lpos.c[0])].push_back(lpos.c[2]);
    }

  /* remove duplicates from vector (used for a set) */  
  sort(V.begin(), V.end()); V.erase(unique(V.begin(),V.end()), V.end());
  
  /* vector indexed by floater equivalence representative,
     giving the list of terms t such that P_x_t exist for some x in the floater equivalence class */
  std::vector<std::vector<TCCFV_class>> Fx_to_c(is_var.size());
  for (Tvar p = 1; p < P_Q_to_x.size(); p++)
    if (P_Q_to_x[p].c)
      Fx_to_c[FFind(P_Q_to_x[p].x)].push_back(P_Q_to_x[p].c);


  for (auto &x : V)
  {
    std::cerr << "variable " << x << " has ground children: " << ground[0][x] << " and " << ground[1][x] << std::endl;
    if (floatable[x]) for (unsigned i = 0; i < 2; i++) {
      CCCFV_prop_constraint constraint;
      sort(children[i][x].begin(), children[i][x].end());
      children[i][x].erase(unique(children[i][x].begin(), children[i][x].end()),
                           children[i][x].end());
      if (ground[i][x])
        for (auto &y : children[i][x]) {
          constraint.flush_clause(SAT_solver,
                                  var_to_lit(get_Q(FFind(x)), false),
                                  var_to_lit(get_P(y, ground[i][x]), true));
        }
      else if (children[i][x].size() > 1)
        for (unsigned j = 0; j < children[i][x].size(); j++)
          for (auto &c : Fx_to_c[FFind(children[i][x][0])])
            constraint.flush_clause(
                SAT_solver, var_to_lit(get_Q(FFind(x)), false),
                var_to_lit(get_P(children[i][x][j], c), false),
                var_to_lit(
                    get_P(children[i][x][(j + 1) % children[i][x].size()], c),
                    true));
      }
  }

  std::cerr << "mutual exclusion constraints" << std::endl;
  /* mutual exclusion constraints */
  /* /!\ very dumb encoding of mutual exclusion, TODO3: find a better one /!\ */
  std::vector<std::vector<Tvar>> tmp(is_var.size()); /*<< vector indexed by variable, giving the list of propositional variables linked to a same var */
  for (Tvar p = 1; p < P_Q_to_x.size(); p++)
    if (P_Q_to_x[p].c)
      tmp[P_Q_to_x[p].x].push_back(p);
  for (auto &v : vars)
    if (floatable[FFind(v)])
      tmp[v].push_back(get_Q(FFind(v)));
    
  for (TCCFV_class c = 1; c < is_var.size(); c++)
    for (unsigned i = 0; i < tmp[c].size(); i++)
      for (unsigned j = i + 1; j < tmp[c].size(); j++)
      {  
        CCCFV_prop_constraint constraint;
        constraint.flush_mex(SAT_solver, tmp[c][i], tmp[c][j] /* to SAT solver */); 
      }

}

void
CCCFV_solver::Ground_or_Type_Constraints(void)
{
  ground[0].resize(is_var.size());
  ground[1].resize(is_var.size());

  for (auto &x : vars) {
    floatable[x] = true;
    ground[0][x] = 0;
    ground[1][x] = 0;
  }

  for (auto &lpos : Lpos)
    if (is_var[lpos.c[0]]) {
      std::cerr << lpos.c[0] << std::endl;
      TCCFV_class x = FFind(lpos.c[0]);
      for (unsigned i = 0; i < 2; i++) {
        if (child[i][x]) {
          std::cerr << "child " << i << ":" << child[i][x] << std::endl;
          TCCFV_class y = FFind(child[i][x]);
          if (class_type[y] != class_type[lpos.c[i+1]])
            floatable[y] = false;
          if (!is_var[lpos.c[i+1]])
            floatable[y] = false;
        }
        if (!is_var[lpos.c[i+1]]) {
          if (ground[i][x])
            floatable[x] = floatable[x] && (lpos.c[i+1] == ground[i][x]);
          else
            ground[i][x] = lpos.c[i+1];
        }
      }
    }
}

Tvar
CCCFV_solver::get_Q(TCCFV_class x)
{
  /* Q_x already exists */
  if (x_to_Q[x]) return x_to_Q[x];

  /* Q_x must be created */
  P_Q_to_x.emplace_back(x, 0);
  x_to_Q[x] = P_Q_to_x.size() -1;
  return (P_Q_to_x.size() - 1);
}

Tlit
CCCFV_solver::build_P(TCCFV_class u0, TCCFV_class u1)
{
  assert(! is_var[u1]);

  if (! is_var[u0])
    return (var_to_lit(0,u0 == u1));
 
  return (var_to_lit(get_P(u0, u1)));
}

Tvar
CCCFV_solver::get_P(TCCFV_class x, TCCFV_class c)
{
  assert(is_var[x]);
  assert(!is_var[c]);

  if (!x_c_to_P[c].empty())
    for (auto &x_p : x_c_to_P[c])
      /* P_{x,c} already exists */
      if (x_p.x == x) return x_p.p;

  /* P_{x,c} must be created */
  P_Q_to_x.emplace_back(x, c);
  Tvar p = P_Q_to_x.size() - 1;
  x_c_to_P[c].emplace_back(x,p);
  return p;
}

/**
   \author Pascal Fontaine
   \brief resets the CCFV solver, but preserves all buffered memory */
void
CCCFV_solver::reset(void)
{
  /* TODO3 */
}

/**
   \author Pascal Fontaine
   \brief resets the CCFV solver and reclaims memory */
void
CCCFV_solver::hard_reset(void)
{
  /* TODO3 */
}

/**
   \author Pascal Fontaine
   \brief ensures that all data structures are sufficiently large to store
     information about some class or variable
   \param c the TCCFV_class to accommodate */
void
CCCFV_solver::expand(TCCFV_class c)
{
  assert(is_var.size() == floatable.size());
  assert(is_var.size() == representative.size());
  assert(is_var.size() == x_to_Q.size());
  assert(is_var.size() == x_c_to_P.size());
  assert(is_var.size() == class_type.size());
  #ifdef DEBUG
  assert(is_var.size() == c_to_string.size());
  #endif

  if (c >= is_var.size())
  {
    is_var.resize(c + 1, false);
    floatable.resize(c + 1, false);
    representative.resize(c + 1, 0);
    x_to_Q.resize(c + 1, 0);
    x_c_to_P.resize(c + 1);
    class_type.resize(c + 1);
    #ifdef DEBUG
    c_to_string.resize(c+1, "");
    #endif
  }
}

void
CCCFV_solver::FCompute(void)
{
  std::vector<TCCFV_class> to_merge;
  representative.resize(is_var.size());
  child[0].resize(is_var.size());
  child[1].resize(is_var.size());

  for (auto &v : vars)
  {
    child[0][v] = 0;
    child[1][v] = 0;
    representative[v] = v;
  }
  for (auto &lpos : Lpos)
    if (is_var[lpos.c[0]])
      for (unsigned i = 0; i < 2; i++)
        if (is_var[lpos.c[i + 1]]) /* TODO fix +1 */
          {
            if (child[i][lpos.c[0]])
              {
                if (child[i][lpos.c[0]] != lpos.c[i + 1])
                  {
                    to_merge.push_back(child[i][lpos.c[0]]);
                    to_merge.push_back(lpos.c[i + 1]);
                  }
              }
            else
              child[i][lpos.c[0]] = lpos.c[i + 1];
          }

  while (!to_merge.empty())
  {
    assert (to_merge.size() % 2 == 0);

    TCCFV_class x2 = FFind(to_merge.back());
    to_merge.pop_back();
    TCCFV_class y2 = FFind(to_merge.back());
    to_merge.pop_back();

    if (x2 != y2)
    {
      for (unsigned i = 0; i < 2; i++)
      {
        if (child[i][y2])
        {
          if (child[i][x2])
          {
            to_merge.push_back(child[i][y2]);
            to_merge.push_back(child[i][x2]);
          }
        }
        else
          child[i][y2] = child[i][x2];
      }
      representative[x2] = y2;
    }
  }
}

TCCFV_class
CCCFV_solver::FFind(TCCFV_class x)
/* possibly more efficient with two passes on representative instead of using S */
{
   assert(is_var[x]);

   std::vector<TCCFV_class> S;

   while (representative[x] != x)
   {
      S.push_back(x);
      x = representative[x];
   }

   for (auto &y : S)
     representative[y] = x;
   
   return x;
}



/**
   \author Pascal Fontaine
   \brief gets the substitution
   \param var the variable for which we want the substituted class
   \return the substituted class */
Tclass_class &
CCCFV_solver::operator[](TCCFV_class var)
{
  return solution[var];
}

void
CCCFV_solver::Substitution_Reconstruction(void)
{
  solution.resize(0);
  solution.resize(is_var.size());
  for (auto &p: SAT_solver.model)
    if (P_Q_to_x[p].c)
      solution[P_Q_to_x[p].x].t1 = P_Q_to_x[p].c;
  
  for (auto &x : vars)
  {
    if (solution[x].t1 == 0)
    {
      std::cerr << "reconstructing solution for " << x << std::endl;
      std::cerr << "data: " << FFind(x) << ", " << child[0][x] << ", " << child[1][x] << ", " << ground[0][x]  << ", " << ground[1][x] << std::endl;
      if (FFind(x) != x)
      {
        std::cerr << "mapped to floater representative" << std::endl;
        solution[x].t1 = FFind(x);
      }
      else if (child[0][x] || ground[0][x])
      {
        std::cerr << "mapped to applied term" << std::endl;
        solution[x].t1 = ground[0][x]? ground[0][x] : child[0][x];
        solution[x].t2 = ground[1][x]? ground[1][x] : child[1][x];
      }

    }
  }
}

void
CCCFV_solver::Substitution_Normalize(void)
{
  normalized.resize(0);
  normalized.resize(is_var.size());

  for (auto &x: vars) normalize_var(x);
}

void
CCCFV_solver::normalize_var(TCCFV_class x)
{
  if (normalized[x]) return;
  if (solution[x].t1 == 0)
  {
     normalized[x] = true;
     solution[x] = x;
     return;
  }

  if (solution[x].t2 == 0 && is_var[solution[x].t1])
  {
    normalize_var(solution[x].t1);
    solution[x] = solution[solution[x].t1];
  }
  else if (solution[x].t2 != 0)
  {
    if (is_var[solution[x].t1]) 
    {
      normalize_var(solution[x].t1);
      if (solution[solution[x].t1].t2 == 0)
        solution[x].t1 = solution[solution[x].t1].t1;
    }
    if (is_var[solution[x].t2]) 
    {
      normalize_var(solution[x].t2);
      if (solution[solution[x].t2].t2 == 0)
        solution[x].t2 = solution[solution[x].t2].t1;
    }
  }
  normalized[x] = true;
}

#ifdef DEBUG
void
CCCFV_solver::debug_P_Q_to_x(void)
{
  for (unsigned i = 1; i < P_Q_to_x.size(); i++)
  {
    if (P_Q_to_x[i].c)
      std::cout << i << " : P_" << P_Q_to_x[i].x << "_" << P_Q_to_x[i].c << std::endl;
    else
      std::cout << i << " : Q_" << P_Q_to_x[i].x << std::endl;
  }
}

void 
CCCFV_solver::debug_symbol(TCCFV_class c, std::string s)
{
  if (c_to_string[c].size() && !c_to_string[c].compare(s))
    {
      std::cerr << "Two strings associated to class " << c << ": " << s << ", " << c_to_string[c] << std::endl;
      exit(-1);
    }
  c_to_string[c] = s;
}

void 
CCCFV_solver::debug_CNF(void)
{
  for (auto &c : SAT_solver.debug_CNF)
    {
      unsigned place = 0;
      if (c.size())
	for (auto &l : c)
	  std::cerr <<
	    (!(place++)?"":" ∨ ") <<
	    ((lit_pol(l))?"":"-") <<
	    ((P_Q_to_x[lit_var(l)].c)?"P{":"Q{") <<
	    c_to_string[P_Q_to_x[lit_var(l)].x] <<
	    ((P_Q_to_x[lit_var(l)].c)?", ":"") <<
	    ((P_Q_to_x[lit_var(l)].c)?c_to_string[P_Q_to_x[lit_var(l)].c]:"") << "}";
      else
	std::cerr << " ☐ ";
      std::cerr << std::endl;
    }
}

void 
CCCFV_solver::debug_solution(void)
{
  std::cerr << "Solution:" << std::endl;
  for (auto &v : vars)
    if (solution[v].t1)
      std::cerr << c_to_string[v] << ": " << c_to_string[solution[v].t1] <<
	" " << ((solution[v].t2)?c_to_string[solution[v].t2]:"") << std::endl;
    else
      std::cerr << c_to_string[v] << ": " << "floating" << std::endl;
}

void 
CCCFV_solver::debug_model(void)
{
  for (auto &v : SAT_solver.model)
    std::cerr << 
        ((P_Q_to_x[v].c)?"P{":"Q{") <<
	      c_to_string[P_Q_to_x[v].x] <<
	      ((P_Q_to_x[v].c)?", ":"") <<
	      ((P_Q_to_x[v].c)?c_to_string[P_Q_to_x[v].c]:"") << "}" << " ";
  std::cerr << std::endl;
}

#endif
