#include "pch.h"
#include "MinecraftBedrockGameInfo.h"
#if __has_include("MinecraftBedrockGameInfo.g.cpp")
#include "MinecraftBedrockGameInfo.g.cpp"
#endif

namespace winrt {

    using namespace Windows::Foundation::Collections;
}

namespace winrt::Citrine::implementation
{
    auto MinecraftBedrockGameInfo::AssociatedFileTypes() -> winrt::IVectorView<winrt::hstring> {

        static auto associatedFileTypes = winrt::single_threaded_vector<winrt::hstring>({

            L".mcpack",
            L".mcworld",
            L".mcperf",
            L".mcshortcut",
            L".mcproject",
            L".mctemplate",
            L".mcaddon",
            L".mceditoraddon"
        });
        return associatedFileTypes.GetView();
    }
}
