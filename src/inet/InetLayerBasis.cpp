/*
 *
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This file contains the basis class for reference counting
 *      objects by the Inet layer as well as a class for representing
 *      the pending or resulting I/O events on a socket.
 */

#include <InetLayer/InetLayerBasis.h>

namespace nl {
namespace Inet {

#if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
/**
 *  Sets the bit for the specified file descriptor in the given sets of file descriptors.
 *
 *  @param[in]      socket      The file descriptor for which the bit is being set.
 *  @param[in,out]  pollFDs     The fd set which is going to be polled
 *  @param[in,out]  numPollFDs  The number of fds in the fd set
 */
void SocketEvents::SetFDs(int socket, struct pollfd * pollFDs, int& numPollFDs)
{
    if (socket != INET_INVALID_SOCKET_FD)
    {
        struct pollfd & event = pollFDs[numPollFDs];
        event.fd = socket;
        event.events = 0;
        event.revents = 0;
        if (IsReadable())
            event.events |= POLLIN;
        if (IsWriteable())
            event.events |= POLLOUT;
        if (event.events != 0)
            numPollFDs++;
    }
}

/**
 *  Set the read, write or exception bit flags for the specified socket based on its status in
 *  the corresponding file descriptor sets.
 *
 *  @param[in]    socket      The file descriptor for which the bit flags are being set.
 *  @param[in]    pollFDs     The result of polled FDs
 *  @param[in]    numPollFDs  The number of fds in the fd set
 */
SocketEvents SocketEvents::FromFDs(int socket, const struct pollfd * pollFDs, int numPollFDs)
{
    SocketEvents res;

    if (socket != INET_INVALID_SOCKET_FD)
    {
        for (int i = 0; i < numPollFDs; ++i)
        {
            const struct pollfd & event = pollFDs[i];
            if (event.fd == socket)
            {
                if ((event.revents & (POLLIN | POLLHUP)) != 0)
                    res.SetRead();
                if ((event.revents & POLLOUT) != 0)
                    res.SetWrite();
                if ((event.revents & POLLERR) != 0)
                    res.SetError();
            }
        }
    }

    return res;
}
#endif // WEAVE_SYSTEM_CONFIG_USE_SOCKETS

} // namespace Inet
} // namespace nl
