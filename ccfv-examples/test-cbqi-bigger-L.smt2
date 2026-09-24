(set-logic UF)
(declare-sort U 0)

(declare-fun f (U) U)
(declare-fun g (U) U)

(declare-const a U)
(declare-const b U)

; Ground assertions (E):
(assert (= (f a) a))
(assert (= (g a) b))

(assert (forall ((x U)) (not (and (= (f x) a) (= (g x) b)))))

(check-sat)
