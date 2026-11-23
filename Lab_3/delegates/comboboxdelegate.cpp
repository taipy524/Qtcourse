#include "comboboxdelegate.h"

#include <QComboBox>

ComboBoxDelegate::ComboBoxDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void ComboBoxDelegate::setItems(const QVariantList &values, const QStringList &texts, bool editable)
{
    m_values = values;
    m_texts = texts;
    m_editable = editable;
}

QString ComboBoxDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
    Q_UNUSED(locale)
    const int index = indexForValue(value);
    if (index >= 0 && index < m_texts.size())
        return m_texts.at(index);
    return QStyledItemDelegate::displayText(value, locale);
}

int ComboBoxDelegate::indexForValue(const QVariant &value) const
{
    for (int i = 0; i < m_values.size(); ++i) {
        if (m_values.at(i) == value)
            return i;
    }
    // 数据库里是 int，模型给的可能是 QString，做一次宽松比较
    const QString text = value.toString();
    for (int i = 0; i < m_values.size(); ++i) {
        if (m_values.at(i).toString() == text)
            return i;
    }
    return -1;
}

QWidget *ComboBoxDelegate::createEditor(QWidget *parent,
                                        const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    QComboBox *editor = new QComboBox(parent);
    editor->setEditable(m_editable);
    for (int i = 0; i < m_texts.size(); ++i)
        editor->addItem(m_texts.at(i), m_values.value(i));
    return editor;
}

void ComboBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *comboBox = static_cast<QComboBox *>(editor);
    const QVariant value = index.model()->data(index, Qt::EditRole);
    const int pos = indexForValue(value);
    if (pos >= 0)
        comboBox->setCurrentIndex(pos);
}

void ComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    QComboBox *comboBox = static_cast<QComboBox *>(editor);
    model->setData(index, comboBox->currentData(), Qt::EditRole);
}

void ComboBoxDelegate::updateEditorGeometry(QWidget *editor,
                                            const QStyleOptionViewItem &option,
                                            const QModelIndex &index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}
