(set-logic ALL)

(declare-const x Int)
(declare-const y Int)
(declare-const z Int)

(assert (= (* x y) 0))
(assert (= (div x y) 1))
(assert (= (abs (- x)) x))

(check-sat)
