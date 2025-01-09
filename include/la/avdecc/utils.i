////////////////////////////////////////
// AVDECC UTILS SWIG file
////////////////////////////////////////

%include <stdint.i>
%include <swiginterface.i>

// Define a helper template to handle std::underlying_type_t
%inline %{
namespace la::avdecc::utils
{
template<typename T>
class UnderlyingType
{
public:
    using value_type = void;
};
}
%}

// Redefine a simplier version of some classes from la::avdecc::utils namespace (easier to parse for SWIG)
namespace la::avdecc::utils
{
template<typename EnumType>
class EnumBitfield final
{
public:
	using value_type = EnumType;
	using underlying_value_type = UnderlyingType<EnumType>::value_type;
	static constexpr size_t value_size = sizeof(underlying_value_type) * 8;

	EnumBitfield() noexcept = default;
	template<typename... Values>
	constexpr explicit EnumBitfield(value_type const value, Values const... values) noexcept;
	void assign(underlying_value_type const value) noexcept;
	constexpr bool test(value_type const flag) const noexcept;
	EnumBitfield& set(value_type const flag) noexcept;
	EnumBitfield& reset(value_type const flag) noexcept;
	void clear() noexcept;
	constexpr bool empty() const noexcept;
	constexpr size_t size() const noexcept;
	constexpr size_t count() const noexcept;
	constexpr underlying_value_type value() const noexcept;
	%rename("isEqual") operator==(EnumBitfield const other) const noexcept;
	constexpr bool operator==(EnumBitfield const other) const noexcept;
	%rename("isDifferent") operator!=(EnumBitfield const other) const noexcept;
	constexpr bool operator!=(EnumBitfield const other) const noexcept;
	%rename("orEqual") operator|=(EnumBitfield const other) noexcept;
	EnumBitfield& operator|=(EnumBitfield const other) noexcept;
	%rename("andEqual") operator&=(EnumBitfield const other) noexcept;
	EnumBitfield& operator&=(EnumBitfield const other) noexcept;
	constexpr value_type at(size_t const setPosition) const;
	constexpr size_t getBitSetPosition(value_type const value) const;
	static constexpr size_t getPosition(value_type const value);

private:
	underlying_value_type _value{};
};

// Extend the class
%extend EnumBitfield
{
#if defined(SWIGCSHARP)
	// Provide a more native Equals() method
	bool Equals(EnumBitfield const& other) const noexcept
	{
		return *$self == other;
	}
#endif
}

template<class Derived, typename DataType>
class TypedDefine
{
public:
	using derived_type = Derived;
	using value_type = DataType;

	explicit TypedDefine(value_type const value) noexcept
		: _value(value)
	{
	}

	value_type getValue() const noexcept
	{
		return _value;
	}

	void setValue(value_type const value) noexcept
	{
		_value = value;
	}

	%rename("isEqual") operator==(TypedDefine const& v) const noexcept;
	bool operator==(TypedDefine const& v) const noexcept
	{
		return v.getValue() == _value;
	}

	%rename("isDifferent") operator!=(TypedDefine const& v) const noexcept;
	bool operator!=(TypedDefine const& v) const noexcept
	{
		return !operator==(v);
	}

	// Defaulted compiler auto-generated methods
	%ignore TypedDefine(TypedDefine&&) noexcept; // Ignore move constructor
	TypedDefine(TypedDefine&&) = default;
	TypedDefine(TypedDefine const&) = default;
	%ignore operator=; // Ignore copy operator
	TypedDefine& operator=(TypedDefine const&) = default;
	TypedDefine& operator=(TypedDefine&&) = default;

private:
	value_type _value{};
};

// Extend the class
%extend TypedDefine
{
#if defined(SWIGCSHARP)
	// Provide a more native Equals() method
	bool Equals(TypedDefine const& other) const noexcept
	{
		return *$self == other;
	}
#endif
}

// Forward declare Subject template class
template<class Derived, class Mut>
class Subject;

template<class Observable>
class Observer
{
public:
	virtual ~Observer() noexcept = default;
};

template<class Derived, class Mut>
class Subject
{
public:
	using mutex_type = Mut;
	using observer_type = Observer<Derived>;

	void registerObserver(observer_type* const observer) const;
	void unregisterObserver(observer_type* const observer) const;
	%rename("_lock") lock() noexcept;
	void lock() noexcept;
	%rename("_unlock") unlock() noexcept;
	void unlock() noexcept;
	size_t countObservers() const noexcept;
	bool isObserverRegistered(observer_type* const observer) const noexcept;
};

} // namespace la::avdecc::utils


////////////////////////////////////////
// Define some useful macros
////////////////////////////////////////

%define DEFINE_ENUM_CLASS(nspacename, bitname, type)
	%nspace nspacename::bitname;
#if defined(SWIGCSHARP)
	%typemap(csbase, replace="1") nspacename::bitname "uint" // Currently hardcode as uint because of SWIG issue https://github.com/swig/swig/issues/2576
#else
	%typemap(csbase) nspacename::bitname type
#endif
%enddef

%define DEFINE_ENUM_BITFIELD_CLASS(nspacename, bitfieldname, bitname, underlyingtype)
	%inline %{
	namespace la::avdecc::utils
	{
	template<>
	class UnderlyingType<nspacename::bitname>
	{
	public:
	using value_type = underlyingtype;
	};
	}
	%}
	%apply underlyingtype { la::avdecc::utils::UnderlyingType<nspacename::bitname>::value_type };
	%template(bitfieldname) la::avdecc::utils::EnumBitfield<nspacename::bitname>;
%enddef

%define DEFINE_OBSERVER_CLASS(classname)
	%nspace classname;
	%rename("%s") classname; // Unignore class
	%feature("director") classname;
%enddef

%define DEFINE_INTERFACE_CLASS(classname)
	%nspace classname;
	%rename("%s") classname; // Unignore class
	%interface_impl(classname); // Define as interface
%enddef
