#include <QMap>
#include <QProcessEnvironment>
#include <QTest>

#include <java/JavaUtils.h>

namespace {

class EnvRestore {
   public:
    explicit EnvRestore(const QStringList& keys)
    {
        for (const auto& key : keys) {
            const QByteArray name = key.toUtf8();
            m_keys.append(name);
            if (qEnvironmentVariableIsSet(name.constData()))
                m_saved.insert(key, qEnvironmentVariable(name.constData()));
        }
    }

    ~EnvRestore()
    {
        for (const auto& name : m_keys) {
            const QString key = QString::fromUtf8(name);
            if (m_saved.contains(key))
                qputenv(name.constData(), m_saved.value(key).toUtf8());
            else
                qunsetenv(name.constData());
        }
    }

   private:
    QList<QByteArray> m_keys;
    QMap<QString, QString> m_saved;
};

#ifdef Q_OS_WIN32
const QChar kDelim = QLatin1Char(';');
#else
const QChar kDelim = QLatin1Char(':');
#endif

}  // namespace

class CleanEnvTest : public QObject {
    Q_OBJECT
   private slots:
    void test_stripsDangerousJavaVariables()
    {
        const QStringList keys = { QStringLiteral("JAVA_HOME"),      QStringLiteral("JAVA_ARGS"),         QStringLiteral("CLASSPATH"),
                                   QStringLiteral("CONFIGPATH"),     QStringLiteral("JRE_HOME"),          QStringLiteral("_JAVA_OPTIONS"),
                                   QStringLiteral("JAVA_OPTIONS"),   QStringLiteral("JAVA_TOOL_OPTIONS"), QStringLiteral("LAUNCHER_FOO") };
        EnvRestore restore(keys);
        for (const auto& key : keys)
            qputenv(key.toUtf8().constData(), QByteArrayLiteral("/dangerous"));

        auto env = CleanEnviroment();
        for (const auto& key : keys)
            QVERIFY2(!env.contains(key), qPrintable(key));
    }

    void test_stripsLauncherPrefixedEntriesFromLibraryPath()
    {
        const QString pathKey = QStringLiteral("QT_PLUGIN_PATH");
        const QString launcherKey = QStringLiteral("LAUNCHER_QT_PLUGIN_PATH");
        EnvRestore restore({ pathKey, launcherKey });

        const QString original = QStringList{ QStringLiteral("/opt/keep"), QStringLiteral("/opt/drop"), QStringLiteral("/opt/also") }.join(
            kDelim);
        qputenv(pathKey.toUtf8().constData(), original.toUtf8());
        qputenv(launcherKey.toUtf8().constData(), QStringLiteral("/opt/drop").toUtf8());

        auto env = CleanEnviroment();
        QVERIFY(!env.contains(launcherKey));
        QCOMPARE(env.value(pathKey), QStringList{ QStringLiteral("/opt/keep"), QStringLiteral("/opt/also") }.join(kDelim));
    }

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
    void test_stripsIbusFromXmodifiers()
    {
        EnvRestore restore({ QStringLiteral("XMODIFIERS") });
        qputenv("XMODIFIERS", QByteArrayLiteral("@im=ibus"));

        auto env = CleanEnviroment();
        QCOMPARE(env.value(QStringLiteral("XMODIFIERS")), QString());
    }

    void test_alwaysExposesLdLibraryPath()
    {
        EnvRestore restore({ QStringLiteral("LD_LIBRARY_PATH"), QStringLiteral("LAUNCHER_LD_LIBRARY_PATH") });
        qunsetenv("LD_LIBRARY_PATH");
        qunsetenv("LAUNCHER_LD_LIBRARY_PATH");

        auto env = CleanEnviroment();
        QVERIFY(env.contains(QStringLiteral("LD_LIBRARY_PATH")));
    }
#endif
};

QTEST_GUILESS_MAIN(CleanEnvTest)

#include "CleanEnv_test.moc"
