#include "network.h"

using namespace std;
using namespace netxpert;

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

        NETXPERT_CNFG = cnfg;

        internalArcData.reserve(_arcsTbl.size());
        newArcs.reserve(2);

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

        NETXPERT_CNFG = cnfg;

        internalArcData.reserve(_arcsTbl.size());
        newArcs.reserve(2);

        this->arcsTbl = _arcsTbl;
    }
    catch (exception& ex)
    {
        LOGGER::LogFatal("Network ctor: Error!");
        LOGGER::LogFatal(ex.what());
    }
}

//TODO TESTME
Network::Network(Arcs arcData, unordered_map<ExtArcID,IntNodeID> distinctNodeIDs,
                        NodeSupplies _nodeSupplies)
{
    internalArcData = arcData;
    internalDistinctNodeIDs = distinctNodeIDs;
    //swap
    for (auto& n : internalDistinctNodeIDs)
        swappedInternalDistinctNodeIDs.insert( make_pair(n.second, n.first ) );

    nodeSupplies = _nodeSupplies;
}

Network::~Network()
{
    //dtor
}

vector<pair<unsigned int, string>> Network::LoadStartNodes(const vector<NewNode>& newNodes, int treshold,
                                                            string arcsTableName, string geomColumnName,
                                                                ColumnMap& cmap, bool withCapacity)
{
    auto qry = DBHELPER::PrepareGetClosestArcQuery(arcsTableName, geomColumnName,
                                            cmap, ArcIDColumnDataType::Number, withCapacity);
    vector<pair<unsigned int, string>> startNodes;

    for (auto& startNode : newNodes)
    {
        if (startNode.supply > 0)
        {
            LOGGER::LogDebug("Loading Node "+ startNode.extNodeID + "..");
            try
            {
                unsigned int newStartNodeID = AddStartNode(startNode, treshold, *qry, withCapacity);
                startNodes.push_back( make_pair(newStartNodeID, startNode.extNodeID ) );
                LOGGER::LogDebug("New Start Node ID " + to_string(newStartNodeID)  + " - " + startNode.extNodeID);
            }
            catch (exception& ex)
            {
                //pass
                cout << "Exception!" << endl;
            }
        }
    }
    return startNodes;
}

vector<pair<unsigned int, string>> Network::LoadEndNodes(const vector<NewNode>& newNodes, int treshold,
                                                            string arcsTableName, string geomColumnName,
                                                                ColumnMap& cmap, bool withCapacity)
{
    auto qry = DBHELPER::PrepareGetClosestArcQuery(arcsTableName, geomColumnName,
                                            cmap, ArcIDColumnDataType::Number, withCapacity);
    vector<pair<unsigned int, string>> endNodes;

    for (auto endNode : newNodes)
    {
        if (endNode.supply < 0)
        {
            LOGGER::LogDebug("Loading Node "+ endNode.extNodeID + "..");
            try
            {
                unsigned int newEndNodeID = AddEndNode(endNode, treshold, *qry, withCapacity);
                endNodes.push_back( make_pair(newEndNodeID, endNode.extNodeID ) );
                LOGGER::LogDebug("New End Node ID " + to_string(newEndNodeID) + " - " + endNode.extNodeID);
            }
            catch (exception& ex)
            {
                //pass
                cout << "Exception!" << endl;
            }
        }
    }
    return endNodes;
}

//Vorgehen:
//1. Hol die nächste Kante (per Spatialite) ArcID + Geometrie
//2. Wurde die Kante schon aufgebrochen oder nicht?
//3. Hole Geometrie neu wenn nötig (aus den newArcs)
//4. isPointOnLine, position of closestPoint (Start / End ) alles in Memory (egal ob aufgebrochene Kante oder vorhandene Kante)
//5. Splitte Kante
// --> einmaliger DB-Zugriff, Rest in memory processing
unsigned int Network::AddStartNode(const NewNode& newStartNode, int treshold, SQLite::Statement& closestArcQry,
                                    bool withCapacity)
{
    unordered_map<string, InternalArc> newSegments;
    newSegments.reserve(2);

    const string extNodeID = newStartNode.extNodeID;
    const Coordinate startPoint = newStartNode.coord;
    const double nodeSupply = newStartNode.supply;

    //if (!startPoint.x)
    //    throw new InvalidValueException("Start coordinate must not be null!");

    //1. Suche die nächste Line und den nächsten Punkt auf der Linie zum temporaeren Punkt
    ExtClosestArcAndPoint closestArcAndPoint = DBHELPER::GetClosestArcFromPoint(startPoint,
            treshold, closestArcQry, withCapacity);

    string extArcID = closestArcAndPoint.extArcID; // kann noch auf leeren string gesetzt werden, wenn Kante bereits aufgebrochen wurde
    //const Coordinate closestPoint = closestArcAndPoint.closestPoint;
    Geometry& closestArc = *closestArcAndPoint.arcGeom;
    const string extFromNode = closestArcAndPoint.extFromNode;
    const string extToNode = closestArcAndPoint.extToNode;
    const double cost = closestArcAndPoint.cost;
    const double capacity = closestArcAndPoint.capacity;

    //2. Wenn die Kante bereits aufgebrochen wurde (=Eintrag in oldArcs.ArcData.extArcID, hol
    // die nächste Linie aus NewArcs
    bool arcHasBeenSplitAlready = false;
    if (swappedOldArcs.count(extArcID) > 0) //found
    {
        arcHasBeenSplitAlready = true;
    }

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
                    AddedPoint pVal  {extNodeID, startPoint};
                    addedStartPoints.insert(make_pair(newID, pVal));

                    //add nodeSupply
                    NodeSupply supVal  {extNodeID, nodeSupply};
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
                    AddedPoint val  {extNodeID, startPoint};
                    addedStartPoints.insert(make_pair(newID, val));

                    //add nodeSupply
                    NodeSupply supVal  {extNodeID, nodeSupply};
                    nodeSupplies.insert(make_pair(newID, supVal));

                    return newID;
                }
            }
        }
        else //point lies somewhere between start and end coordinate of the line --> split!
        {
            //split line
            ExternalArc ftPair  {extFromNode, extToNode};
            ArcData val  {extArcID, cost, capacity};
            pair<ExternalArc,ArcData> arcData = make_pair(ftPair, val);

            auto splittedLine = GetSplittedClosestOldArcToPoint(startPoint, treshold, arcData, closestArc);

            //TODO: Direction!
            vector<InternalArc> newSegmentIDs = insertNewStartNode(true, splittedLine, extArcID, extNodeID, startPoint);
            newSegments.insert( make_pair("toOrigArc", newSegmentIDs[0]) );
            newSegments.insert( make_pair("fromOrigArc", newSegmentIDs[1]) );

            unsigned int newID = newSegmentIDs[0].toNode;

            //add nodeSupply
            NodeSupply supVal  {extNodeID, nodeSupply};
            nodeSupplies.insert( make_pair(newID, supVal));

            return newID;
        }
    }
    else // Kante schon aufgebrochen
    {
        //split line
        auto splittedLine = GetSplittedClosestNewArcToPoint(startPoint, treshold);

        //TODO: Direction!
        //TODO: extArcID bei schon aufgebrochenen Kanten
        extArcID = "";
        vector<InternalArc> newSegmentIDs = insertNewStartNode(true, splittedLine, extArcID, extNodeID, startPoint);
        newSegments.insert( make_pair("toOrigArc", newSegmentIDs[0]) );
        newSegments.insert( make_pair("fromOrigArc", newSegmentIDs[1]) );

        unsigned int newID = newSegmentIDs[0].toNode;

        //add nodeSupply
        NodeSupply supVal  {extNodeID, nodeSupply};
        nodeSupplies.insert( make_pair(newID, supVal));

        return newID;
    }
}


vector<InternalArc> Network::insertNewStartNode(bool isDirected, SplittedArc& splittedLine, string extArcID,
                                            string extNodeID, const Coordinate& startPoint)
{
    vector<InternalArc> result;
    result.reserve(2);

    unsigned int newNodeID = GetMaxNodeCount() + 1;
    //Insert of new node between old start node and old to node
    InternalArc newArc1 { splittedLine.ftNode.fromNode, newNodeID };
    InternalArc newArc2 { newNodeID, splittedLine.ftNode.toNode };

    //ArcData geht aus der gesplitteten Linie hervor
    // extArcID kann aber auch leer sein
    ArcData arcData { extArcID, splittedLine.cost, splittedLine.capacity};
    if (oldArcs.count(splittedLine.ftNode) == 0)
    {
        oldArcs.insert( make_pair(splittedLine.ftNode, arcData) );
        //TODO - checkme
        SwappedOldArc swappedOldArc  {splittedLine.ftNode, splittedLine.cost, splittedLine.capacity };
        //check extArcID for empty string
        swappedOldArcs.insert( make_pair(extArcID, swappedOldArc) );
    }

    //Add new Edges with new oldEdgeIDS and relative cost to geometry length
    //Capacity is not relative!
    const MultiLineString& completeLine = *splittedLine.arcGeom;

    //TODO: shared ptr problem with getGeometryN()
    //Workaround: raw pointers and references; not delete ptr necessary (TEST!)

    // double free corruption!
    /*auto ptr1 = unique_ptr<const Geometry>(completeLine.getGeometryN(0));
    auto ptr2 = unique_ptr<const Geometry>(completeLine.getGeometryN(1));
    const Geometry& g1 = *ptr1;
    const Geometry& g2 = *ptr2;
    const LineString& segment1 = dynamic_cast<const LineString&>(g1);
    const LineString& segment2 = dynamic_cast<const LineString&>(g2);*/

    auto ptr1 = completeLine.getGeometryN(0);
    auto ptr2 = completeLine.getGeometryN(1);
    const Geometry& g1 = *ptr1;
    const Geometry& g2 = *ptr2;
    const LineString& segment1 = dynamic_cast<const LineString&>(g1);
    const LineString& segment2 = dynamic_cast<const LineString&>(g2);

    //OLD with shared_ptrs
    //shared_ptr<const Geometry> gPtr1 (completeLine.getGeometryN(0));
    /*shared_ptr<const Geometry> gPtr2 (completeLine.getGeometryN(1));
    */
    /*shared_ptr<const LineString> sPtr1 ( dynamic_pointer_cast<const LineString>(gPtr1));
    shared_ptr<const LineString> sPtr2 ( dynamic_pointer_cast<const LineString>(gPtr2));
    const LineString& segment1 = *sPtr1;
    const LineString& segment2 = *sPtr2;
    */

    //double newArcCap = splittedLine.capacity;
    double newArc1Cost = getRelativeValueFromGeomLength(splittedLine.cost, completeLine, segment1);
    double newArc2Cost = getRelativeValueFromGeomLength(splittedLine.cost, completeLine, segment2);

    ArcData newArc1data {"", newArc1Cost, splittedLine.capacity};
    ArcData newArc2data {"", newArc2Cost, splittedLine.capacity};

    if (isDirected)
    {
        internalArcData.insert( make_pair(newArc1, newArc1data) );
        internalArcData.insert( make_pair(newArc2, newArc2data) );

        //Save newEdges and set Geometry
        LineString* ptr1 = dynamic_cast<LineString*>(segment1.clone());
        LineString* ptr2 = dynamic_cast<LineString*>(segment2.clone());

        NewArc n1 { shared_ptr<LineString>( ptr1) , AddedNodeType::StartArc, newArc1Cost, splittedLine.capacity};
        NewArc n2 { shared_ptr<LineString>( ptr2) , AddedNodeType::StartArc, newArc2Cost, splittedLine.capacity};

        newArcs.insert ( make_pair(newArc1, n1 ) );
        newArcs.insert ( make_pair(newArc2, n2 ) );

        //Remove oldEdge from internal mapping dict
        internalArcData.erase(splittedLine.ftNode);

    }
    else {
        //double everything
        internalArcData.insert( make_pair(newArc1, newArc1data) );
        internalArcData.insert( make_pair(newArc2, newArc2data) );

        internalArcData.insert( make_pair(InternalArc { newArc1.toNode, newArc1.fromNode }, newArc1data) );
        internalArcData.insert( make_pair(InternalArc { newArc2.toNode, newArc2.fromNode }, newArc2data) );

        //Save newEdges and set Geometry
        LineString* ptr1 = dynamic_cast<LineString*>(segment1.clone());
        LineString* ptr2 = dynamic_cast<LineString*>(segment2.clone());

        NewArc n1 { shared_ptr<LineString>( ptr1) , AddedNodeType::StartArc, newArc1Cost, splittedLine.capacity};
        NewArc n2 { shared_ptr<LineString>( ptr2) , AddedNodeType::StartArc, newArc2Cost, splittedLine.capacity};

        //regular direction
        newArcs.insert ( make_pair(newArc1, n1 ) );
        newArcs.insert ( make_pair(newArc2, n2 ) );

        //reverse direction
        newArcs.insert ( make_pair(InternalArc { newArc1.toNode, newArc1.fromNode }, n1 ) );
        newArcs.insert ( make_pair(InternalArc { newArc2.toNode, newArc2.fromNode }, n2 ) );

        //Remove oldEdge from internal mapping dict
        internalArcData.erase(splittedLine.ftNode);
        //reverse direction
        internalArcData.erase(InternalArc { splittedLine.ftNode.toNode, splittedLine.ftNode.fromNode });
    }

    addedStartPoints.insert( make_pair(newNodeID, AddedPoint {extNodeID, startPoint}) );

    const string newExtNodeID = extArcID + "_" + to_string(newNodeID);
    internalDistinctNodeIDs.insert( make_pair(newExtNodeID, newNodeID  ) );
    swappedInternalDistinctNodeIDs.insert( make_pair(newNodeID, newExtNodeID ) );

    //swappedOldArcs.insert()
    SwappedOldArc swappedOldArc  {splittedLine.ftNode, splittedLine.cost, splittedLine.capacity };
    swappedOldArcs.insert( make_pair(extArcID, swappedOldArc) );

    result.push_back(newArc1);
    result.push_back(newArc2);

    //delete ptr1;
    //delete ptr2;

    return result;
}

unsigned int Network::AddEndNode(const NewNode& newEndNode, int treshold, SQLite::Statement& closestArcQry, bool withCapacity)
{
    unordered_map<string, InternalArc> newSegments;
    newSegments.reserve(2);

    const string extNodeID = newEndNode.extNodeID;
    const Coordinate endPoint = newEndNode.coord;
    const double nodeSupply = newEndNode.supply;

    //if (!endPoint.x)
    //    throw new InvalidValueException("End coordinate must not be null!");

    //1. Suche die nächste Line und den nächsten Punkt auf der Linie zum temporaeren Punkt
    ExtClosestArcAndPoint closestArcAndPoint = DBHELPER::GetClosestArcFromPoint(endPoint,
            treshold, closestArcQry, withCapacity);

    if (!closestArcAndPoint.arcGeom)
        cout << "Geom really empty" << endl;

    string extArcID = closestArcAndPoint.extArcID; //kann noch auf leeren String gesetzt werden bei bereits aufgebrochenen Kante
    const Coordinate closestPoint = closestArcAndPoint.closestPoint;
    Geometry& closestArc = *closestArcAndPoint.arcGeom;
    const string extFromNode = closestArcAndPoint.extFromNode;
    const string extToNode = closestArcAndPoint.extToNode;
    const double cost = closestArcAndPoint.cost;
    const double capacity = closestArcAndPoint.capacity;

    //2. Wenn die Kante bereits aufgebrochen wurde (=Eintrag in oldArcs.ArcData.extArcID, hol
    // die nächste Linie aus NewArcs
    bool arcHasBeenSplitAlready = false;
    if (swappedOldArcs.count(extArcID) > 0) //found
    {
        arcHasBeenSplitAlready = true;
    }

    if (!arcHasBeenSplitAlready)  //es wurde die Kante noch nicht aufgebrochen
    {
        // Prüfe, ob der nächste Punkt gleichzeitig der Start - oder der Endpunkt einer Linie ist
        auto locationOfPoint = GetLocationOfPointOnLine(endPoint, closestArc);
        switch ( locationOfPoint  )
        {
            case StartOrEndLocationOfLine::Start:
                LOGGER::LogDebug("Closest Point for end node "+extNodeID
                    +" is identical to a start node of the network!");
                break;
            case StartOrEndLocationOfLine::End:
                LOGGER::LogDebug("Closest Point for end node "+extNodeID
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
                    AddedPoint pVal  {extNodeID, endPoint};
                    addedEndPoints.insert(make_pair(newID, pVal));

                    //add nodeSupply
                    NodeSupply supVal  {extNodeID, nodeSupply};
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
                    AddedPoint val  {extNodeID, endPoint};
                    addedEndPoints.insert(make_pair(newID, val));

                    //add nodeSupply
                    NodeSupply supVal  {extNodeID, nodeSupply};
                    nodeSupplies.insert(make_pair(newID, supVal));

                    return newID;
                }
            }
        }
        else //point lies somewhere between start and end coordinate of the line --> split!
        {
            //split line
            ExternalArc ftPair  {extFromNode, extToNode};
            ArcData val  {extArcID, cost, capacity};
            pair<ExternalArc,ArcData> arcData = make_pair(ftPair, val);

            auto splittedLine = GetSplittedClosestOldArcToPoint(endPoint, treshold, arcData, closestArc);

            //TODO: Direction!
            vector<InternalArc> newSegmentIDs = insertNewEndNode(true, splittedLine, extArcID, extNodeID, endPoint);
            newSegments.insert( make_pair("toOrigArc", newSegmentIDs[0]) );
            newSegments.insert( make_pair("fromOrigArc", newSegmentIDs[1]) );

            unsigned int newID = newSegmentIDs[0].toNode;

            //add nodeSupply
            NodeSupply supVal  {extNodeID, nodeSupply};
            nodeSupplies.insert( make_pair(newID, supVal));

            return newID;
        }
    }
    else // Kante schon aufgebrochen
    {
        //split line
        auto splittedLine = GetSplittedClosestNewArcToPoint(endPoint, treshold);

        //TODO: Direction!
        //TODO: extArcID leer, wenn Kante bereits aufgebrochen
        extArcID = "";
        vector<InternalArc> newSegmentIDs = insertNewEndNode(true, splittedLine, extArcID, extNodeID, endPoint);
        newSegments.insert( make_pair("toOrigArc", newSegmentIDs[0]) );
        newSegments.insert( make_pair("fromOrigArc", newSegmentIDs[1]) );

        unsigned int newID = newSegmentIDs[0].toNode;

        //add nodeSupply
        NodeSupply supVal  {extNodeID, nodeSupply};
        nodeSupplies.insert( make_pair(newID, supVal));

        return newID;
    }
}

vector<InternalArc> Network::insertNewEndNode(bool isDirected, SplittedArc& splittedLine, string extArcID, string extNodeID,
                                            const Coordinate& endPoint)
{
    vector<InternalArc> result;
    result.reserve(2);

    unsigned int newNodeID = GetMaxNodeCount() + 1;
    //Insert of new node between old start node and old to node
    InternalArc newArc1 { splittedLine.ftNode.fromNode, newNodeID };
    InternalArc newArc2 { newNodeID, splittedLine.ftNode.toNode };

    //ArcData geht aus der gesplitteten Linie hervor
    // extArcID kann aber auch leer sein
    ArcData arcData { extArcID, splittedLine.cost, splittedLine.capacity};
    if (oldArcs.count(splittedLine.ftNode) == 0)
    {
        oldArcs.insert( make_pair(splittedLine.ftNode, arcData) );
        //TODO - checkme
        SwappedOldArc swappedOldArc  {splittedLine.ftNode, splittedLine.cost, splittedLine.capacity };
        //check extArcID for empty string
        swappedOldArcs.insert( make_pair(extArcID, swappedOldArc) );
    }

    //Add new Edges with new oldEdgeIDS and relative cost to geometry length
    //Capacity is not relative!
    //TODO: shared ptr problem with getGeometryN()
    //Workaround: raw pointers and references; not delete ptr necessary (TEST!)
    const MultiLineString& completeLine = *splittedLine.arcGeom;

    auto ptr1 = completeLine.getGeometryN(0);
    auto ptr2 = completeLine.getGeometryN(1);
    const Geometry& g1 = *ptr1;
    const Geometry& g2 = *ptr2;
    const LineString& segment1 = dynamic_cast<const LineString&>(g1);
    const LineString& segment2 = dynamic_cast<const LineString&>(g2);

    /*
    Causes double free corruption
    auto ptr1 = unique_ptr<const Geometry>(completeLine.getGeometryN(0));
    auto ptr2 = unique_ptr<const Geometry>(completeLine.getGeometryN(1));
    const Geometry& g1 = *ptr1;
    const Geometry& g2 = *ptr2;
    const LineString& segment1 = dynamic_cast<const LineString&>(g1);
    const LineString& segment2 = dynamic_cast<const LineString&>(g2);*/

    //OLD with shared_ptrs
    //shared_ptr<const Geometry> gPtr1 (completeLine.getGeometryN(0));
    /*shared_ptr<const Geometry> gPtr2 (completeLine.getGeometryN(1));
    shared_ptr<const LineString> sPtr1 ( dynamic_pointer_cast<const LineString>(gPtr1));
      shared_ptr<const LineString> sPtr2 ( dynamic_pointer_cast<const LineString>(gPtr2));
	  const LineString& segment1 = *sPtr1;
	  const LineString& segment2 = *sPtr2;*/

    //double newArcCap = splittedLine.capacity;
    double newArc1Cost = getRelativeValueFromGeomLength(splittedLine.cost, completeLine, segment1);
    double newArc2Cost = getRelativeValueFromGeomLength(splittedLine.cost, completeLine, segment2);

    ArcData newArc1data {"", newArc1Cost, splittedLine.capacity};
    ArcData newArc2data {"", newArc2Cost, splittedLine.capacity};

    if (isDirected)
    {
        internalArcData.insert( make_pair(newArc1, newArc1data) );
        internalArcData.insert( make_pair(newArc2, newArc2data) );

        //Save newEdges and set Geometry
        LineString* ptr1 = dynamic_cast<LineString*>(segment1.clone());
        LineString* ptr2 = dynamic_cast<LineString*>(segment2.clone());

        NewArc n1 { shared_ptr<LineString>(ptr1) , AddedNodeType::EndArc, newArc1Cost, splittedLine.capacity};
        NewArc n2 { shared_ptr<LineString>(ptr2) , AddedNodeType::EndArc, newArc2Cost, splittedLine.capacity};

        newArcs.insert ( make_pair(newArc1, n1 ) );
        newArcs.insert ( make_pair(newArc2, n2 ) );

        //Remove oldEdge from internal mapping dict
        internalArcData.erase(splittedLine.ftNode);

    }
    else {
        //double everything
        internalArcData.insert( make_pair(newArc1, newArc1data) );
        internalArcData.insert( make_pair(newArc2, newArc2data) );

        internalArcData.insert( make_pair(InternalArc { newArc1.toNode, newArc1.fromNode }, newArc1data) );
        internalArcData.insert( make_pair(InternalArc { newArc2.toNode, newArc2.fromNode }, newArc2data) );

        //Save newEdges and set Geometry
        LineString* ptr1 = dynamic_cast<LineString*>(segment1.clone());
        LineString* ptr2 = dynamic_cast<LineString*>(segment2.clone());

        NewArc n1 { shared_ptr<LineString>(ptr1) , AddedNodeType::EndArc, newArc1Cost, splittedLine.capacity};
        NewArc n2 { shared_ptr<LineString>(ptr2) , AddedNodeType::EndArc, newArc2Cost, splittedLine.capacity};

        //regular direction
        newArcs.insert ( make_pair(newArc1, n1 ) );
        newArcs.insert ( make_pair(newArc2, n2 ) );

        //reverse direction
        newArcs.insert ( make_pair(InternalArc { newArc1.toNode, newArc1.fromNode }, n1 ) );
        newArcs.insert ( make_pair(InternalArc { newArc2.toNode, newArc2.fromNode }, n2 ) );

        //Remove oldEdge from internal mapping dict
        internalArcData.erase(splittedLine.ftNode);
        //reverse direction
        internalArcData.erase(InternalArc { splittedLine.ftNode.toNode, splittedLine.ftNode.fromNode });
    }

    addedEndPoints.insert( make_pair(newNodeID, AddedPoint {extNodeID, endPoint}) );

    const string newExtNodeID = extArcID + "_" + to_string(newNodeID);
    internalDistinctNodeIDs.insert( make_pair(newExtNodeID, newNodeID  ) );
    swappedInternalDistinctNodeIDs.insert( make_pair(newNodeID, newExtNodeID ) );

    result.push_back(newArc1);
    result.push_back(newArc2);

    return result;
}
// Only for unbroken, original network (e.g. MST), if result DB is equal to netXpert DB
// NO Geometry Handling --> creates always a subset of the original geometries
void Network::ProcessResultArcs(string orig, string dest, double cost, double capacity, double flow,
                                        const string& arcIDs,
                                      const string& resultTableName)
{
    saveResults(orig, dest, cost, capacity, flow, arcIDs, resultTableName);
}

//With Geometry Handling
void Network::ProcessResultArcs(string orig, string dest, double cost, double capacity, double flow,
                                        const string& arcIDs, vector<InternalArc>& routeNodeArcRep,
                                        const string& resultTableName, DBWriter& writer)
{
    vector<Geometry*> routeParts;

    switch (NETXPERT_CNFG.GeometryHandling)
    {
        case GEOMETRY_HANDLING::RealGeometry:
            {
                routeParts = processRouteParts(routeNodeArcRep);
                //nullptr for query
                LOGGER::LogWarning("Depointerization of a nullptr..: std::unique_ptr<SQLite::Statement> qry");
                std::unique_ptr<SQLite::Statement> qry;
                saveResults(orig, dest, cost, capacity, flow, arcIDs, routeParts, resultTableName, writer, *qry);
            }
            break;
        default:
            //GEOMETRY_HANDLING::NoGeometry:
            //GEOMETRY_HANDLING::StraightLines:
            {
                saveResults(orig, dest, cost, capacity, flow, resultTableName, writer);
            }
            break;
    }
}
//With Geometry Handling and prepared insert query
void Network::ProcessResultArcs(string orig, string dest, double cost, double capacity, double flow,
                                        const string& arcIDs, vector<InternalArc>& routeNodeArcRep,
                                        const string& resultTableName, DBWriter& writer, SQLite::Statement& qry)
{
    vector<Geometry*> routeParts;

    switch (NETXPERT_CNFG.GeometryHandling)
    {
        case GEOMETRY_HANDLING::RealGeometry:
            {
                routeParts = processRouteParts(routeNodeArcRep);
                saveResults(orig, dest, cost, capacity, flow, arcIDs, routeParts, resultTableName, writer, qry);
            }
            break;
        default:
            //GEOMETRY_HANDLING::NoGeometry:
            //GEOMETRY_HANDLING::StraightLines:
            {
                saveResults(orig, dest, cost, capacity, flow, resultTableName, writer, qry);
            }
            break;
    }
}

vector<Geometry*> Network::processRouteParts(vector<InternalArc>& routeNodeArcRep)
{
    //order of start or end segments do not matter
    vector<Geometry*> routeParts;

    if (addedStartPoints.size() > 0)
    {
        // 1. Add relevant start edges, that are part of the route
        for (auto& arc : newArcs)
        {
            auto key = arc.first;
            auto val = arc.second;

            //cout << arc.first.fromNode << "->" << arc.first.toNode << endl;

            if (val.nodeType == AddedNodeType::StartArc)
            {
                if (NETXPERT_CNFG.IsDirected)
                {
                    if ( std::find(routeNodeArcRep.begin(), routeNodeArcRep.end(), key) != routeNodeArcRep.end() )
                    {
                        /*if (orig == key.fromNode) //touching arc
                        {*/
                        //auto clonedGeom = val.arcGeom.clone(); // clone - sonst verschwindet die Geometrie beim zweiten durchlauf
                        auto clonedGeom = val.arcGeom;//val.arcGeom.clone();

                        //shared_ptr<Geometry> geo (clonedGeom);
                        /*auto it = tmpRes.begin();
                        tmpRes.insert(it, geo);*/
                        routeParts.push_back(clonedGeom.get());
                        //cout << "found (dir): " << key.fromNode << "->" << key.toNode << endl;
                        /*}
                        else {
                            cout << "Int extArcID: " << internalArcData.at(key).extArcID<< endl;
                        }*/
                    }
                }
                // Undirected case
                else
                {
                    bool segmentFound = false;

                    if (std::find(routeNodeArcRep.begin(), routeNodeArcRep.end(), key) != routeNodeArcRep.end())
                    {
                        /*if (orig == key.fromNode)
                        {*/
                            auto clonedGeom = val.arcGeom;//val.arcGeom.clone();
                            //shared_ptr<Geometry> geo (clonedGeom);
                            segmentFound = true;
                            /*auto it = tmpRes.begin();
                            tmpRes.insert(it, geo);*/
                            routeParts.push_back(clonedGeom.get());
                            //cout << "found (undir [normal]): " << key.fromNode << "->" << key.toNode << endl;
                        /*}
                         else {
                            cout << "Int extArcID: " << internalArcData.at(key).extArcID<< endl;
                        }*/
                    }
                    //Check also reverse if not found previously
                    if (segmentFound == false)
                    {
                        if (std::find(routeNodeArcRep.begin(),
                                    routeNodeArcRep.end(),
                                    InternalArc {key.toNode, key.fromNode}) != routeNodeArcRep.end() )
                        {
                            /*if (orig == key.toNode)
                            {*/
                                auto clonedGeom = val.arcGeom;//val.arcGeom.clone();
                                //shared_ptr<Geometry> geo (clonedGeom);
                                /*auto it = tmpRes.begin();
                                tmpRes.insert(it, geo);*/
                                routeParts.push_back(clonedGeom.get());
                                //cout << "found (undir [reverse]): " << key.toNode << "->" << key.fromNode << endl;
                            /*}
                            else {
                                cout << "Int extArcID: " << internalArcData.at(InternalArc {key.toNode, key.fromNode}).extArcID<< endl;
                            }*/
                        }
                    }
                }
            }
        }
    }
    if (addedEndPoints.size() > 0)
    {
        /*cout << "size of addedEndPoints(): " << addedEndPoints.size() << endl;
        cout << "size of newArcs: " << newArcs.size() << endl;*/
        // 1. Add relevant end edges, that are part of the route
        for (auto& arc : newArcs)
        {
            //cout << arc.first.fromNode << "->" << arc.first.toNode << endl;
            auto key = arc.first;
            auto val = arc.second;

            if (arc.second.nodeType == AddedNodeType::EndArc)
            {
                if (NETXPERT_CNFG.IsDirected)
                {
                    if (std::find(routeNodeArcRep.begin(), routeNodeArcRep.end(), key) != routeNodeArcRep.end())
                    {
                        /*if (dest == key.toNode)
                        {*/
                            auto clonedGeom = val.arcGeom;//val.arcGeom.clone();
                            //shared_ptr<Geometry> geo (clonedGeom);
                            /*auto it = tmpRes.begin();
                            tmpRes.insert(it+1, geo); //end segment -> it+1*/
                            routeParts.push_back(clonedGeom.get());
                        //}
                    }
                }
                // Undirected case
                else
                {
                    bool segmentFound = false;

                    if (std::find(routeNodeArcRep.begin(), routeNodeArcRep.end(), key) != routeNodeArcRep.end())
                    {
                        /*if (dest == key.toNode)
                        {*/
                            auto clonedGeom = val.arcGeom;//val.arcGeom.clone();
                            //shared_ptr<Geometry> geo (clonedGeom);
                            segmentFound = true;
                            /*auto it = tmpRes.begin();
                            tmpRes.insert(it+1, geo); //end segment -> it+1*/
                            routeParts.push_back(clonedGeom.get());
                        //}
                    }
                    //Check also reverse if not found previously
                    if (segmentFound == false)
                    {
                        if (std::find(routeNodeArcRep.begin(),
                                routeNodeArcRep.end(),
                                InternalArc {key.toNode, key.fromNode}) != routeNodeArcRep.end() )
                        {
                            /*if (dest == key.fromNode)
                            {*/
                                auto clonedGeom = val.arcGeom;//val.arcGeom.clone();
                                //shared_ptr<Geometry> geo (clonedGeom);
                                /*auto it = tmpRes.begin();
                                tmpRes.insert(it+1, geo); //end segment -> it+1*/
                                routeParts.push_back(clonedGeom.get());
                            //}
                        }
                    }
                }
            }
        }
    }
    return routeParts;
}

void Network::ConvertInputNetwork(bool autoClean)
{
    renameNodes();

    processBarriers();

    //Oneway wird nur bei gerichtetem Netzwerk berücksichtigt
    if (!onewayColName.empty() && NETXPERT_CNFG.IsDirected == true)
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
vector<string> Network::GetOriginalArcIDs(const vector<InternalArc>& ftNodes, bool isDirected) const
{
    vector<string> resultIDs;

    if (isDirected)
    {
        for (const InternalArc& ftNode : ftNodes)
        {
            if (internalArcData.count(ftNode) > 0)
            {
                auto arcData = internalArcData.at(ftNode);
                if (!arcData.extArcID.empty())
                    resultIDs.push_back(arcData.extArcID);
            }
        }
    }
    else
    {
        for (const InternalArc& ftNode : ftNodes)
        {
            if (internalArcData.count(ftNode) > 0)
            {
                auto arcData = internalArcData.at(ftNode);
                if (!arcData.extArcID.empty())
                    resultIDs.push_back(arcData.extArcID);
            }
            else {
                //reverse direction also
                InternalArc flippedFTNode  {ftNode.toNode, ftNode.fromNode};
                if (internalArcData.count(flippedFTNode) > 0)
                {
                    auto arcData = internalArcData.at(flippedFTNode);
                    if (!arcData.extArcID.empty())
                        resultIDs.push_back(arcData.extArcID);
                }
            }
        }
    }
    return resultIDs;
}

vector<ArcData> Network::GetOriginalArcData(const vector<InternalArc>& ftNodes, bool isDirected) const
{
    vector<ArcData> result;

    if (isDirected)
    {
        for (const InternalArc& ftNode : ftNodes)
        {
            if (internalArcData.count(ftNode) > 0)
            {
                auto arcData = internalArcData.at(ftNode);
                if (!arcData.extArcID.empty())
                    result.push_back(arcData);
            }
        }
    }
    else
    {
        for (const InternalArc& ftNode : ftNodes)
        {
            if (internalArcData.count(ftNode) > 0)
            {
                auto arcData = internalArcData.at(ftNode);
                if (!arcData.extArcID.empty())
                    result.push_back(arcData);
            }
            else {
                //reverse direction also
                InternalArc flippedFTNode  {ftNode.toNode, ftNode.fromNode};
                if (internalArcData.count(flippedFTNode) > 0)
                {
                    auto arcData = internalArcData.at(flippedFTNode);
                    if (!arcData.extArcID.empty())
                        result.push_back(arcData);
                }
            }
        }
    }
    return result;
}

void Network::GetOriginalArcDataAndFlow(const list<ArcDataAndFlow>& origArcDataAndFlow, const list<InternalArc>& startEndNodes, bool isDirected){}

double Network::getRelativeValueFromGeomLength(const double attrValue, const MultiLineString& completeLine,
                                                    const LineString& segment)
{
    return ( segment.getLength() / completeLine.getLength() ) * attrValue;
}

unsigned int Network::GetInternalNodeID(string externalNodeID)
{
    unsigned int internalNodeID {};
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
    string externalNodeID {};
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
        LOGGER::LogWarning("Original start/end node ID " +to_string(internalNodeID) +" could not be looked up!");
        throw ex;
    }
    return externalNode.extNodeID;
}
Coordinate Network::GetStartOrEndNodeGeometry(unsigned int internalNodeID)
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
        LOGGER::LogWarning("Original start/end node ID " +to_string(internalNodeID) +" could not be looked up!");
        throw ex;
    }
    return externalNode.coord;
}
Coordinate Network::GetStartOrEndNodeGeometry(std::string externalNodeID)
{
    AddedPoint externalNode;
    bool nodeFound = false;
    try
    {
        //search in values not in keys
        for (auto& kv : addedStartPoints)
        {
            if (kv.second.extNodeID == externalNodeID)
            {
                externalNode = kv.second;
                nodeFound = true;
                break;
            }
        }
        if (!nodeFound) //search in endPoints
        {
            for (auto& kv : addedEndPoints)
            {
                if (kv.second.extNodeID == externalNodeID)
                {
                    externalNode = kv.second;
                    nodeFound = true;
                    break;
                }
            }
        }
    }
    catch (exception& ex)
    {
        LOGGER::LogError("Original start/end node ID " +externalNodeID +" could not be looked up!");
        throw ex;
    }
    return externalNode.coord;
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

        map<double, pair<InternalArc, NewArc> > distanceTbl;

        /*// search in spatial index for relevant geometry
        // with buffer around point (treshold)*/

        //1. Generate Distance Table
        for ( auto nArc : newArcs )
        {
            InternalArc key = nArc.first;
            NewArc val = nArc.second;
            //auto lPtr = val.arcGeom.get();

            //NewArc geometry as shared_ptr
            //Geometry* g2Ptr = static_cast<Geometry*>( val.arcGeom.get() );
            Geometry* g2Ptr = dynamic_cast<Geometry*>( val.arcGeom.get() );

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
        InternalArc nearestArcKey = elem.second.first;
        NewArc nearestArc = elem.second.second;

        shared_ptr<MultiLineString> mLine = splitLine(coord, static_cast<Geometry&>( *nearestArc.arcGeom ));

        SplittedArc result{ nearestArcKey,
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


SplittedArc Network::GetSplittedClosestOldArcToPoint(Coordinate coord, int treshold,
                                                    const pair<ExternalArc,ArcData>& arcData,
                                                    const Geometry& arc)
{
  try
    {
        ExternalArc nearestArcKey = arcData.first;
        ArcData nearestArc = arcData.second;

        shared_ptr<MultiLineString> mLine = splitLine(coord, arc);

        //Lookup ext. from / toNodes
        unsigned int fromNode = GetInternalNodeID(nearestArcKey.extFromNode);
        unsigned int toNode = GetInternalNodeID(nearestArcKey.extToNode);

        InternalArc ftNode {fromNode, toNode};
        SplittedArc result{ ftNode, nearestArc.cost, nearestArc.capacity, mLine };

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
            //They are generated when the Core Solver is called
            try
            {
                //if (nodeSupply != 0)
                //{
                    NodeSupply sVal {extNodeID, nodeSupply};
                    nodeSupplies.insert( make_pair(internalNodeID, sVal) );
                //}
            }
            catch (exception& ex)
            {
                LOGGER::LogWarning("renameNodes(): Error inserting supply values!");
                LOGGER::LogWarning(ex.what());
            }
        }
    }
}

unsigned int Network::GetMaxNodeCount() {
 return static_cast<unsigned int>(internalDistinctNodeIDs.size());
}

unsigned int Network::GetMaxArcCount() {
 return static_cast<unsigned int>(internalArcData.size());
}

unsigned int Network::GetCurrentNodeCount() {
 return static_cast<unsigned int>(internalDistinctNodeIDs.size());
}

unsigned int Network::GetCurrentArcCount() {
 return static_cast<unsigned int>(internalArcData.size());
}

Arcs& Network::GetInternalArcData()
{
    return internalArcData;
}
NodeSupplies Network::GetNodeSupplies()
{
    return nodeSupplies;
}
Arcs& Network::GetOldArcs()
{
    return oldArcs;
}
NewArcs& Network::GetNewArcs()
{
    return newArcs;
}

/**
* For results of original arcs only
*/
void Network::saveResults(string orig, string dest, double cost, double capacity, double flow,
                                        const string& arcIDs, const string& resultTableName)
{
    bool isResultDBequalNetXpertDB = false;
    if (NETXPERT_CNFG.ResultDBPath == NETXPERT_CNFG.NetXDBPath)
    {
        isResultDBequalNetXpertDB = true;
    }

    switch (NETXPERT_CNFG.ResultDBType)
    {
        case RESULT_DB_TYPE::SpatiaLiteDB:
        {
            //special case: we can write directly into netxpert db without creating a new db
            if (isResultDBequalNetXpertDB)
            {
                //Override result DB Path with original netXpert DB path
                //for using MergeAndSaveResultArcs()
                SpatiaLiteWriter sldb (NETXPERT_CNFG, NETXPERT_CNFG.NetXDBPath);
                sldb.OpenNewTransaction();
                sldb.CreateSolverResultTable(resultTableName, true);
                sldb.MergeAndSaveResultArcs(orig, dest, cost, capacity, flow, NETXPERT_CNFG.ArcsGeomColumnName,
                    NETXPERT_CNFG.ArcIDColumnName, NETXPERT_CNFG.ArcsTableName,
                    arcIDs, resultTableName);
                sldb.CommitCurrentTransaction();
                sldb.CloseConnection();
            }
            else{
                //Load result arc from DB
                SpatiaLiteWriter sldb (NETXPERT_CNFG);
                sldb.OpenNewTransaction();
                sldb.CreateSolverResultTable(resultTableName, true);
                //Prepare query once!
                auto qry = sldb.PrepareSaveResultArc(resultTableName);
                //for arc in arcIDs: load geometry from db and save result to resultDB
                unique_ptr<MultiLineString> arc;
                if (arcIDs.size() > 0)
                {
                    auto tokens = UTILS::Split(arcIDs, ',');
                    for (string& s : tokens)
                    {
                        arc = DBHELPER::GetArcGeometriesFromDB(NETXPERT_CNFG.ArcsTableName, NETXPERT_CNFG.ArcIDColumnName,
                                                     NETXPERT_CNFG.ArcsGeomColumnName, ArcIDColumnDataType::Number,
                                                     s);
                        sldb.SaveResultArc(orig, dest, cost, capacity, flow, *arc, resultTableName, *qry);
                    }
                }
                sldb.CommitCurrentTransaction();
                sldb.CloseConnection();
            }
        }
            break;

        case RESULT_DB_TYPE::ESRI_FileGDB:
        {
            FGDBWriter fgdb (NETXPERT_CNFG);
            fgdb.OpenNewTransaction();
            fgdb.CreateSolverResultTable(resultTableName, true);

            unique_ptr<MultiLineString> arc;
            if (arcIDs.size() > 0)
            {
                auto tokens = UTILS::Split(arcIDs, ',');
                for (string& s : tokens)
                {
                    arc = DBHELPER::GetArcGeometriesFromDB(NETXPERT_CNFG.ArcsTableName,
                                                                NETXPERT_CNFG.ArcIDColumnName,
                                                                NETXPERT_CNFG.ArcsGeomColumnName,
                                                            ArcIDColumnDataType::Number, s);
                    fgdb.SaveResultArc(orig, dest, cost, capacity, flow, *arc, resultTableName);
                }
            }
            fgdb.CommitCurrentTransaction();
            fgdb.CloseConnection();
        }
            break;
    }
}

/**
* For GEOMETRTY_HANDLING::StraightLines and GEOMETRTY_HANDLING::NoGeometry
*/
void Network::saveResults(string orig, string dest, double cost, double capacity, double flow,
                                        const string& resultTableName, DBWriter& writer //,
                                        //SQLite::Statement& qry //can be null for ESRI FileGDB
                                        )
{
    //create empty MultiLineString that is filled or not
    unique_ptr<MultiLineString> mline (DBHELPER::GEO_FACTORY->createMultiLineString());

    switch (NETXPERT_CNFG.GeometryHandling)
    {
        //TODO: Lookup for original geometries does not work with Transportation-Solver, because
        // of the double encoding of the IDs (InputArcs -> ODMatrix-Network -> Transport-Network
        case GEOMETRY_HANDLING::StraightLines:
        {
            //create straight line from orig to dest
            //cout << "looking up "<<orig << endl;
            Coordinate origGeom = GetStartOrEndNodeGeometry(orig);
            //cout << "looking up "<<dest <<endl;
            Coordinate destGeom = GetStartOrEndNodeGeometry(dest);

            auto coordFactory = DBHELPER::GEO_FACTORY->getCoordinateSequenceFactory();
            //cast to size_t with zero coordinates, otherwise there are 0.0 0.0 coords in sequence factory!
            auto coordSeq = coordFactory->create(static_cast<size_t>(0), static_cast<size_t>(2)) ; //0 coordinates, 2 dimensions

            coordSeq->add(origGeom);
            coordSeq->add(destGeom);
            unique_ptr<LineString> line ( DBHELPER::GEO_FACTORY->createLineString(coordSeq));
            Geometry* gLine  = dynamic_cast<Geometry*> (line.get());
            mline = unique_ptr<MultiLineString> ( DBHELPER::GEO_FACTORY->createMultiLineString( vector<Geometry*> {gLine} ));
            //cout << mline->toString() << endl;
        }
        break;

        default: //GEOMETRY_HANDLING::NoGeometry: do nothing regarding the geometry (take the empty geometry)
            break;
    }

    switch (NETXPERT_CNFG.ResultDBType)
    {
        case RESULT_DB_TYPE::SpatiaLiteDB:
        {
            auto& sldb = dynamic_cast<SpatiaLiteWriter&>(writer);

            auto qry = sldb.PrepareSaveResultArc(resultTableName);
            //cout << mline->toString() << endl;
            sldb.SaveResultArc(orig, dest, cost, capacity, flow, *mline, resultTableName, *qry);

        }
        break;

        case RESULT_DB_TYPE::ESRI_FileGDB:
        {

            auto& fgdb = dynamic_cast<FGDBWriter&>(writer);
            fgdb.SaveResultArc(orig, dest, cost, capacity, flow,
                                *mline, resultTableName);
        }
        break;
    }
}

void Network::saveResults(string orig, string dest, double cost, double capacity, double flow,
                                        const string& arcIDs, vector<Geometry*> routeParts,
                                        const string& resultTableName, DBWriter& writer,
                                        SQLite::Statement& qry //only used when saved not to the netxpert db
                                        )
{
    bool isResultDBequalNetXpertDB = false;
    if (NETXPERT_CNFG.ResultDBPath == NETXPERT_CNFG.NetXDBPath)
    {
        isResultDBequalNetXpertDB = true;
    }

    switch (NETXPERT_CNFG.ResultDBType)
    {
        case RESULT_DB_TYPE::SpatiaLiteDB:
        {
            //special case: we can write directly into netxpert db without creating a new db
            if (isResultDBequalNetXpertDB)
            {
                auto& sldb = dynamic_cast<SpatiaLiteWriter&>(writer);
                //put all geometries in routeParts into one (perhaps disconnected) Multilinestring
                //MultilineString could also contain only one Linestring
                unique_ptr<MultiLineString> mLine ( DBHELPER::GEO_FACTORY->createMultiLineString( routeParts ));

                //cout << mLine->toString() << endl;

                sldb.MergeAndSaveResultArcs(orig, dest, cost, capacity, flow, NETXPERT_CNFG.ArcsGeomColumnName,
                    NETXPERT_CNFG.ArcIDColumnName, NETXPERT_CNFG.ArcsTableName,
                    arcIDs, *mLine, resultTableName);
            }
            else
            {
                auto& sldb = dynamic_cast<SpatiaLiteWriter&>(writer);
                //put all geometries in routeParts into one (perhaps disconnected) Multilinestring
                //MultilineString could also contain only one Linestring
                unique_ptr<MultiLineString> mLine ( DBHELPER::GEO_FACTORY->createMultiLineString( routeParts ));

                unique_ptr<MultiLineString> mLineDB;
                //load geometry from db
                if (arcIDs.size() > 0)
                    mLineDB = DBHELPER::GetArcGeometriesFromDB(NETXPERT_CNFG.ArcsTableName,
                                                                                NETXPERT_CNFG.ArcIDColumnName,
                                                                                NETXPERT_CNFG.ArcsGeomColumnName,
                                                                                ArcIDColumnDataType::Number, arcIDs);
                //merge routeParts with original arcs
                LineMerger lm;
                lm.add(mLine.get());

                if (mLineDB)
                    lm.add(mLineDB.get());

                vector<LineString *> *mls = lm.getMergedLineStrings();
                auto& mlsRef = *mls;
                vector<Geometry*> mls2;
                for (auto l : mlsRef)
                    mls2.push_back(dynamic_cast<Geometry*>(l));

                unique_ptr<MultiLineString> route (DBHELPER::GEO_FACTORY->createMultiLineString( mls2 ) );

                //save merged route geometry to db
                sldb.SaveResultArc(orig, dest, cost, capacity, flow, *route, resultTableName, qry);
            }
        }
            break;

        case RESULT_DB_TYPE::ESRI_FileGDB:
        {
            /*
                Get all geometries from Spatialite DB (per arcIDs) of one route and merge them with
                all the segments of routeParts;
                Save the geometry with SaveSolveQueryToDB()
            */
            unique_ptr<MultiLineString> mLine ( DBHELPER::GEO_FACTORY->createMultiLineString( routeParts ));

            unique_ptr<MultiLineString> mLineDB;
            if (arcIDs.size() > 0)
                mLineDB = DBHELPER::GetArcGeometriesFromDB(NETXPERT_CNFG.ArcsTableName,
                                                                            NETXPERT_CNFG.ArcIDColumnName,
                                                                            NETXPERT_CNFG.ArcsGeomColumnName,
                                                                            ArcIDColumnDataType::Number, arcIDs);
            //merge
            LineMerger lm;
            lm.add(mLine.get());

            if (mLineDB)
                lm.add(mLineDB.get());

            vector<LineString *> *mls = lm.getMergedLineStrings();
            auto& mlsRef = *mls;
            vector<Geometry*> mls2;
            for (auto l : mlsRef)
                mls2.push_back(dynamic_cast<Geometry*>(l));

            unique_ptr<MultiLineString> route (DBHELPER::GEO_FACTORY->createMultiLineString( mls2 ) );

            cout << route->toString()<< endl;

            auto& fgdb = dynamic_cast<FGDBWriter&>(writer);
            fgdb.SaveResultArc(orig, dest, cost, capacity, flow,
                                *route, resultTableName);
        }
            break;
    }
}


/**
* For GEOMETRTY_HANDLING::StraightLines and GEOMETRTY_HANDLING::NoGeometry and once prepared statement
*/
void Network::saveResults(string orig, string dest, double cost, double capacity, double flow,
                                        const string& resultTableName, DBWriter& writer, SQLite::Statement& qry)
{
    //create empty MultiLineString that is filled or not
    unique_ptr<MultiLineString> mline (DBHELPER::GEO_FACTORY->createMultiLineString());

    switch (NETXPERT_CNFG.GeometryHandling)
    {
        //TODO: Lookup for original geometries does not work with Transportation-Solver, because
        // of the double encoding of the IDs (InputArcs -> ODMatrix-Network -> Transport-Network
        case GEOMETRY_HANDLING::StraightLines:
        {
            //create straight line from orig to dest
            //cout << "looking up "<<orig << endl;
            Coordinate origGeom = GetStartOrEndNodeGeometry(orig);
            //cout << "looking up "<<dest <<endl;
            Coordinate destGeom = GetStartOrEndNodeGeometry(dest);

            auto coordFactory = DBHELPER::GEO_FACTORY->getCoordinateSequenceFactory();
            //cast to size_t with zero coordinates, otherwise there are 0.0 0.0 coords in sequence factory!
            auto coordSeq = coordFactory->create(static_cast<size_t>(0), static_cast<size_t>(2)) ; //0 coordinates, 2 dimensions

            coordSeq->add(origGeom);
            coordSeq->add(destGeom);
            unique_ptr<LineString> line ( DBHELPER::GEO_FACTORY->createLineString(coordSeq));
            Geometry* gLine  = dynamic_cast<Geometry*> (line.get());
            mline = unique_ptr<MultiLineString> ( DBHELPER::GEO_FACTORY->createMultiLineString( vector<Geometry*> {gLine} ));
            //cout << mline->toString() << endl;
        }
        break;

        default: //GEOMETRY_HANDLING::NoGeometry: do nothing regarding the geometry (take the empty geometry)
            break;
    }

    switch (NETXPERT_CNFG.ResultDBType)
    {
        case RESULT_DB_TYPE::SpatiaLiteDB:
        {
            auto& sldb = dynamic_cast<SpatiaLiteWriter&>(writer);
            //auto qry = sldb.PrepareSaveResultArc(resultTableName);
            //cout << mline->toString() << endl;
            sldb.SaveResultArc(orig, dest, cost, capacity, flow, *mline, resultTableName, qry);

        }
        break;

        case RESULT_DB_TYPE::ESRI_FileGDB:
        {
            auto& fgdb = dynamic_cast<FGDBWriter&>(writer);
            fgdb.SaveResultArc(orig, dest, cost, capacity, flow,
                                *mline, resultTableName);
        }
        break;
    }
}



void Network::readNetworkFromTable(bool autoClean, bool oneWay)
{
    unordered_map<InternalArc,DuplicateArcData> duplicates;

    for (InputArcs::iterator it = arcsTbl.begin(); it != arcsTbl.end(); ++it)
    {
        InputArc arc = *it;
        // String -> so content of nodeID fields could be string, int or double
        string externalArcID = arc.extArcID;
        string externalStartNode = arc.extFromNode;
        string externalEndNode = arc.extToNode;
        unsigned int internalStartNode;
        unsigned int internalEndNode;
        try
        {
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

                ExternalArc ftNode {externalStartNode, externalEndNode};
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

    DBHELPER::EliminatedArcs = this->eliminatedArcs;
    /*for (auto& s : DBHELPER::EliminatedArcs)
        cout << s << endl;*/
}
//TODO: isDirected evtl. von Klasse nehmen, nicht vom Config
void Network::processArc(InputArc arc, unsigned int internalStartNode,
                        unsigned int internalEndNode)
{
    bool isDirected = NETXPERT_CNFG.IsDirected;
    string externalArcID = arc.extArcID;

    InternalArc inFromToPair {internalStartNode, internalEndNode};

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
        InternalArc inToFromPair {inFromToPair.toNode, inFromToPair.fromNode};

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
void Network::processBarriers()
{
    if (!this->NETXPERT_CNFG.BarrierPolyTableName.empty())
    {
        LOGGER::LogInfo("Processing barrier polygons from "+NETXPERT_CNFG.BarrierPolyTableName + "..");

        std::unordered_set<string> arcIDs = DBHELPER::GetIntersectingArcs(NETXPERT_CNFG.BarrierPolyTableName,
                                      NETXPERT_CNFG.BarrierPolyGeomColumnName,
                                      NETXPERT_CNFG.ArcsTableName,
                                      NETXPERT_CNFG.ArcIDColumnName,
                                      NETXPERT_CNFG.ArcsGeomColumnName);
        for (auto& id : arcIDs)
        {
            cout << "Elim arc " << id << endl;
            this->eliminatedArcs.insert(id);
        }
        LOGGER::LogDebug("Got "+ to_string(arcIDs.size()) +" intersecting barrier polygons from " + NETXPERT_CNFG.BarrierPolyTableName +".");
    }
    if (!this->NETXPERT_CNFG.BarrierLineTableName.empty())
    {
        LOGGER::LogInfo("Processing barrier lines from "+NETXPERT_CNFG.BarrierLineTableName + "..");

        std::unordered_set<string> arcIDs = DBHELPER::GetIntersectingArcs(NETXPERT_CNFG.BarrierLineTableName,
                                      NETXPERT_CNFG.BarrierLineGeomColumnName,
                                      NETXPERT_CNFG.ArcsTableName,
                                      NETXPERT_CNFG.ArcIDColumnName,
                                      NETXPERT_CNFG.ArcsGeomColumnName);
        for (auto& id : arcIDs)
        {
            this->eliminatedArcs.insert(id);
        }
        LOGGER::LogDebug("Got "+ to_string(arcIDs.size()) +" intersecting barrier lines from " + NETXPERT_CNFG.BarrierLineTableName +".");
    }
    if (!this->NETXPERT_CNFG.BarrierPointTableName.empty())
    {
        LOGGER::LogInfo("Processing barrier points from "+NETXPERT_CNFG.BarrierPointTableName + "..");

        std::unordered_set<string> arcIDs = DBHELPER::GetIntersectingArcs(NETXPERT_CNFG.BarrierPointTableName,
                                      NETXPERT_CNFG.BarrierPointGeomColumnName,
                                      NETXPERT_CNFG.ArcsTableName,
                                      NETXPERT_CNFG.ArcIDColumnName,
                                      NETXPERT_CNFG.ArcsGeomColumnName);
        for (auto& id : arcIDs)
        {
            this->eliminatedArcs.insert(id);
        }
        LOGGER::LogDebug("Got "+ to_string(arcIDs.size()) +" intersecting barrier points from " + NETXPERT_CNFG.BarrierPointTableName +".");
    }
}

void Network::Reset()
{
    //1. Remove all new Segments
    //2. Restore all oldEdges
    const int totalAddedPoints = addedEndPoints.size() + addedStartPoints.size();
    //Reset network only if there were changes
    if (totalAddedPoints > 0)
    {
        //Remove all new Segments
        for (auto& newArc : newArcs)
        {
            //.. in internal mapping Dict
            internalArcData.erase(newArc.first);
        }
        //Add oldEdges to internal mapping dict
        for (auto& oldArc : oldArcs)
        {
            //.. in internal mapping Dict
            internalArcData.insert( make_pair( oldArc.first, oldArc.second) );

            for (auto& point : addedStartPoints)
            {
                try
                {
                    //TODO: CHeck removal of node IDs

                    internalDistinctNodeIDs.erase(
                        oldArc.second.extArcID + "_" + to_string(point.first));
                    swappedInternalDistinctNodeIDs.erase(point.first);
                }
                catch (exception& ex){ }
            }
            for (auto& point : addedEndPoints)
            {
                try
                {
                    //TODO: CHeck removal of node IDs
                    internalDistinctNodeIDs.erase(
                        oldArc.second.extArcID + "_" + to_string(point.first));
                    swappedInternalDistinctNodeIDs.erase(point.first);
                }
                catch (exception& ex){ }
            }
        }
        //Remove all added start and end nodes from nodeSupply
        for (auto point : addedStartPoints)
            nodeSupplies.erase(point.first);

        for (auto point : addedEndPoints)
            nodeSupplies.erase(point.first);

        addedStartPoints.clear();
        addedEndPoints.clear();
        oldArcs.clear();
        swappedOldArcs.clear();
        newArcs.clear();
        eliminatedArcs.clear();
        arcLoops.clear();
    }
}

// MCF
double Network::calcTotalSupply ()
{
    double supplyValue = 0;
    for (auto& supply : nodeSupplies)
    {
        if (supply.second.supply > 0)
            supplyValue += supply.second.supply;
    }
    //cout << supplyValue << endl;
    return supplyValue;
}
double Network::calcTotalDemand ()
{
    double demandValue = 0;
    for (auto& demand : nodeSupplies)
    {
        if (demand.second.supply < 0)
            demandValue += demand.second.supply;
    }
    //cout << demandValue << endl;
    return abs(demandValue);
}
MinCostFlowInstanceType Network::GetMinCostFlowInstanceType()
{
    double totalSupply = calcTotalSupply();
    double totalDemand = calcTotalDemand();

    LOGGER::LogDebug("Supply - Demand: " + to_string( totalSupply ) + " - " + to_string(totalDemand));

    if (totalSupply > totalDemand)
        return MinCostFlowInstanceType::MCFExtrasupply;
    if (totalSupply < totalDemand)
        return MinCostFlowInstanceType::MCFExtrademand;
    else
        return MinCostFlowInstanceType::MCFBalanced;
}

// Arbeiten mit Dummy Nodes; Ziel ist die Ausgewogene Verteilung von Angebot und Nachfrage
void Network::TransformUnbalancedMCF(MinCostFlowInstanceType mcfInstanceType)
{
    switch (mcfInstanceType)
    {
        case MinCostFlowInstanceType::MCFExtrasupply:
            transformExtraSupply();
            break;
        case MinCostFlowInstanceType::MCFExtrademand:
            transformExtraDemand();
            break;
        case MinCostFlowInstanceType::MCFBalanced: //Nothing to do
            break;
        default: //MCFUndefined
            break;
    }
}
void Network::transformExtraDemand()
{
    // Dummy Angebotsknoten mit überschüssiger Nachfrage hinzufügen
    // Dummy-Kosten (0 km) in Netzwerk hinzufügen
    // Reicht es, wenn der neue Dummy-Knoten von allen Nicht-Transshipment-Knoten (=! 0) erreichbar ist?
    // --> In Networkx schon

    //OK - Funktion behandelt Angebotsüberschuss oder Nachfrageüberschuss
    // siehe getSupplyDemandDifference()
    processSupplyOrDemand();
}
void Network::transformExtraSupply()
{
    // Dummy Nachfrageknoten mit überschüssigem Angebot hinzufügen
    // Dummy-Kosten (0 km) in Netzwerk hinzufügen
    // Reicht es, wenn der neue Dummy-Knoten von allen Nicht-Transshipment-Knoten (=! 0) erreichbar ist?

    //OK - Funktion behandelt Angebotsüberschuss oder Nachfrageüberschuss
    // siehe getSupplyDemandDifference()
    processSupplyOrDemand();
}
void Network::processSupplyOrDemand()
{
    // Dummy Knoten
    unsigned int newNodeID = GetMaxNodeCount() + 1;
    internalDistinctNodeIDs.insert(make_pair("dummy", newNodeID));
    swappedInternalDistinctNodeIDs.insert( make_pair( newNodeID, "dummy" ));
    //cout << "Ausgleich: " << getSupplyDemandDifference() << endl;
    auto diff = getSupplyDemandDifference();
    NodeSupply sup {"dummy", diff }; //Differenz ist positiv oder negativ, je nach Überschuss
    nodeSupplies.insert( make_pair( newNodeID, sup));

    // Dummy-Kosten (0 km) in Netzwerk hinzufügen
    const double cost = 0;
    const double capacity = netxpert::DOUBLE_INFINITY;
    for (auto node : nodeSupplies) //Filter dummy und transshipment nodes (=0) raus
    {
        if (node.first == newNodeID) //Filter dummy
            continue;
        if (node.second.supply == 0) //Filter transshipment nodes (=0)
            continue;

        InternalArc key;
        if (diff > 0)
        {
            key.fromNode = newNodeID;
            key.toNode = node.first;
        }
        else
        {
            key.fromNode = node.first;
            key.toNode = newNodeID;
        }

        ArcData value  {"dummy", cost, capacity };
        internalArcData.insert( make_pair(key, value));
    }
}
double Network::getSupplyDemandDifference()
{
    //can be negative or positive
    return (calcTotalDemand() - calcTotalSupply());
}
