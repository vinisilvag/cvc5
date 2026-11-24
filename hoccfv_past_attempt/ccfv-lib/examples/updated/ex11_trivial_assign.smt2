(set-logic HO_UF)
(set-info :smt-lib-version 2.6)

(declare-sort U 0)

(declare-const f (-> U U))
(declare-const fp (-> U U))
(declare-const fpp (-> U U))

(declare-const a U)
(declare-const ap U)

; E = {f a = f' a', f = f''}
(assert (= (f a) (fp ap)))
(assert (= f fpp))

(assert (forall ((u U) (v (-> (-> U U) U)) (w (-> U U)) (w2 (-> U U)) (g (-> U U)) (x U) (y U) (z U) (zp U))
  (or (not (= u (v fpp)))
      (not (= u (v w)))
      (not (= w2 w))
      (not (= x (w2 z)))
      (not (= g fp))
      (not (= x (g zp)))
      (not (= y (w zp))))))

(check-sat)
(exit)
