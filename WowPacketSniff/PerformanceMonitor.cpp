/*
    Copyright (c) 2015, namreeb legal@namreeb.org / http://github.com/namreeb
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

#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/alias_cast.hpp>

#include <Windows.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

PerformanceMonitor::PerformanceMonitor() : m_lastTick(0), m_process(::GetCurrentProcessId())
{
    auto const endSceneCallerOrig = hadesmem::detail::AliasCast<EndSceneCallerT>(Offsets::CVideo__EndSceneCaller);
    auto const endSceneCallerWrap = [this](hadesmem::PatchDetourBase *detour, CVideo *pVid) { return this->EndSceneHook(pVid); };

    m_endSceneCaller = std::make_unique<hadesmem::PatchDetour<EndSceneCallerT>>(m_process, endSceneCallerOrig, endSceneCallerWrap);
    m_endSceneCaller->Apply();

    // no memory leak.  see https://rhubbarb.wordpress.com/2009/10/17/boost-datetime-locales-and-facets/
    auto const facet = new boost::posix_time::wtime_facet(L"%m/%d/%Y %H:%M:%S");

    gLog << "# Log started at ";
    gLog.imbue(std::locale(gLog.getloc(), facet));
    gLog << boost::posix_time::second_clock::local_time() << std::endl;
}

PerformanceMonitor::~PerformanceMonitor()
{
    gLog << "# Log stopped at " << boost::posix_time::second_clock::local_time() << std::endl;
}

HRESULT PerformanceMonitor::EndSceneHook(CVideo *pVid)
{
    auto const trampoline = m_endSceneCaller->GetTrampolineT<EndSceneCallerT>();

    auto const tickCount = ::GetTickCount();
    auto const tickDiff = tickCount - m_lastTick;

    if (!!m_lastTick && tickDiff >= 250)
        gLog << "# " << boost::posix_time::second_clock::local_time() << " Potential freeze.  Tick delay: " << tickDiff << " ms" << std::endl;

    m_lastTick = tickCount;

    return (pVid->*trampoline)();
}