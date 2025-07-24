class StreamFormatInfoApp
{
	static string TypeToString(la.avdecc.entity.model.StreamFormatInfo.Type type)
	{
		switch (type)
		{
			case la.avdecc.entity.model.StreamFormatInfo.Type.None:
				return "None";
			case la.avdecc.entity.model.StreamFormatInfo.Type.IEC_61883_6:
				return "IEC";
			case la.avdecc.entity.model.StreamFormatInfo.Type.AAF:
				return "AAF";
			case la.avdecc.entity.model.StreamFormatInfo.Type.ClockReference:
				return "CRF";
			case la.avdecc.entity.model.StreamFormatInfo.Type.Unsupported:
				return "Unsupported";
			default:
				return "Unhandled";
		}
	}

	static string SamplingRateToString(la.avdecc.entity.model.SamplingRate rate)
	{
		uint freq = (uint)rate.getNominalSampleRate();
		if (freq != 0)
		{
			if (freq < 1000)
			{
				return freq.ToString() + " Hz";
			}
			else
			{
				// Round to nearest integer but keep one decimal part
				var freqRounded = freq / 1000;
				var freqDecimal = (freq % 1000) / 100;
				if (freqDecimal == 0)
				{
					return freqRounded.ToString() + " kHz";
				}
				else
				{
					return freqRounded.ToString() + "." + freqDecimal.ToString() + " kHz";
				}
			}
		}
		return "Unknown";
	}

	static string SampleFormatToString(la.avdecc.entity.model.StreamFormatInfo.SampleFormat format)
	{
		switch (format)
		{
			case la.avdecc.entity.model.StreamFormatInfo.SampleFormat.Int8:
				return "INT8";
			case la.avdecc.entity.model.StreamFormatInfo.SampleFormat.Int16:
				return "INT16";
			case la.avdecc.entity.model.StreamFormatInfo.SampleFormat.Int24:
				return "INT24";
			case la.avdecc.entity.model.StreamFormatInfo.SampleFormat.Int32:
				return "INT32";
			case la.avdecc.entity.model.StreamFormatInfo.SampleFormat.Int64:
				return "INT64";
			case la.avdecc.entity.model.StreamFormatInfo.SampleFormat.FixedPoint32:
				return "FIXED32";
			case la.avdecc.entity.model.StreamFormatInfo.SampleFormat.FloatingPoint32:
				return "FLOAT32";
			case la.avdecc.entity.model.StreamFormatInfo.SampleFormat.Unknown:
				return "Unknown";
			default:
				return "Unhandled";
		}
	}

	static int DoJob(ulong value)
	{
		var sf = new la.avdecc.entity.model.StreamFormat(value);
		var sfi = la.avdecc.entity.model.StreamFormatInfo.create(sf);

		Console.WriteLine($"StreamFormat {value:X} information:");
		Console.WriteLine($" - Type: {TypeToString(sfi.getType())}");
		if (sfi.isUpToChannelsCount())
		{
			Console.WriteLine($" - Max Channels: {sfi.getChannelsCount()}");
		}
		else
		{
			Console.WriteLine($" - Channels: {sfi.getChannelsCount()}");
		}
		Console.WriteLine($" - Sampling Rate: {SamplingRateToString(sfi.getSamplingRate())}");
		Console.WriteLine($" - Sample Format: {SampleFormatToString(sfi.getSampleFormat())}");
		Console.WriteLine($" - Sample Size: {sfi.getSampleSize()}");
		Console.WriteLine($" - Sample Depth: {sfi.getSampleBitDepth()}");
		Console.WriteLine($" - Synchronous Clock: {(sfi.useSynchronousClock() ? "True" : "False")}");

		return 0;
	}

	static void Main(string[] args)
	{
		// Check avdecc library interface version (only required when using the shared version of the library, but the code is here as an example)
		if (!avdecc.isCompatibleWithInterfaceVersion(avdecc.InterfaceVersion))
		{
			Console.WriteLine($"Avdecc shared library interface version invalid:\nCompiled with interface {avdecc.InterfaceVersion} (v{avdecc.getVersion()}), but running interface {avdecc.getInterfaceVersion()}");
			return;
		}

		if (args.Length != 1)
		{
			Console.WriteLine("Usage:\nStreamFormatInfo <stream format value>");
			return;
		}

		try
		{
			ulong value = Convert.ToUInt64(args[0], 16);
			DoJob(value);
		}
		catch (Exception e)
		{
			Console.WriteLine($"Error parsing stream format value: {e.Message}");
			Console.WriteLine("Please provide a valid hexadecimal value (e.g., 0x00604008)");
		}
	}
}
