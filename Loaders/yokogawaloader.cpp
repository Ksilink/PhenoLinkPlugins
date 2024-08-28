#include "yokogawaloader.h"


#include <QFile>
#include <QDir>

#include <QDomDocument>
#include <QDomNode>

#include <QCryptographicHash>
#include <QtConcurrent>
#include <functional>
#include <QColor>

/**
 * @brief analyseDomElement function analyzes a QDomElement and extracts information from its attributes.
 * It processes the attributes related to row, column, timepoint, field index, z index, channel, type, and action.
 * It also checks for errors in the XML file and adds them to the error string.
 * If the row and column values are valid, it updates the ExperimentFileModel with the extracted information.
 * It also checks if the element contains an image file and adds it to the ExperimentFileModel if it does.
 * Finally, it checks the validity of the ExperimentFileModel.
 *
 * @param m The QDomElement to be analyzed.
 * @param r The ExperimentFileModel to be updated.
 * @param dir The QDir object representing the directory of the XML file.
 * @param _file The path of the XML file.
 * @param mutex The QMutex object for thread safety.
 * @param processed The set of processed positions.
 * @return The error string, if any.
 */
QString analyseDomElement(QDomElement m, ExperimentFileModel* r,
                          QDir dir, QString _file, QMutex* mutex, QSet<QPoint>& processed )
{
    QString _error;

    bool isImage = false;
    int row = -1, col = -1,  action = 0;
    int timepoint = 0, fieldidx = 0, zindex = 0, channel = 0;

    for (int i = 0; i < m.attributes().count(); ++i)
    {
        QDomNode a = m.attributes().item(i);

        QString tag = a.nodeName().split(":").back();

        if (tag == "Row") row = a.nodeValue().toInt() - 1;      // The code is 0 indexed , the xml is 1 indexed
        if (tag == "Column") col = a.nodeValue().toInt() - 1;   // The code is 0 indexed, the xml is 1 indexed

        if (tag == "TimePoint") timepoint = a.nodeValue().toInt();
        if (tag == "FieldIndex") fieldidx = a.nodeValue().toInt();
        if (tag == "ZIndex") zindex = a.nodeValue().toInt();
        if (tag == "Ch") channel = a.nodeValue().toInt();

        if (tag == "Type" && a.nodeValue() == "IMG")   isImage = true;
        if (tag=="Action" && a.nodeValue() != "2D")
            action=1;
    }

    for (int i = 0; i < m.attributes().count(); ++i)
    {
        QDomNode a = m.attributes().item(i);

        QString tag = a.nodeName().split(":").back();

        if (tag == "Type" && a.nodeValue() == "ERR" && action == 0) // Need to iterate a second time around the tags to check the Err input on 2D action
        {
            QString err = QString("Error in XML no image produced for %1: [R%2 C%3] %8 T%4 F%5 Z%6 C%7\r\n")
                    .arg(_file).arg(row + 1).arg(col + 1)
                    .arg(timepoint).arg(fieldidx).arg(zindex).arg(channel)
                    .arg(QString("%1%2").arg(QLatin1Char('A'+row)).arg((int)(col+1), 2, 10, QChar('0')));
            qDebug() << err;
            mutex->lock();
            _error += err;
            mutex->unlock();

        }
    }

    if (!((row == -1 && col == -1) ) )
    {
        mutex->lock();
        SequenceFileModel& seq = (*r)(row, col);
        mutex->unlock();
        seq.setOwner(r);

        QStringList skipL = QStringList() << "Row" << "Column" << "TimePoint" << "FieldIndex" << "ZIndex" << "Ch";
        for (int i = 0; i < m.attributes().count(); ++i)
        {
            QDomNode a = m.attributes().item(i);
            QString tag =a.nodeName().split(":").back();
            if (skipL.contains(tag) || (tag == "Type" && (a.nodeValue() == "IMG" || a.nodeValue() == "ERR" )))
                continue; // skip already stored data

            QString mtag = QString("%6f%1s%2t%3c%4%5").arg(fieldidx).arg(zindex).arg(timepoint).arg(channel).arg(tag).arg(seq.Pos());
            mutex->lock();
            seq.setProperties(mtag, a.nodeValue());
            mutex->unlock();
        }

        auto pos = QPoint(row, col);

        mutex->lock();
        if (processed.contains(pos))
        {
            isImage |= r->hasMeasurements(pos);
        }
        else
        {
            processed.insert(pos);
        }

        r->setMeasurements(QPoint(row, col), isImage);

        mutex->unlock();
        if (channel != 0 && timepoint != 0 && fieldidx != 0 && zindex != 0)
        {
            if (!m.childNodes().at(0).nodeValue().isEmpty() && isImage)
            {
                QString file = QString("%1/%2").arg(dir.absolutePath(), m.childNodes().at(0).nodeValue());
                if (!(file.endsWith(".tif") || file.endsWith(".tiff") || file.endsWith(".jxl")))
                    file = QString();


                mutex->lock();
                seq.addFile(timepoint, fieldidx, zindex, channel, file);
                mutex->unlock();
            }
            else
            {
                mutex->lock();
                if (seq.hasFile(timepoint, fieldidx, zindex, channel))
                    seq.addFile(timepoint, fieldidx, zindex, channel, QString());
                mutex->unlock();
            }


            mutex->lock();
            seq.checkValidity();
            mutex->unlock();
        }
    }

    return _error;
}





ExperimentFileModel *YokogawaLoader::getExperimentModel(QString _file)
{
    _error = QString();
    ExperimentFileModel* r = new ExperimentFileModel();

    QStringList sfile = _file.split("/"); if (sfile.size() == 1) sfile = _file.split("\\");  sfile.pop_back();
    r->setGroupName(sfile.at(std::max(0, (int)(sfile.count()-2))));
    r->setName(sfile.back());

    r->addMetadataFile(_file);

    QFile mrf(_file);
    if (!mrf.open(QFile::ReadOnly))
    {
        qDebug() << "Error opening file" << _file;
        _error += QString("Error opening file %1").arg(_file);
        return 0;
    }

    QDomDocument doc("mrf");
    if (!doc.setContent(&mrf))
    {
        mrf.close();

        qDebug() << "Error Empty or malformated xml file" << _file;
        _error += QString("Error Empty or malformated xml file %1").arg(_file);


        return 0;
    }

    QDomElement el = doc.firstChildElement("bts:MeasurementDetail");

    if (el.isNull()) return 0;

    for (int i = 0; i < el.attributes().count(); ++i)
    {
        QDomNode a = el.attributes().item(i);

        QString tag = a.nodeName().split(":").back();
        r->setProperties(tag, a.nodeValue());
    }

    QMap<QString, int> channel_checker;

    for( int ir = 0; ir < el.childNodes().count(); ++ir)
    {
        QDomNode n = el.childNodes().at(ir);

        if (n.attributes().contains("bts:Ch"))
        {
            QString ch = QString("_ch%1").arg(n.attributes().namedItem("bts:Ch").nodeValue());
            channel_checker[n.attributes().namedItem("bts:Ch").nodeValue()]=1;

            for (int i = 0; i < n.attributes().count(); ++i)
            {
                QDomNode a = n.attributes().item(i);
                if (a.nodeName() == "bts:Ch") continue;
                QString tag = a.nodeName().split(":").back() + ch;
                r->setProperties(tag, a.nodeValue());
            }
        }
    }


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

    return r;
}

QString YokogawaLoader::errorString()
{
    return _error;
}


YokogawaLoader::YokogawaLoader() : CheckoutDataLoaderPluginInterface()
{
    threads.setMaxThreadCount(12);

}

YokogawaLoader::~YokogawaLoader()
{

}

QString YokogawaLoader::pluginName()
{
    return "Yokogawa Loader v0.a";
}

QStringList YokogawaLoader::handledFiles()
{
    return QStringList() << "MeasurementDetail.mrf";
}

bool YokogawaLoader::isFileHandled(QString file)
{
    if (file.endsWith("MeasurementDetail.mrf"))
        return true;
    else
        return false;
}
