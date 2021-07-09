#pragma once

#include <memory>
#include <string_view>

#include <provider/provider.hpp>

std::unique_ptr<provider> get_provider(std::string_view name);
