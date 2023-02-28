#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/base/ustring.h"
#include "base/source/fobject.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstunits.h"



using namespace Steinberg;

namespace yg331 {
	//------------------------------------------------------------------------
	// SliderParameter Declaration
	// example of custom parameter (overwriting to and fromString)
	//------------------------------------------------------------------------

	class VuPPMParameter : public Vst::RangeParameter
	{
	public:
		VuPPMParameter(const Vst::TChar* title,
			Vst::ParamID tag,
			const Vst::TChar* units,
			Vst::ParamValue minPlain,
			Vst::ParamValue maxPlain,
			Vst::ParamValue defaultValuePlain,
			int32 flags,
			Vst::UnitID unitID
		);

		Vst::ParamValue toPlain(Vst::ParamValue normValue) const SMTG_OVERRIDE;						/** Converts a normalized value to plain value (e.g. 0.5 to 10000.0Hz). */
		Vst::ParamValue toNormalized(Vst::ParamValue plainValue) const SMTG_OVERRIDE;				/** Converts a plain value to a normalized value (e.g. 10000 to 0.5). */

		void toString(Vst::ParamValue normValue, Vst::String128 string) const SMTG_OVERRIDE;
		bool fromString(const Vst::TChar* string, Vst::ParamValue& normValue) const SMTG_OVERRIDE;
	};

	VuPPMParameter::VuPPMParameter(const Vst::TChar* title, Vst::ParamID tag, const Vst::TChar* units, Vst::ParamValue minPlain,
		Vst::ParamValue maxPlain, Vst::ParamValue defaultValuePlain, int32 flags, Vst::UnitID unitID)
	{

		UString(info.title, str16BufferSize(Vst::String128)).assign(title);
		if (units)
			UString(info.units, str16BufferSize(Vst::String128)).assign(units);

		setMin(minPlain);
		setMax(maxPlain);

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
		normValue	: dB
		1.0 ~ 0.0	: 0 ~ -36
		1.0 ~ 0.5	: 0 ~ -18
		0.5 ~ 0.0	: -18 ~ -60
		1.0 ~ 0.0	: +24 ~ -60

		1. normValue -> dB
		1.0 ~ 0.5	: y = (36 * y) - 36;
		0.5 ~ 0.0	: y = (84 * x) - 60;
		*/

		if (normValue > 0.5) plainValue = (36.0 * normValue) - 36.0;
		else plainValue = (84.0 * normValue) - 60.0;
		
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


		Vst::ParamValue normValue = (plainValue - getMin()) / (getMax() - getMin());
		if (normValue > 1.0) normValue = 1.0;
		else if (normValue < 0.0) normValue = 0.0;
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
}
