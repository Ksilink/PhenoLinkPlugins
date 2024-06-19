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




void GenericLoader::parseFolders(QDir folder, QString pattern, ExperimentFileModel* efm)
{

    QStringList matchers = pattern.split("(", Qt::SkipEmptyParts);
    QString pattern_simpl = pattern;
    for (int i = 0; i < matchers.size(); ++i)
    {
        QString mat = matchers[i].split(":").front();
        matchers[i] = mat;
        pattern_simpl = pattern_simpl.replace(matchers[i]+":", "");
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
            auto match = exp.match(entry.fileName());
            if (match.hasMatch())
            {
                QStringList matches = match.capturedTexts();

                // Row & Col
//                "image_matcher": "(ChanName:.*)_(WellRow:[A-Z])_(WellCol:[0-9]{3})_r_(FieldX:[0-9]{4})_c_(FieldY:[0-9]{4})_t_(TimePoint:[0-9]{8})_z_(Zpos:[0-9]{4}).tif",
                int row = 0, col = 0;
                if (matchers.contains("Well"))
                {
                    int idx = matchers.indexOf("Well")+1;
                    QPoint pt = ExperimentDataTableModel::stringToPos(matches.at(idx));
                    row = pt.x();
                    col = pt.y();
                }
                else if (matchers.contains("WellRow") && matchers.contains("WellCol"))
                {

                    int idx = matchers.indexOf("WellRow")+1;

                    bool ok;
                    row  = matches[idx].toInt(&ok);
                    if (!ok)
                        row = (matches[idx].at(0).toLatin1() - 'A');

                    idx = matchers.indexOf("WellCol")+1;
                    col = matches[idx].toInt(&ok);
                }

                int timepoint = 1, zpos = 1;
                int fieldidx = 0, channel = 0;


                if (matchers.contains("TimePoint"))
                {
                    timepoint  = matches[matchers.indexOf("TimePoint")+1].toInt();
                }
                if (matchers.contains("Zpos"))
                {
                    zpos  = matches[matchers.indexOf("Zpos")+1].toInt()+1;
                }
                if (matchers.contains("Channel"))
                {
                    channel  = matches[matchers.indexOf("Channel")+1].toInt();
                }
                if (matchers.contains("Field"))
                {
                    fieldidx  = matches[matchers.indexOf("Field")+1].toInt();

                }
                else if (matchers.contains("FieldX") && matchers.contains("FieldY"))
                {
                    fieldidx = matches[matchers.indexOf("FieldX")+1].toInt() + 100 * matches[matchers.indexOf("FieldY")+1].toInt();
                }

                if (matchers.contains("ChanName"))
                {
                    QString cn = matches[matchers.indexOf("ChanName")+1];

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

    QString pattern = doc["image_matcher"].toString();

    QStringList path = _file.replace("\\", "/").split("/");
    path.pop_back();
    QDir base(path.join("/"));

    ExperimentFileModel* r = new ExperimentFileModel();

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
        for (auto& sub: entries)
        {
            base.cd(sub);
            parseFolders(base.canonicalPath(), pattern, r);
            base.cdUp();
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
