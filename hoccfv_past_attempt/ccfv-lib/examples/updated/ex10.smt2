(set-logic HO_UF)
(set-info :smt-lib-version 2.6)
(set-info :source |Example 10 from the paper: E = {a = f b, a = b, f != f', k = h f} and L = {x = y x, z = g y, z = v f, y != y'}|)

(declare-sort U 0)

(declare-const f (-> U U))
(declare-const fp (-> U U))
(declare-const g (-> (-> U U) U))
(declare-const h (-> (-> U U) U))

(declare-const a U)
(declare-const b U)
(declare-const k U)

; E = {a = f b, a = b, f != f', k = h f}
(assert (= a (f b)))
(assert (= a b))
(assert (not (= f fp)))
(assert (= k (h f)))

; L = {x = y x, z = g y, z = v f, y != y'}
; Clausal form: ¬(L) = (¬(x = y x) ∨ ¬(z = g y) ∨ ¬(z = v f) ∨ (y = y'))
(assert (forall ((x U) (y (-> U U)) (yp (-> U U)) (z U) (v (-> (-> U U) U)))
  (or (not (= x (y x)))
      (not (= z (g y)))
      (not (= z (v f)))
      (= y yp))))

(check-sat)
(exit)
