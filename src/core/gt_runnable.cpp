/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2023 German Aerospace Center (DLR)
 * Source File: gt_runnable.cpp
 *
 *  Created on: 22.03.2016
 *  Author: Stanislaus Reitenbach (AT-TW)
 *  Tel.: +49 2203 601 2907
 */

#include <QDir>
#include <QDebug>

#include "gt_runnable.h"
#include "gt_objectmemento.h"
#include "gt_objectfactory.h"
#include "gt_coreapplication.h"
#include "gt_task.h"
#include "gt_project.h"
#include "gt_logging.h"

GtRunnable::GtRunnable(QString projectPath) :
    m_projectPath{std::move(projectPath)}
{
    setObjectName("GtRunnable");
}

void
GtRunnable::run()
{
    bool success = true;

    readObjects();

    // set all states to QUEUED
    for (GtProcessComponent* calc : qAsConst(m_queue))
    {
        calc->setStateRecursively(GtProcessComponent::QUEUED);
    }

    qDebug() << "";
    qDebug() << "--- calculator execution ---";

    for (GtProcessComponent* pc : qAsConst(m_queue))
    {
        // success
        if (pc->exec()) continue;

        // failure
        gtError() << "Calculator execution failed!";
        success = false;

        auto const tasks = pc->findChildren<GtTask*>();

        for (GtTask* task : tasks)
        {
            if (task->currentState() ==
                GtProcessComponent::TERMINATION_REQUESTED)
            {
                task->setState(GtProcessComponent::TERMINATED);
            }

            if (task->currentState() == GtProcessComponent::TERMINATED &&
                pc->currentState() != GtProcessComponent::TERMINATED)
            {
                pc->setState(GtProcessComponent::TERMINATED);
            }
        }
        break;
    }

    if (success)
    {
        qDebug() << "--- calculator execution finished ---";
        qDebug() << "";

        writeObjects();
    }
    else
    {
        qDebug() << "--- calculator execution failed ---";
        qDebug() << "";

        writeObjects();
    }

    m_successfulRun = success;

    emit runnableFinished();
}

void
GtRunnable::requestInterruption()
{
    QList<GtTask*> tasks;

    foreach (GtProcessComponent* calc, m_queue)
    {
        GtTask* task = qobject_cast<GtTask*>(calc);

        if (task)
        {
            tasks << task;
        }

        tasks.append(calc->findChildren<GtTask*>());
    }

    foreach (GtTask* task, tasks)
    {
        task->requestInterruption();
    }
}

QDir
GtRunnable::tempDir()
{
    return GtCoreApplication::applicationTempDir();
}

QString
GtRunnable::projectPath()
{
    if (!m_projectPath.isEmpty())
    {
        return m_projectPath;
    }

    if (!gtApp->currentProject())
    {
        return QString();
    }

    return gtApp->currentProject()->path();
}

bool
GtRunnable::clearTempDir(const QString& path)
{
    return GtCoreApplication::removeTempDir(path);
}

void
GtRunnable::readObjects()
{
    for (GtObjectMemento& memento : m_inputData)
    {
        GtObject* obj = memento.restore(gtObjectFactory);

        if (obj)
        {
            m_linkedObjects.append(obj);
        }
    }
}

/*
inline QString stringrepeat(const QString& input, size_t num)
{
    std::ostringstream os;
    std::fill_n(std::ostream_iterator<std::string>(os), num, input.toStdString());
    return QString::fromStdString(os.str());
}

inline void _printObjectWithChildren(GtObject* x, int lvl=0)
{
    QString intend = stringrepeat("  ", lvl);
    qDebug() << intend << x << "->" << x->objectPath();
    //qDebug() << intend << "   p:"  << x->parentObject();
    qDebug() << intend << "   c:";
    foreach(auto c, x->findDirectChildren())
    {
        _printObjectWithChildren(c, lvl+1);
    }

}


inline void _printLinkedObjects(QList<GtObject*>* linkedObjects)
{
    for(int i=0; i<linkedObjects->size(); i++)
    {
        auto x = linkedObjects->at(i);
        qDebug() << "parent:"  << x->parentObject();
        _printObjectWithChildren(x);
    }

}*/

void
GtRunnable::writeObjects()
{
    //qDebug() << "================================writeObjects================================";
    //_printLinkedObjects(&m_linkedObjects);
    //qDebug() << "================================END writeObjects================================";

    for (GtObject* obj : qAsConst(m_linkedObjects))
    {
        m_outputData << GtObjectMemento(obj);
    }
}
