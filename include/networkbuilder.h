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

#ifndef NETWORKBUILDER_H
#define NETWORKBUILDER_H

#include "dbhelper.h" //includes already logger, config, data, utils
#include "dbwriter.h"
#include "slitewriter.h"
#include "fgdbwriter.h"
#include <vector>
#include "geos/io/WKBReader.h"
#include "geos/io/WKBWriter.h"
#include "geos/geom/Point.h"
#include "geos/geom/PrecisionModel.h"
#include "geos/operation/linemerge/LineMerger.h"
#include "geos/geomgraph/GeometryGraph.h"

namespace netxpert {
    /**
    * \brief Builds a network from a set of linestrings
    *
    * \b Concept <br>
    * Builds a network from the given linestrings in such a way that the geometry of the lines is parsed
    * and the start points of a line form the fromNode and the end point of a line
    * the toNode of the internal graph.
    * \li Converts multi part to single part linestrings
    * \li Undirected as default for network builder input
	  *
	  * \b Pro (compared to planarization):
	  * \li no loss of attribute information on the edges of the graph
	  * \li more performance compared to union all linestrings (=noding)
	  *
    * \b Con (compared to planarization):
    * \li much more (useless) egdes and nodes in the resulting graph if the input graph is not clean (crap in - crap out..)
    *
    * \return arcs in network structure with fromNode and toNode and all input attributes
    **/
    class NetworkBuilder
    {
        public:
            ///\brief Constructor
            NetworkBuilder(netxpert::cnfg::Config& cnfg);

            ///\brief Empty Destructor
            ~NetworkBuilder() {};

            /**
            * \brief Loads the arc data into a Graph.
            *
            * There is no check for planarity of the input!<br>
            * Multilinestrings that cannot be merged as a simple Linestring will cause an error message
            **/
            void LoadData();
            ///\brief Saves the built network
            void SaveResults(const std::string& resultTableName, const netxpert::data::ColumnMap& cmap) const;
            ///\brief Gets the built network
            std::unordered_map<uint32_t, netxpert::data::NetworkBuilderResultArc> GetBuiltNetwork();

        private:
            netxpert::cnfg::Config NETXPERT_CNFG;
            std::vector<netxpert::data::NetworkBuilderInputArc> inputArcs;
            std::unordered_map<uint32_t, netxpert::data::NetworkBuilderResultArc> builtNetwork;
            std::unordered_map< std::string, netxpert::data::IntNodeID> builtNodes;
            std::unique_ptr<geos::geomgraph::GeometryGraph> geoGraph;
            void calcNodes();
            std::unique_ptr<geos::geom::LineString> mergeMultiLineString(geos::geom::Geometry& geom);
            std::unique_ptr<geos::geom::GeometryFactory> GEO_FACTORY;
    };
}
#endif // NETWORKBUILDER_H
