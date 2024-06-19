#ifndef PHENOLINKARRAYSCAN_H
#define PHENOLINKARRAYSCAN_H


#include <QObject>
#include <QtPlugin>
#include <QString>

#include "Core/Dll.h"

#include "Core/wellplatemodel.h"

#include "Core/checkoutdataloaderplugininterface.h"


class DllPluginExport PhenoLinkArrayScan: public QObject, public CheckoutDataLoaderPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID CheckoutDataLoaderPluginInterface_iid )
    Q_INTERFACES(CheckoutDataLoaderPluginInterface)
public:
    PhenoLinkArrayScan();

    virtual ~PhenoLinkArrayScan();

    virtual QString pluginName();

    virtual QStringList handledFiles();

    virtual bool isFileHandled(QString file);

    virtual ExperimentFileModel* getExperimentModel(QString file);

    virtual QString errorString();
    //  ExperimentFileModel* getExperimentModel();

protected:
    QString _error;

};

#endif // PHENOLINKARRAYSCAN_H
