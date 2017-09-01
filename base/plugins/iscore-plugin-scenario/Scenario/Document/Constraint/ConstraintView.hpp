#pragma once
#include <QBrush>
#include <QGraphicsItem>
#include <QPen>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentViewConstants.hpp>
#include <Scenario/Document/Constraint/Temporal/Braces/LeftBrace.hpp>
#include <iscore_plugin_scenario_export.h>
#include <Scenario/Document/Constraint/ExecutionState.hpp>
#include <qnamespace.h>
#include <Scenario/Document/CommentBlock/TextItem.hpp>
class QGraphicsSceneMouseEvent;

struct ScenarioStyle;
namespace Scenario
{
class ConstraintPresenter;
class LeftBraceView;
class RightBraceView;
class SimpleTextItem;
class ConstraintMenuOverlay;
class ISCORE_PLUGIN_SCENARIO_EXPORT ConstraintView : public QObject,
                                                     public QGraphicsItem
{
  Q_OBJECT
  Q_INTERFACES(QGraphicsItem)

public:
  ConstraintView(ConstraintPresenter& presenter, QGraphicsItem* parent);
  virtual ~ConstraintView();

  static constexpr int static_type()
  {
    return QGraphicsItem::UserType + ItemType::Constraint;
  }
  int type() const final override
  {
    return static_type();
  }

  const ConstraintPresenter& presenter() const
  {
    return m_presenter;
  }

  void setInfinite(bool);
  bool infinite() const
  {
    return m_infinite;
  }

  void setExecuting(bool);
  void setDefaultWidth(double width);
  void setMaxWidth(bool infinite, double max);
  void setMinWidth(double min);
  void setHeight(double height);
  double setPlayWidth(double width);
  void setValid(bool val);

  double height() const
  {
    return m_height;
  }

  bool isSelected() const
  {
    return m_selected;
  }

  double defaultWidth() const
  {
    return m_defaultWidth;
  }

  double minWidth() const
  {
    return m_minWidth;
  }

  double maxWidth() const
  {
    return m_maxWidth;
  }

  double constraintAndRackHeight() const
  {
    return m_height;
  }

  double playWidth() const
  {
    return m_playWidth;
  }

  bool isValid() const
  {
    return m_validConstraint;
  }

  void setFocused(bool b)
  {
    m_hasFocus = b;
    update();
  }

  void updateOverlay();

  bool warning() const;
  void setWarning(bool warning);

  void setExecutionState(ConstraintExecutionState);
  const QBrush& constraintColor(const ScenarioStyle& skin) const;

  void updateLabelPos();
  void updateCounterPos();
  virtual void updatePaths() = 0;
  virtual void updatePlayPaths() = 0;

  void mousePressEvent(QGraphicsSceneMouseEvent* event) final override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) final override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) final override;

  LeftBraceView& leftBrace()
  {
    return m_leftBrace;
  }
  RightBraceView& rightBrace()
  {
    return m_rightBrace;
  }

signals:
  void requestOverlayMenu(QPointF);

protected:
  void setGripCursor();
  void setUngripCursor();

  LeftBraceView m_leftBrace;
  RightBraceView m_rightBrace;
  SimpleTextItem m_labelItem;
  SimpleTextItem m_counterItem;

  ConstraintMenuOverlay* m_overlay{};

  ConstraintPresenter& m_presenter;
  double m_defaultWidth{};
  double m_maxWidth{};
  double m_minWidth{};
  double m_playWidth{};

  double m_height{};

  bool m_selected{};
  bool m_infinite{};
  bool m_validConstraint{true};
  bool m_warning{};
  bool m_hasFocus{};
  bool m_waiting{};
  ConstraintExecutionState m_state{};
};

}
