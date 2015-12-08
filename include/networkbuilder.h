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
	* --> Vorteil  ggü. Planarisierung: kein Attributverlust an den Kanten des Graphen
	* --> Nachteil ggü. Planarisierung: viel mehr (unnötige) Kanten und Knoten im Graph
    * --> IN: Linien
    * --> OUT: Netzwerk mit FromNode ToNode und allen Attributen
    **/
    class NetworkBuilder
    {
        public:

            NetworkBuilder(Config& cnfg);

            virtual ~NetworkBuilder()  {};

            /**
            * Loads the Edge Data into a Graph.
            * Caution:
            *     - There is no check for planarity of the input!
            *     - Multilinestrings that cannot be merged as a Linestring will throw an exception
            **/
            void LoadData();
            void SaveResults(const std::string& resultTableName, const netxpert::ColumnMap& cmap) const;
            std::unordered_map<unsigned int, NetworkBuilderResultArc> GetBuiltNetwork();

        private:
            netxpert::Config NETXPERT_CNFG;
            std::vector<NetworkBuilderInputArc> inputArcs;
            std::unordered_map<unsigned int, NetworkBuilderResultArc> builtNetwork;
            std::unordered_map< std::string, IntNodeID> builtNodes;
            std::unique_ptr<geos::geomgraph::GeometryGraph> geoGraph;
            void calcNodes();
            std::unique_ptr<geos::geom::LineString> mergeMultiLineString(geos::geom::Geometry& geom);
            std::unique_ptr<geos::geom::GeometryFactory> GEO_FACTORY;

    };
}
#endif // NETWORKBUILDER_H
