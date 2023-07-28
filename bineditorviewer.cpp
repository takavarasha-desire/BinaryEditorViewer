#include "bineditorviewer.h"

#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>


// ********************************************************************** Constructor, destructor

BinEditorViewer::BinEditorViewer(QWidget *parent) : QAbstractScrollArea(parent)
    , _addressArea(true)
    , _addressWidth(8)
    , _asciiArea(true)
    , _binArea(true)
    , _hexArea(true)
    , _bytesPerLine(12)
    , _hexCharsInLine(35)
    , _binCharsInLine(107)
    , _highlighting(true)
    , _overwriteMode(true)
    , _readOnly(false)
    , _editAreaIsAscii(false)
    , _editAreaIsBin(false)
    , _dataChunks(new DataAccess(this))
    , _cursorPosition(0)
    , _lastEventSize(0)
{
#ifdef Q_OS_WIN32
    setFont(QFont("Courier", 10));
#else
    setFont(QFont("Monospace", 10));
#endif

    QFontMetrics metrics = fontMetrics();
#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
    _pxCharWidth = metrics.horizontalAdvance(QLatin1Char('2'));
#else
    _pxCharWidth = metrics.width(QLatin1Char('2'));
#endif
    _pxCharHeight = metrics.height();
    _pxGapAdr = _pxCharWidth / 2;
    _pxGapAdrBin = _pxCharWidth;
    _pxGapBinHex = 2 * _pxCharWidth;
    _pxGapHexAscii = 2 * _pxCharWidth;
    _pxCursorWidth = _pxCharHeight / 7;
    _pxSelectionSub = _pxCharHeight / 5;


    connect(&_cursorTimer, SIGNAL(timeout()), this, SLOT(updateCursor()));
    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(adjust()));
    connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(adjust()));

    _cursorTimer.setInterval(500);
    _cursorTimer.start();

    setOverwriteMode(true);
    setReadOnly(false);

    init();
    setFocusPolicy(Qt::StrongFocus);

}

BinEditorViewer::~BinEditorViewer()
{
}

// ********************************************************************** Properties


bool BinEditorViewer::addressArea()
{
    return _addressArea;
}

qint64 BinEditorViewer::addressOffset()
{
    return _addressOffset;
}

void BinEditorViewer::setCursorPosition(std::size_t position)
{
    // 1. delete old cursor
    _cursorblink = false;
    viewport()->update(_cursorRect);

    // 2. Check if cursor in range and adjust if needed
    std::size_t maxSize = static_cast<std::size_t>(_dataChunks->size()) * 2 - 1;
    if (position > maxSize)
        position = maxSize;

    // No need to check for position < 0, since std::size_t is unsigned.

    // 3. Calc new position of cursor
    _bPosCurrent = position / 2;
    _pxCursorY = ((position / 2 - _bPosFirst) / _bytesPerLine + 1) * _pxCharHeight;
    int x = (position % (2 * _bytesPerLine));

    if (_editAreaIsAscii)
    {
        _pxCursorX = x / 2 * _pxCharWidth + _pxPosAsciiX;
        _cursorPosition = position & 0xFFFFFFFFFFFFFFFE; // to ensure cursor position always holds an even number
    }
    else if (_editAreaIsBin)
    {
        _pxCursorX = (((x / 8) * 9) + (x % 8)) * _pxCharWidth + _pxPosBinX;
        _cursorPosition = position;
    }
    else
    {
        _pxCursorX = (((x / 2) * 3) + (x % 2)) * _pxCharWidth + _pxPosHexX;
        _cursorPosition = position;
    }

    if (_overwriteMode)
        _cursorRect = QRect(_pxCursorX - horizontalScrollBar()->value(), _pxCursorY + _pxCursorWidth, _pxCharWidth, _pxCursorWidth);
    else
        _cursorRect = QRect(_pxCursorX - horizontalScrollBar()->value(), _pxCursorY - _pxCharHeight + 4, _pxCursorWidth, _pxCharHeight);

    // 4. Immediately draw new cursor
    _cursorblink = true;
    viewport()->update(_cursorRect);
    emit currentAddressChanged(_bPosCurrent);
}


std::size_t BinEditorViewer::cursorPosition(const QPoint &position)
{
    std::size_t pos = std::numeric_limits<std::size_t>::max();
    std::size_t posX = static_cast<std::size_t>(position.x()) + horizontalScrollBar()->value();
    std::size_t posY = static_cast<std::size_t>(position.y() - 3);

    if (posX >= static_cast<std::size_t>(_pxPosBinX) && posX < (static_cast<std::size_t>(_pxPosBinX) + static_cast<std::size_t>(_binCharsInLine) * _pxCharWidth))
    {
        std::size_t x = (posX - static_cast<std::size_t>(_pxPosBinX)) / _pxCharWidth;
        x = (x / 9) * 8 + (x % 9 == 0 ? 0 : 1);

        std::size_t firstLineIdx = static_cast<std::size_t>(verticalScrollBar()->value());
        std::size_t y = (posY / _pxCharHeight) * 2 * _bytesPerLine;
        pos = x + y + firstLineIdx * _bytesPerLine * 2;
    }
    else if (posX >= static_cast<std::size_t>(_pxPosHexX) && posX < (static_cast<std::size_t>(_pxPosHexX) + (static_cast<std::size_t>(_bytesPerLine) * 3 - 1) * _pxCharWidth))
    {
        std::size_t x = (posX - static_cast<std::size_t>(_pxPosHexX)) / _pxCharWidth;
        x = (x / 3) * 2 + (x % 3 == 0 ? 0 : 1);

        std::size_t firstLineIdx = static_cast<std::size_t>(verticalScrollBar()->value());
        std::size_t y = (posY / _pxCharHeight) * 2 * _bytesPerLine;
        pos = x + y + firstLineIdx * _bytesPerLine * 2;
    }
    else if (posX >= static_cast<std::size_t>(_pxPosAsciiX) && posX < (static_cast<std::size_t>(_pxPosAsciiX) + (static_cast<std::size_t>(_bytesPerLine) + 1) * _pxCharWidth))
    {
        std::size_t firstLineIdx = static_cast<std::size_t>(verticalScrollBar()->value());
        std::size_t x = 2 * (posX - static_cast<std::size_t>(_pxPosAsciiX)) / _pxCharWidth;
        std::size_t y = (posY / _pxCharHeight) * 2 * _bytesPerLine;
        pos = x + y + firstLineIdx * _bytesPerLine * 2;
    }

    return pos;
}


qint64 BinEditorViewer::cursorPosition()
{
    return _cursorPosition;
}

void BinEditorViewer::setData(const QByteArray &ba)
{
    _data = ba;
    _bData.setData(_data);
    setData(_bData);
}

QByteArray BinEditorViewer::data()
{
    return _dataChunks->getData(0, -1);
}

void BinEditorViewer::setOverwriteMode(bool overwriteMode)
{
    _overwriteMode = overwriteMode;
    emit overwriteModeChanged(overwriteMode);
}

bool BinEditorViewer::overwriteMode()
{
    return _overwriteMode;
}

bool BinEditorViewer::isReadOnly()
{
    return _readOnly;
}

void BinEditorViewer::setReadOnly(bool readOnly)
{
    _readOnly = readOnly;
}

// ********************************************************************** Access to data of qhexedit
bool BinEditorViewer::setData(QIODevice &iODevice)
{
    bool ok = _dataChunks->setIODevice(iODevice);
    init();
    dataChangedPrivate();
    return ok;
}

QByteArray BinEditorViewer::dataAt(qint64 pos, qint64 count)
{
    return _dataChunks->getData(pos, count);
}

bool BinEditorViewer::write(QIODevice &iODevice, qint64 pos, qint64 count)
{
    return _dataChunks->write(iODevice, pos, count);
}

// ********************************************************************** Char handling
void BinEditorViewer::insertChar(qint64 index, char ch)
{
    _dataChunks->insertChar(index, ch);
    refresh();
}

void BinEditorViewer::removeChar(qint64 index)
{
    _dataChunks->removeAt(index);
    refresh();
}

void BinEditorViewer::replaceChar(qint64 index, char ch)
{
    _dataChunks->overwriteChar(index, ch);
    refresh();
}

// ********************************************************************** Utility functions
void BinEditorViewer::ensureVisible()
{
    if (_cursorPosition < (_bPosFirst * 2))
        verticalScrollBar()->setValue((int)(_cursorPosition / 2 / _bytesPerLine));
    if (_cursorPosition > ((_bPosFirst + (_rowsShown - 1)*_bytesPerLine) * 2))
        verticalScrollBar()->setValue((int)(_cursorPosition / 2 / _bytesPerLine) - _rowsShown + 1);
    if (_pxCursorX < horizontalScrollBar()->value())
        horizontalScrollBar()->setValue(_pxCursorX);
    if ((_pxCursorX + _pxCharWidth) > (horizontalScrollBar()->value() + viewport()->width()))
        horizontalScrollBar()->setValue(_pxCursorX + _pxCharWidth - viewport()->width());
    viewport()->update();
}

bool BinEditorViewer::isModified()
{
    return _modified;
}

QString BinEditorViewer::selectedData()
{
    QByteArray ba = _dataChunks->getData(getSelectionBegin(), getSelectionEnd() - getSelectionBegin()).toHex();
    return ba;
}

// ********************************************************************** Handle events
void BinEditorViewer::keyPressEvent(QKeyEvent *event)
{
    // Cursor movements
    if (event->matches(QKeySequence::MoveToNextChar))
    {
        std::size_t pos = _cursorPosition + 1;
        if (_editAreaIsAscii)
            pos += 1;

        else if (_editAreaIsBin)
            pos = _cursorPosition + 8;

        setCursorPosition(pos);
        resetSelection(pos);
    }
    if (event->matches(QKeySequence::MoveToPreviousChar))
    {
        qint64 pos = _cursorPosition - 1;
        if (_editAreaIsAscii)
            pos -= 1;
        setCursorPosition(pos);
        resetSelection(pos);
    }



    if (event->matches(QKeySequence::MoveToEndOfLine))
    {
        qint64 pos = _cursorPosition - (_cursorPosition % (2 * _bytesPerLine)) + (2 * _bytesPerLine) - 1;
        setCursorPosition(pos);
        resetSelection(_cursorPosition);
    }
    if (event->matches(QKeySequence::MoveToStartOfLine))
    {
        qint64 pos = _cursorPosition - (_cursorPosition % (2 * _bytesPerLine));
        setCursorPosition(pos);
        resetSelection(_cursorPosition);
    }
    if (event->matches(QKeySequence::MoveToPreviousLine))
    {
        setCursorPosition(_cursorPosition - (2 * _bytesPerLine));
        resetSelection(_cursorPosition);
    }
    if (event->matches(QKeySequence::MoveToNextLine))
    {
        setCursorPosition(_cursorPosition + (2 * _bytesPerLine));
        resetSelection(_cursorPosition);
    }
    if (event->matches(QKeySequence::MoveToNextPage))
    {
        setCursorPosition(_cursorPosition + (((_rowsShown - 1) * 2 * _bytesPerLine)));
        resetSelection(_cursorPosition);
    }
    if (event->matches(QKeySequence::MoveToPreviousPage))
    {
        setCursorPosition(_cursorPosition - (((_rowsShown - 1) * 2 * _bytesPerLine)));
        resetSelection(_cursorPosition);
    }
    if (event->matches(QKeySequence::MoveToEndOfDocument))
    {
        setCursorPosition(_dataChunks->size() * 2 );
        resetSelection(_cursorPosition);
    }
    if (event->matches(QKeySequence::MoveToStartOfDocument))
    {
        setCursorPosition(0);
        resetSelection(_cursorPosition);
    }

    // Select commands
    if (event->matches(QKeySequence::SelectAll))
    {
        resetSelection(0);
        setSelection(2 * _dataChunks->size() + 1);
    }
    if (event->matches(QKeySequence::SelectNextChar))
    {
        qint64 pos = _cursorPosition + 1;
        if (_editAreaIsAscii)
            pos += 1;
        setCursorPosition(pos);
        setSelection(pos);
    }
    if (event->matches(QKeySequence::SelectPreviousChar))
    {
        qint64 pos = _cursorPosition - 1;
        if (_editAreaIsAscii)
            pos -= 1;
        setSelection(pos);
        setCursorPosition(pos);
    }
    if (event->matches(QKeySequence::SelectEndOfLine))
    {
        qint64 pos = _cursorPosition - (_cursorPosition % (2 * _bytesPerLine)) + (2 * _bytesPerLine) - 1;
        setCursorPosition(pos);
        setSelection(pos);
    }
    if (event->matches(QKeySequence::SelectStartOfLine))
    {
        qint64 pos = _cursorPosition - (_cursorPosition % (2 * _bytesPerLine));
        setCursorPosition(pos);
        setSelection(pos);
    }
    if (event->matches(QKeySequence::SelectPreviousLine))
    {
        qint64 pos = _cursorPosition - (2 * _bytesPerLine);
        setCursorPosition(pos);
        setSelection(pos);
    }
    if (event->matches(QKeySequence::SelectNextLine))
    {
        qint64 pos = _cursorPosition + (2 * _bytesPerLine);
        setCursorPosition(pos);
        setSelection(pos);
    }
    if (event->matches(QKeySequence::SelectNextPage))
    {
        qint64 pos = _cursorPosition + (((viewport()->height() / _pxCharHeight) - 1) * 2 * _bytesPerLine);
        setCursorPosition(pos);
        setSelection(pos);
    }
    if (event->matches(QKeySequence::SelectPreviousPage))
    {
        qint64 pos = _cursorPosition - (((viewport()->height() / _pxCharHeight) - 1) * 2 * _bytesPerLine);
        setCursorPosition(pos);
        setSelection(pos);
    }
    if (event->matches(QKeySequence::SelectEndOfDocument))
    {
        qint64 pos = _dataChunks->size() * 2;
        setCursorPosition(pos);
        setSelection(pos);
    }
    if (event->matches(QKeySequence::SelectStartOfDocument))
    {
        qint64 pos = 0;
        setCursorPosition(pos);
        setSelection(pos);
    }

    // Edit Commands
    if (!_readOnly)
    {

        /* Delete char */
        if (event->matches(QKeySequence::Delete))
        {

            if (_overwriteMode)
                replaceChar(_bPosCurrent, char(0));
            else
                removeChar(_bPosCurrent);

            setCursorPosition(2 * _bPosCurrent);
            resetSelection(2 * _bPosCurrent);
        } else

        /* Backspace */
        if ((event->key() == Qt::Key_Backspace) && (event->modifiers() == Qt::NoModifier))
        {
            if (getSelectionBegin() != getSelectionEnd())
            {
                _bPosCurrent = getSelectionBegin();
                setCursorPosition(2 * _bPosCurrent);

                if (_overwriteMode)
                    removeChar(_bPosCurrent);

                resetSelection(2 * _bPosCurrent);
            }
            else
            {
                bool behindLastByte = false;
                if ((_cursorPosition / 2) == _dataChunks->size())
                    behindLastByte = true;

                _bPosCurrent -= 1;
                if (_overwriteMode)
                    replaceChar(_bPosCurrent, char(0));
                else
                    removeChar(_bPosCurrent);

                if (!behindLastByte)
                    _bPosCurrent -= 1;

                setCursorPosition(2 * _bPosCurrent);
                resetSelection(2 * _bPosCurrent);
            }
        } else


        if ((QApplication::keyboardModifiers() == Qt::NoModifier) ||
            (QApplication::keyboardModifiers() == Qt::KeypadModifier) ||
            (QApplication::keyboardModifiers() == Qt::ShiftModifier) ||
            (QApplication::keyboardModifiers() == (Qt::AltModifier | Qt::ControlModifier)) ||
            (QApplication::keyboardModifiers() == Qt::GroupSwitchModifier))
        {
            // Bin Hex and Ascii input
            int key = 0;
            QString text = event->text();
            if (!text.isEmpty())
            {
                if (_editAreaIsAscii)
                    key = (uchar)text.at(0).toLatin1();
                else
                    key = int(text.at(0).toLower().toLatin1());
            }

            if (_editAreaIsBin)
            {
                // Binary input
                int key = 0;
                QString text = event->text();
                if (!text.isEmpty())
                {
                    // Ensure the input is a valid binary character ('0' or '1')
                    if (text == "0" || text == "1")
                    {
                        key = text.toInt();
                        int bitOffset = _cursorPosition % 8; // Calculate the bit offset within the byte

                        // If insert mode, then insert a byte
                        if (!_overwriteMode && bitOffset == 0)
                            insertChar(_bPosCurrent, char(0));

                        // Update the corresponding bit in the current byte
                        char currentValue = _dataShown.at(_bPosCurrent);
                        char newValue = (currentValue & ~(1 << (7 - bitOffset))) | (key << (7 - bitOffset)); // will check
                        replaceChar(_bPosCurrent, newValue);

                        setCursorPosition(_cursorPosition + 1);
                        resetSelection(_cursorPosition);
                    }
                }
            }


            if ((((key >= '0' && key <= '9') || (key >= 'a' && key <= 'f')) && _editAreaIsAscii == false)
                || (key >= ' ' && _editAreaIsAscii))
            {

                // If insert mode, then insert a byte
                if (_overwriteMode == false)
                    if ((_cursorPosition % 2) == 0)
                        insertChar(_bPosCurrent, char(0));

                // Change content
                if (_dataChunks->size() > 0)
                {
                    char ch = key;
                    if (!_editAreaIsAscii){
                        QByteArray hexValue = _dataChunks->getData(_bPosCurrent, 1).toHex();
                        if ((_cursorPosition % 2) == 0)
                            hexValue[0] = key;
                        else
                            hexValue[1] = key;
                        ch = QByteArray().fromHex(hexValue)[0];
                    }
                    replaceChar(_bPosCurrent, ch);
                    if (_editAreaIsAscii)
                        setCursorPosition(_cursorPosition + 2);
                    else
                        setCursorPosition(_cursorPosition + 1);
                    resetSelection(_cursorPosition);
                }
            }
        }


    }


    // Switch between insert/overwrite mode
    if ((event->key() == Qt::Key_Insert) && (event->modifiers() == Qt::NoModifier))
    {
        setOverwriteMode(!overwriteMode());
        setCursorPosition(_cursorPosition);
    }

    // switch from bin to hex edit
    if (event->key() == Qt::Key_Tab && _editAreaIsBin)
    {
        _editAreaIsHex = true;
        setCursorPosition(_cursorPosition);
    }

    // switch from hex to ascii edit
    if (event->key() == Qt::Key_Tab && _editAreaIsHex){
        _editAreaIsAscii = true;
        setCursorPosition(_cursorPosition);
    }

    // switch from ascii to hex edit
    if (event->key() == Qt::Key_Backtab  && _editAreaIsAscii){
        _editAreaIsAscii = false;
        setCursorPosition(_cursorPosition);
    }

    // switch from hex to bin edit
    if (event->key() == Qt::Key_Backtab && _editAreaIsHex)
    {
        _editAreaIsBin = true;
        setCursorPosition(_cursorPosition);
    }

    refresh();
    QAbstractScrollArea::keyPressEvent(event);
}

void BinEditorViewer::mouseMoveEvent(QMouseEvent * event)
{
    // 1. Delete old cursor
    _cursorblink = false;
    viewport()->update();

    // 2. Get the current position of the mouse cursor
    std::size_t actPos = cursorPosition(event->pos());

    // 3. Set the new cursor position and update the selection
    setCursorPosition(actPos);
    setSelection(actPos);
}


void BinEditorViewer::mousePressEvent(QMouseEvent * event)
{
    _cursorblink = false;

    std::size_t cPos = cursorPosition(event->pos());

    if((QApplication::keyboardModifiers() & Qt::ShiftModifier) && event -> button() == Qt::LeftButton)
        setSelection(cPos);
    else
        resetSelection(cPos);

    if (cPos != std::numeric_limits<std::size_t>::max())
    {
        if (event->button() != Qt::RightButton)
            resetSelection(cPos);
        setCursorPosition(cPos);
    }
    viewport()->update();
}

void BinEditorViewer::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    int pxOfsX = horizontalScrollBar()->value();

    if (event->rect() != _cursorRect)
    {
        int pxPosStartY = _pxCharHeight;

        // draw some patterns if needed
        painter.fillRect(event->rect(), viewport()->palette().color(QPalette::Base));
        if (_addressArea)
            painter.fillRect(QRect(-pxOfsX, event->rect().top(), _pxPosBinX - _pxGapAdrBin/2, height()), Qt::gray);


        if (_hexArea)
        {
            int linePos = _pxPosHexX - (_pxGapBinHex / 2);
            painter.setPen(Qt::gray);
            painter.drawLine(linePos - pxOfsX, event->rect().top(), linePos - pxOfsX, height());
        }

        if (_asciiArea)
        {
            int linePos = _pxPosAsciiX - (_pxGapHexAscii / 2);
            painter.setPen(Qt::gray);
            painter.drawLine(linePos - pxOfsX, event->rect().top(), linePos - pxOfsX, height());
        }

        painter.setPen(viewport()->palette().color(QPalette::WindowText));

        // paint address area
        if (_addressArea)
        {
            QString address;
            for (int row=0, pxPosY = _pxCharHeight; row <= (_dataShown.size()/_bytesPerLine); row++, pxPosY +=_pxCharHeight)
            {
                address = QString("%1").arg(_bPosFirst + row*_bytesPerLine + _addressOffset, _addrDigits, 16, QChar('0'));
                painter.setPen(QPen(Qt::black));
                painter.drawText(_pxPosAdrX - pxOfsX, pxPosY,address.toUpper());
            }
        }

        // paint bin, hex and ascii area
        //QPen colStandard = QPen(viewport()->palette().color(QPalette::WindowText));

        painter.setBackgroundMode(Qt::TransparentMode);

        for (int row = 0, pxPosY = pxPosStartY; row <= _rowsShown; row++, pxPosY +=_pxCharHeight)
        {
            QByteArray hex;

            int pxPosX = _pxPosHexX  - pxOfsX;
            int pxPosBinX2 = _pxPosBinX - pxOfsX;
            int pxPosAsciiX2 = _pxPosAsciiX - pxOfsX;
            qint64 bPosLine = row * _bytesPerLine;
            for (int colIdx = 0; ((bPosLine + colIdx) < _dataShown.size() && (colIdx < _bytesPerLine)); colIdx++)
            {
                QColor c = viewport()->palette().color(QPalette::Base);
                painter.setPen(QPen(Qt::black));

                qint64 posBa = _bPosFirst + bPosLine + colIdx;
                if ((getSelectionBegin() <= posBa) && (getSelectionEnd() > posBa))
                {
                    c = _brushSelection.color();
                    painter.setPen(_penSelection);
                }
                else
                {
                    if (_highlighting)
                        if (_markedShown.at((int)(posBa - _bPosFirst)))
                        {
                            c = _brushHighlighted.color();
                            painter.setPen(_penHighlighted);
                        }
                }

                // render hex value
                QRect r;
                if (colIdx == 0)
                    r.setRect(pxPosX, pxPosY - _pxCharHeight + _pxSelectionSub, 2*_pxCharWidth, _pxCharHeight);
                else
                    r.setRect(pxPosX - _pxCharWidth, pxPosY - _pxCharHeight + _pxSelectionSub, 3*_pxCharWidth, _pxCharHeight);
                painter.fillRect(r, c);
                hex = _hexDataShown.mid((bPosLine + colIdx) * 2, 2);
                painter.drawText(pxPosX, pxPosY, hex.toUpper());
                pxPosX += 3*_pxCharWidth;

                // render binary value

                if (_binArea)
                {
                    QRect rb;
                    uchar byte = (uchar)_dataShown.at(bPosLine + colIdx);


                    rb.setRect(pxPosBinX2, pxPosY - _pxCharHeight + _pxSelectionSub, 9*_pxCharWidth, _pxCharHeight);

                    painter.fillRect(rb, c);
                    QString bin = QString::number(byte, 2).rightJustified(8, '0'); // Convert byte to binary string
                    painter.drawText(pxPosBinX2, pxPosY, bin);
                    pxPosBinX2 += 9 * _pxCharWidth; // Move the next position for rendering
                }

                // render ascii value
                if (_asciiArea)
                {
                    QRect ra;

                    int ch = (uchar)_dataShown.at(bPosLine + colIdx);
                    if ( ch > '~' || ch < ' ' )
                        ch = '.';
                    ra.setRect(pxPosAsciiX2, pxPosY - _pxCharHeight + _pxSelectionSub, _pxCharWidth, _pxCharHeight);
                    painter.fillRect(ra, c);
                    painter.setPen(QPen(Qt::black));
                    painter.drawText(pxPosAsciiX2, pxPosY, QChar(ch));
                    pxPosAsciiX2 += _pxCharWidth;
                }


            }
        }
        painter.setBackgroundMode(Qt::TransparentMode);
        painter.setPen(viewport()->palette().color(QPalette::WindowText));
    }

    // _cursorPosition counts in 2, _bPosFirst counts in 1
    int hexPositionInShowData = _cursorPosition - 2 * _bPosFirst;

    // due to scrolling the cursor can go out of the currently displayed data
    if ((hexPositionInShowData >= 0) && (hexPositionInShowData < _hexDataShown.size()))
    {
        // paint cursor
        if (_readOnly)
        {
            QColor color = viewport()->palette().dark().color();
            painter.fillRect(QRect(_pxCursorX - pxOfsX, _pxCursorY - _pxCharHeight + _pxSelectionSub, _pxCharWidth, _pxCharHeight), color);
        }
        else
        {
            if (_cursorblink && hasFocus())
                painter.fillRect(_cursorRect, this->palette().color(QPalette::WindowText));
        }
            if (_editAreaIsAscii)
            {
                // every 2 hex there is 1 ascii
                int asciiPositionInShowData = hexPositionInShowData / 2;
                int ch = (uchar)_dataShown.at(asciiPositionInShowData);
                if (ch > '~' || ch < ' ')
                    ch = '.';

                painter.drawText(_pxCursorX - pxOfsX, _pxCursorY, QChar(ch));
            }
            else if(_editAreaIsBin)
            {
                int binPositionInShowData = hexPositionInShowData * 4;
                int ch = (uchar)_dataShown.at(binPositionInShowData);
                if ( ch != '0' && ch != '1')
                    ch = '0';

                QString bin = QString::number(ch - '0', 2).rightJustified(8, '0'); // Convert to binary string (ch - '0' to get the integer value of '0' or '1')

                painter.drawText(_pxCursorX - pxOfsX, _pxCursorY, bin);

            } else if(_editAreaIsHex)
            {
                painter.drawText(_pxCursorX - pxOfsX, _pxCursorY, _hexDataShown.mid(hexPositionInShowData, 1).toUpper());
            }
    }

    // emit event, if size has changed
    if (_lastEventSize != _dataChunks->size())
    {
        _lastEventSize = _dataChunks->size();
        emit currentSizeChanged(_lastEventSize);
    }
}

bool BinEditorViewer::focusNextPrevChild(bool next)
{

    if (_addressArea)
    {
        if ((next && _editAreaIsAscii) || (!next && !_editAreaIsAscii))
            return QWidget::focusNextPrevChild(next);
        else
        {
            if (_hexArea)
                _editAreaIsBin = true;
            else if (_binArea && !next)
                _editAreaIsBin = false;
            else if (_asciiArea && !next)
                _editAreaIsAscii = true;
            else if (_addressArea && !next)
            {
                _editAreaIsAscii = false;
                _editAreaIsBin = false;
            }
            return true;
        }
    }
    else if (_hexArea && _editAreaIsBin)
    {
        if (_binArea && !next)
            _editAreaIsBin = false;
        else if (_asciiArea && !next)
            _editAreaIsAscii = true;
        else if (_addressArea && !next)
        {
            _editAreaIsAscii = false;
            _editAreaIsBin = false;
        }
        return true;
    }
    else if (_binArea && !_editAreaIsBin)
    {
        if (_hexArea && !next)
            _editAreaIsBin = true;
        else if (_asciiArea && !next)
            _editAreaIsAscii = true;
        else if (_addressArea && !next)
        {
            _editAreaIsAscii = false;
            _editAreaIsBin = false;
        }
        return true;
    }
    else if (_asciiArea && _editAreaIsAscii && !next)
    {
        if (_binArea)
            _editAreaIsBin = true;
        else if (_hexArea)
            _editAreaIsAscii = false;
        else if (_addressArea)
        {
            _editAreaIsAscii = false;
            _editAreaIsBin = false;
        }
        return true;
    }

    return QWidget::focusNextPrevChild(next);

}

// ********************************************************************** Handle selections
void BinEditorViewer::resetSelection()
{
    _bSelectionBegin = _bSelectionInit;
    _bSelectionEnd = _bSelectionInit;
}

void BinEditorViewer::resetSelection(qint64 pos)
{
    if(_editAreaIsBin)
    {
        int bitOffset = pos % 8; // Calculate the bit offset within the byte
        pos /= 8; // Convert to byte index

        if (pos < 0)
            pos = 0;
        if (pos > _dataChunks->size())
            pos = _dataChunks->size();

        _bSelectionInit = pos * 8 + bitOffset;
        _bSelectionBegin = _bSelectionInit;
        _bSelectionEnd = _bSelectionInit;
    } else if (_editAreaIsHex)
    {
        //int hexCharsInLine = 2 * _bytesPerLine;
        pos = (pos - _bPosFirst) * 2;

        if (pos < 0)
            pos = 0;
        if (pos > _hexDataShown.size())
            pos = _hexDataShown.size();

        _bSelectionInit = pos;
        _bSelectionBegin = _bSelectionInit;
        _bSelectionEnd = _bSelectionInit;

    } else if (_editAreaIsAscii)
    {
        pos = (pos - _bPosFirst) * 2;

        if (pos < 0)
            pos = 0;
        if (pos > _dataShown.size())
            pos = _dataShown.size();

        _bSelectionInit = pos;
        _bSelectionBegin = _bSelectionInit;
        _bSelectionEnd = _bSelectionInit;

    }
}

void BinEditorViewer::setSelection(qint64 pos)
{
    if (_editAreaIsBin)
    {
        int bitOffset = pos % 8; // Calculate the bit offset within the byte
        pos /= 8; // Convert to byte index

        if (pos < 0)
            pos = 0;
        if (pos > _dataChunks->size())
            pos = _dataChunks->size();

        if (pos >= _bSelectionInit)
        {
            _bSelectionEnd = pos * 8 + bitOffset;
            _bSelectionBegin = _bSelectionInit;
        }
        else
        {
            _bSelectionBegin = pos * 8 + bitOffset;
            _bSelectionEnd = _bSelectionInit;
        }
    }
    else if (_editAreaIsHex)
    {
        //int hexCharsInLine = 2 * _bytesPerLine;
        pos = (pos - _bPosFirst) * 2;

        if (pos < 0)
            pos = 0;
        if (pos > _hexDataShown.size())
            pos = _hexDataShown.size();

        if (pos >= _bSelectionInit)
        {
            _bSelectionEnd = pos;
            _bSelectionBegin = _bSelectionInit;
        }
        else
        {
            _bSelectionBegin = pos;
            _bSelectionEnd = _bSelectionInit;
        }
    }
    else if (_editAreaIsAscii)
    {
        pos = (pos - _bPosFirst) * 2;

        if (pos < 0)
            pos = 0;
        if (pos > _dataShown.size())
            pos = _dataShown.size();

        if (pos >= _bSelectionInit)
        {
            _bSelectionEnd = pos;
            _bSelectionBegin = _bSelectionInit;
        }
        else
        {
            _bSelectionBegin = pos;
            _bSelectionEnd = _bSelectionInit;
        }
    }
}

qint64 BinEditorViewer::getSelectionBegin()
{
    return _bSelectionBegin;
}

qint64 BinEditorViewer::getSelectionEnd()
{
    return _bSelectionEnd;
}

// ********************************************************************** Private utility functions
void BinEditorViewer::init()
{

    resetSelection(0);
    setCursorPosition(0);
    verticalScrollBar()->setValue(0);
    _modified = false;
}

void BinEditorViewer::adjust()
{
    // recalculating Graphics

    _addrDigits = _addressWidth;
    _pxPosBinX = _pxGapAdr + _addrDigits*_pxCharWidth + _pxGapAdrBin;
    _pxPosHexX = _pxPosBinX + _binCharsInLine * _pxCharWidth + _pxGapBinHex;
    _pxPosAsciiX = _pxPosHexX + _hexCharsInLine * _pxCharWidth + _pxGapHexAscii;

    // setting horizontalScrollBar()
    int pxWidth = _pxPosAsciiX;
    if (_asciiArea)
        pxWidth += _bytesPerLine*_pxCharWidth;
    horizontalScrollBar()->setRange(0, pxWidth - viewport()->width());
    horizontalScrollBar()->setPageStep(viewport()->width());

    // setting verticalScrollbar()
    _rowsShown = ((viewport()->height()-4)/_pxCharHeight);
    int lineCount = (int)(_dataChunks->size() / (qint64)_bytesPerLine) + 1;
    verticalScrollBar()->setRange(0, lineCount - _rowsShown);
    verticalScrollBar()->setPageStep(_rowsShown);

    int value = verticalScrollBar()->value();
    _bPosFirst = (qint64)value * _bytesPerLine;
    _bPosLast = _bPosFirst + (qint64)(_rowsShown * _bytesPerLine) - 1;
    if (_bPosLast >= _dataChunks->size())
        _bPosLast = _dataChunks->size() - 1;
    readBuffers();
    setCursorPosition(_cursorPosition);
}

void BinEditorViewer::dataChangedPrivate(int)
{
    _modified = true;
    adjust();
    emit dataChanged();
}

void BinEditorViewer::refresh()
{
    ensureVisible();
    readBuffers();
}

void BinEditorViewer::readBuffers()
{
    _dataShown = _dataChunks->getData(_bPosFirst, _bPosLast - _bPosFirst + _bytesPerLine + 1, &_markedShown);
    _hexDataShown = QByteArray(_dataShown.toHex());
}

void BinEditorViewer::updateCursor()
{
    if (_cursorblink)
        _cursorblink = false;
    else
        _cursorblink = true;
    viewport()->update(_cursorRect);
}

