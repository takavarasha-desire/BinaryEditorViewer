#ifndef DATAACCESS_H
#define DATAACCESS_H

// for rounding off to the nearest 4096. (2^12),
// the standard page size used in memory management.

#include <QByteArray>
#include <QIODevice>
#include <QBuffer>


struct DataChunk
{
    QByteArray data;            // holds the original data
    QByteArray dataChanged;     // represent any changes made to the original data
    qint64 absPos;              // represent the absolute position of the DataChunk
};



class DataAccess: public QObject
{
    Q_OBJECT
public:
    DataAccess(QObject *parent);
    DataAccess(QIODevice &ioDevice, QObject *parent);
    bool setIODevice(QIODevice &ioDevice);

    // Getting the data though DataAccess
    QByteArray getData(qint64 pos=0, qint64 count=-1, QByteArray *highlighted=0);
    bool write(QIODevice &iODevice, qint64 pos=0, qint64 count=-1);

    // Set and get highlighting infos
    void setDataChanged(qint64 pos, bool dataChanged);
    bool dataChanged(qint64 pos);

    // Char manipulations
    bool insertChar(qint64 pos, char b);
    bool overwriteChar(qint64 pos, char b);
    bool removeAt(qint64 pos);

    // Utility functions
    char operator[](qint64 pos);
    qint64 pos();
    qint64 size();

private:
    int getChunkIndex(qint64 absPos);

    QIODevice * _ioDevice;
    qint64 _pos;
    qint64 _size;
    QList<DataChunk> _dataChunks;

};

#endif // DATAACCESS_H
