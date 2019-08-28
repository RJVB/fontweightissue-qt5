/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QApplication>
#include <QCommandLineParser>
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>
#include <QSettings>
#include <QDebug>
#include <QString>
#include <QStringList>
#include <QSet>
#include <QDebug>

#include "dialog.h"

class QFontStyleSet : public QSet<QString>
{
public:
    inline QFontStyleSet()
        : QSet<QString>()
    { 
        checkList.clear();
    }
    inline QFontStyleSet(const QFontStyleSet &ref)
        : QSet<QString>(ref)
    {
        checkList = ref.checkList;
    }
    inline QFontStyleSet(const QSet<QString> &ref)
        : QSet<QString>(ref)
    {
        checkList = toList();
    }
    static inline QFontStyleSet fromList(QStringList &list)
    {
        QFontStyleSet newSet(QSet<QString>::fromList(list));
        newSet.checkList = list;
        return newSet;
    }
    inline bool contains(const QString &stylePattern, bool exact) const
    {
//         lastCheckList = checkList;
        if (exact) {
            // QFontStyleSet should inherit contains(const QString &) so why
            // do I have to call the parent class method explicitly here?
            return QSet<QString>::contains(stylePattern);
        } else {
            foreach (const QString &pattern, checkList) {
                if (stylePattern.contains(pattern)) {
                    return true;
                }
            }
        }
        return false;
    }
    inline QFontStyleSet &operator<<(const QString &value)
    {
        insert(value);
        checkList << value;
        return *this;
    }
    inline QStringList list() const
    {
        return checkList;
    }
//     static inline QStringList lastList()
//     {
//         return lastCheckList;
//     }
private:
    QStringList checkList;
//     static QStringList lastCheckList;
};

static inline bool qstringCompareToList(const QString &style, const QStringList &checkList, bool exact, Qt::CaseSensitivity mode = Qt::CaseInsensitive)
{
    if (exact) {
        foreach (const QString &pattern, checkList) {
            if (style.compare(pattern, mode) == 0) {
                return true;
            }
        }
    } else {
        foreach (const QString &pattern, checkList) {
            if (style.contains(pattern, mode)) {
                return true;
            }
        }
    }
    return false;
}

void doSomethingWithQFont(QFont &fnt)
{
    fnt.setBold(true);
}

#include "timing.c"

bool doBenchmark = false;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QSettings::setDefaultFormat(QSettings::IniFormat);

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption benchmark(QStringLiteral("benchmark"), QStringLiteral("measure timings for certain operations"));
    parser.addOption(benchmark);
    parser.process(app);

    doBenchmark = parser.isSet(benchmark);

#ifndef QT_NO_TRANSLATION
    QString translatorFileName = QLatin1String("qt_");
    translatorFileName += QLocale::system().name();
    QTranslator *translator = new QTranslator(&app);
    if (translator->load(translatorFileName, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        app.installTranslator(translator);
#endif
    QFontStyleSet demiBoldStyles;
    demiBoldStyles << QCoreApplication::translate("QFontDatabase", "DemiBold").toLower()
                << QCoreApplication::translate("QFontDatabase", "Demi Bold").toLower()
                << QCoreApplication::translate("QFontDatabase", "SemiBold").toLower()
                << QCoreApplication::translate("QFontDatabase", "Semi Bold").toLower();
    QFontStyleSet blackStyles;
    blackStyles << QCoreApplication::translate("QFontDatabase", "Black").toLower()
                << QCoreApplication::translate("QFontDatabase", "Ultra").toLower()
                << QCoreApplication::translate("QFontDatabase", "Heavy").toLower()
                << QCoreApplication::translate("QFontDatabase", "UltraBold").toLower();
    QStringList blackStyleList = blackStyles.list();
    QString pattern = "black", compareTo = "heavy";
//     qDebug() << "pattern=" << pattern << "matches compareTo=" << compareTo
//         << " in list" << blackStyles << "(" << blackStyleList << "):";
//     qDebug() << "qstringCompareToList:" << qstringCompareToList(pattern, blackStyles.list(), true) << " vs "
//         << qstringCompareToList(compareTo, blackStyles.list(), true);
//     qDebug() << "QFontStyleSet:" << blackStyles.contains(pattern, false) << " vs "
//         << blackStyles.contains(compareTo, true);

    if (doBenchmark) {
        const int N = 1000000;
        init_HRTime();
        bool found = true, exact = true;
        HRTime_tic();
        for( int i = 0 ; i < N && found; ++i ){
            pattern = blackStyleList[i % blackStyleList.size()];
        }
        double overhead = HRTime_toc();
        for( int j = 0 ; j < 2 ; ++j ){
            HRTime_tic();
            for( int i = 0 ; i < N && found; ++i ){
                pattern = blackStyleList[i % blackStyleList.size()];
                found =  qstringCompareToList(pattern, blackStyleList, exact) && qstringCompareToList(compareTo, blackStyleList, exact);
            }
            qInfo() << N << " times qstringCompareToList in " << HRTime_toc() - overhead << " seconds; exact=" << exact;
            HRTime_tic();
            for( int i = 0 ; i < N && found; ++i ){
                pattern = blackStyleList[i % blackStyleList.size()];
                found = blackStyles.contains(pattern, exact) && blackStyles.contains(compareTo, exact);
            }
            qInfo() << N << " times QFontStyleSet::contains in " << HRTime_toc() - overhead << " seconds; exact=" << exact;

            exact = false;
        }
    }

    // to match the default Info.plist that qmake creates:
    app.setOrganizationName("yourcompany");
    Dialog dialog;
    dialog.show();

    return app.exec();
}
