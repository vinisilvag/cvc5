(set-logic QF_UF)
(set-info :source | example.cc in ccfv-lib |)
; Example 2 from the paper
; E = {a = f a, g = f, g b = h c} and L = {x = y d}
(set-info :smt-lib-version 2.0)

(declare-sort U 0)
(declare-fun f (U) U)
(declare-fun g (U) U)
(declare-fun h (U) U)
(declare-const a U)
(declare-const b U)
(declare-const c U)
(declare-const d U)

; quantifiers not handled yet in the parser, thus:
; TODO: delete the two lines below once quantifiers handled
(declare-const x U)
(declare-fun y (U) U)

(check-ccfv
  (and
    (= a (f a))
    (= f g)
    (= (g b) (h c)))
  (= x (y d)))
; TODO replace above by below once quantification handled
; (exists ((x U) (y (-> U U))) (= x (y d))))

(exit)
