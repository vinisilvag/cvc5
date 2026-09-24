(set-logic UF)
(declare-sort U 0)

(declare-fun f (U) U)
(declare-fun g (U) U)
(declare-fun h (U) U)

(declare-const a U)
(declare-const b U)
(declare-const c U)

(assert (= (f a) (g b)))
(assert (= (h a) b))
(assert (= (f a) (f c)))

(assert (forall ((x U)) (not (= (f x) (g (h x))))))

(check-sat)
