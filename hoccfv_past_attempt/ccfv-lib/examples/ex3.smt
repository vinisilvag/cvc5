(set-logic QF_UF)
(set-info :source | example.cc in ccfv-lib |)
; Simple example without floaters or cycles, but with disequalities not in the paper
; E = {b = f a, g b = h c, a != b} and L = {x = y a, x!=z}
(set-info :smt-lib-version 2.0)
(declare-fun f (U) U)
(declare-fun g (U) U)
(declare-fun h (U) U)
(declare-const a U)
(declare-const b U)
(declare-const c U)

(assert (= b (f a)))
(assert (= (g b) (h c)))
(assert (not (= a b)))

(assert (forall ((x U) (y (-> U U)) (z U)) (or (not (= x (y a))) (= x z)))

(check-sat)
(exit)