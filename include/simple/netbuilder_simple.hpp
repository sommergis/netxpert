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
  /**
  * \brief Simple Interface of the NetworkBuilder
  *
  * \li Initialization with a JSON Config in Constructor
  * \li call of Build() method (saves the results to the ResultDB given in the config JSON)
  * \li optional: GetBuiltNetworkAsJSON() returns the network as JSON string
  **/
	class NetworkBuilder
	{
		public:
      ///\brief Constructor
			NetworkBuilder(std::string jsonCnfg);
			///\brief virtual empty Destructor
			virtual ~NetworkBuilder() {}
      /**
      * \brief Builds & saves the network.
      * \returns 0 if successful, 1 if unsuccessful
      */
			int Build();
      ///\brief Gets the built network as JSON string
      ///\todo implement me
			std::string GetBuiltNetworkAsJSON();
			///\brief Gets the built network
			std::unordered_map<uint32_t, netxpert::data::NetworkBuilderResultArc> GetBuiltNetwork();

		private:
			std::unique_ptr<netxpert::NetworkBuilder> builder;
			netxpert::cnfg::Config NETXPERT_CNFG;
	};
 }
}
#endif //NETWORKBUILDER_SIMPLE_H
