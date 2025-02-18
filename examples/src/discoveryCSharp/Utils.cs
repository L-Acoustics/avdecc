static class Utils
{
	public static la.avdecc.protocol.ProtocolInterface.Type? ChooseProtocolInterfaceType(SupportedProtocolInterfaceTypes allowedTypes)
	{
		var protocolInterfaceType = la.avdecc.protocol.ProtocolInterface.Type.None;

		// Get the list of supported protocol interface types, and ask the user to choose one (if many available)
		var protocolInterfaceTypes = la.avdecc.protocol.ProtocolInterface.getSupportedProtocolInterfaceTypes().andEqual(allowedTypes);
		if (protocolInterfaceTypes.empty())
		{
			Console.WriteLine("No protocol interface supported on this computer");
			return null;
		}

		if (protocolInterfaceTypes.count() == 1)
		{
			return protocolInterfaceTypes.at(0);
		}
		else
		{
			Console.WriteLine("Choose a protocol interface type:");

			// SupportedProtocolInterfaceTypes is currently not IEnumerable, so let's create an array that will be
			la.avdecc.protocol.ProtocolInterface.Type[] types = new la.avdecc.protocol.ProtocolInterface.Type[protocolInterfaceTypes.count()];
			uint i = 0;
			if (protocolInterfaceTypes.test(la.avdecc.protocol.ProtocolInterface.Type.PCap))
			{
				types[i++] = la.avdecc.protocol.ProtocolInterface.Type.PCap;
			}
			if (protocolInterfaceTypes.test(la.avdecc.protocol.ProtocolInterface.Type.MacOSNative))
			{
				types[i++] = la.avdecc.protocol.ProtocolInterface.Type.MacOSNative;
			}
			if (protocolInterfaceTypes.test(la.avdecc.protocol.ProtocolInterface.Type.Virtual))
			{
				types[i++] = la.avdecc.protocol.ProtocolInterface.Type.Virtual;
			}

			int protocolInterfaceCount = 1;
			foreach (var type in /* protocolInterfaceTypes */types)
			{
				Console.WriteLine($"{protocolInterfaceCount}: {type}");
				protocolInterfaceCount++;
			}

			// Get the user's selection
			int index = -1;
			while (index == -1)
			{
				Console.Write("> ");
				var selection = Console.ReadLine();

				if (ulong.TryParse(selection, out ulong selectedIndex) && selectedIndex >= 1 && selectedIndex <= protocolInterfaceTypes.count())
				{
					index = (int)selectedIndex - 1;
				}
				else
				{
					Console.WriteLine("\nInvalid selection.");
				}
			}
			return /* protocolInterfaceTypes.at((ulong)index) */types[(ulong)index];
		}
	}

	public static la.networkInterface.Interface? ChooseNetworkInterface()
	{
		var interfaceEnumerator = new InterfaceEnumerator();
		var interfaces = interfaceEnumerator.Interfaces;

		if (interfaces.Count == 0)
		{
			Console.WriteLine("No valid network interface found on this computer");
			return null;
		}

		Console.WriteLine("Choose an interface:");
		int intfcCount = 1;
		foreach (var ifc in interfaces)
		{
			var type = "";
			switch (ifc.type)
			{
				case la.networkInterface.Interface.Type.WiFi:
					type = "WiFi";
					break;
				case la.networkInterface.Interface.Type.Ethernet:
					type = "Ethernet";
					break;
				case la.networkInterface.Interface.Type.Loopback:
					type = "Loopback";
					break;
				default:
					type = "Unknown";
					break;
			}
			Console.WriteLine($"{intfcCount}: {ifc.alias} ({ifc.description}, {type})");
			intfcCount++;
		}

		// Get the user's selection
		int index = -1;
		while (index == -1)
		{
			Console.Write("> ");
			var selection = Console.ReadLine();

			if (int.TryParse(selection, out int selectedIndex) && selectedIndex >= 1 && selectedIndex <= interfaces.Count)
			{
				index = selectedIndex - 1;
			}
			else
			{
				Console.WriteLine("\nInvalid selection.");
			}
		}

		return interfaces[index];
	}

	private class InterfaceEnumerator : la.networkInterface.NetworkInterfaceHelper.Observer
	{
		public InterfaceEnumerator()
		{
			var networkInterfaceHelper = la.networkInterface.NetworkInterfaceHelper.getInstance();
			networkInterfaceHelper.registerObserver(this);
			// Unregister immediately we are guaranteed to get the interfaces already available in the system on register call
			// and we do not require to monitor the interfaces for changes in this sample
			networkInterfaceHelper.unregisterObserver(this);
		}
		public override void onInterfaceAdded(la.networkInterface.Interface intfc)
		{
			// Only select connected, non virtual interfaces
			if (intfc.isConnected && !intfc.isVirtual && !intfc.ipAddressInfos.IsEmpty)
			{
				_interfaces.Add(new la.networkInterface.Interface(intfc));
			}
		}

		public override void onInterfaceRemoved(la.networkInterface.Interface intfc) { }

		public override void onInterfaceEnabledStateChanged(la.networkInterface.Interface intfc, bool isEnabled) { }

		public override void onInterfaceConnectedStateChanged(la.networkInterface.Interface intfc, bool isConnected) { }

		public override void onInterfaceAliasChanged(la.networkInterface.Interface intfc, string alias) { }

		public override void onInterfaceIPAddressInfosChanged(la.networkInterface.Interface intfc, IPAddressInfos ipAddressInfos) { }

		public override void onInterfaceGateWaysChanged(la.networkInterface.Interface intfc, Gateways gateways) { }

		public List<la.networkInterface.Interface> Interfaces { get => _interfaces; }
		private readonly List<la.networkInterface.Interface> _interfaces = [];
	}
}
