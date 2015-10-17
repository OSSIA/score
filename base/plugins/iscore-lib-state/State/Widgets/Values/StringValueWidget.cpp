#include "StringValueWidget.hpp"
#include <iscore/widgets/MarginLess.hpp>
#include <QLineEdit>
#include <QGridLayout>

StringValueWidget::StringValueWidget(const QString &value, QWidget *parent)
    : ValueWidget{parent}
{
    auto lay = new iscore::MarginLess<QGridLayout>;
    m_value = new QLineEdit;
    lay->addWidget(m_value);
    m_value->setText(value);
    this->setLayout(lay);
}

iscore::Value StringValueWidget::value() const
{
    return iscore::Value{m_value->text()};
}



CharValueWidget::CharValueWidget(QChar value, QWidget *parent)
    : ValueWidget{parent}
{
    auto lay = new iscore::MarginLess<QGridLayout>;
    m_value = new QLineEdit;
    m_value->setMaxLength(1);

    lay->addWidget(m_value);
    m_value->setText(value);
    this->setLayout(lay);
}

iscore::Value CharValueWidget::value() const
{
    auto txt = m_value->text();
    return iscore::Value{txt.length() > 0 ? txt[0] : QChar{}};
}
