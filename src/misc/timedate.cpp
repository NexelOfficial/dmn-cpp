#include "dmn/misc/timedate.hpp"

#include <domino/global.h>
#include <domino/misc.h>

#include <stdexcept>

using dmn::time_date;

static_assert(sizeof(TIMEDATE) == sizeof(dmn::time_date));
static_assert(alignof(TIMEDATE) == alignof(dmn::time_date));

auto time_date::from_time_point(std::chrono::system_clock::time_point tp)
  -> std::optional<time_date> {
  using namespace std::chrono;
  const auto hundredths = floor<duration<long long, std::centi>>(tp);
  const auto day_point = floor<days>(hundredths);
  const year_month_day ymd{day_point};
  if (!ymd.ok()) {
    throw std::out_of_range("Date is outside the calendar range");
  }

  const auto time_of_day = hundredths - day_point;
  const hh_mm_ss time{time_of_day};

  TIME output{};
  output.year = static_cast<int>(ymd.year());
  output.month = static_cast<int>(static_cast<unsigned>(ymd.month()));
  output.day = static_cast<int>(static_cast<unsigned>(ymd.day()));
  output.hour = static_cast<int>(time.hours().count());
  output.minute = static_cast<int>(time.minutes().count());
  output.second = static_cast<int>(time.seconds().count());
  output.hundredth = static_cast<int>(time.subseconds().count());

  if (TimeLocalToGM(&output) != NOERROR) {
    return std::nullopt;
  }
  return *reinterpret_cast<dmn::time_date*>(&output.GM);
}

auto time_date::to_time_point() const -> std::optional<std::chrono::system_clock::time_point> {
  using namespace std::chrono;
  constexpr static uint16_t YEAR_EPOCH = 1970;
  TIME t{};
  t.year = YEAR_EPOCH;
  t.month = 1;
  t.day = 1;

  if (TimeLocalToGM(&t) != NOERROR) {
    return std::nullopt;
  }

  const auto secs = TimeDateDifference(reinterpret_cast<const TIMEDATE*>(this), &t.GM);
  return system_clock::time_point{seconds{secs}};
}

auto time_date::as_raw_time_date() noexcept -> tagTIMEDATE* {
  return reinterpret_cast<tagTIMEDATE*>(this);
}