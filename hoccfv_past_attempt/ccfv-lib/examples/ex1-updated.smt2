(set-logic HO_UF)
(set-info :smt-lib-version 2.6)
(set-info :source |Example 2 from the paper: E = {a = f a, g = f, g b = h c} and L = {x = y d}|)

(declare-sort U 0)

(declare-const f (-> U U))
(declare-const g (-> U U))
(declare-const h (-> U U))

(declare-const a U)
(declare-const b U)
(declare-const c U)
(declare-const d U)

(assert (= a (f a)))
(assert (= f g))
(assert (= (g b) (h c)))

(assert (forall ((x U) (y (-> U U)))
  (not (= x (y d)))))

(check-sat)
(exit)
