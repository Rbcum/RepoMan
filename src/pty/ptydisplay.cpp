#include "ptydisplay.h"

PtyDisplay::PtyDisplay(QPlainTextEdit *widget, QObject *parent)
    : QObject { parent }
    , textEdit(widget)
    , prevToken(-1)
{
    timer = new QTimer(this);
    timer->start(100);
    connect(timer, &QTimer::timeout, this, &PtyDisplay::onTimer);
    _codec = QTextCodec::codecForName("UTF-8");
    _decoder = _codec->makeDecoder();
    initTokenizer();
    resetTokenizer();
}

void PtyDisplay::flushContent()
{
    onTimer();
}

void PtyDisplay::onReceiveBlock(const char *buf, int len)
{
    /* XXX: the following code involves encoding & decoding of "UTF-16
     *
     * surrogate pairs", which does not work with characters higher than
     * U+10FFFF
     * https://unicodebook.readthedocs.io/unicode_encodings.html#surrogates
     */
    QString utf16Text = _decoder->toUnicode(buf, len);
    std::wstring unicodeText = utf16Text.toStdWString();

//    qDebug() << utf16Text;
    // send characters to terminal emulator
    for (size_t i = 0; i < unicodeText.length(); i++) {
        //        qDebug() << unicodeText[i] << QString::fromWCharArray(&unicodeText[i], 1);
        receiveChar(unicodeText[i]);
    }
}

void PtyDisplay::onTimer()
{
    if (!line.isEmpty()) {
        textEdit->moveCursor(QTextCursor::End);
        textEdit->insertPlainText(line);
        textEdit->moveCursor(QTextCursor::End);
        line.clear();
    }
}

#define COLOR_SPACE_UNDEFINED 0
#define COLOR_SPACE_DEFAULT 1
#define COLOR_SPACE_SYSTEM 2
#define COLOR_SPACE_256 3
#define COLOR_SPACE_RGB 4

#define TY_CONSTRUCT(T, A, N)                                                                      \
    (((((int)N) & 0xffff) << 16) | ((((int)A) & 0xff) << 8) | (((int)T) & 0xff))

#define TY_CHR() TY_CONSTRUCT(0, 0, 0)
#define TY_CTL(A) TY_CONSTRUCT(1, A, 0)
#define TY_ESC(A) TY_CONSTRUCT(2, A, 0)
#define TY_ESC_CS(A, B) TY_CONSTRUCT(3, A, B)
#define TY_ESC_DE(A) TY_CONSTRUCT(4, A, 0)
#define TY_CSI_PS(A, N) TY_CONSTRUCT(5, A, N)
#define TY_CSI_PN(A) TY_CONSTRUCT(6, A, 0)
#define TY_CSI_PR(A, N) TY_CONSTRUCT(7, A, N)
#define TY_CSI_PS_SP(A, N) TY_CONSTRUCT(11, A, N)

#define TY_VT52(A) TY_CONSTRUCT(8, A, 0)
#define TY_CSI_PG(A) TY_CONSTRUCT(9, A, 0)
#define TY_CSI_PE(A) TY_CONSTRUCT(10, A, 0)

#define MAX_ARGUMENT 4096

// Tokenizer --------------------------------------------------------------- --

/* The tokenizer's state

   The state is represented by the buffer (tokenBuffer, tokenBufferPos),
   and accompanied by decoded arguments kept in (argv,argc).
   Note that they are kept internal in the tokenizer.
*/

void PtyDisplay::resetTokenizer()
{
    tokenBufferPos = 0;
    argc = 0;
    argv[0] = 0;
    argv[1] = 0;
    prevCC = 0;
}

void PtyDisplay::addDigit(int digit)
{
    if (argv[argc] < MAX_ARGUMENT)
        argv[argc] = 10 * argv[argc] + digit;
}

void PtyDisplay::addArgument()
{
    argc = qMin(argc + 1, MAXARGS - 1);
    argv[argc] = 0;
}

void PtyDisplay::addToCurrentToken(wchar_t cc)
{
    tokenBuffer[tokenBufferPos] = cc;
    tokenBufferPos = qMin(tokenBufferPos + 1, MAX_TOKEN_LENGTH - 1);
}

// clang-format off

// Character Class flags used while decoding
#define CTL  1  // Control character
#define CHR  2  // Printable character
#define CPN  4  // TODO: Document me
#define DIG  8  // Digit
#define SCS 16  // TODO: Document me
#define GRP 32  // TODO: Document me
#define CPS 64  // Character which indicates end of window resize
                // escape sequence '\e[8;<row>;<col>t'

void PtyDisplay::initTokenizer()
{
  int i;
  quint8* s;
  for(i = 0;i < 256; ++i)
    charClass[i] = 0;
  for(i = 0;i < 32; ++i)
    charClass[i] |= CTL;
  for(i = 32;i < 256; ++i)
    charClass[i] |= CHR;
  for(s = (quint8*)"@ABCDEFGHILMPSTXZbcdfry"; *s; ++s)
    charClass[*s] |= CPN;
  // resize = \e[8;<row>;<col>t
  for(s = (quint8*)"t"; *s; ++s)
    charClass[*s] |= CPS;
  for(s = (quint8*)"0123456789"; *s; ++s)
    charClass[*s] |= DIG;
  for(s = (quint8*)"()+*%"; *s; ++s)
    charClass[*s] |= SCS;
  for(s = (quint8*)"()+*#[]%"; *s; ++s)
    charClass[*s] |= GRP;

  resetTokenizer();
}

/* Ok, here comes the nasty part of the decoder.

   Instead of keeping an explicit state, we deduce it from the
   token scanned so far. It is then immediately combined with
   the current character to form a scanning decision.

   This is done by the following defines.

   - P is the length of the token scanned so far.
   - L (often P-1) is the position on which contents we base a decision.
   - C is a character or a group of characters (taken from 'charClass').

   - 'cc' is the current character
   - 's' is a pointer to the start of the token buffer
   - 'p' is the current position within the token buffer

   Note that they need to applied in proper order.
*/

#define lec(P,L,C) (p == (P) && s[(L)] == (C))
#define lun(     ) (p ==  1  && cc >= 32 )
#define les(P,L,C) (p == (P) && s[L] < 256 && (charClass[s[(L)]] & (C)) == (C))
#define eec(C)     (p >=  3  && cc == (C))
#define ees(C)     (p >=  3  && cc < 256 && (charClass[cc] & (C)) == (C))
#define eps(C)     (p >=  3  && s[2] != '?' && s[2] != '!' && s[2] != '>' && cc < 256 && (charClass[cc] & (C)) == (C))
#define epp( )     (p >=  3  && s[2] == '?')
#define epe( )     (p >=  3  && s[2] == '!')
#define egt( )     (p >=  3  && s[2] == '>')
#define esp( )     (p ==  4  && s[3] == ' ')
#define Xpe        (tokenBufferPos >= 2 && tokenBuffer[1] == ']')
#define Xte        (Xpe      && (cc ==  7 || (prevCC == 27 && cc == 92) )) // 27, 92 => "\e\\" (ST, String Terminator)
#define ces(C)     (cc < 256 && (charClass[cc] & (C)) == (C) && !Xte)

#define CNTL(c) ((c)-'@')
#define ESC 27
#define DEL 127

// process an incoming unicode character
void PtyDisplay::receiveChar(wchar_t cc)
{
  if (cc == DEL)
    return; //VT100: ignore.

  if (ces(CTL))
  {
    // ignore control characters in the text part of Xpe (aka OSC) "ESC]"
    // escape sequences; this matches what XTERM docs say
    if (Xpe) {
        prevCC = cc;
        return;
    }

    // DEC HACK ALERT! Control Characters are allowed *within* esc sequences in VT100
    // This means, they do neither a resetTokenizer() nor a pushToToken(). Some of them, do
    // of course. Guess this originates from a weakly layered handling of the X-on
    // X-off protocol, which comes really below this level.
    if (cc == CNTL('X') || cc == CNTL('Z') || cc == ESC)
        resetTokenizer(); //VT100: CAN or SUB
    if (cc != ESC)
    {
        processToken(TY_CTL(cc+'@' ),0,0);
        return;
    }
  }
  // advance the state
  addToCurrentToken(cc);

  wchar_t* s = tokenBuffer;
  int  p = tokenBufferPos;

    if (lec(1,0,ESC)) { return; }
    if (lec(1,0,ESC+128)) { s[0] = ESC; receiveChar('['); return; }
    if (les(2,1,GRP)) { return; }
    if (Xte         ) { resetTokenizer(); return; }
    if (Xpe         ) { prevCC = cc; return; }
    if (lec(3,2,'?')) { return; }
    if (lec(3,2,'>')) { return; }
    if (lec(3,2,'!')) { return; }
    if (lun(       )) { processToken( TY_CHR(), cc, 0);   resetTokenizer(); return; }
    if (lec(2,0,ESC)) { processToken( TY_ESC(s[1]), 0, 0);              resetTokenizer(); return; }
    if (les(3,1,SCS)) { processToken( TY_ESC_CS(s[1],s[2]), 0, 0);      resetTokenizer(); return; }
    if (lec(3,1,'#')) { processToken( TY_ESC_DE(s[2]), 0, 0);           resetTokenizer(); return; }
    if (eps(    CPN)) { processToken( TY_CSI_PN(cc), argv[0],argv[1]);  resetTokenizer(); return; }
    if (esp(       )) { return; }
    if (lec(5, 4, 'q') && s[3] == ' ') {
      processToken( TY_CSI_PS_SP(cc, argv[0]), argv[0], 0);
      resetTokenizer();
      return;
    }

    // resize = \e[8;<row>;<col>t
    if (eps(CPS))
    {
        processToken( TY_CSI_PS(cc, argv[0]), argv[1], argv[2]);
        resetTokenizer();
        return;
    }

    if (epe(   )) { processToken( TY_CSI_PE(cc), 0, 0); resetTokenizer(); return; }
    if (ees(DIG)) { addDigit(cc-'0'); return; }
    if (eec(';') || eec(':')) { addArgument(); return; }
    for (int i=0;i<=argc;i++)
    {
        if (epp())
            processToken( TY_CSI_PR(cc,argv[i]), 0, 0);
        else if (egt())
            processToken( TY_CSI_PG(cc), 0, 0); // spec. case for ESC]>0c or ESC]>c
        else if (cc == 'm' && argc - i >= 4 && (argv[i] == 38 || argv[i] == 48) && argv[i+1] == 2)
        {
            // ESC[ ... 48;2;<red>;<green>;<blue> ... m -or- ESC[ ... 38;2;<red>;<green>;<blue> ... m
            i += 2;
            processToken( TY_CSI_PS(cc, argv[i-2]), COLOR_SPACE_RGB, (argv[i] << 16) | (argv[i+1] << 8) | argv[i+2]);
            i += 2;
        }
        else if (cc == 'm' && argc - i >= 2 && (argv[i] == 38 || argv[i] == 48) && argv[i+1] == 5)
        {
            // ESC[ ... 48;5;<index> ... m -or- ESC[ ... 38;5;<index> ... m
            i += 2;
            processToken( TY_CSI_PS(cc, argv[i-2]), COLOR_SPACE_256, argv[i]);
        }
        else
            processToken( TY_CSI_PS(cc,argv[i]), 0, 0);
    }
    resetTokenizer();

}

void PtyDisplay::processToken(int token, wchar_t p, int q)
{
    // Handle \r\n
    bool skipNewLine = false;
    if (prevToken == TY_CTL('M') && token == TY_CTL('J')) {
        skipNewLine = true;
    }
    prevToken = token;

    switch (token)
    {
        case TY_CHR(         ) :
            line.append(QChar(p));
            break; //UTF16
        case TY_CTL('J'      ) : //\n
        case TY_CTL('K'      ) :
        case TY_CTL('L'      ) :
        case TY_CTL('M'      ) : //\r
            if (skipNewLine) break;
            line.append("\n");
            break;
        case TY_CTL('@'      ) : /* NUL: ignored                      */ break;
        case TY_CTL('A'      ) : /* SOH: ignored                      */ break;
        case TY_CTL('B'      ) : /* STX: ignored                      */ break;
        case TY_CTL('C'      ) : /* ETX: ignored                      */ break;
        case TY_CTL('D'      ) : /* EOT: ignored                      */ break;
        case TY_CTL('E'      ) :      /*reportAnswerBack     (          )*/; break; //VT100
        case TY_CTL('F'      ) : /* ACK: ignored                      */ break;
        case TY_CTL('G'      ) : /*emit stateSet(NOTIFYBELL);*/         break; //VT100
        case TY_CTL('H'      ) : /*_currentScreen->backspace            (          );*/ break; //VT100
        case TY_CTL('I'      ) : line.append("\t");                     break; //VT100

        case TY_CTL('P'      ) : /* DLE: ignored                      */ break;
        case TY_CTL('Q'      ) : /* DC1: XON continue                 */ break; //VT100
        case TY_CTL('R'      ) : /* DC2: ignored                      */ break;
        case TY_CTL('S'      ) : /* DC3: XOFF halt                    */ break; //VT100
        case TY_CTL('T'      ) : /* DC4: ignored                      */ break;
        case TY_CTL('U'      ) : /* NAK: ignored                      */ break;
        case TY_CTL('V'      ) : /* SYN: ignored                      */ break;
        case TY_CTL('W'      ) : /* ETB: ignored                      */ break;
        case TY_CTL('Y'      ) : /* EM : ignored                      */ break;
        case TY_CTL('['      ) : /* ESC: cannot be seen here.         */ break;
        case TY_CTL('\\'     ) : /* FS : ignored                      */ break;
        case TY_CTL(']'      ) : /* GS : ignored                      */ break;
        case TY_CTL('^'      ) : /* RS : ignored                      */ break;
        case TY_CTL('_'      ) : /* US : ignored                      */ break;

        case TY_CSI_PR('h',   8) : /* IGNORED: autorepeat on            */ break; //VT100
        case TY_CSI_PR('l',   8) : /* IGNORED: autorepeat off           */ break; //VT100
        case TY_CSI_PR('s',   8) : /* IGNORED: autorepeat on            */ break; //VT100
        case TY_CSI_PR('r',   8) : /* IGNORED: autorepeat off           */ break; //VT100

        case TY_CSI_PR('h',   9) : /* IGNORED: interlace                */ break; //VT100
        case TY_CSI_PR('l',   9) : /* IGNORED: interlace                */ break; //VT100
        case TY_CSI_PR('s',   9) : /* IGNORED: interlace                */ break; //VT100
        case TY_CSI_PR('r',   9) : /* IGNORED: interlace                */ break; //VT100

        case TY_CSI_PR('h',  12) : /* IGNORED: Cursor blink             */ break; //att610
        case TY_CSI_PR('l',  12) : /* IGNORED: Cursor blink             */ break; //att610
        case TY_CSI_PR('s',  12) : /* IGNORED: Cursor blink             */ break; //att610
        case TY_CSI_PR('r',  12) : /* IGNORED: Cursor blink             */ break; //att610


        case TY_CSI_PR('h',  41) : /* IGNORED: obsolete more(1) fix     */ break; //XTERM
        case TY_CSI_PR('l',  41) : /* IGNORED: obsolete more(1) fix     */ break; //XTERM
        case TY_CSI_PR('s',  41) : /* IGNORED: obsolete more(1) fix     */ break; //XTERM
        case TY_CSI_PR('r',  41) : /* IGNORED: obsolete more(1) fix     */ break; //XTERM


        case TY_CSI_PR('h',  67) : /* IGNORED: DECBKM                   */ break; //XTERM
        case TY_CSI_PR('l',  67) : /* IGNORED: DECBKM                   */ break; //XTERM
        case TY_CSI_PR('s',  67) : /* IGNORED: DECBKM                   */ break; //XTERM
        case TY_CSI_PR('r',  67) : /* IGNORED: DECBKM                   */ break; //XTERM

        case TY_CSI_PR('h', 1034) : /* IGNORED: 8bitinput activation     */ break; //XTERM

        case TY_CSI_PR('h', 2004) :         break; //XTERM
        case TY_CSI_PR('l', 2004) :         break; //XTERM
        case TY_CSI_PR('s', 2004) :         break; //XTERM
        case TY_CSI_PR('r', 2004) :         break; //XTERM

        case TY_CSI_PS('K',   0) :  break;
        case TY_CSI_PS('K',   1) :  break;
        case TY_CSI_PS('K',   2) :  break;
        case TY_CSI_PS('J',   0) :  break;
        case TY_CSI_PS('J',   1) :  break;
        case TY_CSI_PS('J',   2) :  break;
        case TY_CSI_PS('J',      3) :     break;
        case TY_CSI_PS('g',   0) :  break; //VT100
        case TY_CSI_PS('g',   3) :  break; //VT100
        case TY_CSI_PS('h',   4) :  break;
        case TY_CSI_PS('h',  20) :  break;
        case TY_CSI_PS('i',   0) : /* IGNORE: attached printer          */ break; //VT100
        case TY_CSI_PS('l',   4) :  break;
        case TY_CSI_PS('l',  20) :  break;
        case TY_CSI_PS('s',   0) :  break;
        case TY_CSI_PS('u',   0) :  break;

        case TY_CSI_PS('m',   0) :  break;
        case TY_CSI_PS('m',   1) :  break; //VT100
        case TY_CSI_PS('m',   2) :  break;
        case TY_CSI_PS('m',   3) :  break; //VT100
        case TY_CSI_PS('m',   4) :  break; //VT100
        case TY_CSI_PS('m',   5) :  break; //VT100
        case TY_CSI_PS('m',   7) :  break;
        case TY_CSI_PS('m',   8) :  break;
        case TY_CSI_PS('m',   9) :  break;
        case TY_CSI_PS('m',  53) :  break;
        case TY_CSI_PS('m',  10) : /* IGNORED: mapping related          */ break; //LINUX
        case TY_CSI_PS('m',  11) : /* IGNORED: mapping related          */ break; //LINUX
        case TY_CSI_PS('m',  12) : /* IGNORED: mapping related          */ break; //LINUX
        case TY_CSI_PS('m',  21) :  break;
        case TY_CSI_PS('m',  22) :  break;
        case TY_CSI_PS('m',  23) :  break; //VT100
        case TY_CSI_PS('m',  24) :  break;
        case TY_CSI_PS('m',  25) :  break;
        case TY_CSI_PS('m',  27) :  break;
        case TY_CSI_PS('m',  28) :  break;
        case TY_CSI_PS('m',  29) :  break;
        case TY_CSI_PS('m',  55) :  break;

        case TY_CSI_PS('m',   30) :  break;
        case TY_CSI_PS('m',   31) :  break;
        case TY_CSI_PS('m',   32) :  break;
        case TY_CSI_PS('m',   33) :  break;
        case TY_CSI_PS('m',   34) :  break;
        case TY_CSI_PS('m',   35) :  break;
        case TY_CSI_PS('m',   36) :  break;
        case TY_CSI_PS('m',   37) :  break;

        case TY_CSI_PS('m',   38) :  break;

        case TY_CSI_PS('m',   39) :  break;

        case TY_CSI_PS('m',   40) :  break;
        case TY_CSI_PS('m',   41) :  break;
        case TY_CSI_PS('m',   42) :  break;
        case TY_CSI_PS('m',   43) :  break;
        case TY_CSI_PS('m',   44) :  break;
        case TY_CSI_PS('m',   45) :  break;
        case TY_CSI_PS('m',   46) :  break;
        case TY_CSI_PS('m',   47) :  break;

        case TY_CSI_PS('m',   48) :  break;

        case TY_CSI_PS('m',   49) :  break;

        case TY_CSI_PS('m',   90) :  break;
        case TY_CSI_PS('m',   91) :  break;
        case TY_CSI_PS('m',   92) :  break;
        case TY_CSI_PS('m',   93) :  break;
        case TY_CSI_PS('m',   94) :  break;
        case TY_CSI_PS('m',   95) :  break;
        case TY_CSI_PS('m',   96) :  break;
        case TY_CSI_PS('m',   97) :  break;

        case TY_CSI_PS('m',  100) :  break;
        case TY_CSI_PS('m',  101) :  break;
        case TY_CSI_PS('m',  102) :  break;
        case TY_CSI_PS('m',  103) :  break;
        case TY_CSI_PS('m',  104) :  break;
        case TY_CSI_PS('m',  105) :  break;
        case TY_CSI_PS('m',  106) :  break;
        case TY_CSI_PS('m',  107) :  break;

        default:
            reportDecodingError();
            break;
    };
}
// clang-format on

void PtyDisplay::reportDecodingError()
{
    if (tokenBufferPos == 0 || (tokenBufferPos == 1 && (tokenBuffer[0] & 0xff) >= 32))
        return;
    qDebug() << "Undecodable sequence:" << QString::fromWCharArray(tokenBuffer, tokenBufferPos);
}
