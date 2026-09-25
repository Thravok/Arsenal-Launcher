#include <QTest>

#include <ui/widgets/InfoFrame.h>

class ColorCodesTest : public QObject {
    Q_OBJECT
   private slots:
    void test_plainTextIsWrapped() { QCOMPARE(InfoFrame::renderColorCodes(QStringLiteral("hello")), QStringLiteral("<html>hello</html>")); }

    void test_colorAndFormattingCodes()
    {
        QCOMPARE(InfoFrame::renderColorCodes(QStringLiteral("§cRed")),
                 QStringLiteral("<html><span style=\"color: #FF5555;\">Red</span></html>"));
        QCOMPARE(InfoFrame::renderColorCodes(QStringLiteral("§lBold")), QStringLiteral("<html><b>Bold</b></html>"));
        QCOMPARE(InfoFrame::renderColorCodes(QStringLiteral("§nUnder")), QStringLiteral("<html><u>Under</u></html>"));
        QCOMPARE(InfoFrame::renderColorCodes(QStringLiteral("§oItalic")), QStringLiteral("<html><i>Italic</i></html>"));
        QCOMPARE(InfoFrame::renderColorCodes(QStringLiteral("§mStrike")), QStringLiteral("<html><s>Strike</s></html>"));
    }

    void test_resetClosesOpenTags()
    {
        QCOMPARE(InfoFrame::renderColorCodes(QStringLiteral("§cRed§rNormal")),
                 QStringLiteral("<html><span style=\"color: #FF5555;\">Red</span>Normal</html>"));
    }

    void test_unknownCodesAndNewlines()
    {
        QCOMPARE(InfoFrame::renderColorCodes(QStringLiteral("§zUnknown")), QStringLiteral("<html>§zUnknown</html>"));
        QCOMPARE(InfoFrame::renderColorCodes(QStringLiteral("line\nbreak")), QStringLiteral("<html>line<br>break</html>"));
        QCOMPARE(InfoFrame::renderColorCodes(QStringLiteral("end§")), QStringLiteral("<html>end§</html>"));
    }
};

QTEST_GUILESS_MAIN(ColorCodesTest)
#include "ColorCodes_test.moc"
