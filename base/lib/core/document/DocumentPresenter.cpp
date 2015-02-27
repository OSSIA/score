#include "DocumentPresenter.hpp"

#include <interface/documentdelegate/DocumentDelegateModelInterface.hpp>
#include <interface/documentdelegate/DocumentDelegatePresenterInterface.hpp>
#include <core/tools/utilsCPP11.hpp>
#include <core/document/DocumentView.hpp>
#include <core/document/DocumentModel.hpp>


using namespace iscore;

DocumentPresenter::DocumentPresenter(DocumentModel* m, DocumentView* v, QObject* parent) :
    NamedObject {"DocumentPresenter", parent},
            m_commandQueue {std::make_unique<CommandQueue> (this) },
            m_view{v},
            m_model{m}
{
    connect(&m_selection, &SelectionStack::currentSelectionChanged,
            this,         &DocumentPresenter::currentSelectionChanged);
    connect(m_model,     &DocumentModel::selectionChanged,
            this,        &DocumentPresenter::newSelection);
}

void DocumentPresenter::setPresenterDelegate(DocumentDelegatePresenterInterface* pres)
{
    if(m_presenter)
    {
        m_presenter->deleteLater();
    }

    m_presenter = pres;

    // Commands
    connect(m_presenter, &DocumentDelegatePresenterInterface::submitCommand,
            this,		 &DocumentPresenter::applyCommand, Qt::QueuedConnection);

    // Selection
    connect(this,        &DocumentPresenter::currentSelectionChanged,
            m_presenter, &DocumentDelegatePresenterInterface::newItemsSelected);
}

//// Selection ////
void DocumentPresenter::newSelection(Selection s)
{
    m_selection.push(s);
}


//// Locking / unlocking ////
void DocumentPresenter::lock_impl()
{
    QByteArray arr;
    Serializer<DataStream> ser {&arr};
    ser.readFrom(m_lockedObject);
    emit lock(arr);
}

void DocumentPresenter::unlock_impl()
{
    QByteArray arr;
    Serializer<DataStream> ser {&arr};
    ser.readFrom(m_lockedObject);
    emit unlock(arr);
    m_lockedObject = ObjectPath();
}


//// Commands ////
void DocumentPresenter::applyCommand(SerializableCommand* cmd)
{
    m_commandQueue->pushAndEmit(cmd);
}

void DocumentPresenter::initiateOngoingCommand(SerializableCommand* cmd, QObject* objectToLock)
{
    // TODO optimize with the known document.
    m_lockedObject = ObjectPath::pathFromObject(objectToLock);
    lock_impl();

    m_ongoingCommand = cmd;
    m_ongoingCommand->redo();
}

void DocumentPresenter::continueOngoingCommand(SerializableCommand* cmd)
{
    m_ongoingCommand->undo();
    m_ongoingCommand->mergeWith(cmd);
    m_ongoingCommand->redo();
    delete cmd;
}

void DocumentPresenter::rollbackOngoingCommand()
{
    if(m_ongoingCommand)
    {
        unlock_impl();
        m_ongoingCommand->undo();
        delete m_ongoingCommand;
        m_ongoingCommand = nullptr;
    }
}

void DocumentPresenter::validateOngoingCommand()
{
    if(m_ongoingCommand)
    {
        unlock_impl();
        m_ongoingCommand->undo();
        m_ongoingCommand->disableMerging();
        m_commandQueue->pushAndEmit(m_ongoingCommand);
        m_ongoingCommand = nullptr;
    }
}
