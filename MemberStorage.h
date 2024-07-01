/*!
 * @file MemberStorage.h
 * @author Bartłomiej Drozd
 *
 */
#pragma once

#include <unordered_map>
#include <mutex>
#include <stdexcept>
#include <typeinfo>
#include <algorithm>

struct MemberTypeBase
{
    MemberTypeBase(size_t hash) : class_hash(hash) {}
    virtual bool isEqual(const MemberTypeBase* rhs) const = 0;
    size_t class_hash;
};

template <typename _ClassType, typename _MemberType>
struct Member_t : MemberTypeBase
{
    Member_t(_MemberType _ClassType::*arg)
        : member(arg), MemberTypeBase(typeid(_ClassType).hash_code())
    {
    }
    _MemberType _ClassType::*member;

    bool isEqual(const MemberTypeBase* rhs) const override
    {
        if(class_hash == rhs->class_hash)
        {
            auto* ref =
                static_cast<const Member_t<_ClassType, _MemberType>*>(rhs);
            if (ref)
                return ref->member == member;
        }
        return false;
    }
};

//! @brief member storage base class used to register members. 
struct MemberStorageBase
{
public:
    //! @brief call this method in derived constructor
    //! @tparam T 
    //! @param  
    template <typename T>
    void callInitMembers(const T*)
    {
        static std::once_flag register_once;
        std::call_once(register_once, &MemberStorageBase::InitMembers, this);
    }

    virtual void InitMembers() = 0;

protected:
    class MemberHasher
    {
    public:
        size_t operator()(const MemberTypeBase* val) const
        {
            return reinterpret_cast<uintptr_t>(val);
        }
    };

    class MemberEqual
    {
    public:
        bool operator()(const MemberTypeBase* val1,
                        const MemberTypeBase* val2) const
        {
            return val1->isEqual(val2);
        }
    };
};

//! @brief simple storage class to associate StoredType value for every registered data member. Works like a singleton storage for T type
//! @tparam T 
//! @tparam StoredType 
template <typename T, typename StoredType>
struct MemberStorage : protected virtual MemberStorageBase
{
public:
    //! @brief returns stored value for provided data member pointer. Throws runtime_error if arg is not registered
    //! @tparam ClassType data member pointer's class owner
    //! @tparam MemberType data member pointer's type
    //! @param arg data member pointer
    //! @return stored value for provided arg
    template <typename ClassType, typename MemberType>
    static const StoredType& get(MemberType ClassType::*arg)
    {
        const auto& it = std::find_if(
            storage.cbegin(), storage.cend(),
            [arg](const std::pair<MemberTypeBase*, StoredType>& _it) -> bool {
                if (_it.first->class_hash == typeid(ClassType).hash_code())
                {
                    const auto* ref =
                        static_cast<const Member_t<ClassType, MemberType>*>(
                            _it.first);
                    if (ref)
                        return ref->member == arg;
                }
                return false;
            });
        if (it != storage.cend())
            return it->second;
        else
            throw std::runtime_error("No storage for " +
                                     std::string(typeid(ClassType).name()) + " " +
                                     std::string(typeid(MemberType).name()));
    }

protected:
    //! @brief register new data member pointer and it's value under T storage
    //! @tparam _ClassType 
    //! @tparam _MemberType 
    //! @param arg data member pointer
    //! @param value value
    //! @return 
    template <typename _ClassType, typename _MemberType>
    bool registerMember(_MemberType _ClassType::*arg, const StoredType value)
    {
        storage[new Member_t(arg)] = value;
        return true;
    }

protected:
    inline static std::unordered_map<MemberTypeBase*, StoredType, MemberHasher,
                                     MemberEqual>
        storage;
};