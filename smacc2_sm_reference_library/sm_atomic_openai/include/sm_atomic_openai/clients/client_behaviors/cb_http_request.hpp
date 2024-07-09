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

#include "sm_atomic_openai/b64_encode.hpp"

#include <cstring>
#include <vector>
#include <http_client/cl_http_client.hpp>
#include <http_client/client_behaviors/cb_http_request.hpp>
#include <smacc2/smacc.hpp>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>

namespace sm_atomic_openai
{

template <typename TSource, typename TOrthogonal>
struct EvHttp : sc::event<EvHttp<TSource, TOrthogonal>>
{
};

class CbHttpRequest : public cl_http::CbHttpRequestBase
{
public:
  CbHttpRequest() : cl_http::CbHttpRequestBase(cl_http::ClHttp::kHttpRequestMethod::POST) {}

  template <typename TOrthogonal, typename TSourceObject>
  void onOrthogonalAllocation()
  {
    triggerTranstition = [this]()
    {
      auto event = new EvHttp<TSourceObject, TOrthogonal>();
      this->postEvent(event);
    };
  }

  void runtimeConfigure() override
  {
    // Get OpenAI key from environemt
    char const * env_key = std::getenv("OPENAI_API_KEY");
    openai_key_ = env_key == NULL ? std::string() : std::string(env_key);

    this->requiresClient(cl_http_);
    cl_http_->onResponseReceived(&CbHttpRequest::onResponseReceived, this);
  }

  void onEntry() override 
  {
    if (openai_key_.empty())
    {
      RCLCPP_FATAL_STREAM(
        this->getNode()->get_logger(), "An OpenAI key environment variable was not found.");
      triggerTranstition();
      return;
    }

    // Gather image data
    auto image = cv::imread("/home/jaycee/Downloads/wood_table.jpg");
    std::vector<uchar> img_buf;
    cv::imencode(".jpg", image, img_buf);
    const auto *encoded_img_b64 = reinterpret_cast<unsigned char *>(img_buf.data());
    const std::string encoded_img = base64_encode(encoded_img_b64, img_buf.size());

    // Construct HTTP request
    nlohmann::json req_body_json;
    req_body_json["model"] = "gpt-4o";
    req_body_json["messages"][0]["role"] = "user";
    req_body_json["messages"][0]["content"][0]["type"] = "text";
    req_body_json["messages"][0]["content"][0]["text"] = "Is this a door, yes or no";
    req_body_json["messages"][0]["content"][1]["type"] = "image_url";
    req_body_json["messages"][0]["content"][1]["image_url"]["url"] = ("data:image/jpg;base64," + encoded_img);

    const std::string req_body = req_body_json.dump();
    cl_http_->makeRequest(cl_http::ClHttp::kHttpRequestMethod::POST, "/", req_body);
  }

  void onResponseReceived(const cl_http::ClHttp::TResponse & response) override
  {
    RCLCPP_INFO_STREAM(this->getNode()->get_logger(), "Responce recieved: ");
    RCLCPP_INFO_STREAM(this->getNode()->get_logger(), response.body());
    triggerTranstition();
  }

private:
  cl_http::ClHttp * cl_http_;

  std::string openai_key_;

  std::function<void()> triggerTranstition;
};
}  // namespace sm_atomic_openai
