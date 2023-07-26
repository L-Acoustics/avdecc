////////////////////////////////////////
// AVDECC ENTITY MODEL SWIG file
////////////////////////////////////////

%module avdeccEntityModel

%include <stdint.i>
%include <std_string.i>
%include <std_pair.i>
%include <std_vector.i>
%include <std_array.i>
%include <std_set.i>
%include "la/avdecc/internals/std_unordered_map.i" // From https://github.com/microsoft/CNTK/blob/master/bindings/csharp/Swig/std_unordered_map.i and https://github.com/swig/swig/pull/2480
%include "la/avdecc/internals/optional.i"

// Generated wrapper file needs to include our header file
%{
		#include <la/avdecc/internals/entityModel.hpp>
%}

// C# Specifics
#if defined(SWIGCSHARP)
// Optimize code generation by enabling RVO
%typemap(out, optimal="1") SWIGTYPE
%{
    $result = new $1_ltype(($1_ltype const&)$1);
%}
// Marshal all std::string as UTF8Str
%typemap(imtype, outattributes="[return: System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPUTF8Str)]", inattributes="[System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPUTF8Str)] ") std::string, std::string const& "string"
// Better debug display
%typemap(csattributes) la::avdecc::entity::model::AvdeccFixedString "[System.Diagnostics.DebuggerDisplay(\"{toString()}\")]"
#endif

// Force define AVDECC C/C++ API Macros to nothing
#define LA_AVDECC_API
#define LA_AVDECC_CALL_CONVENTION

// Other defines
#define ENABLE_AVDECC_FEATURE_REDUNDANCY 1

////////////////////////////////////////
// Utils
////////////////////////////////////////
%include "la/avdecc/utils.i"


////////////////////////////////////////
// UniqueIdentifier
////////////////////////////////////////
%nspace la::avdecc::UniqueIdentifier;
%ignore la::avdecc::UniqueIdentifier::operator value_type() const noexcept; // Ignore, don't need it (already have getValue() method)
%ignore la::avdecc::UniqueIdentifier::operator bool() const noexcept; // Ignore, don't need it (already have isValid() method)
%rename("isEqual") operator==(UniqueIdentifier const& lhs, UniqueIdentifier const& rhs) noexcept; // Not put in a namespace https://github.com/swig/swig/issues/2459
%rename("isDifferent") operator!=(UniqueIdentifier const& lhs, UniqueIdentifier const& rhs) noexcept; // Not put in a namespace https://github.com/swig/swig/issues/2459
%rename("isLess") operator<(UniqueIdentifier const& lhs, UniqueIdentifier const& rhs) noexcept; // Not put in a namespace https://github.com/swig/swig/issues/2459
%ignore la::avdecc::UniqueIdentifier::hash::operator(); // Ignore hash functor
%ignore la::avdecc::UniqueIdentifier::UniqueIdentifier(UniqueIdentifier&&); // Ignore move constructor
%ignore la::avdecc::UniqueIdentifier::operator=; // Ignore copy operator
// Extend the class
%extend la::avdecc::UniqueIdentifier
{
#if defined(SWIGCSHARP)
	// Provide a more native Equals() method
	bool Equals(la::avdecc::UniqueIdentifier const& other) const noexcept
	{
		return *$self == other;
	}
#endif
}


// Include c++ declaration file
%include "la/avdecc/internals/uniqueIdentifier.hpp"


////////////////////////////////////////
// Entity Model Types
////////////////////////////////////////
// Define some macros
%define DEFINE_AEM_TYPES_ENUM_CLASS(name, type)
	%nspace la::avdecc::entity::model::name;
	%typemap(csbase) la::avdecc::entity::model::name type
	%rename("isEqual") la::avdecc::entity::model::operator==(name const, name const); // Not put in a namespace https://github.com/swig/swig/issues/2459
	%rename("$ignore") la::avdecc::entity::model::operator==(name const, std::underlying_type_t<name> const);
%enddef
%define DEFINE_AEM_TYPES_STRUCT(name)
	%nspace la::avdecc::entity::model::name;
	%rename("%s") la::avdecc::entity::model::name; // Unignore class
	%rename("isEqual") operator==(name const& lhs, name const& rhs) noexcept; // Not put in a namespace https://github.com/swig/swig/issues/2459
	%rename("isDifferent") operator!=(name const& lhs, name const& rhs) noexcept; // Not put in a namespace https://github.com/swig/swig/issues/2459
	// Extend the class
	%extend la::avdecc::entity::model::name
	{
#if defined(SWIGCSHARP)
		// Provide a more native Equals() method
		bool Equals(la::avdecc::entity::model::name const& other) const noexcept
		{
			return *$self == other;
		}
#endif
	}
%enddef
%define DEFINE_AEM_TYPES_CLASS_BASE(name)
	%nspace la::avdecc::entity::model::name;
	%rename("%s") la::avdecc::entity::model::name; // Unignore class
	%ignore la::avdecc::entity::model::name::name(name&&); // Ignore move constructor
	%ignore la::avdecc::entity::model::name::operator=; // Ignore copy operator
%enddef
%define DEFINE_AEM_TYPES_CLASS(name)
	DEFINE_AEM_TYPES_CLASS_BASE(name)
	%ignore la::avdecc::entity::model::name::operator value_type() const noexcept;
	%ignore la::avdecc::entity::model::name::operator bool() const noexcept;
	%rename("isEqual") operator==(name const& lhs, name const& rhs) noexcept; // Not put in a namespace https://github.com/swig/swig/issues/2459
	%rename("isDifferent") operator!=(name const& lhs, name const& rhs) noexcept; // Not put in a namespace https://github.com/swig/swig/issues/2459
	%rename("isLess") operator<(name const& lhs, name const& rhs) noexcept; // Not put in a namespace https://github.com/swig/swig/issues/2459
	// Extend the class
	%extend la::avdecc::entity::model::name
	{
#if defined(SWIGCSHARP)
		// Provide a more native Equals() method
		bool Equals(la::avdecc::entity::model::name const& other) const noexcept
		{
			return *$self == other;
		}
#endif
	}
%enddef

// Bind enums
DEFINE_AEM_TYPES_ENUM_CLASS(AudioClusterFormat, "byte")
DEFINE_AEM_TYPES_ENUM_CLASS(ClockSourceType, "ushort")
DEFINE_AEM_TYPES_ENUM_CLASS(DescriptorType, "ushort")
%rename("isValid") la::avdecc::entity::model::operator!(DescriptorType const); // Not put in a namespace https://github.com/swig/swig/issues/2459
DEFINE_AEM_TYPES_ENUM_CLASS(JackType, "ushort")
DEFINE_AEM_TYPES_ENUM_CLASS(MemoryObjectOperationType, "ushort")
DEFINE_AEM_TYPES_ENUM_CLASS(MemoryObjectType, "ushort")
DEFINE_AEM_TYPES_ENUM_CLASS(StandardControlType, "ulong")
DEFINE_AEM_TYPES_ENUM_CLASS(ProbingStatus, "byte")
DEFINE_AEM_TYPES_ENUM_CLASS(MsrpFailureCode, "byte")

// Bind structs and classes
%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable

DEFINE_AEM_TYPES_STRUCT(AudioMapping);
DEFINE_AEM_TYPES_STRUCT(MsrpMapping);
DEFINE_AEM_TYPES_STRUCT(StreamIdentification);
%rename("isLess") la::avdecc::entity::model::operator<(StreamIdentification const&, StreamIdentification const&); // Not put in a namespace https://github.com/swig/swig/issues/2459

DEFINE_AEM_TYPES_CLASS_BASE(AvdeccFixedString);
%ignore la::avdecc::entity::model::AvdeccFixedString::data(); // RIGHT NOW IGNORE IT AS WE NEED TO FIND A WAY TO MARSHALL THE RETURNED POINTER
%ignore la::avdecc::entity::model::AvdeccFixedString::data() const; // RIGHT NOW IGNORE IT AS WE NEED TO FIND A WAY TO MARSHALL THE RETURNED POINTER
%ignore la::avdecc::entity::model::AvdeccFixedString::AvdeccFixedString(void const* const ptr, size_t const size) noexcept; // RIGHT NOW IGNORE IT AS WE NEED TO FIND A WAY TO MARSHALL THE RETURNED POINTER
%ignore la::avdecc::entity::model::AvdeccFixedString::assign(void const* const ptr, size_t const size) noexcept; // RIGHT NOW IGNORE IT AS WE NEED TO FIND A WAY TO MARSHALL THE RETURNED POINTER
%rename("isEqual") la::avdecc::entity::model::AvdeccFixedString::operator==;
%rename("isDifferent") la::avdecc::entity::model::AvdeccFixedString::operator!=;
%rename("toString") la::avdecc::entity::model::AvdeccFixedString::operator std::string;
%ignore la::avdecc::entity::model::AvdeccFixedString::operator[](size_t const pos);
%ignore la::avdecc::entity::model::AvdeccFixedString::operator[](size_t const pos) const;
%ignore operator<<(std::ostream&, la::avdecc::entity::model::AvdeccFixedString const&);
// Extend the class
%extend la::avdecc::entity::model::AvdeccFixedString
{
#if defined(SWIGCSHARP)
	// Provide a more native ToString() method
	std::string ToString()
	{
		return static_cast<std::string>(*$self);
	}
	// Provide a more native Equals() method
	bool Equals(la::avdecc::entity::model::AvdeccFixedString const& other) const noexcept
	{
		return *$self == other;
	}
#endif
}
DEFINE_AEM_TYPES_CLASS(SamplingRate);
DEFINE_AEM_TYPES_CLASS(StreamFormat);
DEFINE_AEM_TYPES_CLASS(LocalizedStringReference);
DEFINE_AEM_TYPES_CLASS(ControlValueUnit);
%typemap(csbase) la::avdecc::entity::model::ControlValueUnit::Unit "byte"
DEFINE_AEM_TYPES_CLASS(ControlValueType);
%typemap(csbase) la::avdecc::entity::model::ControlValueType::Type "ushort"
DEFINE_AEM_TYPES_CLASS_BASE(ControlValues);
%ignore la::avdecc::entity::model::ControlValues::operator bool() const noexcept;

// Include c++ declaration file
%include "la/avdecc/internals/entityModelTypes.hpp"
%rename("%s", %$isclass) ""; // Undo the ignore all structs/classes

// Define templates
%template(MultiplierUnit) std::pair<std::int8_t, la::avdecc::entity::model::ControlValueUnit::Unit>;
%template(PullBaseFrequency) std::pair<std::uint8_t, std::uint32_t>;
%template(OffsetIndex) std::pair<std::uint16_t, std::uint8_t>;
%template(AudioMappingVector) std::vector<la::avdecc::entity::model::AudioMapping>;
%template(MsrpMappingVector) std::vector<la::avdecc::entity::model::MsrpMapping>;
%template(UniqueIdentifierVector) std::vector<la::avdecc::UniqueIdentifier>;
%template(DescriptorCounterArray) std::array<la::avdecc::entity::model::DescriptorCounter, 32>;


////////////////////////////////////////
// Entity Enums
////////////////////////////////////////
// Bind enums
DEFINE_ENUM_CLASS(la::avdecc::entity, EntityCapability, "uint")
DEFINE_ENUM_CLASS(la::avdecc::entity, TalkerCapability, "ushort")
DEFINE_ENUM_CLASS(la::avdecc::entity, ListenerCapability, "ushort")
DEFINE_ENUM_CLASS(la::avdecc::entity, ControllerCapability, "uint")
DEFINE_ENUM_CLASS(la::avdecc::entity, ConnectionFlag, "ushort")
DEFINE_ENUM_CLASS(la::avdecc::entity, StreamFlag, "ushort")
DEFINE_ENUM_CLASS(la::avdecc::entity, JackFlag, "ushort")
DEFINE_ENUM_CLASS(la::avdecc::entity, AvbInterfaceFlag, "ushort")
DEFINE_ENUM_CLASS(la::avdecc::entity, ClockSourceFlag, "ushort")
DEFINE_ENUM_CLASS(la::avdecc::entity, PortFlag, "ushort")
DEFINE_ENUM_CLASS(la::avdecc::entity, StreamInfoFlag, "uint")
DEFINE_ENUM_CLASS(la::avdecc::entity, StreamInfoFlagEx, "uint")
DEFINE_ENUM_CLASS(la::avdecc::entity, AvbInfoFlag, "byte")
DEFINE_ENUM_CLASS(la::avdecc::entity, EntityCounterValidFlag, "uint")
DEFINE_ENUM_CLASS(la::avdecc::entity, AvbInterfaceCounterValidFlag, "uint")
DEFINE_ENUM_CLASS(la::avdecc::entity, ClockDomainCounterValidFlag, "uint")
DEFINE_ENUM_CLASS(la::avdecc::entity, StreamInputCounterValidFlag, "uint")
DEFINE_ENUM_CLASS(la::avdecc::entity, StreamOutputCounterValidFlag, "uint")
DEFINE_ENUM_CLASS(la::avdecc::entity, MilanInfoFeaturesFlag, "uint")

// Bind structs and classes
%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable

// Include c++ declaration file
%include "la/avdecc/internals/entityEnums.hpp"
%rename("%s", %$isclass) ""; // Undo the ignore all structs/classes

// Define templates
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, EntityCapabilities, EntityCapability, std::uint32_t)
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, TalkerCapabilities, TalkerCapability, std::uint16_t)
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, ListenerCapabilities, ListenerCapability, std::uint16_t)
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, ControllerCapabilities, ControllerCapability, std::uint32_t)
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, ConnectionFlags, ConnectionFlag, std::uint16_t)
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, StreamFlags, StreamFlag, std::uint16_t)
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, JackFlags, JackFlag, std::uint16_t)
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, AvbInterfaceFlags, AvbInterfaceFlag, std::uint16_t)
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, ClockSourceFlags, ClockSourceFlag, std::uint16_t)
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, PortFlags, PortFlag, std::uint16_t)
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, StreamInfoFlags, StreamInfoFlag, std::uint32_t)
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, StreamInfoFlagsEx, StreamInfoFlagEx, std::uint32_t)
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, AvbInfoFlags, AvbInfoFlag, std::uint8_t)
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, EntityCounterValidFlags, EntityCounterValidFlag, std::uint32_t)
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, AvbInterfaceCounterValidFlags, AvbInterfaceCounterValidFlag, std::uint32_t)
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, ClockDomainCounterValidFlags, ClockDomainCounterValidFlag, std::uint32_t)
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, StreamInputCounterValidFlags, StreamInputCounterValidFlag, std::uint32_t)
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, StreamOutputCounterValidFlags, StreamOutputCounterValidFlag, std::uint32_t)
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::entity, MilanInfoFeaturesFlags, MilanInfoFeaturesFlag, std::uint32_t)


////////////////////////////////////////
// Protocol Defines
////////////////////////////////////////
%define DEFINE_BASE_PROTOCOL_CLASS(name)
	%nspace la::avdecc::protocol::name;
	%rename("%s") la::avdecc::protocol::name; // Unignore class
	%ignore la::avdecc::protocol::name::name(); // Ignore default constructor
	%rename("toString") la::avdecc::protocol::name::operator std::string() const noexcept;
#if defined(SWIGCSHARP)
	// Better debug display
	%typemap(csattributes) la::avdecc::protocol::name "[System.Diagnostics.DebuggerDisplay(\"{toString()}\")]"
#endif
	// Extend the class
	%extend la::avdecc::protocol::name
	{
#if defined(SWIGCSHARP)
		// Provide a more native ToString() method
		std::string ToString()
		{
			return static_cast<std::string>(*$self);
		}
#endif
	}
%enddef
%define DEFINE_TYPED_PROTOCOL_CLASS(name, typedName, underlyingType)
	DEFINE_BASE_PROTOCOL_CLASS(name)
	// Define the parent TypedDefine class (this template must be declare before including the protocolDefines.hpp file, TypedDefine has already been declared anyway)
	%template(typedName) la::avdecc::utils::TypedDefine<la::avdecc::protocol::name, underlyingType>;
%enddef

// Bind structs and classes
%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable

// TODO: Would be easier to map these types to the underlying integer type (but how to do it?)
DEFINE_TYPED_PROTOCOL_CLASS(AdpMessageType, AdpMessageTypedDefine, std::uint8_t)
DEFINE_TYPED_PROTOCOL_CLASS(AecpMessageType, AecpMessageTypedDefine, std::uint8_t)
DEFINE_TYPED_PROTOCOL_CLASS(AecpStatus, AecpStatusTypedDefine, std::uint8_t)
DEFINE_BASE_PROTOCOL_CLASS(AemAecpStatus)
%ignore la::avdecc::protocol::AemAecpStatus::AemAecpStatus(AecpStatus const status) noexcept; // Ignore constructor
DEFINE_TYPED_PROTOCOL_CLASS(AemCommandType, AemCommandTypedDefine, std::uint16_t)
DEFINE_TYPED_PROTOCOL_CLASS(AemAcquireEntityFlags, AemAcquireEntityFlagsTypedDefine, std::uint32_t)
DEFINE_TYPED_PROTOCOL_CLASS(AemLockEntityFlags, AemLockEntityFlagsTypedDefine, std::uint32_t)
DEFINE_TYPED_PROTOCOL_CLASS(AaMode, AaModeTypedDefine, std::uint8_t)
DEFINE_BASE_PROTOCOL_CLASS(AaAecpStatus)
DEFINE_BASE_PROTOCOL_CLASS(MvuAecpStatus)
DEFINE_TYPED_PROTOCOL_CLASS(MvuCommandType, MvuCommandTypedDefine, std::uint16_t)
DEFINE_TYPED_PROTOCOL_CLASS(AcmpMessageType, AcmpMessageTypedDefine, std::uint8_t)
DEFINE_TYPED_PROTOCOL_CLASS(AcmpStatus, AcmpStatusTypedDefine, std::uint8_t)

// Include c++ declaration file
%include "la/avdecc/internals/protocolDefines.hpp"
%rename("%s", %$isclass) ""; // Undo the ignore all structs/classes


////////////////////////////////////////
// Entity Model
////////////////////////////////////////
// We need to import NetworkInterfaces
%import "la/networkInterfaceHelper/networkInterfaceHelper.i"

// Define some macros
%define DEFINE_AEM_DESCRIPTOR(name)
	%nspace la::avdecc::entity::model::name;
	%rename("%s") la::avdecc::entity::model::name; // Unignore class
%enddef
%define DEFINE_AEM_STRUCT(name)
	%nspace la::avdecc::entity::model::name;
	%rename("%s") la::avdecc::entity::model::name; // Unignore class
	%rename("isEqual") operator==(name const& lhs, name const& rhs) noexcept; // Not put in a namespace https://github.com/swig/swig/issues/2459
	%rename("isDifferent") operator!=(name const& lhs, name const& rhs) noexcept; // Not put in a namespace https://github.com/swig/swig/issues/2459
	// Extend the class
	%extend la::avdecc::entity::model::name
	{
#if defined(SWIGCSHARP)
		// Provide a more native Equals() method
		bool Equals(la::avdecc::entity::model::name const& other) const noexcept
		{
			return *$self == other;
		}
#endif
	}
%enddef

// Define optionals
DEFINE_OPTIONAL_SIMPLE(OptProbingStatus, la::avdecc::entity::model::ProbingStatus, la.avdecc.entity.model.ProbingStatus.Disabled)
DEFINE_OPTIONAL_CLASS(la::avdecc::entity, StreamInfoFlagsEx, OptStreamInfoFlagsEx)
DEFINE_OPTIONAL_CLASS(la::avdecc::protocol, AcmpStatus, OptAcmpStatus)

// Bind structs and classes
%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable

DEFINE_AEM_DESCRIPTOR(EntityDescriptor);
DEFINE_AEM_DESCRIPTOR(ConfigurationDescriptor);
DEFINE_AEM_DESCRIPTOR(AudioUnitDescriptor);
DEFINE_AEM_DESCRIPTOR(StreamDescriptor);
DEFINE_AEM_DESCRIPTOR(JackDescriptor);
DEFINE_AEM_DESCRIPTOR(AvbInterfaceDescriptor);
DEFINE_AEM_DESCRIPTOR(ClockSourceDescriptor);
DEFINE_AEM_DESCRIPTOR(MemoryObjectDescriptor);
DEFINE_AEM_DESCRIPTOR(LocaleDescriptor);
DEFINE_AEM_DESCRIPTOR(StringsDescriptor);
DEFINE_AEM_DESCRIPTOR(StreamPortDescriptor);
DEFINE_AEM_DESCRIPTOR(ExternalPortDescriptor);
DEFINE_AEM_DESCRIPTOR(InternalPortDescriptor);
DEFINE_AEM_DESCRIPTOR(AudioClusterDescriptor);
DEFINE_AEM_DESCRIPTOR(AudioMapDescriptor);
DEFINE_AEM_DESCRIPTOR(ControlDescriptor);
DEFINE_AEM_DESCRIPTOR(ClockDomainDescriptor);
DEFINE_AEM_STRUCT(StreamInfo);
DEFINE_AEM_STRUCT(AvbInfo);
DEFINE_AEM_STRUCT(AsPath);
DEFINE_AEM_STRUCT(MilanInfo);

// Some ignores
%ignore la::avdecc::entity::model::makeEntityModelID; // Ignore, not needed
%ignore la::avdecc::entity::model::splitEntityModelID; // Ignore, not needed

// Include c++ declaration file
%include "la/avdecc/internals/entityModel.hpp"
%rename("%s", %$isclass) ""; // Undo the ignore all structs/classes

// Define templates
%template(DescriptorCountMap) std::unordered_map<la::avdecc::entity::model::DescriptorType, std::uint16_t, la::avdecc::utils::EnumClassHash>;
%template(StringArray) std::array<la::avdecc::entity::model::AvdeccFixedString, 7>;
SWIG_STD_VECTOR_ENHANCED(la::avdecc::entity::model::DescriptorIndex); // Swig is struggling with DescriptorIndex alias (it's a std::uint16_t)
%template(DescriptorVector) std::vector<la::avdecc::entity::model::DescriptorIndex>;
%template(SamplingRateSet) std::set<la::avdecc::entity::model::SamplingRate>;
%template(StreamFormatSet) std::set<la::avdecc::entity::model::StreamFormat>;
%template(RedundantStreamIndexSet) std::set<la::avdecc::entity::model::StreamIndex>;