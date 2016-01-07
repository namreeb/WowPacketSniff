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

#include "PerformanceMonitor.hpp"
#include "offsets.hpp"
#include "Log.hpp"
#include "CMovement_C.hpp"

#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/alias_cast.hpp>

#include <Windows.h>
#include <cmath>

PerformanceMonitor::PerformanceMonitor() : m_lastEndScene(0), m_process(::GetCurrentProcessId())
{
    auto const endSceneCallerOrig = hadesmem::detail::AliasCast<EndSceneCallerT>(Offsets::CVideo__EndSceneCaller);
    auto const endSceneCallerWrap = [this](hadesmem::PatchDetourBase *detour, CVideo *pVid) { return this->EndSceneHook(pVid); };

    m_endSceneCaller = std::make_unique<hadesmem::PatchDetour<EndSceneCallerT>>(m_process, endSceneCallerOrig, endSceneCallerWrap);
    m_endSceneCaller->Apply();

    auto const executeMovementOrig = hadesmem::detail::AliasCast<ExecuteMovementT>(Offsets::CMovement_C__ExecuteMovement);
    auto const executeMovementWrap = [this](hadesmem::PatchDetourBase *detour, CMovement_C *movement, unsigned timeNow, unsigned lastUpdate) { this->ExecuteMovementHook(movement, timeNow, lastUpdate); };

    m_executeMovement = std::make_unique<hadesmem::PatchDetour<ExecuteMovementT>>(m_process, executeMovementOrig, executeMovementWrap);
    m_executeMovement->Apply();

    gLog << "# Log started at " << time(nullptr) << std::endl;
}

PerformanceMonitor::~PerformanceMonitor()
{
    gLog << "# Log stopped at " << time(nullptr) << std::endl;
}

HRESULT PerformanceMonitor::EndSceneHook(CVideo *pVid)
{
    auto const trampoline = m_endSceneCaller->GetTrampolineT<EndSceneCallerT>();

    auto const tickCount = ::GetTickCount();
    auto const tickDiff = tickCount - m_lastEndScene;

    if (!!m_lastEndScene && tickDiff >= 250)
        gLog << "# Potential freeze at " << time(nullptr) << ".  Tick delay: " << tickDiff << " ms" << std::endl;

    m_lastEndScene = tickCount;

    return (pVid->*trampoline)();
}

class CGUnit_C
{
    private:
        unsigned int dword0[2];
    public:
        unsigned int *Descriptors;
};

void PerformanceMonitor::ExecuteMovementHook(CMovement_C *movement, unsigned int timeNow, unsigned int lastUpdate)
{
    auto const trampoline = m_executeMovement->GetTrampolineT<ExecuteMovementT>();

    auto const start = ::GetTickCount();

    auto const startX = movement->ClientPosition[0];
    auto const startY = movement->ClientPosition[1];
    auto const startZ = movement->ClientPosition[2];

    auto const startFlags = movement->Flags;

    (movement->*trampoline)(timeNow, lastUpdate);

    auto const duration = ::GetTickCount() - start;

    if (duration > 1000)
    {
        auto const unit = static_cast<CGUnit_C *>(movement->Owner);

        gLog << "# CMovement_C::ExecuteMovement took " << duration << " ms.  From (" << startX << ", " << startY << ", " << startZ << ") to (";

        if (isnan(movement->ClientPosition[0]))
            gLog << "nan";
        else
            gLog << movement->ClientPosition[0];

        gLog << ", ";

        if (isnan(movement->ClientPosition[1]))
            gLog << "nan";
        else
            gLog << movement->ClientPosition[1];

        gLog << ", ";

        if (isnan(movement->ClientPosition[2]))
            gLog << "nan";
        else
            gLog << movement->ClientPosition[2];

        unsigned __int64 guid = *reinterpret_cast<unsigned __int64 *>(unit->Descriptors);
        gLog << ") GUID: 0x" << std::hex << std::uppercase << guid << std::dec;

        gLog << " Flags from: 0x" << std::hex << std::uppercase << startFlags << " to 0x" << movement->Flags << std::dec << std::endl;
    }
}