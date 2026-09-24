(set-logic UF)
(declare-sort U 0)

(declare-fun f (U) U)
(declare-fun g (U) U)

(declare-const a U)
(declare-const b U)

; Ground assertions
(assert (= (f a) a))
(assert (= (g a) b))

; Quantifier with an AND in the body
(assert (forall ((x U)) (and (= (f x) a) (= (g x) b))))

(check-sat)
