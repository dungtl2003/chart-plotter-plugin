#pragma once

class RenderData {
public:
  RenderData(const RenderData &) = delete;
  RenderData(RenderData &&) = delete;
  RenderData &operator=(const RenderData &) = delete;
  RenderData &operator=(RenderData &&) = delete;
  virtual ~RenderData() = default;
};
