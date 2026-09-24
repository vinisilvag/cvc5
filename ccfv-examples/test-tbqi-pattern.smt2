(set-logic UF)
(declare-sort U 0)

(declare-fun f (U) U)
(declare-fun g (U) U)
(declare-fun h (U) U)

(declare-const a U)
(declare-const b U)
(declare-const c U)

; Ground assertions (E)
(assert (= (f a) b))
(assert (= (g b) c))

(assert (forall ((x U))
  (! (= (g (f x)) c)
     :pattern ((f x))
  )
))

(assert (forall ((x U) (y U))
  (! (= (h x) y)
     :pattern ((f x) (g y))
  )
))

(assert (forall ((x U))
  (! (= (f x) (h x))
     :pattern ((f x))
     :pattern ((h x))
  )
))

(check-sat)
