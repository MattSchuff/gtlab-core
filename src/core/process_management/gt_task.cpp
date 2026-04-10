/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2023 German Aerospace Center (DLR)
 *
 *  Created on: 28.07.2015
 *  Author: Stanislaus Reitenbach (AT-TW)
 *  Tel.: +49 2203 601 2907
 */

#include "gt_task.h"
#include "gt_accessdata.h"
#include "gt_calculator.h"
#include "gt_abstractrunnable.h"
#include "gt_coreapplication.h"
#include "gt_objectlinkproperty.h"
#include "gt_objectpathproperty.h"
#include "gt_processrunnerglobals.h"

#include <QDebug>
#include <QThreadPool>

#include <algorithm>

#include "gt_objectmemento.h"
#include "gt_objectmementodiff.h"

#include "gt_objectfactory.h"

struct GtTask::Impl
{
    /// Event loop
    QEventLoop eventLoop;

    /// List of all data to merge
    QList<GtObjectMemento> dataToMerge;

    /// Monitoring data table
    GtMonitoringDataTable monitoringDataTable;

    /// Interruption flag
    QAtomicInt interrupt;

    /// Access Selection property for the process runner
    GtAccessSelectionProperty processRunner{
        "processRunner", tr("Process Runner"),
        gt::process_runner::S_ACCESS_ID,
        tr("Process Runner to run task with. Only relevant for the root task")
    };

    GtBoolProperty applyMementoEvenWhenCalculatorFails{"applyMementoEvenWhenCalculatorFails", "Apply memento on fail", "", false};

    GtObjectMementoDiff objectMementoDiffAfterTask;
};

GtTask::GtTask() :
    m_maxIter(QStringLiteral("maxIter"),
              tr("Number Of Iterations"),
              tr("Number of iteration steps"),
              GtIntProperty::BoundLow, 1, 1),
    m_currentIter(QStringLiteral("currentIter"), tr("Current Iteration")),
    m_lastEval(GtTask::EVAL_FINISHED),
    pimpl(std::make_unique<Impl>())
{
    setObjectName(QStringLiteral("Task"));

    connect(this, &GtTask::finished,
            &pimpl->eventLoop, &QEventLoop::quit);

    setState(GtTask::NONE);
    setFlag(GtObject::UserRenamable, true);

    m_currentIter.setVal(0);

    qRegisterMetaType<GtMonitoringDataSet>("GtMonitoringDataSet");

    registerProperty(pimpl->processRunner, tr("Execution"));
    registerProperty(pimpl->applyMementoEvenWhenCalculatorFails, "Execution");

    pimpl->processRunner.hide(!gtApp || !gtApp->devMode());
}

GtTask::~GtTask() = default;

bool
GtTask::exec()
{
    setRunnable(nullptr);

    // check skipped indicator
    if (isSkipped())
    {
        setState(GtTask::SKIPPED);
        gtDebug() << objectName() << tr("skipped");
        return true;
    }

    setRunnable(findParent<GtAbstractRunnable*>());

    auto runnable = this->runnable();
    if (!runnable)
    {
        setState(GtProcessComponent::FAILED);
        gtError() << tr("%1: Failed to execute task, runnable not found!")
                         .arg(objectName());
        return false;
    }

    // collect all calculator properties
    QList<GtAbstractProperty*> const props = fullPropertyList();

    // search for object link and object path properties
    for (GtAbstractProperty* prop : props)
    {
        if (auto* linkProp = qobject_cast<GtObjectLinkProperty*>(prop))
        {
            // object link property found
            auto* obj = runnable->data<GtObject*>(linkProp->linkedObjectUUID());

            if (!obj)
            {
                gtWarning().medium()
                    << tr("Linked object for '%1' not found in runnable")
                           .arg(linkProp->objectName());
                continue;
            }

            // linked object found -> store inside list
            linkedObjects().append(obj);
            continue;
        }
        if (auto* pathProp = qobject_cast<GtObjectPathProperty*>(prop))
        {
            // object path property found
            auto* obj = runnable->data<GtObject*>(pathProp->path());

            if (!obj)
            {
                gtWarning().medium()
                    << tr("Linked object path '%1' not found in runnable")
                           .arg(pathProp->path().toString());
                continue;

            }

            // linked object found -> store inside list
            linkedObjects().append(obj);
        }
    }

    // reset evaluator variables
    m_currentIter.setVal(0);

    // Initialize individual evaluator setting
    if (!setUp())
    {
        setState(GtProcessComponent::FAILED);
        return false;
    }

    // check max. iteration steps
    if (m_maxIter <= 0)
    {
        setState(GtProcessComponent::FAILED);
        return false;
    }

    // emit `finihed` signal if task was not triggered by the `run` method
    auto finally = gt::finally([this, isStandaone = pimpl->eventLoop.isRunning()](){
        if (!isStandaone) emit finished();
    });

    setState(GtProcessComponent::RUNNING);

    // clear existing monitoring data
    emit triggerClearMonitoringData();

    // start iteration
    if (!runIteration())
    {
        setState(GtProcessComponent::FAILED);
        return false;
    }

    // max. number of iteration steps reached
    if (childHasWarnings())
    {
        setState(GtProcessComponent::WARN_FINISHED);
    }
    else if (this->currentState() != GtProcessComponent::WARN_FINISHED)
    {
        setState(GtProcessComponent::FINISHED);
    }

    // return success
    return true;
}

void
GtTask::run(GtAbstractRunnable* r)
{
    if (!r)
    {
        setState(GtProcessComponent::FAILED);
        gtError() << tr("%1: Failed to run task, invalid runnable!")
                         .arg(objectName());
        return;
    }

    setState(GtTask::RUNNING);
    pimpl->dataToMerge.clear();

    setRunnable(r);

    QThreadPool* tp = QThreadPool::globalInstance();

    if (!tp)
    {
        return;
    }

    runnable()->setAutoDelete(false);

    connect(runnable().data(), &GtAbstractRunnable::runnableFinished,
            this, &GtTask::handleRunnableFinished);

    tp->start(runnable());

    qDebug() << "#### exec event loop...";

    pimpl->eventLoop.exec();

    qDebug() << "#### exec finished";
}

QList<GtCalculator*>
GtTask::calculators()
{
    return findDirectChildren<GtCalculator*>();
}

QList<GtProcessComponent*>
GtTask::processComponents()
{
    return findDirectChildren<GtProcessComponent*>();
}

QList<GtObjectMemento>&
GtTask::dataToMerge()
{
    return pimpl->dataToMerge;
}

bool
GtTask::setUp()
{
    // nothing to do here
    return true;
}

GtTask::EVALUATION
GtTask::evaluate()
{
    // nothing to do here
    return GtTask::EVAL_OK;
}

bool
GtTask::runIteration()
{
    // get list of all child process components
    //    QList<GtProcessComponent*> childs = processComponents();

    qDebug() << "starting iteration...";

    do
    {
        if (!runChildElements())
        {
            return false;
        }

        // check evaluation result
        switch (m_lastEval)
        {
            case GtTask::EVAL_FINISHED:
            {
                // iteration finished successfully
                setState(GtProcessComponent::FINISHED);
                return true;
            }

            case GtTask::EVAL_FAILED:
            {
                // iteration failed
                setState(GtProcessComponent::FAILED);
                return false;
            }

            default:
                break;
        }

    }
    while (m_currentIter < m_maxIter);

    return true;
}

int
GtTask::monitoringDataSize() const
{
    return pimpl->monitoringDataTable.size();
}

const GtMonitoringDataTable&
GtTask::monitoringDataTable()
{
    return pimpl->monitoringDataTable;
}

int
GtTask::maxIterationSteps() const
{
    return m_maxIter;
}

int
GtTask::currentIterationStep() const
{
    return m_currentIter.getVal();
}

GtAccessData
GtTask::selectedProcessRunner() const
{
    return pimpl->processRunner.accessData();
}

void
GtTask::onObjectDataMerged()
{
    foreach (GtPropertyConnection* connection,
             findDirectChildren<GtPropertyConnection*>())
    {
        if (!connection->isConnected())
        {
            connection->makeConnection();
        }
    }
}

void
GtTask::enableMaxIterationProperty()
{
    registerProperty(m_maxIter);
}

void
GtTask::enableCurrentIterationMonitoring()
{
    registerMonitoringProperty(m_currentIter);
}





inline QString stringrepeat(const QString& input, size_t num)
{
    std::ostringstream os;
    std::fill_n(std::ostream_iterator<std::string>(os), num, input.toStdString());
    return QString::fromStdString(os.str());
}

inline void _printObjectWithChildren(GtObject* x, int lvl=0)
{
    QString intend = stringrepeat("  ", lvl);
    qDebug().noquote() << intend << x << "->" << x->objectPath();
    //qDebug() << intend << "   p:"  << x->parentObject();
    qDebug().noquote() << intend << "   c:";
    foreach(auto c, x->findDirectChildren())
    {
        _printObjectWithChildren(c, lvl+1);
    }
}

inline void _printLinkedObjects2(QList<GtObject*>& linkedObjects)
{
    for(int i=0; i<linkedObjects.size(); i++)
    {
        auto x = linkedObjects.at(i);
        qDebug().noquote() << "parent:"  << x->parentObject();
        _printObjectWithChildren(x);
    }
}


QMap<QString, GtObjectMementoDiff> _diffOldNew(QMap<QString, GtObjectMemento>& oldState, QMap<QString, GtObjectMemento>& newState)
{
    QMap<QString, GtObjectMementoDiff> difflist;

    //qDebug() << "----------------------------------------------mememto before:";
    foreach(auto m, oldState)
    {
        //qDebug().noquote() << QString::fromUtf8(m.toByteArray());
    }

    //qDebug() << "----------------------------------------------mememto after:";
    foreach(auto m, newState)
    {
        //qDebug().noquote() << QString::fromUtf8(m.toByteArray());
    }

    QList<GtObjectMemento> addCandidates;
    QList<GtObjectMemento> deleteCandidates;
    QStringList keysBefore = oldState.keys();
    QStringList keysAfter = newState.keys();
    QStringList keys;
    foreach(auto _obj, keysBefore)
    {
        if (keysAfter.contains(_obj))
        {
            keys.append(_obj);
        }
    }

    foreach(auto _uuid, keysBefore)
    {
        if(!keysAfter.contains(_uuid)) {
            addCandidates.append(oldState[_uuid]);
        }
    }
    foreach(auto _uuid, keysAfter)
    {
        if(!keysBefore.contains(_uuid)) {
            deleteCandidates.append(newState[_uuid]);
        }
    }

    if (addCandidates.size()>0) {

        qDebug().noquote() << "These objects are missing after calculator:";
        foreach(auto _obj, addCandidates)
        {
            qDebug().noquote() << "    - " << _obj.uuid() << _obj.ident();
            qDebug().noquote() << QString::fromUtf8(_obj.toByteArray());
        }
        gtFatal() << "root objects are missing after process component, that shouldn't happen";
        //return {};
    }

    if (deleteCandidates.size()>0) {
        qDebug() << "These objects are added after calculator:";
        foreach(auto _obj, deleteCandidates)
        {
            qDebug().noquote() << "    - " << _obj.uuid() << _obj.ident();
            qDebug().noquote() << QString::fromUtf8(_obj.toByteArray());
        }
        gtFatal() << "root objects are added after process component, that shouldn't happen";
        //return {};
    }

    //qDebug() << "Diffing the rest:";
    foreach(auto _obj, keys)
    {
        qDebug().noquote() << "key: " << _obj;

        GtObjectMemento old = oldState[_obj];
        GtObjectMementoDiff diff(old, newState[_obj]);

        difflist[_obj] = diff;

        qDebug().noquote() << QString::fromUtf8(diff.toByteArray());
    }



    return difflist;
}


bool _resetObjectsToBefore2(QList<GtObject*>& linkedObjects, QMap<QString, GtObjectMemento>& before, QMap<QString, GtObjectMemento>& after )
{

    //auto obj = m.toObject(*(GtObjectFactory::instance()));
    //QPointer<GtObject> obj2{obj.release()};
    //linkedObjects->append(obj2);

    //qDebug().noquote() << "----------------------------------------------RESET, BEFORE:" << linkedObjects.size();
    //_printLinkedObjects2(linkedObjects);


    qDebug().noquote() << "----------------------------------------------diff to revert:";
    auto diff = _diffOldNew(after, before);

    qDebug() << "diffs:";
    foreach(auto key, diff.keys())
    {
        qDebug() << key << ":" << QString::fromUtf8(diff[key].toByteArray());
    }


    qDebug().noquote() << "----------------------------------------------resetting:";
    bool okAll = true;
    foreach(auto m, linkedObjects)
    {
        GtObject* obj = m;

        qDebug() << "reset:" << obj->objectPath();
        bool ok = obj->applyDiff(diff[obj->uuid()]);

        if(!ok)
        {
            qDebug() << "could not apply diff";
            okAll = false;
        }


        //if (!m_source->applyDiff(*helper->sumDiff()))
        //{
        //    gtErrorId(GT_EXEC_ID)
        //    << tr("Data changes from the task '%1' could not be "
        //          "merged back into datamodel!")
        //            .arg(m_task->objectName());
        //    m_task->setState(GtProcessComponent::FAILED);
        //}


    }


    //qDebug().noquote() << "----------------------------------------------RESET, RESTORED:" << linkedObjects.size();
    //_printLinkedObjects2(linkedObjects);



    return okAll;
}

bool
GtTask::runChildElements()
{
    QList<GtProcessComponent*> childs = processComponents();

    // increment current iteration step and continue iteration
    m_currentIter.setVal(m_currentIter.getVal() + 1);

    qDebug() << "iteration step (" << m_currentIter << "/" << m_maxIter <<
             ")";

    // trigger transfer of monitoring properties before running calculators
    emit transferMonitoringProperties();

    // reset state of child prcess elements
    foreach (GtProcessComponent* comp, childs)
    {
        comp->setStateRecursively(GtProcessComponent::QUEUED);
    }

    qDebug() << "running calculators...";

    auto linkedObjs = this->runnable()->linkedObjects();

    // run calculators
    foreach (GtProcessComponent* comp, childs)
    {
        //qDebug().noquote() << "_printLinkedObjects2(linkedObjs) BEFORE CHILD "+comp->objectName();
        //_printLinkedObjects2(linkedObjs);
        //qDebug().noquote() << "<<<<<<<<-------------------------------->>>>>>>>>>";

        QMap<QString,GtObjectMemento> mementoBefore2;
        foreach(auto _obj, linkedObjs)
        {
            mementoBefore2[_obj->uuid()] = _obj->toMemento();
        }


        bool success = comp->exec();


        QMap<QString,GtObjectMemento> mementoAfter2;
        foreach(auto _obj, linkedObjs)
        {
            mementoAfter2[_obj->uuid()] = _obj->toMemento();
        }


        bool ok = true;

        if (!success)
        {
            // calculator run failed
            setState(GtProcessComponent::FAILED);

            qDebug() << "   |-> run failed!";

            ok=false;
        }
        else if (isInterruptionRequested())
        {
            gtWarning() << "task terminated!";
            setState(GtProcessComponent::TERMINATED);

            ok = false;
        }
        else
        {
            GtCalculator* calc = qobject_cast<GtCalculator*>(comp);

            if (calc && calc->runFailsOnWarning())
            {
                if (calc->currentState() == GtProcessComponent::WARN_FINISHED)
                {
                    calc->setState(FAILED);
                    setState(GtProcessComponent::FAILED);
                    ok = false;
                }
            }
        }

        if(!ok)
        {
            qDebug() << "Calculator failed, resetting data tree to before calculator";

            if(!_resetObjectsToBefore2(linkedObjs, mementoBefore2, mementoAfter2))
            {
                qDebug().noquote() << "Could not reset memento";
            }

            return false;
        }
        else
        {
            _diffOldNew(mementoBefore2, mementoAfter2);
        }

    }


    qDebug() << "evaluating...";
    // evaluate current iteration step
    m_lastEval = evaluate();

    // trigger transfer of monitoring properties after evaluation
    emit transferMonitoringProperties();

    // collect monitoring data for entire task
    GtMonitoringDataSet monData = collectMonitoringData();

    // check whether monitoring data has entries
    if (!monData.isEmpty())
    {
        // monitoring data available - emit signal
        emit monitoringDataTransfer(m_currentIter, monData);
    }

    return true;
}

GtMonitoringDataSet
GtTask::collectMonitoringData()
{
    GtMonitoringDataSet retval;

    collectMonitoringDataHelper(retval, this);

    return retval;
}

bool
GtTask::applyMementoEvenWhenCalculatorFails()
{
    return this->pimpl->applyMementoEvenWhenCalculatorFails.getVal();
}

void GtTask::setObjectMementoDiffAfterTask(const GtObjectMementoDiff &diff)
{
    pimpl->objectMementoDiffAfterTask = std::move(diff);
}

const GtObjectMementoDiff &GtTask::objectMementoDiffAfterTask()
{
    return pimpl->objectMementoDiffAfterTask;
}

QList<GtPropertyConnection*>
GtTask::collectPropertyConnections()
{
    QList<GtPropertyConnection*> retval;

    collectPropertyConnectionHelper(retval, this);

    return retval;
}

void
GtTask::requestInterruption()
{
    pimpl->interrupt.testAndSetOrdered(0, 1);
    setState(TERMINATION_REQUESTED);
}

bool
GtTask::isInterruptionRequested() const
{
    return static_cast<int>(pimpl->interrupt);
}

void
GtTask::collectMonitoringDataHelper(GtMonitoringDataSet& map,
                                    GtProcessComponent* component)
{
    // check component
    if (!component)
    {
        return;
    }

    // get monitoring properties
    auto monProps = component->monitoringProperties();
    auto conMonProps = component->containerMonitoringPropertyRefs();

    // check whether monitoring properties exists
    if (!monProps.isEmpty() || !conMonProps.isEmpty())
    {
        // create new monitoring data container
        GtMonitoringData monData;

        // iterate over monitoring properties and append them to container
        for (const auto* prop : monProps)
        {
            monData.addData(prop->ident(), prop->valueToVariant());
        }

        // iterate over container monitoring property references and append
        // them to container
        for (const auto& propRef : conMonProps)
        {
            if (auto* resolved = propRef.resolve(*component))
            {
                monData.addData(propRef.toString(), resolved->valueToVariant());
            }
        }

        // append monitoring data container to monitoring map
        map.insert(component->uuid(), monData);
    }

    // iterate over children
    foreach (GtProcessComponent* child,
             component->findDirectChildren<GtProcessComponent*>())
    {
        // collect data for each child recursively
        collectMonitoringDataHelper(map, child);
    }
}

void
GtTask::collectPropertyConnectionHelper(QList<GtPropertyConnection*>& list,
                                        GtProcessComponent* component)
{
    // check component
    if (!component)
    {
        return;
    }

    foreach (GtPropertyConnection* connection,
             findDirectChildren<GtPropertyConnection*>())
    {
        // check whether same connection already exists in list
        if (!list.contains(connection))
        {
            list << connection;
        }
    }

    // iterate over children
    foreach (GtProcessComponent* child,
             component->findDirectChildren<GtProcessComponent*>())
    {
        // collect data for each child recursively
        collectPropertyConnectionHelper(list, child);
    }
}

bool
GtTask::childHasWarnings() const
{
    auto const childs = findDirectChildren<GtProcessComponent*>();
    return std::any_of(std::begin(childs), std::end(childs),
                       [](const GtProcessComponent* child) {
        return child->currentState() == WARN_FINISHED;
    });
}

void
GtTask::handleRunnableFinished()
{
    bool success = true;

    qDebug() << __FUNCTION__;

    if (!runnable()->successful())
    {
        success = false;
    }
    else
    {
        pimpl->dataToMerge.append(runnable()->outputData());
    }

    disconnect(runnable().data(), &GtAbstractRunnable::runnableFinished,
               this, &GtTask::handleRunnableFinished);

    delete runnable();

    if (success)
    {
        if (childHasWarnings())
        {
            setState(GtProcessComponent::WARN_FINISHED);
        }
        else
        {
            setState(GtProcessComponent::FINISHED);
        }
    }
    else
    {
        setState(GtProcessComponent::FAILED);
    }

    emit finished();
}

void
GtTask::onMonitoringDataAvailable(int iteration, GtMonitoringDataSet const& set)
{
    // append data set to data table and check success
    if (!pimpl->monitoringDataTable.append(iteration, set))
    {
        gtWarning().medium() << tr("Could not append data set!");
        return;
    }

    emit monitoringDataAvailable();
}

void
GtTask::clearMonitoringData()
{
    pimpl->monitoringDataTable.clear();
}
