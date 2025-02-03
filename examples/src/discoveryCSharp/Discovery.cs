// #define LOAD_TEST_VIRTUAL_ENTITY_FROM_AEM
// #define LOAD_TEST_VIRTUAL_ENTITY_FROM_AVE

using la.avdecc.controller;

class DiscoveryApp
{
	static void Main()
	{
		Console.WriteLine("Using AVDECC Library v" + avdecc.getVersion());
		try
		{
			la.networkInterface.Interface? intfc = Utils.ChooseNetworkInterface();

			if (intfc == null)
			{
				Console.WriteLine("No interfaces found, exiting");
				return;
			}

			var allowedTypes = new SupportedProtocolInterfaceTypes();
			allowedTypes.set(la.avdecc.protocol.ProtocolInterface.Type.PCap);
			allowedTypes.set(la.avdecc.protocol.ProtocolInterface.Type.MacOSNative);
			la.avdecc.protocol.ProtocolInterface.Type? type = Utils.ChooseProtocolInterfaceType(allowedTypes);

			if (!type.HasValue)
			{
				Console.WriteLine("No protocol interface found, exiting");
				return;
			}

			var uniqueID = la.avdecc.UniqueIdentifier.getNullUniqueIdentifier();

			Console.WriteLine($"Starting discovery on interface: {intfc} and protocol interface {type}, discovery active:");

			// Create a discovery object
			Discovery? discovery = new(type.Value, intfc.id, 0x99, uniqueID, "en");
			Thread.Sleep(10000);
			Console.WriteLine("Destroying discovery object");
			discovery?.Dispose();
			discovery = null;
		}
		catch (System.Exception e)
		{
			Console.WriteLine($"Unknown exception: {e}");
		}
	}

#if LOAD_TEST_VIRTUAL_ENTITY_FROM_AEM
	class Builder : la.avdecc.controller.model.DefaultedVirtualEntityBuilder
	{
		public Builder() : base() { }

		public override void build(la.avdecc.entity.model.EntityTree entityTree, la.avdecc.entity.Entity.CommonInformation commonInformation, InterfaceInformationMap intfcInformation)
		{
			commonInformation.entityID = new la.avdecc.UniqueIdentifier(0x0102030405060708);
			commonInformation.entityCapabilities = new EntityCapabilities();
			commonInformation.entityCapabilities.set(la.avdecc.entity.EntityCapability.AemSupported);
			commonInformation.listenerStreamSinks = 2;
			commonInformation.listenerCapabilities = new ListenerCapabilities();
			commonInformation.listenerCapabilities.set(la.avdecc.entity.ListenerCapability.Implemented);
			commonInformation.identifyControlIndex = 0;

			var interfaceInfo = new la.avdecc.entity.Entity.InterfaceInformation();
			interfaceInfo.macAddress = new MacAddress(new byte[] { 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 });
			interfaceInfo.validTime = 31;
			intfcInformation[la.avdecc.entity.Entity.GlobalAvbInterfaceIndex] = interfaceInfo;
		}
		public override void build(la.avdecc.controller.ControlledEntity entity, la.avdecc.entity.model.EntityNodeStaticModel staticModel, la.avdecc.entity.model.EntityNodeDynamicModel dynamicModel)
		{
			dynamicModel.entityName = new la.avdecc.entity.model.AvdeccFixedString("SimpleEntity");
		}
		public override void build(CompatibilityFlags flags)
		{
			flags.set(la.avdecc.controller.ControlledEntity.CompatibilityFlag.IEEE17221);
			flags.set(la.avdecc.controller.ControlledEntity.CompatibilityFlag.Milan);
		}
	}
#endif // LOAD_TEST_VIRTUAL_ENTITY_FROM_AEM

	class Discovery : IDisposable
	{
		public Discovery(la.avdecc.protocol.ProtocolInterface.Type type, string interfaceID, ushort progID, la.avdecc.UniqueIdentifier entityModelID, string preferedLocale)
		{
			try
			{
				_controller = la.avdecc.controller.Controller.create(type, interfaceID, progID, entityModelID, preferedLocale, null, null, null);
				_controller.registerObserver(_observer);
				_controller.enableEntityAdvertising(10);
				_controller.enableEntityModelCache();
				_controller.enableFastEnumeration();
#if LOAD_TEST_VIRTUAL_ENTITY_FROM_AEM
				var builder = new Builder();
				var result = _controller.createVirtualEntityFromEntityModelFile("SimpleEntityModel.json", builder, false);
				if (result.get0() != DeserializationError.NoError)
				{
					Console.WriteLine($"Error deserializing entity model: {result.get1()}");
				}
#endif // LOAD_TEST_VIRTUAL_ENTITY_FROM_AEM
#if LOAD_TEST_VIRTUAL_ENTITY_FROM_AVE
				var flags = new Flags();
				flags.set(la.avdecc.entity.model.jsonSerializer.Flag.ProcessADP);
				flags.set(la.avdecc.entity.model.jsonSerializer.Flag.ProcessStaticModel);
				flags.set(la.avdecc.entity.model.jsonSerializer.Flag.ProcessDynamicModel);
				flags.set(la.avdecc.entity.model.jsonSerializer.Flag.ProcessMilan);
				flags.set(la.avdecc.entity.model.jsonSerializer.Flag.ProcessState);
				flags.set(la.avdecc.entity.model.jsonSerializer.Flag.ProcessStatistics);
				flags.set(la.avdecc.entity.model.jsonSerializer.Flag.ProcessCompatibility);
				flags.set(la.avdecc.entity.model.jsonSerializer.Flag.ProcessDiagnostics);
				var result = _controller.loadVirtualEntityFromJson("SimpleEntity.json", flags);
				if (result.get0() != DeserializationError.NoError)
				{
					Console.WriteLine($"Error deserializing entity model: {result.get1()}");
				}
#endif // LOAD_TEST_VIRTUAL_ENTITY_FROM_AVE
			}
			catch (la.avdecc.controller.ControllerException e)
			{
				Console.WriteLine($"Cannot create controller: {e}");
				_controller = null;
			}
		}

		public void Dispose()
		{
			_controller?.unregisterObserver(_observer);
			_controller?.Dispose();
			_controller = null;
		}

		private class Observer : la.avdecc.controller.Controller.DefaultedObserver
		{
			public override void onTransportError(la.avdecc.controller.Controller controller)
			{
				Console.WriteLine("Fatal error on transport layer");
			}

			public override void onEntityQueryError(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, la.avdecc.controller.Controller.QueryCommandError error)
			{
				var entityID = entity.getEntity().getEntityID().getValue().ToString("X");
				Console.WriteLine($"Query error on entity {entityID}: {error}");
			}

			public override void onEntityOnline(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity)
			{
				if (entity.getEntity().getEntityCapabilities().test(la.avdecc.entity.EntityCapability.AemSupported))
				{
					var configNode = entity.getCurrentConfigurationNode();
					foreach (var obj in configNode.memoryObjects)
					{
						if (obj.Value.staticModel.memoryObjectType == la.avdecc.entity.model.MemoryObjectType.PngEntity)
						{
							controller.readDeviceMemory(entity.getEntity().getEntityID(), obj.Value.staticModel.startAddress, obj.Value.staticModel.maximumLength,
							(entity, percentComplete) =>
							{
								Console.WriteLine($"Memory Object progress: {percentComplete}");
								return false;
							},
							(entity, status, memoryBuffer) =>
							{
								var filename = $"{entity.getEntity().getEntityID().getValue()}.png";
								if (status == la.avdecc.entity.LocalEntity.AaCommandStatus.Success && entity != null)
								{
									//using (FileStream fileStream = new FileStream(filename, FileMode.Create, FileAccess.Write))
									{
										// Create a buffer to read in chunks (cannot directly use the unmanaged buffer returned by memoryBuffer.data() as it requires unsafe code)
										var bufferSize = (int)memoryBuffer.size();
										byte[] buffer = new byte[bufferSize];
										System.Runtime.InteropServices.Marshal.Copy(memoryBuffer.data(), buffer, 0, bufferSize);
										File.WriteAllBytes(filename, buffer);
									}
									Console.WriteLine($"Memory Object save to file: {filename}");
								}
							});
						}
					}
					Console.WriteLine($"New AEM entity online: {entity.getEntity().getEntityID()}");
				}
				else
				{
					Console.WriteLine($"New NON-AEM entity online: {entity.getEntity().getEntityID()}");
				}
			}

			public override void onEntityOffline(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity)
			{
				var entityID = entity.getEntity().getEntityID().getValue().ToString("X");
				Console.WriteLine($"Unit going offline: {entityID}");
			}

			public override void onAecpRetryCounterChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ulong value)
			{
				var entityID = entity.getEntity().getEntityID().getValue().ToString("X");
				Console.WriteLine($"Aecp Retry Counter {entityID}: {value}");
			}

			public override void onAecpTimeoutCounterChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ulong value)
			{
				var entityID = entity.getEntity().getEntityID().getValue().ToString("X");
				Console.WriteLine($"Aecp Timeout Counter for {entityID}: {value}");
			}
			public override void onAecpUnexpectedResponseCounterChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ulong value)
			{
				var entityID = entity.getEntity().getEntityID().getValue().ToString("X");
				Console.WriteLine($"Aecp Unexpected Response Counter for {entityID}: {value}");
			}
			public override void onAecpResponseAverageTimeChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, std.chrono.milliseconds value)
			{
				var entityID = entity.getEntity().getEntityID().getValue().ToString("X");
				Console.WriteLine($"Aecp Response Average Time for {entityID}: {value} msec");
			}

			public override void onAemAecpUnsolicitedCounterChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ulong value)
			{
				var entityID = entity.getEntity().getEntityID().getValue().ToString("X");
				Console.WriteLine($"Aem Aecp Unsolicited Counter for {entityID}: {value}");
			}
			public override void onAemAecpUnsolicitedLossCounterChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ulong value)
			{
				var entityID = entity.getEntity().getEntityID().getValue().ToString("X");
				Console.WriteLine($"Aem Aecp Unsolicited Loss Counter for {entityID}: {value}");
			}
			public override void onMaxTransitTimeChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort streamIndex, std.chrono.nanoseconds maxTransitTime)
			{
				var entityID = entity.getEntity().getEntityID().getValue().ToString("X");
				Console.WriteLine($"Max Transit Time for {entityID} Stream {streamIndex}: {maxTransitTime} nsec");
			}
		}

		private la.avdecc.controller.Controller? _controller = null;
		private Observer _observer = new();
	}
}
