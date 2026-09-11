; LIA
(set-logic ALL)

(declare-const c Int)

(assert (forall ((x Int))
  (=> (> x 0) (> (+ x c) c))))

(assert (not (> (+ 1 c) c)))

(check-sat)
