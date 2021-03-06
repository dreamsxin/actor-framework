/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright 2011-2020 Dominik Charousset                                     *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#define CAF_SUITE telemetry.collector.prometheus

#include "caf/telemetry/collector/prometheus.hpp"

#include "caf/test/dsl.hpp"

#include "caf/telemetry/metric_registry.hpp"
#include "caf/telemetry/metric_type.hpp"

using namespace caf;
using namespace caf::literals;
using namespace caf::telemetry;

namespace {

struct fixture {
  collector::prometheus exporter;
  metric_registry registry;
};

} // namespace

CAF_TEST_FIXTURE_SCOPE(prometheus_tests, fixture)

CAF_TEST(the Prometheus collector generates text output) {
  auto fb = registry.gauge_family("foo", "bar", {},
                                  "Some value without labels.", "seconds");
  auto sv = registry.gauge_family("some", "value", {"a", "b"},
                                  "Some (total) value with two labels.", "1",
                                  true);
  auto ov = registry.gauge_family("other", "value", {"x"}, "", "seconds", true);
  std::vector<int64_t> upper_bounds{1, 2, 4};
  auto sr = registry.histogram_family("some", "request-duration", {"x"},
                                      upper_bounds, "Some help.", "seconds");
  fb->get_or_add({})->value(123);
  sv->get_or_add({{"a", "1"}, {"b", "2"}})->value(12);
  sv->get_or_add({{"b", "1"}, {"a", "2"}})->value(21);
  ov->get_or_add({{"x", "true"}})->value(31337);
  auto h = sr->get_or_add({{"x", "get"}});
  h->observe(3);
  h->observe(4);
  h->observe(7);
  CAF_CHECK_EQUAL(exporter.collect_from(registry, 42),
                  R"(# HELP foo_bar_seconds Some value without labels.
# TYPE foo_bar_seconds gauge
foo_bar_seconds 123 42000
# HELP some_value_total Some (total) value with two labels.
# TYPE some_value_total gauge
some_value_total{a="1",b="2"} 12 42000
some_value_total{a="2",b="1"} 21 42000
# TYPE other_value_seconds_total gauge
other_value_seconds_total{x="true"} 31337 42000
# HELP some_request_duration_seconds Some help.
# TYPE some_request_duration_seconds histogram
some_request_duration_seconds_bucket{x="get",le="1"} 0 42000
some_request_duration_seconds_bucket{x="get",le="2"} 0 42000
some_request_duration_seconds_bucket{x="get",le="4"} 2 42000
some_request_duration_seconds_bucket{x="get",le="+Inf"} 1 42000
some_request_duration_seconds_sum{x="get"} 14 42000
some_request_duration_seconds_count{x="get"} 1 42000
)"_sv);
  CAF_MESSAGE("multiple runs generate the same output");
  std::string res1;
  {
    auto buf = exporter.collect_from(registry);
    res1.assign(buf.begin(), buf.end());
  }
  CAF_CHECK_EQUAL(res1, exporter.collect_from(registry));
}

CAF_TEST_FIXTURE_SCOPE_END()
