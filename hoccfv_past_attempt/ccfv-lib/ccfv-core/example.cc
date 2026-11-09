#include <iostream>

#include "ccfv-core.hh"
#include "SAT.hh"

int
example_2_paper(void)
{
/*
  Example 2 from the paper
  E = {a = f a, g = f, g b = h c} and L = {x = y d} */

/*
  Congruence classes of terms in E, one per line
  1: a, f a
  2: g, f
  3: g b, h c
  4: c
  5: b
  6: d
  7: h

  10: x
  11: y

  We work in an untyped setting, i.e., all types are 0 */
  CCCFV_solver CCFV_solver;

  CCFV_solver.class_is_var(10);
  CCFV_solver.class_is_var(11);
  CCFV_solver.class_sig(1, 2, 1);
  CCFV_solver.class_sig(3, 2, 5);
  CCFV_solver.class_sig(3, 7, 4);

  CCFV_solver.constraint_pos(10, 11, 6);

  CCFV_solver.debug_symbol(1, "a");
  CCFV_solver.debug_symbol(2, "g");
  CCFV_solver.debug_symbol(3, "g b");
  CCFV_solver.debug_symbol(4, "c");
  CCFV_solver.debug_symbol(5, "b");
  CCFV_solver.debug_symbol(6, "d");
  CCFV_solver.debug_symbol(7, "h");

  CCFV_solver.debug_symbol(10, "x");
  CCFV_solver.debug_symbol(11, "y");

  if (CCFV_solver.solve())
    {
      std::cout << "CCFV problem has a solution" << std::endl;
    }
  else
    {
      std::cout << "CCFV problem has no solution" << std::endl;
    }

  return 0;
}


int
example_2(void)
{
/*
  Simple example without floaters or cycles, not in the paper
  E = {b = f a, g b = h c} and L = {x = y a} */

/*
  Congruence classes of terms in E, one per line
  1: b, f a
  2: f
  3: g
  4: g b, h c
  5: a
  6: c
  7: h

  10: x
  11: y

  We worked in an untyped setting, i.e., all types are 0 */
  CCCFV_solver CCFV_solver;

  CCFV_solver.class_is_var(10);
  CCFV_solver.class_is_var(11);
  CCFV_solver.class_sig(1, 2, 5);
  CCFV_solver.class_sig(4, 3, 1);
  CCFV_solver.class_sig(4, 7, 6);

  CCFV_solver.constraint_pos(10, 11, 5);

  CCFV_solver.debug_symbol(1, "b");
  CCFV_solver.debug_symbol(2, "f");
  CCFV_solver.debug_symbol(3, "g");
  CCFV_solver.debug_symbol(4, "g b");
  CCFV_solver.debug_symbol(5, "a");
  CCFV_solver.debug_symbol(6, "c");
  CCFV_solver.debug_symbol(7, "h");

  CCFV_solver.debug_symbol(10, "x");
  CCFV_solver.debug_symbol(11, "y");

  if (CCFV_solver.solve())
    {
      std::cout << "CCFV problem has a solution" << std::endl;
    }
  else
    {
      std::cout << "CCFV problem has no solution" << std::endl;
    }

  return 0;
}

int
example_3(void)
{
/*
  Simple example without floaters or cycles, but with disequalities not in the paper
  E = {b = f a, g b = h c, a != b} and L = {x = y a, x!=z} */

/*
  Congruence classes of terms in E, one per line
  1: b, f a
  2: f
  3: g
  4: g b, h c
  5: a
  6: c
  7: h
  5 != 1

  10: x
  11: y
  12: z

  We worked in an untyped setting, i.e., all types are 0 */
  CCCFV_solver CCFV_solver;

  CCFV_solver.class_is_var(10);
  CCFV_solver.class_is_var(11);
  CCFV_solver.class_is_var(12);

  CCFV_solver.class_sig(1, 2, 5);
  CCFV_solver.class_sig(4, 3, 1);
  CCFV_solver.class_sig(4, 7, 6);
  CCFV_solver.class_neq(5, 1);

  CCFV_solver.constraint_pos(10, 11, 5);
  CCFV_solver.constraint_neg(10,12);

  CCFV_solver.debug_symbol(1, "b");
  CCFV_solver.debug_symbol(2, "f");
  CCFV_solver.debug_symbol(3, "g");
  CCFV_solver.debug_symbol(4, "g b");
  CCFV_solver.debug_symbol(5, "a");
  CCFV_solver.debug_symbol(6, "c");
  CCFV_solver.debug_symbol(7, "h");

  CCFV_solver.debug_symbol(10, "x");
  CCFV_solver.debug_symbol(11, "y");
  CCFV_solver.debug_symbol(12, "z");

 if (CCFV_solver.solve())
    {
      std::cout << "CCFV problem has a solution" << std::endl;
    }
  else
    {
      std::cout << "CCFV problem has no solution" << std::endl;
    }

  return 0;
}

int
example_4(void)
{
/*
  Simple example to force two children variables to be equal
  E = {c = h b, d = h a} and L = {f y2 = f a, x = g y1, x = z y2, w = h y1} */

/*7
  Congruence classes of terms in E, one per line
  1: a
  2: b
  3: c, h b
  4: d, h a
  5: h
  6: f
  7: f a
  8: g

  10: x
  11: y1
  12: y2
  13: z
  14: w

  We worked in an untyped setting, i.e., all types are 0 */
  CCCFV_solver CCFV_solver;
  CCFV_solver.class_is_var(10);
  CCFV_solver.class_is_var(11);
  CCFV_solver.class_is_var(12);
  CCFV_solver.class_is_var(13);
  CCFV_solver.class_is_var(14);

  CCFV_solver.class_sig(3, 5, 2);
  CCFV_solver.class_sig(4, 5, 1);
  CCFV_solver.class_sig(7, 6, 1);

  CCFV_solver.constraint_pos(7, 6, 12);
  CCFV_solver.constraint_pos(10, 8, 11);
  CCFV_solver.constraint_pos(10, 13, 12);
  CCFV_solver.constraint_pos(14, 5, 11);

  CCFV_solver.debug_symbol(1, "a");
  CCFV_solver.debug_symbol(2, "b");
  CCFV_solver.debug_symbol(3, "c");
  CCFV_solver.debug_symbol(4, "d");
  CCFV_solver.debug_symbol(5, "h");
  CCFV_solver.debug_symbol(6, "f");
  CCFV_solver.debug_symbol(7, "f a");
  CCFV_solver.debug_symbol(8, "g");

  CCFV_solver.debug_symbol(10, "x");
  CCFV_solver.debug_symbol(11, "y1");
  CCFV_solver.debug_symbol(12, "y2");
  CCFV_solver.debug_symbol(13, "z");
  CCFV_solver.debug_symbol(14, "w");

  if (CCFV_solver.solve())
    {
      std::cout << "CCFV problem has a solution" << std::endl;
    }
  else
    {
      std::cout << "CCFV problem has no solution" << std::endl;
    }

  return 0;
}


int
example_5(void)
{
/*
  Super Simple example to test solution reconstruction
  E = {} and L = {x = f y} */

/*
  Congruence classes
  f: 1

  10: x
  11: y

  We worked in an untyped setting, i.e., all types are 0 */
  CCCFV_solver CCFV_solver;
  CCFV_solver.class_is_var(10);
  CCFV_solver.class_is_var(11);

  CCFV_solver.constraint_pos(10, 1, 11);

  CCFV_solver.debug_symbol(1, "f");

  CCFV_solver.debug_symbol(10, "x");
  CCFV_solver.debug_symbol(11, "y");

  if (CCFV_solver.solve())
    {
      CCFV_solver.debug_CNF();
      std::cout << "CCFV problem has a solution" << std::endl;
      CCFV_solver.debug_solution();
    }
  else
    {
      std::cout << "CCFV problem has no solution" << std::endl;
    }

  return 0;
}

int
example_1_paper(void)
{

/* example 1 from the paper (FOL)
E = {f a = f b, h a = h c, g b != h c}
L = {h x1 = h c, h x2 != g x3, f x1 = f x3, x4 = g x5}
*/

/* preprocessed version:
E = {f a = f b, h a = h c, g b != h c}
L = { h c = h x1, x6 = h x2, x8 = g x3, x6 != x8, x7 = f x1, x7 = f x3, x4 = g x5}
*/

/*
  Congruence classes
  1: a
  2: b
  3: f
  4: f a, f b
  5: h
  6: c
  7: h a, h c
  8: g b
  9: g

  8 != 7

  11: x1
  12: x2
  13: x3
  14: x4
  15: x5
  16: x6
  17: x7
  18: x8

  7 = 5 11 (h c = h x1)

  16 = 5 12 (x6 = h x2)
  18 = 9 13 (x8 = g x3)

  16 != 18 (x6 != x8)

  17 = 3 11 (x7 = f x1)
  17 = 3 13 (x7 = f x1)

  14 = 9 15 (x4 = g x5)
*/

  CCCFV_solver CCFV_solver;

  CCFV_solver.class_is_var(11);
  CCFV_solver.class_is_var(12);
  CCFV_solver.class_is_var(13);
  CCFV_solver.class_is_var(14);
  CCFV_solver.class_is_var(15);
  CCFV_solver.class_is_var(16);
  CCFV_solver.class_is_var(17);
  CCFV_solver.class_is_var(18);

  CCFV_solver.class_sig(4, 3, 1);
  CCFV_solver.class_sig(4, 3, 2);
  CCFV_solver.class_sig(7, 5, 1);
  CCFV_solver.class_sig(7, 5, 6);
  CCFV_solver.class_sig(8, 9, 2);

  CCFV_solver.class_neq(8, 7);

  CCFV_solver.constraint_pos(7, 5, 11);

  CCFV_solver.constraint_pos(16, 5, 12);
  CCFV_solver.constraint_pos(18, 9, 13);

  CCFV_solver.constraint_neg(16, 18);

  CCFV_solver.constraint_pos(17, 3, 11);
  CCFV_solver.constraint_pos(17, 3, 13);

  CCFV_solver.constraint_pos(14, 9, 15);

  CCFV_solver.debug_symbol(1, "a");
  CCFV_solver.debug_symbol(2, "b");
  CCFV_solver.debug_symbol(3, "f");
  CCFV_solver.debug_symbol(4, "f a");
  CCFV_solver.debug_symbol(5, "h");
  CCFV_solver.debug_symbol(6, "c");
  CCFV_solver.debug_symbol(7, "h a");
  CCFV_solver.debug_symbol(8, "g b");
  CCFV_solver.debug_symbol(9, "g");
  CCFV_solver.debug_symbol(11, "x1");
  CCFV_solver.debug_symbol(12, "x2");
  CCFV_solver.debug_symbol(13, "x3");
  CCFV_solver.debug_symbol(14, "x4");
  CCFV_solver.debug_symbol(15, "x5");
  CCFV_solver.debug_symbol(16, "x6");
  CCFV_solver.debug_symbol(17, "x7");
  CCFV_solver.debug_symbol(18, "x8");

  bool status = true;
  while (status) {
    std::cout << "searching for a solution" << std::endl;
    status = CCFV_solver.solve();
  }
  std::cout
      << "The CCFV problem has no (more) solution, or the SAT solver returned"
         " UNSOLVED (e.g., due to a timeout)"
      << std::endl;

  return 0;
}

int
example_3_paper(void)
{
  /*
 {f ≃ g, f a ≃ b, h c ≃ b} and L = {f (x a) ≃ g b}
 */

  return 0;
}

int
example_4_6_paper(void)
{
  /*
  E_4 {(f a) b ≃ (f b) a, g b ≃ g c, h1 ≃ h2 , g ≃ f a, h1 ̸≃ h3 }
  L_6 {z1 ̸≃ b, z1 ≃ h2 x, x ≃ g y, a ≃ h2 z2 , z2 ≃ z3 b, z3 ≃
f x, g c ≃ z3 y} */
  return 0;
}

int
example_10_paper(void)
{
/*E = {a ≃ f b, a ≃ b, f ̸≃ f ′ , k ≃ h f } and L = {x ≃ y x, z ≃
g y, z ≃ v f, y ̸≃ y ′ } */
  return 0;
}

int
example_11_paper(void)
{
  /* {f a ≃ f ′ a′ , f ≃ f ′′ } and L = {u ≃ v f ′′ , u ≃ v w, x ≃
w z, x ≃ f ′ z ′ } */
  return 0;
}

int
main(void)
{
  return example_1_paper();
}
