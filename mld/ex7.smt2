; QF_UF
(set-logic ALL)

(declare-sort T 0)
(declare-const a T)
(declare-const b T)
(declare-const c T)

(assert (= a b))
(assert (= b c))
(assert (distinct a c))

(check-sat) ; unsat
