// Copyright 2021 RobosoftAI Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <functional>
#include <smacc2/smacc.hpp>

namespace sm_atomic_updatable {

template <typename TSource, typename TOrthogonal>
struct EvTimeElapsed : sc::event<EvTimeElapsed<TSource, TOrthogonal>> {};

class CbUpdatable : public smacc2::SmaccClientBehavior,
                    public smacc2::ISmaccUpdatable {
 public:
  CbUpdatable() : smacc2::ISmaccUpdatable{rclcpp::Duration(0, 2000000)} {}

  template <typename TOrthogonal, typename TSourceObject>
  void onOrthogonalAllocation() {
    transitionEvent = [=] {
      this->postEvent<EvTimeElapsed<TSourceObject, TOrthogonal>>();
    };
  }

  void onEntry() override {}

  void update() override {
    counter += 1;
    RCLCPP_INFO(getLogger(), "[CbUpdatable] Adding a cycle");

    // trigger at 5s running @5Hz
    if (counter > 25) {
      transitionEvent();
    }
  }

 private:
  int counter = 0;

  std::function<void()> transitionEvent;
};
}  // namespace sm_atomic_updatable
