/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2023 German Aerospace Center (DLR)
 * Source File: gt_taskchildren_datachangerevert.h
 *
 *  Created on: 10.04.2026
 *  Author: Matthias Schuff (SG-VTM)
 *  Tel.:
 */
#ifndef GT_TASKCHILDREN_DATACHANGEREVERT_H
#define GT_TASKCHILDREN_DATACHANGEREVERT_H

#include "gt_core_exports.h"
#include "gt_object.h"
#include "gt_objectmemento.h"
#include "gt_objectmementodiff.h"


GT_CORE_EXPORT QMap<QString, GtObjectMementoDiff> diffMapOfMementos(QMap<QString, GtObjectMemento>& oldState,
                                                                    QMap<QString, GtObjectMemento>& newState);

GT_CORE_EXPORT bool resetObjectsToOldState(QList<GtObject*>& linkedObjects,
                                           QMap<QString, GtObjectMemento>& oldState,
                                           QMap<QString, GtObjectMemento>& newState );

#endif // GT_TASKCHILDREN_DATACHANGEREVERT_H
