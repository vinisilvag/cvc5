; EXPECT: unsat
(set-option :incremental false)
(set-option :sets-exp true)
(set-logic ALL)
(declare-sort Atom 0)
(declare-fun a () (Relation Atom))
(declare-fun b () (Relation Atom Atom))
(declare-fun x () Atom)
(declare-fun y () Atom)
(assert (not (= x y)))
(assert (set.member (tuple y) a))
(assert (set.member (tuple x y) b))
(assert (= (as set.universe (Relation Atom Atom)) (rel.product (as set.universe (Relation Atom)) (as set.universe (Relation Atom)))))
(declare-fun u () (Relation Atom Atom))
(assert (= u (as set.universe (Relation Atom Atom))))
(assert (not (set.member (tuple x y) u)))
(check-sat)
