#pragma once
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <juce_core/juce_core.h>

namespace tremolo {
inline std::string getFileOutputPath(juce::StringRef fileName) {
    return juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentExecutableFile)
        .getParentDirectory()
        .getChildFile(fileName)
        .getFullPathName()
        .toStdString();
}

template<typename Enum>
    requires std::is_enum_v<Enum>
constexpr auto enumRange() noexcept {
    using Underlying = std::underlying_type_t<Enum>;

    return std::views::iota(Underlying {0}, std::to_underlying(Enum::count))
         | std::views::transform([](Underlying value) { return static_cast<Enum>(value); });
}
}  // namespace tremolo
