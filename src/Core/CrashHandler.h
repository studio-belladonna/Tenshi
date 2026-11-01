#pragma once

#include <string>
#include "Tenshi/Core/Log.h"
#include "Tenshi/Platform/Platform.h"

#include <cstdlib>

namespace Tenshi {

// display a critical error dialog and force the application to terminate
[[noreturn]] static void FatalError(
    Platform* platform,
    const std::string& title,
    const std::string& message
) {
    LogError(LogCategory::Application, "{}: {}", title, message);
    if (platform)
        platform->showError(title.c_str(), message.c_str());

    std::exit(EXIT_FAILURE);
}

} // namespace Tenshi