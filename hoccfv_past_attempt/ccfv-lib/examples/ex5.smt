(set-logic QF_UF)
(set-info :source | example.cc in ccfv-lib |)
; Super Simple example to test solution reconstruction
; E = {} and L = {x = f y}
(set-info :smt-lib-version 2.0)
(declare-fun f (U) U)

(assert (forall ((x U) (y U)) (not (= x (f y)))))

(check-sat)
(exit)