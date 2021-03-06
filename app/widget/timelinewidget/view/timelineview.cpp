/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "timelineview.h"

#include <QDebug>
#include <QMimeData>
#include <QMouseEvent>
#include <QScrollBar>
#include <QtMath>
#include <QPen>

#include "common/flipmodifiers.h"
#include "common/timecodefunctions.h"
#include "core.h"
#include "node/input/media/media.h"
#include "project/item/footage/footage.h"

TimelineView::TimelineView(const TrackType &type, Qt::Alignment vertical_alignment, QWidget *parent) :
  QGraphicsView(parent),
  playhead_(0),
  type_(type)
{
  Q_ASSERT(vertical_alignment == Qt::AlignTop || vertical_alignment == Qt::AlignBottom);
  setAlignment(Qt::AlignLeft | vertical_alignment);

  setScene(&scene_);
  setDragMode(NoDrag);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setBackgroundRole(QPalette::Window);

  connect(&scene_, SIGNAL(changed(const QList<QRectF>&)), this, SLOT(UpdateSceneRect()));

  // Create end item
  end_item_ = new TimelineViewEndItem();
  scene_.addItem(end_item_);

  // Set default scale
  SetScale(1.0);
}

void TimelineView::SetScale(const double &scale)
{
  scale_ = scale;

  // Force redraw for playhead
  viewport()->update();

  end_item_->SetScale(scale_);
}

void TimelineView::SetTimebase(const rational &timebase)
{
  SetTimebaseInternal(timebase);

  // Timebase influences position/visibility of playhead
  viewport()->update();
}

void TimelineView::SelectAll()
{
  QList<QGraphicsItem*> all_items = items();

  foreach (QGraphicsItem* i, all_items) {
    i->setSelected(true);
  }
}

void TimelineView::DeselectAll()
{
  QList<QGraphicsItem*> all_items = items();

  foreach (QGraphicsItem* i, all_items) {
    i->setSelected(false);
  }
}

void TimelineView::SetTime(const int64_t time)
{
  playhead_ = time;

  // Force redraw for playhead
  viewport()->update();
}

void TimelineView::mousePressEvent(QMouseEvent *event)
{
  TimelineViewMouseEvent timeline_event(ScreenToCoordinate(event->pos()),
                                        event->modifiers());

  emit MousePressed(&timeline_event);


}

void TimelineView::mouseMoveEvent(QMouseEvent *event)
{
  TimelineViewMouseEvent timeline_event(ScreenToCoordinate(event->pos()),
                                        event->modifiers());

  emit MouseMoved(&timeline_event);
}

void TimelineView::mouseReleaseEvent(QMouseEvent *event)
{
  TimelineViewMouseEvent timeline_event(ScreenToCoordinate(event->pos()),
                                        event->modifiers());

  emit MouseReleased(&timeline_event);
}

void TimelineView::mouseDoubleClickEvent(QMouseEvent *event)
{
  TimelineViewMouseEvent timeline_event(ScreenToCoordinate(event->pos()),
                                        event->modifiers());

  emit MouseDoubleClicked(&timeline_event);
}

void TimelineView::dragEnterEvent(QDragEnterEvent *event)
{
  TimelineViewMouseEvent timeline_event(ScreenToCoordinate(event->pos()),
                                        event->keyboardModifiers());

  timeline_event.SetMimeData(event->mimeData());
  timeline_event.SetEvent(event);

  emit DragEntered(&timeline_event);
}

void TimelineView::dragMoveEvent(QDragMoveEvent *event)
{
  TimelineViewMouseEvent timeline_event(ScreenToCoordinate(event->pos()),
                                        event->keyboardModifiers());

  timeline_event.SetMimeData(event->mimeData());
  timeline_event.SetEvent(event);

  emit DragMoved(&timeline_event);
}

void TimelineView::dragLeaveEvent(QDragLeaveEvent *event)
{
  emit DragLeft(event);
}

void TimelineView::dropEvent(QDropEvent *event)
{
  TimelineViewMouseEvent timeline_event(ScreenToCoordinate(event->pos()),
                                        event->keyboardModifiers());

  timeline_event.SetMimeData(event->mimeData());
  timeline_event.SetEvent(event);

  emit DragDropped(&timeline_event);
}

void TimelineView::resizeEvent(QResizeEvent *event)
{
  QGraphicsView::resizeEvent(event);

  UpdateSceneRect();
}

void TimelineView::drawForeground(QPainter *painter, const QRectF &rect)
{
  QGraphicsView::drawForeground(painter, rect);

  if (!timebase().isNull()) {
    double x = TimeToScene(rational(playhead_ * timebase().numerator(), timebase().denominator()));
    double width = TimeToScene(timebase());

    QRectF playhead_rect(x, rect.top(), width, rect.height());

    painter->setPen(Qt::NoPen);
    painter->setBrush(playhead_style_.PlayheadHighlightColor());
    painter->drawRect(playhead_rect);

    painter->setPen(playhead_style_.PlayheadColor());
    painter->setBrush(Qt::NoBrush);
    painter->drawLine(QLineF(playhead_rect.topLeft(), playhead_rect.bottomLeft()));
  }
}

Stream::Type TimelineView::TrackTypeToStreamType(TrackType track_type)
{
  switch (track_type) {
  case kTrackTypeNone:
  case kTrackTypeCount:
    break;
  case kTrackTypeVideo:
    return Stream::kVideo;
  case kTrackTypeAudio:
    return Stream::kAudio;
  case kTrackTypeSubtitle:
    return Stream::kSubtitle;
  }

  return Stream::kUnknown;
}

TimelineCoordinate TimelineView::ScreenToCoordinate(const QPoint& pt)
{
  return SceneToCoordinate(mapToScene(pt));
}

TimelineCoordinate TimelineView::SceneToCoordinate(const QPointF& pt)
{
  return TimelineCoordinate(SceneToTime(pt.x()), TrackReference(type_, SceneToTrack(pt.y())));
}

int TimelineView::GetTrackY(int track_index)
{
  int y = 0;

  for (int i=0;i<track_index;i++) {
    y += GetTrackHeight(i);
  }

  if (alignment() & Qt::AlignBottom) {
    y = -y - GetTrackHeight(0);
  }

  return y;
}

int TimelineView::GetTrackHeight(int track_index)
{
  // FIXME: Make this adjustable
  Q_UNUSED(track_index)

  return fontMetrics().height() * 3;
}

QPoint TimelineView::GetScrollCoordinates()
{
  return QPoint(horizontalScrollBar()->value(), verticalScrollBar()->value());
}

void TimelineView::SetScrollCoordinates(const QPoint &pt)
{
  horizontalScrollBar()->setValue(pt.x());
  verticalScrollBar()->setValue(pt.y());
}

int TimelineView::SceneToTrack(double y)
{
  int track = -1;
  int heights = 0;

  if (alignment() & Qt::AlignBottom) {
    y = -y;
  }

  do {
    track++;
    heights += GetTrackHeight(track);
  } while (y > heights);

  return track;
}

void TimelineView::UserSetTime(const int64_t &time)
{
  SetTime(time);
  emit TimeChanged(time);
}

rational TimelineView::GetPlayheadTime()
{
  return rational(playhead_ * timebase().numerator(), timebase().denominator());
}

void TimelineView::UpdateSceneRect()
{
  QRectF bounding_rect = scene_.itemsBoundingRect();

  // Ensure the scene height is always AT LEAST the height of the view
  // The scrollbar appears to have a 1px margin on the top and bottom, hence the -2
  int minimum_height = height() - horizontalScrollBar()->height() - 2;

  if (alignment() & Qt::AlignBottom) {
    // Ensure the scene left and bottom are always 0
    bounding_rect.setBottomLeft(QPointF(0, 0));

    if (bounding_rect.top() > minimum_height) {
      bounding_rect.setTop(-minimum_height);
    }
  } else {
    // Ensure the scene left and top are always 0
    bounding_rect.setTopLeft(QPointF(0, 0));

    if (bounding_rect.height() < minimum_height) {
      bounding_rect.setHeight(minimum_height);
    }
  }

  // Ensure the scene is always the full length of the timeline with a gap at the end to work with
  end_item_->SetEndPadding(width()/4);

  // If the scene is already this rect, do nothing
  if (scene_.sceneRect() == bounding_rect) {
    return;
  }

  scene_.setSceneRect(bounding_rect);
}

void TimelineView::SetEndTime(const rational &length)
{
  end_item_->SetEndTime(length);
}
