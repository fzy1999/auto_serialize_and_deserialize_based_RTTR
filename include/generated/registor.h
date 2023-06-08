#pragma once
#include "myrttr/type"
#include "myrttr/registration"
#include "../common.h"

#include "../test/basic_test/initiate.h"

RTTR_REGISTRATION
{
  rttr::registration::class_<c2redis::IdHolder>("IdHolder")
      .property("id", &c2redis::IdHolder::id)
      .property("derive_type", &c2redis::IdHolder::derive_type);
  rttr::registration::class_<TopClass>("TopClass")
      .constructor<>()

      .property("secplist", &TopClass::secplist)
      .property("second", &TopClass::second)
      .property("tops", &TopClass::tops)
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
      .property("bot_inst", &SecondClass::bot_inst)

      ;
  rttr::registration::class_<TopClass::Top>("TopClass::Top")
      .constructor<>()

      .property("x", &TopClass::Top::x)
      .property("second", &TopClass::Top::top_sec)

      ;
  rttr::registration::class_<BottomClass>("BottomClass")
      .constructor<>()

      .property("name", &BottomClass::name)
      .property("second", &BottomClass::second)

      ;
  rttr::registration::class_<Base>("Base")
      .constructor<>()

      .property("bx", &Base::bx)

      ;
}