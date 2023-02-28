//------------------------------------------------------------------------
// Copyright(c) 2023 yg331.
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"

using namespace Steinberg;

namespace yg331 {

//------------------------------------------------------------------------
//  VariMuProcessor
//------------------------------------------------------------------------
class VariMuProcessor : public Steinberg::Vst::AudioEffect
{
public:
	VariMuProcessor ();
	~VariMuProcessor () SMTG_OVERRIDE;

    // Create function
	static FUnknown* createInstance (void* /*context*/) 
	{ 
		return (Vst::IAudioProcessor*)new VariMuProcessor; 
	}

	//--- ---------------------------------------------------------------------
	// AudioEffect overrides:
	//--- ---------------------------------------------------------------------
	/** Called at first after constructor */
	tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
	
	/** Called at the end before destructor */
	tresult PLUGIN_API terminate () SMTG_OVERRIDE;

	tresult PLUGIN_API setBusArrangements(
		Vst::SpeakerArrangement* inputs, int32 numIns,
		Vst::SpeakerArrangement* outputs, int32 numOuts
	) SMTG_OVERRIDE;
	
	/** Switch the Plug-in on/off */
	tresult PLUGIN_API setActive (TBool state) SMTG_OVERRIDE;

	/** Will be called before any process call */
	tresult PLUGIN_API setupProcessing (Vst::ProcessSetup& newSetup) SMTG_OVERRIDE;
	
	/** Asks if a given sample size is supported see SymbolicSampleSizes. */
	tresult PLUGIN_API canProcessSampleSize (int32 symbolicSampleSize) SMTG_OVERRIDE;

	/** Here we go...the process call */
	tresult PLUGIN_API process (Vst::ProcessData& data) SMTG_OVERRIDE;
		
	/** For persistence */
	tresult PLUGIN_API setState (IBStream* state) SMTG_OVERRIDE;
	tresult PLUGIN_API getState (IBStream* state) SMTG_OVERRIDE;
	
//------------------------------------------------------------------------
protected:

	Vst::Sample64 VuPPMconvert(Vst::Sample64 plainValue);

	Vst::Sample64 norm_to_gain(Vst::Sample64 plainValue);

	template <typename SampleType>
	void processAudio(SampleType** inputs, SampleType** outputs, double getSampleRate, int32 sampleFrames, int32 precision);

	template <typename SampleType>
	void processVuPPM(SampleType** inputs, int32 sampleFrames);

	Vst::Sample64 muVaryL = 1;
	Vst::Sample64 muAttackL = 0;
	Vst::Sample64 muNewSpeedL = 0;
	Vst::Sample64 muSpeedAL = 10000;
	Vst::Sample64 muSpeedBL = 10000;
	Vst::Sample64 muCoefficientAL = 1;
	Vst::Sample64 muCoefficientBL = 1;
	Vst::Sample64 previousL = 0;

	Vst::Sample64 muVaryR = 1;
	Vst::Sample64 muAttackR = 0;
	Vst::Sample64 muNewSpeedR = 0;
	Vst::Sample64 muSpeedAR = 10000;
	Vst::Sample64 muSpeedBR = 10000;
	Vst::Sample64 muCoefficientAR = 1;
	Vst::Sample64 muCoefficientBR = 1;
	Vst::Sample64 previousR = 0;

	bool flip = false;

	uint32_t fpdL = 1.0;
	uint32_t fpdR = 1.0;
	//default stuff

	Vst::Sample64 In_db = 1.0;
	Vst::Sample64 Out_db = 1.0;

	float fParamInput = 0.5;
	float fParamOutput = 0.5;
	float fParamIntensity = 0.0;
	float fParamSpeed = 0.5;
	float fParamTrim = 1.0;
	float fParamMix = 1.0;
	float fInVuPPMLOld = 0.f;
	float fInVuPPMROld = 0.f;
	float fOutVuPPMLOld = 0.f;
	float fOutVuPPMROld = 0.f;
	float fInVuPPML = 0.f;
	float fInVuPPMR = 0.f;
	float fOutVuPPML = 0.f;
	float fOutVuPPMR = 0.f;
	float fOutGRVuPPM = 0.f;
	float fOutGRVuPPMOld = 0.f;
	bool bBypass = false;

};

//------------------------------------------------------------------------
} // namespace yg331
