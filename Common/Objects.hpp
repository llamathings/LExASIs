#pragma once

#include <LESDK/Headers.hpp>
#include "Common/Base.hpp"


namespace Common
{

    /// @brief      Unreal-style iterator filtering objects by a specific type.
    /// @tparam     T - A type derived from @c UObject of which all filtered objects must be.
    /// @tparam     bAllowDerived - Whether filtered objects are allowed to have types derived from the given type.
    /// @tparam     bIncludeDefaults - Whether filtered objects are allowed to be Class Default Objects.
    template<std::derived_from<UObject> T, bool bAllowDerived = true, bool bIncludeDefaults = true>
    class TypedObjectIterator final : public NonCopyable
    {
        UObject** Current{ nullptr };

        void SkipToNext()
        {
            static constexpr QWORD RF_ClassDefaultObject = 0x200;
            UObject** const IterEnd = UObject::GObjObjects->GetData()
                + UObject::GObjObjects->Count();

            if constexpr (bAllowDerived)
            {
                while (++Current <= IterEnd)
                {
                    if (*Current && (*Current)->Cast<T>())
                    {
                        if constexpr (!bIncludeDefaults)
                        {
                            if (((*Current)->ObjectFlags & RF_ClassDefaultObject) != 0)
                                continue;
                        }

                        break;
                    }
                }
            }
            else
            {
                while (++Current <= IterEnd)
                {
                    if (*Current && (*Current)->CastDirect<T>())
                    {
                        if constexpr (!bIncludeDefaults)
                        {
                            if (((*Current)->ObjectFlags & RF_ClassDefaultObject) != 0)
                                continue;
                        }

                        break;
                    }
                }
            }
        }

    public:

        TypedObjectIterator()
        {
            Current = UObject::GObjObjects->GetData();
            Current--;
            SkipToNext();
        }

        operator bool() const
        {
            UObject** const IterEnd = UObject::GObjObjects->GetData()
                + UObject::GObjObjects->Count();
            return Current < IterEnd;
        }

        T* operator*() const
        {
            LESDK_CHECK(*this, "invalid iterator");

            using BareType = std::remove_cvref_t<std::remove_pointer_t<T>>;
            return reinterpret_cast<BareType*>(*Current);
        }

        TypedObjectIterator& operator++()
        {
            SkipToNext();
            return *this;
        }
    };


    /// @brief      Finds first non-default object of the given type in the global object array.
    /// @tparam     T - A type derived from @c UObject of which all filtered objects must be.
    /// @tparam     bAllowDerived - Whether filtered objects are allowed to have types derived from the given type.
    /// @returns    An object pointer if found, or @c nullptr otherwise.
    template<std::derived_from<UObject> T, bool bAllowDerived = true>
    T* FindFirstObject()
    {
        for (TypedObjectIterator<T, bAllowDerived, false> Iterator{}; Iterator; ++Iterator)
            return *Iterator;

        return nullptr;
    }

    /// @brief      Finds first non-default object of the given type and passing the given filter in the global object array.
    /// @tparam     T - A type derived from @c UObject of which all filtered objects must be.
    /// @tparam     bAllowDerived - Whether filtered objects are allowed to have types derived from the given type.
    /// @param[in]  Filter - Additional filter which an object pointer must pass before being returned.
    /// @returns    An object pointer if found, or @c nullptr otherwise.
    template<std::derived_from<UObject> T, bool bAllowDerived = true>
    T* FindFirstObject(std::function<bool(T*)> Filter)
    {
        for (TypedObjectIterator<T, bAllowDerived, false> Iterator; Iterator; ++Iterator)
        {
            if (Filter(*Iterator))
                return *Iterator;
        }

        return nullptr;
    }
}
