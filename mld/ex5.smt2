; ALIA
(set-logic ALL)

(declare-const a (Array Int Int))
(declare-const n Int)

(assert (forall ((i Int))
  (=> (and (>= i 0) (< i n))
      (> (select a i) 0))))

(assert (= n 5))
(assert (<= (select a 2) 0))

(check-sat) ; unsat
