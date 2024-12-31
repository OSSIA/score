#pragma once
#include <Device/Protocol/DeviceSettings.hpp>
#include <Device/Protocol/ProtocolSettingsWidget.hpp>

#include <Protocols/Bitfocus/BitfocusSpecificSettings.hpp>

#include <QJSEngine>

#include <verdigris>

class QStackedLayout;
class QLineEdit;
class QVBoxLayout;
class QFormLayout;
class QSpinBox;
class QWidget;
class QLabel;

namespace Protocols
{

class BasicTCPWidget;
class WebsocketClientWidget;
class RateWidget;

class BitfocusProtocolSettingsWidget final : public Device::ProtocolSettingsWidget
{
public:
  explicit BitfocusProtocolSettingsWidget(QWidget* parent = nullptr);

  Device::DeviceSettings getSettings() const override;
  void setSettings(const Device::DeviceSettings& settings) override;

  using Device::ProtocolSettingsWidget::checkForChanges;

private:
  void updateFields();
  QFormLayout* m_rootLayout{};
  QLineEdit* m_deviceNameEdit{};
  BitfocusSpecificSettings m_settings;
  QWidget* m_subWidget{};
  QVBoxLayout* m_subForm{};

  // Get the configuration for each widget
  struct widget
  {
    QLabel* label{};
    QWidget* widget{};
    std::function<QVariant()> getValue;
  };

  std::map<QString, widget> m_widgets;
  QJSEngine m_uiEngine;
};
}