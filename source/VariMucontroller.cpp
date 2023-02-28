//------------------------------------------------------------------------
// Copyright(c) 2023 yg331.
//------------------------------------------------------------------------

#include "VariMucontroller.h"
#include "VariMucids.h"
#include "vstgui/plugin-bindings/vst3editor.h"

#include "pluginterfaces/base/ustring.h"

#include "base/source/fstreamer.h"

#include "VariMuparam.h"

#include <cstdio>
#include <cmath>


using namespace Steinberg;

namespace yg331 {
	//------------------------------------------------------------------------
	// SliderParameter Declaration
	// example of custom parameter (overwriting to and fromString)
	//------------------------------------------------------------------------
	class SliderParameter : public Vst::RangeParameter
	{
	public:
		SliderParameter(UString256 title, Vst::ParamValue minPlain, Vst::ParamValue maxPlain, Vst::ParamValue defaultValuePlain, int32 flags, int32 id);

		Vst::ParamValue toPlain(Vst::ParamValue normValue) const SMTG_OVERRIDE;						/** Converts a normalized value to plain value (e.g. 0.5 to 10000.0Hz). */
		Vst::ParamValue toNormalized(Vst::ParamValue plainValue) const SMTG_OVERRIDE;				/** Converts a plain value to a normalized value (e.g. 10000 to 0.5). */

		void toString(Vst::ParamValue normValue, Vst::String128 string) const SMTG_OVERRIDE;
		bool fromString(const Vst::TChar* string, Vst::ParamValue& normValue) const SMTG_OVERRIDE;
	};

	//------------------------------------------------------------------------
	// GainParameter Implementation
	//------------------------------------------------------------------------
	SliderParameter::SliderParameter(UString256 title, Vst::ParamValue minPlain, Vst::ParamValue maxPlain, Vst::ParamValue defaultValuePlain, int32 flags, int32 id)
	{
		Steinberg::UString(info.title, USTRINGSIZE(info.title)).assign(title);
		Steinberg::UString(info.units, USTRINGSIZE(info.units)).assign(USTRING("dB"));

		setMin(minPlain);
		setMax(maxPlain);

		info.flags = flags;
		info.id = id;
		info.stepCount = 0;
		info.defaultNormalizedValue = valueNormalized = toNormalized(defaultValuePlain);
		info.unitId = Vst::kRootUnitId;
	}
	//         mM, string
	// param,  db, gain
	//   0.5,   0,  1.0
	//     0, -12,  0.25
	//     1, +12,  3.98

	Vst::ParamValue SliderParameter::toPlain(Vst::ParamValue normValue) const
	{
		Vst::ParamValue plainValue = ((getMax() - getMin()) * normValue) + getMin();
		if (plainValue > getMax()) plainValue = getMax();
		else if (plainValue < getMin()) plainValue = getMin();
		return plainValue;
	}

	Vst::ParamValue SliderParameter::toNormalized(Vst::ParamValue plainValue) const
	{
		Vst::ParamValue normValue = (plainValue - getMin()) / (getMax() - getMin());
		if (normValue > 1.0) normValue = 1.0;
		else if (normValue < 0.0) normValue = 0.0;
		return normValue;
	}

	//------------------------------------------------------------------------
	void SliderParameter::toString(Vst::ParamValue normValue, Vst::String128 string) const
	{
		Vst::ParamValue plainValue = SliderParameter::toPlain(normValue);

		char text[32];
		snprintf(text, 32, "%.2f", plainValue);

		Steinberg::UString(string, 128).fromAscii(text);
	}

	//------------------------------------------------------------------------
	bool SliderParameter::fromString(const Vst::TChar* string, Vst::ParamValue& normValue) const
	{
		String wrapper((Vst::TChar*)string); // don't know buffer size here!
		Vst::ParamValue plainValue;
		if (wrapper.scanFloat(plainValue))
		{
			normValue = SliderParameter::toNormalized(plainValue);
			return true;
		}
		return false;
	}



	class VuPPMParameter : public Vst::RangeParameter
	{
	public:
		VuPPMParameter(const Vst::TChar* title,
			int32 flags,
			Vst::ParamID tag,
			Vst::ParamValue minPlain = -60, 
			Vst::ParamValue maxPlain = 0, 
			Vst::ParamValue defaultValuePlain = -18,
			Vst::UnitID unitID = Vst::kRootUnitId
		);

		Vst::ParamValue toPlain(Vst::ParamValue normValue) const SMTG_OVERRIDE;						/** Converts a normalized value to plain value (e.g. 0.5 to 10000.0Hz). */
		Vst::ParamValue toNormalized(Vst::ParamValue plainValue) const SMTG_OVERRIDE;				/** Converts a plain value to a normalized value (e.g. 10000 to 0.5). */

		void toString(Vst::ParamValue normValue, Vst::String128 string) const SMTG_OVERRIDE;
		bool fromString(const Vst::TChar* string, Vst::ParamValue& normValue) const SMTG_OVERRIDE;
	private:
		Vst::ParamValue midValue;
	};

	VuPPMParameter::VuPPMParameter(const Vst::TChar* title,
		int32 flags,
		Vst::ParamID tag,
		Vst::ParamValue minPlain,
		Vst::ParamValue maxPlain,
		Vst::ParamValue defaultValuePlain,
		Vst::UnitID unitID
	)
	{

		UString(info.title, str16BufferSize(Vst::String128)).assign(title);
		Steinberg::UString(info.units, USTRINGSIZE(info.units)).assign(USTRING("dB"));

		setMin(minPlain);
		setMax(maxPlain);

		midValue = defaultValuePlain;

		info.stepCount = 0;
		info.defaultNormalizedValue = valueNormalized = toNormalized(defaultValuePlain);
		info.flags = flags;
		info.id = tag;
		info.unitId = unitID;
	}

	Vst::ParamValue VuPPMParameter::toPlain(Vst::ParamValue normValue) const
	{
		Vst::ParamValue plainValue;


		/*
		normValue	: dB		: gain
		1.0 ~ 0.0	:   0 ~ -36	: 1.0 ~ 0.015849
		1.0 ~ 0.5	:   0 ~ -18 : 1.0 ~ 0.12589254117941673
		0.5 ~ 0.0	: -18 ~ -60 : 0.12589254117941673 ~ 
		1.0 ~ 0.0	: +24 ~ -60

		1. normValue -> dB
		1.0 ~ 0.5	: y = (36 * y) - 36;
		0.5 ~ 0.0	: y = (84 * x) - 60;
		*/
		if (normValue > 0.5) plainValue = (2 * (getMax() - midValue) * normValue) + (2 * midValue - getMax());
		else plainValue = (2 * (midValue - getMin()) * normValue) + getMin();

		return plainValue;
	}

	Vst::ParamValue VuPPMParameter::toNormalized(Vst::ParamValue plainValue) const
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
	void VuPPMParameter::toString(Vst::ParamValue normValue, Vst::String128 string) const
	{
		Vst::ParamValue plainValue = VuPPMParameter::toPlain(normValue);

		char text[32];
		snprintf(text, 32, "%.2f", plainValue);

		Steinberg::UString(string, 128).fromAscii(text);
	}

	//------------------------------------------------------------------------
	bool VuPPMParameter::fromString(const Vst::TChar* string, Vst::ParamValue& normValue) const
	{
		String wrapper((Vst::TChar*)string); // don't know buffer size here!
		Vst::ParamValue plainValue;
		if (wrapper.scanFloat(plainValue))
		{
			normValue = VuPPMParameter::toNormalized(plainValue);
			return true;
		}
		return false;
	}


//------------------------------------------------------------------------
// VariMuController Implementation
//------------------------------------------------------------------------
tresult PLUGIN_API VariMuController::initialize (FUnknown* context)
{
	// Here the Plug-in will be instantiated

	//---do not forget to call parent ------
	tresult result = EditControllerEx1::initialize (context);
	if (result != kResultOk)
	{
		return result;
	}

	// Here you could register some parameters

	int32 stepCount;
	int32 flags;
	int32 tag;
	Vst::ParamValue defaultVal;
	Vst::ParamValue minPlain;
	Vst::ParamValue maxPlain;
	Vst::ParamValue defaultPlain;

	tag = kParamInput;
	flags = Vst::ParameterInfo::kCanAutomate;
	minPlain = -12.0;
	maxPlain = 12.0;
	defaultPlain = 0.0;
	auto* gainParamIn = new SliderParameter(USTRING("Input"), minPlain, maxPlain, defaultPlain, flags, tag);
	parameters.addParameter(gainParamIn);

	tag = kParamOutput;
	flags = Vst::ParameterInfo::kCanAutomate;
	minPlain = -12.0;
	maxPlain = 12.0;
	defaultPlain = 0.0;
	auto* gainParamOut = new SliderParameter(USTRING("Output"), minPlain, maxPlain, defaultPlain, flags, tag);
	parameters.addParameter(gainParamOut);

	/*
	tag = kParamInVuPPML;
	flags = Vst::ParameterInfo::kIsReadOnly;
	minPlain = -44.0;
	maxPlain = 6.0;
	defaultPlain = -44.0;
	auto* InVuPPML = new SliderParameter(USTRING("InVuPPML"), minPlain, maxPlain, defaultPlain, flags, tag);
	parameters.addParameter(InVuPPML);
	tag = kParamInVuPPMR;
	flags = Vst::ParameterInfo::kIsReadOnly;
	minPlain = -44.0;
	maxPlain = 6.0;
	defaultPlain = -44.0;
	auto* InVuPPMR = new SliderParameter(USTRING("InVuPPMR"), minPlain, maxPlain, defaultPlain, flags, tag);
	parameters.addParameter(InVuPPMR);
	tag = kParamOutVuPPML;
	flags = Vst::ParameterInfo::kIsReadOnly;
	minPlain = -44.0;
	maxPlain = 6.0;
	defaultPlain = -44.0;
	auto* OutVuPPML = new SliderParameter(USTRING("OutVuPPML"), minPlain, maxPlain, defaultPlain, flags, tag);
	parameters.addParameter(OutVuPPML);
	tag = kParamOutVuPPMR;
	flags = Vst::ParameterInfo::kIsReadOnly;
	minPlain = -44.0;
	maxPlain = 6.0;
	defaultPlain = -44.0;
	auto* OutVuPPMR = new SliderParameter(USTRING("OutVuPPMR"), minPlain, maxPlain, defaultPlain, flags, tag);
	parameters.addParameter(OutVuPPMR);
	*/

	// Here you could register some parameters

	tag = kParamInVuPPML;
	flags = Vst::ParameterInfo::kIsReadOnly;
	minPlain = -60;
	maxPlain = 0;
	defaultPlain = -18;
	auto* InVuPPML = new VuPPMParameter(USTRING("InVuPPML"), flags, tag, minPlain, maxPlain, defaultPlain);
	parameters.addParameter(InVuPPML);
	tag = kParamInVuPPMR;
	flags = Vst::ParameterInfo::kIsReadOnly;
	auto* InVuPPMR = new VuPPMParameter(USTRING("InVuPPMR"), flags, tag, minPlain, maxPlain, defaultPlain);
	parameters.addParameter(InVuPPMR);
	tag = kParamOutVuPPML;
	flags = Vst::ParameterInfo::kIsReadOnly;
	auto* OutVuPPML = new VuPPMParameter(USTRING("OutVuPPML"), flags, tag, minPlain, maxPlain, defaultPlain);
	parameters.addParameter(OutVuPPML);
	tag = kParamOutVuPPMR;
	flags = Vst::ParameterInfo::kIsReadOnly;
	auto* OutVuPPMR = new VuPPMParameter(STR16("OutVuPPMR"), flags, tag, minPlain, maxPlain, defaultPlain);
	parameters.addParameter(OutVuPPMR);

	tag = kParamGRVuPPM;
	flags = Vst::ParameterInfo::kIsReadOnly;
	minPlain = -12;
	maxPlain = 0;
	defaultPlain = -6;
	auto* GRVuPPM = new VuPPMParameter(STR16("GRVuPPM"), flags, tag, minPlain, maxPlain, defaultPlain);
	parameters.addParameter(GRVuPPM);
	

	tag = kParamIntensity;
	stepCount = 0;
	defaultVal = 1.0;
	flags = Vst::ParameterInfo::kCanAutomate;
	parameters.addParameter(STR16("Intensity"), STR16("%"), stepCount, defaultVal, flags, tag);

	tag = kParamSpeed;
	stepCount = 0;
	defaultVal = 0.5;
	flags = Vst::ParameterInfo::kCanAutomate;
	parameters.addParameter(STR16("Speed"), nullptr, stepCount, defaultVal, flags, tag);

	tag = kParamTrim;
	stepCount = 0;
	defaultVal = 0.5;
	flags = Vst::ParameterInfo::kCanAutomate;
	parameters.addParameter(STR16("Gain"), nullptr, stepCount, defaultVal, flags, tag);

	tag = kParamMix;
	stepCount = 0;
	defaultVal = 0.5;
	flags = Vst::ParameterInfo::kCanAutomate;
	parameters.addParameter(STR16("Mix"), nullptr, stepCount, defaultVal, flags, tag);

	tag = kParamBypass;
	stepCount = 1;
	defaultVal = 0;
	flags = Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass;
	parameters.addParameter(STR16("Bypass"), nullptr, stepCount, defaultVal, flags, tag);




	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VariMuController::terminate ()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!

	//---do not forget to call parent ------
	return EditControllerEx1::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API VariMuController::setComponentState (IBStream* state)
{
	// Here you get the state of the component (Processor part)
	if (!state)
		return kResultFalse;

	IBStreamer streamer(state, kLittleEndian);

	float savedInput = 0.f;
	if (streamer.readFloat(savedInput) == false)
		return kResultFalse;
	setParamNormalized(kParamInput, savedInput);

	float savedOutput = 0.f;
	if (streamer.readFloat(savedOutput) == false)
		return kResultFalse;
	setParamNormalized(kParamOutput, savedOutput);

	float savedIntensity = 0.f;
	if (streamer.readFloat(savedIntensity) == false)
		return kResultFalse;
	setParamNormalized(kParamIntensity, savedIntensity);

	float savedSpeed = 0.f;
	if (streamer.readFloat(savedSpeed) == false)
		return kResultFalse;
	setParamNormalized(kParamSpeed, savedSpeed);

	float savedTrim = 0.f;
	if (streamer.readFloat(savedTrim) == false)
		return kResultFalse;
	setParamNormalized(kParamTrim, savedTrim);

	float savedMix = 0.f;
	if (streamer.readFloat(savedMix) == false)
		return kResultFalse;
	setParamNormalized(kParamMix, savedMix);

	int32 bypassState = 0;
	if (streamer.readInt32(bypassState) == false)
		return kResultFalse;
	setParamNormalized(kParamBypass, bypassState ? 1 : 0);


	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VariMuController::setState (IBStream* state)
{
	// Here you get the state of the controller

	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VariMuController::getState (IBStream* state)
{
	// Here you are asked to deliver the state of the controller (if needed)
	// Note: the real state of your plug-in is saved in the processor

	return kResultTrue;
}

//------------------------------------------------------------------------
IPlugView* PLUGIN_API VariMuController::createView (FIDString name)
{
	// Here the Host wants to open your editor (if you have one)
	if (FIDStringsEqual (name, Vst::ViewType::kEditor))
	{
		// create your editor here and return a IPlugView ptr of it
		auto* view = new VSTGUI::VST3Editor (this, "view", "VariMueditor.uidesc");
		return view;
	}
	return nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VariMuController::setParamNormalized (Vst::ParamID tag, Vst::ParamValue value)
{
	// called by host to update your parameters
	tresult result = EditControllerEx1::setParamNormalized (tag, value);
	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VariMuController::getParamStringByValue (Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string)
{
	// called by host to get a string for given normalized value of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamStringByValue (tag, valueNormalized, string);
}

//------------------------------------------------------------------------
tresult PLUGIN_API VariMuController::getParamValueByString (Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized)
{
	// called by host to get a normalized value from a string representation of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamValueByString (tag, string, valueNormalized);
}
 
//------------------------------------------------------------------------
} // namespace yg331
