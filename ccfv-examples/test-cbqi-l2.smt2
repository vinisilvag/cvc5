(set-logic UF)
(declare-sort U 0)

(declare-fun f (U) U)
(declare-fun g (U) U)

(declare-const a U)
(declare-const b U)
(declare-const c U)

; Ground assertions (E):
(assert (= (f a) b))
(assert (not (= (g a) c)))

; Quantified assertion: forall x. (f(x) = b => g(x) = c)
; In clausal form, this is: (or (not (= (f x) b)) (= (g x) c))
(assert (forall ((x U)) (=> (= (f x) b) (= (g x) c))))

(check-sat)
