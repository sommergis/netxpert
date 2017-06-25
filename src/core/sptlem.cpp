#include "sptlem.h"

using namespace std;
using namespace lemon;
using namespace netxpert::core;

SPT_LEM::SPT_LEM(bool Drctd)
{
	//nmax = 0;
	//mmax = 0;
	isDrctd = Drctd;
	allDests = true; //unless set with SetDest()
}

/*void
 SPT_LEM::SolveSPT(netxpert::data::cost_t treshold){

    using namespace netxpert::data;

    //this->dijk = new Dijkstra<graph_t, graph_t::ArcMap<cost_t>>(this->g, *this->costMap);
    //  Dijkstra<filtered_graph_t, graph_t::ArcMap<cost_t>>::SetPath<path_t, NodeMap>::Create
    this->dijk = new dijkstra_t (*this->g, *this->costMap);
    //this->predMap = new lemon::concepts::ReadWriteMap<node_t, arc_t>();
    //this->distMap = new netxpert::data::graph_t::NodeMap<cost_t>(*this->g, t);
    //this->dijk->distMap(*this->distMap);
    //this->dijk->predMap(*this->predMap);

    this->dijk->init();
    this->dijk->addSource(this->orig);

    auto currentNode = this->dijk->nextNode();

    if (treshold > 0) {
        while (currentNode != INVALID &&
                !this->dijk->emptyQueue() &&
                this->dijk->dist(currentNode) <= treshold ) {
            this->dijk->processNextNode();
            currentNode = this->dijk->nextNode();
        }
    }
    else {
        while (currentNode != INVALID &&
                !this->dijk->emptyQueue() ) {
            this->dijk->processNextNode();
            currentNode = this->dijk->nextNode();
        }
    }
}*/

void
 SPT_LEM::SolveSPT(netxpert::data::cost_t treshold, bool bidirectional){

    this->bidirectional = bidirectional;

    using namespace netxpert::data;

    //this->dijk = new Dijkstra<graph_t, graph_t::ArcMap<cost_t>>(this->g, *this->costMap);
    //this->predMap = new lemon::concepts::ReadWriteMap<node_t, arc_t>();
    //this->distMap = new netxpert::data::graph_t::NodeMap<cost_t>(*this->g, t);
    //this->dijk->distMap(*this->distMap);
    //this->dijk->predMap(*this->predMap);

    if (this->bidirectional) {
        //std::cout << "Bidirectional Dijkstra" << std::endl;
        //this->bijk = new bijkstra_t (*this->g, *this->costMap);
        this->bijk = std::unique_ptr<bijkstra_t> (new bijkstra_t (*this->g, *this->costMap));

        this->bijk->init();
        this->bijk->addSource(this->orig);

        /*auto currentNode = this->bijk->nextNode();

        if (treshold > 0) {
            while (currentNode != INVALID &&
                    !this->bijk->emptyQueue() &&
                    this->bijk->dist(currentNode) <= treshold
                    && !this->bijk->processed(this->dest)
                    ) {
                this->bijk->processNextNode();
                currentNode = this->bijk->nextNode();
            }
        }
        else {
            while (currentNode != INVALID &&
                    !this->bijk->emptyQueue()
                     && !this->bijk->processed(this->dest)
                    ) {
                this->bijk->processNextNode();
                currentNode = this->bijk->nextNode();
            }
        }*/

        this->bijk->run(this->orig, this->dest);
    }
    else {
        //std::cout << "Regular Dijkstra" << std::endl;
        //this->dijk = new dijkstra_t (*this->g, *this->costMap);
        this->dijk = std::unique_ptr<dijkstra_t> (new dijkstra_t (*this->g, *this->costMap));

        this->dijk->init();
        this->dijk->addSource(this->orig);

        auto currentNode = this->dijk->nextNode();

        //only if search distance is defined
        if (treshold > 0) {
            //s-t search
            if (!this->allDests) {
                while (currentNode != INVALID &&
                        !this->dijk->emptyQueue() &&
                        this->dijk->dist(currentNode) <= treshold
                        && !this->dijk->processed(dest) //search only until dest is reached; -30 secs on 2 Mio arcs
                         ) {
                    this->dijk->processNextNode();
                    currentNode = this->dijk->nextNode();
                }
            }
            //shortest path tree
            else {
                while (currentNode != INVALID &&
                        !this->dijk->emptyQueue() &&
                        this->dijk->dist(currentNode) <= treshold
                         ) {
                    this->dijk->processNextNode();
                    currentNode = this->dijk->nextNode();
                    //todo: save all processed nodes (that are within the search radius)
                    //and keep all arcs with processed nodes as end nodes
                }
            }
        }
        else {
            //s-t search
            if (!this->allDests) {
                while (currentNode != INVALID &&
                        !this->dijk->emptyQueue()
                        && !this->dijk->processed(dest) //search only until dest is reached; -30 secs on 2 Mio arcs
                        ) {
                    this->dijk->processNextNode();
                    currentNode = this->dijk->nextNode();
                }
            }
            //shortest path tree
            else {
                while (currentNode != INVALID &&
                        !this->dijk->emptyQueue()
                        ) {
                    this->dijk->processNextNode();
                    currentNode = this->dijk->nextNode();
                }
            }
        }
    }
}

const uint32_t
 SPT_LEM::GetArcCount() {

    return lemon::countArcs(*this->g);
}

const uint32_t
 SPT_LEM::GetNodeCount() {

    return lemon::countNodes(*this->g);
}

void
 SPT_LEM::LoadNet(const uint32_t nmax,  const uint32_t mmax,
                      /*const lemon::FilterArcs<const netxpert::data::graph_t,
                                              const netxpert::data::graph_t::ArcMap<bool>>& sg,*/
                      lemon::FilterArcs<netxpert::data::graph_t,
                                              netxpert::data::graph_t::ArcMap<bool>>* sg,
                      netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>* cm)
{
    //TODO convert sg to g
    using namespace lemon;
    using namespace netxpert::data;

    //this->distMap = new SmartDigraph::NodeMap<double> (g);
    //this->length  = new SmartDigraph::ArcMap<double> (g);
    this->g = sg;
    this->costMap = cm;
    /*graph_t::NodeMap<node_t> node_ref(this->g);
    graph_t::ArcMap<netxpert::data::cost_t> sg_cost_map(this->g);
    graph_t::ArcMap<arc_t> arc_cross_ref(this->g);

    DigraphCopy<FilteredGraph, graph_t> cg(sg, this->g);
    cg.nodeRef(node_ref);
    cg.arcMap(cm, sg_cost_map);
    cg.arcCrossRef(arc_cross_ref);
    cg.run();
    // use tmp_graph*/
}

void SPT_LEM::SetOrigin( netxpert::data::node_t _origin )
{
	this->orig = _origin;
}

void SPT_LEM::SetDest( netxpert::data::node_t _dest )
{
	if (_dest != lemon::INVALID) {
		this->dest = _dest;
	}
	else {
		this->allDests = true;
	}
}
bool
 SPT_LEM::Reached( netxpert::data::node_t _node )
{
    if (this->bidirectional) {
        bool t1 = this->bijk->reached(_node);
        //bool t2 = this->bijk->revReached(_node);
        // true if either t1 or t2 is true
        //return t1 || t2;
        return t1;
    }
    else
        return this->dijk->reached(_node);
}

/* Walk in whole SPT is possible from specified orig and end
   but dest must be part of the SPT and
   an orig node must not be a dest node */
const std::vector<netxpert::data::node_t>
 SPT_LEM::GetPredecessors(netxpert::data::node_t _dest) {
    std::vector<netxpert::data::node_t> result;

    if (this->bidirectional) {
        for (netxpert::data::node_t v = _dest; v != this->orig; v = this->bijk->predNode(v)) {
          if (v != lemon::INVALID && this->bijk->reached(v)) {
             result.push_back(v);
          }
        }
    }
    else {
        for (netxpert::data::node_t v = _dest; v != this->orig; v = this->dijk->predNode(v)) {
          if (v != lemon::INVALID && this->dijk->reached(v)) {
             result.push_back(v);
          }
        }
    }
    result.push_back(this->orig);

	return result;
}

const netxpert::data::cost_t
 SPT_LEM::GetDist(netxpert::data::node_t _dest) {

    if (this->bidirectional) {
        if (this->bijk->reached(_dest))
            return this->bijk->dist(_dest);
    }
    else {
        if (this->dijk->reached(_dest))
            return this->dijk->dist(_dest);
    }
}

const std::vector<netxpert::data::arc_t>
 SPT_LEM::GetPath(netxpert::data::node_t _dest) {

    using namespace netxpert::data;

    std::vector<netxpert::data::arc_t> path;
    /// walk the path from dest to orig with the help of predNode
    /// and save the predArc for each node -->path
    if (this->bidirectional) {
        for (node_t v = _dest; v != this->orig; v = this->bijk->predNode(v)) {
            auto arc = this->bijk->predArc(v);
            if (arc != lemon::INVALID)
                path.push_back(arc);
        }
    }
    else {
        for (node_t v = _dest; v != this->orig; v = this->dijk->predNode(v)) {
            auto arc = this->dijk->predArc(v);
            path.push_back(arc);
//            std::cout << g->id(g->source(arc)) << "->" << g->id(g->target(arc)) << " , ";
        }
//        std::cout << std::endl;
    }

    //reverse arcs, so path goes from start to dest
    std::reverse(path.begin(), path.end());

    return path;
}

SPT_LEM::~SPT_LEM()
{
    //dtor
    /*if (this->bidirectional)
        delete bijk;
    else
        delete dijk;*/
}
