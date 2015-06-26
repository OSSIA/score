#include "BaseElementModel.hpp"

#include "source/Document/Constraint/ConstraintModel.hpp"
#include "source/Document/Constraint/ViewModels/FullView/FullViewConstraintViewModel.hpp"

#include <iscore/document/DocumentInterface.hpp>

#include "Commands/Constraint/AddProcessToConstraint.hpp"
#include "Commands/Constraint/AddBoxToConstraint.hpp"
#include "Commands/Constraint/Box/AddSlotToBox.hpp"
#include "Commands/Constraint/Box/Slot/ResizeSlotVertically.hpp"
#include "Commands/Constraint/Box/Slot/AddLayerModelToSlot.hpp"
#include "Commands/Scenario/ShowBoxInViewModel.hpp"

#include "Document/Constraint/Box/BoxModel.hpp"
#include "Document/Constraint/Box/Slot/SlotModel.hpp"
#include "ProcessInterface/ProcessModel.hpp"
#include "ProcessInterface/LayerModel.hpp"

#include <QApplication>
using namespace Scenario;

#include <core/document/Document.hpp>
#include <iscore/selection/SelectionDispatcher.hpp>
#include "Control/ScenarioControl.hpp"
#include <iscore/presenter/PresenterInterface.hpp>
#include <iscore/tools/SettableIdentifierGeneration.hpp>

BaseElementModel::BaseElementModel(QObject* parent) :
    iscore::DocumentDelegateModelInterface {id_type<iscore::DocumentDelegateModelInterface>(getNextId()), "BaseElementModel", parent},
    m_baseConstraint {new ConstraintModel{
                            id_type<ConstraintModel>{0},
                            id_type<AbstractConstraintViewModel>{0},
                            0,
                            this}}
{
    ConstraintModel::Algorithms::changeAllDurations(*m_baseConstraint, std::chrono::minutes{3});
    m_baseConstraint->setObjectName("BaseConstraintModel");

    initializeNewDocument(m_baseConstraint->fullView());

    // Help for the FocusDispatcher.
    connect(this, &BaseElementModel::setFocusedPresenter,
            &m_focusManager, &ProcessFocusManager::setFocusedPresenter);

    connect(&m_focusManager, &ProcessFocusManager::sig_defocusedViewModel,
            this, &BaseElementModel::on_viewModelDefocused);
    connect(&m_focusManager, &ProcessFocusManager::sig_focusedViewModel,
            this, &BaseElementModel::on_viewModelFocused);
}

void BaseElementModel::initializeNewDocument(const FullViewConstraintViewModel *viewmodel)
{
    using namespace Scenario::Command;
    const auto& constraint_model = viewmodel->model();

    AddProcessToConstraint cmd1
    {
        {
            {"BaseElementModel", this->id()},
            {"BaseConstraintModel", {}}
        },
        "Scenario"
    };
    cmd1.redo();
    auto scenarioId = (*constraint_model.processes().begin())->id();

    AddBoxToConstraint cmd2
    {
        ObjectPath{
            {"BaseElementModel", this->id()},
            {"BaseConstraintModel", {}}
        }
    };
    cmd2.redo();
    auto box = *constraint_model.boxes().begin();

    ShowBoxInViewModel cmd3 {
        ObjectPath{
            {"BaseElementModel", this->id()},
            {"BaseConstraintModel", {}},
            {"FullViewConstraintViewModel", viewmodel->id()}
        },
        box->id() };
    cmd3.redo();

    AddSlotToBox cmd4
    {
        ObjectPath{
            {"BaseElementModel", this->id()},
            {"BaseConstraintModel", {}},
            {"BoxModel", box->id() }
        }
    };
    cmd4.redo();
    auto slotId = (*box->getSlots().begin())->id();

    ResizeSlotVertically cmd5
    {
        ObjectPath{
            {"BaseElementModel", this->id()},
            {"BaseConstraintModel", {}},
            {"BoxModel", box->id() },
            {"SlotModel", slotId}
        },
        1500
    };
    cmd5.redo();

    AddLayerModelToSlot cmd6
    {
        {
            {"BaseElementModel", this->id()},
            {"BaseConstraintModel", {}},
            {"BoxModel", box->id() },
            {"SlotModel", slotId}
        },
        {
            {"BaseElementModel", this->id()},
            {"BaseConstraintModel", {}},
            {"ScenarioModel", scenarioId}
        }
    };
    cmd6.redo();
}

namespace {
void updateSlotFocus(const LayerModel* lm, bool b)
{
    if(lm && lm->parent())
    {
        if(auto slot = dynamic_cast<SlotModel*>(lm->parent()))
        {
            slot->setFocus(b);
        }
    }
}
}

void BaseElementModel::on_viewModelDefocused(const LayerModel* vm)
{
    // Disable the focus on previously focused view model
    updateSlotFocus(vm, false);

    // Deselect
    iscore::SelectionDispatcher selectionDispatcher(
                iscore::IDocument::documentFromObject(*this)->selectionStack());
    selectionDispatcher.setAndCommit(Selection{});
}

void BaseElementModel::on_viewModelFocused(const LayerModel* process)
{
    // TODO why not presenter ?
    // Enable focus on the new viewmodel
    updateSlotFocus(process, true);
}

// TODO candidate for ProcessSelectionManager.
void BaseElementModel::setNewSelection(const Selection& s)
{;
    auto process = m_focusManager.focusedModel();

    if(s.empty())
    {
        if(process)
        {
            process->setSelection(Selection{});
            m_displayedConstraint->selection.set(false);
            m_focusManager.focusNothing();
        }
    }
    else if(*s.begin() == m_displayedConstraint)
    {
        if(process)
        {
            process->setSelection(Selection{});
            m_focusManager.focusNothing();
        }

        m_displayedConstraint->selection.set(true);
    }
    else
    {
        // We know by the presenter that all objects
        // in a given selection are in the same Process.
        m_displayedConstraint->selection.set(false);
        auto newProc = parentProcess(*s.begin());
        if(process && newProc != process)
        {
            process->setSelection(Selection{});
        }

        if(newProc)
        {
            newProc->setSelection(s);
        }
    }

    emit focusMe();
}

void BaseElementModel::setDisplayedConstraint(const ConstraintModel* constraint)
{
    // TODO only keep it saved at one place.
    m_displayedConstraint = constraint;
    m_focusManager.focusNothing();
}
