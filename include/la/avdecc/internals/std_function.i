// Adapted from https://stackoverflow.com/a/32668302

%{
	#include <functional>
#ifndef SWIG_DIRECTORS
#error "Directors must be enabled in your SWIG module for std_function.i to work correctly"
#endif
%}

// These are the things we actually use
#define %param(num,type) $typemap(cstype,type) arg ## num
#define %unpack_type(num,type) $typemap(cstype,type)
#define %unpack_arg(num,type) arg##num

// This is the mechanics
#define %unwind_arg_action_internal_0(...)
#define %unwind_arg_action_internal_1(action,a1) action(0,a1)
#define %unwind_arg_action_internal_2(action,a1,a2) action(0,a1), action(1,a2)
#define %unwind_arg_action_internal_3(action,a1,a2,a3) action(0,a1), action(1,a2), action(2,a3)
#define %unwind_arg_action_internal_4(action,a1,a2,a3,a4) action(0,a1), action(1,a2), action(2,a3), action(3,a4)
#define %unwind_arg_action_internal_5(action,a1,a2,a3,a4,a5) action(0,a1), action(1,a2), action(2,a3), action(3,a4), action(4,a5)
#define %unwind_arg_action_internal_6(action,a1,a2,a3,a4,a5,a6) action(0,a1), action(1,a2), action(2,a3), action(3,a4), action(4,a5), action(5,a6)
#define %unwind_arg_action_internal_7(action,a1,a2,a3,a4,a5,a6,a7) action(0,a1), action(1,a2), action(2,a3), action(3,a4), action(4,a5), action(5,a6), action(6,a7)
#define %unwind_arg_action_internal_8(action,a1,a2,a3,a4,a5,a6,a7,a8) action(0,a1), action(1,a2), action(2,a3), action(3,a4), action(4,a5), action(5,a6), action(6,a7), action(7,a8)
#define %unwind_arg_action_internal_9(action,a1,a2,a3,a4,a5,a6,a7,a8,a9) action(0,a1), action(1,a2), action(2,a3), action(3,a4), action(4,a5), action(5,a6), action(6,a7), action(7,a8), action(8,a9)

#define %deduce_macro_internal(_1,_2,_3,_4,_5,_6,_7,_8,_9,NAME,...) NAME
%define %foreach(action,...) %deduce_macro_internal(__VA_ARGS__, %unwind_arg_action_internal_9, %unwind_arg_action_internal_8, %unwind_arg_action_internal_7, %unwind_arg_action_internal_6, %unwind_arg_action_internal_5, %unwind_arg_action_internal_4, %unwind_arg_action_internal_3, %unwind_arg_action_internal_2, %unwind_arg_action_internal_1, %unwind_arg_action_internal_0)(action,__VA_ARGS__) %enddef


%define %std_function(Name, Ret, ...)

// Generate swig directed handler class
%feature("director") Name;

#if defined(SWIGCSHARP)
#if (#Ret == "void")
	%typemap(cscode) Name %{
		public class Action : Name {
			private readonly System.Action<%foreach(%unpack_type, __VA_ARGS__)> delegateAction;

			public Action(System.Action<%foreach(%unpack_type, __VA_ARGS__)> action) : base() {
				delegateAction = action;
			}

			public static implicit operator Action(System.Action<%foreach(%unpack_type, __VA_ARGS__)> action) => new Action(action);

			protected override void Invoke(%foreach(%param, __VA_ARGS__))
			{
				delegateAction?.Invoke(%foreach(%unpack_arg, __VA_ARGS__));
			}
		}

		public static implicit operator Name##Native(Name handler) => new Name##Native(handler);
	%}
#else
	%typemap(cscode) Name %{
		public class Action : Name {
			private readonly System.Func<%foreach(%unpack_type, __VA_ARGS__), %unpack_type(0, Ret)> delegateAction;

			public Action(System.Func<%foreach(%unpack_type, __VA_ARGS__), %unpack_type(0, Ret)> action) : base() {
				delegateAction = action;
			}

			public static implicit operator Action(System.Func<%foreach(%unpack_type, __VA_ARGS__), %unpack_type(0, Ret)> action) => new Action(action);

			protected override %unpack_type(0, Ret) Invoke(%foreach(%param, __VA_ARGS__))
			{
				return delegateAction?.Invoke(%foreach(%unpack_arg, __VA_ARGS__)) ?? throw new global::System.InvalidOperationException("Callback not assigned.");
			}
		}

		public static implicit operator Name##Native(Name handler) => new Name##Native(handler);
	%}
#endif

%typemap(csclassmodifiers) Name "public abstract class"
%csmethodmodifiers Name::Invoke "protected abstract";
%warnfilter(844) Name::Invoke;
%typemap(csout) Ret Name::Invoke ";"
#endif

%{
	struct Name {
		virtual ~Name() {}
		virtual Ret Invoke(__VA_ARGS__) = 0;
	};
%}

struct Name {
	virtual ~Name();
protected:
	virtual Ret Invoke(__VA_ARGS__) = 0;
};

// Create native std::function wrapper with a nice name
%rename(Name##Native) std::function<Ret(__VA_ARGS__)>;
%rename(Invoke) std::function<Ret(__VA_ARGS__)>::operator();

namespace std {
	struct function<Ret(__VA_ARGS__)> {
		// Copy constructor
		function<Ret(__VA_ARGS__)>(const std::function<Ret(__VA_ARGS__)>&);

		// Call operator
		Ret operator()(__VA_ARGS__) const;

		// Extension for directed forward handler
		%extend {
			function<Ret(__VA_ARGS__)>(Name *in) {
				return new std::function<Ret(__VA_ARGS__)>([=](auto&& ...param){
					return in->Invoke(std::forward<decltype(param)>(param)...);
				});
			}
		}
	};
}

%enddef
