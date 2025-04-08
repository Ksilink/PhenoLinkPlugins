#include "PhenoLinkMetadataGenericLoader.h"


#include <QFile>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QDir>

#include <QDomDocument>
#include <QDomNode>

#include <QCryptographicHash>
#include <QtConcurrent>
#include <functional>
#include <QColor>




void GenericLoader::parseFolders(QDir folder, QString pattern, ExperimentFileModel* efm, QString well)
{

    QStringList matchers_init = pattern.split("(", Qt::SkipEmptyParts);
    QString pattern_simpl = pattern;
    int offset = 0;
    QStringList matchers;
    for (int i = 0; i < matchers_init.size(); ++i)
    {
        QString mat = matchers_init[i].split(":").front();
        if (!matchers_init[i].contains(":")) continue;

        matchers << mat;

        pattern_simpl = pattern_simpl.replace(mat +":", "");
    }

    QRegularExpression exp(pattern_simpl);

    auto entries = folder.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    QStringList chans = efm->getChannelNames();


    for (auto& entry : entries)
    {
//        qDebug() << /*entry.filePath();*/
        if (entry.isDir())
            parseFolders(QDir(entry.filePath()), pattern, efm);
        if (entry.isFile())
        {
            qDebug() << entry.filePath() << entry.fileName();
            auto match = exp.match(entry.fileName());
            if (match.hasMatch())
            {
                QStringList matches = match.capturedTexts();
                // remove first because it captures the whole string
                matches.removeFirst();


                // Row & Col
//                "image_matcher": "(ChanName:.*)_(WellRow:[A-Z])_(WellCol:[0-9]{3})_r_(FieldX:[0-9]{4})_c_(FieldY:[0-9]{4})_t_(TimePoint:[0-9]{8})_z_(Zpos:[0-9]{4}).tif",
                int row = 0, col = 0;

                if (!well.isEmpty())
                {
                    QPoint pt = ExperimentDataTableModel::stringToPos(well);
                    row = pt.x();
                    col = pt.y();
                }

                if (matchers.contains("Well"))
                {
                    int idx = matchers.indexOf("Well");
                    QPoint pt = ExperimentDataTableModel::stringToPos(matches.at(idx));
                    row = pt.x();
                    col = pt.y();
                }
                else if (matchers.contains("WellRow") && matchers.contains("WellCol"))
                {

                    int idx = matchers.indexOf("WellRow");

                    bool ok;
                    row  = matches[idx].toInt(&ok);
                    if (!ok)
                        row = (matches[idx].at(0).toLatin1() - 'A');

                    idx = matchers.indexOf("WellCol");
                    col = matches[idx].toInt(&ok)-1;
                }

                int timepoint = 1, zpos = 1;
                int fieldidx = 0, channel = 0;


                if (matchers.contains("TimePoint"))
                {
                    timepoint  = matches[matchers.indexOf("TimePoint")].toInt();
                }
                if (matchers.contains("Zpos"))
                {
                    zpos  = matches[matchers.indexOf("Zpos")].toInt()+1;
                }
                if (matchers.contains("Channel"))
                {
                    channel  = matches[matchers.indexOf("Channel")].toInt();
                }
                if (matchers.contains("Field"))
                {
                    fieldidx  = matches[matchers.indexOf("Field")].toInt();

                }
                else if (matchers.contains("FieldX") && matchers.contains("FieldY"))
                {
                    fieldidx = matches[matchers.indexOf("FieldX")].toInt() + 100 * 
                        matches[matchers.indexOf("FieldY")].toInt();
                }

                if (matchers.contains("ChanName"))
                {
                    QString cn = matches[matchers.indexOf("ChanName")];

                    if (!chans.contains(cn))
                    {
                        chans << cn;
                        efm->setChannelNames(chans);
                    }
                    channel = chans.indexOf(cn)+1;
                }

                ncols = std::max(ncols, col);
                nrows = std::max(nrows, row);
                qDebug() << ncols << nrows;

                SequenceFileModel& seq = (*efm)(row, col);
                seq.setOwner(efm);
                efm->setMeasurements(QPoint(row, col), true);
                seq.addFile(timepoint, fieldidx, zpos, channel, entry.filePath() );
                seq.checkValidity();

            }
        }
    }
}



ExperimentFileModel *GenericLoader::getExperimentModel(QString _file)
{
    _error = QString();

    QFile file(_file);

    if (!file.open(QFile::ReadOnly))
    {
        _error = "Cannot open file " + _file;
        qDebug() << _error;
        return nullptr;
    }

    auto doc = QJsonDocument::fromJson(file.readAll()).object();

    QStringList skipMeta = QStringList() << "subfolder" << "image_matcher";

//    {
//        "subfolder": ["WT", "DCM"],
//        "image_matcher": "(ChanName:.*)_(WellRow:[A-Z])_(WellCol:[0-9]{3})_r_(FieldX:[0-9]{4})_c_(FieldY:[0-9]{4})_t_(TimePoint:[0-9]{8}_z_(Zpos:[0-9]{4}).tif",
//        "PlateName":"StanfordTest"
//    }

    if (!doc.contains("image_matcher"))
    {
        _error += "Image Matcher not found in metadata, cannot load content";
        return nullptr;
    }

    ExperimentFileModel* r = new ExperimentFileModel();


    if (doc["image_matcher"].isObject())
    {

		auto obj = doc["image_matcher"].toObject();
		for (auto objkey : obj.keys())
		{

            QString pattern = obj[objkey].toString();

            QStringList path = _file.replace("\\", "/").split("/");
            path.pop_back();
            QDir base(path.join("/"));

            if (doc.contains("subfolder"))
            {

                auto arr = doc["subfolder"].toArray();
                for (auto sub : arr)
                {
                    base.cd(sub.toString());
                    parseFolders(base.canonicalPath(), pattern, r, objkey);
                    base.cdUp();
                }

            }
            else
            {
                auto entries = base.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (auto& sub : entries)
                {
                    base.cd(sub);
                    parseFolders(base.canonicalPath(), pattern, r, objkey);
                    base.cdUp();
                }
            }
		}

    }
    else
    {

        QString pattern = doc["image_matcher"].toString();

        QStringList path = _file.replace("\\", "/").split("/");
        path.pop_back();
        QDir base(path.join("/"));

        if (doc.contains("subfolder"))
        {

            auto arr = doc["subfolder"].toArray();
            for (auto sub : arr)
            {
                base.cd(sub.toString());
                parseFolders(base.canonicalPath(), pattern, r);
                base.cdUp();
            }

        }
        else
        {
            auto entries = base.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (auto& sub : entries)
            {
                base.cd(sub);
                parseFolders(base.canonicalPath(), pattern, r);
                base.cdUp();
            }
        }
    }
    r->setProperties("RowCount", QString::number(nrows + 1));
    r->setProperties("ColumnCount", QString::number(ncols + 1));

    if (doc.contains("PlateName"))
    {
        r->setGroupName(doc["PlateName"].toString());
        r->setName(doc["PlateName"].toString());
    }

    if (doc.contains("GroupName"))
        r->setGroupName(doc["PlateName"].toString());


    for (auto key: doc.keys())
        if (!skipMeta.contains(key))
        {
            r->setProperties(key, doc[key].toString());
        }
    if (doc.contains("Channels"))
    {
        QStringList chans;
        for (auto v : doc["Channels"].toArray().toVariantList())
            chans << v.toString();
        r->setChannelNames(chans);
    }

    return r;
}

QString GenericLoader::errorString()
{
    return _error;
}


GenericLoader::GenericLoader(): CheckoutDataLoaderPluginInterface()
{

}

GenericLoader::~GenericLoader()
{

}

QString GenericLoader::pluginName()
{
    return "Generic Loader (Manual Metadata) v0.1";
}

QStringList GenericLoader::handledFiles()
{
    return QStringList() << "Metadata*.json";
}

bool GenericLoader::isFileHandled(QString file)
{
    
    if (file.endsWith(".json") && file.split("/").last().startsWith("Metadata"))
        return true;
    else
        return false;
}
