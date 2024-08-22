#include "CarlZeissImaging.h"


#include <QFile>
#include <QDir>

#include <QDomDocument>
#include <QDomNode>

#include <QCryptographicHash>
#include <QColor>

//#include "libCZI_Config.h"
#include "CZIReader.h"

QDebug operator<<(QDebug dbg, const QDomNode& node)
{
    QString s;
    QTextStream str(&s, QIODevice::WriteOnly);
    node.save(str, 2);
    dbg << qPrintable(s);
    return dbg;
}

QDebug operator<<(QDebug dbg, const QDomElement& node)
{
    QString s;
    QTextStream str(&s, QIODevice::WriteOnly);
    node.save(str, 2);
    dbg << qPrintable(s);
    return dbg;
}


ExperimentFileModel *CZILoader::getExperimentModel(QString _file)
{
    _error = QString();
    ExperimentFileModel* r = new ExperimentFileModel();

    QStringList sfile = _file.split("/"); if (sfile.size() == 1) sfile = _file.split("\\"); // sfile.pop_back();

    // Basic metadata from file


    wchar_t* czi_file = (wchar_t*)_file.utf16();

    auto stream = libCZI::CreateStreamFromFile(czi_file);
    auto cziReader = libCZI::CreateCZIReader();
    cziReader->Open(stream);

    QString name = sfile.back(); name.chop(4);

    r->setGroupName(sfile.at(std::max(0, (int)(sfile.count()-2))));
    r->setName(name);
    r->addMetadataFile(_file);

    auto seg_metadata = cziReader->ReadMetadataSegment().get()->CreateMetaFromMetadataSegment();

    QMap<int, QDomElement> scene_map;

    if (seg_metadata->IsXmlValid())
    {
        auto metadata = seg_metadata->GetXml();
        // seg_metadata->
        // qDebug() << ;

        QDomDocument doc("CZI");
        if (!doc.setContent(QString::fromStdString(metadata).replace("\r\n","")))
        {
            qDebug() << "Not loading" << _file;
            _error += QString("Error openning file %1\r\n").arg(_file);
            return r;
        }
        {
            QFile file("c:/Temp/LastCZI.xml");
            if (!file.open(QIODevice::WriteOnly))
            {
                qDebug() << "Not Exporting CZI xml to file (c:/Temp/LastCZI.xml)" ;
            }
            else
            {
                QTextStream out(&file);
                out << doc.toString();
                file.close();
            }
        }
        // from czi lib / metadata
        //   ImageDocument.Metadata.Information.Image.Dimensions.S.Scenes.Scene
        //   ImageDocument/Metadata/Information/Image/Dimensions/S/Scenes

        auto id= doc.firstChildElement("ImageDocument");
        auto md = id.firstChildElement("Metadata");
        auto inf = md.firstChildElement("Information");
        auto img = inf.firstChildElement("Image");
        auto dim  = img.firstChildElement("Dimensions");
        auto s = dim.firstChildElement("S");
        auto scenes = s.firstChildElement("Scenes");
        auto scene = scenes.firstChildElement("Scene");

        while (!scene.isNull())
        {

            scene_map[scene.attributes().namedItem("Index").nodeValue().toInt()] = scene;
            scene = scene.nextSiblingElement("Scene");
        }


        // /ImageDocument/Metadata/Information/Image/Dimensions/Channels/Channel[1]


        auto chans = dim.firstChildElement("Channels");
        auto chan = chans.firstChildElement("Channel");
        QMap<int, QString> channel_map;
        QMap<QString, QString> chan_colors;
        while (!chan.isNull())
        {
            int chid = 1+chan.attributes().namedItem("Id").nodeValue().split(":").last().toInt();
            auto name = chan.attributes().namedItem("Name").nodeValue();
            auto color = chan.firstChildElement("Color").text();
            chan_colors[QString("ChannelsColor%1").arg(chid)] =  color;

            channel_map[chid] = name;

            chan = chan.nextSiblingElement("Channel");
        }

        for (auto c=chan_colors.begin(); c != chan_colors.end(); ++c)
            r->setProperties(c.key(), c.value());

        QStringList cnames;
        for (auto v: channel_map)
            cnames << v;
        r->setChannelNames(cnames);


        auto xp = md.firstChildElement("Experiment");
        // qDebug() << "XP" << xp;
        auto xpb = xp.firstChildElement("ExperimentBlocks");
        // qDebug() << "XPB" << xpb;
        auto ab = xpb.firstChildElement("AcquisitionBlock");
        // qDebug() << "ab" << ab;
        auto sds = ab.firstChildElement("SubDimensionSetups");
        // qDebug() << "sds" << sds;
        auto rs = sds.firstChildElement("RegionsSetup");
        // qDebug() << "rs" << rs;
        auto sh = rs.firstChildElement("SampleHolder");
        // qDebug() << "sh" << sh;
        auto templ = sh.firstChildElement("Template");
        // qDebug() << "temp" << templ;


        // qDebug() << templ.firstChildElement("ShapeRows") << templ.firstChildElement("ShapeColumns");


        int rows = templ.firstChildElement("ShapeRows").text().toInt();
        r->setRowCount(rows);
        int cols = templ.firstChildElement("ShapeColumns").text().toInt();
        r->setColCount(cols);
        qDebug() << rows << cols;



    }
    else
    {
        qDebug() << "Missing XML description of plate in" << _file;
        _error += QString("Missing XML description of plate in %1\r\n").arg(_file);
        return r;
    }


// /ImageDocument/Metadata/Experiment//AcquisitionBlock/SubDimensionSetups/RegionsSetup/SampleHolder/Template/




    QMap<QString, QMap<int, QMap<int,  QMap<int, int>> > > field_counter;

    QString file_no_ext=_file;
    file_no_ext.chop(4); // remove .czi


    cziReader->EnumerateSubBlocks(
        [r, &scene_map, &field_counter, &file_no_ext](int idx, const libCZI::SubBlockInfo& info)
        {

            int coordS, coordC;

            if (info.coordinate.TryGetPosition(libCZI::DimensionIndex::S, &coordS) &&
                info.coordinate.TryGetPosition(libCZI::DimensionIndex::C, &coordC))
            {

                std::string tmp = libCZI::Utils::DimCoordinateToString(&info.coordinate);

                auto scene = scene_map[coordS];

                QString well;

                auto arrayname = scene.firstChildElement("ArrayName");

                if (arrayname.text().isEmpty())
                {
                    well = scene.attributes().namedItem("Name").nodeValue();
                }
                    else
                {
                    well = arrayname.text();
                }

                QPoint p = ExperimentDataTableModel::stringToPos(well);

                SequenceFileModel& seq = (*r)(p);
                seq.setOwner(r);

                int z = 0;
                if (!info.coordinate.TryGetPosition(libCZI::DimensionIndex::Z, &z))
                    z=0;

                field_counter[well][coordS][coordC][z] = field_counter[well][coordS][coordC][z]+1;
                int field = field_counter[well][coordS][coordC][z];

                int t = 0;
                if (!info.coordinate.TryGetPosition(libCZI::DimensionIndex::T, &t))
                    t = 0;
                else
                    field = 1;



                seq.setProperties(QString("%6f%1s%2t%3c%4%5").arg(field).arg(z+1).arg(t+1).arg(coordC+1).arg("X").arg(well), QString("%1").arg(info.logicalRect.x));
                seq.setProperties(QString("%6f%1s%2t%3c%4%5").arg(field).arg(z+1).arg(t+1).arg(coordC+1).arg("Y").arg(well), QString("%1").arg(info.logicalRect.y));
                // seq.setProperties(QString("f%1s%2t%3c%4%5").arg(field).arg(z).arg(t).arg(c).arg("Z"), z);

                qDebug() << well << tmp << info.logicalRect.x << info.logicalRect.y;



                seq.addFile(t+1, field, z+1, coordC+1, QString("%1.%2.czi").arg(file_no_ext).arg(idx) );
                seq.checkValidity();

                r->setMeasurements(p, true);

            }
            else
            {
                QString msg = QString("Unable to retrieve a scene from file %1").arg(file_no_ext+".czi");
                qDebug() <<  msg;

                return false;
            }



            return true;
        });



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
