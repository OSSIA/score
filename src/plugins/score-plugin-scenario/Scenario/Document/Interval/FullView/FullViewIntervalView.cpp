// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "FullViewIntervalView.hpp"

#include "FullViewIntervalPresenter.hpp"

#include <Process/Style/ScenarioStyle.hpp>
#include <Scenario/Document/Interval/IntervalView.hpp>
#include <score/graphics/GraphicsItem.hpp>

#include <QColor>
#include <QCursor>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include <QPen>
#include <QtGlobal>
#include <qnamespace.h>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Scenario::FullViewIntervalView)

namespace Scenario
{
FullViewIntervalView::FullViewIntervalView(
    FullViewIntervalPresenter& presenter,
    QGraphicsItem* parent)
    : IntervalView{presenter, parent}
{
  this->setCacheMode(QGraphicsItem::NoCache);
  this->setParentItem(parent);
  this->setFlag(ItemIsSelectable);

  this->setZValue(ZPos::Interval);
}

FullViewIntervalView::~FullViewIntervalView() {}

void FullViewIntervalView::updatePaths() { }
void FullViewIntervalView::drawPaths(
      QPainter& p,
      QRectF visibleRect,
      const score::Brush& defaultColor,
      const Process::Style& skin)
{
  solidPath = QPainterPath{};
  dashedPath = QPainterPath{};
  playedSolidPath = QPainterPath{};
  playedDashedPath = QPainterPath{};
  waitingDashedPath = QPainterPath{};

  const qreal min_w = minWidth();
  const qreal max_w = maxWidth();
  const qreal def_w = defaultWidth();
  const qreal play_w = m_waiting ? playWidth() : 0.;
  const qreal gui_w = m_guiWidth;
  const auto& solidPen = skin.IntervalSolidPen(defaultColor);
  const auto& dashPen = skin.IntervalDashPen(defaultColor);

  const auto& solidPlayPen = skin.IntervalSolidPen(skin.IntervalPlayFill());
  const auto dashPlayPen = skin.IntervalDashPen(skin.IntervalPlayFill());
  const auto& waitingPlayPen = skin.IntervalDashPen(skin.IntervalWaitingDashFill());

  // Paths
  if (play_w <= 0.)
  {
    if (infinite())
    {
      if (min_w != 0.)
      {
        p.setPen(solidPen);
        p.drawLine(QPointF{0, 0}, QPointF{min_w, 0.});
      }

      // TODO end state should be hidden
      {
        p.setPen(dashPen);
        p.drawLine(QPointF{min_w, 0}, QPointF{def_w, 0.});
      }
    }
    else if (min_w == max_w) // TODO rigid()
    {
      p.setPen(solidPen);
      p.drawLine(QPointF{0, 0}, QPointF{def_w, 0.});
    }
    else
    {
      if (min_w != 0.)
      {
        p.setPen(solidPen);
        p.drawLine(QPointF{0, 0}, QPointF{min_w, 0.});
      }
      p.setPen(dashPen);
      p.drawLine(QPointF{min_w, 0}, QPointF{max_w, 0.});
    }
  }
  else
  {
    if (infinite())
    {
      if (min_w != 0.)
      {
        const auto min_pt = std::min(play_w, min_w);
        p.setPen(solidPlayPen);
        p.drawLine(QPointF{0, 0}, QPointF{min_pt, 0.});

        if(min_pt < min_w)
        {
          p.setPen(solidPen);
          p.drawLine(QPointF{min_pt, 0}, QPointF{min_w, 0.});
        }
      }

      if (play_w > min_w)
      {
        const auto min_pt = std::min(def_w, play_w);

        if(min_pt < def_w)
        {
          p.setPen(waitingPlayPen);
          p.drawLine(QPointF{min_w, 0.}, QPointF{def_w, 0.});
        }

        p.setPen(dashPlayPen);
        p.drawLine(QPointF{min_w, 0.}, QPointF{min_pt, 0.});
      }
      else
      {
        p.setPen(dashPen);
        p.drawLine(QPointF{min_w, 0}, QPointF{def_w, 0.});
      }
    }
    else if (min_w == max_w) // TODO rigid()
    {
      const auto min_pt = std::min(play_w, def_w);
      p.setPen(solidPlayPen);
      p.drawLine(QPointF{0, 0}, QPointF{min_pt, 0.});

      if(min_pt < def_w)
      {
        p.setPen(solidPen);
        p.drawLine(QPointF{min_pt, 0}, QPointF{def_w, 0.});
      }
    }
    else
    {
      if (min_w != 0.)
      {
        const auto min_pt = std::min(play_w, min_w);
        p.setPen(solidPlayPen);
        p.drawLine(QPointF{0, 0}, QPointF{min_pt, 0.});
        if(min_pt < min_w)
        {
          p.setPen(solidPen);
          p.drawLine(QPointF{min_pt, 0}, QPointF{min_w, 0.});
          solidPath.lineTo(min_w, 0.);
        }
      }

      if (play_w > min_w)
      {
        if(max_w > play_w)
        {
          p.setPen(waitingPlayPen);
          p.drawLine(QPointF{min_w, 0.}, QPointF{max_w, 0.});
        }

        p.setPen(dashPlayPen);
        p.drawLine(QPointF{min_w, 0.}, QPointF{play_w, 0.});
      }
      else
      {
        p.setPen(dashPen);
        p.drawLine(QPointF{min_w, 0.}, QPointF{max_w, 0.});
      }
    }
  }
}

void FullViewIntervalView::updatePlayPaths()
{
  /*
  playedSolidPath = QPainterPath{};
  playedDashedPath = QPainterPath{};
  waitingDashedPath = QPainterPath{};

  const qreal min_w = minWidth();
  const qreal max_w = maxWidth();
  const qreal def_w = defaultWidth();
  const qreal play_w = playWidth();

  // Paths
  if (play_w <= 0.)
  {
    return;
  }
  else
  {
    if (infinite())
    {
      if (min_w != 0.)
      {
        playedSolidPath.lineTo(std::min(play_w, min_w), 0.);
      }

      if (play_w > min_w)
      {
        playedDashedPath.moveTo(min_w, 0.);
        playedDashedPath.lineTo(std::min(def_w, play_w), 0.);

        waitingDashedPath.moveTo(min_w, 0.);
        waitingDashedPath.lineTo(def_w, 0.);
      }
    }
    else if (min_w == max_w) // TODO rigid()
    {
      playedSolidPath.lineTo(std::min(play_w, def_w), 0.);
    }
    else
    {
      if (min_w != 0.)
      {
        playedSolidPath.lineTo(std::min(play_w, min_w), 0.);
      }

      if (play_w > min_w)
      {
        playedDashedPath.moveTo(min_w, 0.);
        playedDashedPath.lineTo(play_w, 0.);

        waitingDashedPath.moveTo(min_w, 0.);
        waitingDashedPath.lineTo(max_w, 0.);
      }
    }
  }
  */
}

void FullViewIntervalView::updateOverlayPos() {}

void FullViewIntervalView::setSelected(bool selected)
{
  m_selected = selected;
  setZValue(m_selected ? ZPos::SelectedInterval : ZPos::Interval);
  update();
}
QRectF FullViewIntervalView::boundingRect() const
{
  return {0,
          -3,
          qreal(std::max(defaultWidth(), m_guiWidth)) + 3,
          qreal(intervalAndRackHeight()) + 6};
}

void FullViewIntervalView::paint(
    QPainter* p,
    const QStyleOptionGraphicsItem* option,
    QWidget* widget)
{
  auto view = ::getView(*this);
  if(!view)
    return;

  auto rect = boundingRect();
  QPointF sceneDrawableTopLeft = view->mapToScene(-10, 0);
  QPointF sceneDrawableBottomRight = view->mapToScene(view->width() + 10, view->height() + 10);
  QPointF itemDrawableTopLeft = this->mapFromScene(sceneDrawableTopLeft);
  QPointF itemDrawableBottomRight = this->mapFromScene(sceneDrawableBottomRight);

  itemDrawableTopLeft.rx() = std::max(itemDrawableTopLeft.x(), 0.);
  itemDrawableTopLeft.ry() = std::max(itemDrawableTopLeft.y(), 0.);

  itemDrawableBottomRight.rx() = std::min(itemDrawableBottomRight.x(), boundingRect().width());
  itemDrawableBottomRight.ry() = std::min(itemDrawableBottomRight.y(), boundingRect().height());
  if(itemDrawableTopLeft.x() > boundingRect().width())
  {
    return;
  }
  if(itemDrawableBottomRight.y() > boundingRect().height())
  {
    return;
  }

  QPointF sceneTopLeft = this->mapToScene(QPointF{0, 0});
  QPointF sceneBottomRight = this->mapToScene(boundingRect().bottomRight());
  QPointF viewTopLeft = view->mapFromScene(sceneTopLeft);
  QPointF viewBottomRight = view->mapFromScene(sceneBottomRight);

  if(itemDrawableTopLeft.y() > 20)
  {
    return;
  }


  auto& painter = *p;
  auto& skin = Process::Style::instance();
  painter.setRenderHint(QPainter::Antialiasing, false);

  const auto& defaultColor = this->intervalColor(skin);

  const auto visibleRect = QRectF{itemDrawableTopLeft, itemDrawableBottomRight};
  drawPaths(painter, visibleRect, defaultColor, skin);
/*
  auto& solidPen = skin.IntervalSolidPen(defaultColor);
  auto& dashPen = skin.IntervalDashPen(defaultColor);

  // Drawing
  if (!solidPath.isEmpty())
  {
    painter.setPen(solidPen);
    painter.drawPath(solidPath);
  }

  if (!dashedPath.isEmpty())
  {
    painter.setPen(dashPen);
    painter.drawPath(dashedPath);
  }

  if (!playedSolidPath.isEmpty())
  {
    painter.setPen(skin.IntervalSolidPen(skin.IntervalPlayFill()));
    painter.drawPath(playedSolidPath);
  }

  if (!waitingDashedPath.isEmpty())
  {
    if (this->m_waiting)
    {
      painter.setPen(skin.IntervalDashPen(skin.IntervalWaitingDashFill()));
      painter.drawPath(waitingDashedPath);
    }
  }

  if (!playedDashedPath.isEmpty())
  {
    if (this->m_waiting)
    {
      painter.setPen(skin.IntervalDashPen(skin.IntervalPlayFill()));
    }
    else
    {
      painter.setPen(skin.IntervalSolidPen(skin.IntervalPlayFill()));
    }

    painter.drawPath(playedDashedPath);
  }*/
#if defined(SCORE_SCENARIO_DEBUG_RECTS)
  p.setPen(Qt::red);
  p.drawRect(boundingRect());
#endif
}

void FullViewIntervalView::setGuiWidth(double w)
{
  m_guiWidth = w;
  update();
}
}
