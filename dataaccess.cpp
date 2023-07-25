#include "dataaccess.h"


#define NORMAL 0
#define DATACHUNK_SIZE 0x1000

#define READ_DATACHUNK_MASK Q_INT64_C(0xfffffffffffff000) // round off to 4096 in decimal form (2^12)

// File systems and storage devices allocation units are multiples 64KB
#define BUFFER_SIZE 0x10000     // 64KB


DataAccess::DataAccess(QObject *parent): QObject(parent)
{
    QBuffer *buf = new QBuffer(this);
    setIODevice(*buf);
}


DataAccess::DataAccess(QIODevice &ioDevice, QObject *parent): QObject(parent)
{
    setIODevice(ioDevice);
}


bool DataAccess::setIODevice(QIODevice &ioDevice)
{
    _ioDevice = &ioDevice;
    bool ok = _ioDevice->open(QIODevice::ReadOnly);
    if (ok)   // Try to open IODevice
    {
        _size = _ioDevice->size();
        _ioDevice->close();
    }
    else                                        // Fallback is an empty buffer
    {
        QBuffer *buf = new QBuffer(this);
        _ioDevice = buf;
        _size = 0;
    }
    _dataChunks.clear();
    _pos = 0;
    return ok;
}

// ***************************************** Getting data out of Chunks

QByteArray DataAccess::getData(qint64 pos, qint64 maxSize, QByteArray *highlighted)
{
    qint64 ioDelta = 0;
    int dataChunkIdx = 0;

    DataChunk dataChunk;
    QByteArray buffer;

    // Do some checks and some arrangements
    if (highlighted)
        highlighted->clear();

    if (pos >= _size)
        return buffer;

    if (maxSize < 0)
        maxSize = _size;
    else
        if ((pos + maxSize) > _size)
            maxSize = _size - pos;

    _ioDevice->open(QIODevice::ReadOnly);

    while (maxSize > 0)
    {
        dataChunk.absPos = LLONG_MAX;
        bool dataChunksLoopOngoing = true;
        while ((dataChunkIdx < _dataChunks.count()) && dataChunksLoopOngoing)
        {
            // In this section, we track changes before our required data and
            // we take the editdet data, if availible. ioDelta is a difference
            // counter to justify the read pointer to the original data, if
            // data in between was deleted or inserted.

            dataChunk = _dataChunks[dataChunkIdx];
            if (dataChunk.absPos > pos)
                dataChunksLoopOngoing = false;
            else
            {
                dataChunkIdx += 1;
                qint64 count;
                qint64 dataChunkOffset = pos - dataChunk.absPos;
                if (maxSize > ((qint64)dataChunk.data.size() - dataChunkOffset))
                {
                    count = (qint64)dataChunk.data.size() - dataChunkOffset;
                    ioDelta += DATACHUNK_SIZE - dataChunk.data.size();
                }
                else
                    count = maxSize;
                if (count > 0)
                {
                    buffer += dataChunk.data.mid(dataChunkOffset, (int)count);
                    maxSize -= count;
                    pos += count;
                    if (highlighted)
                        *highlighted += dataChunk.dataChanged.mid(dataChunkOffset, (int)count);
                }
            }
        }

        if ((maxSize > 0) && (pos < dataChunk.absPos))
        {
            // In this section, we read data from the original source. This only will
            // happen, whe no copied data is available

            qint64 byteCount;
            QByteArray readBuffer;
            if ((dataChunk.absPos - pos) > maxSize)
                byteCount = maxSize;
            else
                byteCount = dataChunk.absPos - pos;

            maxSize -= byteCount;
            _ioDevice->seek(pos + ioDelta);
            readBuffer = _ioDevice->read(byteCount);
            buffer += readBuffer;
            if (highlighted)
                *highlighted += QByteArray(readBuffer.size(), NORMAL);
            pos += readBuffer.size();
        }
    }
    _ioDevice->close();
    return buffer;
}


bool DataAccess::write(QIODevice &iODevice, qint64 pos, qint64 count)
{
    if (count == -1)
        count = _size;
    bool ok = iODevice.open(QIODevice::WriteOnly);
    if (ok)
    {
        for (qint64 idx=pos; idx < count; idx += BUFFER_SIZE)
        {
            QByteArray ba = getData(idx, BUFFER_SIZE);
            iODevice.write(ba);
        }
        iODevice.close();
    }
    return ok;
}


// ***************************************** Set and get highlighting infos

void DataAccess::setDataChanged(qint64 pos, bool dataChanged)
{
    if ((pos < 0) || (pos >= _size))
        return;
    int dataChunkIdx = getChunkIndex(pos);
    qint64 posInBa = pos - _dataChunks[dataChunkIdx].absPos;
    _dataChunks[dataChunkIdx].dataChanged[(int)posInBa] = char(dataChanged);
}

bool DataAccess::dataChanged(qint64 pos)
{
    QByteArray highlighted;
    getData(pos, 1, &highlighted);
    return bool(highlighted.at(0));
}


// ***************************************** Char manipulations

bool DataAccess::insert(qint64 pos, char b)
{
    if ((pos < 0) || (pos > _size))
        return false;
    int dataChunkIdx;
    if (pos == _size)
        dataChunkIdx = getChunkIndex(pos-1);
    else
        dataChunkIdx = getChunkIndex(pos);
    qint64 posInBa = pos - _dataChunks[dataChunkIdx].absPos;
    _dataChunks[dataChunkIdx].data.insert(posInBa, b);
    _dataChunks[dataChunkIdx].dataChanged.insert(posInBa, char(1));
    for (int idx=dataChunkIdx+1; idx < _dataChunks.size(); idx++)
        _dataChunks[idx].absPos += 1;
    _size += 1;
    _pos = pos;
    return true;
}

bool DataAccess::overwrite(qint64 pos, char b)
{
    if ((pos < 0) || (pos >= _size))
        return false;
    int dataChunkIdx = getChunkIndex(pos);
    qint64 posInBa = pos - _dataChunks[dataChunkIdx].absPos;
    _dataChunks[dataChunkIdx].data[(int)posInBa] = b;
    _dataChunks[dataChunkIdx].dataChanged[(int)posInBa] = char(1);
    _pos = pos;
    return true;
}

bool DataAccess::removeAt(qint64 pos)
{
    if ((pos < 0) || (pos >= _size))
        return false;
    int dataChunkIdx = getChunkIndex(pos);
    qint64 posInBa = pos - _dataChunks[dataChunkIdx].absPos;
    _dataChunks[dataChunkIdx].data.remove(posInBa, 1);
    _dataChunks[dataChunkIdx].dataChanged.remove(posInBa, 1);
    for (int idx=dataChunkIdx+1; idx < _dataChunks.size(); idx++)
        _dataChunks[idx].absPos -= 1;
    _size -= 1;
    _pos = pos;
    return true;
}


// ***************************************** Utility functions

char DataAccess::operator[](qint64 pos)
{
    return getData(pos, 1).at(0);
}

qint64 DataAccess::pos()
{
    return _pos;
}

qint64 DataAccess::size()
{
    return _size;
}

int DataAccess::getChunkIndex(qint64 absPos)
{
    // This routine checks, if there is already a copied chunk available. If os, it
    // returns a reference to it. If there is no copied chunk available, original
    // data will be copied into a new chunk.

    int foundIdx = -1;
    int insertIdx = 0;
    qint64 ioDelta = 0;


    for (int idx=0; idx < _dataChunks.size(); idx++)
    {
        DataChunk dataChunk = _dataChunks[idx];
        if ((absPos >= dataChunk.absPos) && (absPos < (dataChunk.absPos + dataChunk.data.size())))
        {
            foundIdx = idx;
            break;
        }
        if (absPos < dataChunk.absPos)
        {
            insertIdx = idx;
            break;
        }
        ioDelta += dataChunk.data.size() - DATACHUNK_SIZE;
        insertIdx = idx + 1;
    }

    if (foundIdx == -1)
    {
        DataChunk newDataChunk;
        qint64 readAbsPos = absPos - ioDelta;
        qint64 readPos = (readAbsPos & READ_DATACHUNK_MASK);
        _ioDevice->open(QIODevice::ReadOnly);
        _ioDevice->seek(readPos);
        newDataChunk.data = _ioDevice->read(DATACHUNK_SIZE);
        _ioDevice->close();
        newDataChunk.absPos = absPos - (readAbsPos - readPos);
        newDataChunk.dataChanged = QByteArray(newDataChunk.data.size(), char(0));
        _dataChunks.insert(insertIdx, newDataChunk);
        foundIdx = insertIdx;
    }
    return foundIdx;
}
