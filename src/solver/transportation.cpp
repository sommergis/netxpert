#include "transportation.h"

using namespace netxpert;
using namespace std;

Transportation::Transportation(Config& cnfg) : MinCostFlow(cnfg)
{
    //ctor
    LOGGER::LogInfo("Transportation Solver instantiated");
    NETXPERT_CNFG = cnfg;
    algorithm = cnfg.McfAlgorithm;
    solverStatus = MCFSolverStatus::MCFUnSolved;
    IsDirected = true; //TODO CHECK always true

}

Transportation::~Transportation()
{
    //dtor
}

vector<unsigned int> Transportation::GetOrigins() const
{
    return this->originNodes;
}
void Transportation::SetOrigins(vector<unsigned int> origs)
{
    this->originNodes = origs;
}

vector<unsigned int> Transportation::GetDestinations() const
{
    return this->destinationNodes;
}
void Transportation::SetDestinations(vector<unsigned int> dests)
{
    this->destinationNodes = dests;
}
void Transportation::SetExtODMatrix(vector<ExtODMatrixArc> _extODMatrix)
{
    extODMatrix = _extODMatrix;
}

std::unordered_map<ODPair, DistributionArc> Transportation::GetDistribution() const
{
    return distribution;
}
std::vector<ExtDistributionArc> Transportation::GetExtDistribution() const
{
    vector<ExtDistributionArc> distArcs;
    unsigned int counter = 0;
    for (auto& dist : distribution)
    {
        counter += 1;
        if (counter % 2500 == 0)
            LOGGER::LogInfo("Processed #" + to_string(counter) + " rows.");

        ODPair key = dist.first;
        DistributionArc val = dist.second;
        CompressedPath path = val.path;
        vector<unsigned int> ends = val.path.first;

        // only one arc
        vector<string> arcIDs = network->GetOriginalArcIDs(vector<InternalArc>
                                                                    { InternalArc  {key.origin, key.dest} }, IsDirected);
        ExtArcID arcID = arcIDs.at(0);

        double costPerPath = path.second;
        double flow = val.flow;
        //TODO: get capacity per arc
        double cap = -1;

        string orig;
        string dest;
        try{
            orig = network->GetOriginalNodeID(key.origin);
        }
        catch (exception& ex) {
            LOGGER::LogError(ex.what());
        }
        try{
            dest = network->GetOriginalNodeID(key.dest);
        }
        catch (exception& ex) {
            LOGGER::LogError(ex.what());
        }
        //JSON output!
        distArcs.push_back( ExtDistributionArc {arcID, ExternalArc {orig, dest}, costPerPath, flow });
    }
    return distArcs;
}

std::string Transportation::GetJSONExtDistribution() const
{
    return UTILS::SerializeObjectToJSON<vector<ExtDistributionArc>>(GetExtDistribution(),"distribution")+ "\n }";
}

std::string Transportation::GetSolverJSONResult() const
{
    TransportationResult transpRes {this->optimum, GetExtDistribution() };
    return UTILS::SerializeObjectToJSON<TransportationResult>(transpRes, "result") + "\n }";
}

void Transportation::SetExtNodeSupply(vector<ExtNodeSupply> _nodeSupply)
{
    extNodeSupply = _nodeSupply;
}

std::pair<double,double> Transportation::getFlowCostData(const vector<FlowCost>& fc, const ODPair& key) const
{
    pair<double,double> result;
    for (const auto& f : fc)
    {
        const ODPair vKey {f.intArc.fromNode, f.intArc.toNode};
        if ( vKey == key )
        {
            result = make_pair(f.flow, f.cost);
            break;
        }
    }
    return result;
}

void Transportation::Solve()
{
    if (this->extODMatrix.size() == 0 || this->extNodeSupply.size() == 0)
        throw std::runtime_error("OD-Matrix and node supply must be filled in Transportation Solver!");

    //arcData from ODMatrix
    InputArcs arcs;
    for (ExtODMatrixArc& v : extODMatrix)
    {
        ExtArcID key = v.extArcID;
        ExternalArc arc = v.extArc;
        double cost = v.cost;
        double cap = DOUBLE_INFINITY; // TRANSPORTATION Problem, no caps
        string fromNode = arc.extFromNode;
        string toNode = arc.extToNode;
        string oneway = "";
        arcs.push_back( InputArc {key, fromNode, toNode, cost, cap, oneway } );
    }
    Config cnfg = NETXPERT_CNFG;
    ColumnMap cmap { cnfg.ArcIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
                        cnfg.CostColumnName, cnfg.CapColumnName, cnfg.OnewayColumnName,
                        cnfg.NodeIDColumnName, cnfg.NodeSupplyColumnName };

    //construct nodeSupply from external nodeSupply
    InputNodes nodes;
    for (ExtNodeSupply& v : extNodeSupply)
    {
        ExtNodeID key = v.extNodeID;
        double supply = v.supply;
        nodes.push_back( InputNode {key, supply });
    }

    bool autoCleanNetwork = cnfg.CleanNetwork;

    //construct network from external ODMatrix
    //Network net(arcs, nodes, cmap, cnfg);
    //net.ConvertInputNetwork(autoCleanNetwork);

    this->network = std::unique_ptr<Network>(new Network(arcs, nodes, cmap, cnfg));
    this->network->ConvertInputNetwork(autoCleanNetwork);

    Network net = *this->network;

    //Check Balancing of Transportation Problem Instance and
    //transform the instance if needed is already done in
    //parent class method MinCostFlow::Solve()

    //Call MCF Solver of parent class
    MinCostFlow::Solve(net);

    //Go back from Origin->Dest: flow,cost to real path with a list of start and end nodes of the ODMatrix
    //1. Translate back to original node ids
    unordered_map<IntNodeID, ExtNodeID> startNodeMap;
    unordered_map<IntNodeID, ExtNodeID> endNodeMap;
    for (auto& n : net.GetNodeSupplies())
    {
        IntNodeID key = n.first;
        NodeSupply ns = n.second;
        if (ns.supply > 0 || ns.supply == 0) //start node
        {
            //cout<< "sm: inserting " << key << ": "<< ns.supply << endl;
            startNodeMap.insert(make_pair(key, net.GetOriginalNodeID(key)));
        }
        if (ns.supply < 0 || ns.supply == 0) //end node
        {
            //cout<< "em: inserting " << key << ": "<< ns.supply << endl;
            endNodeMap.insert(make_pair(key, net.GetOriginalNodeID(key)));
        }
    }
    /*cout << "start node map" << endl;
    for (auto& s : startNodeMap)
        cout << s.first << " : " << s.second << endl;
    cout << "end node map" << endl;
    for (auto& e : endNodeMap)
        cout << e.first << " : " << e.second << endl;*/
    //result
    unordered_map<ODPair, DistributionArc> result;

    //2. Search for the ODpairs in ODMatrix Solver result with the original IDs of the
    // Min Cost Flow Solver result
    vector<FlowCost> flowCost = MinCostFlow::GetMinCostFlow();
    for (auto& fc : flowCost)
    {
        // Here the original node id can be an arbitrary string, because it was set externally
        // so we cannot translate back to unsigned ints as in Solve(net)
        string oldStartNodeStr;
        string oldEndNodeStr;
        try
        {
            //TODO: dummys
            // At the moment they will not be given back, because their arc cannot be resolved
            oldStartNodeStr = startNodeMap.at(fc.intArc.fromNode);
            oldEndNodeStr = endNodeMap.at(fc.intArc.toNode);

            if (oldStartNodeStr != "dummy" && oldEndNodeStr != "dummy")
            {
                //build key for result map
                ODPair resultKey {fc.intArc.fromNode, fc.intArc.toNode};
                // flow and cost
                // search for a match of startNode -> endNode in FlowCost,
                // take the first occurence
                // as flow and cost
                auto data = getFlowCostData(flowCost, resultKey);

                // there is no path, because we do not have the values from the OD solver
                DistributionArc resultVal { CompressedPath { make_pair(vector<unsigned int> {}, data.second) }, data.first };

                this->distribution.insert(make_pair(resultKey, resultVal));
            }
            /*else
            {
                LOGGER::LogInfo("Dummys!");
            }*/
        }
        catch (exception& ex)
        {
            LOGGER::LogError("Something strange happened - maybe a key error. ");
            LOGGER::LogError(ex.what());
        }
    }
}

void Transportation::Solve(Network& net)
{
    if (this->originNodes.size() == 0 || this->destinationNodes.size() == 0)
        throw std::runtime_error("Origin nodes and destination nodes must be filled in Transportation Solver!");

    //Call the ODMatrixSolver
    OriginDestinationMatrix ODsolver(this->NETXPERT_CNFG);
    //Precondition: Starts and Ends has been filled and thus
    // they were already converted to ascending ints
    ODsolver.SetOrigins(this->originNodes);
    ODsolver.SetDestinations(this->destinationNodes);
    ODsolver.Solve(net);
    this->odMatrix = ODsolver.GetODMatrix();

    std::unordered_set<IntNodeID> odKeys;
    //arcData from ODMatrix
    InputArcs arcs;
    int counter = 0;
    for (auto& kv : this->odMatrix)
    {
        counter += 1;
        ODPair key = kv.first;
        double cost = kv.second;
        double cap = DOUBLE_INFINITY; // TRANSPORTATION Problem, no caps
        string fromNode = to_string(key.origin);
        string toNode = to_string(key.dest);
        string oneway = "";
        // We do not have a original ArcID - so we set counter as ArcID
        arcs.push_back( InputArc {to_string(counter), fromNode, toNode,
                                    cost, cap, oneway } );
        // Populate a unique key set of OD-Nodes
        if(odKeys.count(key.origin) == 0)
            odKeys.insert(key.origin);
        if(odKeys.count(key.dest) == 0)
            odKeys.insert(key.dest);
    }
    Config cnfg = this->NETXPERT_CNFG;
    ColumnMap cmap { cnfg.ArcIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
                        cnfg.CostColumnName, cnfg.CapColumnName, cnfg.OnewayColumnName,
                        cnfg.NodeIDColumnName, cnfg.NodeSupplyColumnName };

    // Node supply has been set already in the given Network (parameter for Solve())

    // Add nodes data only, if reached through ODMatrix Solver
    InputNodes nodes;
    NodeSupplies supplies = net.GetNodeSupplies();
    for (auto& v : supplies )
    {
        IntNodeID key = v.first;
        double supply = v.second.supply;
        // condition: only reached nodes from ODMatrix solver shall be
        // processed
        if (odKeys.count(key) > 0)
            nodes.push_back( InputNode {to_string(key), supply });
    }

    bool autoCleanNetwork = cnfg.CleanNetwork;

    //construct network from ODMatrix
    Network newNet(arcs, nodes, cmap, cnfg);
    newNet.ConvertInputNetwork(autoCleanNetwork);

    //Check Balancing of Transportation Problem Instance and
    //transform the instance if needed is already done in
    //parent class method MinCostFlow::Solve()

    //Call MCF Solver of parent class
    MinCostFlow::Solve(newNet);

    // BEWARE of KEY Errors in Dictionarys!

    //Go back from Origin->Dest: flow,cost to real path with a list of start and end nodes of the ODMatrix
    //1. Translate back to original node ids
    unordered_map<IntNodeID, ExtNodeID> startNodeMap;
    unordered_map<IntNodeID, ExtNodeID> endNodeMap;
    NodeSupplies newSupplies = newNet.GetNodeSupplies();
    for (auto& n : newSupplies)
    {
        IntNodeID key = n.first;
        NodeSupply ns = n.second;
        if (ns.supply > 0 || ns.supply == 0) //start node
        {
            //cout<< "sm: inserting " << key << ": "<< ns.supply << endl;
            // We know that the original node id is actually an uint because it was transformed in the ODMatrix solver		            }
            // but dummys are possible because of balancing of the TP
            if (ns.extNodeID != "dummy")
            {
                IntNodeID oridNodeID = static_cast<IntNodeID>(std::stoul(ns.extNodeID, nullptr, 0));
                startNodeMap.insert(make_pair(oridNodeID, ns.extNodeID));
            }
        }
        if (ns.supply < 0 || ns.supply == 0) //end node
        {
            //cout<< "em: inserting " << key << ": "<< ns.supply << endl;
            // We know that the original node id is actually an uint because it was transformed in the ODMatrix solver
            // but dummys are possible because of balancing of the TP
            if (ns.extNodeID != "dummy")
            {
                IntNodeID oridNodeID = static_cast<IntNodeID>(std::stoul(ns.extNodeID, nullptr, 0));
                endNodeMap.insert(make_pair(oridNodeID, ns.extNodeID));
            }
        }
    }

    //result
    unordered_map<ODPair, DistributionArc> result;

    //2. Search for the ODpairs in ODMatrix Solver result with the original IDs of the
    // Min Cost Flow Solver result
    vector<FlowCost> flowCost = MinCostFlow::GetMinCostFlow();
    unordered_map<ODPair, CompressedPath> shortestPaths = ODsolver.GetShortestPaths();
    for (auto& fc : flowCost)
    {
        // We know that the original node id is actually an IntNodeID because it was transformed
        // in the ODMatrix solver
        string oldStartNodeStr;
        string oldEndNodeStr;
        IntNodeID oldStartNode = 0;
        IntNodeID oldEndNode = 0;
        //cout << fc.intArc.fromNode << "->" <<fc.intArc.toNode << " f: "<< fc.flow << " c: "<<fc.cost <<endl;
        try
        {
            //If there are no values for the keys: dummys!
            if (startNodeMap.count(fc.intArc.fromNode) > 0 &&
                 endNodeMap.count(fc.intArc.toNode) > 0)
            {
                oldStartNodeStr = startNodeMap.at(fc.intArc.fromNode);
                oldEndNodeStr = endNodeMap.at(fc.intArc.toNode);
                //cout << oldEndNodeStr << endl;
                if (oldStartNodeStr.size()>0)
                    oldStartNode = static_cast<IntNodeID>(std::stoul(oldStartNodeStr, nullptr, 0));
                if (oldEndNodeStr.size()>0)
                    oldEndNode = static_cast<IntNodeID>(std::stoul(oldEndNodeStr, nullptr, 0));

                if (oldStartNode > 0 && oldEndNode > 0)
                {
                    //build key for result map
                    ODPair resultKey {oldStartNode, oldEndNode};
                    // flow and cost
                    // search for a match of startNode -> endNode in FlowCost,
                    // take the first occurence
                    // as flow and cost
                    auto data = getFlowCostData(flowCost, resultKey);
                    auto path = shortestPaths.at( resultKey );

                    DistributionArc resultVal { path, data.first };

                    this->distribution.insert(make_pair(resultKey, resultVal));
                }
                else
                {
                    LOGGER::LogWarning("Should never reach here! FromTo "+
                                        oldStartNodeStr + " - "+ oldEndNodeStr+
                                        " could not be looked up!");
                }
            }// else: dummys
        }
        catch (exception& ex)
        {
            LOGGER::LogError("Something strange happened - maybe a key error. ");
            LOGGER::LogError(ex.what());
        }
    }
}

vector<InternalArc> Transportation::UncompressRoute(unsigned int orig, vector<unsigned int>& ends) const
{
    vector<InternalArc> startsNends;
    for (int i = 0; i < ends.size(); i++)
    {
        if (i != ends.size() - 1)
            startsNends.push_back( InternalArc { ends[i + 1], ends[i] } );
        else
            startsNends.push_back( InternalArc {  orig, ends[i] } );
    }
    return startsNends;
}
