/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Generation of the triggers used by CCFV for trigger-based instantiation,
 * following cvc5's InstStrategyUserPatterns and InstStrategyAutoGenTriggers.
 */

#include "theory/quantifiers/ccfv/ccfv_trigger_generator.h"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include "base/output.h"
#include "theory/quantifiers/ccfv/ccfv_utils.h"
#include "theory/quantifiers/ematching/pattern_term_selector.h"
#include "theory/quantifiers/ematching/trigger_database.h"
#include "theory/quantifiers/quantifiers_registry.h"
#include "theory/quantifiers/quantifiers_state.h"
#include "theory/quantifiers/term_util.h"
#include "theory/valuation.h"
#include "util/random.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

namespace {

/**
 * Same as Trigger::ensureGroundTermPreprocessed: replaces the ground subterms
 * of n (in instantiation constant form) by their preprocessed form, which are
 * also added to gts.
 */
Node ensureGroundTermPreprocessed(Valuation& val,
                                  Node n,
                                  std::vector<Node>& gts)
{
  NodeManager* nm = n.getNodeManager();
  std::unordered_map<TNode, Node> visited;
  std::unordered_map<TNode, Node>::iterator it;
  std::vector<TNode> visit;
  visit.push_back(n);

  TNode cur;

  do
  {
    cur = visit.back();
    visit.pop_back();
    it = visited.find(cur);
    if (it == visited.end())
    {
      if (cur.getNumChildren() == 0 || cur.getKind() == Kind::BOUND_VAR_LIST)
      {
        visited[cur] = cur;
      }
      else if (!TermUtil::hasInstConstAttr(cur))
      {
        // cur has no INST_CONSTANT, thus is ground
        Node vcur = val.getPreprocessedTerm(cur);
        gts.push_back(vcur);
        visited[cur] = vcur;
      }
      else
      {
        visited[cur] = Node::null();
        visit.push_back(cur);
        visit.insert(visit.end(), cur.begin(), cur.end());
      }
    }
    else if (it->second.isNull())
    {
      Node ret = cur;
      bool childChanged = false;
      std::vector<Node> children;
      if (cur.getMetaKind() == kind::metakind::PARAMETERIZED)
      {
        children.push_back(cur.getOperator());
      }
      for (const Node& cn : cur)
      {
        it = visited.find(cn);
        childChanged = childChanged || cn != it->second;
        children.push_back(it->second);
      }
      if (childChanged)
      {
        ret = nm->mkNode(cur.getKind(), children);
      }
      visited[cur] = ret;
    }
  } while (!visit.empty());

  return visited[n];
}

/** Same as InstStrategy::getInstUserPatMode. */
options::UserPatMode userPatMode(const Options& opts, uint64_t instRounds)
{
  options::UserPatMode mode = opts.quantifiers.userPatternsQuant;
  if (mode == options::UserPatMode::INTERLEAVE)
  {
    return instRounds % 2 == 0 ? options::UserPatMode::USE
                               : options::UserPatMode::RESORT;
  }
  return mode;
}

}  // namespace

CcfvTriggerGenerator::CcfvTriggerGenerator(Env& env,
                                           QuantifiersState& qs,
                                           QuantifiersRegistry& qr)
    : EnvObj(env), d_qstate(qs), d_qreg(qr)
{
}

options::UserPatMode CcfvTriggerGenerator::getUserPatMode() const
{
  return userPatMode(options(), d_qstate.getInstRounds());
}

CcfvQuantTriggers& CcfvTriggerGenerator::getQuantTriggers(Node q)
{
  auto it = d_qTriggers.find(q);
  if (it != d_qTriggers.end())
  {
    return it->second;
  }

  CcfvQuantTriggers& qt = d_qTriggers[q];
  if (q.getNumChildren() == 3)
  {
    bool useUser =
        options().quantifiers.userPatternsQuant != options::UserPatMode::IGNORE;
    Node subsPat = d_qreg.substituteBoundVariablesToInstConstants(q[2], q);
    for (const Node& p : subsPat)
    {
      if (p.getKind() == Kind::INST_PATTERN)
      {
        qt.d_hasUserPatterns = true;
        if (!useUser)
        {
          continue;
        }
        std::vector<Node> nodes;
        for (const Node& pc : p)
        {
          if (std::find(nodes.begin(), nodes.end(), pc) == nodes.end())
          {
            nodes.push_back(pc);
          }
        }
        CcfvTriggerEntry tr;
        if (makeTrigger(q, nodes, tr))
        {
          qt.d_user.push_back(std::move(tr));
        }
      }
      else if (p.getKind() == Kind::INST_NO_PATTERN)
      {
        if (std::find(qt.d_noPatterns.begin(), qt.d_noPatterns.end(), p[0])
            == qt.d_noPatterns.end())
        {
          qt.d_noPatterns.push_back(p[0]);
        }
      }
    }
  }

  Trace("ccfv-debug") << "[triggers] user patterns of " << q << ": "
                      << (qt.d_hasUserPatterns ? "yes" : "no") << ", "
                      << qt.d_user.size() << " usable trigger(s), "
                      << qt.d_noPatterns.size() << " no-pattern(s)"
                      << std::endl;
  return qt;
}

bool CcfvTriggerGenerator::makeTrigger(Node q,
                                       const std::vector<Node>& icPats,
                                       CcfvTriggerEntry& tr)
{
  Valuation& val = d_qstate.getValuation();
  for (const Node& n : icPats)
  {
    Node np = ensureGroundTermPreprocessed(val, n, tr.d_groundTerms);
    Node pat = d_qreg.substituteInstConstantsToBoundVariables(np, q);
    if (!ccfv::isSupportedTriggerTerm(pat))
    {
      Trace("ccfv-debug") << "[triggers] unsupported trigger term: " << pat
                          << std::endl;
      return false;
    }
    tr.d_pats.push_back(pat);
  }
  return true;
}

bool CcfvTriggerGenerator::generatePatternTerms(Node q, CcfvQuantTriggers& qt)
{
  if (qt.d_patternsGenerated)
  {
    return true;
  }
  qt.d_patternsGenerated = true;

  std::vector<Node> patterns;
  std::map<Node, inst::TriggerTermInfo> tinfo;
  inst::PatternTermSelector pts(options(),
                                q,
                                options().quantifiers.triggerSelMode,
                                qt.d_noPatterns,
                                true);
  pts.collect(d_qreg.getInstConstantBody(q), patterns, tinfo);

  std::unordered_set<Node> covered;
  for (const Node& p : patterns)
  {
    covered.insert(tinfo[p].d_fv.begin(), tinfo[p].d_fv.end());
  }
  qt.d_numTriggerVars = covered.size();
  size_t nvars = q[0].getNumChildren();
  // Partial triggers are not supported: as cvc5 without them, no trigger is
  // generated if the patterns do not cover all variables
  if (qt.d_numTriggerVars > 0 && qt.d_numTriggerVars < nvars)
  {
    Trace("ccfv") << "[triggers] no triggers for " << q
                  << ": the patterns do not contain all its variables"
                  << std::endl;
    return false;
  }
  for (const Node& p : patterns)
  {
    qt.d_pool[tinfo[p].d_fv.size() == nvars ? 0 : 1].push_back(p);
  }
  Trace("ccfv-debug") << "[triggers] pattern pools of " << q << ": "
                      << qt.d_pool[0].size() << " single, "
                      << qt.d_pool[1].size() << " multi" << std::endl;
  return true;
}

void CcfvTriggerGenerator::addAutoTrigger(Node q,
                                          CcfvQuantTriggers& qt,
                                          size_t r,
                                          const std::vector<Node>& icPats)
{
  std::vector<Node> key = icPats;
  std::sort(key.begin(), key.end());
  for (const CcfvTriggerEntry& t : qt.d_auto[r])
  {
    if (t.d_key == key)
    {
      Trace("ccfv-debug") << "[triggers] trigger already exists for " << q
                          << std::endl;
      return;
    }
  }
  CcfvTriggerEntry tr;
  tr.d_key = std::move(key);
  if (makeTrigger(q, icPats, tr))
  {
    qt.d_auto[r].push_back(std::move(tr));
    Trace("ccfv") << "[triggers] new " << (r == 0 ? "single" : "multi")
                  << " trigger for " << q << ": " << qt.d_auto[r].back().d_pats
                  << std::endl;
  }
}

void CcfvTriggerGenerator::generateTriggers(Node q, CcfvQuantTriggers& qt)
{
  if (!generatePatternTerms(q, qt))
  {
    return;
  }
  // making triggers again may generate new ones, e.g. for multi-triggers
  // where the selection is nondeterministic
  bool alreadyMadeTriggers = qt.d_madeTriggers;
  qt.d_madeTriggers = true;

  // Generating single triggers is deterministic, so it is only done the
  // first time
  if (!alreadyMadeTriggers && !qt.d_pool[0].empty())
  {
    for (const Node& p : qt.d_pool[0])
    {
      addAutoTrigger(q, qt, 0, {p});
    }
    if (!options().quantifiers.multiTriggerWhenSingle)
    {
      return;
    }
  }

  // now consider multi-triggers
  std::vector<Node>& multi = qt.d_pool[1];
  if (multi.empty())
  {
    return;
  }
  if (alreadyMadeTriggers)
  {
    Trace("ccfv-debug") << "[triggers] regenerating the multi-trigger of " << q
                        << std::endl;
    // shuffle randomly if we have already made a multi-trigger
    std::shuffle(multi.begin(), multi.end(), Random::getRandom());
  }
  std::vector<Node> trNodes;
  if (inst::TriggerDatabase::mkTriggerTerms(
          q, multi, qt.d_numTriggerVars, trNodes))
  {
    // we only add a single multi-trigger
    addAutoTrigger(q, qt, 1, trNodes);
  }
}

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
