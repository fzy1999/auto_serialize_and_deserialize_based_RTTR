#pragma once
#include <cstdint>
#include "myrttr/registration.h"
#include "myrttr/type"
#include "myrttr/registration"
#include "../common.h"

#include "/home/sxx/workSpace/c2redis/output/../test/basic_test/initiate.h"

RTTR_REGISTRATION
{
  rttr::registration::class_<io::IdHolder>("IdHolder")
      .property("id", &io::IdHolder::id)
      .property("derive_type", &io::IdHolder::derive_type);
  rttr::registration::class_<TpltClass<int32_t>>("TpltClass<int32_t>").property("num", &TpltClass<int32_t>::num);
  rttr::registration::class_<TopClass>("TopClass")
      .constructor<>()

      .property("secplist", &TopClass::secplist)
      .property("second", &TopClass::second)
      .property("top", &TopClass::top)
      .property("tplt", &TopClass::tplt)
      .property("name", &TopClass::name)
      .property("x", &TopClass::x)

      ;
  rttr::registration::class_<SecondClass>("SecondClass")
      .constructor<>()

      .property("bottom", &SecondClass::bottom)
      .property("name", &SecondClass::name)
      .property("y", &SecondClass::y)
      .property("bottom_map", &SecondClass::bottom_map)
      .property("bases", &SecondClass::bases)
      .property("opt_bot", &SecondClass::opt_bot)
      .property("opt_int", &SecondClass::opt_int)

      ;
  rttr::registration::class_<TopClass::Top>("TopClass::Top")
      .constructor<>()

      .property("x", &TopClass::Top::x)

      ;
  rttr::registration::class_<BottomClass>("BottomClass")
      .constructor<>()

      .property("name", &BottomClass::name)

      ;
  rttr::registration::class_<Base>("Base")
      .constructor<>()

      .property("bx", &Base::bx)

      ;
}