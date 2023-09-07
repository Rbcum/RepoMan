#ifndef PTYDISPLAY_H
#define PTYDISPLAY_H

#include <QObject>
#include <QPlainTextEdit>
#include <QTextCodec>
#include <QTimer>

class PtyDisplay : public QObject
{
    Q_OBJECT
public:
    explicit PtyDisplay(QPlainTextEdit *widget, QObject *parent = nullptr);
    void flushContent();

private:
    QPlainTextEdit *textEdit;
    QTimer *timer;

    const QTextCodec *_codec;
    QTextDecoder *_decoder;
    QString line;
    int prevToken;

    void receiveChar(wchar_t cc);
    char eraseChar() const;

    void initTokenizer();
    void resetTokenizer();
    void addToCurrentToken(wchar_t cc);
    void addDigit(int dig);
    void addArgument();

    void reportDecodingError();
    void processToken(int code, wchar_t p, int q);

#define MAX_TOKEN_LENGTH 256 // Max length of tokens (e.g. window title)
    wchar_t tokenBuffer[MAX_TOKEN_LENGTH]; // FIXME: overflow?
    int tokenBufferPos;
#define MAXARGS 15
    int argv[MAXARGS];
    int argc;
    int prevCC;

    // Set of flags for each of the ASCII characters which indicates
    // what category they fall into (printable character, control, digit etc.)
    // for the purposes of decoding terminal output
    int charClass[256];

private slots:
    void onTimer();

public slots:
    void onReceiveBlock(const char *buf, int len);
};

#endif // PTYDISPLAY_H
