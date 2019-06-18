#pragma once

#include <string>

#include "BLI_array_ref.hpp"
#include "BLI_small_vector.hpp"
#include "BLI_math.hpp"
#include "BLI_string_ref.hpp"
#include "BLI_range.hpp"
#include "BLI_small_set_vector.hpp"

namespace BParticles {

using BLI::ArrayRef;
using BLI::float3;
using BLI::Range;
using BLI::SmallSetVector;
using BLI::SmallVector;
using BLI::StringRef;
using BLI::StringRefNull;

enum AttributeType {
  Byte,
  Float,
  Float3,
};

inline uint size_of_attribute_type(AttributeType type)
{
  switch (type) {
    case AttributeType::Byte:
      return sizeof(uint8_t);
    case AttributeType::Float:
      return sizeof(float);
    case AttributeType::Float3:
      return sizeof(float3);
    default:
      BLI_assert(false);
      return 0;
  };
}

class AttributesInfo {
 private:
  Range<uint> m_byte_attributes;
  Range<uint> m_float_attributes;
  Range<uint> m_float3_attributes;
  SmallVector<AttributeType> m_types;
  SmallSetVector<std::string> m_indices;

 public:
  AttributesInfo() = default;
  AttributesInfo(ArrayRef<std::string> byte_names,
                 ArrayRef<std::string> float_names,
                 ArrayRef<std::string> float3_names);

  uint amount() const
  {
    return m_indices.size();
  }

  StringRefNull name_of(uint index) const
  {
    return m_indices.values()[index];
  }

  AttributeType type_of(uint index) const
  {
    return m_types[index];
  }

  uint attribute_index(StringRef name) const
  {
    int index = m_indices.index(name.to_std_string());
    BLI_assert(index >= 0);
    return (uint)index;
  }

  Range<uint> byte_attributes() const
  {
    return m_byte_attributes;
  }

  Range<uint> float_attributes() const
  {
    return m_float_attributes;
  }

  Range<uint> float3_attributes() const
  {
    return m_float3_attributes;
  }

  friend bool operator==(const AttributesInfo &a, const AttributesInfo &b)
  {
    return &a == &b;
  }

  SmallVector<void *> allocate_separate_arrays(uint size) const;
};

class AttributeArrays;

class AttributeArraysCore {
 private:
  AttributesInfo &m_info;
  SmallVector<void *> m_arrays;
  uint m_size = 0;

 public:
  AttributeArraysCore() = default;

  AttributeArraysCore(AttributesInfo &info, ArrayRef<void *> arrays, uint size);

  AttributesInfo &info();
  void *get_ptr(uint index);
  AttributeType get_type(uint index);
  AttributeArrays slice_all();
  uint size() const;
  ArrayRef<void *> pointers();
};

class AttributeArrays {
 private:
  AttributeArraysCore &m_core;
  uint m_start, m_size;

 public:
  AttributeArrays(AttributeArraysCore &core, uint start, uint size);

  uint size() const;
  AttributesInfo &info();

  uint attribute_index(StringRef name);

  void *get_ptr(uint index) const;

  ArrayRef<uint8_t> get_byte(uint index) const;
  ArrayRef<uint8_t> get_byte(StringRef name);
  ArrayRef<float> get_float(uint index) const;
  ArrayRef<float> get_float(StringRef name);
  ArrayRef<float3> get_float3(uint index) const;
  ArrayRef<float3> get_float3(StringRef name);

  AttributeArrays slice(uint start, uint size) const;
  AttributeArrays take_front(uint n) const;
};

class JoinedAttributeArrays {
 private:
  AttributesInfo &m_info;
  SmallVector<AttributeArrays> m_arrays;
  uint m_size;

 public:
  JoinedAttributeArrays(AttributesInfo &info, ArrayRef<AttributeArrays> arrays_list);

  AttributesInfo &info();

  uint size() const;
  ArrayRef<AttributeArrays> arrays_list();

  void set_byte(uint index, ArrayRef<uint8_t> data);
  void set_byte(StringRef name, ArrayRef<uint8_t> data);
  void set_float(uint index, ArrayRef<float> data);
  void set_float(StringRef name, ArrayRef<float> data);
  void set_float3(uint index, ArrayRef<float3> data);
  void set_float3(StringRef name, ArrayRef<float3> data);

 private:
  void set_elements(uint index, void *data);
};

/* Attribute Arrays Core
 *****************************************/

inline AttributeArraysCore::AttributeArraysCore(AttributesInfo &info,
                                                ArrayRef<void *> arrays,
                                                uint size)
    : m_info(info), m_arrays(arrays.to_small_vector()), m_size(size)
{
}

inline AttributesInfo &AttributeArraysCore::info()
{
  return m_info;
}

inline void *AttributeArraysCore::get_ptr(uint index)
{
  return m_arrays[index];
}

inline AttributeType AttributeArraysCore::get_type(uint index)
{
  return m_info.type_of(index);
}

inline AttributeArrays AttributeArraysCore::slice_all()
{
  return AttributeArrays(*this, 0, m_size);
}

inline uint AttributeArraysCore::size() const
{
  return m_size;
}

inline ArrayRef<void *> AttributeArraysCore::pointers()
{
  return m_arrays;
}

/* Attribute Arrays
 ******************************************/

inline AttributeArrays::AttributeArrays(AttributeArraysCore &core, uint start, uint size)
    : m_core(core), m_start(start), m_size(size)
{
  BLI_assert(m_start + m_size <= m_core.size());
}

inline uint AttributeArrays::size() const
{
  return m_size;
}

inline AttributesInfo &AttributeArrays::info()
{
  return m_core.info();
}

inline uint AttributeArrays::attribute_index(StringRef name)
{
  return this->info().attribute_index(name);
}

inline void *AttributeArrays::get_ptr(uint index) const
{
  void *ptr = m_core.get_ptr(index);
  AttributeType type = m_core.get_type(index);
  uint size = size_of_attribute_type(type);
  return POINTER_OFFSET(ptr, m_start * size);
}

inline ArrayRef<uint8_t> AttributeArrays::get_byte(uint index) const
{
  BLI_assert(m_core.get_type(index) == AttributeType::Byte);
  return ArrayRef<uint8_t>((uint8_t *)m_core.get_ptr(index) + m_start, m_size);
}

inline ArrayRef<uint8_t> AttributeArrays::get_byte(StringRef name)
{
  return this->get_byte(this->attribute_index(name));
}

inline ArrayRef<float> AttributeArrays::get_float(uint index) const
{
  BLI_assert(m_core.get_type(index) == AttributeType::Float);
  return ArrayRef<float>((float *)m_core.get_ptr(index) + m_start, m_size);
}

inline ArrayRef<float> AttributeArrays::get_float(StringRef name)
{
  return this->get_float(this->attribute_index(name));
}

inline ArrayRef<float3> AttributeArrays::get_float3(uint index) const
{
  BLI_assert(m_core.get_type(index) == AttributeType::Float3);
  return ArrayRef<float3>((float3 *)m_core.get_ptr(index) + m_start, m_size);
}

inline ArrayRef<float3> AttributeArrays::get_float3(StringRef name)
{
  return this->get_float3(this->attribute_index(name));
}

inline AttributeArrays AttributeArrays::slice(uint start, uint size) const
{
  return AttributeArrays(m_core, m_start + start, size);
}

inline AttributeArrays AttributeArrays::take_front(uint n) const
{
  BLI_assert(n <= m_size);
  return AttributeArrays(m_core, m_start, n);
}

/* Joined Attribute Arrays
 ******************************************/

inline JoinedAttributeArrays::JoinedAttributeArrays(AttributesInfo &info,
                                                    ArrayRef<AttributeArrays> arrays_list)
    : m_info(info), m_arrays(arrays_list.to_small_vector())
{
  m_size = 0;
  for (AttributeArrays arrays : arrays_list) {
    BLI_assert(arrays.info() == m_info);
    m_size += arrays.size();
  }
}

inline AttributesInfo &JoinedAttributeArrays::info()
{
  return m_info;
}

inline uint JoinedAttributeArrays::size() const
{
  return m_size;
}

inline ArrayRef<AttributeArrays> JoinedAttributeArrays::arrays_list()
{
  return m_arrays;
}

}  // namespace BParticles