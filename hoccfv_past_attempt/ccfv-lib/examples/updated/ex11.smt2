(set-logic HO_UF)
(set-info :smt-lib-version 2.6)
(set-info :source |Example 11 from the paper: E = {f a = f' a', f = f''} and L = {u = v f'', u = v w, x = w z, x = f' z'}|)

(declare-sort U 0)

(declare-const f (-> U U))
(declare-const fp (-> U U))
(declare-const fpp (-> U U))

(declare-const a U)
(declare-const ap U)

; E = {f a = f' a', f = f''}
(assert (= (f a) (fp ap)))
(assert (= f fpp))

; L = {u = v f'', u = v w, x = w z, x = f' z'}
; Clausal form: ¬(L) = (¬(u = v f'') ∨ ¬(u = v w) ∨ ¬(x = w z) ∨ ¬(x = f' z'))
(assert (forall ((u U) (v (-> (-> U U) U)) (w (-> U U)) (x U) (z U) (zp U))
  (or (not (= u (v fpp)))
      (not (= u (v w)))
      (not (= x (w z)))
      (not (= x (fp zp))))))

(check-sat)
(exit)
