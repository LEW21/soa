/** zookeeper_configuration_service.cc
    Jeremy Barnes, 26 September 2012
    Copyright (c) 2012 Datacratic Inc.  All rights reserved.

    Configuration service using Zookeeper.
*/

#include "zookeeper_configuration_service.h"
#include "soa/service/zookeeper.h"
#include "jml/utils/exc_assert.h"
#include <boost/algorithm/string.hpp>
#include <sys/utsname.h>
#include <cxxcompat/optional>

using namespace std;
using namespace ML;

namespace Datacratic {

std::string printZookeeperEventType(int type)
{

    if (type == ZOO_CREATED_EVENT)
        return "CREATED";
    if (type == ZOO_DELETED_EVENT)
        return "DELETED";
    if (type == ZOO_CHANGED_EVENT)
        return "CHANGED";
    if (type == ZOO_CHILD_EVENT)
        return "CHILD";
    if (type == ZOO_SESSION_EVENT)
        return "SESSION";
    if (type == ZOO_NOTWATCHING_EVENT)
        return "NOTWATCHING";
    return ML::format("UNKNOWN(%d)", type);
}

std::string printZookeeperState(int state)
{
    if (state == ZOO_EXPIRED_SESSION_STATE)
        return "ZOO_EXPIRED_SESSION_STATE";
    if (state == ZOO_AUTH_FAILED_STATE)
        return "ZOO_AUTH_FAILED_STATE";
    if (state == ZOO_CONNECTING_STATE)
        return "ZOO_CONNECTING_STATE";
    if (state == ZOO_ASSOCIATING_STATE)
        return "ZOO_ASSOCIATING_STATE";
    if (state == ZOO_CONNECTED_STATE)
        return "ZOO_CONNECTED_STATE";
    return ML::format("ZOO_UNKNOWN_STATE(%d)", state);
}

auto translateType(int zookeeper) -> optional<ConfigurationService::ChangeType>
{
	if (zookeeper == ZOO_CREATED_EVENT)
		return ConfigurationService::CREATED;
	if (zookeeper == ZOO_DELETED_EVENT)
		return ConfigurationService::DELETED;
	if (zookeeper == ZOO_CHANGED_EVENT)
		return ConfigurationService::VALUE_CHANGED;
	if (zookeeper == ZOO_CHILD_EVENT)
		return ConfigurationService::NEW_CHILD;

	return nullopt;
}

auto getWatcherFn(const ConfigurationService::Watch & watch, const string& path) -> ZookeeperConnection::Callback
{
    if (!watch)
        return nullptr;

	auto item = watch.data;
	return [item, path](int type)
	{
		auto t = translateType(type);
		if (t && item->watchReferences > 0)
			item->onChange(path, *t);
	};
}



/*****************************************************************************/
/* ZOOKEEPER CONFIGURATION SERVICE                                           */
/*****************************************************************************/

ZookeeperConfigurationService::
ZookeeperConfigurationService()
{
}

ZookeeperConfigurationService::
ZookeeperConfigurationService(std::string host,
                              std::string prefix,
                              std::string location,
                              int timeout)
{
    init(std::move(host), std::move(prefix), std::move(location));
}
    
ZookeeperConfigurationService::
~ZookeeperConfigurationService()
{
}

void
ZookeeperConfigurationService::
init(std::string host,
     std::string prefix,
     std::string location,
     int timeout)
{
    currentLocation = std::move(location);

    zoo.reset(new ZookeeperConnection());
    zoo->connect(host, timeout);

    if (!prefix.empty() && prefix[prefix.size() - 1] != '/')
        prefix = prefix + "/";

    if (!prefix.empty() && prefix[0] != '/')
        prefix = "/" + prefix;

    this->prefix = std::move(prefix);
    zoo->createPath(this->prefix);
}

Json::Value
ZookeeperConfigurationService::
getJson(const std::string & key, Watch watch)
{
    ExcAssert(zoo);
    auto val = zoo->readNode(prefix + key, getWatcherFn(watch, prefix + key));
    try {
        if (val == "")
            return Json::Value();
        return Json::parse(val);
    } catch (...) {
        cerr << "error parsing JSON entry '" << val << "'" << endl;
        throw;
    }
}
    
void
ZookeeperConfigurationService::
set(const std::string & key,
    const Json::Value & value)
{
    //cerr << "setting " << key << " to " << value << endl;
    // TODO: race condition
    if (!zoo->createNode(prefix + key, boost::trim_copy(value.toString()),
                         false, false,
                         false /* must succeed */,
                         true /* create path */).second)
        zoo->writeNode(prefix + key, boost::trim_copy(value.toString()));
    ExcAssert(zoo);
}

std::string
ZookeeperConfigurationService::
setUnique(const std::string & key,
          const Json::Value & value)
{
    //cerr << "setting unique " << key << " to " << value << endl;
    ExcAssert(zoo);
    return zoo->createNode(prefix + key, boost::trim_copy(value.toString()),
                           true /* ephemeral */,
                           false /* sequential */,
                           true /* mustSucceed */,
                           true /* create path */)
        .first;
}

std::vector<std::string>
ZookeeperConfigurationService::
getChildren(const std::string & key,
            Watch watch)
{
    //cerr << "getChildren " << key << " watch " << watch << endl;
    return zoo->getChildren(prefix + key,
                            false /* fail if not there */,
                            getWatcherFn(watch, prefix + key));
}

bool
ZookeeperConfigurationService::
forEachEntry(const OnEntry & onEntry,
                      const std::string & startPrefix) const
{
    //cerr << "forEachEntry: startPrefix = " << startPrefix << endl;

    ExcAssert(zoo);

    std::function<bool (const std::string &)> doNode
        = [&] (const std::string & currentPrefix)
        {
            //cerr << "doNode " << currentPrefix << endl;

            string r = zoo->readNode(prefix + currentPrefix);

            //cerr << "r = " << r << endl;
            
            if (r != "") {
                if (!onEntry(currentPrefix, Json::parse(r)))
                    return false;
            }

            vector<string> children = zoo->getChildren(prefix + currentPrefix,
                                                       false);
            
            for (auto child: children) {
                //cerr << "child = " << child << endl;
                string newPrefix = currentPrefix + "/" + child;
                if (currentPrefix.empty())
                    newPrefix = child;
                
                if (!doNode(newPrefix))
                    return false;
            }
            
            return true;
        };

    if (!zoo->nodeExists(prefix + startPrefix)) {
        

        return true;
    }
    
    return doNode(startPrefix);
}

void
ZookeeperConfigurationService::
removePath(const std::string & path)
{
    ExcAssert(zoo);
    zoo->removePath(prefix + path);
}


} // namespace Datacratic
