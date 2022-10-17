%module(directors="1") avdecc

%include <stl.i>
%include <std_string.i>
%include <std_set.i>
%include <stdint.i>
%include <std_pair.i>
%include <std_map.i>
%include <windows.i>
#ifdef SWIGCSHARP
%include <arrays_csharp.i>
#endif

// Generated wrapper file needs to include our header file
%{
    #include <la/avdecc/utils.hpp>
    #include <la/avdecc/memoryBuffer.hpp>
    #include <la/avdecc/executor.hpp>
    #include <la/avdecc/avdecc.hpp>

    #include <la/networkInterfaceHelper/networkInterfaceHelper.hpp>

    using namespace la; // Need to force namespace because generated wrapper file belongs to the global namespace
    using namespace la::avdecc; // Need to force namespace because generated wrapper file belongs to the global namespace (cannot find utils::)
    using namespace la::avdecc::entity; // Need to force namespace because generated wrapper file belongs to the global namespace
%}

// Optimize code generation be enabling RVO
%typemap(out, optimal="1") SWIGTYPE
%{
    $result = new $1_ltype((const $1_ltype &)$1);
%}

#define LA_AVDECC_API
#define LA_AVDECC_CALL_CONVENTION

////////////////////////////////////////
// MemoryBuffer class
////////////////////////////////////////
%nspace la::avdecc::MemoryBuffer;
%rename("isEqual") la::avdecc::MemoryBuffer::operator==;
%rename("isDifferent") la::avdecc::MemoryBuffer::operator!=;
%ignore la::avdecc::MemoryBuffer::operator bool; // Ignore operator bool, isValid() is already defined
// Currently no resolution is performed in order to match function parameters. This means function parameter types must match exactly. For example, namespace qualifiers and typedefs will not work.
%ignore la::avdecc::MemoryBuffer::operator=;
// Ignore move constructor
%ignore la::avdecc::MemoryBuffer::MemoryBuffer(MemoryBuffer&&);
// Rename const data() getter
//%rename("constData") la::avdecc::MemoryBuffer::data() const; // RIGHT NOW IGNORE IT AS WE NEED TO FIND A WAY TO MARSHALL THE RETURNED POINTER
%ignore la::avdecc::MemoryBuffer::data(); // RIGHT NOW IGNORE IT AS WE NEED TO FIND A WAY TO MARSHALL THE RETURNED POINTER
%ignore la::avdecc::MemoryBuffer::data() const; // RIGHT NOW IGNORE IT AS WE NEED TO FIND A WAY TO MARSHALL THE RETURNED POINTER

#ifdef SWIGCSHARP
// Marshalling for void pointers
%apply unsigned char INPUT[]  { void const* const }
#endif

%include "la/avdecc/memoryBuffer.hpp"

////////////////////////////////////////
// Executor/ExecutorManager classes
////////////////////////////////////////
%nspace la::avdecc::ExecutorManager;
%ignore la::avdecc::ExecutorManager::pushJob(std::string const& name, Executor::Job&& job); // Disable this version of the method, we'll wrap it
%ignore la::avdecc::ExecutorManager::isExecutorRegistered; // RIGHT NOW IGNORE THIS METHOD
%ignore la::avdecc::ExecutorManager::registerExecutor; // RIGHT NOW IGNORE THIS METHOD
%ignore la::avdecc::ExecutorManager::destroyExecutor; // RIGHT NOW IGNORE THIS METHOD
%ignore la::avdecc::ExecutorManager::getExecutorThread; // RIGHT NOW IGNORE THIS METHOD
%ignore la::avdecc::ExecutorManager::ExecutorWrapper; // RIGHT NOW IGNORE THIS CLASS
%ignore la::avdecc::Executor; // RIGHT NOW IGNORE THIS CLASS
%ignore la::avdecc::ExecutorWithDispatchQueue; // RIGHT NOW IGNORE THIS CLASS
%ignore la::avdecc::ExecutorProxy; // RIGHT NOW IGNORE THIS CLASS

%feature("director") la::avdecc::executorManager::PushJobHandler;
%nspace la::avdecc::executorManager::PushJobHandler;
%inline %{
namespace la::avdecc::executorManager
{
    class PushJobHandler
    {
    public:
        virtual void call() = 0;
        virtual ~PushJobHandler() {}
    };
}
%}

// We must 'remove' the explicit keyword due to SWIG c++11 parser limitation (https://github.com/swig/swig/issues/2474)
#define explicit
%include "la/avdecc/executor.hpp"

// Define some wrapper functions
%extend la::avdecc::ExecutorManager
{
public:
    void pushJob(std::string const& name, executorManager::PushJobHandler* handler)
    {
        self->pushJob(name, [handler]()
        {
            if (handler)
            {
                handler->call();
            }
        });
    }
};


////////////////////////////////////////
// avdecc global
////////////////////////////////////////
%ignore la::avdecc::CompileOption;
%ignore la::avdecc::CompileOptionInfo;
%ignore la::avdecc::getCompileOptions();
%ignore la::avdecc::getCompileOptionsInfo();

%include "la/avdecc/avdecc.hpp"

////////////////////////////////////////
// Exception class
////////////////////////////////////////
%nspace la::avdecc::Exception;
// Currently no resolution is performed in order to match function parameters. This means function parameter types must match exactly. For example, namespace qualifiers and typedefs will not work.
%ignore la::avdecc::Exception::operator=;
// Ignore char* constructor
%ignore la::avdecc::Exception::Exception(char const* const);
// Ignore move constructor
%ignore la::avdecc::Exception::Exception(Exception&&);

%include "la/avdecc/internals/exception.hpp"

////////////////////////////////////////
// UniqueIdentifier
////////////////////////////////////////
%nspace la::avdecc::UniqueIdentifier;

// We must 'remove' the constexpr keyword due to SWIG c++11 parser limitation
#define constexpr
%include "la/avdecc/internals/uniqueIdentifier.hpp" // Cannot be included right now, due to SWIG c++11 parser limitation
#undef constexpr

////////////////////////////////////////
// Entity/LocalEntity
////////////////////////////////////////
%nspace la::avdecc::entity::Entity;
%nspace la::avdecc::entity::LocalEntity;
// Currently no resolution is performed in order to match function parameters. This means function parameter types must match exactly. For example, namespace qualifiers and typedefs will not work.
%ignore la::avdecc::entity::Entity::operator=;
// Ignore move constructor
%ignore la::avdecc::entity::Entity(Entity&&);
// Ignore const overloads
%ignore la::avdecc::entity::Entity::getCommonInformation() const;
%ignore la::avdecc::entity::Entity::getInterfaceInformation(model::AvbInterfaceIndex const interfaceIndex) const;
%ignore la::avdecc::entity::Entity::getInterfacesInformation() const;
// Rename operators
//%rename("isDifferent") la::avdecc::entity::LocalEntity::AemCommandStatus::operator!; // Doesn't apply :(
//%rename("Or") la::avdecc::entity::LocalEntity::AemCommandStatus::operator|; // Doesn't apply :(
//%rename("OrEqual") la::avdecc::entity::LocalEntity::AemCommandStatus::operator|=; // Doesn't apply :(

%include "la/avdecc/internals/entity.hpp"

////////////////////////////////////////
// ControllerEntity
////////////////////////////////////////
%nspace la::avdecc::entity::controller::Interface;
%nspace la::avdecc::entity::controller::Delegate;
%nspace la::avdecc::entity::controller::ControllerEntity;
// Currently no resolution is performed in order to match function parameters. This means function parameter types must match exactly. For example, namespace qualifiers and typedefs will not work.
%ignore la::avdecc::entity::controller::Interface::operator=;
// Ignore move constructor
%ignore la::avdecc::controller::Interface::Interface(Interface&&);
// Currently no resolution is performed in order to match function parameters. This means function parameter types must match exactly. For example, namespace qualifiers and typedefs will not work.
%ignore la::avdecc::entity::controller::Delegate::operator=;
// Ignore move constructor
%ignore la::avdecc::controller::Delegate::Delegate(Delegate&&);
//%include "la/avdecc/internals/controllerEntity.hpp"

////////////////////////////////////////
// EndStation
////////////////////////////////////////
%nspace la::avdecc::EndStation;
//%include "la/avdecc/internals/endStation.hpp"
