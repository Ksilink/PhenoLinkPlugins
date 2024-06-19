#include "PhenoLinkArrayScan.h"

#include <QDir>

PhenoLinkArrayScan::PhenoLinkArrayScan()
{

}

PhenoLinkArrayScan::~PhenoLinkArrayScan()
{

}

QString PhenoLinkArrayScan::pluginName()
{
    return "Array Scan file loader v0.a";
}


ExperimentFileModel *PhenoLinkArrayScan::getExperimentModel(QString _file)
{
    _error = QString();
    ExperimentFileModel* r = new ExperimentFileModel();

    QStringList sfile = _file.split("/"); if (sfile.size() == 1) sfile = _file.split("\\");  sfile.pop_back();
    r->setGroupName(sfile.at(std::max(0, (int)(sfile.count()-2))));
    r->setName(sfile.back());
    r->addMetadataFile(_file);

    QDir dir(_file); dir.cdUp();

    QStringList entries = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);

    int rCount = 0, cCount = 0, nchans=0;
    for (auto entry: entries)
    {
        qDebug() << "Entry" << entry;
        dir.cd(entry);
        QStringList files = dir.entryList(QStringList() << "*.tif", QDir::Files);

        for (auto& file: files)
        {
            QString wellpos = file.mid(entry.size()+1, 3);
            int field = file.mid(entry.size()+5, 2).toInt();
            int channel = file.mid(entry.size()+8, 1).toInt()+1;
            nchans = nchans < channel ? channel : nchans;

            int row = (wellpos.at(0).toLatin1()-'A'),
                col = wellpos.mid(1).toInt()-1;

            if (rCount < row) rCount = row;
            if (cCount < col) cCount = col;
//            qDebug() << "Image at" << wellpos << field << file.mid(entry.size()+8, 1) << channel <<;

             SequenceFileModel& seq = (*r)(row, col);
            seq.setOwner(r);
            r->setMeasurements(QPoint(row, col), true);
            qDebug() << "adding" << field << channel <<  row << col << dir.absolutePath()+file;
            seq.addFile(1, field, 1, channel, dir.absolutePath()+"/"+file);
            seq.checkValidity();

        }
        r->setProperties("RowCount", QString("%1").arg(rCount+1));
        r->setProperties("ColumnCount", QString("%1").arg(cCount+1));

    }

    QStringList chans;
    for (int i = 0; i < nchans-1; ++i)
        chans << QString("Channel %1").arg(i);
     r->setChannelNames(chans);

    return r;
}

QStringList PhenoLinkArrayScan::handledFiles()
{

    return QStringList() << "arrayscan.html";
}

bool PhenoLinkArrayScan::isFileHandled(QString file)
{
    if (file.endsWith("arrayscan.html"))
        return true;
    else
        return false;
}


QString PhenoLinkArrayScan::errorString()
{
  return _error;
}
