#pragma once
#include <Interpolation/Commands/CommandFactory.hpp>
#include <Interpolation/InterpolationProcess.hpp>
#include <iscore/command/Dispatchers/CommandDispatcher.hpp>

#include <iscore/command/PropertyCommand.hpp>

namespace Interpolation
{
class ProcessModel;
class ChangeAddress final
    : public iscore::Command
{
  ISCORE_COMMAND_DECL(
      CommandFactoryName(), ChangeAddress, "Change Interpolation Address")
public:
  ChangeAddress(
      const ProcessModel& proc,
      const State::AddressAccessor& addr,
      const State::Value& start,
      const State::Value& end,
      const State::Unit& u);

public:
  void undo(const iscore::DocumentContext& ctx) const override;
  void redo(const iscore::DocumentContext& ctx) const override;

protected:
  void serializeImpl(DataStreamInput&) const override;
  void deserializeImpl(DataStreamOutput&) override;

private:
  Path<ProcessModel> m_path;
  State::AddressAccessor m_oldAddr, m_newAddr;
  State::Unit m_oldUnit, m_newUnit;
  State::Value m_oldStart, m_newStart;
  State::Value m_oldEnd, m_newEnd;
};

void ChangeInterpolationAddress(const Interpolation::ProcessModel& proc, const ::State::AddressAccessor& addr, CommandDispatcher<>& disp);

// MOVEME && should apply to both Interpolation and Automation
class SetTween final
    : public iscore::PropertyCommand
{
  ISCORE_COMMAND_DECL(CommandFactoryName(), SetTween, "Set interpolation tween")
public:
  SetTween(const ProcessModel& path, bool newval)
      : iscore::PropertyCommand{std::move(path), "tween", newval}
  {
  }
};
}
