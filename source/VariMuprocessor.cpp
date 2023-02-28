//------------------------------------------------------------------------
// Copyright(c) 2023 yg331.
//------------------------------------------------------------------------

#include "VariMuprocessor.h"
#include "VariMucids.h"
#include "VariMuparam.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "public.sdk/source/vst/vsthelpers.h"

using namespace Steinberg;

namespace yg331 {
	//------------------------------------------------------------------------
	// VariMuProcessor
	//------------------------------------------------------------------------
	VariMuProcessor::VariMuProcessor()
	{
		//--- set the wanted controller for our processor
		setControllerClass(kVariMuControllerUID);
	}

	//------------------------------------------------------------------------
	VariMuProcessor::~VariMuProcessor()
	{}

	//------------------------------------------------------------------------
	tresult PLUGIN_API VariMuProcessor::initialize(FUnknown* context)
	{
		// Here the Plug-in will be instantiated

		//---always initialize the parent-------
		tresult result = AudioEffect::initialize(context);
		// if everything Ok, continue
		if (result != kResultOk)
		{
			return result;
		}

		//--- create Audio IO ------
		addAudioInput(STR16("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
		addAudioOutput(STR16("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

		/* If you don't need an event bus, you can remove the next line */
		addEventInput(STR16("Event In"), 1);

		fpdL = 1.0; while (fpdL < 16386) fpdL = rand() * UINT32_MAX;
		fpdR = 1.0; while (fpdR < 16386) fpdR = rand() * UINT32_MAX;
		In_db = norm_to_gain((Vst::Sample64)fParamInput);
		Out_db = norm_to_gain((Vst::Sample64)fParamOutput);

		return kResultOk;
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API VariMuProcessor::terminate()
	{
		// Here the Plug-in will be de-instantiated, last possibility to remove some memory!

		//---do not forget to call parent ------
		return AudioEffect::terminate();
	}

	tresult PLUGIN_API VariMuProcessor::setBusArrangements(
		Vst::SpeakerArrangement* inputs, int32 numIns,
		Vst::SpeakerArrangement* outputs, int32 numOuts)
	{

		/*
		// we only support one in and output bus and these busses must have the same number of channels
		if (numIns == 1 && numOuts == 1 && inputs[0] == outputs[0])
			return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
		return kResultFalse;
		*/


		if (numIns == 1 && numOuts == 1)
		{
			// the host wants Stereo => Stereo (or 2 channel -> 2 channel)
			if (Vst::SpeakerArr::getChannelCount(inputs[0])  == 2 &&
				Vst::SpeakerArr::getChannelCount(outputs[0]) == 2)
			{
				auto* bus = FCast<Vst::AudioBus>(audioInputs.at(0));
				if (bus)
				{
					// check if we are Mono => Mono, if not we need to recreate the busses
					if (bus->getArrangement() != inputs[0])
					{
						getAudioInput(0)->setArrangement(inputs[0]);
						getAudioInput(0)->setName(STR16("Stereo In"));
						getAudioOutput(0)->setArrangement(outputs[0]);
						getAudioOutput(0)->setName(STR16("Stereo Out"));
					}
					return kResultOk;
				}
			}
			// the host wants something else than Stereo => Stereo
			// in this case we are always Stereo => Stereo
			else
			{
				auto* bus = FCast<Vst::AudioBus>(audioInputs.at(0));
				if (bus)
				{
					tresult result = kResultFalse;

					if (bus->getArrangement() != Vst::SpeakerArr::kStereo)
					{
						getAudioInput(0)->setArrangement(Vst::SpeakerArr::kStereo);
						getAudioInput(0)->setName(STR16("Stereo In"));
						getAudioOutput(0)->setArrangement(Vst::SpeakerArr::kStereo);
						getAudioOutput(0)->setName(STR16("Stereo Out"));
						result = kResultFalse;
					}

					return result;
				}
			}
		}
		return kResultFalse;
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API VariMuProcessor::setActive(TBool state)
	{
		//--- called when the Plug-in is enable/disable (On/Off) -----
		return AudioEffect::setActive(state);
		// reset the VuMeter value
		fInVuPPMLOld = 0.f;
		fInVuPPMROld = 0.f;
		fOutVuPPMLOld = 0.f;
		fOutVuPPMROld = 0.f;
		fInVuPPML = 0.f;
		fInVuPPMR = 0.f;
		fOutVuPPML = 0.f;
		fOutVuPPMR = 0.f;
		fOutGRVuPPM = 0.f;
		fOutGRVuPPMOld = 0.f;
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API VariMuProcessor::process(Vst::ProcessData& data)
	{
		//---1) Read inputs parameter changes-----------

		Vst::IParameterChanges* paramChanges = data.inputParameterChanges;


		if (paramChanges)
		{
			int32 numParamsChanged = paramChanges->getParameterCount();

			for (int32 index = 0; index < numParamsChanged; index++)
			{
				Vst::IParamValueQueue* paramQueue = paramChanges->getParameterData(index);

				if (paramQueue)
				{
					Vst::ParamValue value;
					int32 sampleOffset;
					int32 numPoints = paramQueue->getPointCount();

					/*/*/
					if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
						switch (paramQueue->getParameterId()) {
						case kParamInput:
							fParamInput = (float)value;
							In_db = norm_to_gain((Vst::Sample64)value);
							break;
						case kParamOutput:
							fParamOutput = (float)value;
							Out_db = norm_to_gain((Vst::Sample64)value);
							break;
						case kParamIntensity:
							fParamIntensity = (float)value;
							break;
						case kParamSpeed:
							fParamSpeed = (float)value;
							break;
						case kParamTrim:
							fParamTrim = (float)value;
							break;
						case kParamMix:
							fParamMix = (float)value;
							break;
						case kParamBypass:
							bBypass = (value > 0.5f);
							break;
						}
					}
				}
			}
		}

		//--- Here you have to implement your processing



		if (data.numInputs == 0 || data.numOutputs == 0) {
			return kResultOk;
		}

		// (simplification) we suppose in this example that we have the same input channel count than the output
		// int32 numChannels = data.inputs[0].numChannels;

		//---get audio buffers----------------
		uint32 sampleFramesSize = getSampleFramesSizeInBytes(processSetup, data.numSamples);
		void** in = getChannelBuffersPointer(processSetup, data.inputs[0]);
		void** out = getChannelBuffersPointer(processSetup, data.outputs[0]);
		double getSampleRate = processSetup.sampleRate;


		//---check if silence---------------
		// normally we have to check each channel (simplification)
		if (data.inputs[0].silenceFlags != 0) // if flags is not zero => then it means that we have silent!
		{
			// in the example we are applying a gain to the input signal
			// As we know that the input is only filled with zero, the output will be then filled with zero too!

			data.outputs[0].silenceFlags = 0;

			if (data.inputs[0].silenceFlags & (uint64)1) { // Left
				if (in[0] != out[0])
				{
					memset(out[0], 0, sampleFramesSize); // this is faster than applying a gain to each sample!
				}
				data.outputs[0].silenceFlags |= (uint64)1 << (uint64)0;
			}

			if (data.inputs[0].silenceFlags & (uint64)2) { // Right
				if (in[1] != out[1])
				{
					memset(out[1], 0, sampleFramesSize); // this is faster than applying a gain to each sample!
				}
				data.outputs[0].silenceFlags |= (uint64)1 << (uint64)1;
			}

			if (data.inputs[0].silenceFlags & (uint64)3) {
				return kResultOk;
			}
		}

		data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;

		fInVuPPML = 0.f;
		fInVuPPMR = 0.f;
		fOutVuPPML = 0.f;
		fOutVuPPMR = 0.f;
		fOutGRVuPPM = 0.f;

		//---in bypass mode outputs should be like inputs-----
		if (bBypass)
		{
			if (in[0] != out[0]) { memcpy(out[0], in[0], sampleFramesSize); }
			if (in[1] != out[1]) { memcpy(out[1], in[1], sampleFramesSize); }

			if (data.symbolicSampleSize == Vst::kSample32) {
				processVuPPM<Vst::Sample32>((Vst::Sample32**)in, data.numSamples);
			}
			else if (data.symbolicSampleSize == Vst::kSample64) {
				processVuPPM<Vst::Sample64>((Vst::Sample64**)in, data.numSamples);
			}
		}
		else
		{
			if (data.symbolicSampleSize == Vst::kSample32) {
				int32 aa = Vst::kSample32;
				processAudio<Vst::Sample32>((Vst::Sample32**)in, (Vst::Sample32**)out, getSampleRate, data.numSamples, aa);
			}
			else {
				int32 aa = Vst::kSample64;
				processAudio<Vst::Sample64>((Vst::Sample64**)in, (Vst::Sample64**)out, getSampleRate, data.numSamples, aa);
			}
		}

		//---3) Write outputs parameter changes-----------
		Vst::IParameterChanges* outParamChanges = data.outputParameterChanges;
		// a new value of VuMeter will be send to the host
		// (the host will send it back in sync to our controller for updating our editor)
		if (outParamChanges && (fInVuPPMLOld != fInVuPPML || fInVuPPMROld != fInVuPPMR || fOutVuPPMLOld != fOutVuPPML || fOutVuPPMLOld != fOutVuPPMR || fOutGRVuPPMOld != fOutGRVuPPM))
		{
			int32 index = 0;
			Vst::IParamValueQueue* paramQueue = outParamChanges->addParameterData(kParamInVuPPML, index);
			if (paramQueue)
			{
				int32 index2 = 0;
				paramQueue->addPoint(0, fInVuPPML, index2);
			}
			index = 0;
			paramQueue = outParamChanges->addParameterData(kParamInVuPPMR, index);
			if (paramQueue)
			{
				int32 index2 = 0;
				paramQueue->addPoint(0, fInVuPPMR, index2);
			}
			index = 0;
			paramQueue = outParamChanges->addParameterData(kParamOutVuPPML, index);
			if (paramQueue)
			{
				int32 index2 = 0;
				paramQueue->addPoint(0, fOutVuPPML, index2);
			}
			index = 0;
			paramQueue = outParamChanges->addParameterData(kParamOutVuPPMR, index);
			if (paramQueue)
			{
				int32 index2 = 0;
				paramQueue->addPoint(0, fOutVuPPMR, index2);
			}
			index = 0;
			paramQueue = outParamChanges->addParameterData(kParamGRVuPPM, index);
			if (paramQueue)
			{
				int32 index2 = 0;
				paramQueue->addPoint(0, fOutGRVuPPM, index2);
			}

		}
		fInVuPPMLOld = fInVuPPML;
		fInVuPPMROld = fInVuPPMR;
		fOutVuPPMLOld = fOutVuPPMR;
		fOutVuPPMLOld = fOutVuPPMR;
		fOutGRVuPPMOld = fOutGRVuPPM;

		return kResultOk;
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API VariMuProcessor::setupProcessing(Vst::ProcessSetup& newSetup)
	{
		//--- called before any processing ----
		return AudioEffect::setupProcessing(newSetup);
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API VariMuProcessor::canProcessSampleSize(int32 symbolicSampleSize)
	{
		// by default kSample32 is supported
		if (symbolicSampleSize == Vst::kSample32)
			return kResultTrue;

		// disable the following comment if your processing support kSample64
		if (symbolicSampleSize == Vst::kSample64)
			return kResultTrue;

		return kResultFalse;
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API VariMuProcessor::setState(IBStream* state)
	{
		// called when we load a preset, the model has to be reloaded
		IBStreamer streamer(state, kLittleEndian);

		float savedInput = 0.f;
		if (streamer.readFloat(savedInput) == false)
			return kResultFalse;

		float savedOutput = 0.f;
		if (streamer.readFloat(savedOutput) == false)
			return kResultFalse;

		float savedIntensity = 0.f;
		if (streamer.readFloat(savedIntensity) == false)
			return kResultFalse;

		float savedSpeed = 0.f;
		if (streamer.readFloat(savedSpeed) == false)
			return kResultFalse;

		float savedTrim = 0.f;
		if (streamer.readFloat(savedTrim) == false)
			return kResultFalse;

		float savedMix = 0.f;
		if (streamer.readFloat(savedMix) == false)
			return kResultFalse;

		int32 savedBypass = 0;
		if (streamer.readInt32(savedBypass) == false)
			return kResultFalse;

		fParamInput = savedInput;
		fParamOutput = savedOutput;
		fParamIntensity = savedIntensity;
		fParamSpeed = savedSpeed;
		fParamTrim = savedTrim;
		fParamMix = savedMix;
		bBypass = savedBypass > 0;

		if (Vst::Helpers::isProjectState(state) == kResultTrue)
		{
			// we are in project loading context...

			// Example of using the IStreamAttributes interface
			FUnknownPtr<Vst::IStreamAttributes> stream(state);
			if (stream)
			{
				if (Vst::IAttributeList* list = stream->getAttributes())
				{
					// get the full file path of this state
					Vst::TChar fullPath[1024];
					memset(fullPath, 0, 1024 * sizeof(Vst::TChar));
					if (list->getString(Vst::PresetAttributes::kFilePathStringType, fullPath,
						1024 * sizeof(Vst::TChar)) == kResultTrue)
					{
						// here we have the full path ...
					}
				}
			}
		}

		return kResultOk;
	}

	//------------------------------------------------------------------------
	tresult PLUGIN_API VariMuProcessor::getState(IBStream* state)
	{
		// here we need to save the model
		IBStreamer streamer(state, kLittleEndian);

		streamer.writeFloat(fParamInput);
		streamer.writeFloat(fParamOutput);
		streamer.writeFloat(fParamIntensity);
		streamer.writeFloat(fParamSpeed);
		streamer.writeFloat(fParamTrim);
		streamer.writeFloat(fParamMix);
		streamer.writeInt32(bBypass ? 1 : 0);

		return kResultOk;
	}

	template <typename SampleType>
	void VariMuProcessor::processAudio(SampleType** inputs, SampleType** outputs, double getSampleRate, int32 sampleFrames, int32 precision)
	{
		SampleType* in1 = (SampleType*)inputs[0];
		SampleType* in2 = (SampleType*)inputs[1];
		SampleType* out1 = (SampleType*)outputs[0];
		SampleType* out2 = (SampleType*)outputs[1];

		SampleType tmpInL = 0.0; /*/ VuPPM /*/
		SampleType tmpInR = 0.0; /*/ VuPPM /*/
		SampleType tmpOutL = 0.0; /*/ VuPPM /*/
		SampleType tmpOutR = 0.0; /*/ VuPPM /*/
		SampleType tmpGR = 1.0;

		Vst::Sample64 overallscale = 2.0;
		overallscale /= 44100.0;
		overallscale *= getSampleRate;

		Vst::Sample64 threshold = 1.001 - (1.0 - pow(1.0 - fParamIntensity, 3));

		Vst::Sample64 muMakeupGain = sqrt(1.0 / threshold);
		muMakeupGain = (muMakeupGain + sqrt(muMakeupGain)) / 2.0;
		muMakeupGain = sqrt(muMakeupGain);

		Vst::Sample64 outGain = sqrt(muMakeupGain);
		//gain settings around threshold

		Vst::Sample64 release = pow((1.15 - fParamSpeed), 5) * 32768.0;
		release /= overallscale;

		Vst::Sample64 fastest = sqrt(release);

		//speed settings around release
		Vst::Sample64 coefficient;
		Vst::Sample64 output = outGain * fParamTrim;
		Vst::Sample64 wet = fParamMix;
		Vst::Sample64 squaredSampleL;
		Vst::Sample64 squaredSampleR;

		// µ µ µ µ µ µ µ µ µ µ µ µ is the kitten song o/~

		while (--sampleFrames >= 0)
		{
			Vst::Sample64 inputSampleL = *in1;
			Vst::Sample64 inputSampleR = *in2;

			/*/ VuPPM /*/
			inputSampleL *= In_db;
			inputSampleR *= In_db;
			if (inputSampleL > tmpInL) { tmpInL = inputSampleL; }
			if (inputSampleR > tmpInR) { tmpInR = inputSampleR; }

			static int32 noisesourceL = 0;
			static int32 noisesourceR = 850010;

			int32 residue;
			Vst::Sample64 applyresidue;

			noisesourceL = noisesourceL % 1700021; noisesourceL++;
			residue = noisesourceL * noisesourceL;
			residue = residue % 170003; residue *= residue;
			residue = residue % 17011; residue *= residue;
			residue = residue % 1709; residue *= residue;
			residue = residue % 173; residue *= residue;
			residue = residue % 17;
			applyresidue = residue;
			applyresidue *= 0.00000001;
			applyresidue *= 0.00000001;
			inputSampleL += applyresidue;
			if (inputSampleL < 1.2e-38 && -inputSampleL < 1.2e-38) {
				inputSampleL -= applyresidue;
			}

			noisesourceR = noisesourceR % 1700021; noisesourceR++;
			residue = noisesourceR * noisesourceR;
			residue = residue % 170003; residue *= residue;
			residue = residue % 17011; residue *= residue;
			residue = residue % 1709; residue *= residue;
			residue = residue % 173; residue *= residue;
			residue = residue % 17;
			applyresidue = residue;
			applyresidue *= 0.00000001;
			applyresidue *= 0.00000001;
			inputSampleR += applyresidue;
			if (inputSampleR < 1.2e-38 && -inputSampleR < 1.2e-38) {
				inputSampleR -= applyresidue;
			}

			//for live air, we always apply the dither noise. Then, if our result is 
			//effectively digital black, we'll subtract it aVariMu. We want a 'air' hiss
			// drySample is after noiseshaping
			Vst::Sample64 drySampleL = inputSampleL;
			Vst::Sample64 drySampleR = inputSampleR;

			if (fabs(inputSampleL) > fabs(previousL))
				squaredSampleL = previousL * previousL;
			else
				squaredSampleL = inputSampleL * inputSampleL;

			previousL = inputSampleL;
			inputSampleL *= muMakeupGain;

			if (fabs(inputSampleR) > fabs(previousR))
				squaredSampleR = previousR * previousR;
			else
				squaredSampleR = inputSampleR * inputSampleR;

			previousR = inputSampleR;
			inputSampleR *= muMakeupGain;


			//adjust coefficients for L
			if (flip)
			{
				if (fabs(squaredSampleL) > threshold)
				{
					muVaryL = threshold / fabs(squaredSampleL);
					muAttackL = sqrt(fabs(muSpeedAL));
					muCoefficientAL = muCoefficientAL * (muAttackL - 1.0);

					if (muVaryL < threshold)
						muCoefficientAL = muCoefficientAL + threshold;
					else
						muCoefficientAL = muCoefficientAL + muVaryL;

					muCoefficientAL = muCoefficientAL / muAttackL;
				}
				else
				{
					muCoefficientAL = muCoefficientAL * ((muSpeedAL * muSpeedAL) - 1.0);
					muCoefficientAL = muCoefficientAL + 1.0;
					muCoefficientAL = muCoefficientAL / (muSpeedAL * muSpeedAL);
				}
				muNewSpeedL = muSpeedAL * (muSpeedAL - 1);
				muNewSpeedL = muNewSpeedL + fabs(squaredSampleL * release) + fastest;
				muSpeedAL = muNewSpeedL / muSpeedAL;
			}
			else // if not flip
			{
				if (fabs(squaredSampleL) > threshold)
				{
					muVaryL = threshold / fabs(squaredSampleL);
					muAttackL = sqrt(fabs(muSpeedBL));
					muCoefficientBL = muCoefficientBL * (muAttackL - 1);

					if (muVaryL < threshold)
						muCoefficientBL = muCoefficientBL + threshold;
					else
						muCoefficientBL = muCoefficientBL + muVaryL;

					muCoefficientBL = muCoefficientBL / muAttackL;
				}
				else
				{
					muCoefficientBL = muCoefficientBL * ((muSpeedBL * muSpeedBL) - 1.0);
					muCoefficientBL = muCoefficientBL + 1.0;
					muCoefficientBL = muCoefficientBL / (muSpeedBL * muSpeedBL);
				}
				muNewSpeedL = muSpeedBL * (muSpeedBL - 1);
				muNewSpeedL = muNewSpeedL + fabs(squaredSampleL * release) + fastest;
				muSpeedBL = muNewSpeedL / muSpeedBL;
			}
			//got coefficients, adjusted speeds for L

			//adjust coefficients for R
			if (flip)
			{
				if (fabs(squaredSampleR) > threshold)
				{
					muVaryR = threshold / fabs(squaredSampleR);
					muAttackR = sqrt(fabs(muSpeedAR));
					muCoefficientAR = muCoefficientAR * (muAttackR - 1.0);

					if (muVaryR < threshold)
						muCoefficientAR = muCoefficientAR + threshold;
					else
						muCoefficientAR = muCoefficientAR + muVaryR;

					muCoefficientAR = muCoefficientAR / muAttackR;
				}
				else
				{
					muCoefficientAR = muCoefficientAR * ((muSpeedAR * muSpeedAR) - 1.0);
					muCoefficientAR = muCoefficientAR + 1.0;
					muCoefficientAR = muCoefficientAR / (muSpeedAR * muSpeedAR);
				}
				muNewSpeedR = muSpeedAR * (muSpeedAR - 1);
				muNewSpeedR = muNewSpeedR + fabs(squaredSampleR * release) + fastest;
				muSpeedAR = muNewSpeedR / muSpeedAR;
			}
			else // if not flip
			{
				if (fabs(squaredSampleR) > threshold)
				{
					muVaryR = threshold / fabs(squaredSampleR);
					muAttackR = sqrt(fabs(muSpeedBR));
					muCoefficientBR = muCoefficientBR * (muAttackR - 1);

					if (muVaryR < threshold)
						muCoefficientBR = muCoefficientBR + threshold;
					else
						muCoefficientBR = muCoefficientBR + muVaryR;

					muCoefficientBR = muCoefficientBR / muAttackR;
				}
				else
				{
					muCoefficientBR = muCoefficientBR * ((muSpeedBR * muSpeedBR) - 1.0);
					muCoefficientBR = muCoefficientBR + 1.0;
					muCoefficientBR = muCoefficientBR / (muSpeedBR * muSpeedBR);
				}
				muNewSpeedR = muSpeedBR * (muSpeedBR - 1);
				muNewSpeedR = muNewSpeedR + fabs(squaredSampleR * release) + fastest;
				muSpeedBR = muNewSpeedR / muSpeedBR;
			}
			//got coefficients, adjusted speeds for R



			if (flip)
			{
				coefficient = (muCoefficientAL + pow(muCoefficientAL, 2)) / 2.0;
				inputSampleL *= coefficient;
				coefficient = (muCoefficientAR + pow(muCoefficientAR, 2)) / 2.0;
				inputSampleR *= coefficient;
			}
			else
			{
				coefficient = (muCoefficientBL + pow(muCoefficientBL, 2)) / 2.0;
				inputSampleL *= coefficient;
				coefficient = (muCoefficientBR + pow(muCoefficientBR, 2)) / 2.0;
				inputSampleR *= coefficient;
			}
			//applied compression with vari-vari-µ-µ-µ-µ-µ-µ-is-the-kitten-song o/~
			//applied gain correction to control output level- tends to constrain sound rather than inflate it
			flip = !flip;


			/*/ VuPPM /*/
			// if ((inputSampleL / (drySampleL * muMakeupGain)) < tmpGR) { tmpGR = (inputSampleL / (drySampleL * muMakeupGain)); }
			// if ((inputSampleR / (drySampleR * muMakeupGain)) < tmpGR) { tmpGR = (inputSampleR / (drySampleR * muMakeupGain)); }
			if ((inputSampleL / (drySampleL)) < tmpGR) { tmpGR = (inputSampleL / (drySampleL)); }
			if ((inputSampleR / (drySampleR)) < tmpGR) { tmpGR = (inputSampleR / (drySampleR)); }



			if (output < 1.0) {
				inputSampleL *= output;
				inputSampleR *= output;
			}
			if (wet < 1.0) {
				inputSampleL = (drySampleL * (1.0 - wet)) + (inputSampleL * wet);
				inputSampleR = (drySampleR * (1.0 - wet)) + (inputSampleR * wet);
			}
			//nice little output stage template: if we have another scale of floating point
			//number, we really don't want to meaninglessly multiply that by 1.0.


			/*/ VuPPM /*/
			inputSampleL *= Out_db;
			inputSampleR *= Out_db;
			if (inputSampleL > tmpOutL) { tmpOutL = inputSampleL; }
			if (inputSampleR > tmpOutR) { tmpOutR = inputSampleR; }




			if (precision == 0) {
				//begin 32 bit stereo floating point dither
				int expon;
				frexpf((float)inputSampleL, &expon);
				fpdL ^= fpdL << 13; fpdL ^= fpdL >> 17; fpdL ^= fpdL << 5;
				inputSampleL += ((double(fpdL) - uint32_t(0x7fffffff)) * 5.5e-36l * pow(2, expon + 62));
				frexpf((float)inputSampleR, &expon);
				fpdR ^= fpdR << 13; fpdR ^= fpdR >> 17; fpdR ^= fpdR << 5;
				inputSampleR += ((double(fpdR) - uint32_t(0x7fffffff)) * 5.5e-36l * pow(2, expon + 62));
				//end 32 bit stereo floating point dither
			}
			else {
				//begin 64 bit stereo floating point dither
				//int expon; 
				//frexp((double)inputSampleL, &expon);
				fpdL ^= fpdL << 13; fpdL ^= fpdL >> 17; fpdL ^= fpdL << 5;
				//inputSampleL += ((double(fpdL)-uint32_t(0x7fffffff)) * 1.1e-44l * pow(2,expon+62));
				//frexp((double)inputSampleR, &expon);
				fpdR ^= fpdR << 13; fpdR ^= fpdR >> 17; fpdR ^= fpdR << 5;
				//inputSampleR += ((double(fpdR)-uint32_t(0x7fffffff)) * 1.1e-44l * pow(2,expon+62));
				//end 64 bit stereo floating point dither
			}


			*out1 = inputSampleL;
			*out2 = inputSampleR;

			in1++;
			in2++;
			out1++;
			out2++;
		}

		/*/ VuPPM /*/
		fInVuPPML = VuPPMconvert(tmpInL);
		fInVuPPMR = VuPPMconvert(tmpInR);
		fOutVuPPML = VuPPMconvert(tmpOutL);
		fOutVuPPMR = VuPPMconvert(tmpOutR);
		// fOutGRVuPPM = VuPPMconvert(tmpGR);
		double dB = 20.0 * log10(tmpGR);
		if		(dB > 0.0)		fOutGRVuPPM = 1.0;
		else if (dB > -12.0)	fOutGRVuPPM = (dB + 12) / 12;
		else					fOutGRVuPPM = 0.0;

		return;
	}


	template <typename SampleType>
	void VariMuProcessor::processVuPPM(SampleType** inputs, int32 sampleFrames)
	{
		SampleType* in1 = (SampleType*)inputs[0];
		SampleType* in2 = (SampleType*)inputs[1];

		Vst::Sample64 tmpInL = 0.0; /*/ VuPPM /*/
		Vst::Sample64 tmpInR = 0.0; /*/ VuPPM /*/

		int32 samples = sampleFrames;

		while (--samples >= 0)
		{
			Vst::Sample64 inputSampleL = *in1;
			Vst::Sample64 inputSampleR = *in2;
			if (inputSampleL > tmpInL) { tmpInL = inputSampleL; }
			if (inputSampleR > tmpInR) { tmpInR = inputSampleR; }
			in1++;
			in2++;
		}

		/*/ VuPPM /*/
		fInVuPPML = VuPPMconvert(tmpInL);
		fInVuPPMR = VuPPMconvert(tmpInR);
		fOutVuPPML = fInVuPPML;
		fOutVuPPMR = fInVuPPMR;

		return;
	}

	Vst::Sample64 VariMuProcessor::norm_to_gain(Vst::Sample64 plainValue) {
		return exp(log(10.0) * (24.0 * plainValue - 12.0) / 20.0);
	}

	Vst::Sample64 VariMuProcessor::VuPPMconvert(Vst::Sample64 plainValue)
	{
		double dB = 20.0 * log10(plainValue);
		double normValue;

		/*
		   db     : normValue
		0   ~ -36 : 1.0 ~ 0.0
		0   ~ -18 : 1.0 ~ 0.5   <-
		-18 ~ -60 : 0.5 ~ 0.0   <-
		+24 ~ -60 : 1.0 ~ 0.0

		1. param(gain) -> dB
		2.	if (dB > 0.0) y = 1.0;
			if (dB > -18) y = (x + 36) / 36;
			if (dB > -60) y = (x + 60) / 84;
			if (-60 > dB) y = 0.0;

		*/

		if (dB > 0.0) normValue = 1.0;
		else if (dB > -18.0) normValue = (dB + 36) / 36;
		else if (dB > -60.0) normValue = (dB + 60) / 84;
		else normValue = 0.0;

		return normValue;
	}



	//------------------------------------------------------------------------
} // namespace yg331
