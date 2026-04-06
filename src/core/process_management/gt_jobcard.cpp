#include "gt_jobcard.h"

#include "gt_boolproperty.h"
#include "gt_modeproperty.h"
#include "gt_modetypeproperty.h"
#include "gt_structproperty.h"

#include "gt_doublelistproperty.h"
#include "gt_intproperty.h"
#include "gt_doubleproperty.h"
#include "gt_coreapplication.h"

#include <QMetaEnum>
#include <QRegularExpressionValidator>



/*

struct GtJobCard::Impl
{
public:

    GtStringProperty uuid{"jobid", "Job ID", "", "",
        new QRegularExpressionValidator{
            QRegularExpression{R"(^\{[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[1-5][0-9a-fA-F]{3}-[89abAB][0-9a-fA-F]{3}-[0-9a-fA-F]{12}\}$)"}
        }
    };
    GtPropertyStructContainer inputData{"inputData", "Input Data"};
    GtPropertyStructContainer outputData{"outputData", "Output Data"};


    GtStringProperty sessionId{"sessionId", "Session ID", ""};
    GtStringProperty projectName{"projectName", "Project Name", ""};
    GtStringProperty taskName{"taskName", "Task Name", ""};


    enum Datatype
    {
        Integer,
        Double,
        DoubleList,
        String,
        Bool,
        ObjectUUID
    };
    static QList<Datatype> DatatypesList() {
        return {Datatype::Integer, Datatype::Double, Datatype::DoubleList, Datatype::String, Datatype::Bool, Datatype::ObjectUUID};
    }

    static GtPropertyStructDefinition getStructDefinition(Datatype datatype)
    {
        switch (datatype)
        {
            case Datatype::Bool:        return newProp("Bool",        gt::makeBoolProperty(true));
            case Datatype::Integer:     return newProp("Integer",     gt::makeIntProperty(0));
            case Datatype::Double:      return newProp("Double",      gt::makeDoubleProperty(0.0));
            case Datatype::DoubleList:  return newProp("Double List", gt::makeDoubleListProperty());
            case Datatype::ObjectUUID:  return newProp("Object UUID", gt::makeStringProperty(""));
            case Datatype::String:      return newProp("String",      gt::makeStringProperty(""));
        }
        return newProp("String",      gt::makeStringProperty(""));
    };

private:
    static GtPropertyStructDefinition newProp(const QString typeName, gt::PropertyFactoryFunction propFunc)
    {
        GtPropertyStructDefinition prop(typeName);
        prop.defineMember("target-obj-uuid", gt::makeStringProperty());
        prop.defineMember("target-obj-path", gt::makeStringProperty());
        prop.defineMember("property-id", gt::makeStringProperty());
        prop.defineMember("value", propFunc);
        return prop;
    }

};



    // GtPropertyStructContainer m_inputData;
    // QList<GtAbstractProperty*> m_inputData_cleanupList;
    // GtPropertyStructContainer m_outputData;
    // QList<GtAbstractProperty*> m_outputData_cleanupList;


GtJobCard::GtJobCard():
    pimpl(std::make_unique<Impl>())
{
    setObjectName("JobCard");
    registerProperty(pimpl->uuid);
    registerProperty(pimpl->sessionId);
    registerProperty(pimpl->projectName);
    registerProperty(pimpl->taskName);


    // auto makeMode = [&](const QString& id)
    // {
    //     auto* p = new GtModeProperty(id, id, "");

    //     QStringList vars;
    //     vars << "PressureStagnationAbs";
    //     vars << "TemperatureStagnationAbs";
    //     vars << "VelocityAngleThetaAbs";
    //     vars << "VelocityAngleR";
    //     vars << "TurbulenceIntensityAbs";
    //     vars << "TurbulentLengthScale";

    //     foreach(QString var, vars)
    //     {
    //         GtModeTypeProperty* prop = new GtModeTypeProperty(var, var);
    //         p->registerSubProperty(*prop);
    //         m_cleanupList.append(prop);
    //     }

    //     return p;
    // };

    // GtPropertyStructDefinition fixedvar("Known Variable");
    // fixedvar.defineMember("Name", makeMode);
    // fixedvar.defineMember("Value", gt::makeDoubleProperty(0));

    auto dts = Impl::DatatypesList();

    foreach (auto dt, dts)
    {
        auto p = Impl::getStructDefinition(dt);
        pimpl->inputData.registerAllowedType(p);
        pimpl->outputData.registerAllowedType(p);
    }

    registerPropertyStructContainer(pimpl->inputData);
    registerPropertyStructContainer(pimpl->outputData);

}

GtJobCard::~GtJobCard()
{
    //qDeleteAll(m_inputData_cleanupList);
}

const QString GtJobCard::getJobId() { return pimpl->uuid.getVal(); }

const QVariant GtJobCard::getValueX()
{
    auto &p = pimpl->inputData.at(0);
    QVariant x = p.valueToVariant("value");
    return x;
}

const QVariant GtJobCard::getValueY()
{
    auto &p= pimpl->inputData.at(1);
    QVariant x = p.valueToVariant("value");
    return x;
}

const QString GtJobCard::debugInfo()
{
    QString r;

    r += "Jobcard: "+this->objectName()+ " ("+this->uuid()+" / "+this->getJobId()+")\n";

    qDebug() << this->properties();
    qDebug() << this->findProperty("sessionId");
    qDebug() << this->findProperty("sessionId")->valueToVariant().toString();
    r += "   Session: "+pimpl->sessionId.getVal() + "\n";
    r += "   Project: "+pimpl->projectName.getVal() + "\n";
    r += "   Task:    "+pimpl->taskName.getVal() + "\n";

    r += "   Input Data:\n";
    for(int i=0; i< pimpl->inputData.size(); i++)
    {
        auto &x = pimpl->inputData.at(i);
        r += QString::number(i)+": " + x +", " + x.typeName() +", " + x +"\n";
    }


    for(auto& p : pimpl->inputData)
    {
        qDebug() << p;

        QString valueStr;

        valueStr = p.getMemberValToVariant("value").toString();

        QString valueType;

        valueType = p.typeName();   ; //p.valueToVariant("value").typeName();

        r += "\n";
        r += "   o target-obj-uuid: "+p.getMemberVal<QString>("target-obj-uuid")+"\n";
        r += "     target-obj-path: "+p.getMemberVal<QString>("target-obj-path")+"\n";
        r += "     property-id:     "+p.getMemberVal<QString>("property-id")+"\n";
        r += "     value-type:      "+valueType+"\n";
        r += "     value:           "+valueStr+"\n";
    }



    return r;
}

gt::jobcard::Registry::Registry(GtObject *parent)
    : GtObject(parent),
    m_activeJobcard(nullptr)
{
    setObjectName("Jobcard Registry");
}


const QList<GtJobCard *>
gt::jobcard::Registry::all()
{
    return findDirectChildren<GtJobCard*>();
}

bool gt::jobcard::Registry::setActiveJobcard(GtJobCard *obj) {
    m_activeJobcard = obj;
    return true;
}


GtJobCard* gt::jobcard::Registry::activeJobcard() {
    return m_activeJobcard;
}

GtJobCard *gt::jobcard::Registry::newJobcard() {
    GtJobCard* jc = new GtJobCard;
    appendChild(jc);
    return jc;
}
*/
