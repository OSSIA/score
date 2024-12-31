#include "BitfocusContext.hpp"

namespace bitfocus
{

module_handler_base::module_handler_base(QString module_path)
{
  // Create socketpair
  socketpair(PF_LOCAL, SOCK_STREAM, 0, pfd);

  // Create env
  auto genv = QProcessEnvironment::systemEnvironment();
  genv.insert("CONNECTION_ID", "connectionId");
  genv.insert("VERIFICATION_TOKEN", "foobar");
  genv.insert("MODULE_MANIFEST", module_path + "/companion/manifest.json");
  genv.insert("NODE_CHANNEL_SERIALIZATION_MODE", "json");
  genv.insert("NODE_CHANNEL_FD", QString::number(pfd[1]).toUtf8());

  auto socket = new QSocketNotifier(pfd[0], QSocketNotifier::Read, this);
  QObject::connect(
      socket, &QSocketNotifier::activated, this, &module_handler_base::on_read);

  process.setProcessChannelMode(QProcess::ForwardedChannels);
  process.setProgram("node");
  process.setArguments({"main.js"}); // FIXME entrypoint from spec
  process.setWorkingDirectory(module_path);
  process.setProcessEnvironment(genv);

  process.start();

  // See https://forum.qt.io/topic/33964/solved-child-qprocess-that-dies-with-parent/10

  /// Connection flow:
  // Create process
  // <- register call
  // -> register response

  // -> init call
  //   <- upgradedItems
  //   <- setActionDefinitions
  //   <- setVariableDefinitions
  //   <- etc.
  // <- init response
}

void module_handler_base::on_read(QSocketDescriptor, QSocketNotifier::Type)
{
  ssize_t rl = ::read(pfd[0], buf, sizeof(buf));
  if(rl <= 0)
    return;
  char* pos = buf;
  char* idx = buf;
  char* const end = pos + rl;
  do
  {
    idx = std::find(pos, end, '\n');
    if(idx < end)
    {
      std::ptrdiff_t diff = idx - pos;
      std::string_view message(pos, diff);
      this->processMessage(message);
      pos = idx + 1;
      continue;
    }
  } while(idx < end);
}

void module_handler_base::do_write(std::string_view res)
{
  ::write(pfd[0], res.data(), res.size());
}

void module_handler_base::do_write(const QByteArray& res)
{
  ::write(pfd[0], res.data(), res.size());
}

}
