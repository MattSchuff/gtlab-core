#ifndef GT_DEEPCOPYTASK_H
#define GT_DEEPCOPYTASK_H

#include "gt_core_exports.h"
#include "gt_task.h"

namespace gt {


GT_CORE_EXPORT bool updatePropertyConnectionsViaMapping(GtTask* copy, QMap<QString, QString>* mappingUuidOldToNew);

// GtTask* deepCopyTask(GtTask* taskOrig);

} // namespace gt

#endif // GT_DEEPCOPYTASK_H
