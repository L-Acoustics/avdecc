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
// Rename const data() getter
//%rename("constData") la::avdecc::MemoryBuffer::data() const; // RIGHT NOW IGNORE IT AS WE NEED TO FIND A WAY TO MARSHALL THE RETURNED POINTER
%ignore la::avdecc::MemoryBuffer::data(); // RIGHT NOW IGNORE IT AS WE NEED TO FIND A WAY TO MARSHALL THE RETURNED POINTER
%ignore la::avdecc::MemoryBuffer::data() const; // RIGHT NOW IGNORE IT AS WE NEED TO FIND A WAY TO MARSHALL THE RETURNED POINTER
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
// Rename const data() getter
//%rename("constData") la::avdecc::MemoryBufferView::data() const; // RIGHT NOW IGNORE IT AS WE NEED TO FIND A WAY TO MARSHALL THE RETURNED POINTER
%ignore la::avdecc::MemoryBufferView::data(); // RIGHT NOW IGNORE IT AS WE NEED TO FIND A WAY TO MARSHALL THE RETURNED POINTER
%ignore la::avdecc::MemoryBufferView::data() const; // RIGHT NOW IGNORE IT AS WE NEED TO FIND A WAY TO MARSHALL THE RETURNED POINTER
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
// Marshalling for void pointers
%apply unsigned char INPUT[]  { void const* const }
#endif

// Include c++ declaration file
%include "la/avdecc/memoryBuffer.hpp"


