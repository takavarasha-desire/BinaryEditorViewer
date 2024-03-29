#ifndef BINEDITORVIEWER_H
#define BINEDITORVIEWER_H

#include "dataaccess.h"

#include <QAbstractScrollArea>
#include <QByteArray>
#include <QList>
#include <QRect>
#include <QTimer>
#include <QPen>
#include <QBrush>

class BinEditorViewer: public QAbstractScrollArea

{
    Q_OBJECT
public:

    BinEditorViewer(QWidget *parent = 0);
    ~BinEditorViewer();

    bool setData(QIODevice &dataDevice);

    QByteArray data();
    void setData(const QByteArray &ba);

    QByteArray dataAt(qint64 pos, qint64 count=-1);

    bool write(QIODevice &dataDevice, qint64 pos=0, qint64 count=-1);

    // char insert
    void insertChar(qint64 pos, char ch);

    // char remove
    void removeChar(qint64 pos);


    // overwrite char
    void replaceChar(qint64 pos, char ch);


    qint64 cursorPosition(QPoint pos);

    void ensureVisible();

    bool isModified();

    void setCursorPosition(std::size_t position);
    std::size_t cursorPosition();


    bool overwriteMode();
    void setOverwriteMode(bool overwriteMode);

signals:
    void currentAddressChanged(qint64 address);

    void currentSizeChanged(qint64 size);

    void dataChanged();

    void overwriteModeChanged(bool state);



protected:
    void paintEvent(QPaintEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    virtual bool focusNextPrevChild(bool next);

private slots:
    void adjust();                              // recalc pixel positions
    void dataChangedPrivate(int idx=0);         // emit dataChanged() signal
    void refresh();                             // ensureVisible() and readBuffers()
    void updateCursor();                        // update blinking cursor


private:
    // Handle selections
    void resetSelection(qint64 pos);            // set selectionStart and selectionEnd to pos
    void resetSelection();                      // set selectionEnd to selectionStart
    void setSelection(qint64 pos);              // set min (if below init) or max (if greater init)
    qint64 getSelectionBegin();
    qint64 getSelectionEnd();

    // Private utility functions
    void init();
    void readBuffers();


    // _px for pixel positions
        int _pxCharWidth, _pxCharHeight;            // char dimensions
        int _pxPosBinX;                             // X-Pos of Bin Area
        int _pxPosHexX;                             // X-Pos of Hex Area
        int _pxPosAdrX;                             // X-Pos of Address Area
        int _pxPosAsciiX;                           // X-Pos of Ascii Area
        int _pxGapAdr;                              // gap on the left of Address
        int _pxGapAdrBin;                           // gap between Adr and Bin Area
        int _pxGapBinHex;                           // gap between Bin and Hex Area
        int _pxGapHexAscii;                         // gap between Hex and Ascii Area
        int _pxCursorWidth;                         // cursor width
        int _pxSelectionSub;                        // offset selection rect
        int _pxCursorX;                             // current cursor pos
        int _pxCursorY;                             // current cursor pos

        // _b for absolute byte positions in _dataChunk
        qint64 _bSelectionBegin;
        qint64 _bSelectionEnd;
        qint64 _bSelectionInit;
        qint64 _bPosFirst;
        qint64 _bPosLast;
        qint64 _bPosCurrent;


        bool _addressArea;
        int _addressWidth;
        bool _asciiArea;
        bool _binArea;
        bool _hexArea;
        qint64 _addressOffset;
        int _bytesPerLine;
        int _hexCharsInLine;
        int _binCharsInLine;
        bool _overwriteMode;

        bool _readOnly;


        bool _editAreaIsAscii;
        bool _editAreaIsBin;
        bool _editAreaIsHex;
        int _addrDigits;
        bool _cursorblink;
        QBuffer _bData;
        DataAccess *_dataChunks;
        QTimer _cursorTimer;
        std::size_t _cursorPosition;
        QRect _cursorRect;
        QByteArray _data;
        QByteArray _dataShown;
        QByteArray _hexDataShown;
        qint64 _lastEventSize;
        QByteArray _markedShown;
        bool _modified;
        int _rowsShown;



};



#endif // BINEDITORVIEWER_H
