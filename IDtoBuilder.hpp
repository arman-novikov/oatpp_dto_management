#pragma once

#include <unordered_map>
#include <deque>
#include <optional>
#include <future>
#include <functional>

#include "driver_analyzer.hpp"
#include "controller/EngineController.hpp"
#include "oatpp/parser/json/mapping/ObjectMapper.hpp"

/*!
* @todo check if it is possible to use string_view
*/
class IDtoBuilder
{
public:
	using histories_t = std::unordered_map<oatpp::String, DriverHistory>;
	using response_t = std::shared_ptr<
		oatpp::web::protocol::http::incoming::Response
	>;
	using controller_method_t = std::function<
		response_t(
			EngineController&,
            std::shared_ptr<ManagementRequest>,
            const std::shared_ptr<
				oatpp::web::client::RequestExecutor::ConnectionHandle
            >&
		)
	>;
	using shares_container_t = oatpp::data::mapping::type::VectorObjectWrapper<
		oatpp::data::mapping::type::Primitive<
			long int,
			oatpp::data::mapping::type::__class::Int64
		>,
		oatpp::data::mapping::type::__class::Vector<
			oatpp::data::mapping::type::Primitive<
				long int, oatpp::data::mapping::type::__class::Int64
			>
		>
	>::TemplateObjectType;

	using object_mapper_t = std::shared_ptr<
		oatpp::parser::json::mapping::ObjectMapper
	>;

    IDtoBuilder(const timestamp_t& start,
                oatpp::UInt32 timeout = 9000,
                std::string_view description = "");

	virtual bool Process(const std::string& hash_id,
		const AnalysisSummary& summary) = 0;

	virtual void Update(histories_t& histories,
        object_mapper_t objectMapper) = 0;

	virtual bool RequestRequired() const = 0;

    virtual void LaunchRequest(std::shared_ptr<EngineController> engineClient) = 0;

	virtual ~IDtoBuilder() = default;
protected:
	static oatpp::String convert_string(const std::string& str);

	static timestamp_t to_timestamp_t(const oatpp::String& str);
	static oatpp::String to_oatpp_str(const timestamp_t& ts);

    void handleErrorCode_(const std::string& info);
	
	void ProcessResponse_(
		histories_t& histories,
        response_t resp,
        object_mapper_t objectMapper,
        const timestamp_t& ts);

    virtual void UpdateDriverHistory(const shares_container_t& shares,
        const timestamp_t& ts,
        DriverHistory& dh) = 0;

	std::shared_ptr<ManagementRequest> defaultDto_;
    std::string info_; // used for logging
};

class ISingleDtoBuilder: public IDtoBuilder
{
protected:
    ISingleDtoBuilder(const timestamp_t& start,
                      oatpp::UInt32 timeout = 9000,
                      std::string_view description = ""):
        IDtoBuilder(start, timeout, description){}

    void Request(std::shared_ptr<EngineController> engineClient,
        controller_method_t method);

	void Update(histories_t& histories,
        object_mapper_t objectMapper) override;

	bool RequestRequired() const override;

	bool PushBack(bool predicate, const std::string& hash_id);
	virtual ~ISingleDtoBuilder() = default;

	std::future<response_t> response_;
};

class IMultipleDtoBuilder: public IDtoBuilder
{
protected:
    IMultipleDtoBuilder(const timestamp_t& start,
                        oatpp::UInt32 timeout = 9000,
                        std::string_view description = ""):
        IDtoBuilder(start, timeout, description) {}

    void Update(histories_t& histories,
        object_mapper_t objectMapper) override;

    bool RequestRequired() const override;

    void UpdateDtos(const timestamp_t& key,
        const std::string& hash_id,
        const std::optional<oatpp::String>& start,
        const std::optional<oatpp::String>& end);

    void Request(std::shared_ptr<EngineController> engineClient,
        controller_method_t method);

    virtual ~IMultipleDtoBuilder() = default;

    std::unordered_map<timestamp_t, std::shared_ptr<ManagementRequest>> dtos_;
    std::deque<std::pair<timestamp_t, std::future<response_t>>> responses_;
};
