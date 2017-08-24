// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <Scenario/Process/ScenarioModel.hpp>

#include <boost/container/flat_set.hpp>
#include <iscore/tools/std/Optional.hpp>
#include <limits>

#include "VerticalMovePolicy.hpp"
#include <Scenario/Document/Constraint/ConstraintModel.hpp>
#include <Scenario/Document/Event/EventModel.hpp>
#include <Scenario/Document/State/StateModel.hpp>
#include <Scenario/Document/TimeSync/TimeSyncModel.hpp>
#include <Scenario/Document/VerticalExtent.hpp>
#include <iscore/model/EntityMap.hpp>
#include <iscore/model/Identifier.hpp>

namespace Scenario
{
void updateEventExtent(const Id<EventModel>& id, Scenario::ProcessModel& s)
{
  auto& ev = s.event(id);
  double min = std::numeric_limits<double>::max();
  double max = std::numeric_limits<double>::lowest();
  for (const auto& state_id : ev.states())
  {
    const auto& st = s.states.at(state_id);

    if (st.heightPercentage() < min)
      min = st.heightPercentage();
    if (st.heightPercentage() > max)
      max = st.heightPercentage();
  }

  ev.setExtent({min, max});
  updateTimeSyncExtent(ev.timeSync(), s);
}

void updateTimeSyncExtent(
    const Id<TimeSyncModel>& id, Scenario::ProcessModel& s)
{
  auto& tn = s.timeSyncs.at(id);
  double min = std::numeric_limits<double>::max();
  double max = std::numeric_limits<double>::lowest();
  for (const auto& ev_id : tn.events())
  {
    const auto& ev = s.events.at(ev_id);
    if (ev.extent().top() < min)
      min = ev.extent().top();
    if (ev.extent().bottom() > max)
      max = ev.extent().bottom();
  }

  tn.setExtent({min, max});
}

void updateConstraintVerticalPos(
    double y, const Id<ConstraintModel>& id, Scenario::ProcessModel& s)
{
  auto& cst = s.constraints.at(id);

  // First make the list of all the constraints to update
  static boost::container::flat_set<ConstraintModel*> constraintsToUpdate;
  static boost::container::flat_set<StateModel*> statesToUpdate;

  constraintsToUpdate.insert(&cst);
  StateModel* rec_state = &s.state(cst.startState());

  statesToUpdate.insert(rec_state);
  while (rec_state->previousConstraint())
  {
    ConstraintModel* rec_cst
        = &s.constraints.at(*rec_state->previousConstraint());
    constraintsToUpdate.insert(rec_cst);
    statesToUpdate.insert(rec_state);
    rec_state = &s.states.at(rec_cst->startState());
  }
  statesToUpdate.insert(rec_state); // Add the first state

  rec_state = &s.state(cst.endState());
  statesToUpdate.insert(rec_state);
  while (rec_state->nextConstraint())
  {
    ConstraintModel* rec_cst = &s.constraints.at(*rec_state->nextConstraint());
    constraintsToUpdate.insert(rec_cst);
    statesToUpdate.insert(rec_state);
    rec_state = &s.states.at(rec_cst->endState());
  }
  statesToUpdate.insert(rec_state); // Add the last state

  // Set the correct height
  for (auto& constraint : constraintsToUpdate)
  {
    constraint->setHeightPercentage(y);
    s.constraintMoved(*constraint);
  }

  for (auto& state : statesToUpdate)
  {
    state->setHeightPercentage(y);
    updateEventExtent(state->eventId(), s);
  }

  constraintsToUpdate.clear();
  statesToUpdate.clear();

}
}
