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

#include <QtGlobal>
#include <QtWidgets>
#include <QWindow>
#include <QApplication>

#if QT_VERSION >= QT_VERSION_CHECK(5,4,0)
#	include <QtGui/private/qfontengine_p.h>
#endif

#include "dialog.h"
#include "timing.h"

// #define QRAWFONT_FROM_DATA

#define MESSAGE \
    Dialog::tr("<p>Message boxes have a caption, a text, " \
               "and any number of buttons, each with standard or custom texts." \
               "<p>Click a button to close the message box. Pressing the Esc button " \
               "will activate the detected escape button (if any).")
#define MESSAGE_DETAILS \
    Dialog::tr("If a message box has detailed text, the user can reveal it " \
               "by pressing the Show Details... button.")

QFont stripStyleName(QFont &f, QFontDatabase &db)
{
    const QString &styleName = f.styleName();
    if (styleName.isEmpty()) {
        return f;
    } else {
        QFont g = (db.styleString(f) != styleName) ?
            db.font(f.family(), styleName, f.pointSize())
            : QFont(f.family(), f.pointSize(), f.weight());
        if (auto s = f.pixelSize() > 0) {
            g.setPixelSize(s);
        }
        g.setStyleHint(f.styleHint(), f.styleStrategy());
        g.setStyle(f.style());
        if (f.underline()) {
            g.setUnderline(true);
        }
        if (f.strikeOut()) {
            g.setStrikeOut(true);
        }
        if (f.fixedPitch()) {
            g.setFixedPitch(true);
        }
        return g;
    }
}

// pointless clone function just for benchmarking purposes
QFont clone(const QFont &f)
{
    QFont g(f);
    return g;
}

void benchmarkCloning(QFont &font)
{
    extern bool doBenchmark;

    if (!doBenchmark) {
        return;
    }

    int N = 1000000, fact = 1;
    int i;
    extern void doSomethingWithQFont(QFont&);
    double overhead;
    do {
        N *= fact;
        HRTime_tic();
        QFont tmp(font);
        for (i = 0 ; i < N ; ++i) {
            doSomethingWithQFont(tmp);
        }
        overhead = HRTime_toc();
        qInfo() << "overhead=" << overhead;
        fact = 2;
    } while (overhead < 0.05);
    QFontDatabase db;
    HRTime_tic();
    for (i = 0 ; i < N ; ++i) {
        QFont tmp(stripStyleName(font, db));
        doSomethingWithQFont(tmp);
    }
    double elapsed = HRTime_toc();
    qWarning() << N << "times `QFont tmp=stripStyleName(font)`:" << elapsed - overhead << "seconds";
    HRTime_tic();
    for (i = 0 ; i < N ; ++i) {
        QFont tmp(clone(font));
        doSomethingWithQFont(tmp);
    }
    elapsed = HRTime_toc();
    qWarning() << N << "times `QFont tmp(clone(font))`:" << elapsed - overhead << "seconds";
}

class DialogOptionsWidget : public QGroupBox
{
public:
    explicit DialogOptionsWidget(QWidget *parent = 0);

    QCheckBox *addCheckBox(const QString &text, int value);
    void addSpacer();
    int value() const;

private:
    typedef QPair<QCheckBox *, int> CheckBoxEntry;
    QVBoxLayout *layout;
    QList<CheckBoxEntry> checkBoxEntries;
};

DialogOptionsWidget::DialogOptionsWidget(QWidget *parent) :
    QGroupBox(parent) , layout(new QVBoxLayout)
{
    setTitle(Dialog::tr("Options"));
    setLayout(layout);
}

QCheckBox *DialogOptionsWidget::addCheckBox(const QString &text, int value)
{
    QCheckBox *checkBox = new QCheckBox(text);
    layout->addWidget(checkBox);
    checkBoxEntries.append(CheckBoxEntry(checkBox, value));
    return checkBox;
}

void DialogOptionsWidget::addSpacer()
{
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
}

int DialogOptionsWidget::value() const
{
    int result = 0;
    foreach (const CheckBoxEntry &checkboxEntry, checkBoxEntries)
        if (checkboxEntry.first->isChecked())
            result |= checkboxEntry.second;
    return result;
}

static QString fontRepr(QFont &font)
{
    return QString("%1,%2pt,w=%3,it=%4").arg(font.family()).arg(font.pointSize()).arg(font.weight()).arg(font.italic());
}

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
{
    setAttribute(Qt::WA_QuitOnClose, true);
    // will be toggled by fontDetails()
    setWindowModified(true);

    qApp->setStyleSheet("QLabel{ background: white }");
    int frameStyle = QFrame::Sunken | QFrame::Panel;
    QSettings store;
    QVariant prefStoreType = store.value("storeNativeQFont");
    if (prefStoreType != QVariant()) {
        storeNativeQFont = prefStoreType.toBool();
    }
    else{
        storeNativeQFont = true;
    }
    QVariant prefFont = store.value("font");
    fontLabel = new QLabel;
    fontLabel->setFrameStyle(frameStyle);
    fontLabel->setToolTip(tr("this shows what QFont::key() returns for the current font"));
    font = fontLabel->font();
    if (prefFont != QVariant()) {
        if (storeNativeQFont) {
            if (prefFont.canConvert<QFont>()) {
                font = prefFont.value<QFont>();
                qWarning() << "Restoring saved native font" << font << "from" << prefFont;
                fontDetails(font, stdout);
            }
            else {
                qWarning() << "Cannot restore font from" << prefFont;
            }
        }
        else {
            QFont fn;
            if (!fn.fromString(prefFont.toString())) {
                qWarning() << "Failed to restore font from" << prefFont << "toString=" << prefFont.toString();
            }
            else {
                qWarning() << "Restoring saved font" << fn;
                fontDetails(fn, stdout);
                font = fn;
            }
        }
    }
    fontLabel->setFont(font);
    fontLabel->setText(font.key());
    QPushButton *fontButton = new QPushButton(tr("QFontDialog::get&Font()"));
    fontButton->setToolTip(tr("this initialises the getFont dialog with the font object selected previously"));

    fontLabel2 = new QLabel;
    fontLabel2->setFrameStyle(frameStyle);
    fontLabel2->setToolTip(tr("this shows the current font's common attributes"));
    fontLabel2->setFont(font);
    fontLabel2->setText(fontRepr(font));
    QPushButton *fontButton2 = new QPushButton(tr("QFontDialog::getFont(font.key())"));
    fontButton2->setToolTip(tr("this initialises the getFont dialog with the shown representation of previously selected font"));

    connect(fontButton, SIGNAL(clicked()), this, SLOT(setFont()));
    connect(fontButton2, SIGNAL(clicked()), this, SLOT(setFontFromSpecs()));

    fontPreview = new QLabel;
    fontPreview->setFrameStyle(frameStyle);
    fontPreview->setToolTip(tr("this shows current font family, QFont::styleString() and decimal point size"));
    fontPreview->setFont(font);
    QFontDatabase db;
    fontPreview->setText( font.family() + tr(" ") + db.styleString(font) + tr(" @ ") + QString("%1pt").arg(font.pointSizeF()) );

    clonedFontPreview = new QLabel;
    clonedFontPreview->setFrameStyle(frameStyle);
    clonedFontPreview->setToolTip(tr("this shows the font cloned without styleName"));
    clonedBoldFontPreview = new QLabel;
    clonedBoldFontPreview->setFrameStyle(frameStyle);
    clonedBoldFontPreview->setToolTip(tr("this shows the font cloned without styleName and made bold"));
    {   QFontDatabase db;
        QFont tmp = stripStyleName(font, db);
        clonedFontPreview->setFont(tmp);
        tmp.setBold(true);
        clonedBoldFontPreview->setFont(tmp);
    }
    clonedFontPreview->setText(clonedFontPreview->font().key());
    clonedBoldFontPreview->setText(clonedBoldFontPreview->font().key());

    QPushButton *styleButton = new QPushButton(tr("set styleName"));
    styledFontPreview = new QLabel;
    styledFontPreview->setFrameStyle(frameStyle);
#define STYLEDFNTPREVIEWTT "this shows the result of setting a stylename on the selected font\nResolves to: %1"
    styledFontPreview->setToolTip(tr(STYLEDFNTPREVIEWTT).arg(styledFontPreview->font().toString()));
    connect(styleButton, SIGNAL(clicked()), this, SLOT(setFontStyleName()));

    QPushButton *famButton = new QPushButton(tr("Lookup from Family"));
    fontFamilyPreview = new QLabel;
    fontFamilyPreview->setFrameStyle(frameStyle);
    connect(famButton, SIGNAL(clicked()), this, SLOT(getFontFromFamily()));

    fontStyleName = new QLabel;
    fontStyleName->setFrameStyle(frameStyle);
    fontStyleName->setToolTip(tr("this shows the current styleName attribute that has been set on the font"));

    QGridLayout *layout = new QGridLayout;
    const QString doNotUseNativeDialog = tr("Do not use native dialog");

    layout->setColumnStretch(1, 1);
    layout->addWidget(fontButton, 0, 0);
    layout->addWidget(fontLabel, 0, 1);
    layout->addWidget(fontButton2, 1, 0);
    layout->addWidget(fontLabel2, 1, 1);
    layout->addWidget(fontPreview, 2, 0);
    layout->addWidget(fontStyleName, 2, 1);
    layout->addWidget(clonedFontPreview, 3, 0);
    layout->addWidget(clonedBoldFontPreview, 3, 1);
    layout->addWidget(styleButton, 4, 0);
    layout->addWidget(styledFontPreview, 4, 1);
    layout->addWidget(famButton, 5, 0);
    layout->addWidget(fontFamilyPreview, 5, 1);

    QPushButton *rawButton = new QPushButton(tr("Load font file"));
    connect(rawButton, SIGNAL(clicked()), this, SLOT(getFontFromFile()));
    rawFontSize = new QSpinBox;
    rawFontSize->setRange(1, 256);
    rawFontSize->setValue(12);
    rawFontSize->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    connect(rawFontSize, SIGNAL(valueChanged(int)), this, SLOT(getFontFromFile()));
    paintLabel = new QFrame;
    paintLabel->setFrameStyle(frameStyle);
    paintLabel->setToolTip(tr("This shows a low-level render of the font file via QRawFont"));
    QGridLayout *rf = new QGridLayout;
    rf->addWidget(rawButton, 0, 0);
    rf->addWidget(rawFontSize, 0, 1);
    layout->addLayout(rf, 7, 0);
    layout->addWidget(paintLabel, 7, 1);
//     layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding), 1, 0);

    fontStretchOrSpace = new QCheckBox(tr("stretch/space"), this);
    fontStretchOrSpace->setToolTip(tr("Apply stretch (checked) or letter-spacing (unchecked)"));
    fontStretchOrSpace->setChecked(true);
    stretchedFontPreview = new QLabel(this);
    stretchedFontPreview->setFrameStyle(frameStyle);
    stretchedFontPreview->setToolTip(tr("this shows the current font after stretching"));
    fontStretch = new QSpinBox(this);
    fontStretch->setRange(1, 4000);
    fontStretch->setValue(QFont::Unstretched);
    fontStretch->setToolTip(tr("stretch or letter-spacing in percentage"));
    connect(fontStretch, SIGNAL(valueChanged(int)), this, SLOT(applyStretch()));
    rf = new QGridLayout;
    rf->addWidget(fontStretchOrSpace, 0, 0);
    rf->addWidget(fontStretch, 0, 1);
    layout->addLayout(rf, 8, 0);
    layout->addWidget(stretchedFontPreview, 8, 1);

    fontDialogOptionsWidget = new DialogOptionsWidget;
    fontDialogOptionsWidget->addCheckBox(doNotUseNativeDialog, QFontDialog::DontUseNativeDialog);
    fontDialogOptionsWidget->addCheckBox(tr("No buttons") , QFontDialog::NoButtons);
    fontStoreTypeSel = fontDialogOptionsWidget->addCheckBox(tr("Store the selected font as a text representation"), 0);
    fontStoreTypeSel->setToolTip(tr("Store the selected font as a text representation (QFont::toString()) or as a QFont"));
    fontStoreTypeSel->setChecked(!storeNativeQFont);
    connect(fontStoreTypeSel, SIGNAL(clicked()), this, SLOT(setFontStoreType()));
#if 0
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding), 1, 0);
#endif
    layout->addWidget(fontDialogOptionsWidget, 9, 0, 1 ,2);

    setLayout(layout);

    setWindowTitle(tr("Font Selection"));

    update();

#if QT_VERSION >= QT_VERSION_CHECK(5,5,0)
    qWarning() << "For reference: QFont::ExtraLight=" << QFont::ExtraLight
             << "QFont::Thin=" << QFont::Thin
             << "QFont::Light=" << QFont::Light
             << "QFont::Normal=" << QFont::Normal
             << "QFont::Medium" << QFont::Medium
             << "QFont::DemiBold=" << QFont::DemiBold
             << "QFont::Bold=" << QFont::Bold
             << "QFont::Black=" << QFont::Black;
#elif QT_VERSION >= QT_VERSION_CHECK(5,4,0)
    qWarning() << "For reference: qt_extralightFontWeight=" << qt_extralightFontWeight
             << "qt_thinFontWeight=" << qt_thinFontWeight
             << "QFont::Light=" << QFont::Light
             << "QFont::Normal=" << QFont::Normal
             << "qt_mediumFontWeight" << qt_mediumFontWeight
             << "QFont::DemiBold=" << QFont::DemiBold
             << "QFont::Bold=" << QFont::Bold
             << "QFont::Black=" << QFont::Black;
#else
    qWarning() << "For reference: QFont::Light=" << QFont::Light
             << "QFont::Normal=" << QFont::Normal
             << "QFont::DemiBold=" << QFont::DemiBold
             << "QFont::Bold=" << QFont::Bold
             << "QFont::Black=" << QFont::Black;
#endif
    styleString[QFont::StyleNormal] = "normal";
    styleString[QFont::StyleItalic] = "italic";
    styleString[QFont::StyleOblique] = "oblique";
    styleHintString[QFont::Helvetica] = "Helvetica";
    styleHintString[QFont::SansSerif] = "SansSerif (Helvetica)";
    styleHintString[QFont::Times] = "Times";
    styleHintString[QFont::Serif] = "Serif (Times)";
    styleHintString[QFont::Courier] = "Courier";
    styleHintString[QFont::TypeWriter] = "TypeWriter (Courier)";
    styleHintString[QFont::OldEnglish] = "OldEnglish";
    styleHintString[QFont::Decorative] = "Decorative (OldEnglish)";
    styleHintString[QFont::System] = "System";
    styleHintString[QFont::AnyStyle] = "AnyStyle";
    styleHintString[QFont::Cursive] = "Cursive";
    styleHintString[QFont::Monospace] = "Monospace";
    styleHintString[QFont::Fantasy] = "Fantasy";

    QFont::insertSubstitution(QStringLiteral("Helvetica"), QStringLiteral("Helvetica Neue"));
    qWarning() << "Current substitutions:";
    foreach (const auto subst, QFont::substitutions()) {
        qWarning() << "\t" << subst << "->" << QFont::substitutes(subst);
    }

//     benchmarkCloning(font);
}

// Linux/X11:
// QFontInfo for Monaco,9,-1,5,0,0,0,0,0,0 :
//     family Monaco Regular/normal 9.2093pt; weight 0
//     styleHint: AnyStyle, fixed pitch
// QFontMetrics:
//     ascent, descent: 9,3; average width=7
//     height, x-height, max.width: 12,6,7; natural line spacing: 0
// QFontInfo for Segoe UI,10,-1,5,63,0,0,0,0,0 :
//         family Segoe UI Semibold/normal bold 10.0465pt; weight 63
//         styleHint: AnyStyle
// QFontMetrics:
//         ascent, descent: 13,3; average width=7
//         height, x-height, max.width: 16,6,17; natural line spacing: 0

// Mac OS X/cocoa/CoreText
// QFontInfo for Monaco,9,-1,5,50,0,0,0,0,0 (exact match):
//         family Monaco Regular/normal 9pt; weight 50
//         styleHint: AnyStyle, fixed pitch
// QFontMetrics:
//         ascent, descent: 9,2; average width=5
//         height, x-height, max.width: 11,5,5; natural line spacing: 1
// QFontInfo for Segoe UI,10,-1,5,63,0,0,0,0,0 (exact match):
//         family Segoe UI Semibold/normal bold 10pt; weight 63
//         styleHint: AnyStyle
// QFontMetrics:
//         ascent, descent: 11,3; average width=6
//         height, x-height, max.width: 14,5,10; natural line spacing: 0

// Mac OS X/cocoa/freetype
// QFontInfo for Monaco,9,-1,5,50,0,0,0,0,0 (exact match):
//         family Monaco Regular/normal 9pt; weight 50
//         styleHint: AnyStyle, fixed pitch
// QFontMetrics:
//         ascent, descent: 9,3; average width=5
//         height, x-height, max.width: 12,5,5; natural line spacing: 0
// QFontInfo for Segoe UI,10,-1,5,63,0,0,0,0,0 (exact match):
//         family Segoe UI Semibold/normal bold 10pt; weight 63
//         styleHint: AnyStyle
// QFontMetrics:
//         ascent, descent: 11,3; average width=6
//         height, x-height, max.width: 14,5,14; natural line spacing: -1

// Mac OS X/xcb
// QFontInfo for Monaco [PfEd],9,-1,5,0,0,0,0,0,0 (exact match):
//         family Monaco [PfEd] Regular/normal 9pt; weight 0
//         styleHint: AnyStyle, fixed pitch
// QFontMetrics:
//         ascent, descent: 8,2; average width=5
//         height, x-height, max.width: 10,5,5; natural line spacing: 0
// QFontInfo for Monaco [unknown],9,-1,5,50,0,0,0,0,0 (exact match):
//         family Monaco [unknown] Regular/normal 9pt; weight 50
//         styleHint: AnyStyle, fixed pitch
// QFontMetrics:
//         ascent, descent: 9,3; average width=5
//         height, x-height, max.width: 12,5,5; natural line spacing: 0
// QFontInfo for Segoe UI,10,-1,5,63,0,0,0,0,0 (exact match):
//         family Segoe UI Semibold/normal bold 10pt; weight 63
//         styleHint: AnyStyle
// QFontMetrics:
//         ascent, descent: 11,3; average width=6
//         height, x-height, max.width: 14,5,14; natural line spacing: -1

QFont Dialog::fontDetails(QFont &font, QTextStream &sink)
{
    QFont ret = font;
    QFontInfo fi(font);
    setWindowModified(!isWindowModified());
    setWindowFilePath(QString());
    setWindowTitle(fi.family() + " [*]");
    sink << "QFontInfo for " << font.toString() << (fi.exactMatch()? " (exact match):" : " :") << endl;
    sink << "\tfamily " << fi.family() << " styleName=" << fi.styleName() << ", style=" << styleString[fi.style()]
        << (fi.bold()? " bold " : " ")
        << fi.pointSizeF() << "pt; " << fi.pixelSize() << "px; weight " << fi.weight() << "\n\t"
        << "stretch: " << font.stretch() << "; l.spacing:" << font.letterSpacing()
        << " type: " << font.letterSpacingType() << endl;
    sink << "\tstyleHint: " << styleHintString[fi.styleHint()] << (fi.fixedPitch()? ", fixed pitch" : "") << endl;
    QFontDatabase db;
    sink << "\tQFontDatabase::styleString() = " << db.styleString(font) << endl;
    if (!font.styleName().isEmpty()) {
        ret = db.font(font.family(), font.styleName(), font.pointSize());
        sink << "\tQFontDatabase::font(" << font.family() << "," << font.styleName() << "," << font.pointSize() << ") = "
            << ret.toString() << endl;
    }
    QFontMetrics fm(font);
    sink << "QFontMetrics:" << endl;
    sink << "\tleading, ascent, descent: " << fm.leading() << "," << fm.ascent() << "," << fm.descent()
        << "; bearings: " << fm.minLeftBearing() << "," << fm.minRightBearing()
        << "; average width=" << fm.averageCharWidth() << endl;
    sink << "\theight, x-height, max.width: " << fm.height() << "," << fm.xHeight() << "," << fm.maxWidth()
        << "; natural line spacing: " << fm.leading() << endl;
    return ret;
}

QString Dialog::fontDetails(QFont &font)
{
    QString details;
    QTextStream sink(&details);
    fontDetails(font, sink);
    return details;
}

QFont Dialog::fontDetails(QFont &font, FILE *fp)
{
    QTextStream sink(fp);
    return fontDetails(font, sink);
}

QFont Dialog::fontDetails(QRawFont &font, QTextStream &sink)
{
    QFont ret;
    setWindowModified(!isWindowModified());
    setWindowTitle(font.familyName() + " [*]");
    sink << "QRawFontInfo:" << endl;
    sink << "\tfamily " << font.familyName() << " styleName=" << font.styleName() << ", style=" << styleString[font.style()]
        << " " << font.pixelSize() << "px; weight " << font.weight() << "\n\t" << endl;
    QFontDatabase db;
    if (!font.styleName().isEmpty()) {
        ret = db.font(font.familyName(), font.styleName(), font.pixelSize());
        sink << "\tQFontDatabase::font(" << font.familyName() << "," << font.styleName() << "," << font.pixelSize() << ") = "
            << ret.toString() << endl;
    }
    sink << "QRawFontMetrics:" << endl;
    sink << "\tleading, ascent, descent: " << font.leading() << "," << font.ascent() << "," << font.descent()
        << "; average width=" << font.averageCharWidth() << endl;
    sink << "\tcapHeight, x-height, max.Charwidth: " << font.capHeight() << "," << font.xHeight() << "," << font.maxCharWidth()
        << "; natural line spacing: " << font.leading() << endl;
    return ret;
}

void Dialog::setFont(QFont &fnt)
{
    font = fnt;
    fontLabel->setText(font.key());
    fontLabel->setFont(font);
    fontLabel2->setText(fontRepr(font));
    fontLabel2->setFont(font);
    fontStyleName->setText(font.styleName());
    fontStyleName->setFont(font);
    QFontDatabase db;
    fontPreview->setFont(font);
    fontPreview->setText( font.family() + tr(" ") + db.styleString(font) + tr(" @ ") + QString("%1pt").arg(font.pointSizeF()) );

    {   QFontDatabase db;
        QFont tmp = stripStyleName(font, db);
        clonedFontPreview->setFont(tmp);
        tmp.setBold(true);
        clonedBoldFontPreview->setFont(tmp);
    }
    clonedFontPreview->setText(clonedFontPreview->font().key());
    clonedBoldFontPreview->setText(clonedBoldFontPreview->font().key());

    qWarning() << "QFontDatabase::styleString for this typeface:" << db.styleString(font);
    qWarning() << "font.key():" << font.key();
    QFont dum;
    dum.fromString(font.toString());
    qWarning() << "QFont::fromString(" << font.toString() << ")" << dum;
    fontDetails(font, stdout);
    if (storeNativeQFont) {
        QSettings().setValue("font", font);
    }
    else{
        QSettings().setValue("font", font.toString());
    }
    fontLabel->update();
    setPaintFont(font);
    fontStretch->setValue(QFont::Unstretched);
}

void Dialog::setFont()
{
    const QFontDialog::FontDialogOptions options = QFlag(fontDialogOptionsWidget->value());
//     bool ok;
//     QFont font = QFontDialog::getFont(&ok, QFont(fontLabel->text()), this, "Select Font", options);
//     if (ok) {
//         fontLabel->setText(font.key());
//         fontLabel->setFont(font);
//     }
    bool ok;
    qWarning() << "Preselecting font=" << font;
    QFont fnt = QFontDialog::getFont(&ok, font, this, "Select Font", options);
    if (ok) {
        setFont(fnt);
    }
    benchmarkCloning(fnt);
}

void Dialog::setFontFromSpecs()
{
    const QFontDialog::FontDialogOptions options = QFlag(fontDialogOptionsWidget->value());
    bool ok;
    qWarning() << "Preselecting font.key=" << fontLabel2->text() << "(font=" << font << ")";
    QFont font2 = QFont(font.family(), font.pointSize(), font.weight(), font.italic());
    qWarning() << "QFont:family=" << font.family() << "weight=" << font.weight() << font.styleName() << "italic=" << font.italic() << "=" << font2;
    qWarning() << "Preselecting font.key=" << fontLabel2->text() << "(font=" << font << "=>" << font2 << ")";
    font2 = QFontDialog::getFont(&ok, font2, this, "Select Font", options);
    if (ok) {
        fontLabel2->setText(fontRepr(font2));
        fontLabel2->setFont(font2);
        fontLabel->setText(font2.key());
        fontLabel->setFont(font2);
        fontStyleName->setText(font2.styleName());
        fontStyleName->setFont(font2);
        qWarning() << "Selected font" << font2 << "which" << ((font == font2)? "is" : "is not") << "equal to the previous font" << font;
        font = font2;
        QFontDatabase db;
        fontPreview->setFont(font);
        fontPreview->setText( font.family() + tr(" ") + db.styleString(font) + tr(" @ ") + QString("%1pt").arg(font.pointSizeF()) );

        {   QFontDatabase db;
            QFont tmp = stripStyleName(font2, db);
            clonedFontPreview->setFont(tmp);
            tmp.setBold(true);
            clonedBoldFontPreview->setFont(tmp);
        }
        clonedFontPreview->setText(clonedFontPreview->font().key());
        clonedBoldFontPreview->setText(clonedBoldFontPreview->font().key());

        qWarning() << "QFontDatabase::styleString for this typeface:" << db.styleString(font);
        qWarning() << "font.key():" << font.key();
        QFont dum;
        dum.fromString(font.toString());
        qWarning() << "QFont::fromString(" << font.toString() << ")" << dum;
        fontDetails(font, stdout);
        QSettings store;
        if (storeNativeQFont) {
            store.setValue("font", font);
        }
        else{
            store.setValue("font", font.toString());
        }
//         store.sync();
//         qWarning() << "Font QSetting" << store.allKeys() << "status:" << store.status();
//         qWarning() << "settings(\"font\")=" << store.value("font") << "canConvert<QFont>:" << store.value("font").canConvert<QFont>();
        fontLabel2->update();
        setPaintFont(font2);
        fontStretch->setValue(QFont::Unstretched);
    }
}

void Dialog::setFontStoreType()
{
    storeNativeQFont = !fontStoreTypeSel->isChecked();
    QSettings store;
    store.setValue("storeNativeQFont", storeNativeQFont);
    if (storeNativeQFont) {
        store.setValue("font", font);
    }
    else{
        store.setValue("font", font.toString());
    }
    store.sync();
    qWarning() << "Font QSetting" << store.allKeys() << "status:" << store.status();
    qWarning() << "settings(\"font\")=" << store.value("font") << "canConvert<QFont>:" << store.value("font").canConvert<QFont>();
}

void Dialog::getFontFromFamily()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("QFont::family()"),
                                         tr("Font Family:"), QLineEdit::Normal,
                                         famFont.family(), &ok);
    if (ok && !text.isEmpty()) {
//         famFont.setStyleStrategy(QFont::ForceOutline);
        famFont.setFamily(text);
        fontFamilyPreview->setFont(famFont);
        fontFamilyPreview->setText(text + QLatin1String(" -> ") + famFont.toString()
            + QLatin1String(" = ") + QFontInfo(famFont).family());
        fontFamilyPreview->update();
        setPaintFont(famFont);
        fontDetails(famFont, stdout);
    }
}

void Dialog::getFontFromFile()
{
    int pointSize = rawFontSize->value();
    if (sender() != rawFontSize) {
        static QString startDir = QStandardPaths::writableLocation(QStandardPaths::FontsLocation);
//         QString fName = QFileDialog::getOpenFileName(this, tr("Pick a font file"), startDir);
//         if (!fName.isEmpty())
        auto fDialog = new QFileDialog(this, tr("Pick a font file"), startDir);
        auto variant = QVariant(QVariant::UserType);
        variant.setValue(fDialog);
        setProperty("QFileDialogInstance", variant);
        qWarning() << "\t" << fDialog << property("QFileDialogInstance");
        if (fDialog->exec() && !fDialog->selectedFiles().isEmpty())
        {
            QString fName = fDialog->selectedFiles().at(0);
            QFileInfo fi(fName);
            startDir = fi.absoluteDir().path();
#ifdef QRAWFONT_FROM_DATA
            QFile f(fName);
            f.open(QIODevice::ReadOnly);
            QByteArray fontData = f.readAll();
            f.close();
            QRawFont rFont(fontData, pointSize, QFont::PreferFullHinting);
#else
            QRawFont rFont(fName, pointSize, QFont::PreferFullHinting);
#endif
            if (rFont.isValid()) {
                qWarning() << "Read font from" << fName;
                rawFont = rFont;
#ifdef QRAWFONT_FROM_DATA
                qWarning() << "addApplicationFontFromData() returns:" << QFontDatabase::addApplicationFontFromData(fontData);
#else
                qWarning() << "addApplicationFont() returns:" << QFontDatabase::addApplicationFont(fName);
#endif
            } else {
                qWarning() << fName << "doesn't give a valid font";
                return;
            }
            setWindowFilePath(fName);
        }
        delete fDialog;
    }
    if (rawFont.isValid()) {
        rawFont.setPixelSize(pointSize);
        const QString label = QStringLiteral("%1 %2 @ %3pt")\
            .arg(rawFont.familyName()).arg(rawFont.styleName()).arg(pointSize);
        qWarning() << "Raw font:" << label;
        setPaintFont(rawFont, label);
        QTextStream sink(stdout);
        fontDetails(rawFont, sink);
    }
}

void Dialog::setFontStyleName()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("QFont::styleName()"),
                                         tr("Font styleName:"), QLineEdit::Normal,
                                         font.styleName(), &ok);
    if (ok) {
        QFont fnt(font);
        fnt.setStyleName(text);
        styledFontPreview->setFont(fnt);
        styledFontPreview->setText(fnt.key());
        fnt = fontDetails(fnt, stdout);
        styledFontPreview->setToolTip(tr(STYLEDFNTPREVIEWTT).arg(fnt.toString()));
        QFont boldFnt(fnt);
        boldFnt.setStyleName(QString());
        boldFnt.setBold(true);
        qWarning() << "style name emptied, font boldened:" << boldFnt.toString();
        QFontDatabase db;
        QFont stripped = stripStyleName(fnt, db);
        qWarning() << "stripStyleName() gives" << stripped.toString();
        stripped.setBold(true);
        qWarning() << "\tboldened:" << stripped.toString();
    }
}

void Dialog::setPaintFont(const QRawFont &rFont, const QString &text)
{
    rawFont = rFont;

    const auto glIdx = rawFont.glyphIndexesForString(text);
    const auto advances = rawFont.advancesForGlyphIndexes(glIdx, QRawFont::SeparateAdvances);
    qWarning() << text << "advances=" << advances;

    QTextLayout layout(text);
    layout.setRawFont(rFont);
    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setLineWidth(INT_MAX/256);
    layout.endLayout();

    glyphRuns = line.glyphRuns();
    qWarning() << "Bounding rect(s):";
    foreach (const auto glyph, glyphRuns) {
        qWarning() << glyph.boundingRect();
    }
    paintLabel->setFixedHeight(rawFont.ascent() + rawFont.descent() + 4);
    paintLabel->update();

    update();
}

void Dialog::setPaintFont(const QFont &font)
{
    QRawFont rFont = QRawFont::fromFont(font);
    rFont.setPixelSize(rawFontSize->value());
    setPaintFont(rFont,
         QStringLiteral("%1 %2 @ %3pt").arg(rFont.familyName()).arg(rFont.styleName()).arg(rFont.pixelSize()));
}

void Dialog::applyStretch()
{
    auto stretch = fontStretch->value();
    QFont fnt(font);
    if (fontStretchOrSpace->isChecked()) {
        fnt.setStretch(stretch);
    } else {
        fnt.setLetterSpacing(QFont::PercentageSpacing, stretch);
    }
    stretchedFontPreview->setFont(fnt);
    stretchedFontPreview->setText(fnt.key());
    fnt = fontDetails(fnt, stdout);
}

void Dialog::paintEvent(QPaintEvent *)
{
    const auto margins = paintLabel->contentsMargins();
    const auto frame = paintLabel->geometry().marginsRemoved(margins);
//     qWarning() << "paintLabel at" << frame << frame.topLeft();

//     qWarning() << "Drawing" << glyphRuns.size() << "glyphruns at" << frame.topLeft();
    if (glyphRuns.isEmpty())
        return;

    QPainter p;
    p.begin(this);
    p.setPen(Qt::black);

    QPoint point(frame.topLeft());
    foreach (const auto glyph, glyphRuns) {
        p.drawGlyphRun(point, glyph);
    }
    p.end();
}

const char *qFontToString(QFont *qfont)
{
    return qfont->toString().toLatin1().data();
}

