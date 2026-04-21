#include "gt_deepcopytask.h"


namespace gt {

bool updatePropertyConnectionsViaMapping(GtTask *copy, QMap<QString, QString> *mappingUuidOldToNew)
{
    QList<GtPropertyConnection*> newCons = copy->findChildren<GtPropertyConnection*>();

    foreach (GtPropertyConnection* con, newCons)
    {
        auto origSrcUuid = con->sourceUuid();
        auto origTargetUuid = con->targetUuid();

        bool ok = true;

        if (mappingUuidOldToNew->contains(origSrcUuid))
        {
            QString newUuid = mappingUuidOldToNew->find(origSrcUuid).value();
            con->setSourceUuid( newUuid );
        }
        else
        {
            gtInfo() << "Could not update property connection: source!";
            con->setSourceUuid("");
            ok = false;
        }

        if (mappingUuidOldToNew->contains(origTargetUuid))
        {
            QString newUuid = mappingUuidOldToNew->find(origTargetUuid).value();
            con->setTargetUuid( newUuid );
        }
        else
        {
            gtInfo() << "Could not update property connection: target!";
            con->setTargetUuid("");
            ok = false;
        }

        if(ok)
        {
            con->makeConnection();
        }

    }

    return true;
}




// void makePropertyConnection(GtProcessComponent *parent, GtProcessComponent *src, const QString &propSrc, GtProcessComponent *dst, const QString &propDst)
// {
//     if(!src->findProperty(propSrc)) {
//         gtFatal() << "Parent:" << parent;
//         gtFatal() << "src:" << src;
//         gtFatal() << "dst:" << dst;
//         gtFatal() << "source property not found:" << propSrc;
//         return;
//     }
//     if(!dst->findProperty(propDst)) {
//         gtFatal() << "Parent:" << parent;
//         gtFatal() << "src:" << src;
//         gtFatal() << "dst:" << dst;
//         gtFatal() << "target property not found:" << propDst;
//         return;
//     }

//     GtPropertyConnection* c = new GtPropertyConnection;
//     c->setSourceUuid(src->uuid());
//     c->setSourceProp(src->findProperty(propSrc)->ident());
//     c->setTargetUuid(dst->uuid());
//     c->setTargetProp(dst->findProperty(propDst)->ident());
//     parent->appendChild(c);
//     c->makeConnection();
// }


// GtTask* deepCopyTask(GtTask* taskOrig)
// {
//     GtTask* taskNew = nullptr;







//     return taskNew;
// }

} // namespace gt
