#ifndef FMOD_DSP_HELPERS_H
#define FMOD_DSP_HELPERS_H

#include <fmod.h>
#include <fmod_dsp.h>
#include <cstring>

static_assert(FMOD_VERSION >= 0x00020400, "FMOD 2.04 or newer is required");

inline void fmod_configure_filter_description(FMOD_DSP_DESCRIPTION& description) {
	description.mode = FMOD_DSP_MODE_FILTER;
	// Delay and reverb keep processing their tails after the input becomes idle.
	description.defaultcooldown = FMOD_DSP_COOLDOWN_INFINITE;
}

inline bool fmod_dsp_input_is_idle(FMOD_DSP_STATE* state) {
	unsigned int idle_time = 0;
	return state && state->functions && state->functions->getidletime &&
		FMOD_DSP_GETIDLETIME(state, &idle_time) == FMOD_OK && idle_time > 0;
}

inline FMOD_RESULT fmod_dsp_passthrough(unsigned int frames, const FMOD_DSP_BUFFER* input, const FMOD_DSP_BUFFER* output) {
	if (!output || !output->buffer || output->numchannels <= 0) {
		return FMOD_ERR_INVALID_PARAM;
	}
	const size_t samples = static_cast<size_t>(frames) * output->numchannels;
	if (input && input->buffer && input->numchannels == output->numchannels) {
		std::memmove(output->buffer, input->buffer, samples * sizeof(float));
	} else {
		std::memset(output->buffer, 0, samples * sizeof(float));
	}
	return FMOD_OK;
}

#endif
