#ifndef PerkinElmerLoader_H
#define PerkinElmerLoader_H

#include <QtPlugin>
#include <QString>

#include "Core/Dll.h"

#include "Core/wellplatemodel.h"


#include "Core/checkoutdataloaderplugininterface.h"


class DllPluginExport PerkinElmerLoader: public QObject, public CheckoutDataLoaderPluginInterface
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID CheckoutDataLoaderPluginInterface_iid )
  Q_INTERFACES(CheckoutDataLoaderPluginInterface)

public:
  PerkinElmerLoader();

  virtual ~PerkinElmerLoader();

  virtual QString pluginName();

  virtual QStringList handledFiles();

  virtual bool isFileHandled(QString file);

  virtual ExperimentFileModel* getExperimentModel(QString file);

  virtual QString errorString();
//  ExperimentFileModel* getExperimentModel();
  
protected:
  QString _error;

};


#endif // PerkinElmerLoader
