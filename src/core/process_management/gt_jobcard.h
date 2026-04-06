#ifndef GT_JOBCARD_H
#define GT_JOBCARD_H
/*
#include "gt_core_exports.h"
#include "gt_object.h"

#include "gt_stringproperty.h"
#include "gt_intproperty.h"
#include "gt_propertystructcontainer.h"

#include <QMutex>


class GT_CORE_EXPORT GtJobCard : public GtObject
{
    Q_OBJECT
public:
    Q_INVOKABLE GtJobCard();
    ~GtJobCard();

    const QString getJobId();

    const QVariant getValueX();
    const QVariant getValueY();

    const QString debugInfo();
private:
    //GtIntProperty m_jobnumber;
    //GtIntProperty m_processnumber;

    struct Impl;
    std::unique_ptr<Impl> pimpl;


};




namespace gt {
namespace jobcard {

class GT_CORE_EXPORT Registry : public GtObject
{
    Q_OBJECT

public:
    static Registry& instance()
    {
        static Registry instance; // thread-safe since C++11
        return instance;
    };

    // Delete copy/move

  //  Registry(const Registry&) = delete;
  //  Registry& operator=(const Registry&) = delete;
   // Registry(Registry&&) = delete;
   // Registry& operator=(Registry&&) = delete;


    GtJobCard* newJobcard();;
    const QList<GtJobCard*> all();

    GtJobCard* activeJobcard();
    bool setActiveJobcard(GtJobCard* obj);

private:
    explicit Registry(GtObject* parent = nullptr);

    ~Registry() = default;

    GtJobCard* m_activeJobcard;

};



}
}


//inline gt::jobcard::Registry* objReg = &gt::jobcard::Registry::instance();
#define gtJobcards (&gt::jobcard::Registry::instance())

*/





#endif // GT_JOBCARD_H



