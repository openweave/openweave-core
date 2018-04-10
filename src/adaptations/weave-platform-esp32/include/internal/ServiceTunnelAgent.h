#ifndef TUNNEL_AGENT_H
#define TUNNEL_AGENT_H

#include <internal/WeavePlatformInternal.h>
#include <Weave/Profiles/weave-tunneling/WeaveTunnelAgent.h>

namespace WeavePlatform {
namespace Internal {

extern ::nl::Weave::Profiles::WeaveTunnel::WeaveTunnelAgent ServiceTunnelAgent;

extern WEAVE_ERROR InitServiceTunnelAgent();

} // namespace Internal
} // namespace WeavePlatform

#endif // TUNNEL_AGENT_H
