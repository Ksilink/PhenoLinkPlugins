#include "perkinelmerloader.h"


#include <QFile>
#include <QDir>

#include <QDomDocument>
#include <QDomNode>

#include <QCryptographicHash>
#include <QtConcurrent>
#include <functional>
#include <QColor>

QRgb WavelengthToRGB(float w)
{
    float R,G,B;

    if (w >= 380 && w < 440)
    {
        R = -(w - 440.) / (440. - 350.);
        G = 0.0;
        B = 1.0;
    }
    else if (w >= 440 && w < 490)
    {
        R = 0.0;
        G = (w - 440.) / (490. - 440.);
        B = 1.0;
    }
    else if (w >= 490 && w < 510)
    {
        R = 0.0;
        G = 1.0;
        B = -(w - 510.) / (510. - 490.);
    }
    else if ( w >= 510 && w < 580)
    {
        R = (w - 510.) / (580. - 510.);
        G = 1.0;
        B = 0.0;
    }
    else if ( w >= 580 && w < 645)
    {
        R = 1.0;
        G = -(w - 645.) / (645. - 580.);
        B = 0.0;
    }
    else if ( w >= 645 && w <= 780)
    {
        R = 1.0;
        G = 0.0;
        B = 0.0;
    }
    else
    {
        R = 0.0;
        G = 0.0;
        B = 0.0;
    }
    float SSS = 0;
    if (w >= 380 && w < 420)
        SSS = 0.3 + 0.7*(w - 350) / (420 - 350);
    else if ( w >= 420 && w <= 700)
        SSS = 1.0;
    else if ( w > 700 && w <= 780)
        SSS = 0.3 + 0.7*(780 - w) / (780 - 700);
    else
        SSS = 0.0;
    SSS *= 255;

    return qRgb(SSS*R, SSS*G, SSS*B);

}


QString analyseDomElement(QDomElement m, ExperimentFileModel* r,
                          QDir dir, QString _file, QMutex* mutex)
{
    QString _error;
    int row = -1, col = -1;
    int timepoint = 0, fieldidx = 0, zindex = 0, channel = 0;

    row = m.firstChildElement("Row").text().toInt();
    col = m.firstChildElement("Col").text().toInt();

    timepoint = 1+m.firstChildElement("TimePointID").text().toInt();
    fieldidx = m.firstChildElement("FieldID").text().toInt();
    zindex = m.firstChildElement("PlaneID").text().toInt();
    channel = m.firstChildElement("ChannelID").text().toInt();


    if (m.firstChildElement("State").text() != "Ok")
    {
        mutex->lock();
        QString err = QString("Error in XML no image produced for %1: R%2 C%3 T%4 F%5 Z%6 C%7\r\n")
                          .arg(_file).arg(row).arg(col).arg(timepoint).arg(fieldidx).arg(zindex).arg(channel);
        qDebug() << err;
        _error += err;
        mutex->unlock();
    }

    mutex->lock();
    if (!r->hasProperty(QString("ChannelColor%1").arg(channel)))
    {
        float w = m.firstChildElement("MainEmissionWavelength").text().toFloat();
        r->setProperties(QString("ChannelColor%1").arg(channel),
                         QColor(WavelengthToRGB(w)).name(QColor::HexArgb));
    }

    SequenceFileModel& seq = (*r)(row, col);
    mutex->unlock();
    //            qDebug() << row << col << seq.Pos();
    seq.setOwner(r);

    // See if we want thoses tags !
    //        for (int i = 0; i < m.attributes().count(); ++i)
    //        {
    //            QDomNode a = m.attributes().item(i);
    //            QString tag =a.nodeName().split(":").back();
    //            if (tag == "Row" || tag == "Column"||tag == "TimePoint"|| tag == "FieldIndex"||tag == "ZIndex"|| tag == "Ch"|| (tag == "Type" && a.nodeValue() == "IMG"))
    //                continue; // skip already stored data
    //            QString mtag = QString("f%1s%2t%3c%4%5").arg(fieldidx).arg(zindex).arg(timepoint).arg(channel).arg(tag);

    //            //                qDebug() << tag << a.nodeValue();
    //            mutex->lock();
    //            seq.setProperties(mtag, a.nodeValue());
    //            mutex->unlock();
    //        }

    QString x = m.firstChildElement("PositionX").text();
    QString y = m.firstChildElement("PositionY").text();
    QString z = m.firstChildElement("PositionZ").text();

    mutex->lock();
    seq.setProperties(QString("%6f%1s%2t%3c%4%5").arg(fieldidx).arg(zindex).arg(timepoint).arg(channel).arg("X").arg(seq.Pos()), x);
    seq.setProperties(QString("%6f%1s%2t%3c%4%5").arg(fieldidx).arg(zindex).arg(timepoint).arg(channel).arg("Y").arg(seq.Pos()), y);
    seq.setProperties(QString("%6f%1s%2t%3c%4%5").arg(fieldidx).arg(zindex).arg(timepoint).arg(channel).arg("Z").arg(seq.Pos()), z);
    mutex->unlock();


    r->setMeasurements(QPoint(row, col), true);
    QString url = m.firstChildElement("URL").text();

    if (!url.isEmpty())
    {

        QString file = QString("%1/%2").arg(dir.absolutePath()).arg(url);
        //        qDebug() <<row << col << timepoint << fieldidx << zindex << channel<<file;
        if (true /*|| QFileInfo::exists(file)*/)
        {
            mutex->lock();
            seq.addFile(timepoint, fieldidx, zindex, channel, file);
            mutex->unlock();
        }
        else
        {
            mutex->lock();
            //                error_count ++;
            QString err = QString("Error : Unable to locate file: %2 at timepoint %3, field %4, zpos %5 for channel %6\r\n")
                              .arg(file).arg(timepoint).arg(fieldidx).arg(zindex).arg(channel);


            qDebug() << err;
            _error += err;
            seq.setInvalid();
            mutex->unlock();
        }
    }
    mutex->lock();
    seq.checkValidity();
    mutex->unlock();


    return _error;
}

ExperimentFileModel *PerkinElmerLoader::getExperimentModel(QString _file)
{
    QThreadPool threads; threads.setMaxThreadCount(12);

    ExperimentFileModel* r = new ExperimentFileModel();




    _error = QString();


    QFile mrf(_file);
    if (!mrf.open(QFile::ReadOnly))
    {
        qDebug() << "Error opening file" << _file;
        _error += QString("Error oppenning file %1").arg(_file);
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


    // We have the perkin elmer plate, let see what we can extract from that big stuff

    QDomElement el = doc.firstChildElement("EvaluationInputData").firstChildElement("Images");


    if (el.isNull()) {
        return 0;
    }




    mrf.seek(0);
    QCryptographicHash hash(QCryptographicHash::Md5);

    hash.addData(&mrf);

    QStringList sfile = _file.split("/"); if (sfile.size() == 1) sfile = _file.split("\\");  sfile.pop_back();
    r->setGroupName(sfile.at(std::max(0, (int)(sfile.count()-2))));
    r->setName(sfile.back());

    r->setProperties("hash", hash.result().toHex());
    r->setProperties("file", _file);
    r->addMetadataFile(_file);

    mrf.close();

    auto plate = doc.firstChildElement("EvaluationInputData").firstChildElement("Plates").firstChildElement("Plate");
    r->setRowCount(plate.firstChildElement("PlateRows").text().toInt());
    r->setColCount(plate.firstChildElement("PlateColumns").text().toInt());


    QDomElement m = el.firstChildElement("Image");

    QList<QDomElement> stack;

    do
    {
        stack.push_back(m);
        m = m.nextSiblingElement();
    } while (!m.isNull());

    QDir dir(_file); dir.cdUp();

    QMutex mutex;
    auto analyseDomElemFunc = std::bind(analyseDomElement, std::placeholders::_1,
                                        r, dir, _file, &mutex);
    QStringList errs =  QtConcurrent::blockingMapped(&threads, stack, analyseDomElemFunc);

    foreach (QString e, errs)
    {
        if (!e.isEmpty())
            _error += e;
    }

    QStringList chansName;

    SequenceFileModel& sfm = r->getFirstValidSequenceFiles();

    //m.firstChildElement("")
    int nb_chans = sfm.getChannels(); //
    QMap<int, QString> chan_names;
    m = el.firstChildElement("Image");
    do
    {
        int ch = m.firstChildElement("ChannelID").text().toInt();
        QString name = m.firstChildElement("ChannelName").text();
        chan_names[ch] = name;
        if (chan_names.size() > nb_chans)
            break;
        m = m.nextSiblingElement();
    } while (!m.isNull());

    if (chan_names.isEmpty())
    {
        m = doc.firstChildElement("EvaluationInputData").firstChildElement("Maps").firstChildElement("Map");
        do
        {
            int ch = m.firstChildElement("Entry").attribute("ChannelID", "-1").toInt();
            if (ch != -1)
            {
                if ( !m.firstChildElement("Entry").firstChildElement("ChannelName").isNull())
                {
                    QString name = m.firstChildElement("Entry").firstChildElement("ChannelName").text();
                    chan_names[ch] = name;
                    if (chan_names.size() > nb_chans)
                        break;
                }


                if (!r->hasProperty(QString("ChannelColor%1").arg(ch)) && !m.firstChildElement("Entry").firstChildElement("MainEmissionWavelength").isNull())
                {
                    float w = m.firstChildElement("Entry").firstChildElement("MainEmissionWavelength").text().toFloat();
                    if (w != 0.0)
                        r->setProperties(QString("ChannelColor%1").arg(ch),
                                         QColor(WavelengthToRGB(w)).name(QColor::HexArgb));
                    else
                        r->setProperties(QString("ChannelColor%1").arg(ch),
                                         QColor(Qt::white).name(QColor::HexArgb));
                }

            }
            m = m.nextSiblingElement();
        } while (!m.isNull());

    }


    QStringList channames;
    for (auto v: chan_names)
    {
        channames << v;
    }
    r->setChannelNames(channames);

    qDebug() << channames;
    //    chansName.reserve();





    return r;
}

QString PerkinElmerLoader::errorString()
{
    return _error;
}


PerkinElmerLoader::PerkinElmerLoader(): CheckoutDataLoaderPluginInterface()
{

}

PerkinElmerLoader::~PerkinElmerLoader()
{

}

QString PerkinElmerLoader::pluginName()
{
    return "PerkinElmer Loader v0.a";
}

QStringList PerkinElmerLoader::handledFiles()
{
    return QStringList() << "Index.xml" <<  "*.idx.xml";
}

bool PerkinElmerLoader::isFileHandled(QString file)
{

    if (file.endsWith(".idx.xml"))
        return true;
    if  (file == "Index.xml")
        return true;

    return false;
}
