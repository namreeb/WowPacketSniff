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

#define NAME                L"WowPacketSniff"
#define VERSION             L"v0.1"
#define PROCESS_NAME        L"Wow.exe"
#define DLL_NAME            L"WowPacketSniff.dll"
#define DLL_LOAD_FUNCTION   "Load"

#include <iostream>
#include <vector>
#include <AclAPI.h>

#include <boost/filesystem.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <hadesmem/process_list.hpp>
#include <hadesmem/injector.hpp>

int main()
{
    try
    {
        std::wcout << NAME << " " << VERSION << " injector" << std::endl;
        std::wcout << "by namreeb http://github.com/namreeb / github@namreeb.org" << std::endl;

        if (!boost::filesystem::exists(PROCESS_NAME))
        {
            std::wcerr << L"Error.  Must be run from within Wow root folder" << std::endl;
            return EXIT_FAILURE;
        }

        const std::wstring dllPath(DLL_NAME);

        HMODULE module;
        std::vector<std::wstring> createArgs;
        const std::wstring processPath(PROCESS_NAME);

        std::wcout << L"Starting " << processPath << "... ";

        const hadesmem::CreateAndInjectData injectData = hadesmem::CreateAndInject(processPath, L"", std::begin(createArgs), std::end(createArgs),
            dllPath, "", hadesmem::InjectFlags::kPathResolution);

        std::wcout << L" Done.  Process ID: " << injectData.GetProcess() << std::endl;

        std::unique_ptr<hadesmem::Process> process(new hadesmem::Process(injectData.GetProcess()));
        module = injectData.GetModule();

        std::wcout << L"Injected." << std::endl;

        try
        {
            hadesmem::CallExport(*process, module, DLL_LOAD_FUNCTION);
        }
        catch (std::exception const &e)
        {
            std::wcout << L"Load failed.  Patching ACLs and trying again..." << std::endl;

            // some versions of wow, namely tbc, screwed around with their ACLs to prevent dll injection.  we can patch this, though
            SID_IDENTIFIER_AUTHORITY sia;
            sia.Value[0] = 0;
            sia.Value[1] = 0;
            sia.Value[2] = 0;
            sia.Value[3] = 0;
            sia.Value[4] = 0;
            sia.Value[5] = 1;
            PSID sid;

            if (!::AllocateAndInitializeSid(&sia, 1, 0, 0, 0, 0, 0, 0, 0, 0, &sid))
                throw e;

            const int aclSize = 0x200;
            PACL acl = static_cast<PACL>(malloc(aclSize));

            if (!::InitializeAcl(acl, aclSize, ACL_REVISION))
                throw e;

            if (!::AddAccessAllowedAce(acl, ACL_REVISION, PROCESS_ALL_ACCESS, sid))
                throw e;

            if (::SetSecurityInfo(process->GetHandle(), SE_KERNEL_OBJECT, PROTECTED_DACL_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, nullptr, nullptr, acl, nullptr))
                throw e;

            // finally, try again! \o/
            hadesmem::CallExport(*process, module, DLL_LOAD_FUNCTION);

            free(acl);
        }
    }
    catch (std::exception const &e)
    {
        std::cerr << std::endl << "Error: " << std::endl;
        std::cerr << boost::diagnostic_information(e) << std::endl;

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}