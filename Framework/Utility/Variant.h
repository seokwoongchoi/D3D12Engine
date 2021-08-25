#pragma once
#include "Framework.h"


#define __VARIANT_TYPES         \
     char,                       \
    unsigned char,              \
    int,                        \
    uint,                       \
    long,                       \
    unsigned long,              \
    long long,                  \
    bool,                       \
    float,                      \
    double,                     \
    void*,                      \
    Vector2,                    \
    Vector3,                    \
    Vector4,                    \
    Color,                      \
    Quaternion,                 \
    Matrix,                      \
    void

#define VARIANT_TYPES std::variant<__VARIANT_TYPES>

typedef std::variant<__VARIANT_TYPES, VARIANT_TYPES> Variant_internal;

class Variant final
{
public:
	Variant() = default;
	~Variant() = default;

	Variant(const Variant& var)
	{
		variant = var.GetVariant();
	}

	template <typename T, typename = std::enable_if<!std::is_same<T, Variant>::value>>
	Variant(T value)
	{
		this->variant = value;
	}

	Variant& operator=(const Variant& rhs)
	{
		this->variant = rhs.GetVariant();
	}

	template <typename T, typename = std::enable_if<!std::is_same<T, Variant>::value>>
	Variant& operator=(T rhs) { return variant = rhs; }

	auto GetVariant() const -> const Variant_internal& { return variant; }

	template <typename T>
	auto Get() const -> const T& { return std::get<T>(variant); }

private:
	Variant_internal variant;
};