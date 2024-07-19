#include "CarlZeissImaging.h"


#include <QFile>
#include <QDir>

#include <QDomDocument>
#include <QDomNode>

#include <QCryptographicHash>
#include <QColor>

//#include "libCZI_Config.h"
#include "CZIReader.h"


ExperimentFileModel *CZILoader::getExperimentModel(QString _file)
{
    _error = QString();
    ExperimentFileModel* r = new ExperimentFileModel();

    QStringList sfile = _file.split("/"); if (sfile.size() == 1) sfile = _file.split("\\");  sfile.pop_back();

    // Basic metadata from file


    wchar_t* czi_file = (wchar_t*)_file.utf16();

    auto stream = libCZI::CreateStreamFromFile(czi_file);
    auto cziReader = libCZI::CreateCZIReader();
    cziReader->Open(stream);

    r->setGroupName(sfile.at(std::max(0, (int)(sfile.count()-2))));
    r->setName(sfile.back());
    r->addMetadataFile(_file);

    auto seg_metadata = cziReader->ReadMetadataSegment().get()->CreateMetaFromMetadataSegment();

    if (seg_metadata->IsXmlValid())
    {
        auto metadata = seg_metadata->GetXml();
        // seg_metadata->
        // qDebug() << ;

        QDomDocument doc("CZI");
        if (!doc.setContent(QString::fromStdString(metadata)))
        {
            qDebug() << "Not loading" << _file;
            _error += QString("Error openning file %1\r\n").arg(_file);
            return r;
        }

        // doc.firstChildElement("");
        qDebug() << doc.toString();

        // Need to dig into the Xml structures to extract infos regarding the positions, etc..
    }

    // cziReader->mo

    cziReader->EnumerateSubBlocks(
        [r](int idx, const libCZI::SubBlockInfo& info)
        {

            std::string tmp = libCZI::Utils::DimCoordinateToString(&info.coordinate);

            // SequenceFileModel& seq = (*r)(row, col);
            // seq.setOwner(r);




            // info.coordinate.IsValid(x);


            qDebug() << "Index" << idx
                     << QString::fromStdString(tmp)
                     << " Rect=" << info.logicalRect.x << info.logicalRect.y
                     << info.logicalRect.w << info.logicalRect.h
                     << info.physicalSize.w << info.physicalSize.h

                     <<info.coordinate.GetNumberOfValidDimensions();
                ;

            return true;
        });



/*


    r->setProperties("ChannelCount", QString("%1").arg(channel_checker.size()));

    mrf.seek(0);
    QCryptographicHash hash(QCryptographicHash::Md5);

    hash.addData(&mrf);

    r->setProperties("hash", hash.result().toHex());
    r->setProperties("file", _file);

    mrf.close();



    QDir dir(_file); dir.cdUp();
    QString _mlf = dir.absolutePath() + "/MeasurementData.mlf";

    QFile mlf(_mlf);
    r->addMetadataFile(_mlf);

    if (!mlf.open(QIODevice::ReadOnly))
    {
        qDebug() << "Error opening file" << _mlf;
        _error += QString("Error opening file %1").arg(_mlf);
        return 0;
    }


    QDomDocument d("mlf");

    if (!d.setContent(&mlf))
    {
        qDebug() << "Not loading" << _mlf;
        _error += QString("Error opening file %1").arg(_mlf);
        mlf.close();
        return 0;
    }



    QDomElement m = d.firstChildElement("bts:MeasurementData").firstChildElement("bts:MeasurementRecord");

    QList<QDomElement> stack;

    do
    {
        stack.push_back(m.cloneNode(true).toElement());
        m = m.nextSiblingElement();
    } while (!m.isNull());

    QSet<QPoint> processed;

    QMutex mutex;
    auto analyseDomElemFunc = std::bind(analyseDomElement, std::placeholders::_1,
                                        r, dir, _file, &mutex, processed);
    QStringList errs =  QtConcurrent::blockingMapped(&threads, stack, analyseDomElemFunc);

    foreach (QString e, errs)
    {
        if (!e.isEmpty())
            _error += e;
    }

    QString MesSettings = r->property("MeasurementSettingFileName");


    dir = QDir(_file); dir.cdUp();
    QString _mes = dir.absolutePath() + "/" +MesSettings;
    r->addMetadataFile(_mes);


    QFile mes(_mes);



    if (!mes.open(QIODevice::ReadOnly))
    {
        qDebug() << "Error opening file" << _mes;
        _error += QString("Error opening file %1").arg(_mes);
        return 0;
    }


    d = QDomDocument("mes");




    if (!d.setContent(&mes))
    {
        qDebug() << "Not loading" << _mes;
        _error += QString("Error oppenning file %1\r\n").arg(_mes);
        mes.close();
        return r;
    }


    // Search for "ChannelList" && for each Channel description: Color field

    m = d.firstChildElement("bts:MeasurementSetting").firstChildElement("bts:ChannelList").firstChildElement("bts:Channel");
    //    qDebug() << d.firstChildElement("bts:MeasurementSetting").tagName();
    int binning = -1;
    QStringList chans;
    do
    {
        QString ch = m.attribute("bts:Ch");
        if (channel_checker.contains(ch))
        {
            QString color = m.attribute("bts:Color");
            //            qDebug() << "channel" << ch << "Color" << color;
            r->setProperties("ChannelsColor"+ch, color);
            QString nm = m.attribute("bts:Target") + " " + m.attribute("bts:Fluorophore");
            nm = nm.replace("Microscope Image ", "");
            if (nm.isEmpty())
                nm = ch; // Force a channel number as a name if Empty !!
            chans << nm;
        }
        int  b = m.attribute("bts:Binning").toInt();

        if (binning == -1)
            binning = b;
        else if (binning != b)
        {
            auto e = QString("Discrepancy in the binning ! cancelling the load");
            qDebug() << e;
            _error += e;
            delete r;
            return NULL;
        }

        m = m.nextSiblingElement();
    } while (!m.isNull());

    r->setChannelNames(chans);
    // qDebug() << chans;
*/

    return r;
}

QString CZILoader::errorString()
{
    return _error;
}


CZILoader::CZILoader() : CheckoutDataLoaderPluginInterface()
{
}

CZILoader::~CZILoader()
{

}

QString CZILoader::pluginName()
{
    return "CZILoader Loader v0.a";
}

QStringList CZILoader::handledFiles()
{
    return QStringList() << "*.czi";
}

bool CZILoader::isFileHandled(QString file)
{
    if (file.endsWith(".czi"))
        return true;
    else
        return false;
}
