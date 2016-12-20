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
    * \Class Builds a network of the given linestrings.
    * - Multipart to Single Part Linestrings
	* - StartPoint(Linestring): FromNode, EndPoint(Linestring): ToNode
	* - Undirected als Standard in Solver input
	* --> Pro (compared to planarization): no loss of attribute information on the edges of the graph
	* --> Con (compared to planarization): much more (useless) egdes and nodes in the resulting graph
    * --> IN: lines
    * --> OUT: edges in network structure with FromNode and ToNode and all input attributes
    **/
    class NetworkBuilder
    {
        public:

            NetworkBuilder(netxpert::cnfg::Config& cnfg);

            virtual ~NetworkBuilder()  {};

            /**
            * Loads the Edge Data into a Graph.
            * Caution:
            *     - There is no check for planarity of the input!
            *     - Multilinestrings that cannot be merged as a Linestring will throw an exception
            **/
            void LoadData();
            void SaveResults(const std::string& resultTableName, const netxpert::data::ColumnMap& cmap) const;
            std::unordered_map<unsigned int, netxpert::data::NetworkBuilderResultArc> GetBuiltNetwork();

        private:
            netxpert::cnfg::Config NETXPERT_CNFG;
            std::vector<netxpert::data::NetworkBuilderInputArc> inputArcs;
            std::unordered_map<unsigned int, netxpert::data::NetworkBuilderResultArc> builtNetwork;
            std::unordered_map< std::string, netxpert::data::IntNodeID> builtNodes;
            std::unique_ptr<geos::geomgraph::GeometryGraph> geoGraph;
            void calcNodes();
            std::unique_ptr<geos::geom::LineString> mergeMultiLineString(geos::geom::Geometry& geom);
            std::unique_ptr<geos::geom::GeometryFactory> GEO_FACTORY;
    };
}
#endif // NETWORKBUILDER_H
