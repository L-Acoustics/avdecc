////////////////////////////////////////
// AVDECC LOGGER SWIG file
////////////////////////////////////////

%include <stdint.i>
%include <std_string.i>
%include <std_vector.i>

// Bind structs and classes
%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable

////////////////////////////////////////
// Enum classes
////////////////////////////////////////
%nspace la::avdecc::logger::Layer;
%nspace la::avdecc::logger::Level;

////////////////////////////////////////
// LogItem class
////////////////////////////////////////
%nspace la::avdecc::logger::LogItem;
%rename("%s") la::avdecc::logger::LogItem; // Unignore class
%ignore la::avdecc::logger::LogItem::LogItem(LogItem&&); // Ignore move constructor
%ignore la::avdecc::logger::LogItem::operator=; // Ignore copy operator

////////////////////////////////////////
// Logger class
////////////////////////////////////////
DEFINE_OBSERVER_CLASS(la::avdecc::logger::Logger::Observer)

%nspace la::avdecc::logger::Logger;
%rename("%s") la::avdecc::logger::Logger; // Unignore class

// Include c++ declaration file
%include "la/avdecc/logger.hpp"
%rename("%s", %$isclass) ""; // Undo the ignore all structs/classes
