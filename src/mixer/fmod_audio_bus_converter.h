#ifndef FMOD_AUDIO_BUS_CONVERTER_H
#define FMOD_AUDIO_BUS_CONVERTER_H
#include "dsp/fmod_audio_effect.h"
#include <godot_cpp/classes/audio_effect.hpp>
namespace godot {
Ref<FmodAudioEffect> convert_godot_audio_effect(const Ref<AudioEffect> &effect);
void copy_fmod_effect_parameters(const Ref<FmodAudioEffect> &source, const Ref<FmodAudioEffect> &target);
} //namespace godot
#endif
