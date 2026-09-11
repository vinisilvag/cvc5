(set-logic QF_ALL)

(declare-datatypes ((StringTree 0)) (
  ((Node (left StringTree) (val String) (right StringTree))
    (Leaf))
))

(declare-const t StringTree)
(declare-const s1 String)
(declare-const s2 String)

(assert ((_ is Node) t))
(assert (= (val t) (str.++ s1 s2)))
(assert (= (str.len s1) 4))
(assert (> (str.len s2) 0))

(check-sat) ; sat
