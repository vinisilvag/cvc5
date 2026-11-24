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

; L = {u = v f'', u = v w, w2 = w, x = w2 z, g = f', x = g z', y = w z'}
;   1. var = ground:               g = f'   -> substitute g |-> f'
;   2. var = var:                  w2 = w   -> substitute w2 |-> w
;   3. var = u (unused elsewhere): y = w z' -> remove literal (record y |-> w z')
; After eliminating:
; L = {u = v f'', u = v w, x = w z, x = f' z'}
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
