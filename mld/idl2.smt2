(set-logic ALL)
(declare-const x Int)
(declare-const y Int)

; Before rewrite: 2x - (x + y) <= 3
; After rewrite: x - y <= 3
(assert (<= (- (* 2 x) (+ x y)) 3))

(assert (> (- x y) 5))

(check-sat) ; unsat
