;; (--miniscope-quant=off keeps the ground literal inside the forall; otherwise
;; cvc5 moves it out before CCFV sees it)
(set-logic HO_UF)
(set-info :smt-lib-version 2.6)

(declare-sort U 0)

(declare-const f (-> U U))

(declare-const a U)
(declare-const b U)
(declare-const c U)
(declare-const d U)

; E = {f a = b, f d = c}
(assert (= (f a) b))
(assert (= (f d) c))

; L = {f a = c, f x = c}
;   f a = c is ground and NOT entailed by E (b and c are in different
;   classes) -> replaced by F, so L = {F} and there is no solution.
(assert (forall ((x U))
  (or (not (= (f a) c))
      (not (= (f x) c)))))

(check-sat)
(exit)
