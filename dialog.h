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

#ifndef DIALOG_H
#define DIALOG_H

#include <QWidget>
#include <QDialog>
#include <QMap>
#include <QList>
#include <QGlyphRun>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLabel;
class QErrorMessage;
class QFrame;
class QSpinBox;
QT_END_NAMESPACE

class DialogOptionsWidget;
class QTextStream;
class KFontRequester;

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = 0);

    void setPaintFont(const QRawFont &font, const QString &text);
    void setPaintFont(const QFont &font);
    void paintEvent(QPaintEvent *);

private slots:
    void setFont();
    void setFontFromSpecs();
    void setFontStoreType();
    void getFontFromFamily();
    void getFontFromFile();
    void setFontStyleName();
    void applyStretch();

private slots:
    void setFont(const QFont &fnt);

private:
    QLabel *fontLabel, *fontLabel2, *fontPreview, *fontFamilyPreview, *fontStyleName;
    QLabel *clonedFontPreview, *clonedBoldFontPreview, *styledFontPreview, *stretchedFontPreview;
    QFont font, famFont;
    DialogOptionsWidget *fontDialogOptionsWidget;
    QCheckBox *fontStoreTypeSel, *fontStretchOrSpace;
    bool storeNativeQFont;
    QMap<QFont::Style, QString> styleString;
    QMap<QFont::StyleHint, QString> styleHintString;
    QFont fontDetails(QFont &font, QTextStream &sink);
    QString fontDetails(QFont &font);
    QFont fontDetails(QFont &font, FILE *fp);
    QFont fontDetails(QRawFont &font, QTextStream &sink);

    QRawFont rawFont;
    QSpinBox *rawFontSize, *fontStretch;
    QFrame *paintLabel;
    QList<QGlyphRun> glyphRuns;

    KFontRequester *fontRequester;
};

#endif
