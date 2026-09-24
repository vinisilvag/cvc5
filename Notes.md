- cvc5 tem procedimentos diferentes (dependentes da técnica de instanciação usada)
  para encontrar as substituições depois de ter E e L em mãos:
  - Para Trigger-based / E-matching: Usa a engine clássica de casamentos com índices
    de termos e trie de instanciações (InstEngine, Trigger, etc.)
  - Para Conflict-based (CBQI): O cvc5 usa historicamente a heurística do Reynolds
    (FMCAD 2014, QuantConflictFind), que transforma sucessivamente a fórmula em
    restrições de matching. Embora muito rápida na prática, não é um procedimento de
    decisão completo para o problema de E-ground (dis)unification geral.
  - MBQI está fora do escopo por enquanto

- Queremos comparar CCFV vs CCFV SAT-encoded vs padrão do cvc5
- E depois comparar em mais detalhes CCFV e CCFV SAT-encoded
