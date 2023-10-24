// Adapted from https://stackoverflow.com/a/32668302

%{
  #include <functional>
#ifndef SWIG_DIRECTORS
#error "Directors must be enabled in your SWIG module for std_function.i to work correctly"
#endif
%}

%include "swig_foreach.i"


%define %std_function(Name, Ret, ...)

// Generate swig directed handler class
%feature("director") Name;

#if defined(SWIGCSHARP)
%csmethodmodifiers Name::Invoke "protected abstract";
%warnfilter(844) Name::Invoke;
%typemap(csout) Ret Name::Invoke ";"

%typemap(csin) std::function<Ret(##__VA_ARGS__)> *, std::function<Ret(##__VA_ARGS__)> &, std::function<Ret(##__VA_ARGS__)> [] "$csclassname.getCPtr(new Name.Action($csinput))"
%typemap(csin) std::function<Ret(##__VA_ARGS__)> && "$csclassname.swigRelease(new Name.Action($csinput))"

#if (#Ret == "void")
  #if ((#__VA_ARGS__ == "") || (#__VA_ARGS__ == "void"))
    %typemap(cstype) std::function<Ret(##__VA_ARGS__)> *, std::function<Ret(##__VA_ARGS__)> &, std::function<Ret(##__VA_ARGS__)> [], std::function<Ret(##__VA_ARGS__)> && %{System.Action%}
    %typemap(csclassmodifiers) Name %{
    using DelegateSignature = System.Action;
    public abstract class%}

    %typemap(cscode) Name %{
      public class Action : Name {
        private readonly DelegateSignature _callback;

        public Action(DelegateSignature callback) : base() {
          _callback = callback;
        }

        public static implicit operator Action(DelegateSignature callback) => new Action(callback);

        protected override void Invoke()
        {
          _callback?.Invoke();
        }
      }

      public static implicit operator Name##Native(Name handler) => new Name##Native(handler);
    %}

    %typemap(cscode) std::function<Ret(##__VA_ARGS__)> %{
       public static implicit operator System.Action(Name##Native instance)
       {
          return new System.Action(() => {
            instance.Invoke();
          });
       }
    %}
  #else
    %typemap(cstype) std::function<Ret(##__VA_ARGS__)> *, std::function<Ret(##__VA_ARGS__)> &, std::function<Ret(##__VA_ARGS__)> [], std::function<Ret(##__VA_ARGS__)> && %{System.Action<%foreach(%unpack_type, __VA_ARGS__)>%}
    %typemap(csclassmodifiers) Name %{
    using DelegateSignature = System.Action<%foreach(%unpack_type, __VA_ARGS__)>;
    public abstract class%}

    %typemap(cscode) Name %{
      public class Action : Name {
        private readonly DelegateSignature _callback;

        public Action(DelegateSignature callback) : base() {
          _callback = callback;
        }

        public static implicit operator Action(DelegateSignature callback) => new Action(callback);

        protected override void Invoke(%foreach(%unpack_param, __VA_ARGS__))
        {
          _callback?.Invoke(%foreach(%unpack_arg, __VA_ARGS__));
        }
      }

      public static implicit operator Name##Native(Name handler) => new Name##Native(handler);
    %}

    %typemap(cscode) std::function<Ret(##__VA_ARGS__)> %{
       public static implicit operator System.Action<%foreach(%unpack_type, __VA_ARGS__)>(Name##Native instance)
       {
          return new System.Action<%foreach(%unpack_type, __VA_ARGS__)>((%foreach(%unpack_arg, __VA_ARGS__)) => {
            instance.Invoke(%foreach(%unpack_arg, __VA_ARGS__));
          });
       }
    %}
  #endif
#else
  #if ((#__VA_ARGS__ == "") ||  (#__VA_ARGS__ == "void"))
    %typemap(cstype) std::function<Ret(##__VA_ARGS__)> *, std::function<Ret(##__VA_ARGS__)> &, std::function<Ret(##__VA_ARGS__)> [], std::function<Ret(##__VA_ARGS__)> && %{System.Func<%unpack_type(0, Ret)>%}
    %typemap(csclassmodifiers) Name %{
    using DelegateSignature = System.Func<%unpack_type(0, Ret)>;
    public abstract class%}

    %typemap(cscode) Name %{
      public class Action : Name {
        private readonly DelegateSignature _callback;

        public Action(DelegateSignature callback) : base() {
          _callback = callback;
        }

        public static implicit operator Action(DelegateSignature callback) => new Action(callback);

        protected override %unpack_type(0, Ret) Invoke()
        {
          return _callback?.Invoke()
            ?? throw new global::System.InvalidOperationException("Callback not assigned.");
        }
      }

      public static implicit operator Name##Native(Name handler) => new Name##Native(handler);
    %}

    %typemap(cscode) std::function<Ret(##__VA_ARGS__)> %{
       public static implicit operator System.Func<%unpack_type(0, Ret)>(Name##Native instance)
       {
          return new System.Func<%unpack_type(0, Ret)>(() => {
            return instance.Invoke());
          });
       }
    %}
  #else
    %typemap(cstype) std::function<Ret(##__VA_ARGS__)> *, std::function<Ret(##__VA_ARGS__)> &, std::function<Ret(##__VA_ARGS__)> [], std::function<Ret(##__VA_ARGS__)> && %{System.Func<%foreach(%unpack_type, __VA_ARGS__), %unpack_type(0, Ret)>%}
    %typemap(csclassmodifiers) Name %{
    using DelegateSignature = System.Func<%foreach(%unpack_type, __VA_ARGS__), %unpack_type(0, Ret)>;
    public abstract class%}

    %typemap(cscode) Name %{
      public class Action : Name {
        private readonly DelegateSignature _callback;

        public Action(DelegateSignature callback) : base() {
          _callback = callback;
        }

        public static implicit operator Action(DelegateSignature callback) => new Action(callback);

        protected override %unpack_type(0, Ret) Invoke(%foreach(%unpack_param, __VA_ARGS__))
        {
          return _callback?.Invoke(%foreach(%unpack_arg, __VA_ARGS__))
            ?? throw new global::System.InvalidOperationException("Callback not assigned.");
        }
      }

      public static implicit operator Name##Native(Name handler) => new Name##Native(handler);
    %}

    %typemap(cscode) std::function<Ret(##__VA_ARGS__)> %{
       public static implicit operator System.Func<%foreach(%unpack_type, __VA_ARGS__), %unpack_type(0, Ret)>(Name##Native instance)
       {
          return new System.Func<%foreach(%unpack_type, __VA_ARGS__), %unpack_type(0, Ret)>((%foreach(%unpack_arg, __VA_ARGS__)) => {
            return instance.Invoke(%foreach(%unpack_arg, __VA_ARGS__));
          });
       }
    %}
  #endif
#endif

#endif

%{
  struct Name {
    virtual ~Name() {}
    virtual Ret Invoke(##__VA_ARGS__) = 0;
  };
%}

struct Name {
  virtual ~Name();
protected:
  virtual Ret Invoke(##__VA_ARGS__) = 0;
};

// Create native std::function wrapper with a nice name
%rename(Name##Native) std::function<Ret(##__VA_ARGS__)>;
%rename(Invoke) std::function<Ret(##__VA_ARGS__)>::operator();

namespace std {
  struct function<Ret(##__VA_ARGS__)> {
    // Copy constructor
    function<Ret(##__VA_ARGS__)>(const std::function<Ret(##__VA_ARGS__)>&);

    // Call operator
    Ret operator()(##__VA_ARGS__) const;

    // Extension for directed forward handler
    %extend {
      function<Ret(##__VA_ARGS__)>(Name *in) {
        return new std::function<Ret(##__VA_ARGS__)>([=](auto&& ...param){
          return in->Invoke(std::forward<decltype(param)>(param)...);
        });
      }
    }
  };
}

%enddef
