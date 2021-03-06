/*
    SPDX-FileCopyrightText: 1996 Bernd Johannes Wuebben <wuebben@kde.org>
    SPDX-FileCopyrightText: 1999 Preston Brown <pbrown@kde.org>
    SPDX-FileCopyrightText: 1999 Mario Weilguni <mweilguni@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kfontchooser.h"
#include "fonthelpers_p.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGuiApplication>
#include <QLabel>
#include <QLayout>
#include <QLocale>
#include <QSplitter>
#include <QScrollBar>
#include <QFontDatabase>
#include <QGroupBox>
#include <QListWidget>
#include <QTextEdit>

#include <cmath>

// When message extraction needs to be avoided.
#define TR_NOX tr

static int minimumListWidth(const QListWidget *list)
{
    int w = 0;
    for (int i = 0; i < list->count(); i++) {
        int itemWidth = list->visualItemRect(list->item(i)).width();
        // ...and add a space on both sides for not too tight look.
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        itemWidth += list->fontMetrics().horizontalAdvance(QLatin1Char(' ')) * 2;
#else
        itemWidth += list->fontMetrics().width(QLatin1Char(' ')) * 2;
#endif
        w = qMax(w, itemWidth);
    }
    if (w == 0) {
        w = 40;
    }
    w += list->frameWidth() * 2;
    w += list->verticalScrollBar()->sizeHint().width();
    return w;
}

static int minimumListHeight(const QListWidget *list, int numVisibleEntry)
{
    int w = list->count() > 0 ? list->visualItemRect(list->item(0)).height() :
            list->fontMetrics().lineSpacing();

    if (w < 0) {
        w = 10;
    }
    if (numVisibleEntry <= 0) {
        numVisibleEntry = 4;
    }
    return (w * numVisibleEntry + 2 * list->frameWidth());
}

static QString formatFontSize(qreal size)
{
    return QLocale::system().toString(size, 'f', (size == floor(size)) ? 0 : 1);
}

class Q_DECL_HIDDEN KFontChooser::Private
{
public:
    Private(KFontChooser *qq)
        : q(qq)
    {
        m_palette.setColor(QPalette::Active, QPalette::Text, Qt::black);
        m_palette.setColor(QPalette::Active, QPalette::Base, Qt::white);
        signalsAllowed = true;
        selectedSize = -1;
        customSizeRow = -1;
    }

    // pointer to an optinally supplied list of fonts to
    // inserted into the fontdialog font-family combo-box
//    QStringList  fontList;

    void init(const DisplayFlags &flags, const QStringList &fontList,
              int visibleListSize, Qt::CheckState *sizeIsRelativeState);
    void setFamilyBoxItems(const QStringList &fonts);
    void fillFamilyListBox(bool onlyFixedFonts = false);
    int nearestSizeRow(qreal val, bool customize);
    qreal fillSizeList(const QList<qreal> &sizes = QList<qreal>());
    qreal setupSizeListBox(const QString &family, const QString &style);

    void setupDisplay();
    QString styleIdentifier(const QFont &font);

    void _k_family_chosen_slot(const QString &);
    void _k_size_chosen_slot(const QString &);
    void _k_style_chosen_slot(const QString &);
    void _k_displaySample(const QFont &font);
    void _k_size_value_slot(double);

    KFontChooser *q;

    QPalette m_palette;

    QDoubleSpinBox *sizeOfFont = nullptr;

    QTextEdit   *sampleEdit = nullptr;

    QLabel       *familyLabel = nullptr;
    QLabel       *styleLabel = nullptr;
    QCheckBox    *familyCheckbox = nullptr;
    QCheckBox    *styleCheckbox = nullptr;
    QCheckBox    *sizeCheckbox = nullptr;
    QLabel       *sizeLabel = nullptr;
    QListWidget     *familyListBox = nullptr;
    QListWidget     *styleListBox = nullptr;
    QListWidget     *sizeListBox = nullptr;
    QCheckBox    *sizeIsRelativeCheckBox = nullptr;

    QCheckBox *onlyFixedCheckbox = nullptr;

    QFont        selFont;

    QString      selectedStyle;
    qreal        selectedSize;

    QString      standardSizeAtCustom;
    int          customSizeRow;

    bool signalsAllowed: 1;

    bool usingFixed: 1;

    // Mappings of translated to Qt originated family and style strings.
    QHash<QString, QString> qtFamilies;
    QHash<QString, QString> qtStyles;
    // Mapping of translated style strings to internal style identifiers.
    QHash<QString, QString> styleIDs;

};

KFontChooser::KFontChooser(QWidget *parent,
                           const DisplayFlags &flags,
                           const QStringList &fontList,
                           int visibleListSize,
                           Qt::CheckState *sizeIsRelativeState)
    : QWidget(parent),
      d(new KFontChooser::Private(this))
{
    d->init(flags, fontList, visibleListSize, sizeIsRelativeState);
}

KFontChooser::~KFontChooser()
{
    delete d;
}

void KFontChooser::Private::init(const DisplayFlags &flags, const QStringList &fontList,
                                 int visibleListSize, Qt::CheckState *sizeIsRelativeState)
{
    usingFixed = flags & FixedFontsOnly;

    // The main layout is divided horizontally into a top part with
    // the font attribute widgets (family, style, size) and a bottom
    // part with a preview of the selected font
    QVBoxLayout *mainLayout = new QVBoxLayout(q);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    auto *style = q->style();
    const int sizeListBoxGap = style->pixelMetric(QStyle::PM_LayoutVerticalSpacing) / 2;
    const int checkBoxGap = style->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) / 2;

    // Build the grid of font attribute widgets for the upper part of mainLayout
    QWidget *page;
    QGridLayout *gridLayout;
    int row = flags & DisplayFrame ? 1 : 0;

    if (flags & DisplayFrame) {
        page = new QGroupBox(KFontChooser::tr("Requested Font", "@title:group"), q);
        mainLayout->addWidget(page);
        gridLayout = new QGridLayout(page);
    } else {
        page = new QWidget(q);
        mainLayout->addWidget(page);
        gridLayout = new QGridLayout(page);
        gridLayout->setContentsMargins(0, 0, 0, 0);
    }

    // set up the family list view and checkbox
    QHBoxLayout *familyLayout = new QHBoxLayout();
    familyLayout->addSpacing(checkBoxGap);
    if (flags & ShowDifferences) {
        familyCheckbox = new QCheckBox(KFontChooser::tr("Font", "@option:check"), page);
        familyLayout->addWidget(familyCheckbox, 0, Qt::AlignLeft);
        familyLabel = nullptr;
    } else {
        familyCheckbox = nullptr;
        familyLabel = new QLabel(KFontChooser::tr("Font:", "@label"), page);
        familyLayout->addWidget(familyLabel, 1, Qt::AlignLeft);
    }
    gridLayout->addLayout(familyLayout, row, 0);

    ++row;

    familyListBox = new QListWidget(page);
    gridLayout->addWidget(familyListBox, row, 0);

    connect(familyListBox, &QListWidget::currentTextChanged, [this](const QString &family) {
        _k_family_chosen_slot(family);
    });

    if (flags & ShowDifferences) {
        familyListBox->setEnabled(false);
        connect(familyCheckbox, &QAbstractButton::toggled, familyListBox, &QWidget::setEnabled);
    }

    if (!fontList.isEmpty()) {
        setFamilyBoxItems(fontList);
    } else {
        fillFamilyListBox(flags & FixedFontsOnly);
    }

    familyListBox->setMinimumWidth(minimumListWidth(familyListBox));
    familyListBox->setMinimumHeight(minimumListHeight(familyListBox, visibleListSize));


    // set up the sytle list view and checkbox
    row = flags & DisplayFrame ? 1 : 0;
    QHBoxLayout *styleLayout = new QHBoxLayout();
    if (flags & ShowDifferences) {
        styleCheckbox = new QCheckBox(KFontChooser::tr("Font style", "@option:check"), page);
        styleLayout->addWidget(styleCheckbox, 0, Qt::AlignLeft);
        styleLabel = nullptr;
    } else {
        styleCheckbox = nullptr;
        styleLabel = new QLabel(KFontChooser::tr("Font style:", "@label"), page);
        styleLayout->addWidget(styleLabel, 1, Qt::AlignLeft);
    }
    styleLayout->addSpacing(checkBoxGap);
    gridLayout->addLayout(styleLayout, row, 1);

    ++row;

    styleListBox = new QListWidget(page);
    gridLayout->addWidget(styleListBox, row, 1);

    // Populate usual styles, to determine minimum list width;
    // will be replaced later with correct styles.
    styleListBox->addItem(KFontChooser::tr("Normal", "@item font"));
    styleListBox->addItem(KFontChooser::tr("Italic", "@item font"));
    styleListBox->addItem(KFontChooser::tr("Oblique", "@item font"));
    styleListBox->addItem(KFontChooser::tr("Bold", "@item font"));
    styleListBox->addItem(KFontChooser::tr("Bold Italic", "@item font"));
    styleListBox->setMinimumWidth(minimumListWidth(styleListBox));
    styleListBox->setMinimumHeight(minimumListHeight(styleListBox, visibleListSize));

    connect(styleListBox, &QListWidget::currentTextChanged, [this](const QString &style) {
        _k_style_chosen_slot(style);
    });

    if (flags & ShowDifferences) {
        styleListBox->setEnabled(false);
        connect(styleCheckbox, &QAbstractButton::toggled, styleListBox, &QWidget::setEnabled);
    }

    // set the the size list view / font size spinbox; and the font size checkbox
    row = flags & DisplayFrame ? 1 : 0;
    QHBoxLayout *sizeLayout = new QHBoxLayout();
    if (flags & ShowDifferences) {
        sizeCheckbox = new QCheckBox(KFontChooser::tr("Size", "@option:check"), page);
        sizeLayout->addWidget(sizeCheckbox, 0, Qt::AlignLeft);
        sizeLabel = nullptr;
    } else {
        sizeCheckbox = nullptr;
        sizeLabel = new QLabel(KFontChooser::tr("Size:", "@label:listbox Font size"), page);
        sizeLayout->addWidget(sizeLabel, 1, Qt::AlignLeft);
    }
    sizeLayout->addSpacing(checkBoxGap);
    sizeLayout->addSpacing(checkBoxGap);   // prevent label from eating border
    gridLayout->addLayout(sizeLayout, row, 2);

    ++row;

    sizeListBox = new QListWidget(page);
    sizeOfFont = new QDoubleSpinBox(page);
    sizeOfFont->setMinimum(4);
    sizeOfFont->setMaximum(512);
    sizeOfFont->setDecimals(1);
    sizeOfFont->setSingleStep(1);

    if (sizeIsRelativeState) {
        QString sizeIsRelativeCBText =
            KFontChooser::tr("Relative", "@item font size");
        QString sizeIsRelativeCBToolTipText =
            KFontChooser::tr("Font size<br /><i>fixed</i> or <i>relative</i><br />to environment", "@info:tooltip");
        QString sizeIsRelativeCBWhatsThisText =
            KFontChooser::tr("Here you can switch between fixed font size and font size "
               "to be calculated dynamically and adjusted to changing "
               "environment (e.g. widget dimensions, paper size).", "@info:whatsthis");
        sizeIsRelativeCheckBox = new QCheckBox(sizeIsRelativeCBText, page);
        sizeIsRelativeCheckBox->setTristate(flags & ShowDifferences);
        QGridLayout *sizeLayout2 = new QGridLayout();
        sizeLayout2->setVerticalSpacing(sizeListBoxGap);
        gridLayout->addLayout(sizeLayout2, row, 2);
        sizeLayout2->setColumnStretch(1, 1);   // to prevent text from eating the right border
        sizeLayout2->addWidget(sizeOfFont, 0, 0, 1, 2);
        sizeLayout2->addWidget(sizeListBox, 1, 0, 1, 2);
        sizeLayout2->addWidget(sizeIsRelativeCheckBox, 2, 0, Qt::AlignLeft);
        sizeIsRelativeCheckBox->setWhatsThis(sizeIsRelativeCBWhatsThisText);
        sizeIsRelativeCheckBox->setToolTip(sizeIsRelativeCBToolTipText);
    } else {
        sizeIsRelativeCheckBox = nullptr;
        QGridLayout *sizeLayout2 = new QGridLayout();
        sizeLayout2->setVerticalSpacing(sizeListBoxGap);
        gridLayout->addLayout(sizeLayout2, row, 2);
        sizeLayout2->addWidget(sizeOfFont, 0, 0);
        sizeLayout2->addWidget(sizeListBox, 1, 0);
    }

    // Populate with usual sizes, to determine minimum list width;
    // will be replaced later with correct sizes.
    fillSizeList();
    sizeListBox->setMinimumWidth(minimumListWidth(sizeListBox) + sizeListBox->fontMetrics().maxWidth());
    sizeListBox->setMinimumHeight(minimumListHeight(sizeListBox, visibleListSize));

    connect(sizeOfFont, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](const double size) {
        _k_size_value_slot(size);
    });

    connect(sizeListBox, &QListWidget::currentTextChanged, [this](const QString &size) {
        _k_size_chosen_slot(size);
    });

    if (flags & ShowDifferences) {
        sizeListBox->setEnabled(false);
        sizeOfFont->setEnabled(false);
        connect(sizeCheckbox, &QAbstractButton::toggled, sizeListBox, &QWidget::setEnabled);
        connect(sizeCheckbox, &QAbstractButton::toggled, sizeOfFont, &QWidget::setEnabled);
    }

    // Add the font preview into the lower part of mainLayout
    sampleEdit = new QTextEdit(page);
    sampleEdit->setAcceptRichText(false);
    QFont tmpFont(q->font().family(), 64, QFont::Black);
    sampleEdit->setFont(tmpFont);
    sampleEdit->setMinimumHeight(sampleEdit->fontMetrics().lineSpacing());
    // tr: A classical test phrase, with all letters of the English alphabet.
    // Replace it with a sample text in your language, such that it is
    // representative of language's writing system.
    // If you wish, you can input several lines of text separated by \n.
    q->setSampleText(KFontChooser::tr("The Quick Brown Fox Jumps Over The Lazy Dog"));
    sampleEdit->setTextCursor(QTextCursor(sampleEdit->document()));
    QString sampleEditWhatsThisText =
        KFontChooser::tr("This sample text illustrates the current settings. "
           "You may edit it to test special characters.", "@info:whatsthis");
    sampleEdit->setWhatsThis(sampleEditWhatsThisText);

    connect(q, &KFontChooser::fontSelected, q, [this](const QFont &font) {
        _k_displaySample(font);
    });

    mainLayout->addWidget(sampleEdit);

    // If the calling app sets FixedFontsOnly, respect its decision
    if (!usingFixed) {
        // Add a checkbox to toggle showing only monospace/fixed-width fonts
        onlyFixedCheckbox = new QCheckBox(KFontChooser::tr("Show only monospaced fonts", "@option:check"));
        onlyFixedCheckbox->setChecked(usingFixed);

        connect(onlyFixedCheckbox, &QAbstractButton::toggled, q, [this](const bool state) {
            q->setFont(selFont, state);
        });

        if (flags & ShowDifferences) { // In this mode follow the state of the familyCheckbox
            onlyFixedCheckbox->setEnabled(false);
            connect(familyCheckbox, &QAbstractButton::toggled, onlyFixedCheckbox, &QWidget::setEnabled);
        }
        mainLayout->addWidget(onlyFixedCheckbox);
    }
    // Finished setting up the chooser layout

    // lets initialize the display if possible
    if (usingFixed) {
        q->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont), true);
    } else {
        q->setFont(QGuiApplication::font(), false);
    }

    // check or uncheck or gray out the "relative" checkbox
    if (sizeIsRelativeState && sizeIsRelativeCheckBox) {
        q->setSizeIsRelative(*sizeIsRelativeState);
    }

    // Set focus to the size list as this is the most commonly changed property
    sizeListBox->setFocus();
}

void KFontChooser::setColor(const QColor &col)
{
    d->m_palette.setColor(QPalette::Active, QPalette::Text, col);
    QPalette pal = d->sampleEdit->palette();
    pal.setColor(QPalette::Active, QPalette::Text, col);
    d->sampleEdit->setPalette(pal);
    QTextCursor cursor = d->sampleEdit->textCursor();
    d->sampleEdit->selectAll();
    d->sampleEdit->setTextColor(col);
    d->sampleEdit->setTextCursor(cursor);
}

QColor KFontChooser::color() const
{
    return d->m_palette.color(QPalette::Active, QPalette::Text);
}

void KFontChooser::setBackgroundColor(const QColor &col)
{
    d->m_palette.setColor(QPalette::Active, QPalette::Base, col);
    QPalette pal = d->sampleEdit->palette();
    pal.setColor(QPalette::Active, QPalette::Base, col);
    d->sampleEdit->setPalette(pal);
}

QColor KFontChooser::backgroundColor() const
{
    return d->m_palette.color(QPalette::Active, QPalette::Base);
}

void KFontChooser::setSizeIsRelative(Qt::CheckState relative)
{
    // check or uncheck or gray out the "relative" checkbox
    if (d->sizeIsRelativeCheckBox) {
        if (Qt::PartiallyChecked == relative) {
            d->sizeIsRelativeCheckBox->setCheckState(Qt::PartiallyChecked);
        } else {
            d->sizeIsRelativeCheckBox->setCheckState((Qt::Checked == relative)  ? Qt::Checked : Qt::Unchecked);
        }
    }
}

Qt::CheckState KFontChooser::sizeIsRelative() const
{
    return d->sizeIsRelativeCheckBox
           ? d->sizeIsRelativeCheckBox->checkState()
           : Qt::PartiallyChecked;
}

QString KFontChooser::sampleText() const
{
    return d->sampleEdit->toPlainText();
}

void KFontChooser::setSampleText(const QString &text)
{
    d->sampleEdit->setPlainText(text);
}

void KFontChooser::setSampleBoxVisible(bool visible)
{
    d->sampleEdit->setVisible(visible);
}

QSize KFontChooser::sizeHint(void) const
{
    return minimumSizeHint();
}

void KFontChooser::enableColumn(int column, bool state)
{
    if (column & FamilyList) {
        d->familyListBox->setEnabled(state);
    }
    if (column & StyleList) {
        d->styleListBox->setEnabled(state);
    }
    if (column & SizeList) {
        d->sizeListBox->setEnabled(state);
        d->sizeOfFont->setEnabled(state);
    }
}

void KFontChooser::setFont(const QFont &aFont, bool onlyFixed)
{
    d->selFont = aFont;
    d->selectedSize = aFont.pointSizeF();
    if (d->selectedSize == -1) {
        d->selectedSize = QFontInfo(aFont).pointSizeF();
    }

    if (onlyFixed != d->usingFixed) {
        d->usingFixed = onlyFixed;
        d->fillFamilyListBox(d->usingFixed);
    }
    d->setupDisplay();
}

KFontChooser::FontDiffFlags KFontChooser::fontDiffFlags() const
{
    FontDiffFlags diffFlags = NoFontDiffFlags;

    if (d->familyCheckbox && d->familyCheckbox->isChecked()) {
        diffFlags |= FontDiffFamily;
    }

    if (d->styleCheckbox && d->styleCheckbox->isChecked()) {
        diffFlags |= FontDiffStyle;
    }

    if (d->sizeCheckbox && d->sizeCheckbox->isChecked()) {
        diffFlags |= FontDiffSize;
    }

    return diffFlags;
}

QFont KFontChooser::font() const
{
    return d->selFont;
}

void KFontChooser::Private::_k_family_chosen_slot(const QString &family)
{
    if (!signalsAllowed) {
        return;
    }
    signalsAllowed = false;

    QString currentFamily;
    if (family.isEmpty()) {
        Q_ASSERT(familyListBox->currentItem());
        if (familyListBox->currentItem()) {
            currentFamily = qtFamilies[familyListBox->currentItem()->text()];
        }
    } else {
        currentFamily = qtFamilies[family];
    }

    // Get the list of styles available in this family.
    QFontDatabase dbase;
    QStringList styles = dbase.styles(currentFamily);
    if (styles.isEmpty()) {
        // Avoid extraction, it is in kdeqt.po
        styles.append(TR_NOX("Normal", "QFontDatabase"));
    }

    // Filter style strings and add to the listbox.
    QString pureFamily;
    splitFontString(family, &pureFamily);
    QStringList filteredStyles;
    qtStyles.clear();
    styleIDs.clear();

    const QStringList origStyles = styles;
    for (const QString &style : origStyles) {
        // Sometimes the font database will report an invalid style,
        // that falls back back to another when set.
        // Remove such styles, by checking set/get round-trip.
        QFont testFont = dbase.font(currentFamily, style, 10);
        if (dbase.styleString(testFont) != style) {
            styles.removeAll(style);
            continue;
        }

        QString fstyle = tr("%1", "@item Font style").arg(style);
        if (!filteredStyles.contains(fstyle)) {
            filteredStyles.append(fstyle);
            qtStyles.insert(fstyle, style);
            styleIDs.insert(fstyle, styleIdentifier(testFont));
        }
    }
    styleListBox->clear();
    styleListBox->addItems(filteredStyles);

    // Try to set the current style in the listbox to that previous.
    int listPos = filteredStyles.indexOf(selectedStyle.isEmpty() ?  TR_NOX("Normal", "QFontDatabase") : selectedStyle);
    if (listPos < 0) {
        // Make extra effort to have Italic selected when Oblique was chosen,
        // and vice versa, as that is what the user would probably want.
        QString styleIt = tr("Italic", "@item font");
        QString styleOb = tr("Oblique", "@item font");
        for (int i = 0; i < 2; ++i) {
            int pos = selectedStyle.indexOf(styleIt);
            if (pos >= 0) {
                QString style = selectedStyle;
                style.replace(pos, styleIt.length(), styleOb);
                listPos = filteredStyles.indexOf(style);
                if (listPos >= 0) {
                    break;
                }
            }
            qSwap(styleIt, styleOb);
        }
    }
    styleListBox->setCurrentRow(listPos >= 0 ? listPos : 0);
    QString currentStyle = qtStyles[styleListBox->currentItem()->text()];

    // Recompute the size listbox for this family/style.
    qreal currentSize = setupSizeListBox(currentFamily, currentStyle);
    sizeOfFont->setValue(currentSize);

    selFont = dbase.font(currentFamily, currentStyle, int(currentSize));
    if (dbase.isSmoothlyScalable(currentFamily, currentStyle) && selFont.pointSize() == floor(currentSize)) {
        selFont.setPointSizeF(currentSize);
    }
    emit q->fontSelected(selFont);

    signalsAllowed = true;
}

void KFontChooser::Private::_k_style_chosen_slot(const QString &style)
{
    if (!signalsAllowed) {
        return;
    }
    signalsAllowed = false;

    QFontDatabase dbase;
    QString currentFamily = qtFamilies[familyListBox->currentItem()->text()];
    QString currentStyle;
    if (style.isEmpty()) {
        currentStyle = qtStyles[styleListBox->currentItem()->text()];
    } else {
        currentStyle = qtStyles[style];
    }

    // Recompute the size listbox for this family/style.
    qreal currentSize = setupSizeListBox(currentFamily, currentStyle);
    sizeOfFont->setValue(currentSize);

    selFont = dbase.font(currentFamily, currentStyle, int(currentSize));
    if (dbase.isSmoothlyScalable(currentFamily, currentStyle) && selFont.pointSize() == floor(currentSize)) {
        selFont.setPointSizeF(currentSize);
    }
    emit q->fontSelected(selFont);

    if (!style.isEmpty()) {
        selectedStyle = currentStyle;
    }

    signalsAllowed = true;
}

void KFontChooser::Private::_k_size_chosen_slot(const QString &size)
{
    if (!signalsAllowed) {
        return;
    }

    signalsAllowed = false;

    qreal currentSize;
    if (size.isEmpty()) {
        currentSize = QLocale::system().toDouble(sizeListBox->currentItem()->text());
    } else {
        currentSize = QLocale::system().toDouble(size);
    }

    // Reset the customized size slot in the list if not needed.
    if (customSizeRow >= 0 && selFont.pointSizeF() != currentSize) {
        sizeListBox->item(customSizeRow)->setText(standardSizeAtCustom);
        customSizeRow = -1;
    }

    sizeOfFont->setValue(currentSize);
    selFont.setPointSizeF(currentSize);
    emit q->fontSelected(selFont);

    if (!size.isEmpty()) {
        selectedSize = currentSize;
    }

    signalsAllowed = true;
}

void KFontChooser::Private::_k_size_value_slot(double dval)
{
    if (!signalsAllowed) {
        return;
    }
    signalsAllowed = false;

    // We compare with qreal, so convert for platforms where qreal != double.
    qreal val = qreal(dval);

    QFontDatabase dbase;
    QString family = qtFamilies[familyListBox->currentItem()->text()];
    QString style = qtStyles[styleListBox->currentItem()->text()];

    // Reset current size slot in list if it was customized.
    if (customSizeRow >= 0 && sizeListBox->currentRow() == customSizeRow) {
        sizeListBox->item(customSizeRow)->setText(standardSizeAtCustom);
        customSizeRow = -1;
    }

    bool canCustomize = true;

    // For Qt-bad-sizes workaround: skip this block unconditionally
    if (!dbase.isSmoothlyScalable(family, style)) {
        // Bitmap font, allow only discrete sizes.
        // Determine the nearest in the direction of change.
        canCustomize = false;
        int nrows = sizeListBox->count();
        int row = sizeListBox->currentRow();
        int nrow;
        if (val - selFont.pointSizeF() > 0) {
            for (nrow = row + 1; nrow < nrows; ++nrow)
                if (QLocale::system().toDouble(sizeListBox->item(nrow)->text()) >= val) {
                    break;
                }
        } else {
            for (nrow = row - 1; nrow >= 0; --nrow)
                if (QLocale::system().toDouble(sizeListBox->item(nrow)->text()) <= val) {
                    break;
                }
        }
        // Make sure the new row is not out of bounds.
        nrow = nrow < 0 ? 0 : nrow >= nrows ? nrows - 1 : nrow;
        // Get the size from the new row and set the spinbox to that size.
        val = QLocale::system().toDouble(sizeListBox->item(nrow)->text());
        sizeOfFont->setValue(val);
    }

    // Set the current size in the size listbox.
    int row = nearestSizeRow(val, canCustomize);
    sizeListBox->setCurrentRow(row);

    selectedSize = val;
    selFont.setPointSizeF(val);
    emit q->fontSelected(selFont);

    signalsAllowed = true;
}

void KFontChooser::Private::_k_displaySample(const QFont &font)
{
    sampleEdit->setFont(font);
    //sampleEdit->setCursorPosition(0);

    //QFontInfo a = QFontInfo(font);
    //qCDebug(KWidgetsAddonsLog) << "font: " << a.family () << ", " << a.pointSize ();
    //qCDebug(KWidgetsAddonsLog) << "      (" << font.toString() << ")\n";
}

int KFontChooser::Private::nearestSizeRow(qreal val, bool customize)
{
    qreal diff = 1000;
    int row = 0;
    for (int r = 0; r < sizeListBox->count(); ++r) {
        qreal cval = QLocale::system().toDouble(sizeListBox->item(r)->text());
        if (qAbs(cval - val) < diff) {
            diff = qAbs(cval - val);
            row = r;
        }
    }
    // For Qt-bad-sizes workaround: ignore value of customize, use true
    if (customize && diff > 0) {
        customSizeRow = row;
        standardSizeAtCustom = sizeListBox->item(row)->text();
        sizeListBox->item(row)->setText(formatFontSize(val));
    }
    return row;
}

qreal KFontChooser::Private::fillSizeList(const QList<qreal> &sizes_)
{
    if (!sizeListBox) {
        return 0; //assertion.
    }

    QList<qreal> sizes = sizes_;
    bool canCustomize = false;
    if (sizes.isEmpty()) {
        static const int c[] = {
            4,  5,  6,  7,
            8,  9,  10, 11,
            12, 13, 14, 15,
            16, 17, 18, 19,
            20, 22, 24, 26,
            28, 32, 48, 64,
            72, 80, 96, 128,
            0
        };
        for (int i = 0; c[i]; ++i) {
            sizes.append(c[i]);
        }
        // Since sizes were not supplied, this is a vector font,
        // and size slot customization is allowed.
        canCustomize = true;
    }

    // Insert sizes into the listbox.
    sizeListBox->clear();
    std::sort(sizes.begin(), sizes.end());
    for (qreal size : qAsConst(sizes)) {
        sizeListBox->addItem(formatFontSize(size));
    }

    // Return the nearest to selected size.
    // If the font is vector, the nearest size is always same as selected,
    // thus size slot customization is allowed.
    // If the font is bitmap, the nearest size need not be same as selected,
    // thus size slot customization is not allowed.
    customSizeRow = -1;
    int row = nearestSizeRow(selectedSize, canCustomize);
    return QLocale::system().toDouble(sizeListBox->item(row)->text());
}

qreal KFontChooser::Private::setupSizeListBox(const QString &family, const QString &style)
{
    QFontDatabase dbase;
    QList<qreal> sizes;
    const bool smoothlyScalable = dbase.isSmoothlyScalable(family, style);
    if (!smoothlyScalable) {
        const QList<int> smoothSizes = dbase.smoothSizes(family, style);
        for (int size : smoothSizes) {
            sizes.append(size);
        }
    }

    // Fill the listbox (uses default list of sizes if the given is empty).
    // Collect the best fitting size to selected size, to use if not smooth.
    qreal bestFitSize = fillSizeList(sizes);

    // Set the best fit size as current in the listbox if available.
    const QList<QListWidgetItem *> selectedSizeList = sizeListBox->findItems(
                                                        formatFontSize(bestFitSize), Qt::MatchExactly);
    if (!selectedSizeList.isEmpty()) {
        sizeListBox->setCurrentItem(selectedSizeList.first());
    }

    return bestFitSize;
}

void KFontChooser::Private::setupDisplay()
{
    QFontDatabase dbase;
    QString family = selFont.family().toLower();
    QString styleID = styleIdentifier(selFont);
    qreal size = selFont.pointSizeF();
    if (size == -1) {
        size = QFontInfo(selFont).pointSizeF();
    }

    int numEntries, i;

    // Direct family match.
    numEntries = familyListBox->count();
    for (i = 0; i < numEntries; ++i) {
        if (family == qtFamilies[familyListBox->item(i)->text()].toLower()) {
            familyListBox->setCurrentRow(i);
            break;
        }
    }

    // 1st family fallback.
    if (i == numEntries) {
        const int bracketPos = family.indexOf(QLatin1Char('['));
        if (bracketPos != -1) {
            family = family.leftRef(bracketPos).trimmed().toString();
            for (i = 0; i < numEntries; ++i) {
                if (family == qtFamilies[familyListBox->item(i)->text()].toLower()) {
                    familyListBox->setCurrentRow(i);
                    break;
                }
            }
        }
    }

    // 2nd family fallback.
    if (i == numEntries) {
        QString fallback = family + QLatin1String(" [");
        for (i = 0; i < numEntries; ++i) {
            if (qtFamilies[familyListBox->item(i)->text()].toLower().startsWith(fallback)) {
                familyListBox->setCurrentRow(i);
                break;
            }
        }
    }

    // 3rd family fallback.
    if (i == numEntries) {
        for (i = 0; i < numEntries; ++i) {
            if (qtFamilies[familyListBox->item(i)->text()].toLower().startsWith(family)) {
                familyListBox->setCurrentRow(i);
                break;
            }
        }
    }

    // Family fallback in case nothing matched. Otherwise, diff doesn't work
    if (i == numEntries) {
        familyListBox->setCurrentRow(0);
    }

    // By setting the current item in the family box, the available
    // styles and sizes for that family have been collected.
    // Try now to set the current items in the style and size boxes.

    // Set current style in the listbox.
    numEntries = styleListBox->count();
    for (i = 0; i < numEntries; ++i) {
        if (styleID == styleIDs[styleListBox->item(i)->text()]) {
            styleListBox->setCurrentRow(i);
            break;
        }
    }
    if (i == numEntries) {
        // Style not found, fallback.
        styleListBox->setCurrentRow(0);
    }

    // Set current size in the listbox.
    // If smoothly scalable, allow customizing one of the standard size slots,
    // otherwise just select the nearest available size.
    QString currentFamily = qtFamilies[familyListBox->currentItem()->text()];
    QString currentStyle = qtStyles[styleListBox->currentItem()->text()];
    bool canCustomize = dbase.isSmoothlyScalable(currentFamily, currentStyle);
    sizeListBox->setCurrentRow(nearestSizeRow(size, canCustomize));

    // Set current size in the spinbox.
    sizeOfFont->setValue(QLocale::system().toDouble(sizeListBox->currentItem()->text()));
}

void KFontChooser::getFontList(QStringList &list, uint fontListCriteria)
{
    QFontDatabase dbase;
    QStringList lstSys(dbase.families());

    // if we have criteria; then check fonts before adding
    if (fontListCriteria) {
        QStringList lstFonts;
        for (const QString &family : qAsConst(lstSys)) {
            if ((fontListCriteria & FixedWidthFonts) > 0 && !dbase.isFixedPitch(family)) {
                continue;
            }
            if (((fontListCriteria & (SmoothScalableFonts | ScalableFonts)) == ScalableFonts) &&
                    !dbase.isBitmapScalable(family)) {
                continue;
            }
            if ((fontListCriteria & SmoothScalableFonts) > 0 && !dbase.isSmoothlyScalable(family)) {
                continue;
            }
            lstFonts.append(family);
        }

        if ((fontListCriteria & FixedWidthFonts) > 0) {
            // Fallback.. if there are no fixed fonts found, it's probably a
            // bug in the font server or Qt.  In this case, just use 'fixed'
            if (lstFonts.isEmpty()) {
                lstFonts.append(QStringLiteral("fixed"));
            }
        }

        lstSys = lstFonts;
    }

    lstSys.sort();

    list = lstSys;
}

void KFontChooser::Private::setFamilyBoxItems(const QStringList &fonts)
{
    signalsAllowed = false;

    QStringList trfonts = translateFontNameList(fonts, &qtFamilies);
    familyListBox->clear();
    familyListBox->addItems(trfonts);

    signalsAllowed = true;
}

void KFontChooser::Private::fillFamilyListBox(bool onlyFixedFonts)
{
    QStringList fontList;
    getFontList(fontList, onlyFixedFonts ? FixedWidthFonts : 0);
    setFamilyBoxItems(fontList);
}

// Human-readable style identifiers returned by QFontDatabase::styleString()
// do not always survive round trip of QFont serialization/deserialization,
// causing wrong style in the style box to be highlighted when
// the chooser dialog is opened. This will cause the style to be changed
// when the dialog is closed and the user did not touch the style box.
// Hence, construct custom style identifiers sufficient for the purpose.
QString KFontChooser::Private::styleIdentifier(const QFont &font)
{
    const int weight = font.weight();
    QString styleName = font.styleName();
    // If the styleName property is empty and the weight is QFont::Normal, that
    // could mean it's a "Regular"-like style with the styleName part stripped
    // so that subsequent calls to setBold(true) can work properly (i.e. selecting
    // the "Bold" style provided by the font itself) without resorting to font
    // "emboldening" which looks ugly.
    // See also KConfigGroupGui::writeEntryGui().
    if (styleName.isEmpty() && weight == QFont::Normal) {
        QFontDatabase fdb;
        const QStringList styles = fdb.styles(font.family());
        for (const QString &style : styles) {
            // orderded by commonness, i.e. "Regular" is the most common
            if (style == QLatin1String("Regular")
                || style == QLatin1String("Normal")
                || style == QLatin1String("Book")
                || style == QLatin1String("Roman")) {
                styleName = style;
            } else {
                // nothing more we can do
            }
        }
    }

    const QChar comma(QLatin1Char(','));
    return   QString::number(weight) + comma
             + QString::number((int)font.style()) + comma
             + QString::number(font.stretch()) + comma
             + styleName;
}

#include "moc_kfontchooser.cpp"
