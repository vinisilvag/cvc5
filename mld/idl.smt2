(set-logic QF_ALL) 

(declare-const t1 Int)
(declare-const t2 Int)
(declare-const t3 Int)
(declare-const t4 Int)

; t2 - t1 >= 3  <=>  t1 - t2 <= -3
(assert (>= (- t2 t1) 3))

; t3 - t2 >= 2  <=>  t2 - t3 <= -2
(assert (>= (- t3 t2) 2))

; t4 - t3 >= 4  <=>  t3 - t4 <= -4
(assert (>= (- t4 t3) 4))

(assert (<= (- t4 t1) 8))

(check-sat) ; unsat
