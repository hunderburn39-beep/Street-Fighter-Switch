#pragma once
#include "visual_queue.hpp"
#include <stdexcept>
class VisualSession {
public:
 using Bind=void(*)(const SftmVisualSink*);
 explicit VisualSession(Bind bind):binding(bind){
  if(!binding) throw std::invalid_argument("Missing visual bridge");
  auto sink=queue.sink();binding(&sink);
 }
 ~VisualSession(){binding(nullptr);}
 VisualSession(const VisualSession&)=delete;
 VisualSession& operator=(const VisualSession&)=delete;
 const VisualQueue& frames() const{return queue;}
private:
 Bind binding;
 VisualQueue queue;
};
