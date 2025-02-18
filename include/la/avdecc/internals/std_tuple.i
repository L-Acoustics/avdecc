
%{
  #include <tuple>
  #include <utility>
%}

%include "swig_foreach.i"


#define %std_tuple_ctor_unpack_arg(num,type) const type& arg##num
#define %std_tuple_make_getter(pos, type)    const type& get##pos() const { return std::get<pos>(*$self); }
#define %std_tuple_make_setter(pos, type)    void set##pos(const type& val) { std::get<pos>(*$self) = val; }

#define %std_tuple_cs_unpack_out_getter(pos, type)  arg##pos = get##pos();
#define %std_tuple_cs_unpack_getter(pos, type)      self.get##pos()


%define %std_tuple(Name, ...)

#if defined(SWIGCSHARP)
  %typemap(cscode) std::tuple<__VA_ARGS__> %{
    public void Deconstruct(%foreach(%unpack_param_out, __VA_ARGS__)) {
      %foreach_plain(%std_tuple_cs_unpack_out_getter, __VA_ARGS__)
    }

    public static implicit operator (%foreach(%unpack_type, __VA_ARGS__))(Name self) {
      return (%foreach(%std_tuple_cs_unpack_getter, __VA_ARGS__));
    }
  %}
#endif

%rename(Name) std::tuple<__VA_ARGS__>;

namespace std {
  struct tuple<__VA_ARGS__> {
    // Copy constructor
    tuple(%foreach(%std_tuple_ctor_unpack_arg, __VA_ARGS__));

    %extend {
      %foreach_plain(%std_tuple_make_getter, __VA_ARGS__)
      %foreach_plain(%std_tuple_make_setter, __VA_ARGS__)

      size_t Length() const {
        return std::tuple_size<std::decay_t<decltype(*$self)>>{};
      }
    }
  };
}

%enddef
