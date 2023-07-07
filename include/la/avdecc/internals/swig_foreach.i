#ifndef SWIG_FOREACH_H
#define SWIG_FOREACH_H

// Important: the %foreach* macros can not handle zero argument list
//            see issue https://github.com/swig/swig/issues/1574

// Common unpack actions for the %foreach macro
#define %unpack_param(num,type)   $typemap(cstype,type) arg ## num
#define %unpack_type(num,type)    $typemap(cstype,type)
#define %unpack_arg(num,type)     arg##num

// Following unwind helper macros for different count of arguments
// Note: we can not unwind zero parameters

// Comma separated defines
#define %unwind_arg_action_1(action,a1) action(0,a1)
#define %unwind_arg_action_2(action,a1,a2) action(0,a1), action(1,a2)
#define %unwind_arg_action_3(action,a1,a2,a3) action(0,a1), action(1,a2), action(2,a3)
#define %unwind_arg_action_4(action,a1,a2,a3,a4) action(0,a1), action(1,a2), action(2,a3), action(3,a4)
#define %unwind_arg_action_5(action,a1,a2,a3,a4,a5) action(0,a1), action(1,a2), action(2,a3), action(3,a4), action(4,a5)
#define %unwind_arg_action_6(action,a1,a2,a3,a4,a5,a6) action(0,a1), action(1,a2), action(2,a3), action(3,a4), action(4,a5), action(5,a6)
#define %unwind_arg_action_7(action,a1,a2,a3,a4,a5,a6,a7) action(0,a1), action(1,a2), action(2,a3), action(3,a4), action(4,a5), action(5,a6), action(6,a7)
#define %unwind_arg_action_8(action,a1,a2,a3,a4,a5,a6,a7,a8) action(0,a1), action(1,a2), action(2,a3), action(3,a4), action(4,a5), action(5,a6), action(6,a7), action(7,a8)
#define %unwind_arg_action_9(action,a1,a2,a3,a4,a5,a6,a7,a8,a9) action(0,a1), action(1,a2), action(2,a3), action(3,a4), action(4,a5), action(5,a6), action(6,a7), action(7,a8), action(8,a9)

// Raw defines without comma
#define %unwind_raw_action_1(action,a1) action(0,a1)
#define %unwind_raw_action_2(action,a1,a2) action(0,a1) action(1,a2)
#define %unwind_raw_action_3(action,a1,a2,a3) action(0,a1) action(1,a2) action(2,a3)
#define %unwind_raw_action_4(action,a1,a2,a3,a4) action(0,a1) action(1,a2) action(2,a3) action(3,a4)
#define %unwind_raw_action_5(action,a1,a2,a3,a4,a5) action(0,a1) action(1,a2) action(2,a3) action(3,a4) action(4,a5)
#define %unwind_raw_action_6(action,a1,a2,a3,a4,a5,a6) action(0,a1) action(1,a2) action(2,a3) action(3,a4) action(4,a5) action(5,a6)
#define %unwind_raw_action_7(action,a1,a2,a3,a4,a5,a6,a7) action(0,a1) action(1,a2) action(2,a3) action(3,a4) action(4,a5) action(5,a6) action(6,a7)
#define %unwind_raw_action_8(action,a1,a2,a3,a4,a5,a6,a7,a8) action(0,a1) action(1,a2) action(2,a3) action(3,a4) action(4,a5) action(5,a6) action(6,a7) action(7,a8)
#define %unwind_raw_action_9(action,a1,a2,a3,a4,a5,a6,a7,a8,a9) action(0,a1) action(1,a2) action(2,a3) action(3,a4) action(4,a5) action(5,a6) action(6,a7) action(7,a8) action(8,a9)

// Parameter index selector
#define %deduce_macro_internal(_1,_2,_3,_4,_5,_6,_7,_8,_9,NAME,...) NAME

// The %foreach macros itself which this module is all about
// Comma separated defines
%define %foreach(action,...) %deduce_macro_internal(__VA_ARGS__, %unwind_arg_action_9, %unwind_arg_action_8, %unwind_arg_action_7, %unwind_arg_action_6, %unwind_arg_action_5, %unwind_arg_action_4, %unwind_arg_action_3, %unwind_arg_action_2, %unwind_arg_action_1)(action,__VA_ARGS__) %enddef
// Raw defines without comma
%define %foreach_plain(action,...) %deduce_macro_internal(__VA_ARGS__, %unwind_raw_action_9, %unwind_raw_action_8, %unwind_raw_action_7, %unwind_raw_action_6, %unwind_raw_action_5, %unwind_raw_action_4, %unwind_raw_action_3, %unwind_raw_action_2, %unwind_raw_action_1)(action,__VA_ARGS__) %enddef


#endif /* SWIG_FOREACH_H */
