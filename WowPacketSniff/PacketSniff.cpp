/*
    Copyright (c) 2016, namreeb legal@namreeb.org / http://github.com/namreeb
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
    ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    The views and conclusions contained in the software and documentation are those
    of the authors and should not be interpreted as representing official policies,
    either expressed or implied, of the FreeBSD Project.
*/

#include "PacketSniff.hpp"
#include "offsets.hpp"
#include "Log.hpp"

#include <hadesmem/patcher.hpp>
#include <hadesmem/detail/alias_cast.hpp>
#include <hadesmem/process.hpp>

#include <memory>
#include <iomanip>
#include <ctime>

PacketSniff::PacketSniff() : m_process(::GetCurrentProcessId())
{
    auto const sendOrig = hadesmem::detail::AliasCast<SendT>(Offsets::NetClient__Send);
    auto const sendWrap = [this](hadesmem::PatchDetourBase *detour, NetClient *netClient, CDataStore *packet) { this->SendHook(netClient, packet); };

    m_send = std::make_unique<hadesmem::PatchDetour<SendT>>(m_process, sendOrig, sendWrap);
    m_send->Apply();

    auto const receiveOrig = hadesmem::detail::AliasCast<ReceiveT>(Offsets::NetClient__ProcessMessage);
    auto const receiveWrap = [this](hadesmem::PatchDetourBase *detour, NetClient *netClient, int unknown, CDataStore *packet) { return this->ReceiveHook(netClient, unknown, packet); };

    m_receive = std::make_unique<hadesmem::PatchDetour<ReceiveT>>(m_process, receiveOrig, receiveWrap);
    m_receive->Apply();
}

void PacketSniff::SendHook(NetClient *netClient, CDataStore *packet) const
{
    auto const trampoline = m_send->GetTrampolineT<SendT>();

    LogPacket(packet, true);

    auto const start = ::GetTickCount();

    (netClient->*trampoline)(packet);

    auto const duration = ::GetTickCount() - start;

    if (duration > 1000)
        gLog << "# Last packet took " << duration << " ms to parse" << std::endl;
}

int PacketSniff::ReceiveHook(NetClient *netClient, int unknown, CDataStore *packet) const
{
    auto const trampoline = m_receive->GetTrampolineT<ReceiveT>();

    LogPacket(packet, false);

    auto const start = ::GetTickCount();

    auto const ret = (netClient->*trampoline)(unknown, packet);

    auto const duration = ::GetTickCount() - start;

    if (duration > 1000)
        gLog << "# Last packet took " << duration << " ms to parse" << std::endl;

    return ret;
}

void PacketSniff::LogPacket(CDataStore *packet, bool outgoing) const
{
    gLog << (outgoing ? L"> " : L"< ") << time(nullptr);

    for (unsigned int i = 0; i < packet->m_bytesWritten; ++i)
        gLog << " 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill(L'0') << static_cast<PBYTE>(packet->m_data)[i];

    gLog << std::dec << std::endl;
}