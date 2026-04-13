#pragma once

#include "gt_object.h"
#include <QDebug>
#include <QPointer>
#include <sstream>

namespace debugprintout {

inline QString stringrepeat(const QString& input, size_t num)
{
    std::ostringstream os;
    std::fill_n(std::ostream_iterator<std::string>(os), num, input.toStdString());
    return QString::fromStdString(os.str());
}

inline void printObjectWithChildren(GtObject* x, int lvl=0)
{
    QString intend = stringrepeat("  ", lvl);
    qDebug().noquote() << intend << x << "->" << x->objectPath();
    //qDebug() << intend << "   p:"  << x->parentObject();
    qDebug().noquote() << intend << "   c:";
    foreach(auto c, x->findDirectChildren())
    {
        printObjectWithChildren(c, lvl+1);
    }
}

inline void printObjectList(QList<QPointer<GtObject>>& objects)
{
    for(auto x: objects)
    {
        qDebug().noquote() << "parent:"  << x->parentObject();
        printObjectWithChildren(x,0);
    }

}

inline void printObjectList(QList<GtObject*>& objects)
{
    for(auto x: objects)
    {
        qDebug().noquote() << "parent:"  << x->parentObject();
        printObjectWithChildren(x,0);
    }
}

/*
inline void _printObj(GtObject &newObj, int lvl)
{
    QString intend = "";
    for(int i=0;i<lvl;i++) intend += "  ";
    qDebug() << "OK:" << intend + newObj.objectPath() << &newObj;
    foreach(auto c, newObj.findDirectChildren())
    {
        _printObj(*c, lvl+1);
    }
}
*/



}


