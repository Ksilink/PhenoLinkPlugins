#ifndef PROCESSINGSPEEDTESTING_H
#define PROCESSINGSPEEDTESTING_H
#include <QtPlugin>
#include <QString>

#include "Core/Dll.h"

//#include "Core/wellplatemodel.h"

#include "Core/checkoutprocessplugininterface.h"
#include <PhenoLinkPluginCore/PhenoLinkPluginCore.h>

class ProcessingSpeedTesting : public QObject, public PhenoLinkPluginCore
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID CheckoutProcessPluginInterface_iid)
    Q_INTERFACES(CheckoutProcessPluginInterface)

public:
    ProcessingSpeedTesting();

    virtual CheckoutProcessPluginInterface* clone();
    virtual void exec();

    virtual QString plugin_version() const { return  GitHash ; }


protected: // Inputs

    StackedImage images;



    int option;
    double duration;
    bool plugin_semaphore;



protected: // outputs


    int    pixels;
    double time;


};

#endif // PROCESSINGSPEEDTESTING_H
