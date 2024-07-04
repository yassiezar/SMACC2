// Copyright 2023 RobosoftAI Inc.
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

/*****************************************************************************************************************
 *
 * 	 Authors: Jaycee Lock
 *
 ******************************************************************************************************************/

#pragma once

#include <cstring>
#include <http_client/cl_http_client.hpp>
#include <http_client/client_behaviors/cb_http_post_request.hpp>
#include <smacc2/smacc.hpp>

namespace sm_atomic_openai
{

template <typename TSource, typename TOrthogonal>
struct EvHttp : sc::event<EvHttp<TSource, TOrthogonal>>
{
};

class CbHttpRequest : public cl_http::CbHttpPostRequest
{
public:
  template <typename TOrthogonal, typename TSourceObject>
  void onOrthogonalAllocation()
  {
    triggerTranstition = [this]()
    {
      auto event = new EvHttp<TSourceObject, TOrthogonal>();
      this->postEvent(event);
    };
  }

  virtual void runtimeConfigure() override
  {
    // Get OpenAI key from environemt
    char const * env_key = std::getenv("OPENAI_API_KEY");
    openai_key_ = env_key == NULL ? std::string() : std::string(env_key);
    if (openai_key_.empty())
    {
      RCLCPP_FATAL_STREAM(this->get_logger(), "An OpenAI key environment variable was not found.");
    }

    this->requiresClient(cl_http_);
    cl_http_->onResponseReceived(&CbHttpRequest::onResponseReceived, this);
  }

  void onResponseReceived(const cl_http::ClHttp::TResponse & response) override
  {
    RCLCPP_INFO_STREAM(this->getLogger(), "Responce recieved: ");
    RCLCPP_INFO_STREAM(this->getLogger(), response.body());
    triggerTranstition();
  }

private:
  cl_http::ClHttp * cl_http_;

  std::string openai_key_;

  std::function<void()> triggerTranstition;
};
}  // namespace sm_atomic_openai
