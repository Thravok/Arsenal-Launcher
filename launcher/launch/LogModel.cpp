#include "LogModel.h"

LogModel::LogModel(QObject* parent) : QAbstractListModel(parent)
{
    m_content.resize(m_maxLines);
}

int LogModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return m_numLines;
}

QVariant LogModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_numLines)
        return QVariant();

    auto row = index.row();
    auto realRow = (row + m_firstLine) % m_maxLines;
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        return m_content[realRow].line;
    }
    if (role == LevelRole) {
        return static_cast<int>(m_content[realRow].level.value());
    }

    return QVariant();
}

void LogModel::append(MessageLevel level, QString line)
{
    append({ { level, std::move(line) } });
}

void LogModel::append(const QList<QPair<MessageLevel, QString>>& lines)
{
    if (m_suspended || lines.isEmpty()) {
        return;
    }

    QList<QPair<MessageLevel, QString>> toAdd = lines;

    if (m_stopOnOverflow) {
        const int room = m_maxLines - m_numLines;
        if (room <= 0) {
            return;
        }
        if (toAdd.size() > room) {
            toAdd = toAdd.mid(0, room);
        }
        if (m_numLines + toAdd.size() >= m_maxLines) {
            toAdd.last() = { MessageLevel::Fatal, m_overflowMessage };
        }
    } else if (m_numLines + toAdd.size() > m_maxLines) {
        const int removeCount = qMin(m_numLines, m_numLines + toAdd.size() - m_maxLines);
        if (removeCount > 0) {
            beginRemoveRows(QModelIndex(), 0, removeCount - 1);
            m_firstLine = (m_firstLine + removeCount) % m_maxLines;
            m_numLines -= removeCount;
            endRemoveRows();
        }
        if (toAdd.size() > m_maxLines) {
            toAdd = toAdd.mid(toAdd.size() - m_maxLines);
        }
    }

    if (toAdd.isEmpty()) {
        return;
    }

    const int first = m_numLines;
    const int last = m_numLines + toAdd.size() - 1;
    beginInsertRows(QModelIndex(), first, last);
    for (const auto& entry : toAdd) {
        const int lineNum = (m_firstLine + m_numLines) % m_maxLines;
        m_content[lineNum].level = entry.first;
        m_content[lineNum].line = entry.second;
        m_numLines++;
    }
    endInsertRows();
}

void LogModel::suspend(bool suspend)
{
    m_suspended = suspend;
}

bool LogModel::suspended()
{
    return m_suspended;
}

void LogModel::clear()
{
    beginResetModel();
    m_firstLine = 0;
    m_numLines = 0;
    endResetModel();
}

QString LogModel::toPlainText()
{
    QString out;
    out.reserve(m_numLines * 80);
    for (int i = 0; i < m_numLines; i++) {
        QString& line = m_content[(m_firstLine + i) % m_maxLines].line;
        out.append(line + '\n');
    }
    out.squeeze();
    return out;
}

void LogModel::setMaxLines(int maxLines)
{
    // no-op
    if (maxLines == m_maxLines) {
        return;
    }
    // if it all still fits in the buffer, just resize it
    if (m_firstLine + m_numLines < m_maxLines) {
        m_maxLines = maxLines;
        m_content.resize(maxLines);
        return;
    }
    // otherwise, we need to reorganize the data because it crosses the wrap boundary
    QList<entry> newContent;
    newContent.resize(maxLines);
    if (m_numLines <= maxLines) {
        // if it all fits in the new buffer, just copy it over
        for (int i = 0; i < m_numLines; i++) {
            newContent[i] = m_content[(m_firstLine + i) % m_maxLines];
        }
        m_content.swap(newContent);
    } else {
        // if it doesn't fit, part of the data needs to be thrown away (the oldest log messages)
        int lead = m_numLines - maxLines;
        beginRemoveRows(QModelIndex(), 0, lead - 1);
        for (int i = 0; i < maxLines; i++) {
            newContent[i] = m_content[(m_firstLine + lead + i) % m_maxLines];
        }
        m_numLines = m_maxLines;
        m_content.swap(newContent);
        endRemoveRows();
    }
    m_firstLine = 0;
    m_maxLines = maxLines;
}

int LogModel::getMaxLines()
{
    return m_maxLines;
}

void LogModel::setStopOnOverflow(bool stop)
{
    m_stopOnOverflow = stop;
}

void LogModel::setOverflowMessage(const QString& overflowMessage)
{
    m_overflowMessage = overflowMessage;
}

void LogModel::setLineWrap(bool state)
{
    if (m_lineWrap != state) {
        m_lineWrap = state;
    }
}

bool LogModel::wrapLines() const
{
    return m_lineWrap;
}

void LogModel::setColorLines(bool state)
{
    if (m_colorLines != state) {
        m_colorLines = state;
    }
}

bool LogModel::colorLines() const
{
    return m_colorLines;
}

bool LogModel::isOverFlow()
{
    return m_numLines >= m_maxLines && m_stopOnOverflow;
}

MessageLevel LogModel::previousLevel()
{
    if (m_numLines > 0) {
        return m_content[m_numLines - 1].level;
    }
    return MessageLevel::Unknown;
}
