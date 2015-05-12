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

#if QT_VERSION >= QT_VERSION_CHECK(5,4,0)
#	include <QtGui/private/qfontengine_p.h>
#endif

#include "dialog.h"

#define MESSAGE \
    Dialog::tr("<p>Message boxes have a caption, a text, " \
               "and any number of buttons, each with standard or custom texts." \
               "<p>Click a button to close the message box. Pressing the Esc button " \
               "will activate the detected escape button (if any).")
#define MESSAGE_DETAILS \
    Dialog::tr("If a message box has detailed text, the user can reveal it " \
               "by pressing the Show Details... button.")


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
    : QWidget(parent)
{
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
    font = fontLabel->font();
    if (prefFont != QVariant()) {
        if (storeNativeQFont) {
            if (prefFont.canConvert<QFont>()) {
                font = prefFont.value<QFont>();
                qDebug() << "Restoring saved font" << font;
                fontLabel->setFont(font);
            }
            else {
                qDebug() << "Cannot restore font from" << prefFont;
            }
        }
        else {
            QFont fn;
            if (!fn.fromString(prefFont.toString())) {
                qDebug() << "Failed to restore font from" << prefFont << "toString=" << prefFont.toString();
            }
            else {
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
    fontLabel2->setFont(font);
    fontLabel2->setText(fontRepr(font));
    QPushButton *fontButton2 = new QPushButton(tr("QFontDialog::getFont(font.key())"));
    fontButton2->setToolTip(tr("this initialises the getFont dialog with the shown representation of previously selected font"));

    connect(fontButton, SIGNAL(clicked()), this, SLOT(setFont()));
    connect(fontButton2, SIGNAL(clicked()), this, SLOT(setFontFromSpecs()));

    QGridLayout *layout = new QGridLayout;
    const QString doNotUseNativeDialog = tr("Do not use native dialog");

    layout->setColumnStretch(1, 1);
    layout->addWidget(fontButton, 0, 0);
    layout->addWidget(fontLabel, 0, 1);
    layout->addWidget(fontButton2, 1, 0);
    layout->addWidget(fontLabel2, 1, 1);
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
    layout->addWidget(fontDialogOptionsWidget, 2, 0, 1 ,2);
    setLayout(layout);

    setWindowTitle(tr("Font Selection"));
#if QT_VERSION >= QT_VERSION_CHECK(5,4,0)
    qDebug() << "For reference: qt_extralightFontWeight=" << qt_extralightFontWeight
             << "qt_thinFontWeight=" << qt_thinFontWeight
             << "QFont::Light=" << QFont::Light
             << "QFont::Normal=" << QFont::Normal
             << "qt_mediumFontWeight" << qt_mediumFontWeight
             << "QFont::DemiBold=" << QFont::DemiBold
             << "QFont::Bold=" << QFont::Bold
             << "QFont::Black=" << QFont::Black;
#else
    qDebug() << "For reference: QFont::Light=" << QFont::Light
             << "QFont::Normal=" << QFont::Normal
             << "QFont::DemiBold=" << QFont::DemiBold
             << "QFont::Bold=" << QFont::Bold
             << "QFont::Black=" << QFont::Black;
#endif
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
    qDebug() << "Preselecting font=" << font;
    font = QFontDialog::getFont(&ok, font, this, "Select Font", options);
    if (ok) {
        fontLabel->setText(font.key());
        fontLabel->setFont(font);
        fontLabel2->setText(fontRepr(font));
        fontLabel2->setFont(font);
        QFontDatabase db;
        qDebug() << "QFontDatabase::styleString for this typeface:" << db.styleString(font);
        QFont dum;
        dum.fromString(font.toString());
        qDebug() << "QFont::fromString(" << font.toString() << ")" << dum;
        if (storeNativeQFont) {
            QSettings().setValue("font", font);
        }
        else{
            QSettings().setValue("font", font.toString());
        }
    }
}

void Dialog::setFontFromSpecs()
{
    const QFontDialog::FontDialogOptions options = QFlag(fontDialogOptionsWidget->value());
    bool ok;
    qDebug() << "Preselecting font.key=" << fontLabel2->text() << "(font=" << font << ")";
    QFont font2 = QFont(font.family(), font.pointSize(), font.weight(), font.italic());
    qDebug() << "QFont:family=" << font.family() << "weight=" << font.weight() << font.styleName() << "italic=" << font.italic() << "=" << font2;
    qDebug() << "Preselecting font.key=" << fontLabel2->text() << "(font=" << font << "=>" << font2 << ")";
    font2 = QFontDialog::getFont(&ok, font2, this, "Select Font", options);
    if (ok) {
        fontLabel2->setText(fontRepr(font2));
        fontLabel2->setFont(font2);
        fontLabel->setText(font2.key());
        fontLabel->setFont(font2);
        font = font2;
        QFontDatabase db;
        qDebug() << "QFontDatabase::styleString for this typeface:" << db.styleString(font);
        QFont dum;
        dum.fromString(font.toString());
        qDebug() << "QFont::fromString(" << font.toString() << ")" << dum;
        QSettings store;
        if (storeNativeQFont) {
            store.setValue("font", font);
        }
        else{
            store.setValue("font", font.toString());
        }
//         store.sync();
//         qDebug() << "Font QSetting" << store.allKeys() << "status:" << store.status();
//         qDebug() << "settings(\"font\")=" << store.value("font") << "canConvert<QFont>:" << store.value("font").canConvert<QFont>();
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
    qDebug() << "Font QSetting" << store.allKeys() << "status:" << store.status();
    qDebug() << "settings(\"font\")=" << store.value("font") << "canConvert<QFont>:" << store.value("font").canConvert<QFont>();
}
