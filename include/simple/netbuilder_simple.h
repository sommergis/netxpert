/*
 * This file is a part of netxpert.
 *
 * Copyright (C) 2013-2017
 * Johannes Sommer, Christopher Koller
 *
 * Permission to use, modify and distribute this software is granted
 * provided that this copyright notice appears in all copies. For
 * precise terms see the accompanying LICENSE file.
 *
 * This software is provided "AS IS" with no warranty of any kind,
 * express or implied, and with no claim as to its suitability for any
 * purpose.
 *
 */

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
