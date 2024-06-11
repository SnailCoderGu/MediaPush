#pragma once
class AudioEncoder
{
public:
	struct ACodecConfig
	{
		int codec_type;
		int codec_id;
		int sample_rate;
		int channel;
		int format;
	};

public:
	inline virtual ACodecConfig& GetCodecConfig() { return codec_config; }
protected:
	ACodecConfig codec_config;
};

