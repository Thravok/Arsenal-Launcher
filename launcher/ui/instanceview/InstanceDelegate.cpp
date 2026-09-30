// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2026 Arsenal Launcher Contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "InstanceDelegate.h"
#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QTextLayout>
#include <QTextOption>
#include <QtMath>

#include <QIcon>
#include <QTextEdit>
#include "BaseInstance.h"
#include "InstanceList.h"
#include "InstanceView.h"

// Origin: Qt
static void viewItemTextLayout(QTextLayout& textLayout, int lineWidth, qreal& height, qreal& widthUsed)
{
    height = 0;
    widthUsed = 0;
    textLayout.beginLayout();
    while (true) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
            break;
        if (line.textLength() == 0)
            break;
        line.setLineWidth(lineWidth);
        line.setPosition(QPointF(0, height));
        height += line.height();
        widthUsed = qMax(widthUsed, line.naturalTextWidth());
    }
    textLayout.endLayout();
}

ListViewDelegate::ListViewDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

static constexpr int kInstanceCardWidth = 120;
static constexpr int kInstanceIconSize = 64;
static constexpr int kInstanceCardRadius = 12;
static constexpr int kInstanceCardPadding = 10;
static constexpr int kPlayButtonSize = 22;

QRect ListViewDelegate::playButtonRect(const QRect& itemRect)
{
    // Sit on the lower-right of the icon tile area (above the label).
    const int tileBottom = itemRect.top() + kInstanceCardPadding + kInstanceIconSize;
    const int x = itemRect.right() - kInstanceCardPadding - kPlayButtonSize + 2;
    const int y = tileBottom - kPlayButtonSize + 2;
    return QRect(x, y, kPlayButtonSize, kPlayButtonSize);
}

bool ListViewDelegate::hitPlayButton(const QRect& itemRect, const QPoint& pos)
{
    return playButtonRect(itemRect).adjusted(-2, -2, 2, 2).contains(pos);
}

int ListViewDelegate::preferredItemWidth()
{
    return kInstanceCardWidth;
}

static void drawInstanceCard(QPainter* painter, const QStyleOptionViewItem& option)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // Full item card — icon + name share one bordered plate.
    const QRectF cardRect = QRectF(option.rect).adjusted(1, 1, -1, -1);

    QColor fill = option.palette.color(QPalette::Button);
    fill.setAlphaF(0.92);
    QColor border = option.palette.color(QPalette::WindowText);
    border.setAlphaF(0.14);

    if (option.state & QStyle::State_Selected) {
        fill = option.palette.color(QPalette::Base);
        fill.setAlphaF(0.95);
        border = option.palette.color(QPalette::Highlight);
        border.setAlphaF(1.0);
    } else if (option.state & QStyle::State_MouseOver) {
        border.setAlphaF(0.28);
        fill = fill.lighter(108);
    }

    painter->setPen(QPen(border, option.state & QStyle::State_Selected ? 2.0 : 1.0));
    painter->setBrush(fill);
    painter->drawRoundedRect(cardRect, kInstanceCardRadius, kInstanceCardRadius);

    if (option.state & QStyle::State_Selected) {
        QColor glow = option.palette.color(QPalette::Highlight);
        glow.setAlphaF(0.22);
        painter->setPen(QPen(glow, 4.0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(cardRect.adjusted(-1.5, -1.5, 1.5, 1.5), kInstanceCardRadius + 1, kInstanceCardRadius + 1);
    }

    painter->restore();
}

static void drawPlayButton(QPainter* painter, const QStyleOptionViewItem& option, BaseInstance* instance)
{
    if (!instance) {
        return;
    }

    const bool running = instance->isRunning();
    const bool canLaunch = instance->canLaunch() && !running;
    if (!canLaunch && !running) {
        return;
    }

    // Show on hover/selection, or always when running (stop affordance).
    const bool show = running || (option.state & (QStyle::State_MouseOver | QStyle::State_Selected));
    if (!show) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRect rect = ListViewDelegate::playButtonRect(option.rect);
    QColor fill = option.palette.color(QPalette::Highlight);
    // Prefer success green from palette link/bright if available — use a fixed Lunar-like green.
    fill = QColor(0x2f, 0xbf, 0x71);
    if (running) {
        fill = QColor(0xe0, 0x5a, 0x5a);
    }

    painter->setPen(Qt::NoPen);
    painter->setBrush(fill);
    painter->drawEllipse(rect);

    painter->setBrush(Qt::white);
    if (running) {
        const int inset = 7;
        painter->drawRoundedRect(rect.adjusted(inset, inset, -inset, -inset), 2, 2);
    } else {
        QPolygonF tri;
        const qreal cx = rect.center().x() + 1.0;
        const qreal cy = rect.center().y();
        tri << QPointF(cx - 4.0, cy - 5.0) << QPointF(cx - 4.0, cy + 5.0) << QPointF(cx + 5.0, cy);
        painter->drawPolygon(tri);
    }

    painter->restore();
}

void drawProgressOverlay(QPainter* painter, const QStyleOptionViewItem& option, const int value, const int maximum)
{
    if (maximum <= 0 || value >= maximum) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRectF cardRect = QRectF(option.rect).adjusted(1, 1, -1, -1);
    QPainterPath clip;
    clip.addRoundedRect(cardRect, kInstanceCardRadius, kInstanceCardRadius);
    painter->setClipPath(clip);

    QColor dim = option.palette.color(QPalette::Window);
    dim.setAlphaF(0.58);
    painter->fillRect(cardRect, dim);

    const qreal barHeight = 5.0;
    const qreal margin = 10.0;
    // Keep the progress bar under the icon, above the label.
    const qreal iconBottom = cardRect.top() + kInstanceCardPadding + kInstanceIconSize;
    QRectF track(cardRect.left() + margin, iconBottom - margin - barHeight, cardRect.width() - 2.0 * margin, barHeight);

    QColor trackColor = option.palette.color(QPalette::WindowText);
    trackColor.setAlphaF(0.18);
    painter->setPen(Qt::NoPen);
    painter->setBrush(trackColor);
    painter->drawRoundedRect(track, 2.5, 2.5);

    const qreal percent = qBound(0.0, static_cast<qreal>(value) / static_cast<qreal>(maximum), 1.0);
    QRectF fill = track;
    fill.setWidth(track.width() * percent);
    painter->setBrush(QColor(0x2f, 0xbf, 0x71));
    painter->drawRoundedRect(fill, 2.5, 2.5);

    painter->restore();
}

void drawBadges(QPainter* painter, const QStyleOptionViewItem& option, BaseInstance* instance, QIcon::Mode mode, QIcon::State state)
{
    QList<QString> pixmaps;
    if (instance->isRunning()) {
        pixmaps.append("status-running");
    } else if (instance->hasCrashed() || instance->hasVersionBroken()) {
        pixmaps.append("status-bad");
    }
    if (instance->hasUpdateAvailable()) {
        pixmaps.append("checkupdate");
    }

    static const int itemSide = 16;
    static const int spacing = 3;
    static const int inset = 6;
    const int itemsPerRow = qMax(1, qFloor(double(option.rect.width() - 2 * inset + spacing) / double(itemSide + spacing)));
    const int rows = qCeil((double)pixmaps.size() / (double)itemsPerRow);
    QListIterator<QString> it(pixmaps);
    painter->translate(option.rect.topLeft());
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < itemsPerRow; ++x) {
            if (!it.hasNext()) {
                painter->translate(-option.rect.topLeft());
                return;
            }
            auto icon = QIcon::fromTheme(it.next());
            // Keep badges top-left so they don't collide with play (bottom-right).
            const int xPos = inset + x * (itemSide + spacing);
            const int yPos = inset + y * (itemSide + spacing);
            QRect badgeRect(xPos, yPos, itemSide, itemSide);
            icon.paint(painter, badgeRect, Qt::AlignCenter, mode, state);
        }
    }
    painter->translate(-option.rect.topLeft());
}

static QSize viewItemTextSize(const QStyleOptionViewItem* option)
{
    QStyle* style = option->widget ? option->widget->style() : QApplication::style();
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    QTextLayout textLayout;
    textLayout.setTextOption(textOption);
    textLayout.setFont(option->font);
    textLayout.setText(option->text);
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, option, option->widget) + 1;
    QRect bounds(0, 0, kInstanceCardWidth - 2 * textMargin - 2 * kInstanceCardPadding, 600);
    qreal height = 0, widthUsed = 0;
    viewItemTextLayout(textLayout, bounds.width(), height, widthUsed);
    // Cap to two lines for cleaner Lunar-like labels.
    const qreal lineHeight = option->fontMetrics.lineSpacing();
    height = qMin(height, lineHeight * 2.0);
    const QSize size(qCeil(widthUsed), qCeil(height));
    return QSize(size.width() + 2 * textMargin, size.height());
}

void ListViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    painter->save();
    painter->setClipRect(opt.rect);
    painter->setRenderHint(QPainter::Antialiasing, true);

    opt.features |= QStyleOptionViewItem::WrapText;
    opt.text = index.data().toString();
    opt.textElideMode = Qt::ElideRight;
    opt.displayAlignment = Qt::AlignTop | Qt::AlignHCenter;

    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();

    drawInstanceCard(painter, opt);

    const int iconSize = kInstanceIconSize;
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, opt.widget) + 1;
    QRect contentRect = opt.rect.adjusted(kInstanceCardPadding, kInstanceCardPadding, -kInstanceCardPadding, -kInstanceCardPadding);
    QRect iconbox = contentRect;
    QRect textRect = contentRect;
    textRect.adjust(textMargin / 2, iconSize + textMargin + 2, -textMargin / 2, 0);

    QIcon::Mode mode = QIcon::Normal;
    if (!(opt.state & QStyle::State_Enabled))
        mode = QIcon::Disabled;
    else if (opt.state & QStyle::State_Selected)
        mode = QIcon::Selected;
    QIcon::State state = opt.state & QStyle::State_Open ? QIcon::On : QIcon::Off;

    {
        iconbox.setHeight(iconSize);
        opt.icon.paint(painter, iconbox, Qt::AlignCenter, mode, state);
    }

    QPalette::ColorGroup cg = opt.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(opt.state & QStyle::State_Active))
        cg = QPalette::Inactive;
    painter->setPen(opt.palette.color(cg, QPalette::Text));

    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    textOption.setTextDirection(opt.direction);
    textOption.setAlignment(QStyle::visualAlignment(opt.direction, opt.displayAlignment));
    QTextLayout textLayout;
    textLayout.setTextOption(textOption);
    textLayout.setFont(opt.font);
    textLayout.setText(opt.text);

    qreal width, height;
    viewItemTextLayout(textLayout, textRect.width(), height, width);

    const int maxLines = qMin(2, textLayout.lineCount());

    const QRect layoutRect = QStyle::alignedRect(opt.direction, opt.displayAlignment, QSize(textRect.width(), int(height)), textRect);
    const QPointF position = layoutRect.topLeft();
    for (int i = 0; i < maxLines; ++i) {
        const QTextLine line = textLayout.lineAt(i);
        line.draw(painter, position);
    }

    auto instance = (BaseInstance*)index.data(InstanceList::InstancePointerRole).value<void*>();
    if (instance) {
        drawBadges(painter, opt, instance, mode, state);
        drawPlayButton(painter, opt, instance);
    }

    drawProgressOverlay(painter, opt, index.data(InstanceViewRoles::ProgressValueRole).toInt(),
                        index.data(InstanceViewRoles::ProgressMaximumRole).toInt());

    painter->restore();
}

QSize ListViewDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.features |= QStyleOptionViewItem::WrapText;
    opt.text = index.data().toString();
    opt.textElideMode = Qt::ElideRight;
    opt.displayAlignment = Qt::AlignTop | Qt::AlignHCenter;

    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &option, opt.widget) + 1;
    int height = kInstanceIconSize + textMargin * 2 + 4 + 2 * kInstanceCardPadding;
    QSize szz = viewItemTextSize(&opt);
    height += szz.height();
    return QSize(kInstanceCardWidth, height);
}

class NoReturnTextEdit : public QTextEdit {
    Q_OBJECT
   public:
    explicit NoReturnTextEdit(QWidget* parent) : QTextEdit(parent)
    {
        setTextInteractionFlags(Qt::TextEditorInteraction);
        setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    }
    bool event(QEvent* event) override
    {
        auto eventType = event->type();
        if (eventType == QEvent::KeyPress || eventType == QEvent::KeyRelease) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            auto key = keyEvent->key();
            if ((key == Qt::Key_Return || key == Qt::Key_Enter) && eventType == QEvent::KeyPress) {
                emit editingDone();
                return true;
            }
            if (key == Qt::Key_Tab) {
                return true;
            }
        }
        return QTextEdit::event(event);
    }
   signals:
    void editingDone();
};

void ListViewDelegate::updateEditorGeometry(QWidget* editor,
                                            const QStyleOptionViewItem& option,
                                            [[maybe_unused]] const QModelIndex& index) const
{
    const int iconSize = kInstanceIconSize;
    QRect textRect = option.rect;
    textRect.adjust(kInstanceCardPadding, iconSize + kInstanceCardPadding + 2, -kInstanceCardPadding, -kInstanceCardPadding);
    editor->setGeometry(textRect);
}

void ListViewDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto text = index.data(Qt::EditRole).toString();
    QTextEdit* realEditor = qobject_cast<NoReturnTextEdit*>(editor);
    realEditor->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    realEditor->append(text);
    realEditor->selectAll();
    realEditor->document()->clearUndoRedoStacks();
}

void ListViewDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QTextEdit* realEditor = qobject_cast<NoReturnTextEdit*>(editor);
    QString text = realEditor->toPlainText();
    text.replace(QChar('\n'), QChar(' '));
    text = text.trimmed();
    text.truncate(128);
    if (text.size() != 0) {
        const auto before = model->data(index).toString();
        model->setData(index, text);
        emit textChanged(before, text);
    }
}

QWidget* ListViewDelegate::createEditor(QWidget* parent,
                                        [[maybe_unused]] const QStyleOptionViewItem& option,
                                        [[maybe_unused]] const QModelIndex& index) const
{
    auto editor = new NoReturnTextEdit(parent);
    connect(editor, &NoReturnTextEdit::editingDone, this, &ListViewDelegate::editingDone);
    return editor;
}

void ListViewDelegate::editingDone()
{
    NoReturnTextEdit* editor = qobject_cast<NoReturnTextEdit*>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}

#include "InstanceDelegate.moc"
