(set-logic HO_UF)
(set-info :smt-lib-version 2.6)
(set-info :source | example.cc in ccfv-lib |)
; Super Simple example to test solution reconstruction
; E = {} and L = {x = f y}

(declare-sort U 0)

(declare-const f (-> U U))

(assert (forall ((x U) (y U)) (not (= x (f y)))))

(check-sat)
(exit)
