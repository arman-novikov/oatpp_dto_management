#include "SpecificDtoBuilders.hpp"
#include "TimeParser.hpp"

#include "utils/Logger.hpp"

/*!
 * @module contains specializations for each of EngineController
 * API_CALL methods
*/

bool HistoryHourlyDtoBuilder::Process(
	const std::string& hash_id,
	const AnalysisSummary& summary)
{
	return PushBack(summary.windowRequired, hash_id);
}

void HistoryHourlyDtoBuilder::LaunchRequest(
        std::shared_ptr<EngineController> engineClient)
{
    Request(engineClient, &EngineController::postHistoryHourly);
}

void HistoryHourlyDtoBuilder::UpdateDriverHistory(
    const IDtoBuilder::shares_container_t& shares,
    const timestamp_t& ts,
	DriverHistory& dh)
{
    Logger::trace(std::string("HistoryHourly, new info ") +
                  Logger::make(shares));
    dh.ProcessWindow(shares, ts);
}

bool HourlyDtoBuilder::Process(
	const std::string& hash_id,
	const AnalysisSummary& summary)
{
	return PushBack(summary.hourlyRequired, hash_id);
}

void HourlyDtoBuilder::LaunchRequest(
        std::shared_ptr<EngineController> engineClient)
{
	Request(engineClient, &EngineController::postHourly);
}

void HourlyDtoBuilder::UpdateDriverHistory(
    const IDtoBuilder::shares_container_t& shares,
    const timestamp_t& ts,
	DriverHistory& dh)
{
    Logger::trace(std::string("OnlineHourly, new info ") +
                  Logger::make(shares));
    dh.AppendBigInterim(shares[0], ts); // todo: check boundaries?
}

bool QuarterHourlyDtoBuilder::Process(
	const std::string& hash_id,
	const AnalysisSummary& summary)
{
	if (summary.clarify.empty()) {
        return false;
	}

	for (const auto& clarification_ts: summary.clarify) {
        UpdateDtos(clarification_ts, hash_id,
                   {to_oatpp_str(clarification_ts)},
                   std::nullopt);
    }

    return true;
}

void QuarterHourlyDtoBuilder::LaunchRequest(
        std::shared_ptr<EngineController> engineClient)
{
	Request(engineClient, &EngineController::postQuarterHourly);
}

void QuarterHourlyDtoBuilder::UpdateDriverHistory(
    const IDtoBuilder::shares_container_t& shares,
    const timestamp_t& ts,
	DriverHistory& dh)
{
    Logger::trace(std::string("OnlineQuarterHourly, new info ") +
                  Logger::make(shares));
	dh.Patch(shares, ts);
}

bool OnOrderDtoBuilder::Process(
	const std::string& hash_id,
	const AnalysisSummary& summary)
{

	if (!summary.onOrderRequiredFrom) {
        return false;
	}
    const timestamp_t start = summary.onOrderRequiredFrom.value();

    UpdateDtos(start, hash_id, to_oatpp_str(start),
        defaultDto_->end);

	return true;
}

void OnOrderDtoBuilder::LaunchRequest(
        std::shared_ptr<EngineController> engineClient)
{
	Request(engineClient, &EngineController::postOnOrder);
}

void OnOrderDtoBuilder::UpdateDriverHistory(
    const IDtoBuilder::shares_container_t& shares,
    const timestamp_t& ts,
	DriverHistory& dh)
{
    auto start_ts = to_timestamp_t( dtos_.at(ts)->start );
    auto end_ts = to_timestamp_t( dtos_.at(ts)->end );
    Logger::trace(std::string("OnOrder, new info ") +
                  Logger::make(shares));
    dh.SetOnOrder(start_ts, end_ts, shares[0]);
}
