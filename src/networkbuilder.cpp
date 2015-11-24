#include "networkbuilder.h"
using namespace netxpert;

NetworkBuilder::NetworkBuilder(Config& cnfg)
{
    //FIXED = 0 Kommastellen
    //FLOATING_SINGLE = 6 Kommastellen
    //FLOATING = 16 Kommastellen
    // Sollte FLOATING sein - sonst gibts evtl geometriefehler (Lücken beim CreateRouteGeometries())
    // Grund ist, dass SpatiaLite eine hohe Präzision hat und diese beim splitten von Linien natürlich auch hoch sein
    // muss.
    // Performance ist zu vernachlässigen, weil ja nur geringe Mengen an Geometrien eingelesen und verarbeitet werden
    // (nur die Kanten, die aufgebrochen werden)
    // --> Zusammengefügt werden die Kanten (Route) ja in der DB bei Spatialite (FGDB?).
	unique_ptr<PrecisionModel> pm (new PrecisionModel( geos::geom::PrecisionModel::FLOATING));

	// Initialize global factory with defined PrecisionModel
	// and a SRID of -1 (undefined).
	GEO_FACTORY = unique_ptr<GeometryFactory> ( new GeometryFactory( pm.get(), -1)); //SRID = -1

    NETXPERT_CNFG = cnfg;

    if (!DBHELPER::IsInitialized)
        DBHELPER::Initialize(NETXPERT_CNFG);
}

void NetworkBuilder::LoadData()
{
    LOGGER::LogInfo("Loading data from DB..");

    netxpert::ColumnMap cmap {  NETXPERT_CNFG.ArcIDColumnName,  NETXPERT_CNFG.FromNodeColumnName,
                                NETXPERT_CNFG.ToNodeColumnName, NETXPERT_CNFG.CostColumnName,
                                NETXPERT_CNFG.CapColumnName,    NETXPERT_CNFG.OnewayColumnName,
                                NETXPERT_CNFG.NodeIDColumnName, NETXPERT_CNFG.NodeSupplyColumnName };

    DBHELPER::OpenNewTransaction();

    this->inputArcs = DBHELPER::LoadNetworkToBuildFromDB(NETXPERT_CNFG.ArcsTableName, cmap);

    DBHELPER::CommitCurrentTransaction();
    DBHELPER::CloseConnection();

    LOGGER::LogInfo("Done!");

    std::vector<Geometry*> geoms;
    std::vector<NetworkBuilderInputArc>::const_iterator it;

    LOGGER::LogDebug("Processing #"+ to_string(this->inputArcs.size()) + " arcs..");

    for (it = this->inputArcs.begin(); it != this->inputArcs.end(); ++it)
    {
        //auto a = *it;
        switch (it->geom->getGeometryTypeId())
        {
            case geos::geom::GEOS_LINESTRING:
            {
                #pragma omp critical
                {
                geoms.push_back( it->geom->clone() );
                }
                break;
            }
            case geos::geom::GEOS_MULTILINESTRING:
            {
                //merge
                geos::operation::linemerge::LineMerger lm;
                lm.add( it->geom.get() );
                std::vector<LineString *> *mls = lm.getMergedLineStrings();
                auto& mlsRef = *mls;
                if (mlsRef.size() > 1)
                    LOGGER::LogError("MultiLineString could not be merged to LineString!");
                else
                {
                    auto mlsFirst = *(mlsRef.begin());
                    auto geom = std::shared_ptr<geos::geom::Geometry>( mlsFirst->clone() );
                    #pragma omp critical
                    {
                    geoms.push_back(geom.get());
                    }
                }
                break;
            }
            default:
            {
                LOGGER::LogError("Geometry type " + it->geom->getGeometryType() +  " is not supported!");
                break;
            }
        }
    }
    // multilinestring
    unique_ptr<MultiLineString> mLine (DBHELPER::GEO_FACTORY->createMultiLineString( geoms ) );
    this->geoGraph = unique_ptr<geos::geomgraph::GeometryGraph>(new geos::geomgraph::GeometryGraph( 0, mLine.get() ));
}

void NetworkBuilder::calcNodes()
{
    std::vector< geos::geomgraph::Node * > * nodePtr = this->geoGraph->getBoundaryNodes();
    unsigned int counter = 0;
    std::vector< geos::geomgraph::Node* >& nodeRef = *nodePtr;
    std::vector< geos::geomgraph::Node* >::const_iterator it = nodeRef.begin();
    for (it = nodeRef.begin(); it != nodeRef.end(); ++it)
    {
        auto curNode = *it;
        counter += 1;
        geos::geom::Point* p = DBHELPER::GEO_FACTORY->createPoint(curNode->getCoordinate());
        std::unique_ptr<geos::geom::Point> geom (p);
        #pragma omp critical
        {
        this->builtNodes.insert( make_pair( geom->toString(), counter) );
        }
    }
}

std::unordered_map<unsigned int, NetworkBuilderResultArc>
NetworkBuilder::GetBuiltNetwork()
{
    using namespace geos::geom;
    LOGGER::LogDebug("Calculating nodes..");
    calcNodes();
    LOGGER::LogDebug("Done!");
    std::unordered_map<unsigned int, NetworkBuilderResultArc> result(this->inputArcs.size());

    unsigned int counter = 0;
    //kein const_iterator, weil move() unten angewendet wird!
    std::vector<NetworkBuilderInputArc>::iterator it;

    LOGGER::LogDebug("Calculating edges..");

    for (it = this->inputArcs.begin(); it != this->inputArcs.end(); ++it)
    {
        //dereferencing iterator that points to unique_ptr as member of struct: njet!
        //NetworkBuilderInputArc arc = *it;

        //DOUBLE FREE with unique_ptr!
        //auto linePtr = unique_ptr<LineString>( dynamic_cast<LineString*>( it->geom.get() ));
        auto linePtr = dynamic_cast<LineString*>( it->geom.get() );

        counter += 1;

        unique_ptr<Point> startPoint (linePtr->getStartPoint() );
        unique_ptr<Point> endPoint   (linePtr->getEndPoint() );
        unsigned int fromNode   = 0;
        unsigned int toNode     = 0;

        if ( this->builtNodes.count(startPoint->toString()) > 0)
        {
            fromNode = this->builtNodes.at(startPoint->toString());
        }
        if ( this->builtNodes.count(endPoint->toString()) > 0)
        {
            toNode = this->builtNodes.at(endPoint->toString());
        }
        NetworkBuilderResultArc resArc {it->extArcID, fromNode,  toNode, it->cost, it->capacity, it->oneway, move( it->geom ) };
        #pragma omp critical
        {
        result.insert( make_pair(counter, move(resArc)) );
        }
    }
    LOGGER::LogDebug("Done!");
    return result;
}

