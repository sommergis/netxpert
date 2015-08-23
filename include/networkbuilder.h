#ifndef NETWORKBUILDER_H
#define NETWORKBUILDER_H

#include "dbhelper.h" //includes already logger, config, data, utils
#include <vector>
#include "geos/io/WKBReader.h"
#include "geos/io/WKBWriter.h"
#include "geos/geom/Point.h"
#include "geos/geom/PrecisionModel.h"
#include "geos/operation/linemerge/LineMerger.h"
#include "geos/geomgraph/GeometryGraph.h"

using namespace geos::geom;
using namespace geos::geomgraph;
using namespace geos::operation::linemerge;

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

            shared_ptr<GeometryFactory> GEO_FACTORY;

        private:
            Config NETXPERT_CNFG;
            InputArcs arcsTbl;
            NetworkBuilderArcs builtArcs;
            NetworkBuilderNodes builtNodes;

    };
}
#endif // NETWORKBUILDER_H
