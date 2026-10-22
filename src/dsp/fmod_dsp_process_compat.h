#ifndef FMOD_DSP_PROCESS_COMPAT_H
#define FMOD_DSP_PROCESS_COMPAT_H

#include <fmod.h>
#include <fmod_dsp.h>

#include <cstring>

// Keep the extension's existing effect processing and script callback contract
// while adapting FMOD 2.04's single-buffer, process-only plugin interface.
#if FMOD_VERSION >= 0x00020400
struct FmodDSPBufferArray {
	int numbuffers;
	int* buffernumchannels;
	FMOD_CHANNELMASK* bufferchannelmask;
	float** buffers;
	FMOD_SPEAKERMODE speakermode;
};

enum FmodDSPProcessOperation {
	FMOD_PLAYER_DSP_PROCESS_PERFORM,
	FMOD_PLAYER_DSP_PROCESS_QUERY,
};
#else
using FmodDSPBufferArray = FMOD_DSP_BUFFER_ARRAY;
using FmodDSPProcessOperation = FMOD_DSP_PROCESS_OPERATION;
constexpr auto FMOD_PLAYER_DSP_PROCESS_PERFORM = FMOD_DSP_PROCESS_PERFORM;
constexpr auto FMOD_PLAYER_DSP_PROCESS_QUERY = FMOD_DSP_PROCESS_QUERY;
#endif

using FmodDSPProcessCallback = FMOD_RESULT(F_CALL*)(FMOD_DSP_STATE*, unsigned int,
	const FmodDSPBufferArray*, FmodDSPBufferArray*, FMOD_BOOL, FmodDSPProcessOperation);

inline void fmod_configure_filter_description(FMOD_DSP_DESCRIPTION& description) {
#if FMOD_VERSION >= 0x00020400
	description.mode = FMOD_DSP_MODE_FILTER;
	// Existing effects decide when to become idle themselves. In particular,
	// delay and reverb must continue processing after their input becomes idle.
	description.defaultcooldown = FMOD_DSP_COOLDOWN_INFINITE;
#else
	description.numinputbuffers = 1;
	description.numoutputbuffers = 1;
#endif
}

#if FMOD_VERSION >= 0x00020400
template <FmodDSPProcessCallback Callback>
FMOD_RESULT F_CALL fmod_process_single_buffer(FMOD_DSP_STATE* state, unsigned int length,
	const FMOD_DSP_BUFFER* input, const FMOD_DSP_BUFFER* output) {
	if (!state || !state->functions || !input || !output ||
		input->numchannels <= 0 || output->numchannels <= 0 || !input->buffer || !output->buffer) {
		return FMOD_ERR_INVALID_PARAM;
	}
	int input_channels = input->numchannels;
	int output_channels = output->numchannels;
	FMOD_CHANNELMASK input_mask = state->channelmask;
	FMOD_CHANNELMASK output_mask = input_mask;
	float* input_samples = input->buffer;
	float* output_samples = output->buffer;
	FmodDSPBufferArray inputs = { 1, &input_channels, &input_mask, &input_samples, input->speakermode };
	FmodDSPBufferArray outputs = { 1, &output_channels, &output_mask, &output_samples, output->speakermode };
	unsigned int idle_time = 0;
	if (state->functions->getidletime) {
		FMOD_RESULT result = FMOD_DSP_GETIDLETIME(state, &idle_time);
		if (result != FMOD_OK) {
			return result;
		}
	}
	const FMOD_BOOL inputs_idle = idle_time > 0;
	FMOD_RESULT result = Callback(state, length, &inputs, &outputs, inputs_idle, FMOD_PLAYER_DSP_PROCESS_QUERY);
	// The existing effects preserve their input format. FMOD 2.04 owns the
	// output allocation, so a legacy callback must not change its dimensions.
	if (output_channels != output->numchannels || outputs.speakermode != output->speakermode) {
		return FMOD_ERR_DSP_FORMAT;
	}
	if (result == FMOD_OK) {
		result = Callback(state, length, &inputs, &outputs, inputs_idle, FMOD_PLAYER_DSP_PROCESS_PERFORM);
	}
	if (result == FMOD_ERR_DSP_SILENCE) {
		std::memset(output->buffer, 0, static_cast<size_t>(length) * output->numchannels * sizeof(float));
		return state->functions->setsilence ? state->functions->setsilence(state, true) : FMOD_OK;
	}
	if (result == FMOD_ERR_DSP_DONTPROCESS) {
		if (input->numchannels != output->numchannels) {
			return FMOD_ERR_DSP_FORMAT;
		}
		std::memmove(output->buffer, input->buffer, static_cast<size_t>(length) * input->numchannels * sizeof(float));
		return state->functions->setbypass ? state->functions->setbypass(state, true) : FMOD_OK;
	}
	if (result == FMOD_OK && state->functions->setsilence) {
		return state->functions->setsilence(state, false);
	}
	return result;
}
#endif

template <FmodDSPProcessCallback Callback>
constexpr FMOD_DSP_PROCESS_CALLBACK fmod_dsp_process_callback() {
#if FMOD_VERSION >= 0x00020400
	return fmod_process_single_buffer<Callback>;
#else
	return Callback;
#endif
}

#endif
