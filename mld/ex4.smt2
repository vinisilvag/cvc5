; QF_UFNIRA
(set-logic ALL)

(declare-const x Int)
(declare-const y Int)
(declare-const r Real)
(declare-fun h (Real) Real)

(assert (= (* x x) y))
(assert (> (* r (to_real x)) 10.5))
(assert (< (h r) 0.0))

(check-sat) ; sat
