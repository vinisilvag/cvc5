(set-logic QF_ALL)

(declare-const x Real)
(declare-const y Real)
(declare-const z Real)

; y - x <= 2.5
(assert (<= (- y x) 2.5))

; z - y <= 1.25
(assert (<= (- z y) 1.25))

; z - x >= 3.0
(assert (>= (- z x) 3.0))

(assert (>= x 0.0))

(check-sat) ; sat
