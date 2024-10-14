#ifndef ZStackProjection_H
#define ZStackProjection_H
#include <QtPlugin>
#include <QString>

#include "Core/Dll.h"

//#include "Core/wellplatemodel.h"

#include "Core/checkoutprocessplugininterface.h"
#include <PhenoLinkPluginCore/PhenoLinkPluginCore.h>

class ZStackProjection : public QObject, public PhenoLinkPluginCore
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID CheckoutProcessPluginInterface_iid)
    Q_INTERFACES(CheckoutProcessPluginInterface)

public:
    ZStackProjection();

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

#endif // ZStackProjection
