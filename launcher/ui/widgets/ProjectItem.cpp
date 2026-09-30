#include "ProjectItem.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include "Common.h"

namespace {
constexpr int kCardRadius = 12;
constexpr int kCardPadding = 8;
constexpr int kRowHeight = 80;

void drawProjectCard(QPainter* painter, const QStyleOptionViewItem& option, bool selected, bool hovered)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRectF cardRect = QRectF(option.rect).adjusted(2.0, 1.0, -2.0, -1.0);
    QColor fill;
    QColor border = option.palette.color(QPalette::WindowText);
    border.setAlphaF(0.32);

    if (selected) {
        fill = option.palette.color(QPalette::Highlight);
        fill.setAlphaF(0.34);
        border = option.palette.color(QPalette::Highlight);
        border.setAlphaF(0.9);
    } else if (hovered) {
        fill = option.palette.color(QPalette::Button);
        fill.setAlphaF(0.94);
        border.setAlphaF(0.48);
    } else {
        fill = option.palette.color(QPalette::Button);
        fill.setAlphaF(0.88);
    }

    painter->setPen(QPen(border, 1.0));
    painter->setBrush(fill);
    painter->drawRoundedRect(cardRect, kCardRadius, kCardRadius);

    QLinearGradient sheen(cardRect.topLeft(), cardRect.topLeft() + QPointF(0, 10));
    QColor hi = Qt::white;
    hi.setAlphaF(selected ? 0.2 : 0.12);
    QColor clear = hi;
    clear.setAlphaF(0.0);
    sheen.setColorAt(0.0, hi);
    sheen.setColorAt(1.0, clear);

    QPainterPath clip;
    clip.addRoundedRect(cardRect, kCardRadius, kCardRadius);
    painter->setClipPath(clip);
    painter->fillRect(QRectF(cardRect.left(), cardRect.top(), cardRect.width(), 10.0), sheen);
    painter->restore();
}
}  // namespace

ProjectItemDelegate::ProjectItemDelegate(QWidget* parent) : QStyledItemDelegate(parent) {}

QSize ProjectItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index);
    const int icon = option.decorationSize.isValid() ? option.decorationSize.height() : 48;
    const int width = option.rect.width() > 0 ? option.rect.width() : 280;
    return QSize(width, qMax(icon + 2 * kCardPadding, kRowHeight));
}

void ProjectItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const bool isInstalled = index.data(UserDataTypes::INSTALLED).toBool();
    const bool isChecked = opt.checkState == Qt::Checked;
    const bool isSelected = option.state & QStyle::State_Selected;
    const bool isHovered = option.state & QStyle::State_MouseOver;

    const QStyle* style = opt.widget == nullptr ? QApplication::style() : opt.widget->style();

    drawProjectCard(painter, opt, isSelected, isHovered);

    QRect rect = opt.rect.adjusted(kCardPadding, kCardPadding, -kCardPadding, -kCardPadding);

    if (opt.features & QStyleOptionViewItem::HasCheckIndicator) {
        QStyleOptionViewItem checkboxOpt = makeCheckboxStyleOption(opt, style);
        style->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &checkboxOpt, painter, opt.widget);
        rect.setLeft(checkboxOpt.rect.right() + 8);
    }

    if (!isSelected && !isChecked && isInstalled)
        painter->setOpacity(0.45);

    int iconWidth = 0;
    int iconHeight = 0;
    if (!opt.icon.isNull()) {
        const QSize iconSize = opt.decorationSize.isValid() ? opt.decorationSize : QSize(48, 48);
        iconWidth = iconSize.width();
        iconHeight = iconSize.height();
        const int y = rect.y() + qMax(0, (rect.height() - iconHeight) / 2);
        if (iconWidth > 0 && iconHeight > 0)
            opt.icon.paint(painter, rect.x(), y, iconWidth, iconHeight);
    }

    const int textX = rect.x() + (iconWidth > 0 ? iconWidth + 12 : 0);
    const int remainingWidth = qMax(0, rect.right() - textX);
    QRect textRect(textX, rect.y(), remainingWidth, rect.height());

    QPalette::ColorGroup cg = opt.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(opt.state & QStyle::State_Active))
        cg = QPalette::Inactive;

    {
        auto title = index.data(UserDataTypes::TITLE).toString();
        if (isInstalled)
            title = tr("%1 [installed]").arg(title);

        QFont font = opt.font;
        font.setPointSize(font.pointSize() + 2);
        font.setBold(isChecked || isSelected);
        painter->setFont(font);
        painter->setPen(opt.palette.color(cg, isSelected ? QPalette::HighlightedText : QPalette::Text));

        const int titleHeight = QFontMetrics(font).height();
        painter->drawText(textRect.x(), textRect.y(), textRect.width(), titleHeight, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
                          title);
        textRect.setTop(textRect.y() + titleHeight + 2);
    }

    {
        auto description = index.data(UserDataTypes::DESCRIPTION).toString().simplified();
        painter->setFont(opt.font);
        QColor descColor = opt.palette.color(cg, isSelected ? QPalette::HighlightedText : QPalette::Text);
        if (!isSelected)
            descColor.setAlphaF(0.72);
        painter->setPen(descColor);

        QTextLayout textLayout(description, opt.font);
        qreal height = 0;
        auto cutText = viewItemTextLayout(textLayout, remainingWidth, height);

        QString shown;
        if (!cutText.isEmpty())
            shown = cutText.first().second;
        if (cutText.size() > 1) {
            if (textRect.height() <= 2.5 * opt.fontMetrics.height()) {
                shown = opt.fontMetrics.elidedText(description, opt.textElideMode, remainingWidth);
            } else {
                shown += QLatin1Char(' ');
                if (cutText.size() > 2)
                    shown += opt.fontMetrics.elidedText(cutText.at(1).second, opt.textElideMode, cutText.at(1).first);
                else
                    shown += cutText.at(1).second;
            }
        }

        painter->drawText(textRect, Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, shown);
    }

    painter->restore();
}

bool ProjectItemDelegate::editorEvent(QEvent* event,
                                      QAbstractItemModel* model,
                                      const QStyleOptionViewItem& option,
                                      const QModelIndex& index)
{
    if (!(event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseButtonPress ||
          event->type() == QEvent::MouseButtonDblClick))
        return false;

    auto mouseEvent = static_cast<QMouseEvent*>(event);

    if (mouseEvent->button() != Qt::LeftButton)
        return false;

    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const QStyle* style = opt.widget == nullptr ? QApplication::style() : opt.widget->style();

    const QStyleOptionViewItem checkboxOpt = makeCheckboxStyleOption(opt, style);

    if (!checkboxOpt.rect.contains(mouseEvent->pos().x(), mouseEvent->pos().y()))
        return false;

    if (event->type() != QEvent::MouseButtonRelease)
        return true;

    emit checkboxClicked(index);
    return true;
}

QStyleOptionViewItem ProjectItemDelegate::makeCheckboxStyleOption(const QStyleOptionViewItem& opt, const QStyle* style) const
{
    QStyleOptionViewItem checkboxOpt = opt;

    checkboxOpt.state &= ~QStyle::State_HasFocus;

    if (checkboxOpt.checkState == Qt::Checked)
        checkboxOpt.state |= QStyle::State_On;
    else
        checkboxOpt.state |= QStyle::State_Off;

    QRect checkboxRect = style->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &checkboxOpt, opt.widget);
    checkboxOpt.rect = QRect(opt.rect.x() + kCardPadding + 4, opt.rect.y() + (opt.rect.height() / 2 - checkboxRect.height() / 2),
                             checkboxRect.width(), checkboxRect.height());

    return checkboxOpt;
}
