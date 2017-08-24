// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <QObject>
#include <Scenario/Document/BaseScenario/BaseScenario.hpp>
#include <Scenario/Document/Constraint/ConstraintModel.hpp>
#include <Scenario/Document/Event/EventPresenter.hpp>
#include <Scenario/Document/State/StatePresenter.hpp>
#include <Scenario/Document/TimeSync/TimeSyncPresenter.hpp>

#include "BaseScenarioDisplayedElementsProvider.hpp"
#include <Scenario/Document/Constraint/FullView/FullViewConstraintPresenter.hpp>

namespace Scenario
{
bool BaseScenarioDisplayedElementsProvider::matches(
    const ConstraintModel& cst) const
{
  return dynamic_cast<BaseScenario*>(cst.parent());
}

DisplayedElementsContainer
BaseScenarioDisplayedElementsProvider::make(ConstraintModel& cst) const
{
  if (auto parent_base = dynamic_cast<BaseScenario*>(cst.parent()))
  {
    return DisplayedElementsContainer{cst,
                                      parent_base->startState(),
                                      parent_base->endState(),

                                      parent_base->startEvent(),
                                      parent_base->endEvent(),

                                      parent_base->startTimeSync(),
                                      parent_base->endTimeSync()};
  }

  return {};
}

DisplayedElementsPresenterContainer
BaseScenarioDisplayedElementsProvider::make_presenters(
    const ConstraintModel& m,
    const Process::ProcessPresenterContext& ctx,
    QGraphicsItem* view_parent,
    QObject* parent) const
{
  if (auto bs = dynamic_cast<BaseScenario*>(m.parent()))
  {
    return DisplayedElementsPresenterContainer{
        new FullViewConstraintPresenter{m, ctx, view_parent,
                                        parent},
        new StatePresenter{bs->startState(), view_parent, parent},
        new StatePresenter{bs->endState(), view_parent, parent},
        new EventPresenter{bs->startEvent(), view_parent, parent},
        new EventPresenter{bs->endEvent(), view_parent, parent},
        new TimeSyncPresenter{bs->startTimeSync(), view_parent, parent},
        new TimeSyncPresenter{bs->endTimeSync(), view_parent, parent}};
  }
  return {};
}
}
