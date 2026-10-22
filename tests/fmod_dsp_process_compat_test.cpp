#include "dsp/fmod_dsp_process_compat.h"

#include <cassert>
#include <cstdio>

namespace {

unsigned int idle_samples;
FMOD_RESULT idle_result = FMOD_OK;
FMOD_RESULT processing_result = FMOD_OK;
FMOD_BOOL observed_idle;
bool silence;
bool bypass;
bool change_format;
int processed_blocks;

FMOD_RESULT F_CALL get_idle_time(FMOD_DSP_STATE*, unsigned int* samples) {
	*samples = idle_samples;
	return idle_result;
}

FMOD_RESULT F_CALL set_silence(FMOD_DSP_STATE*, FMOD_BOOL value) {
	silence = value != 0;
	return FMOD_OK;
}

FMOD_RESULT F_CALL set_bypass(FMOD_DSP_STATE*, FMOD_BOOL value) {
	bypass = value != 0;
	return FMOD_OK;
}

FMOD_RESULT F_CALL process_gain(FMOD_DSP_STATE*, unsigned int frames,
	const FmodDSPBufferArray* input, FmodDSPBufferArray* output,
	FMOD_BOOL idle, FmodDSPProcessOperation operation) {
	observed_idle = idle;
	if (operation == FMOD_PLAYER_DSP_PROCESS_QUERY) {
		if (change_format) {
			output->buffernumchannels[0] = 8;
		}
		return processing_result;
	}
	++processed_blocks;
	for (unsigned int i = 0; i < frames * input->buffernumchannels[0]; ++i) {
		output->buffers[0][i] = input->buffers[0][i] * 0.5f;
	}
	return FMOD_OK;
}

}

int main() {
	static_assert(FMOD_VERSION >= 0x00020400, "This test exercises the FMOD 2.04 ABI adapter");
	FMOD_DSP_STATE_FUNCTIONS functions = {};
	functions.getidletime = get_idle_time;
	functions.setsilence = set_silence;
	functions.setbypass = set_bypass;
	FMOD_DSP_STATE state = {};
	state.functions = &functions;
	float samples[] = {1.0f, -1.0f, 0.5f, -0.5f};
	float result[4] = {};
	FMOD_DSP_BUFFER input = {2, samples, FMOD_SPEAKERMODE_STEREO};
	FMOD_DSP_BUFFER output = {2, result, FMOD_SPEAKERMODE_STEREO};
	const FMOD_DSP_PROCESS_CALLBACK process = fmod_dsp_process_callback<process_gain>();

	assert(process(&state, 2, &input, &output) == FMOD_OK);
	assert(processed_blocks == 1 && !observed_idle);
	for (int i = 0; i < 4; ++i) {
		assert(result[i] == samples[i] * 0.5f);
	}
	idle_samples = 512;
	assert(process(&state, 2, &input, &output) == FMOD_OK && observed_idle);

	processing_result = FMOD_ERR_DSP_SILENCE;
	assert(process(&state, 2, &input, &output) == FMOD_OK && silence);
	assert(processed_blocks == 2);
	for (float sample : result) {
		assert(sample == 0.0f);
	}
	processing_result = FMOD_OK;
	assert(process(&state, 2, &input, &output) == FMOD_OK && !silence);
	processing_result = FMOD_ERR_DSP_DONTPROCESS;
	assert(process(&state, 2, &input, &output) == FMOD_OK && bypass);
	for (int i = 0; i < 4; ++i) {
		assert(result[i] == samples[i]);
	}

	processing_result = FMOD_ERR_INTERNAL;
	assert(process(&state, 2, &input, &output) == FMOD_ERR_INTERNAL);
	processing_result = FMOD_OK;
	change_format = true;
	assert(process(&state, 2, &input, &output) == FMOD_ERR_DSP_FORMAT);
	assert(processed_blocks == 3);
	change_format = false;
	idle_result = FMOD_ERR_INVALID_HANDLE;
	assert(process(&state, 2, &input, &output) == FMOD_ERR_INVALID_HANDLE);
	assert(process(nullptr, 2, &input, &output) == FMOD_ERR_INVALID_PARAM);
	assert(process(&state, 2, nullptr, &output) == FMOD_ERR_INVALID_PARAM);
	output.buffer = nullptr;
	assert(process(&state, 2, &input, &output) == FMOD_ERR_INVALID_PARAM);

	FMOD_DSP_DESCRIPTION description = {};
	fmod_configure_filter_description(description);
	assert(description.mode == FMOD_DSP_MODE_FILTER);
	assert(description.defaultcooldown == FMOD_DSP_COOLDOWN_INFINITE);
	assert(description.inchannels == 0 && description.outchannels == 0);
	std::puts("FMOD 2.04 DSP adapter tests passed");
}
