#pragma once
#define _USE_MATH_DEFINES

#include <cassert>
#include <cctype>
#include <cerrno>
#include <cfenv>
#include <cfloat>
#include <cinttypes>
#include <climits>
#include <clocale>
#include <cmath>
#include <csetjmp>
#include <csignal>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cuchar>
#include <cwchar>
#include <cwctype>
#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <barrier>
#include <bit>
#include <bitset>
#include <charconv>
#include <chrono>
#include <compare>
#include <complex>
#include <concepts>
#include <condition_variable>
#include <deque>
#include <exception>
#include <execution>
#include <filesystem>
#include <format>
#include <forward_list>
#include <fstream>
#include <functional>
#include <future>
#include <initializer_list>
#include <iomanip>
#include <iosfwd>
#include <ios>
#include <iostream>
#include <istream>
#include <iterator>
#include <latch>
#include <limits>
#include <locale>
#include <map>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <new>
#include <numbers>
#include <numeric>
#include <optional>
#include <ostream>
#include <queue>
#include <random>
#include <ranges>
#include <regex>
#include <ratio>
#include <scoped_allocator>
#include <semaphore>
#include <set>
#include <shared_mutex>
#include <source_location>
#include <span>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <string_view>
#include <syncstream>
#include <system_error>
#include <thread>
#include <tuple>
#include <typeindex>
#include <typeinfo>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <variant>
#include <vector>
#include <version>

#include <SKSE/SKSE.h>
#include <RE/Skyrim.h>
#include <REL/Relocation.h>

#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <tbb/concurrent_vector.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>
#include <tbb/parallel_invoke.h>
#include <tbb/parallel_reduce.h>
#include <tbb/parallel_sort.h>
#include <tbb/parallel_scan.h>
#include <tbb/blocked_range.h>
#include <tbb/blocked_range2d.h>

#include <absl/container/flat_hash_map.h>
#include <absl/strings/string_view.h>
#include <absl/strings/str_format.h>
#include <absl/container/flat_hash_set.h>

namespace logger = SKSE::log;
using namespace std::literals;
using namespace REL::literals;

//Global Includes
#include "Util/Text/Text.hpp"
#include "Util/Windows/MessageBox.hpp"
#include "Util/Singleton.hpp"
#include "Util/Random.hpp"
#include "Util/Text/Text.hpp"

//#define LOG(fmt, ...) logger::info(fmt, ##__VA_ARGS__)
//#define LOG_ERR(fmt, ...) logger::error(fmt, ##__VA_ARGS__)
//#define LOG_INFO(fmt, ...) logger::info(fmt, ##__VA_ARGS__)

#define LOG(fmt, ...)
#define LOG_ERR(fmt, ...)
#define LOG_INFO(fmt, ...) 