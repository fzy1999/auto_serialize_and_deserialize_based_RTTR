#pragma once
#include "myrttr/type"
#include "myrttr/registration"
#include "../common.h"

#include "/home/sxx/workSpace/c2redis/output/../test/basic_test/initiate.h" 


RTTR_REGISTRATION{
    rttr::registration::class_<io::IdHolder>("IdHolder").property("id", &io::IdHolder::id);
  rttr::registration::class_<TopClass>("TopClass")
      .constructor<>()
      
      .property("secplist", &TopClass::secplist)
      .property("second", &TopClass::second)
      .property("name", &TopClass::name)
      .property("x", &TopClass::x)
      
      ;
        rttr::registration::class_<SecondClass>("SecondClass")
      .constructor<>()
      
      .property("bottom", &SecondClass::bottom)
      .property("name", &SecondClass::name)
      .property("y", &SecondClass::y)
      .property("bottom_map", &SecondClass::bottom_map)
      
      ;
        rttr::registration::class_<BottomClass>("BottomClass")
      .constructor<>()
      
      .property("name", &BottomClass::name)
      
      ;
      
}