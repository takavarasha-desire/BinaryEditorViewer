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
    void removeChar(qint64 pos, qint64 len=1);


    // overwrite char
    void replaceChar(qint64 pos, char ch);

    // inserting byte array
    void insertByteArray(qint64 pos, const QByteArray &ba);

    // replace byte array
    void replaceByteArray(qint64 pos, qint64 len, const QByteArray &ba);

    qint64 cursorPosition(QPoint point);

    void ensureVisible();

    bool isModified();

    bool isReadOnly();
    void setReadOnly(bool readOnly);



    bool addressArea();
    qint64 addressOffset();

    void setCursorPosition(qint64 position);
    qint64 cursorPosition();

    QString selectedData();


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


    // Name convention: pixel positions start with _px
        int _pxCharWidth, _pxCharHeight;            // char dimensions (dependend on font)
        int _pxPosHexX;                             // X-Pos of HeaxArea
        int _pxPosAdrX;                             // X-Pos of Address Area
        int _pxPosAsciiX;                           // X-Pos of Ascii Area
        int _pxGapAdr;                              // gap left from AddressArea
        int _pxGapAdrHex;                           // gap between AddressArea and HexAerea
        int _pxGapHexAscii;                         // gap between HexArea and AsciiArea
        int _pxCursorWidth;                         // cursor width
        int _pxSelectionSub;                        // offset selection rect
        int _pxCursorX;                             // current cursor pos
        int _pxCursorY;                             // current cursor pos

        // Name convention: absolute byte positions in chunks start with _b
        qint64 _bSelectionBegin;                    // first position of Selection
        qint64 _bSelectionEnd;                      // end of Selection
        qint64 _bSelectionInit;                     // memory position of Selection
        qint64 _bPosFirst;                          // position of first byte shown
        qint64 _bPosLast;                           // position of last byte shown
        qint64 _bPosCurrent;                        // current position

        // variables to store the property values
        bool _addressArea;                          // left area of QHexEdit
        int _addressWidth;
        bool _asciiArea;
        qint64 _addressOffset;
        int _bytesPerLine;
        int _hexCharsInLine;
        bool _highlighting;
        bool _overwriteMode;
        QBrush _brushSelection;
        QPen _penSelection;
        QBrush _brushHighlighted;
        QPen _penHighlighted;
        bool _readOnly;

        // other variables
        bool _editAreaIsAscii;                      // flag about the ascii mode edited
        int _addrDigits;                            // real no of addressdigits, may be > addressWidth
        bool _blink;                                // help get cursor blinking
        QBuffer _bData;                             // buffer, when setup with QByteArray
        DataAccess *_dataChunks;                            // IODevice based access to data
        QTimer _cursorTimer;                        // for blinking cursor
        qint64 _cursorPosition;                     // absolute position of cursor, 1 Byte == 2 tics
        QRect _cursorRect;                          // physical dimensions of cursor
        QByteArray _data;                           // QHexEdit's data, when setup with QByteArray
        QByteArray _dataShown;                      // data in the current View
        QByteArray _hexDataShown;                   // data in view, transformed to hex
        qint64 _lastEventSize;                      // size, which was emitted last time
        QByteArray _markedShown;                    // marked data in view
        bool _modified;                             // Is any data in editor modified?
        int _rowsShown;                             // lines of text shown



};



#endif // BINEDITORVIEWER_H
