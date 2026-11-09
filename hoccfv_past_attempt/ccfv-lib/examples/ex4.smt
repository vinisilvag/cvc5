(set-logic QF_UF)
(set-info :source | example.cc in ccfv-lib |)
; Simple example to force two children variables to be equal
; E = {c = h b, d = h a} and L = {f y2 = f a, x = g y1, x = z y2, w = h y1}
(set-info :smt-lib-version 2.0)
(declare-fun f (U) U)
(declare-fun g (U) U)
(declare-fun h (U) U)
(declare-const a U)
(declare-const b U)
(declare-const c U)
(declare-const d U)

(assert (= c (h b)))
(assert (= d (h a)))

(assert (forall ((w U) (x U) (y1 U) (y2 U) (z (-> U U))) (or (not (= (f y2) (f a))) (not (= x (g y1))) (not (= x (z y2))) (not (= h y1)) )))

(check-sat)
(exit)