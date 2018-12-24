#pragma once
#include <QCheckBox>
#include <QSpinBox>
#include <QHBoxLayout>
#include <ossia/detail/optional.hpp>
#include <wobjectdefs.h>

namespace Protocols
{
class RateWidget final : public QWidget
{
  QCheckBox* m_check{};
  QSpinBox* m_spin{};

  W_OBJECT(RateWidget)

  public:
  RateWidget(QWidget* parent = nullptr)
    : QWidget{parent}
  {
    auto lay = new QHBoxLayout;

    lay->addWidget(m_check);
    lay->addWidget(m_spin);

    connect(m_check, &QCheckBox::toggled,
            this, [=] (bool t) {
      rateChanged(ossia::optional<int>{});
      m_spin->setEnabled(t);
    });

    m_check->setChecked(false);
    m_spin->setEnabled(false);

    setLayout(lay);
  }

  ossia::optional<int> rate() const noexcept
  {
    if(!m_check->isChecked())
    {
      return ossia::optional<int>{};
    }
    else
    {
      return m_spin->value();
    }
  }

  void setRate(ossia::optional<int> r) noexcept
  {
    if(r)
    {
      m_check->setChecked(true);
      m_spin->setValue(*r);
    }
    else
    {
      m_check->setChecked(false);
    }
  }

  void rateChanged(ossia::optional<int> v)
  W_SIGNAL(rateChanged, v);
};


}

W_REGISTER_ARGTYPE(ossia::optional<int>)
Q_DECLARE_METATYPE(ossia::optional<int>)
