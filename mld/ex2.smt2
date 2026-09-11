; QF_SAT
(declare-const p Bool)
(declare-const q Bool)
(declare-const r Bool)

(assert (or p q))
(assert (=> p r))
(assert (and (not r) (not q)))

(check-sat) ; unsat
