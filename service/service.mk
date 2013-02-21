# services makefile
# Jeremy Barnes, 29 May 2012


LIBOPSTATS_SOURCES := \
	statsd_connector.cc carbon_connector.cc stat_aggregator.cc process_stats.cc

LIBOPSTATS_LINK := \
	ACE arch utils boost_thread types

$(eval $(call library,opstats,$(LIBOPSTATS_SOURCES),$(LIBOPSTATS_LINK)))



LIBRECOSET_ZEROMQ_SOURCES := \
	socket_per_thread.cc \
	zmq_utils.cc

LIBRECOSET_ZEROMQ_LINK := \
	zmq

$(eval $(call library,zeromq,$(LIBRECOSET_ZEROMQ_SOURCES),$(LIBRECOSET_ZEROMQ_LINK)))

LIBSERVICES_SOURCES := \
	transport.cc \
	endpoint.cc \
	connection_handler.cc \
	http_endpoint.cc \
	json_endpoint.cc \
	active_endpoint.cc \
	passive_endpoint.cc \
	chunked_http_endpoint.cc \
	epoller.cc \
	http_header.cc \
	service_base.cc \
	message_loop.cc \
	named_endpoint.cc \
	zookeeper_configuration_service.cc \
	zmq_endpoint.cc \
	async_event_source.cc \
	rest_service_endpoint.cc \
	http_named_endpoint.cc \
	rest_proxy.cc \
	rest_request_router.cc \
	zookeeper.cc

LIBSERVICES_LINK := opstats curl curlpp boost_regex zeromq zookeeper_mt ACE arch utils jsoncpp boost_thread zmq types

$(eval $(call library,services,$(LIBSERVICES_SOURCES),$(LIBSERVICES_LINK)))


LIBENDPOINT_SOURCES := \

LIBENDPOINT_LINK := \
	services

$(eval $(call library,endpoint,$(LIBENDPOINT_SOURCES),$(LIBENDPOINT_LINK)))




LIBCLOUD_SOURCES := \
	sftp.cc \
	s3.cc \

LIBCLOUD_LINK := crypto++ curlpp utils arch types tinyxml2 services ssh2


$(eval $(call library,cloud,$(LIBCLOUD_SOURCES),$(LIBCLOUD_LINK)))


LIBREDIS_SOURCES := \
	redis.cc

LIBREDIS_LINK := hiredis utils types boost_thread

$(eval $(call library,redis,$(LIBREDIS_SOURCES),$(LIBREDIS_LINK)))


$(eval $(call program,s3_transfer_cmd,cloud boost_program_options boost_filesystem utils))
$(eval $(call program,s3tee,cloud boost_program_options utils))

SERVICEDUMP_LINK = services boost_program_options

$(eval $(call program,service_dump,$(SERVICEDUMP_LINK)))


$(eval $(call include_sub_make,service_js,js,service_js.mk))
$(eval $(call include_sub_make,service_testing,testing,service_testing.mk))
