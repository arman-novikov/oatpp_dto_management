#include <chrono>
#include <algorithm>

#include "IDtoBuilder.hpp"
#include "TimeParser.hpp"
#include "utils/Logger.hpp"

/*!
 * @brief IDtoBuilder::IDtoBuilder Interface class to compose DTOs
 * @param start timestamp to start request from
 * @param timeout seconds for request timeout
 * @param description of the specified method like postHourly and etc
 *  generally used for logging
 */
IDtoBuilder::IDtoBuilder(const timestamp_t& start,
                         oatpp::UInt32 timeout,
                         std::string_view description):
    defaultDto_(std::make_shared<ManagementRequest>()),
    info_{description}
{
    defaultDto_->drivers = oatpp::Vector<oatpp::String>::createShared();
    defaultDto_->start = to_oatpp_str(start);
    defaultDto_->timeout = timeout;
}

oatpp::String IDtoBuilder::convert_string(const std::string& str) {
    return oatpp::String(str.c_str());
}

oatpp::String IDtoBuilder::to_oatpp_str(const timestamp_t& ts)
{
    return oatpp::String(TimeParser<>::to_str(ts).c_str());
}

timestamp_t IDtoBuilder::to_timestamp_t(const oatpp::String& str)
{
    auto tm = TimeParser<>::from_str_to_timegm(str->std_str());

    if (!tm) {
		throw std::runtime_error("failed to convert string to timestamp");
    }
    return tm.value();
}

/*!
 * @brief used in Update method heirs' overrided
 * @param histories map of {hash_id: DriverHistory}
 * @param resp response got from engine
 * @param objectMapper required for converting to Dto
 * @param ts timestamp of the !sent! request
 */
void IDtoBuilder::ProcessResponse_(histories_t& histories,
    response_t resp,
    object_mapper_t objectMapper,
    const timestamp_t& ts)
{
	auto management_response = resp->readBodyToDto<
			oatpp::Object<ManagementResponse>
		>(&(*objectMapper)); // has to be supplied with a raw pointer to objectMapper

    for (const auto& driver: *(management_response->drivers)) {
		try {
			UpdateDriverHistory(
                *driver->shares,
                ts,
				histories.at(driver->hash_id)
			);

		} catch (const std::out_of_range&) {
			throw std::runtime_error("can't find history for a driver");
		}
	}
}


void IDtoBuilder::handleErrorCode_(const std::string& info) {
    Logger::error("response code is not 200: " + info);
}

/*!
 * @brief if predicate is true pushes a driver to Dto to be sent
 * @param predicate condition to added a hash_id into request
 * @param hash_id
 * @return true if hash_id is added, otherwise false
 */
bool ISingleDtoBuilder::PushBack(bool predicate, const std::string& hash_id)
{
	if (!predicate) {
		return false;
	}
	defaultDto_->drivers->push_back(convert_string(hash_id));
	return true;
}

/*!
 * @brief updates histories with info received from engine
 * @param histories data to be updated
 * @param objectMapper required for converting to Dto
 */
void ISingleDtoBuilder::Update(IDtoBuilder::histories_t& histories,
    object_mapper_t objectMapper)
{
    if (!response_.valid()) {
        return; // throw exception?
    }
	response_.wait();
	auto r = response_.get();
	if (r->getStatusCode() != 200) {
        handleErrorCode_(r->readBodyToString()->std_str());
        return;
	}

    ProcessResponse_(histories, r, objectMapper,
                     to_timestamp_t(defaultDto_->start));
}

/*!
 * @brief sends request to engine to execute MPC
 * @param engineClient API_CALL controller
 * @param method one of engineClient methods
 */
void ISingleDtoBuilder::Request(std::shared_ptr<EngineController> engineClient,
	controller_method_t method)
{
    Logger::info(Logger::make(defaultDto_, info_));
    Logger::trace(Logger::make(defaultDto_->drivers));
    response_ = std::async(std::launch::async, [this]
            (std::shared_ptr<EngineController> engineClient,
             controller_method_t method)
        {
            return method(*engineClient, defaultDto_, nullptr);
        }, engineClient, method);
}

/*!
 * @return true if request is required, false otherwise
 */
bool ISingleDtoBuilder::RequestRequired() const {
	return !defaultDto_->drivers->empty();
}

/*!
 * @brief updates Dtos with a new hash id
 * @param key timestamp hash ids are aggregated by
 * @param hash_id value to add into one of Dtos in dtos_
 * @param start timestamp for ManagementRequest::start
 * @param end timestamp for ManagementRequest::end
 */
void IMultipleDtoBuilder::UpdateDtos(const timestamp_t& key,
    const std::string& hash_id,
	const std::optional<oatpp::String>& start,
	const std::optional<oatpp::String>& end)
{        
    if(auto it = dtos_.find(key); it != dtos_.end()) {
		it->second->drivers->push_back(convert_string(hash_id));
		return;
	}

    auto new_dto = std::make_shared<ManagementRequest>();
    new_dto->drivers = oatpp::Vector<oatpp::String>::createShared();
    new_dto->timeout = defaultDto_->timeout;

	
	if (start) {
		new_dto->start = start.value();
	}

	if (end) {
		new_dto->end = end.value();
	}

	new_dto->drivers->push_back(convert_string(hash_id));
	dtos_[key] = new_dto;
}

/*!
 * @brief sends request to engine to execute MPC
 * @param engineClient API_CALL controller
 * @param method one of engineClient methods
 */
void IMultipleDtoBuilder::Request(
    std::shared_ptr<EngineController> engineClient,
	controller_method_t method)
{
    for(auto& iter: dtos_) {
        Logger::info(Logger::make(iter.second, info_));
        Logger::trace(Logger::make(iter.second->drivers));
        responses_.push_back({iter.first,
            std::async(std::launch::async, [&iter]
                   (std::shared_ptr<EngineController> engineClient,
                    controller_method_t method)
               {
                   return method(*engineClient, iter.second, nullptr);
               }, engineClient, method)});
	}
}

/*!
 * @brief updates histories with info received from engine
 * @param histories data to be updated
 * @param objectMapper required for converting to Dto
 */
void IMultipleDtoBuilder::Update(histories_t& histories,
    object_mapper_t objectMapper)
{
	for (auto& response: responses_) {
        response.second .wait();
        auto r = response.second.get();
		if(r->getStatusCode() != 200) {
            handleErrorCode_(r->readBodyToString()->std_str());
            continue;
		}

        ProcessResponse_(histories, r, objectMapper, response.first);
	}
}

/*!
 * @return true if request/s is/are required, false otherwise
 */
bool IMultipleDtoBuilder::RequestRequired() const {
	auto res = std::find_if(dtos_.begin(), dtos_.end(),
        [](const auto& i){return !i.second->drivers->empty();}
	);

	return res != dtos_.end();
}
