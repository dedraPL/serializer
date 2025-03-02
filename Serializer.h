/*!
 * @file Serializer.h
 * @author Bartłomiej Drozd
 *
 */
#pragma once

#include <nlohmann/json.hpp>
#include <variant>

#include "MemberStorage.h"

#define SERIALIZER_ADD_MEMBER(Class, Member) \
    Class::registerMember(&Class::Member, #Member);

template <typename T>
struct is_variant : std::false_type {
};

template <typename... Args>
struct is_variant<std::variant<Args...>> : std::true_type {
};

template <typename T>
inline constexpr bool is_variant_v = is_variant<T>::value;

template <typename T>
struct is_shared_ptr : std::false_type {
};

template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {
};

template <typename T>
inline constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

class SerializerException : public std::runtime_error {
   private:
    std::string _msg;

   public:
    SerializerException(const char* msg) throw()
        : std::runtime_error(msg), _msg(msg) {
    }
    virtual char const* what() const throw() { return _msg.c_str(); }
};

template <typename T>
struct Serializer : public MemberStorage<Serializer<T>, std::string> {
   public:
    //! @brief simple add data member to json file
    //! @tparam _MemberType data member type
    //! @tparam _TargetType specify if underlying type is other than container
    //! @tparam _ClassType class type owning data member pointer
    //! @param target target json structure
    //! @param arg source data member pointer
    template <typename _MemberType, typename _TargetType = _MemberType,
              typename _ClassType>
    inline void to_json_simple(nlohmann::json& target,
                               const _MemberType _ClassType::* arg) const {
        Serializer<T>::to_json_simple<_MemberType, _TargetType, _ClassType>(
            target, arg, dynamic_cast<const _ClassType&>(*this));
    }

    //! @brief simple read value from json into data member pointer
    //! @tparam _MemberType data member type
    //! @tparam _TargetType specify if underlying type is other than container
    //! @tparam _ClassType class type owning data member pointer
    //! @param source source json structure
    //! @param arg target data member pointer
    template <typename _MemberType, typename _TargetType = _MemberType,
              typename _ClassType>
    inline void from_json_simple(const nlohmann::json& source,
                                 _MemberType _ClassType::* arg) {
        Serializer<T>::from_json_simple<_MemberType, _TargetType, _ClassType>(
            source, arg, dynamic_cast<_ClassType&>(*this));
    }

    //! @brief add add source json struct under data member pointer name into target json struct
    //! @tparam _MemberType data member type
    //! @tparam _ClassType class type owning data member pointer
    //! @param target target json where new property is going to be added
    //! @param arg data member pointer used to obtain property name
    //! @param source new json property to be added to target
    template <typename _MemberType, typename _ClassType>
    inline void add_json_to_property(nlohmann::json& target,
                                     _MemberType _ClassType::* arg,
                                     const nlohmann::json& source) const {
        try {
            auto& name = Serializer::get(arg);
            target[name] = source;
        } catch (std::exception& e) {
            handleException(e);
        }
    }

    //! @brief from source returns json structure for data member pointer's associated name
    //! @tparam _MemberType data member type
    //! @tparam _ClassType class type owning data member pointer
    //! @param source source json struct
    //! @param arg data member pointer used to obtain property name
    //! @return extracted json structure for data member pointer's property name
    template <typename _MemberType, typename _ClassType>
    inline nlohmann::json get_json_for_property(const nlohmann::json& source,
                                                _MemberType _ClassType::* arg) {
        try {
            auto& name = Serializer::get(arg);
            return source.at(name);
        } catch (std::exception& e) {
            handleException(e);
            return nlohmann::json();
        }
    }

    //! @brief simple add data member to json file
    //! @tparam _MemberType data member type
    //! @tparam _TargetType specify if underlying type is other than container
    //! @tparam _ClassType class type owning data member pointer
    //! @param target target json structure
    //! @param arg source data member pointer
    //! @param instance
    template <typename _MemberType, typename _TargetType = _MemberType,
              typename _ClassType>
    static void to_json_simple(nlohmann::json& target,
                               const _MemberType _ClassType::* arg,
                               const _ClassType& instance) {
        try {
            auto& name = Serializer::get(arg);
            target[name] = getMemberRef<_TargetType>(arg, instance);
        } catch (std::exception& e) {
            handleException(e);
        }
    }

    //! @brief simple read value from json into data member pointer
    //! @tparam _MemberType data member type
    //! @tparam _TargetType specify if underlying type is other than container
    //! @tparam _ClassType class type owning data member pointer
    //! @param source source json structure
    //! @param arg target data member pointer
    //! @param instance
    template <typename _MemberType, typename _TargetType = _MemberType,
              typename _ClassType>
    static void from_json_simple(const nlohmann::json& source,
                                 _MemberType _ClassType::* arg,
                                 _ClassType& instance) {
        try {
            auto& name = Serializer::get(arg);
            source.at(name).get_to(getMemberRef<_TargetType>(arg, instance));
        } catch (std::exception& e) {
            handleException(e);
        }
    }

   public:
    friend void to_json(nlohmann::json& j, const Serializer<T>& p) {
        p.to_json(j);
    }
    friend void from_json(const nlohmann::json& j, Serializer<T>& p) {
        p.from_json(j);
    }

   private:
    template <typename _TargetType, typename _ClassType, typename _MemberType>
    static const _TargetType& getMemberRef(const _MemberType _ClassType::* arg,
                                           const _ClassType& instance) {
        if constexpr (is_variant_v<_MemberType>)
            return std::get<_TargetType>(instance.*arg);
        else if constexpr (is_shared_ptr_v<_MemberType>)
            return *(instance.*arg).get();
        else
            return instance.*arg;
    }

    template <typename _TargetType, typename _ClassType, typename _MemberType>
    static _TargetType& getMemberRef(_MemberType _ClassType::* arg,
                                     _ClassType& instance) {
        if constexpr (is_variant_v<_MemberType>)
            return std::get<_TargetType>(instance.*arg);
        else if constexpr (is_shared_ptr_v<_MemberType>)
            return *(instance.*arg).get();
        else
            return instance.*arg;
    }

    static void handleException(std::exception& e) {
#ifdef DEBUG
        std::cout << "[SERIALIZER] " << e.what() << std::endl;
#endif  // DEBUG
        // throw SerializerException(e.what());
    }

   protected:
    virtual inline void to_json(nlohmann::json& j) const = 0;
    virtual inline void from_json(const nlohmann::json& j) = 0;
};
