(set-logic UF)

(declare-sort U 0)
(declare-fun a () U)
(declare-fun b () U)
(declare-fun c () U)
(declare-fun f (U) U)
(declare-fun g (U) U)
(declare-fun h (U) U)

;; Ground context E
(assert (= (f a) (f b)))
(assert (= (h a) (h c)))
(assert (not (= (g b) (h c))))

;; Quantified formula containing L (negated inside CCFV search)
(assert (forall ((x1 U) (x2 U) (x3 U) (x4 U) (x5 U))
  (not (and (= (h x1) (h c))
            (not (= (h x2) (g x3)))
            (= (f x1) (f x3))
            (= x4 (g x5))))))

(check-sat)
