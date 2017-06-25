#ifndef NETWORKBUILDER_SIMPLE_H
#define NETWORKBUILDER_SIMPLE_H

#include "data.h"
#include "utils.h"
#include "network.h"
#include "dbhelper.h"
#include "networkbuilder.h"

namespace netxpert {
 namespace simple {

	class NetworkBuilder
	{
		public:
			NetworkBuilder(std::string jsonCnfg);
			virtual ~NetworkBuilder() {}

			int Build();

			std::string GetBuiltNetworkAsJSON();
			std::unordered_map<uint32_t, netxpert::data::NetworkBuilderResultArc> GetBuiltNetwork();

		private:
			std::unique_ptr<netxpert::NetworkBuilder> builder;
			netxpert::cnfg::Config NETXPERT_CNFG;
	};
 }
}
#endif //NETWORKBUILDER_SIMPLE_H
