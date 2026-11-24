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

(assert (forall ((u U) (v (-> (-> U U) U)) (w (-> U U)) (x U) (z U) (zp U))
  (or (not (= u (v fpp)))
      (not (= u (v f)))
      (not (= u (v w)))
      (not (= x (w z)))
      (not (= x (fp zp)))
      (not (= (f a) (w z)))
      (not (= (fp ap) (fp zp))))))

(check-sat)
(exit)
