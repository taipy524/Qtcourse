#ifndef COMBOBOXDELEGATE_H
#define COMBOBOXDELEGATE_H

#include <QStyledItemDelegate>
#include <QStringList>
#include <QVariantList>

// 自定义代理：把数据库里的整数（0/1/2）显示并编辑为“未知/男/女”
// 参考教材第 11 章示例 TComboBoxDelegate，增加了 值-文本 映射
class ComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ComboBoxDelegate(QObject *parent = nullptr);

    // values 与 texts 一一对应；editable 为 false 时只能下拉选择
    void setItems(const QVariantList &values, const QStringList &texts, bool editable = false);

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;

protected:
    QString displayText(const QVariant &value, const QLocale &locale) const override;

private:
    QVariantList m_values;
    QStringList  m_texts;
    bool         m_editable = false;

    int indexForValue(const QVariant &value) const;
};

#endif // COMBOBOXDELEGATE_H
