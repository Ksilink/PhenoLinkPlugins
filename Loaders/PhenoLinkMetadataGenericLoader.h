#ifndef GENERICLOADER_H
#define GENERICLOADER_H

#include <QObject>
#include <QtPlugin>
#include <QString>

#include "Core/Dll.h"

#include "Core/wellplatemodel.h"

#include "Core/checkoutdataloaderplugininterface.h"


class DllPluginExport GenericLoader: public QObject, public CheckoutDataLoaderPluginInterface
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID CheckoutDataLoaderPluginInterface_iid )
  Q_INTERFACES(CheckoutDataLoaderPluginInterface)

public:
  GenericLoader();

  virtual ~GenericLoader();

  virtual QString pluginName();

  virtual QStringList handledFiles();

  virtual bool isFileHandled(QString file);

  virtual ExperimentFileModel* getExperimentModel(QString file);

  virtual QString errorString();
//  ExperimentFileModel* getExperimentModel();

protected:

  void parseFolders(QDir folder, QString pattern, ExperimentFileModel* efm, QString well = QString());


  QString _error;
  int ncols = 1, nrows = 1;

};


#endif // GENERICLOADER_H
