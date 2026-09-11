(set-logic ALL)

(declare-fun f (Int) Int)
(declare-const x Int)
(declare-const y Int)

; f(x) - y <= 2
(assert (<= (- (f x) y) 2))

; y - f(x) <= -3
(assert (<= (- y (f x)) -3))

(check-sat) ; unsat
