#pragma once

#include "IDtoBuilder.hpp"

class HistoryHourlyDtoBuilder final: public ISingleDtoBuilder
{
public:
    HistoryHourlyDtoBuilder(const timestamp_t& start,
		oatpp::UInt32 timeout = 9000):
    ISingleDtoBuilder(start, timeout, {"HistoryHourly"}) {}

	bool Process(const std::string& hash_id,
		const AnalysisSummary& summary) override;

	void LaunchRequest(
        std::shared_ptr<EngineController> engineClient) override;

	void UpdateDriverHistory(const IDtoBuilder::shares_container_t& shares,
        const timestamp_t& ts,
        DriverHistory& dh) override;

	virtual ~HistoryHourlyDtoBuilder() = default;
};

class HourlyDtoBuilder final: public ISingleDtoBuilder
{
public:
    HourlyDtoBuilder(const timestamp_t& start,
		oatpp::UInt32 timeout = 9000):
    ISingleDtoBuilder(start, timeout, {"OnlineHourly"}) {}

	bool Process(const std::string& hash_id,
		const AnalysisSummary& summary) override;

	void LaunchRequest(
        std::shared_ptr<EngineController> engineClient) override;

	void UpdateDriverHistory(const IDtoBuilder::shares_container_t& shares,
        const timestamp_t& ts,
        DriverHistory& dh) override;

	virtual ~HourlyDtoBuilder() = default;
};

class QuarterHourlyDtoBuilder final: public IMultipleDtoBuilder
{
public:
    QuarterHourlyDtoBuilder(const timestamp_t& start,
		oatpp::UInt32 timeout = 9000):
    IMultipleDtoBuilder(start, timeout, {"OnlineQuarterHourly"}) {}

	bool Process(const std::string& hash_id,
		const AnalysisSummary& summary) override;

	void LaunchRequest(
        std::shared_ptr<EngineController> engineClient) override;

	void UpdateDriverHistory(const IDtoBuilder::shares_container_t& shares,
        const timestamp_t& ts,
        DriverHistory& dh) override;

	virtual ~QuarterHourlyDtoBuilder() = default;
};

class OnOrderDtoBuilder final: public IMultipleDtoBuilder
{
public:
    OnOrderDtoBuilder(const timestamp_t& end,
		oatpp::UInt32 timeout = 9000):
        IMultipleDtoBuilder(end, timeout, {"OnOrder"}) {
        defaultDto_->end = to_oatpp_str(end);
    }

	bool Process(const std::string& hash_id,
		const AnalysisSummary& summary) override;

	void LaunchRequest(
        std::shared_ptr<EngineController> engineClient) override;

	void UpdateDriverHistory(const IDtoBuilder::shares_container_t& shares,
        const timestamp_t& ts,
        DriverHistory& dh) override;

	virtual ~OnOrderDtoBuilder() = default;
};
