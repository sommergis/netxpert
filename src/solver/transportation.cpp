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
void Transportation::SetDestinations(vector<unsigned int>& dests)
{
    this->destinationNodes = dests;
}
void Transportation::SetExtODMatrix(unordered_map<string, ExtODMatrixArc> _extODMatrix)
{
    extODMatrix = _extODMatrix;
}
unordered_map<string, ExtODMatrixArc> Transportation::GetExtODMatrix() const
{
    return extODMatrix;
}

unordered_map<ODPair, DistributionArc> Transportation::GetDistribution() const
{
    return distribution;
}
unordered_map<ExtNodeID, double> Transportation::GetNodeSupply() const
{
    return nodeSupply;
}

void Transportation::SetNodeSupply(unordered_map<ExtNodeID, double> _nodeSupply)
{
    nodeSupply = _nodeSupply;
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
    if (extODMatrix.size() == 0 || nodeSupply.size() == 0)
        throw std::runtime_error("OD-Matrix and node supply must be filled in Transportation Solver!");

    //arcData from ODMatrix
    InputArcs arcs;
    for (auto& kv : extODMatrix)
    {
        ExtArcID key = kv.first;
        ExtODMatrixArc val = kv.second;
        double cost = val.cost;
        double cap = DOUBLE_INFINITY; // TRANSPORTATION Problem, no caps
        string fromNode = val.extArc.extFromNode;
        string toNode = val.extArc.extToNode;
        string oneway = "";
        arcs.push_back( InputArc {key, fromNode, toNode, cost, cap, oneway } );
    }
    Config cnfg = NETXPERT_CNFG;
    ColumnMap cmap { cnfg.ArcIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
                        cnfg.CostColumnName, cnfg.CapColumnName, cnfg.OnewayColumnName,
                        cnfg.NodeIDColumnName, cnfg.NodeSupplyColumnName };

    //construct nodeSupply from external nodeSupply
    InputNodes nodes;
    for (auto& kv : nodeSupply)
    {
        ExtNodeID key = kv.first;
        double supply = kv.second;
        nodes.push_back( InputNode {key, supply });
    }

    bool autoCleanNetwork = cnfg.CleanNetwork;

    // construct network from external ODMatrix
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
        // We know that the original node id is actually an IntNodeID because it was transformed
        // in the ODMatrix solver
        string oldStartNodeStr;
        string oldEndNodeStr;
        IntNodeID oldStartNode = 0;
        IntNodeID oldEndNode = 0;
        //cout << fc.intArc.fromNode << "->" <<fc.intArc.toNode << " f: "<< fc.flow << " c: "<<fc.cost <<endl;
        try
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

                // there is no path, because we do not have the values from the OD solver
                DistributionArc resultVal { CompressedPath { make_pair(vector<unsigned int> {}, data.second) }, data.first };

                this->distribution.insert(make_pair(resultKey, resultVal));
            }
            else
            {
                LOGGER::LogWarning("Should never reach here! FromTo "+
                                    oldStartNodeStr + " - "+ oldEndNodeStr+
                                    " could not be looked up!");
            }
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