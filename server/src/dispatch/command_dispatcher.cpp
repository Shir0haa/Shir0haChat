#include "dispatch/command_dispatcher.h"

#include <QDebug>

#include "client_session.h"
#include "handlers/i_command_handler.h"
#include "protocol/commands.h"
#include "protocol/packet.h"

namespace ShirohaChat
{

CommandDispatcher::CommandDispatcher(QObject* parent)
    : QObject(parent)
{
}

void CommandDispatcher::registerHandler(ICommandHandler* handler)
{
    const QList<Command> commands = handler->supportedCommands();
    for (const Command cmd : commands)
    {
        if (m_handlers.contains(cmd))
        {
            qWarning() << "[CommandDispatcher] Command" << toString(cmd)
                       << "already registered, overwriting";
        }
        m_handlers.insert(cmd, handler);
    }
}

void CommandDispatcher::dispatch(ClientSession* session, const Packet& packet)
{
    auto it = m_handlers.find(packet.cmd());
    if (it != m_handlers.end())
    {
        it.value()->handlePacket(session, packet);
        return;
    }

    qWarning() << "[CommandDispatcher] Unhandled command:"
               << QString::fromUtf8(toString(packet.cmd()))
               << "from" << session->connectionId();
    session->sendError(packet.msgId(), 501, QStringLiteral("Command not implemented"));
}

} // namespace ShirohaChat
