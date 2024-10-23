////////////////////////////////////////
// AVDECC MEMORY BUFFER SWIG file
////////////////////////////////////////

%include <stdint.i>
%include <std_string.i>
%include <std_vector.i>

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
%ignore la::avdecc::MemoryBuffer::data() const; // Ignore const data getter, always return the non-const version
// Extend the class
%extend la::avdecc::MemoryBuffer
{
#if defined(SWIGCSHARP)
	// Provide a more native Equals() method
	bool Equals(la::avdecc::MemoryBuffer const& other) const noexcept
	{
		return *$self == other;
	}
#endif
}

%nspace la::avdecc::MemoryBufferView;
%rename("isEqual") la::avdecc::MemoryBufferView::operator==;
%rename("isDifferent") la::avdecc::MemoryBufferView::operator!=;
%ignore la::avdecc::MemoryBufferView::operator bool; // Ignore operator bool, isValid() is already defined
// Currently no resolution is performed in order to match function parameters. This means function parameter types must match exactly. For example, namespace qualifiers and typedefs will not work.
%ignore la::avdecc::MemoryBufferView::operator=;
// Ignore move constructor
%ignore la::avdecc::MemoryBufferView::MemoryBufferView(MemoryBufferView&&);
%ignore la::avdecc::MemoryBufferView::data() const; // Ignore const data getter, always return the non-const version
// Extend the class
%extend la::avdecc::MemoryBufferView
{
#if defined(SWIGCSHARP)
	// Provide a more native Equals() method
	bool Equals(la::avdecc::MemoryBufferView const& other) const noexcept
	{
		return *$self == other;
	}
#endif
}

#ifdef SWIGCSHARP
// Marshalling for void pointers (as input parameters)
%apply unsigned char INPUT[]  { void const* const }
// Marshalling for uint8 pointers as function return value
%apply void* VOID_INT_PTR { std::uint8_t* }
#endif

// Include c++ declaration file
%include "la/avdecc/memoryBuffer.hpp"

#ifdef SWIGCSHARP
// Clear marshalling for void pointers
%clear void const* const;
// Clear marshalling for uint8 pointers
%clear std::uint8_t*;
#endif
