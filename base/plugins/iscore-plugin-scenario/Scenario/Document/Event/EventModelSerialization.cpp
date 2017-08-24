// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <QDataStream>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QtGlobal>
#include <algorithm>
#include <iscore/serialization/DataStreamVisitor.hpp>
#include <iscore/serialization/JSONVisitor.hpp>

#include <Process/TimeValue.hpp>
#include <Scenario/Document/Event/EventModel.hpp>
#include <Scenario/Document/VerticalExtent.hpp>
#include <State/Expression.hpp>
#include <iscore/model/ModelMetadata.hpp>
#include <iscore/serialization/JSONValueVisitor.hpp>
#include <iscore/model/Identifier.hpp>
#include <iscore/model/tree/TreeNode.hpp>


template <>
ISCORE_PLUGIN_SCENARIO_EXPORT void
DataStreamReader::read(const Scenario::EventModel& ev)
{
  m_stream << ev.m_timeSync << ev.m_states << ev.m_condition << ev.m_extent
           << ev.m_date << ev.m_offset;

  insertDelimiter();
}


template <>
ISCORE_PLUGIN_SCENARIO_EXPORT void
DataStreamWriter::write(Scenario::EventModel& ev)
{
  m_stream >> ev.m_timeSync >> ev.m_states >> ev.m_condition >> ev.m_extent
      >> ev.m_date >> ev.m_offset;

  checkDelimiter();
}


template <>
ISCORE_PLUGIN_SCENARIO_EXPORT void
JSONObjectReader::read(const Scenario::EventModel& ev)
{
  obj[strings.TimeSync] = toJsonValue(ev.m_timeSync);
  obj[strings.States] = toJsonArray(ev.m_states);

  obj[strings.Condition] = toJsonObject(ev.m_condition);

  obj[strings.Extent] = toJsonValue(ev.m_extent);
  obj[strings.Date] = toJsonValue(ev.m_date);
  obj[strings.Offset] = (int32_t)ev.m_offset;
}


template <>
ISCORE_PLUGIN_SCENARIO_EXPORT void
JSONObjectWriter::write(Scenario::EventModel& ev)
{
  ev.m_timeSync
      = fromJsonValue<Id<Scenario::TimeSyncModel>>(obj[strings.TimeSync]);
  fromJsonValueArray(obj[strings.States].toArray(), ev.m_states);

  fromJsonObject(obj[strings.Condition], ev.m_condition);

  ev.m_extent = fromJsonValue<Scenario::VerticalExtent>(obj[strings.Extent]);
  ev.m_date = fromJsonValue<TimeVal>(obj[strings.Date]);
  ev.m_offset = static_cast<Scenario::OffsetBehavior>(obj[strings.Offset].toInt());
}
