////////////////////////////////////////
// AVDECC CONTROLLER LIBRARY SWIG file
////////////////////////////////////////

%module(directors="1") avdeccController

%include <stl.i>
%include <std_string.i>
%include <std_set.i>
%include <stdint.i>
%include <std_pair.i>
%include <std_map.i>
%include <windows.i>
%include <std_unique_ptr.i>
#if 0
%include <swiginterface.i>
#endif
#ifdef SWIGCSHARP
%include <arrays_csharp.i>
#endif
//%include "la/avdecc/internals/chrono.i"

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
%enddef

// Bind enums
%nspace la::avdecc::controller::model::AcquireState;
%nspace la::avdecc::controller::model::LockState;
%nspace la::avdecc::controller::model::MediaClockChainNode::Type;
%nspace la::avdecc::controller::model::MediaClockChainNode::Status;

// Define optionals
//DEFINE_OPTIONAL_CLASS(la::avdecc::entity::model, StreamIndex, OptStreamIndex, null) // Not compiling in C# (Inconsistent accessibility)

// Bind structs and classes
%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable

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
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(StreamNode)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(StreamNodeInput)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(StreamNodeOutput)
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


////////////////////////////////////////
// AVDECC CONTROLLED ENTITY
////////////////////////////////////////
// Bind enums
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::controller::ControlledEntity, CompatibilityFlags, CompatibilityFlag, "byte")

// Bind structs and classes
%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable

%nspace la::avdecc::controller::ControlledEntity;
%rename("%s") la::avdecc::controller::ControlledEntity; // Unignore class
%rename("lockEntity") la::avdecc::controller::ControlledEntity::lock; // Rename method
%rename("unlockEntity") la::avdecc::controller::ControlledEntity::unlock; // Rename method
//%ignore la::avdecc::entity::controller::ControllerEntity::ControllerEntity(ControllerEntity&&); // Ignore move constructor
//%ignore la::avdecc::entity::controller::ControllerEntity::operator=; // Ignore copy operator

// Include c++ declaration file
%include "la/avdecc/controller/internals/avdeccControlledEntity.hpp"
%rename("%s", %$isclass) ""; // Undo the ignore all structs/classes


////////////////////////////////////////
// AVDECC CONTROLLER
////////////////////////////////////////
// Bind structs and classes
%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable

%nspace la::avdecc::controller::Controller;
%rename("%s") la::avdecc::controller::Controller; // Unignore class
%ignore la::avdecc::controller::Controller::Exception; // Ignore Exception, will be created as native exception
%ignore la::avdecc::controller::Controller::create(protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, UniqueIdentifier const entityModelID, std::string const& preferedLocale, entity::model::EntityTree const* const entityModelTree); // Ignore it, will be wrapped
%unique_ptr(la::avdecc::controller::Controller) // Define unique_ptr for Controller

%nspace la::avdecc::controller::Controller::Observer;
%rename("%s") la::avdecc::controller::Controller::Observer; // Unignore class
%feature("director") la::avdecc::controller::Controller::Observer;

%nspace la::avdecc::controller::Controller::ExclusiveAccessToken;
%rename("%s") la::avdecc::controller::Controller::ExclusiveAccessToken; // Unignore class
%unique_ptr(la::avdecc::controller::Controller::ExclusiveAccessToken) // Define unique_ptr for ExclusiveAccessToken

// Bind enums
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::controller, CompileOptions, CompileOption, "uint")

// Define C# exception handling for la::avdecc::controller::Controller::Exception
%insert(runtime) %{
	typedef void (SWIGSTDCALL* ControllerExceptionCallback_t)(la::avdecc::controller::Controller::Error const error, char const* const message);
	ControllerExceptionCallback_t controllerExceptionCallback = NULL;

	extern "C" SWIGEXPORT void SWIGSTDCALL ControllerExceptionRegisterCallback(ControllerExceptionCallback_t exceptionCallback)
	{
		controllerExceptionCallback = exceptionCallback;
	}

	static void SWIG_CSharpSetPendingExceptionController(la::avdecc::controller::Controller::Error const error, char const* const message)
	{
		controllerExceptionCallback(error, message);
	}
%}
%pragma(csharp) imclasscode=%{
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
	static ControllerExceptionHelper exceptionHelper = new ControllerExceptionHelper();
%}
%pragma(csharp) moduleimports=%{
namespace la.avdecc.controller
{
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
// Throw typemap
%typemap (throws, canthrow=1) la::avdecc::controller::Controller::Exception %{
	SWIG_CSharpSetPendingExceptionController($1.getError(), $1.what());
	return $null;
%}

// Define catches for methods that can throw
%catches(la::avdecc::controller::Controller::Exception) la::avdecc::controller::Controller::createController;

// Workaround for SWIG bug
#define constexpr
%ignore la::avdecc::controller::InterfaceVersion; // Ignore because of constexpr undefined

// Include c++ declaration file
%rename("$ignore", regextarget=1, fullname=1, $isfunction) "la::avdecc::controller::Controller::.*"; // Temp ignore all methods
%rename("%s") "la::avdecc::controller::Controller::createController"; // Force declare createController
%include "la/avdecc/controller/avdeccController.hpp"
%rename("%s", %$isclass) ""; // Undo the ignore all structs/classes

// Define wrapped functions
%extend la::avdecc::controller::Controller
{
public:
	static std::unique_ptr<la::avdecc::controller::Controller> createController(/*protocol::ProtocolInterface::Type const protocolInterfaceType, */std::string const& interfaceName, std::uint16_t const progID, UniqueIdentifier const entityModelID, std::string const& preferedLocale, entity::model::EntityTree const* const entityModelTree)
	{
		try
		{
			// Right now, force PCap as we cannot bind the protocolInterfaceType enum correctly
			return std::unique_ptr<la::avdecc::controller::Controller>{ la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::PCap, interfaceName, progID, entityModelID, preferedLocale, entityModelTree).release() };
		}
		catch (la::avdecc::controller::Controller::Exception const& e)
		{
			SWIG_CSharpSetPendingExceptionController(e.getError(), e.what());
			return nullptr;
		}
	}
};
