/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2023 German Aerospace Center (DLR)
 * Source File: gt_taskchildren_datachangerevert.cpp
 *
 *  Created on: 10.04.2026
 *  Author: Matthias Schuff (SG-VTM)
 *  Tel.:
 */
#include "gt_taskchildren_datachangerevert.h"


bool resetObjectsToOldState(QList<GtObject *> &linkedObjects, QMap<QString, GtObjectMemento> &oldState, QMap<QString, GtObjectMemento> &newState)
{
    qDebug().noquote() << "----------------------------------------------diff to revert:";
    auto diff = diffMapOfMementos(newState, oldState);

    qDebug() << "diffs:";
    foreach(auto key, diff.keys())
    {
        qDebug() << key << ":" << QString::fromUtf8(diff[key].toByteArray());
    }

    qDebug().noquote() << "----------------------------------------------resetting:";
    bool okAll = true;
    foreach(auto obj, linkedObjects)
    {
        qDebug().noquote() << "reset:" << obj->objectPath();
        bool ok = obj->applyDiff(diff[obj->uuid()]);

        if(!ok)
        {
            qDebug().noquote() << "could not apply diff";
            okAll = false;
        }
    }

    return okAll;
}

QMap<QString, GtObjectMementoDiff> diffMapOfMementos(QMap<QString, GtObjectMemento> &oldState, QMap<QString, GtObjectMemento> &newState)
{
    QMap<QString, GtObjectMementoDiff> difflist;

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
        gtFatal() << "root objects are missing after process component, that shouldn't happen?";
        //return {};
    }

    if (deleteCandidates.size()>0) {
        qDebug() << "These objects are added after calculator:";
        foreach(auto _obj, deleteCandidates)
        {
            qDebug().noquote() << "    - " << _obj.uuid() << _obj.ident();
            qDebug().noquote() << QString::fromUtf8(_obj.toByteArray());
        }
        gtFatal() << "root objects are added after process component, that shouldn't happen?";
        //return {};
    }

    foreach(auto _obj, keys)
    {
        difflist[_obj] = GtObjectMementoDiff{oldState[_obj], newState[_obj]};
    }

    return difflist;
}
