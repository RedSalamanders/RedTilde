#pragma once
// clang-format off
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
// clang-format on

#include <DxUi/Failure.h>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string_view>
#include <wil/resource.h>
#include <yyjson.h>

namespace RedTilde
{
using unique_yyjson_doc     = wil::unique_any<yyjson_doc*, decltype(&yyjson_doc_free), yyjson_doc_free>;
using unique_yyjson_mut_doc = wil::unique_any<yyjson_mut_doc*, decltype(&yyjson_mut_doc_free), yyjson_mut_doc_free>;
using unique_yyjson_text    = wil::unique_any<char*, decltype(&::free), ::free>;

template <typename T> [[nodiscard]] inline T* CheckedJsonAlloc(T* ptr) noexcept
{
    if (! ptr)
        DxUi::Failure::FailAllocation();
    return ptr;
}

[[nodiscard]] inline unique_yyjson_doc ParseJsonDocument(std::string_view jsonBytes, yyjson_read_flag flags = 0u, yyjson_read_err* outError = nullptr) noexcept
{
    if ((flags & YYJSON_READ_INSITU) != 0u)
        DxUi::Failure::FailContract(E_INVALIDARG);
    const char* const ptr = jsonBytes.empty() ? "" : jsonBytes.data();
    yyjson_read_err err{};
    unique_yyjson_doc doc(yyjson_read_opts(const_cast<char*>(ptr), jsonBytes.size(), flags, nullptr, &err));
    if (! doc && err.code == YYJSON_READ_ERROR_MEMORY_ALLOCATION)
        DxUi::Failure::FailAllocation();
    if (outError)
        *outError = err;
    return doc;
}

[[nodiscard]] inline unique_yyjson_mut_doc CreateMutableJsonDoc() noexcept
{
    return unique_yyjson_mut_doc(CheckedJsonAlloc(yyjson_mut_doc_new(nullptr)));
}

[[nodiscard]] inline unique_yyjson_mut_doc CopyMutableJsonDoc(const yyjson_doc* source) noexcept
{
    return unique_yyjson_mut_doc(CheckedJsonAlloc(yyjson_doc_mut_copy(const_cast<yyjson_doc*>(source), nullptr)));
}

[[nodiscard]] inline yyjson_mut_val* CreateMutableJsonObject(yyjson_mut_doc* doc) noexcept
{
    return CheckedJsonAlloc(yyjson_mut_obj(doc));
}

[[nodiscard]] inline yyjson_mut_val* CreateMutableJsonArray(yyjson_mut_doc* doc) noexcept
{
    return CheckedJsonAlloc(yyjson_mut_arr(doc));
}

inline void PutJsonObjectValue(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, yyjson_mut_val* value) noexcept
{
    yyjson_mut_val* const keyVal = CheckedJsonAlloc(yyjson_mut_strcpy(doc, key));
    if (! yyjson_mut_obj_put(obj, keyVal, CheckedJsonAlloc(value)))
        DxUi::Failure::FailAllocation();
}

inline void PutJsonObjectString(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, std::string_view value) noexcept
{
    const char* const ptr = value.empty() ? "" : value.data();
    PutJsonObjectValue(doc, obj, key, yyjson_mut_strncpy(doc, ptr, value.size()));
}

inline void PutJsonObjectBool(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, bool value) noexcept
{
    PutJsonObjectValue(doc, obj, key, yyjson_mut_bool(doc, value));
}

inline void PutJsonObjectInt(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, std::int64_t value) noexcept
{
    PutJsonObjectValue(doc, obj, key, yyjson_mut_sint(doc, value));
}

inline void PutJsonObjectUint(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, std::uint64_t value) noexcept
{
    PutJsonObjectValue(doc, obj, key, yyjson_mut_uint(doc, value));
}

inline void AppendJsonArrayValue(yyjson_mut_val* arr, yyjson_mut_val* value) noexcept
{
    if (! yyjson_mut_arr_append(arr, CheckedJsonAlloc(value)))
        DxUi::Failure::FailAllocation();
}

inline void AppendJsonArrayString(yyjson_mut_doc* doc, yyjson_mut_val* arr, std::string_view value) noexcept
{
    const char* const ptr = value.empty() ? "" : value.data();
    AppendJsonArrayValue(arr, yyjson_mut_strncpy(doc, ptr, value.size()));
}

[[nodiscard]] inline unique_yyjson_text WriteJsonDocument(yyjson_mut_doc* doc,
                                                          yyjson_write_flag flags,
                                                          std::size_t& outLength,
                                                          yyjson_write_err* outError = nullptr) noexcept
{
    outLength = 0u;
    yyjson_write_err err{};
    unique_yyjson_text text(yyjson_mut_write_opts(doc, flags, nullptr, &outLength, &err));
    if (! text && err.code == YYJSON_WRITE_ERROR_MEMORY_ALLOCATION)
        DxUi::Failure::FailAllocation();
    if (outError)
        *outError = err;
    return text;
}

[[nodiscard]] inline bool TryReadJsonString(yyjson_val* object, const char* key, std::string_view& out) noexcept
{
    yyjson_val* const field = yyjson_obj_get(object, key);
    if (! field || ! yyjson_is_str(field))
        return false;
    out = std::string_view(yyjson_get_str(field), yyjson_get_len(field));
    return true;
}

inline void ReadJsonBool(yyjson_val* object, const char* key, bool& target) noexcept
{
    yyjson_val* const field = yyjson_obj_get(object, key);
    if (field && yyjson_is_bool(field))
        target = yyjson_get_bool(field);
}

[[nodiscard]] inline bool TryReadJsonIntegral(yyjson_val* value, std::int64_t minValue, std::int64_t maxValue, std::int64_t& result) noexcept
{
    if (! value)
        return false;
    if (yyjson_is_sint(value))
    {
        const std::int64_t signedVal = yyjson_get_sint(value);
        if (signedVal < minValue || signedVal > maxValue)
            return false;
        result = signedVal;
        return true;
    }
    if (yyjson_is_uint(value))
    {
        const std::uint64_t unsignedVal = yyjson_get_uint(value);
        if (maxValue < 0 || unsignedVal > static_cast<std::uint64_t>(maxValue))
            return false;
        const auto signedVal = static_cast<std::int64_t>(unsignedVal);
        if (signedVal < minValue)
            return false;
        result = signedVal;
        return true;
    }
    if (yyjson_is_real(value))
    {
        const double realVal = yyjson_get_real(value);
        if (! std::isfinite(realVal) || realVal != std::floor(realVal))
            return false;
        if (realVal < -0x1p63 || realVal >= 0x1p63)
            return false;
        if (realVal < static_cast<double>(minValue) || realVal > static_cast<double>(maxValue))
            return false;
        const auto converted = static_cast<std::int64_t>(realVal);
        if (converted < minValue || converted > maxValue)
            return false;
        result = converted;
        return true;
    }
    return false;
}
} // namespace RedTilde
