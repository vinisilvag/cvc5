; QF_AUFBV
(set-logic ALL)

(declare-fun f ((_ BitVec 32)) (_ BitVec 32))
(declare-const mem (Array (_ BitVec 32) (_ BitVec 32)))
(declare-const addr (_ BitVec 32))
(declare-const val (_ BitVec 32))

(assert (= (select (store mem addr val) addr) (f val)))
(assert (bvugt val (_ bv100 32)))
(assert (distinct (f val) val))

(check-sat) ; sat
