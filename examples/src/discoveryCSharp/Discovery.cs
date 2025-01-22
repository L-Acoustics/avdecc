// #define LOAD_TEST_VIRTUAL_ENTITY

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

#if LOAD_TEST_VIRTUAL_ENTITY
	class Builder : la.avdecc.controller.model.VirtualEntityBuilder
	{
		public Builder() : base() { }

		public override void build(Entity.CommonInformation commonInformation, InterfaceInformationMap intfcInformation)
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
		public override void build(ControlledEntity entity, EntityNodeDynamicModel model)
		{
			model.entityName = new AvdeccFixedString("SimpleEntity");
		}
		public override void build(ControlledEntity entity, ushort descriptorIndex, ConfigurationNodeDynamicModel model)
		{
		}
	}
#endif // LOAD_TEST_VIRTUAL_ENTITY

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
#if LOAD_TEST_VIRTUAL_ENTITY
				var builder = new Builder();
				var result = _controller.createVirtualEntityFromEntityModelFile("SimpleEntityModel.json", builder, false);
				if (result.get0() != DeserializationError.NoError)
				{
					Console.WriteLine($"Error deserializing entity model: {result.get1()}");
				}
#endif // LOAD_TEST_VIRTUAL_ENTITY
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

		private class Observer : la.avdecc.controller.Controller.Observer
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

			// unimplemented methods
			public override void onEntityRedundantInterfaceOnline(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort avbInterfaceIndex, la.avdecc.entity.Entity.InterfaceInformation interfaceInfo) { }
			public override void onEntityRedundantInterfaceOffline(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort avbInterfaceIndex) { }
			public override void onEntityCapabilitiesChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity) { }
			public override void onEntityAssociationIDChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity) { }
			public override void onGptpChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort avbInterfaceIndex, la.avdecc.UniqueIdentifier grandMasterID, byte grandMasterDomain) { }
			public override void onUnsolicitedRegistrationChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, bool isSubscribed) { }
			public override void onCompatibilityFlagsChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, CompatibilityFlags compatibilityFlags) { }
			public override void onIdentificationStarted(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity) { }
			public override void onIdentificationStopped(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity) { }
			public override void onStreamInputConnectionChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort streamIndex, la.avdecc.entity.model.StreamInputConnectionInfo info, bool changedByOther) { }
			public override void onStreamOutputConnectionsChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort streamIndex, StreamIdentificationSet connections) { }
			public override void onAcquireStateChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, la.avdecc.controller.model.AcquireState acquireState, la.avdecc.UniqueIdentifier owningEntity) { }
			public override void onLockStateChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, la.avdecc.controller.model.LockState lockState, la.avdecc.UniqueIdentifier lockingEntity) { }
			public override void onStreamInputFormatChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort streamIndex, la.avdecc.entity.model.StreamFormat streamFormat) { }
			public override void onStreamOutputFormatChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort streamIndex, la.avdecc.entity.model.StreamFormat streamFormat) { }
			public override void onStreamInputDynamicInfoChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort streamIndex, la.avdecc.entity.model.StreamDynamicInfo info) { }
			public override void onStreamOutputDynamicInfoChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort streamIndex, la.avdecc.entity.model.StreamDynamicInfo info) { }
			public override void onEntityNameChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, la.avdecc.entity.model.AvdeccFixedString entityName) { }
			public override void onEntityGroupNameChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, la.avdecc.entity.model.AvdeccFixedString entityGroupName) { }
			public override void onConfigurationNameChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort configurationIndex, la.avdecc.entity.model.AvdeccFixedString configurationName) { }
			public override void onAudioUnitNameChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort configurationIndex, ushort audioUnitIndex, la.avdecc.entity.model.AvdeccFixedString audioUnitName) { }
			public override void onStreamInputNameChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort configurationIndex, ushort streamIndex, la.avdecc.entity.model.AvdeccFixedString streamName) { }
			public override void onStreamOutputNameChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort configurationIndex, ushort streamIndex, la.avdecc.entity.model.AvdeccFixedString streamName) { }
			public override void onJackInputNameChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort configurationIndex, ushort jackIndex, la.avdecc.entity.model.AvdeccFixedString jackName) { }
			public override void onJackOutputNameChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort configurationIndex, ushort jackIndex, la.avdecc.entity.model.AvdeccFixedString jackName) { }
			public override void onAvbInterfaceNameChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort configurationIndex, ushort avbInterfaceIndex, la.avdecc.entity.model.AvdeccFixedString avbInterfaceName) { }
			public override void onClockSourceNameChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort configurationIndex, ushort clockSourceIndex, la.avdecc.entity.model.AvdeccFixedString clockSourceName) { }
			public override void onMemoryObjectNameChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort configurationIndex, ushort memoryObjectIndex, la.avdecc.entity.model.AvdeccFixedString memoryObjectName) { }
			public override void onAudioClusterNameChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort configurationIndex, ushort audioClusterIndex, la.avdecc.entity.model.AvdeccFixedString audioClusterName) { }
			public override void onControlNameChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort configurationIndex, ushort controlIndex, la.avdecc.entity.model.AvdeccFixedString controlName) { }
			public override void onClockDomainNameChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort configurationIndex, ushort clockDomainIndex, la.avdecc.entity.model.AvdeccFixedString clockDomainName) { }
			public override void onTimingNameChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort configurationIndex, ushort timingIndex, la.avdecc.entity.model.AvdeccFixedString timingName) { }
			public override void onPtpInstanceNameChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort configurationIndex, ushort ptpInstanceIndex, la.avdecc.entity.model.AvdeccFixedString ptpInstanceName) { }
			public override void onPtpPortNameChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort configurationIndex, ushort ptpPortIndex, la.avdecc.entity.model.AvdeccFixedString ptpPortName) { }
			public override void onAssociationIDChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, la.avdecc.UniqueIdentifier associationID) { }
			public override void onAudioUnitSamplingRateChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort audioUnitIndex, la.avdecc.entity.model.SamplingRate samplingRate) { }
			public override void onClockSourceChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort clockDomainIndex, ushort clockSourceIndex) { }
			public override void onControlValuesChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort controlIndex, la.avdecc.entity.model.ControlValues controlValues) { }
			public override void onStreamInputStarted(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort streamIndex) { }
			public override void onStreamOutputStarted(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort streamIndex) { }
			public override void onStreamInputStopped(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort streamIndex) { }
			public override void onStreamOutputStopped(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort streamIndex) { }
			public override void onAvbInterfaceInfoChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort avbInterfaceIndex, la.avdecc.entity.model.AvbInterfaceInfo info) { }
			public override void onAsPathChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort avbInterfaceIndex, la.avdecc.entity.model.AsPath asPath) { }
			public override void onAvbInterfaceLinkStatusChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort avbInterfaceIndex, la.avdecc.controller.ControlledEntity.InterfaceLinkStatus linkStatus) { }
			public override void onEntityCountersChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, EntityCounters counters) { }
			public override void onAvbInterfaceCountersChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort avbInterfaceIndex, AvbInterfaceCounters counters) { }
			public override void onClockDomainCountersChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort clockDomainIndex, ClockDomainCounters counters) { }
			public override void onStreamInputCountersChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort streamIndex, StreamInputCounters counters) { }
			public override void onStreamOutputCountersChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort streamIndex, StreamOutputCounters counters) { }
			public override void onMemoryObjectLengthChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort configurationIndex, ushort memoryObjectIndex, ulong length) { }
			public override void onStreamPortInputAudioMappingsChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort streamPortIndex) { }
			public override void onStreamPortOutputAudioMappingsChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort streamPortIndex) { }
			public override void onOperationProgress(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, la.avdecc.entity.model.DescriptorType descriptorType, ushort descriptorIndex, ushort operationID, float percentComplete) { }
			public override void onOperationCompleted(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, la.avdecc.entity.model.DescriptorType descriptorType, ushort descriptorIndex, ushort operationID, bool failed) { }
			public override void onMediaClockChainChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, ushort clockDomainIndex, MediaClockChainDeque mcChain) { }
			public override void onDiagnosticsChanged(la.avdecc.controller.Controller controller, la.avdecc.controller.ControlledEntity entity, la.avdecc.controller.ControlledEntity.Diagnostics diags) { }
		}

		private la.avdecc.controller.Controller? _controller = null;
		private Observer _observer = new();
	}
}
