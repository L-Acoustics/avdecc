////////////////////////////////////////
// AVDECC CONTROLLER LIBRARY SWIG file
////////////////////////////////////////

%module(directors="1") avdeccController

%include <stl.i>
%include <std_string.i>
%include <std_set.i>
%include <stdint.i>
%include <std_pair.i>
%include <std_deque.i>
%include <std_map.i>
%include <windows.i>
%include <std_unique_ptr.i>
#ifdef SWIGCSHARP
%include <arrays_csharp.i>
#endif

// Generated wrapper file needs to include our header file (include as soon as possible using 'insert(runtime)' as target language exceptions are defined early in the generated wrapper file)
%insert(runtime) %{
	#include <la/avdecc/controller/avdeccController.hpp>
%}

// Optimize code generation be enabling RVO
%typemap(out, optimal="1") SWIGTYPE
%{
	$result = new $1_ltype((const $1_ltype &)$1);
%}

#define LA_AVDECC_CONTROLLER_API
#define LA_AVDECC_CONTROLLER_CALL_CONVENTION

////////////////////////////////////////
// Utils
////////////////////////////////////////
%include "la/avdecc/utils.i"


////////////////////////////////////////
// Entity Model
////////////////////////////////////////
%import "la/avdecc/internals/entityModel.i"


////////////////////////////////////////
// AVDECC Library
////////////////////////////////////////
%import "la/avdecc/avdecc.i"


////////////////////////////////////////
// AVDECC CONTROLLED ENTITY MODEL
////////////////////////////////////////
// Define some macros
%define DEFINE_CONTROLLED_ENTITY_MODEL_NODE(name)
	%nspace la::avdecc::controller::model::name##Node;
	%rename("%s") la::avdecc::controller::model::name##Node; // Unignore class
	// DO NOT extend the struct with copy-constructor (we don't want to copy the full hierarchy, and also there is no default constructor)
%enddef

// Bind enums
%nspace la::avdecc::controller::model::AcquireState;
%nspace la::avdecc::controller::model::LockState;
%nspace la::avdecc::controller::model::MediaClockChainNode::Type;
%nspace la::avdecc::controller::model::MediaClockChainNode::Status;

// Define optionals
DEFINE_OPTIONAL_CLASS(la::avdecc::entity::model, MilanInfo, OptMilanInfo)

// Bind structs and classes
%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable

DEFINE_OBSERVER_CLASS(la::avdecc::controller::model::EntityModelVisitor)

DEFINE_CONTROLLED_ENTITY_MODEL_NODE(MediaClockChain)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE()
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(EntityModel)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(Virtual)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(Control)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(AudioMap)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(AudioCluster)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(StreamPort)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(StreamPortInput)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(StreamPortOutput)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(AudioUnit)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(Stream)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(StreamInput)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(StreamOutput)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(RedundantStream)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(RedundantStreamInput)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(RedundantStreamOutput)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(Jack)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(JackInput)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(JackOutput)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(AvbInterface)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(ClockSource)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(MemoryObject)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(Strings)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(Locale)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(ClockDomain)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(Configuration)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(Entity)

// Include c++ declaration file
%include "la/avdecc/controller/internals/avdeccControlledEntityModel.hpp"
%rename("%s", %$isclass) ""; // Undo the ignore all structs/classes

// Define templates
// WARNING: Requires https://github.com/swig/swig/issues/2625 to be fixed (or a modified version of the std_map.i file)
%template(AudioClusterNodeMap) std::map<la::avdecc::entity::model::ClusterIndex, la::avdecc::controller::model::AudioClusterNode>;
%template(AudioMapNodeMap) std::map<la::avdecc::entity::model::MapIndex, la::avdecc::controller::model::AudioMapNode>;
%template(ControlNodeMap) std::map<la::avdecc::entity::model::ControlIndex, la::avdecc::controller::model::ControlNode>;
%template(StreamPortInputsNodeMap) std::map<la::avdecc::entity::model::StreamPortIndex, la::avdecc::controller::model::StreamPortInputNode>;
%template(StreamPortOutputNodeMap) std::map<la::avdecc::entity::model::StreamPortIndex, la::avdecc::controller::model::StreamPortOutputNode>;
%template(StringNodeMap) std::map<la::avdecc::entity::model::StringsIndex, la::avdecc::controller::model::StringsNode>;
%template(AudioUnitNodeMap) std::map<la::avdecc::entity::model::AudioUnitIndex, la::avdecc::controller::model::AudioUnitNode>;
%template(StreamInputNodeMap) std::map<la::avdecc::entity::model::StreamIndex, la::avdecc::controller::model::StreamInputNode>;
%template(StreamOutputNodeMap) std::map<la::avdecc::entity::model::StreamIndex, la::avdecc::controller::model::StreamOutputNode>;
%template(JackInputNodeMap) std::map<la::avdecc::entity::model::JackIndex, la::avdecc::controller::model::JackInputNode>;
%template(JackOutputNodeMap) std::map<la::avdecc::entity::model::JackIndex, la::avdecc::controller::model::JackOutputNode>;
%template(AvbInterfaceNodeMap) std::map<la::avdecc::entity::model::AvbInterfaceIndex, la::avdecc::controller::model::AvbInterfaceNode>;
%template(ClockSourceNodeMap) std::map<la::avdecc::entity::model::ClockSourceIndex, la::avdecc::controller::model::ClockSourceNode>;
%template(MemoryObjectNodeMap) std::map<la::avdecc::entity::model::MemoryObjectIndex, la::avdecc::controller::model::MemoryObjectNode>;
%template(LocaleNodeMap) std::map<la::avdecc::entity::model::LocaleIndex, la::avdecc::controller::model::LocaleNode>;
%template(ClockDomainNodeMap) std::map<la::avdecc::entity::model::ClockDomainIndex, la::avdecc::controller::model::ClockDomainNode>;
%template(RedundantStreamInputNodeMap) std::map<la::avdecc::controller::model::VirtualIndex, la::avdecc::controller::model::RedundantStreamInputNode>;
%template(RedundantStreamOutputNodeMap) std::map<la::avdecc::controller::model::VirtualIndex, la::avdecc::controller::model::RedundantStreamOutputNode>;
%template(ConfigurationNodeMap) std::map<la::avdecc::entity::model::ConfigurationIndex, la::avdecc::controller::model::ConfigurationNode>;
%template(MediaClockChainDeque) std::deque<la::avdecc::controller::model::MediaClockChainNode>;


////////////////////////////////////////
// AVDECC CONTROLLED ENTITY
////////////////////////////////////////
// Bind enums
DEFINE_ENUM_CLASS(la::avdecc::controller::ControlledEntity, CompatibilityFlag, "byte")

// Bind structs and classes
%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable

%nspace la::avdecc::controller::ControlledEntity;
%rename("%s") la::avdecc::controller::ControlledEntity; // Unignore class
%rename("lockEntity") la::avdecc::controller::ControlledEntity::lock; // Rename method
%rename("unlockEntity") la::avdecc::controller::ControlledEntity::unlock; // Rename method

%nspace la::avdecc::controller::ControlledEntity::Diagnostics;
%rename("%s") la::avdecc::controller::ControlledEntity::Diagnostics; // Unignore class
// Extend the struct
%extend la::avdecc::controller::ControlledEntity::Diagnostics
{
	// Add default constructor
	Diagnostics()
	{
		return new la::avdecc::controller::ControlledEntity::Diagnostics();
	}
	// Add a copy-constructor
	Diagnostics(la::avdecc::controller::ControlledEntity::Diagnostics const& other)
	{
		return new la::avdecc::controller::ControlledEntity::Diagnostics(other);
	}
#if defined(SWIGCSHARP)
  // Provide a more native Equals() method
  bool Equals(la::avdecc::controller::ControlledEntity::Diagnostics const& other) const noexcept
  {
    return $self->redundancyWarning == other.redundancyWarning && $self->streamInputOverLatency == other.streamInputOverLatency;
  }
#endif
};

%nspace la::avdecc::controller::ControlledEntityGuard;
%rename("%s") la::avdecc::controller::ControlledEntityGuard; // Unignore class
%ignore la::avdecc::controller::ControlledEntityGuard::operator bool; // Ignore operator bool, isValid() is already defined

// Throw typemap
%typemap (throws, canthrow=1) la::avdecc::controller::ControlledEntity::Exception %{
	SWIG_CSharpSetPendingExceptionControlledEntity($1.getType(), $1.what());
	return $null;
%}

// Define catches for methods that can throw
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::isStreamInputRunning;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::isStreamOutputRunning;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getCurrentConfigurationIndex;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getEntityNode;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getConfigurationNode;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getCurrentConfigurationNode;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getAudioUnitNode;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getStreamInputNode;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getStreamOutputNode;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getRedundantStreamInputNode;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getRedundantStreamOutputNode;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getJackInputNode;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getJackOutputNode;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getAvbInterfaceNode;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getClockSourceNode;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getStreamPortInputNode;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getStreamPortOutputNode;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getAudioClusterNode;
//%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getAudioMapNode;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getControlNode;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getClockDomainNode;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::findLocaleNode;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getLocalizedString;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getSinkConnectionInformation;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getStreamPortInputAudioMappings;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getStreamPortInputNonRedundantAudioMappings;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getStreamPortOutputAudioMappings;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getStreamPortOutputNonRedundantAudioMappings;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getStreamPortInputInvalidAudioMappingsForStreamFormat;
%catches(la::avdecc::controller::ControlledEntity::Exception) la::avdecc::controller::ControlledEntity::getStreamOutputConnections;

// Include c++ declaration file
%include "la/avdecc/controller/internals/avdeccControlledEntity.hpp"
%rename("%s", %$isclass) ""; // Undo the ignore all structs/classes

// Define templates
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::controller::ControlledEntity, CompatibilityFlags, CompatibilityFlag, std::uint8_t)
%template("StreamPortInvalidAudioMappings") std::map<la::avdecc::entity::model::StreamPortIndex, la::avdecc::entity::model::AudioMappings>;


////////////////////////////////////////
// AVDECC CONTROLLER
////////////////////////////////////////
// Define Observer templates
%template("ControllerSubject") la::avdecc::utils::Subject<la::avdecc::controller::Controller, std::recursive_mutex>;
%template("ControllerObserver") la::avdecc::utils::Observer<la::avdecc::controller::Controller>;

// Bind enums
DEFINE_ENUM_CLASS(la::avdecc::controller, CompileOption, "uint")
DEFINE_ENUM_CLASS(la::avdecc::controller::Controller, Error, "uint")
DEFINE_ENUM_CLASS(la::avdecc::controller::Controller, QueryCommandError, "uint")

// Bind structs and classes
%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable

%nspace la::avdecc::controller::CompileOptionInfo;
%rename("%s") la::avdecc::controller::CompileOptionInfo; // Unignore class

%nspace la::avdecc::controller::Controller;
%rename("%s") la::avdecc::controller::Controller; // Unignore class
%ignore la::avdecc::controller::Controller::Exception; // Ignore Exception, will be created as native exception
%unique_ptr(la::avdecc::controller::Controller) // Define unique_ptr for Controller
%rename("lockController") la::avdecc::controller::Controller::lock; // Rename method
%rename("unlockController") la::avdecc::controller::Controller::unlock; // Rename method
%std_tuple(Tuple_SerializationError_String, la::avdecc::jsonSerializer::SerializationError, std::string);
%std_tuple(Tuple_DeserializationError_String, la::avdecc::jsonSerializer::DeserializationError, std::string);
//%std_tuple(Tuple_DeserializationError_String_VectorSharedControlledEntity, la::avdecc::jsonSerializer::DeserializationError, std::string, std::vector<la::avdecc::controller::SharedControlledEntity>); // Temp ignore
//%std_tuple(Tuple_DeserializationError_String_SharedControlledEntity, la::avdecc::jsonSerializer::DeserializationError, std::string, la::avdecc::controller::SharedControlledEntity); // Temp ignore
// Extend the class
%extend la::avdecc::controller::Controller
{
public:
	static std::unique_ptr<la::avdecc::controller::Controller> create(/*protocol::ProtocolInterface::Type const protocolInterfaceType, */std::string const& interfaceName, std::uint16_t const progID, UniqueIdentifier const entityModelID, std::string const& preferedLocale, entity::model::EntityTree const* const entityModelTree, std::optional<std::string> const& executorName, entity::controller::Interface const* const virtualEntityInterface)
	{
		try
		{
			// Right now, force PCap as we cannot bind the protocolInterfaceType enum correctly
			return std::unique_ptr<la::avdecc::controller::Controller>{ la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::PCap, interfaceName, progID, entityModelID, preferedLocale, entityModelTree, executorName, virtualEntityInterface).release() };
		}
		catch (la::avdecc::controller::Controller::Exception const& e)
		{
			SWIG_CSharpSetPendingExceptionController(e.getError(), e.what());
			return nullptr;
		}
	}
};
%ignore la::avdecc::controller::Controller::create; // Ignore it, will be wrapped (because std::unique_ptr doesn't support custom deleters - Ticket #2411)

DEFINE_OBSERVER_CLASS(la::avdecc::controller::Controller::Observer)

#if SUPPORT_EXCLUSIVE_ACCESS
%nspace la::avdecc::controller::Controller::ExclusiveAccessToken;
%rename("%s") la::avdecc::controller::Controller::ExclusiveAccessToken; // Unignore class
%unique_ptr(la::avdecc::controller::Controller::ExclusiveAccessToken) // Define unique_ptr for ExclusiveAccessToken // FIXME need second template parameter for deleter (see https://github.com/swig/swig/issues/2411)
#else
%ignore la::avdecc::controller::Controller::requestExclusiveAccess; // Ignore until https://github.com/swig/swig/issues/2411 is fixed
#endif

// %rename("%s") la::avdecc::controller::Controller::Error; // Must unignore the enum since it's inside a class
// %rename("%s") la::avdecc::controller::Controller::QueryCommandError; // Must unignore the enum since it's inside a class

// Throw typemap
%typemap (throws, canthrow=1) la::avdecc::controller::Controller::Exception %{
	SWIG_CSharpSetPendingExceptionController($1.getError(), $1.what());
	return $null;
%}

// Define catches for methods that can throw
%catches(la::avdecc::controller::Controller::Exception) la::avdecc::controller::Controller::create;

// Workaround for SWIG bug
#define constexpr
%ignore la::avdecc::controller::InterfaceVersion; // Ignore because of constexpr undefined
%ignore la::avdecc::controller::Controller::ChecksumVersion; // Ignore because of constexpr undefined
%ignore la::avdecc::controller::getCompileOptions; // Ignore because CompileOptions fails to be mapped correctly // TODO: FIXME
%ignore la::avdecc::controller::getCompileOptionsInfo; // Ignore for now // TODO: FIXME

%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::deserializeControlledEntitiesFromJsonNetworkState"; // Temp ignore method
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::deserializeControlledEntityFromJson"; // Temp ignore method

// Unignore functions automatically generated by the following std_function calls (because we asked to ignore all methods earlier)
%rename("%s") Handler_Entity_AemCommandStatus_UniqueIdentifier;
%rename("%s") Handler_Entity_AemCommandStatus;
%rename("%s") Handler_Entity_AemCommandStatus_OperationID;
%rename("%s") Handler_Entity_float;
%rename("%s") Handler_Entity_AaCommandStatus_DeviceMemoryBuffer;
%rename("%s") Handler_Entity_AaCommandStatus;
#if TYPED_DESCRIPTOR_INDEXES
%rename("%s") Handler_Entity_Entity_StreamIndex_StreamIndex_ControlStatus;
%rename("%s") Handler_Entity_Entity_StreamIndex_ControlStatus;
%rename("%s") Handler_Entity_StreamIndex_ControlStatus;
%rename("%s") Handler_Entity_Entity_StreamIndex_StreamIndex_uint16_ConnectionFlags_ControlStatus;
#else
%rename("%s") Handler_Entity_Entity_DescriptorIndex_DescriptorIndex_ControlStatus;
%rename("%s") Handler_Entity_Entity_DescriptorIndex_ControlStatus;
%rename("%s") Handler_Entity_DescriptorIndex_ControlStatus;
%rename("%s") Handler_Entity_Entity_DescriptorIndex_DescriptorIndex_uint16_ConnectionFlags_ControlStatus;
#endif
%rename("%s") Handler_Entity_ControlStatus;
#if SUPPORT_EXCLUSIVE_ACCESS
%rename("%s") Handler_Entity_AemCommandStatus_ExclusiveAccessToken;
#endif
%rename("%s") Handler_bool_bool;
// TODO: Would be nice to have the handler in the same namespace as the class (ie. be able to pass a namespace to std_function)
%std_function(Handler_Entity_AemCommandStatus_UniqueIdentifier, void, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const entityID);
%std_function(Handler_Entity_AemCommandStatus, void, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::LocalEntity::AemCommandStatus const status);
%std_function(Handler_Entity_AemCommandStatus_OperationID, void, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::OperationID const operationID);
%std_function(Handler_Entity_float, bool, la::avdecc::controller::ControlledEntity const* const entity, float const percentComplete);
%std_function(Handler_Entity_AaCommandStatus_DeviceMemoryBuffer, void, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::LocalEntity::AaCommandStatus const status, la::avdecc::controller::Controller::DeviceMemoryBuffer const& memoryBuffer);
%std_function(Handler_Entity_AaCommandStatus, void, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::LocalEntity::AaCommandStatus const status);
#if TYPED_DESCRIPTOR_INDEXES
%std_function(Handler_Entity_Entity_StreamIndex_StreamIndex_ControlStatus, void, la::avdecc::controller::ControlledEntity const* const talkerEntity, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, la::avdecc::entity::LocalEntity::ControlStatus const status);
%std_function(Handler_Entity_StreamIndex_ControlStatus, void, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, la::avdecc::entity::LocalEntity::ControlStatus const status);
%std_function(Handler_Entity_Entity_StreamIndex_StreamIndex_uint16_ConnectionFlags_ControlStatus, void, la::avdecc::controller::ControlledEntity const* const talkerEntity, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status);
#else
%std_function(Handler_Entity_Entity_DescriptorIndex_DescriptorIndex_ControlStatus, void, la::avdecc::controller::ControlledEntity const* const talkerEntity, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::DescriptorIndex const talkerDescriptorIndex, la::avdecc::entity::model::DescriptorIndex const listenerDescriptorIndex, la::avdecc::entity::LocalEntity::ControlStatus const status);
%std_function(Handler_Entity_DescriptorIndex_ControlStatus, void, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::DescriptorIndex const listenerDescriptorIndex, la::avdecc::entity::LocalEntity::ControlStatus const status);
%std_function(Handler_Entity_Entity_DescriptorIndex_DescriptorIndex_uint16_ConnectionFlags_ControlStatus, void, la::avdecc::controller::ControlledEntity const* const talkerEntity, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::DescriptorIndex const talkerDescriptorIndex, la::avdecc::entity::model::DescriptorIndex const listenerDescriptorIndex, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status);
#endif
%std_function(Handler_Entity_ControlStatus, void, la::avdecc::entity::LocalEntity::ControlStatus const status);
#if SUPPORT_EXCLUSIVE_ACCESS
%std_function(Handler_Entity_AemCommandStatus_ExclusiveAccessToken, void, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::controller::Controller::ExclusiveAccessToken::UniquePointer&& token);
#endif
%std_function(Handler_bool_bool, bool, bool const isDesiredClockSync, bool const isAvailableClockSync);


// Include c++ declaration file
%include "la/avdecc/controller/avdeccController.hpp"
%rename("%s", %$isclass) ""; // Undo the ignore all structs/classes

// Define templates
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::controller, CompileOptions, CompileOption, std::uint32_t)


// Define C# exception handling
%insert(runtime) %{
	// la::avdecc::controller::ControlledEntity::Exception
	typedef void (SWIGSTDCALL* ControlledEntityExceptionCallback_t)(la::avdecc::controller::ControlledEntity::Exception::Type const type, char const* const message);
	ControlledEntityExceptionCallback_t controlledEntityExceptionCallback = NULL;

	extern "C" SWIGEXPORT void SWIGSTDCALL ControlledEntityExceptionRegisterCallback(ControlledEntityExceptionCallback_t cb)
	{
		controlledEntityExceptionCallback = cb;
	}

	static void SWIG_CSharpSetPendingExceptionControlledEntity(la::avdecc::controller::ControlledEntity::Exception::Type const type, char const* const message)
	{
		controlledEntityExceptionCallback(type, message);
	}

	// la::avdecc::controller::Controller::Exception
	typedef void (SWIGSTDCALL* ControllerExceptionCallback_t)(la::avdecc::controller::Controller::Error const error, char const* const message);
	ControllerExceptionCallback_t controllerExceptionCallback = NULL;

	extern "C" SWIGEXPORT void SWIGSTDCALL ControllerExceptionRegisterCallback(ControllerExceptionCallback_t cb)
	{
		controllerExceptionCallback = cb;
	}

	static void SWIG_CSharpSetPendingExceptionController(la::avdecc::controller::Controller::Error const error, char const* const message)
	{
		controllerExceptionCallback(error, message);
	}
%}
%pragma(csharp) imclasscode=%{
	// la::avdecc::controller::ControlledEntity::Exception
	class ControlledEntityExceptionHelper
	{
		public delegate void ControlledEntityExceptionDelegate(la.avdecc.controller.ControlledEntityException.Type type, string message);
		static ControlledEntityExceptionDelegate controlledEntityDelegate = new ControlledEntityExceptionDelegate(SetPendingControlledEntityException);

		[global::System.Runtime.InteropServices.DllImport(DllImportPath, EntryPoint="ControlledEntityExceptionRegisterCallback")]
		public static extern void ControlledEntityExceptionRegisterCallback(ControlledEntityExceptionDelegate controlledEntityDelegate);

		static void SetPendingControlledEntityException(la.avdecc.controller.ControlledEntityException.Type type, string message)
		{
			SWIGPendingException.Set(new la.avdecc.controller.ControlledEntityException(type, message));
		}

		static ControlledEntityExceptionHelper()
		{
			ControlledEntityExceptionRegisterCallback(controlledEntityDelegate);
		}
	}
	static ControlledEntityExceptionHelper controlledEntityExceptionHelper = new ControlledEntityExceptionHelper();

	// la::avdecc::controller::Controller::Exception
	class ControllerExceptionHelper
	{
		public delegate void ControllerExceptionDelegate(la.avdecc.controller.ControllerException.Error error, string message);
		static ControllerExceptionDelegate controllerDelegate = new ControllerExceptionDelegate(SetPendingControllerException);

		[global::System.Runtime.InteropServices.DllImport(DllImportPath, EntryPoint="ControllerExceptionRegisterCallback")]
		public static extern void ControllerExceptionRegisterCallback(ControllerExceptionDelegate controllerDelegate);

		static void SetPendingControllerException(la.avdecc.controller.ControllerException.Error error, string message)
		{
			SWIGPendingException.Set(new la.avdecc.controller.ControllerException(error, message));
		}

		static ControllerExceptionHelper()
		{
			ControllerExceptionRegisterCallback(controllerDelegate);
		}
	}
	static ControllerExceptionHelper controllerExceptionHelper = new ControllerExceptionHelper();
%}
%pragma(csharp) moduleimports=%{
namespace la.avdecc.controller
{
	// la::avdecc::controller::ControlledEntity::Exception
	class ControlledEntityException : global::System.ApplicationException
	{
		public enum Type
		{
			None = 0,
			NotSupported, /**< Query not support by the Entity */
			InvalidConfigurationIndex, /**< Specified ConfigurationIndex does not exist */
			InvalidDescriptorIndex, /**< Specified DescriptorIndex (or any derivated) does not exist */
			InvalidLocaleName, /**< Specified Locale does not exist */
			EnumerationError, /**< Trying to get information from an Entity that got an error during descriptors enumeration. Only non-throwing methods can be called. */
			Internal = 99, /**< Internal library error, please report */
		}
		public ControlledEntityException(Type type, string message)
			: base(message)
		{
			_type = type;
		}
		Type getType()
		{
			return _type;
		}
		private Type _type = Type.None;
	}

	// la::avdecc::controller::Controller::Exception
	class ControllerException : global::System.ApplicationException
	{
		public enum Error
		{
			NoError = 0,
			InvalidProtocolInterfaceType = 1, /**< Selected protocol interface type is invalid */
			InterfaceOpenError = 2, /**< Failed to open interface. */
			InterfaceNotFound = 3, /**< Specified interface not found. */
			InterfaceInvalid = 4, /**< Specified interface is invalid. */
			DuplicateProgID = 5, /**< Specified ProgID is already in use on the local computer. */
			InvalidEntityModel = 6, /**< Provided EntityModel is invalid. */
			DuplicateExecutorName = 7, /**< Provided executor name already exists. */
			UnknownExecutorName = 8, /**< Provided executor name doesn't exist. */
			InternalError = 99, /**< Internal error, please report the issue. */
		}
		public ControllerException(Error error, string message)
			: base(message)
		{
			_error = error;
		}
		Error getError()
		{
			return _error;
		}
		private Error _error = Error.NoError;
	}
}
%}
