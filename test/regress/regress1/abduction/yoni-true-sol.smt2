; COMMAND-LINE: --produce-abducts
; EXPECT: (define-fun A () Bool (>= j i))
(set-logic QF_LIA)
(set-option :produce-abducts true)
(declare-fun n () Int)
(declare-fun m () Int)
(declare-fun i () Int)
(declare-fun j () Int)
(assert (and (>= n 0) (>= m 0)))
(assert (< n i))
(assert (< (+ i j) m))
(get-abduct A
  (<= n m)
  ((GA Bool) (GJ Int) (GI Int))
  (
  (GA Bool ((>= GJ GI)))
  (GJ Int ( j))
  (GI Int ( i))
  )
)