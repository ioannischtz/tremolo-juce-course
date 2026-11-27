namespace tremolo {
namespace {
    struct SerializableParameters {
        float rateHz;
        float inputGainDB;
        float outputGainDB;
        bool isBypassed;
        juce::String waveformShape;

        static constexpr auto marshallingVersion = 1;

        template<typename Archive, typename T>
        static void serialise(Archive& archive, T& t) {
            using namespace juce;

            if (archive.getVersion() != 1) {
                return;
            }

            std::string pluginName = TREMOLO_PLUGIN_NAME;

            archive(named("pluginName", pluginName));

            if (pluginName != TREMOLO_PLUGIN_NAME) {
                return;
            }

            archive(named("modulationRateHz", t.rateHz),
                    named("inputGainDB", t.inputGainDB),
                    named("outputGainDB", t.outputGainDB),
                    named("bypassed", t.isBypassed),
                    named("modulationWaveform", t.waveformShape));
        }
    };

    SerializableParameters from(const tremolo::Parameters& parameters) {
        return {
            .rateHz        = parameters.rateHz.get(),
            .inputGainDB   = parameters.inputGainDB.get(),
            .outputGainDB  = parameters.outputGainDB.get(),
            .isBypassed    = parameters.isBypassed.get(),
            .waveformShape = parameters.waveformShape.getCurrentChoiceName(),
        };
    }
}  // namespace

void JsonSerializer::serialize(const Parameters& parameters, juce::OutputStream& output) {
    const auto parametersToSerialize = from(parameters);

    const auto json = juce::ToVar::convert(parametersToSerialize);

    if (!json.has_value()) {
        return;
    }

    juce::JSON::writeToStream(
        output,
        *json,
        juce::JSON::FormatOptions {}.withSpacing(juce::JSON::Spacing::multiLine).withMaxDecimalPlaces(2));
}

juce::Result JsonSerializer::deserialize(juce::InputStream& input, const Parameters& parameters) {
    juce::var parsedResult;

    if (auto result = juce::JSON::parse(input.readEntireStreamAsString(), parsedResult); result.failed()) {
        return result;
    }

    const auto parsedParameters = juce::FromVar::convert<SerializableParameters>(parsedResult);

    if (!parsedParameters.has_value()) {
        return juce::Result::fail("failed to parse parameters from JSON representation");
    }

    const auto modulationWaveformIndex = parameters.waveformShape.choices.indexOf(parsedParameters->waveformShape);
    if (modulationWaveformIndex == -1) {
        return juce::Result::fail("invalid waveform name");
    }
    parameters.waveformShape = modulationWaveformIndex;

    parameters.rateHz       = parsedParameters->rateHz;
    parameters.inputGainDB  = parsedParameters->inputGainDB;
    parameters.outputGainDB = parsedParameters->outputGainDB;
    parameters.isBypassed   = parsedParameters->isBypassed;

    return juce::Result::ok();
}
}  // namespace tremolo
