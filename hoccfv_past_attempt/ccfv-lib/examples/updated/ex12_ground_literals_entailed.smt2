;; ./build/bin/cvc5 --ccfv=sat --simplification=none --miniscope-quant=off -t ccfv-debug ex12_ground_literals_entailed.smt2
;; (--miniscope-quant=off keeps the ground literal inside the forall; otherwise
;; cvc5 moves it out before CCFV sees it)
;; Expected: normalize turns the entailed ground literal into true, which is removed; L = {c = f x} gives x |-> d, so unsat
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

; L = {f a = b, f x = c}
;   f a = b is ground and entailed by E -> replaced by T and removed.
; After removing ground literals:
; L = {f x = c}  (solution: x |-> d)
(assert (forall ((x U))
  (or (not (= (f a) b))
      (not (= (f x) c)))))

(check-sat)
(exit)
