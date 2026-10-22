#include "dsp/fmod_audio_effect_spectrum_analyzer.h"
#include "core/fmod_server.h"
#include "core/fmod_system.h"
#include "playback/fmod_channel_group.h"
#include <fmod_dsp_effects.h>
#include <godot_cpp/core/class_db.hpp>
#include <cmath>

namespace godot {
	static int _fft_size_to_value(FmodAudioEffectSpectrumAnalyzer::FFTSize size) {
		switch (size) {
			case FmodAudioEffectSpectrumAnalyzer::FFT_SIZE_256: return 256;
			case FmodAudioEffectSpectrumAnalyzer::FFT_SIZE_512: return 512;
			case FmodAudioEffectSpectrumAnalyzer::FFT_SIZE_1024: return 1024;
			case FmodAudioEffectSpectrumAnalyzer::FFT_SIZE_2048: return 2048;
			case FmodAudioEffectSpectrumAnalyzer::FFT_SIZE_4096: return 4096;
			default: return 1024;
		}
	}

	static float _get_system_sample_rate() {
		FmodServer* server = FmodServer::get_singleton();
		if (!server) {
			return 48000.0f;
		}

		Ref<FmodSystem> system = server->get_main_system();
		if (system.is_null() || system->get_system() == nullptr) {
			return 48000.0f;
		}

		int rate = 0;
		FMOD_SPEAKERMODE speaker_mode = FMOD_SPEAKERMODE_DEFAULT;
		int raw_speakers = 0;
		if (system->get_system()->getSoftwareFormat(&rate, &speaker_mode, &raw_speakers) == FMOD_OK && rate > 0) {
			return static_cast<float>(rate);
		}

		return 48000.0f;
	}

	void FmodAudioEffectSpectrumAnalyzer::_bind_methods() {
		BIND_ENUM_CONSTANT(FFT_SIZE_256);
		BIND_ENUM_CONSTANT(FFT_SIZE_512);
		BIND_ENUM_CONSTANT(FFT_SIZE_1024);
		BIND_ENUM_CONSTANT(FFT_SIZE_2048);
		BIND_ENUM_CONSTANT(FFT_SIZE_4096);
		BIND_ENUM_CONSTANT(FFT_SIZE_MAX);

		BIND_ENUM_CONSTANT(MAGNITUDE_AVERAGE);
		BIND_ENUM_CONSTANT(MAGNITUDE_MAX);

		ClassDB::bind_method(D_METHOD("set_buffer_length", "buffer_length"), &FmodAudioEffectSpectrumAnalyzer::set_buffer_length);
		ClassDB::bind_method(D_METHOD("get_buffer_length"), &FmodAudioEffectSpectrumAnalyzer::get_buffer_length);
		ClassDB::bind_method(D_METHOD("set_fft_size", "fft_size"), &FmodAudioEffectSpectrumAnalyzer::set_fft_size);
		ClassDB::bind_method(D_METHOD("get_fft_size"), &FmodAudioEffectSpectrumAnalyzer::get_fft_size);
		ClassDB::bind_method(D_METHOD("update_spectrum"), &FmodAudioEffectSpectrumAnalyzer::update_spectrum);
		ClassDB::bind_method(D_METHOD("get_bin_count"), &FmodAudioEffectSpectrumAnalyzer::get_bin_count);
		ClassDB::bind_method(D_METHOD("get_rms"), &FmodAudioEffectSpectrumAnalyzer::get_rms);
		ClassDB::bind_method(D_METHOD("get_centroid"), &FmodAudioEffectSpectrumAnalyzer::get_centroid);
		ClassDB::bind_method(D_METHOD("get_magnitude_for_frequency_range", "begin", "end", "mode"), &FmodAudioEffectSpectrumAnalyzer::get_magnitude_for_frequency_range, DEFVAL(MAGNITUDE_MAX));

		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "buffer_length", PROPERTY_HINT_RANGE, "0.1,10,0.1,suffix:s"), "set_buffer_length", "get_buffer_length");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "fft_size", PROPERTY_HINT_ENUM, "256,512,1024,2048,4096"), "set_fft_size", "get_fft_size");
	}

	FmodAudioEffectSpectrumAnalyzer::FmodAudioEffectSpectrumAnalyzer() {
	}

	FmodAudioEffectSpectrumAnalyzer::~FmodAudioEffectSpectrumAnalyzer() {
	}

	void FmodAudioEffectSpectrumAnalyzer::apply_to(Ref<FmodChannelGroup> p_bus) {
		ERR_FAIL_COND_MSG(p_bus.is_null(), "Invalid bus");

		if (bus.is_valid()) {
			remove_from_bus(bus);
		}

		bus = p_bus;
		_invalidate_snapshot();
		sample_rate = _get_system_sample_rate();

		Ref<FmodSystem> system = FmodServer::get_singleton()->get_main_system();
		ERR_FAIL_COND_MSG(system.is_null(), "FMOD system not initialized");

		fft_dsp = system->create_dsp_by_type(FmodDSP::DSP_TYPE_FFT);
		if (!fft_dsp.is_valid()) {
			return;
		}

		fft_dsp->set_parameter_int(FMOD_DSP_FFT_WINDOWSIZE, _fft_size_to_value(fft_size));
		fft_dsp->set_parameter_int(FMOD_DSP_FFT_WINDOW, FMOD_DSP_FFT_WINDOW_HANNING);
		fft_dsp->set_parameter_float(FMOD_DSP_FFT_BAND_START_FREQ, 0.0f);
		fft_dsp->set_parameter_float(FMOD_DSP_FFT_BAND_STOP_FREQ, sample_rate * 0.5f);
		fft_dsp->set_parameter_int(FMOD_DSP_FFT_DOWNMIX, FMOD_DSP_FFT_DOWNMIX_NONE);
		fft_dsp->set_parameter_int(FMOD_DSP_FFT_CHANNEL, -1);

		bus->add_dsp(-1, fft_dsp);
		dsp_chain.push_back(fft_dsp);
	}

	void FmodAudioEffectSpectrumAnalyzer::remove_from_bus(Ref<FmodChannelGroup> p_bus) {
		FmodAudioEffect::remove_from_bus(p_bus);
		fft_dsp.unref();
		_invalidate_snapshot();
	}

	void FmodAudioEffectSpectrumAnalyzer::_invalidate_snapshot() {
		snapshot_valid = false;
		spectrum_data.clear();
		current_rms = 0.0f;
		current_centroid = 0.0f;
	}

	void FmodAudioEffectSpectrumAnalyzer::set_buffer_length(float p_buffer_length) {
		buffer_length = CLAMP(p_buffer_length, 0.1f, 10.0f);
	}

	float FmodAudioEffectSpectrumAnalyzer::get_buffer_length() const {
		return buffer_length;
	}

	void FmodAudioEffectSpectrumAnalyzer::set_fft_size(FFTSize p_fft_size) {
		if (p_fft_size >= FFT_SIZE_256 && p_fft_size < FFT_SIZE_MAX) {
			if (fft_size != p_fft_size) {
				_invalidate_snapshot();
			}
			fft_size = p_fft_size;
			if (fft_dsp.is_valid()) {
				fft_dsp->set_parameter_int(FMOD_DSP_FFT_WINDOWSIZE, _fft_size_to_value(fft_size));
			}
		}
	}

	FmodAudioEffectSpectrumAnalyzer::FFTSize FmodAudioEffectSpectrumAnalyzer::get_fft_size() const {
		return fft_size;
	}

	void FmodAudioEffectSpectrumAnalyzer::update_spectrum() {
		_refresh_spectrum(true);
	}

	void FmodAudioEffectSpectrumAnalyzer::_refresh_spectrum(bool p_force) {
		if (fft_dsp.is_null() || fft_dsp->dsp == nullptr) {
			return;
		}

		// Use the audio clock rather than a Godot frame counter: manual/NRT
		// mixing can advance several audio blocks within the same game frame.
		unsigned long long dsp_clock = 0;
		const bool clock_valid = bus.is_valid() && bus->channel_group &&
			bus->channel_group->getDSPClock(&dsp_clock, nullptr) == FMOD_OK;
		if (!p_force && clock_valid && snapshot_valid && snapshot_dsp_clock == dsp_clock) {
			return;
		}

		sample_rate = _get_system_sample_rate();

		void* raw_fft_data = nullptr;
		unsigned int raw_fft_data_length = 0;
		FMOD_RESULT result = fft_dsp->dsp->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, &raw_fft_data, &raw_fft_data_length, nullptr, 0);
		if (result != FMOD_OK || raw_fft_data == nullptr) {
			return;
		}

		FMOD_DSP_PARAMETER_FFT* fft_data = static_cast<FMOD_DSP_PARAMETER_FFT*>(raw_fft_data);
		if (fft_data->length <= 0 || fft_data->numchannels <= 0) {
			return;
		}

		spectrum_data.resize(fft_data->length);
		Vector2* spectrum = spectrum_data.ptrw();
		const float* left_spectrum = fft_data->spectrum[0];
		const float* right_spectrum = fft_data->numchannels > 1 ? fft_data->spectrum[1] : nullptr;
		for (int i = 0; i < fft_data->length; i++) {
			const float left = left_spectrum ? left_spectrum[i] : 0.0f;
			const float right = right_spectrum ? right_spectrum[i] : left;
			spectrum[i] = Vector2(left, right);
		}

		float value = 0.0f;
		if (fft_dsp->dsp->getParameterFloat(FMOD_DSP_FFT_RMS, &value, nullptr, 0) == FMOD_OK) {
			current_rms = value;
		}

		if (fft_dsp->dsp->getParameterFloat(FMOD_DSP_FFT_SPECTRAL_CENTROID, &value, nullptr, 0) == FMOD_OK) {
			current_centroid = value;
		}
		// Retain the clock read before copying. If the mixer advances during
		// the copy, the next getter refreshes instead of caching under a newer clock.
		snapshot_dsp_clock = dsp_clock;
		snapshot_valid = clock_valid;
	}

	int FmodAudioEffectSpectrumAnalyzer::get_bin_count() const {
		const_cast<FmodAudioEffectSpectrumAnalyzer*>(this)->_refresh_spectrum(false);
		return spectrum_data.size();
	}

	float FmodAudioEffectSpectrumAnalyzer::get_rms() const {
		const_cast<FmodAudioEffectSpectrumAnalyzer*>(this)->_refresh_spectrum(false);
		return current_rms;
	}

	float FmodAudioEffectSpectrumAnalyzer::get_centroid() const {
		const_cast<FmodAudioEffectSpectrumAnalyzer*>(this)->_refresh_spectrum(false);
		return current_centroid;
	}

	Vector2 FmodAudioEffectSpectrumAnalyzer::get_magnitude_for_frequency_range(float p_begin, float p_end, MagnitudeMode p_mode) const {
		if (p_begin > p_end) {
			float temp = p_begin;
			p_begin = p_end;
			p_end = temp;
		}

		const_cast<FmodAudioEffectSpectrumAnalyzer*>(this)->_refresh_spectrum(false);

		if (spectrum_data.is_empty() || p_begin == p_end) {
			return Vector2();
		}

		int num_bins = spectrum_data.size();
		// FMOD returns the full FFT window, including its mirrored half.
		// Each bin spans sample_rate / num_bins Hz, not Nyquist / num_bins.
		const float bin_hz = MAX(sample_rate, 1.0f) / num_bins;
		const int last_positive_bin = num_bins / 2;

		int begin_bin = static_cast<int>(std::floor(MAX(p_begin, 0.0f) / bin_hz));
		int end_bin = static_cast<int>(std::ceil(MAX(p_end, 0.0f) / bin_hz));

		begin_bin = CLAMP(begin_bin, 0, last_positive_bin);
		end_bin = CLAMP(end_bin, 0, last_positive_bin);

		Vector2 max_magnitude;
		Vector2 avg_magnitude;
		int count = 0;
		const Vector2* spectrum = spectrum_data.ptr();

		for (int i = begin_bin; i <= end_bin; i++) {
			const Vector2& magnitude = spectrum[i];
			if (p_mode == MAGNITUDE_MAX) {
				max_magnitude.x = MAX(max_magnitude.x, magnitude.x);
				max_magnitude.y = MAX(max_magnitude.y, magnitude.y);
			} else {
				avg_magnitude += magnitude;
			}
			count++;
		}

		if (p_mode != MAGNITUDE_MAX && count > 0) {
			avg_magnitude /= static_cast<float>(count);
		}

		return p_mode == MAGNITUDE_MAX ? max_magnitude : avg_magnitude;
	}

	void FmodAudioEffectSpectrumAnalyzer::_update_spectrum_data(const float* spectrum, int num_bins, float p_sample_rate) {
		if (spectrum == nullptr || num_bins <= 0) {
			return;
		}

		sample_rate = p_sample_rate > 0.0f ? p_sample_rate : sample_rate;
		spectrum_data.resize(num_bins);
		Vector2* data = spectrum_data.ptrw();
		for (int i = 0; i < num_bins; i++) {
			data[i] = Vector2(spectrum[i], spectrum[i]);
		}
	}

	void FmodAudioEffectSpectrumAnalyzer::_update_rms(float rms) {
		current_rms = rms;
	}

	void FmodAudioEffectSpectrumAnalyzer::_update_centroid(float centroid) {
		current_centroid = centroid;
	}
}
