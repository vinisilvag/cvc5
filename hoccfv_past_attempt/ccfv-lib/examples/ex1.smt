(set-logic QF_UF)
(set-info :source | example.cc in ccfv-lib |)
; Example 2 from the paper
; E = {a = f a, g = f, g b = h c} and L = {x = y d}
(set-info :smt-lib-version 2.0)
(declare-fun f (U) U)
(declare-fun g (U) U)
(declare-fun h (U) U)
(declare-const a U)
(declare-const b U)
(declare-const c U)
(declare-const d U)

(assert (= a (f a)))
(assert (= f g))
(assert (= (g b) (h c)))

(assert (forall ((x U) (y (-> U U))) (not (= x (y d)))))

(check-sat)
(exit)
