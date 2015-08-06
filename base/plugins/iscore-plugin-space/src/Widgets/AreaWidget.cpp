#include "AreaWidget.hpp"
#include <QFormLayout>
#include "src/Space/SpaceModel.hpp"
#include "src/SpaceProcess.hpp"
#include "src/Area/AreaModel.hpp"
#include "src/Area/AreaParser.hpp"

#include "src/Commands/AddArea.hpp"

#include <iscore/document/DocumentInterface.hpp>
AreaWidget::AreaWidget(iscore::CommandStack& stack, const SpaceProcess& space, QWidget *parent):
    QWidget{parent},
    m_dispatcher{stack},
    m_space{space}
{
    auto lay = new QGridLayout;
    this->setLayout(lay);

    m_lineEdit = new QLineEdit;
    lay->addWidget(m_lineEdit);
    connect(m_lineEdit, &QLineEdit::editingFinished, this, &AreaWidget::on_formulaChanged);

    // This contains a list of comboboxes mapping each dimension of the space we're in,
    // to a parameter of the area
    {
        QGroupBox* dimBox = new QGroupBox;
        lay->addWidget(dimBox);
        m_spaceMappingLayout = new QFormLayout;
        dimBox->setLayout(m_spaceMappingLayout);
        for(const DimensionModel& dim : m_space.space().dimensions())
        {
            auto cb = new QComboBox;
            m_spaceMappingLayout->addRow(dim.name(), cb);
            connect(cb,  static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &AreaWidget::on_dimensionMapped);
        }
    }

    {
        // The remaining, non-space-mapped parameters are here.
        QGroupBox* paramBox = new QGroupBox;
        lay->addWidget(paramBox);
        m_paramMappingLayout = new QFormLayout;
        paramBox->setLayout(m_paramMappingLayout);
    }

    QPushButton* val = new QPushButton{"Create / update"};
    lay->addWidget(val);

    connect(val, &QPushButton::pressed, this, &AreaWidget::validate);
}

void AreaWidget::setActiveArea(const AreaModel *area)
{
    m_area = area;
    m_lineEdit->setText(m_area->toString());

    on_formulaChanged();

    if(m_area)
    {
        // Load all the data
        const auto& dim_map = area->spaceMapping();
        for(int symb_i = 0; symb_i < m_spaceMappingLayout->rowCount(); symb_i++)
        {
            auto label = static_cast<QLabel*>(m_spaceMappingLayout->itemAt(symb_i, QFormLayout::ItemRole::LabelRole)->widget());
            Q_ASSERT(label);
            auto cb = static_cast<QComboBox*>(m_spaceMappingLayout->itemAt(symb_i, QFormLayout::ItemRole::FieldRole)->widget());
            Q_ASSERT(cb);

            auto dim_map_it = std::find_if(dim_map.begin(), dim_map.end(), [&] (const auto& exp) { return GiNaC::ex_to<GiNaC::symbol>(exp.first).get_name() == label->text().toStdString(); });
            if(dim_map_it == dim_map.end())
            {
                // Error : the requested dimension does not seem to exist in our space.
                continue;
            }

            auto target_sym = GiNaC::ex_to<GiNaC::symbol>((*dim_map_it).second).get_name();
            // Find it in the combobox

            cb->setCurrentIndex(cb->findText(QString::fromStdString(target_sym)));
        }



        const auto& param_map = area->parameterMapping();
        // debug
        for(auto& elt : param_map.keys())
            qDebug() << elt;

        for(int param_i = 0; param_i < m_paramMappingLayout->rowCount(); param_i++)
        {
            auto it = m_paramMappingLayout->itemAt(param_i, QFormLayout::ItemRole::LabelRole);
            if(!it)
                continue;

            auto widg = it->widget();
            if(!widg)
                continue;

            auto label = qobject_cast<QLabel*>(widg);
            Q_ASSERT(label);
            auto line_edit = static_cast<QLineEdit*>(m_paramMappingLayout->itemAt(param_i, QFormLayout::ItemRole::FieldRole)->widget());
            Q_ASSERT(line_edit);
            line_edit->setEnabled(false);

            auto param_it = param_map.find(label->text());
            if(param_it == param_map.end())
            {
                // The parameter is not mapped
                continue;
            }

            if((*param_it).second.first)
            {
                // Storing a value
                line_edit->setText((*param_it).second.second.value.val.toString());
            }
            else
            {
                // Storing an address
                ISCORE_TODO;
            }

            line_edit->setEnabled(true);

        }

    }
}



void AreaWidget::on_formulaChanged()
{
    cleanup();

    AreaParser parser{m_lineEdit->text()};
    if(!parser.check())
    {
        return;
    }

    auto area = parser.result();
    for(int i = 0; i < m_spaceMappingLayout->rowCount(); i++)
    {
        auto cb = qobject_cast<QComboBox*>(m_spaceMappingLayout->itemAt(i, QFormLayout::ItemRole::FieldRole)->widget());
        cb->addItem(""); // What happens if left blank ? e.g. x=5 in 2D space ? Should just do the correct stuff.
        for(const auto& sym: area->symbols())
        {
            cb->addItem(QString::fromStdString(sym.get_name()));
        }
    }

    for(const auto& sym: area->symbols())
    {
        m_paramMappingLayout->addRow(QString::fromStdString(sym.get_name()), new QLineEdit);
    }
}


void AreaWidget::on_dimensionMapped(int)
{
    // Disable the line corresponding to the mapped dimensions in the m_paramMappingLayout.
    for(int param_i = 0; param_i < m_paramMappingLayout->rowCount(); param_i++)
    {
        auto it = m_paramMappingLayout->itemAt(param_i, QFormLayout::ItemRole::LabelRole);
        if(!it)
            continue;

        auto widg = it->widget();
        if(!widg)
            continue;

        auto label = qobject_cast<QLabel*>(widg);
        auto field = m_paramMappingLayout->itemAt(param_i, QFormLayout::ItemRole::FieldRole)->widget();
        field->setEnabled(true);

        auto param_name = label->text();
        for(int symb_i = 0; symb_i < m_spaceMappingLayout->rowCount(); symb_i++)
        {
            auto cb = static_cast<QComboBox*>(m_spaceMappingLayout->itemAt(symb_i, QFormLayout::ItemRole::FieldRole)->widget());
            if(cb->currentText() == param_name)
            {
                field->setEnabled(false);
            }
        }
    }
}


void AreaWidget::validate()
{
    // Make the dimension map
    QMap<QString, QString> dim_map;
    for(int symb_i = 0; symb_i < m_spaceMappingLayout->rowCount(); symb_i++)
    {
        auto label = static_cast<QLabel*>(m_spaceMappingLayout->itemAt(symb_i, QFormLayout::ItemRole::LabelRole)->widget());
        Q_ASSERT(label);
        auto cb = static_cast<QComboBox*>(m_spaceMappingLayout->itemAt(symb_i, QFormLayout::ItemRole::FieldRole)->widget());
        Q_ASSERT(cb);

        if(cb->currentText() != "")
        {
            dim_map.insert(label->text(), cb->currentText());
        }
    }


    QMap<QString, QPair<bool, iscore::FullAddressSettings>> param_map;
    for(int param_i = 0; param_i < m_paramMappingLayout->rowCount(); param_i++)
    {
        auto it = m_paramMappingLayout->itemAt(param_i, QFormLayout::ItemRole::LabelRole);
        if(!it)
            continue;

        auto widg = it->widget();
        if(!widg)
            continue;

        auto label = qobject_cast<QLabel*>(widg);
        Q_ASSERT(label);
        auto line_edit = static_cast<QLineEdit*>(m_paramMappingLayout->itemAt(param_i, QFormLayout::ItemRole::FieldRole)->widget());
        Q_ASSERT(line_edit);

        if(!line_edit->isEnabled())
            continue;

        iscore::FullAddressSettings addr;
        addr.value = iscore::Value::fromVariant(line_edit->text().toDouble());
        param_map.insert(label->text(), {true, addr});
    }


    if(m_area)
    {
        // Make a UpdateArea command.
    }
    else
    {
        auto cmd = new AddArea(iscore::IDocument::safe_path(m_space), m_lineEdit->text(), dim_map, param_map);
        m_dispatcher.submitCommand(cmd);
    }
}

void AreaWidget::cleanup()
{
    for(int i = 0; i < m_spaceMappingLayout->rowCount(); i++)
    {
        auto cb = qobject_cast<QComboBox*>(m_spaceMappingLayout->itemAt(i, QFormLayout::ItemRole::FieldRole)->widget());
        cb->clear();
    }

    while (m_paramMappingLayout->count() != 0)
    {
        auto item = m_paramMappingLayout->takeAt(0);
        item->widget()->deleteLater();
        delete item;
    }
}
