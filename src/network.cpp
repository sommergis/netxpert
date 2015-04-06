#include "network.h"

using namespace std;
using namespace NetXpert;

// ColumnMap can override entries in Config
Network::Network(const InputArcs& _arcsTbl, const InputNodes& _nodesTbl, const ColumnMap& _map, const Config& cnfg)
{
    //ctor
    try
    {
        arcIDColName = _map.arcIDColName;
        fromColName = _map.fromColName;
        toColName = _map.toColName;
        costColName = _map.costColName;
        if (!_map.capColName.empty())
            capColName = _map.capColName;
        else
            capColName = "";
        if (!_map.onewayColName.empty())
            onewayColName = _map.onewayColName;
        else
            onewayColName = "";

        nodeIDColName = _map.nodeIDColName;
        supplyColName = _map.supplyColName;

        netXpertConfig = cnfg;

        internalArcData.reserve(_arcsTbl.size());

        this->arcsTbl = _arcsTbl;
        this->nodesTbl = _nodesTbl;
    }
    catch (exception& ex)
    {
        LOGGER::LogFatal("Network ctor: Error!");
        LOGGER::LogFatal(ex.what());
    }
}

Network::Network(const InputArcs& _arcsTbl, const ColumnMap& _map, const Config& cnfg)
{
    //ctor
    try
    {
        arcIDColName = _map.arcIDColName;
        fromColName = _map.fromColName;
        toColName = _map.toColName;
        costColName = _map.costColName;
        if (!_map.capColName.empty())
            capColName = _map.capColName;
        else
            capColName = "";
        if (!_map.onewayColName.empty())
            onewayColName = _map.onewayColName;
        else
            onewayColName = "";

        netXpertConfig = cnfg;

        internalArcData.reserve(_arcsTbl.size());

        this->arcsTbl = _arcsTbl;
    }
    catch (exception& ex)
    {
        LOGGER::LogFatal("Network ctor: Error!");
        LOGGER::LogFatal(ex.what());
    }
}

Network::Network(Arcs arcData, unordered_map<ExtArcID,IntNodeID> distinctNodeIDs,
                        NodeSupplies _nodeSupplies){}

Network::~Network()
{
    //dtor
}

//public

//TODO:
//1. Hol die nächste Kante (per Spatialite) ArcID + Geometrie
//2. Wurde die Kante schon aufgebrochen oder nicht?
//3. Hole Geometrie neu wenn nötig (aus den newArcs)
//4. isPointOnLine, position of closestPoint (Start / End ) alles in Memory (egal ob aufgebrochene Kante oder vorhandene Kante)
//5. Splitte Kante
// --> einmaliger DB-Zugriff, Rest in memory processing

unsigned int Network::AddStartNode(const NewNode& newStartNode, int treshold, SQLite::Statement& closestArcQry, bool withCapacity)
{
    unordered_map<string, FTNode> newSegments;
    newSegments.reserve(2);

    const string extNodeID = newStartNode.extNodeID;
    const Coordinate startPoint = newStartNode.coord;
    const double nodeSupply = newStartNode.supply;

    //if (!startPoint.x)
    //    throw new InvalidValueException("Start coordinate must not be null!");

    //1. Suche die nächste Line und den nächsten Punkt auf der Linie zum temporaeren Punkt
    ExtClosestArcAndPoint closestArcAndPoint = DBHELPER::GetClosestArcFromPoint(startPoint,
            treshold, closestArcQry, withCapacity);
    const string extArcID = closestArcAndPoint.extArcID;
    const Coordinate closestPoint = closestArcAndPoint.closestPoint;
    Geometry& closestArc = *closestArcAndPoint.arcGeom;
    const string extFromNode = closestArcAndPoint.extFromNode;
    const string extToNode = closestArcAndPoint.extToNode;
    const double cost = closestArcAndPoint.cost;
    const double capacity = closestArcAndPoint.capacity;

    //2. Wenn die Kante bereits aufgebrochen wurde (=Eintrag in oldArcs.ArcData.extArcID, hol
    // die nächste Linie aus NewArcs
    // Lambda expression

    /*auto it = std::count_if(oldArcs.begin(), oldArcs.end(),
        [](const std::pair<FTNode, ArcData> & t) -> bool {
        return t.second.x == extArcID; }
    );*/
    bool arcHasBeenSplitAlready = false;
    if (!arcHasBeenSplitAlready)  //es wurde die Kante noch nicht aufgebrochen
    {

        // Prüfe, ob der nächste Punkt gleichzeitig der Start - oder der Endpunkt einer Linie ist
        auto locationOfPoint = GetLocationOfPointOnLine(startPoint, closestArc);
        switch ( locationOfPoint  )
        {
            case StartOrEndLocationOfLine::Start:
                LOGGER::LogDebug("Closest Point for start node "+extNodeID
                    +" is identical to a start node of the network!");
                break;
            case StartOrEndLocationOfLine::End:
                LOGGER::LogDebug("Closest Point for start node "+extNodeID
                    +" is identical to a end node of the network!");
                break;
            default:
                break;
        }

        if (locationOfPoint != StartOrEndLocationOfLine::Intermediate)
        {
            //fromNode und toNode von oben nehmen und keinen Split durchführen
            if (locationOfPoint == StartOrEndLocationOfLine::Start) //closestPoint == startPoint of line
            {
                if (internalDistinctNodeIDs.count(extFromNode) > 0)
                {
                    unsigned int newID = internalDistinctNodeIDs.at(extFromNode);
                    //save closestPoint geom for lookup later on straight lines for ODMatrix
                    AddedPoint pVal = {extNodeID, startPoint};
                    addedStartPoints.insert(make_pair(newID, pVal));

                    //add nodeSupply
                    NodeSupply supVal = {extNodeID, nodeSupply};
                    nodeSupplies.insert(make_pair(newID, supVal));

                    return newID;
                }
            }
            else //closestPoint == endPoint of line
            {
                if (internalDistinctNodeIDs.count(extToNode) > 0)
                {
                    unsigned int newID = internalDistinctNodeIDs.at(extToNode);
                    //save closestPoint geom for lookup later on straight lines for ODMatrix
                    AddedPoint val = {extNodeID, startPoint};
                    addedStartPoints.insert(make_pair(newID, val));

                    //add nodeSupply
                    NodeSupply supVal = {extNodeID, nodeSupply};
                    nodeSupplies.insert(make_pair(newID, supVal));

                    return newID;
                }
            }
        }
        else //point lies somewhere between start and end coordinate of the line --> split!
        {
            //split line
            ExtFTNode ftPair = {extFromNode, extToNode};
            ArcData val = {extArcID, cost, capacity};
            pair<ExtFTNode,ArcData> arcData = make_pair(ftPair, val);

            cout << closestArc.toString() << endl;

            auto splittedLine = GetSplittedClosestOldArcToPoint(startPoint, treshold, arcData, closestArc);

            //TODO: Direction!
            vector<FTNode> newSegmentIDs = insertNewStartNode(true, splittedLine, extNodeID, startPoint);
            newSegments.insert( make_pair("toOrigArc", newSegmentIDs[0]) );
            newSegments.insert( make_pair("fromOrigArc", newSegmentIDs[1]) );

            unsigned int newID = newSegmentIDs[0].toNode;

            //add nodeSupply
            NodeSupply supVal = {extNodeID, nodeSupply};
            nodeSupplies.insert( make_pair(newID, supVal));

            return newID;
        }

        // in den internen Datenstrukturen hinzufügen
       /*
        //Berechne die neuen Kosten pro Kante (relativ zur Länge)
        //Hole die Infos der EdgeID aus der internen Repräsentation: Kosten und knoten-IDs


        //TODO Directed!
        //if arc's directed = false: should contain two!
        //if arc's oneway = Y: and directed = true: should contain only one!
        //if arc's oneway = N: and directed = true: should contain two!
        //var internalNetKVPair = this.InternalArcData.Where(x => x.Value.Item1 == extEdgeID).First();

        auto queryResult = internalArcData. Where(x => x.Value.Item1 == extEdgeID).ToList();

        //Regardless of count (1 or 2) take always startEndPair and cost information from first occurence
        var internalNetKVPair = queryResult[0];
        Tuple<uint, uint> startEndPair = new Tuple<uint, uint>(internalNetKVPair.Key.Item1,
                                                internalNetKVPair.Key.Item2);
        double cost = internalNetKVPair.Value.Item2;

        //Erzeuge eine neue Knoten-ID (maximale ID der Knoten +1)
        //Erzeuge für die beiden Segmente der Kante die neue Knoten/Kanten Repräsentation
        //Speichere die Knoten/Kanten Repräsentation der ursprünglichen Kante
        //Lösche die Knoten/Kanten Repräsentation der ursprünglichen Kante aus dem Netzwerk
        //Füge die neuen Segmente der Kante in Knoten/Kanten Repräsentation in das Netzwerk ein
        //Speichere die beiden Segmente der Kante (inkl. Geometrie und Knoten/Kanten Repräsentation)
        Tuple<Tuple<uint, uint>, Tuple<uint, uint>> newSegmentIDs = null;
        switch (queryResult.Count)
        {
            case 1:
                newSegmentIDs = this.insertNewStartNode(startEndPair,
                                                    true,
                                                    newSegmentsGeom, oldNodeID, startPoint);
                break;
            case 2:
                newSegmentIDs = this.insertNewStartNode(startEndPair,
                                                    false,
                                                    newSegmentsGeom, oldNodeID, startPoint);
                break;
            default:
                break;
        }*/

    }
    else // Kante schon aufgebrochen
    {
        //split line
        auto splittedLine = GetSplittedClosestNewArcToPoint(startPoint, treshold);

        //TODO: Direction!
        vector<FTNode> newSegmentIDs = insertNewStartNode(true, splittedLine, extNodeID, startPoint);
        newSegments.insert( make_pair("toOrigArc", newSegmentIDs[0]) );
        newSegments.insert( make_pair("fromOrigArc", newSegmentIDs[1]) );

        unsigned int newID = newSegmentIDs[0].toNode;

        //add nodeSupply
        NodeSupply supVal = {extNodeID, nodeSupply};
        nodeSupplies.insert( make_pair(newID, supVal));

        return newID;
    }
}

vector<FTNode> Network::insertNewStartNode(bool isDirected, SplittedArc& splittedLine, string extNodeID,
                                            const Coordinate& startPoint)
{
    vector<FTNode> result;
    result.reserve(2);

    return result;
}

void Network::AddEndNode(){}

vector<FTNode> Network::insertNewEndNode(bool isDirected, SplittedArc& splittedLine, string extNodeID,
                                            const Coordinate& endPoint)
{
    vector<FTNode> result;
    result.reserve(2);

    return result;
}

void Network::BuildTotalRouteGeometry(){}

void Network::ConvertInputNetwork(bool autoClean)
{
    renameNodes();

    processBarriers();

    //Oneway wird nur bei gerichtetem Netzwerk berücksichtigt
    if (!onewayColName.empty() && netXpertConfig.IsDirected == true)
    {
        readNetworkFromTable(autoClean, true);
    }
    else {
        readNetworkFromTable(autoClean, false);
    }
    LOGGER::LogDebug("Count of internalArcData: "+ to_string(internalArcData.size()));
    LOGGER::LogDebug("Count of arcsTbl: "+ to_string(arcsTbl.size()));
    LOGGER::LogDebug("Count of internalDistinctNodes: "+ to_string(internalDistinctNodeIDs.size()));
    LOGGER::LogDebug("Count of nodeSupplies: " + to_string(nodeSupplies.size()));
    LOGGER::LogDebug("Count of nodesTbl: " + to_string(nodesTbl.size()));
    //LOGGER::LogDebug("Last element of eliminatedArcs: " + eliminatedArcs[i]);
    LOGGER::LogDebug("Count of eliminatedArcs: " + to_string(eliminatedArcs.size()));
    //LOGGER::LogDebug("Size of eliminatedArcsCount: "+ to_string(eliminatedArcsCount) );
}

void Network::GetOriginalArcData(const list<ArcData>& origArcData, const list<FTNode>& startEndNodes, bool isDirected){}
void Network::GetOriginalArcDataAndFlow(const list<ArcDataAndFlow>& origArcDataAndFlow, const list<FTNode>& startEndNodes, bool isDirected){}

unsigned int Network::GetInternalNodeID(string externalNodeID)
{
    unsigned int internalNodeID;
    try
    {
        internalNodeID = internalDistinctNodeIDs.at(externalNodeID);
    }
    catch (exception& ex)
    {
        LOGGER::LogError("Original node ID " +externalNodeID +" could not be looked up!");
        throw ex;
    }
    return internalNodeID;
}

string Network::GetOriginalNodeID(unsigned int internalNodeID)
{
    string externalNodeID = "";
    try
    {
        externalNodeID = swappedInternalDistinctNodeIDs.at(internalNodeID);
    }
    catch (exception& ex)
    {
        LOGGER::LogError("Internal node ID " +to_string(internalNodeID) +" could not be looked up!");
        throw ex;
    }
    return externalNodeID;
}
string Network::GetOriginalStartOrEndNodeID(unsigned int internalNodeID)
{
    AddedPoint externalNode;
    try
    {
        if (addedStartPoints.count(internalNodeID) == 1)
            externalNode = addedStartPoints.at(internalNodeID);
        else
            externalNode = addedEndPoints.at(internalNodeID);
    }
    catch (exception& ex)
    {
        LOGGER::LogError("Original start/end node ID " +to_string(internalNodeID) +" could not be looked up!");
        throw ex;
    }
    return externalNode.extNodeID;
}
void Network::GetStartOrEndNodeGeometry(Coordinate& coord, unsigned int internalNodeID)
{
    AddedPoint externalNode;
    try
    {
        if (addedStartPoints.count(internalNodeID) == 1)
            externalNode = addedStartPoints.at(internalNodeID);
        else
            externalNode = addedEndPoints.at(internalNodeID);
    }
    catch (exception& ex)
    {
        LOGGER::LogError("Original start/end node ID " +to_string(internalNodeID) +" could not be looked up!");
        throw ex;
    }
    coord = externalNode.coord;
}

SplittedArc Network::GetSplittedClosestNewArcToPoint(Coordinate coord, int treshold)
{
    try
    {
        //GeometryFactory gFac;
        shared_ptr<const Point> p ( DBHELPER::GEO_FACTORY->createPoint(coord) );
        shared_ptr<const Geometry> gPtr ( dynamic_pointer_cast<const Geometry>(p) );
        shared_ptr<const Geometry> buf ( gPtr->buffer(treshold) );
        //must not defined as smart_pointer!
        const Envelope* bufEnv = buf->getEnvelopeInternal();

        map<double, pair<FTNode, NewArc> > distanceTbl;

        /*// search in spatial index for relevant geometry
        // with buffer around point (treshold)*/

        //1. Generate Distance Table
        for ( auto nArc : this->newArcs )
        {
            FTNode key = nArc.first;
            NewArc val = nArc.second;
            Geometry* g2Ptr = &val.arcGeom;

            // "spatial index like":
            // calculate distance table only for those geometries
            // that envelopes do intersect

            //must not defined as smart_pointer!
            const Envelope* g2Env = g2Ptr->getEnvelopeInternal() ;

            if(bufEnv->intersects(g2Env))
            {
                DistanceOp distCalc(*gPtr, *g2Ptr);
                //cout << distCalc.distance() << endl << val.arcGeom.toString() << endl;
                distanceTbl.insert( make_pair(distCalc.distance(),
                                    make_pair(key, val) ) );
            }
        }

        auto elem = *distanceTbl.begin();
        //double dist = elem.first;
        FTNode nearestArcKey = elem.second.first;
        NewArc nearestArc = elem.second.second;
        //shared_ptr<const Geometry> nGeomPtr ( nearestArc.arcGeom );

        shared_ptr<MultiLineString> mLine = splitLine(coord, nearestArc.arcGeom);

        SplittedArc result{ nearestArcKey.fromNode, nearestArcKey.toNode,
                            nearestArc.cost, nearestArc.capacity, mLine };

        //cout << seg1->toString() << endl << seg2->toString() << endl;
        return result;
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error splitting new line!" );
        LOGGER::LogError( ex.what() );
        throw ex;
    }
}


SplittedArc Network::GetSplittedClosestOldArcToPoint(Coordinate coord, int treshold,
                                                    const pair<ExtFTNode,ArcData>& arcData,
                                                    const Geometry& arc)
{
  try
    {
        ExtFTNode nearestArcKey = arcData.first;
        ArcData nearestArc = arcData.second;
        //shared_ptr<const Geometry> nGeomPtr ( &arc );

        //cout << arc.toString() << endl;
        shared_ptr<MultiLineString> mLine = splitLine(coord, arc);

        //Lookup ext. from / toNodes
        unsigned int fromNode = GetInternalNodeID(nearestArcKey.extFromNode);
        unsigned int toNode = GetInternalNodeID(nearestArcKey.extToNode);

        SplittedArc result{ fromNode, toNode,
                            nearestArc.cost, nearestArc.capacity, mLine };

        return result;
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error splitting new line!" );
        LOGGER::LogError( ex.what() );
        throw ex;
    }
}


shared_ptr<MultiLineString> Network::splitLine(Coordinate coord, const Geometry& lineGeom)
{
    const Geometry* gPtr = &lineGeom;

    /*geos::io::WKTReader r;
    string wkt = "LINESTRING (703475.8942999998107553 5364706.0032000001519918, 703442.5213000001385808 5364728.6747999992221594, 703419.0372000001370907 5364740.5065999999642372, 703391.1070999996736646 5364743.4655000008642673)";
    shared_ptr<const Geometry> geomPtr1 (r.read(wkt));*/

    //do the split with Linear Referencing
    // Line must be LINESTRING as Linear Ref on MULTILINESTRINGs does not work properly
    //orientation of line should not be important
    LengthIndexedLine idxLine = geos::linearref::LengthIndexedLine( gPtr );//get for raw pointer

    double pointIdx = idxLine.indexOf(coord);
    double startIdx = idxLine.getStartIndex();
    double endIdx = idxLine.getEndIndex();

    //cout << "Indexes: "<< endl;
    //cout << pointIdx << endl << startIdx << endl << endIdx << endl;

    shared_ptr<Geometry> seg1 ( idxLine.extractLine(startIdx, pointIdx) );
    shared_ptr<Geometry> seg2 ( idxLine.extractLine(pointIdx, endIdx) );

    shared_ptr<MultiLineString> mLine ( DBHELPER::GEO_FACTORY->createMultiLineString(
                                          vector<Geometry*>{ seg1.get() , seg2.get() } )
                                        );
    return mLine;
}

bool Network::IsPointOnArc(Coordinate coords, const Geometry& arc)
{
    bool isPointOnLine = false;
    const double tolerance = 0.000001;

    try
    {
        //GeometryFactory gFac;
        const Geometry* pPtr = dynamic_cast<const Geometry*>( DBHELPER::GEO_FACTORY->createPoint(coords) );

        // Buffer with minimal tolerance
        isPointOnLine = arc.intersects(pPtr->buffer(tolerance));

        return isPointOnLine;
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error testing for point on line!" );
        LOGGER::LogError( ex.what() );
        throw ex;
    }
}

double Network::GetPositionOfPointAlongLine(Coordinate coord, const Geometry& arc)
{
    try
    {
        LengthIndexedLine idxLine ( &arc );

        double pointIdx = idxLine.indexOf(coord);
        //double startIdx = idxLine.getStartIndex();
        //double endIdx = idxLine.getEndIndex();

        //cout << "Indexes: "<< endl;
        //cout << pointIdx << endl << startIdx << endl << endIdx << endl;
        if (idxLine.isValidIndex( pointIdx) )
            return pointIdx;
        else
            return idxLine.clampIndex( pointIdx );

    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error getting position along line!" );
        LOGGER::LogError( ex.what() );
        throw ex;
    }
}

StartOrEndLocationOfLine Network::GetLocationOfPointOnLine(Coordinate coord, const Geometry& arc)
{
    try
    {
        LengthIndexedLine idxLine ( &arc );

        double pointIdx = idxLine.indexOf(coord);
        double startIdx = idxLine.getStartIndex();
        double endIdx = idxLine.getEndIndex();

        //cout << "Indexes: "<< endl;
        //cout << pointIdx << endl << startIdx << endl << endIdx << endl;

        //validate Index
        if (!idxLine.isValidIndex( pointIdx) )
            pointIdx = idxLine.clampIndex( pointIdx );

        if (pointIdx == startIdx)
            return StartOrEndLocationOfLine::Start;

        if (pointIdx == endIdx)
            return StartOrEndLocationOfLine::End;

        else
            return StartOrEndLocationOfLine::Intermediate;

    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error getting position along line!" );
        LOGGER::LogError( ex.what() );
        throw ex;
    }
}
//private
void Network::renameNodes()
{
    set<string> sortedDistinctNodesSet;
    for (InputArcs::iterator it = arcsTbl.begin(); it != arcsTbl.end(); ++it)
    {
        auto itD = *it;
        //if element is not in set - add it!
        if (sortedDistinctNodesSet.find(itD.extFromNode) == sortedDistinctNodesSet.end() )
        {
            sortedDistinctNodesSet.insert(itD.extFromNode);
        }
        if (sortedDistinctNodesSet.find(itD.extToNode) == sortedDistinctNodesSet.end() )
        {
            sortedDistinctNodesSet.insert(itD.extToNode);
        }
    }
    //copy set to vector for index access
    vector<string> sortedDistinctNodes(sortedDistinctNodesSet.begin(), sortedDistinctNodesSet.end());
    // Count from 0 to sortedDistinctNodes.Count:
    // --> internal NodeIDs start from 1 to n.
    for (unsigned int i = 0; i < sortedDistinctNodes.size(); i++)
    {
        // Add [oldNodeID],[internalNodeID]
        internalDistinctNodeIDs.insert( make_pair(sortedDistinctNodes[i], i+1) );
        // Add [internalNodeID],[oldNodeID]
        swappedInternalDistinctNodeIDs.insert( make_pair(i+1, sortedDistinctNodes[i]) );
    }
    // We have to care for the nodes and their supply values also if they are present
    if (nodesTbl.size() > 0)
    {
        for (InputNodes::iterator it = nodesTbl.begin(); it != nodesTbl.end(); it++)
        {
            auto itD = *it;
            string extNodeID;
            unsigned int internalNodeID;
            double nodeSupply;
            extNodeID = itD.extNodeID;
            nodeSupply = itD.nodeSupply;
            //Get internal node ID from dictionary:
            //add values to NodeSupply
            //throws an exception if not found!
            try {
                internalNodeID = internalDistinctNodeIDs.at(extNodeID);
            }
            catch (exception& ex)
            {
                LOGGER::LogError("Original node ID " +extNodeID +" from nodes table not found in arcs!");
                throw ex;
            }
            //filter out transshipment nodes -> they're not important here.
            //They are generated when the C++ Solver is called
            try
            {
                if (nodeSupply != 0)
                {
                    NodeSupply sVal {extNodeID, nodeSupply};
                    nodeSupplies.insert( make_pair(internalNodeID, sVal) );
                }
            }
            catch (exception& ex)
            {
                LOGGER::LogWarning("renameNodes(): Error inserting supply values!");
                LOGGER::LogWarning(ex.what());
            }
        }
    }
    maxNodeCount = static_cast<unsigned int>(internalDistinctNodeIDs.size());
    maxArcCount = static_cast<unsigned int>(arcsTbl.size());
    currentArcCount = maxArcCount;
    currentNodeCount = maxNodeCount;
}

void Network::readNetworkFromTable(bool autoClean, bool oneWay)
{
    unordered_map<FTNode,DuplicateArcData> duplicates;

    for (InputArcs::iterator it = arcsTbl.begin(); it != arcsTbl.end(); ++it)
    {
        InputArc arc = *it;
        // String -> so content of nodeID fields could be string, int or double
        string externalArcID = arc.extArcID;
        string externalStartNode = arc.extFromNode;
        string externalEndNode = arc.extToNode;
        unsigned int internalStartNode;
        unsigned int internalEndNode;
        try{
            internalStartNode = internalDistinctNodeIDs.at(externalStartNode);
            internalEndNode = internalDistinctNodeIDs.at(externalEndNode);
        }
        catch (exception& ex)
        {
            LOGGER::LogError("readNetworkFromTable(): Error getting internal Nodes from internalDistinctNodIDs!");
            LOGGER::LogError(ex.what());
            throw ex;
        }

        bool isArcOneway = false;
        if (!onewayColName.empty())
        {
            if (arc.oneway == "Y")
                isArcOneway = true;
        }

        //CHECKS!
        //if found in eliminatedArcs go to next one
        if (eliminatedArcs.count(externalArcID) == 1) //could be eliminated from barriers
        //if (std::find(eliminatedArcs.begin(), eliminatedArcs.end(), externalArcID)!=eliminatedArcs.end())
            continue;
        if (internalStartNode == internalEndNode)
        {
            if (autoClean)
            {
                LOGGER::LogInfo("Loop at "+externalStartNode+ " - " + externalEndNode+ " ignored.");

                ExtFTNode ftNode {externalStartNode, externalEndNode};
                arcLoops.push_back(ftNode);
                eliminatedArcs.insert(externalArcID);
                //eliminatedArcsCount = eliminatedArcsCount + 1;
                continue; // ignorier einfach diese fehlerhafte Kante
            }
            else{
                LOGGER::LogWarning("Loop at "+externalStartNode+ " - " + externalEndNode+ "!");
            }
        }
        if (oneWay)//Global oneway
        {
            if (isArcOneway == false) //arc level oneway Y or N
            {
                //double the from-to-pair
                processArc(arc, internalStartNode, internalEndNode);
                //TODO: check for lookups!
                //processArc(row, externalArcID + "_toFrom", internalEndNode, internalStartNode);
                processArc(arc, internalEndNode, internalStartNode);
            }
            else //regular addition of directed from-to-pair
            {
                processArc(arc, internalStartNode, internalEndNode);
            }
        }
        else
        {
            processArc(arc, internalStartNode, internalEndNode);
        }
    }
    /*
    //TEST
    cout << "Internal Arcs:" << endl;
    for (Arcs::iterator it = internalArcData.begin(); it != internalArcData.end(); ++it)
    {
        auto itD = *it;
        FTNode ft = itD.first;
        ArcData arc = itD.second;
        cout << arc.extArcID << " " << ft.fromNode << " " << ft.toNode << " " << arc.cost << " " << arc.capacity << endl;
    }
    cout << "Eliminated Arcs" << endl;
    for (list<string>::iterator it = eliminatedArcs.begin(); it != eliminatedArcs.end(); ++it)
    {
        string extArcID = *it;
        cout << extArcID << endl;
    }
    cout << "Internal distinct nodes" << endl;
    for (unordered_map<ExtNodeID,NodeID>::iterator it = internalDistinctNodeIDs.begin(); it != internalDistinctNodeIDs.end(); ++it)
    {
        auto itD = *it;
        ExtNodeID extNodeID = itD.first;
        NodeID intNodeID = itD.second;
        cout << extNodeID << " " << intNodeID << endl;
    }*/

    //TODO
    //DBHELPER.EliminatedArcs = this.eliminatedArcs;
}
//TODO: isDirected evtl. von Klasse nehmen, nicht vom Config
void Network::processArc(InputArc arc, unsigned int internalStartNode,
                        unsigned int internalEndNode)
{
    bool isDirected = netXpertConfig.IsDirected;
    string externalArcID = arc.extArcID;

    FTNode inFromToPair {internalStartNode, internalEndNode};

    // Calculates the internal representation of the graph
    // [startNode,endNode],[oldArcID],[cost]
    double cost = arc.cost;
    double cap;
    if (arc.capacity != DOUBLE_NULL)
        cap = arc.capacity;
    else
        cap = DOUBLE_INFINITY; //We do not have a column for caps, so there is no limit

    ArcData arcIDAndCost {externalArcID, cost, cap};

    if (isDirected)
    {
        if (internalArcData.count(inFromToPair) == 0) // 0: key not found, 1: key found
            internalArcData.insert( make_pair(inFromToPair, arcIDAndCost));
        // we have a duplicate arc
        else
        {
            // compare the costs and add only the cheapest
            ArcData outArcData;
            outArcData = internalArcData.at(inFromToPair);
            //LOGGER::LogDebug(to_string(outArcData.cost));
            //LOGGER::LogDebug(to_string(cost));
            if (outArcData.cost > cost)
            {
                //remove old arc
                internalArcData.erase(inFromToPair);
                eliminatedArcs.insert(outArcData.extArcID);
                //eliminatedArcsCount = eliminatedArcsCount + 1;
                //and add new one
                internalArcData.insert( make_pair(inFromToPair, arcIDAndCost));
            }
            //else keep the old one
        }
    }
    else //undirected
    {
        // Hier muss auch das vertauschte FromTo Paar geprüft werden;
        FTNode inToFromPair {inFromToPair.toNode, inFromToPair.fromNode};

        // erst wenn auch bei der Vertauschung auch keine Kante vorhanden ist, füge sie hinzu
        if (internalArcData.count(inFromToPair) +
            internalArcData.count(inToFromPair) == 0)
        {
            internalArcData.insert(make_pair(inFromToPair, arcIDAndCost)); //OK
        }
        // we have a duplicate arc
        else
        {
            // compare the costs and add only the cheapest
            ArcData outArcData;
            // if not found regular, check for swapped
            bool checkSwapped = false;
            if (internalArcData.count(inFromToPair) == 0)
            {
                checkSwapped = true;
                outArcData = internalArcData.at(inToFromPair);
            }
            else
            {
                outArcData = internalArcData.at(inFromToPair);
            }

            if (outArcData.cost > cost)
            {
                if (checkSwapped)
                {
                    //remove old arc
                    internalArcData.erase(inToFromPair);
                    eliminatedArcs.insert(outArcData.extArcID);
                    //eliminatedArcsCount = eliminatedArcsCount + 1;
                    //and add new one
                    internalArcData.insert(make_pair(inToFromPair, arcIDAndCost));
                }
                else
                {
                    //remove old arc
                    internalArcData.erase(inFromToPair);
                    eliminatedArcs.insert(outArcData.extArcID);
                    //eliminatedArcsCount = eliminatedArcsCount + 1;
                    //and add new one
                    internalArcData.insert(make_pair(inFromToPair, arcIDAndCost));
                }
            }
            //else keep the old one
        }
    }
}
void Network::processBarriers(){}
