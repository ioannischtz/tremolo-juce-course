#pragma once

#include "Crossfade.h"

#include <concepts>
#include <optional>

namespace tremolo {

template<typename T>
concept SwitchOption = std::copyable<T> && std::equality_comparable<T>;

template<SwitchOption Option>
class CrossfadeSwitcher {
  public:
    // Pair of option and its crossfade.Mix value
    using ActiveOption  = std::pair<Option, float>;
    using ActiveOptions = std::array<ActiveOption, 2>;

    CrossfadeSwitcher(const double fadeSeconds, Option initialOptionA, Option initialOptionB)
        : crossfade {fadeSeconds, Crossfade::Side::A},
          slotA {initialOptionA},
          slotB {initialOptionB} {}

    CrossfadeSwitcher(double fadeSeconds, Option initialOption)
        : CrossfadeSwitcher {fadeSeconds, initialOption, initialOption} {}

    // ========
    // Public Methods
    // ========

    void prepare(const double sampleRateHz) noexcept {
        crossfade.prepare(sampleRateHz);
    }

    void switchTo(Option option) noexcept {
        const auto targetSide = crossfade.getTargetSide();

        // same option, i.e. no-op
        if (option == slotFor(targetSide)) {
            pending.reset();
            return;
        }

        // switch to the other option
        if (option == slotFor(otherSide(targetSide))) {
            pending.reset();
            crossfade.fadeTo(otherSide(targetSide));
            return;
        }

        // a third option was requested, while still Fading
        // so queue it in as the pending option
        if (crossfade.isFading()) {
            pending = option;
            return;
        }

        // a third option was requested
        // put it in the "silent" slot
        fadeToNewOption(option);
    }

    void switchImmediatelyTo(Option option) noexcept {
        if (option == slotA) {
            pending.reset();
            crossfade.snapTo(Crossfade::Side::A);
            return;
        }

        if (option == slotB) {
            pending.reset();
            crossfade.snapTo(Crossfade::Side::B);
            return;
        }

        snapToNewOption(option);
    }

    [[nodiscard]] ActiveOptions nextActive() noexcept {
        // 1. Produce this sample's active set
        const auto mix     = crossfade.getNextMix();
        const auto weightA = mix.get(Crossfade::Side::A);
        const auto weightB = mix.get(Crossfade::Side::B);

        ActiveOptions activeOptions {{{slotA, weightA}, {slotB, weightB}}};

        // 2. prepare the pending transition for the next sample
        // (only if previous fading has completed)
        if (!crossfade.isFading() && pending.has_value()) {
            fadeToNewOption(*pending);
        }

        return activeOptions;
    }

    [[nodiscard]] bool isFading() const noexcept {
        return crossfade.isFading();
    }

  private:
    // ========
    // Private Methods
    // ========
    static constexpr Crossfade::Side otherSide(Crossfade::Side side) noexcept {
        return side == Crossfade::Side::A ? Crossfade::Side::B : Crossfade::Side::A;
    }

    Option& slotFor(Crossfade::Side side) noexcept {
        return side == Crossfade::Side::A ? slotA : slotB;
    }

    void fadeToNewOption(Option option) noexcept {
        const auto silentSide = otherSide(crossfade.getTargetSide());
        slotFor(silentSide)   = option;
        pending.reset();
        crossfade.fadeTo(silentSide);
    }

    void snapToNewOption(Option option) noexcept {
        const auto silentSide = otherSide(crossfade.getTargetSide());
        slotFor(silentSide)   = option;
        pending.reset();
        crossfade.snapTo(silentSide);
    }

    // ========
    // State
    // ========
    Crossfade crossfade;
    Option slotA;
    Option slotB;
    std::optional<Option> pending;
};

}  // namespace tremolo