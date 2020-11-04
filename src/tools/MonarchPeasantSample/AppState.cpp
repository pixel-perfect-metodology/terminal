#include "pch.h"
#include <conio.h>
#include "Monarch.h"
#include "Peasant.h"
#include "AppState.h"
#include "../../types/inc/utils.hpp"

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace ::Microsoft::Console;

void AppState::_setupConsole()
{
    hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOutput, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOutput, dwMode);
}

void AppState::initializeState()
{
    // Initialize the console handles
    _setupConsole();

    // Set up WinRT
    init_apartment();
}

bool AppState::areWeTheKing(const bool logPIDs)
{
    auto kingPID = _monarch.GetPID();
    auto ourPID = GetCurrentProcessId();
    if (logPIDs)
    {
        if (ourPID == kingPID)
        {
            printf(fmt::format("We're the \x1b[33mking\x1b[m - our PID is {}\n", ourPID).c_str());
        }
        else
        {
            printf(fmt::format("We're a lowly peasant - the king is {}\n", kingPID).c_str());
        }
    }
    return (ourPID == kingPID);
}

void AppState::remindKingWhoTheyAre(const winrt::MonarchPeasantSample::IPeasant& peasant)
{
    winrt::com_ptr<MonarchPeasantSample::implementation::Monarch> monarchImpl;
    monarchImpl.copy_from(winrt::get_self<MonarchPeasantSample::implementation::Monarch>(_monarch));
    if (monarchImpl)
    {
        auto ourID = peasant.GetID();
        monarchImpl->SetSelfID(ourID);
        printf("The king is peasant #%lld\n", ourID);
    }
    else
    {
        // printf("Shoot, we wanted to be able to get the impl here but couldnt\n");
    }
}

winrt::MonarchPeasantSample::Monarch AppState::instantiateAMonarch()
{
    auto monarch = create_instance<winrt::MonarchPeasantSample::Monarch>(Monarch_clsid, CLSCTX_LOCAL_SERVER);
    return monarch;
}

MonarchPeasantSample::IPeasant AppState::_createOurPeasant()
{
    auto peasant = winrt::make_self<MonarchPeasantSample::implementation::Peasant>();
    auto ourID = _monarch.AddPeasant(*peasant);
    printf("The monarch assigned us the ID %llu\n", ourID);

    if (areWeTheKing())
    {
        remindKingWhoTheyAre(*peasant);
    }

    return *peasant;
}

// void AppState::createMonarchAndPeasant()
// {
//     _monarch = AppState::instantiateAMonarch();
//     _peasant = _createOurPeasant();
// }

void AppState::createMonarch()
{
    _monarch = AppState::instantiateAMonarch();
}

// return true to exit early, false if we should continue into the main loop
bool AppState::processCommandline()
{
    const bool isKing = areWeTheKing(false);
    // If we're the king, we _definitely_ want to process the arguments, we were
    // launched with them!
    //
    // Otherwise, the King will tell us if we should make a new window
    const bool createNewWindow = isKing || _monarch.ProposeCommandline({ args }, { L"placeholder CWD" });

    if (createNewWindow)
    {
        _peasant = _createOurPeasant();
        _peasant.ExecuteCommandline({ args }, { L"placeholder CWD" });
        return false;
    }
    else
    {
        printf("The Monarch instructed us to not create a new window. We'll be exiting now.\n");
    }

    return true;
}