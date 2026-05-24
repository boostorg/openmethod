template<class Type, class Guide = Type&, typename = void>
struct static_st {
    using declspec = void; static Type st;
};

template<class Type, class Guide, typename Enable>
Type static_st<Type, Guide, Enable>::st ;

template<class Type, class Guide> struct static_st< Type, Guide, std::enable_if_t<std::is_same_v<get_declspec<Guide>, dllexport>>> {
    using declspec = dllexport; static __declspec(dllexport) Type st ;
};

template<class Type, class Guide>
Type static_st< Type, Guide, std::enable_if_t<std::is_same_v<get_declspec<Guide>, dllexport>>>::st;

template<class Type, class Guide> struct static_st< Type, Guide, std::enable_if_t<std::is_same_v<get_declspec<Guide>, dllimport>>> { using declspec = dllimport; static __declspec(dllimport) Type st; };
