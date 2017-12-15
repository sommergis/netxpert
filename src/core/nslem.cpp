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

#include "nslem.h"

using namespace std;
using namespace lemon;
using namespace netxpert::core;

void
 NS_LEM::SolveMCF() {

    this->nsimplex = std::unique_ptr<netsimplex_t>(new netsimplex_t(*this->g, false));

	//set input maps
	this->nsimplex->upperMap(*this->capacityMap);
	this->nsimplex->costMap(*this->costMap);
	this->nsimplex->supplyMap(*this->supplyMap);

	this->status = nsimplex->run();
}

const uint32_t
 NS_LEM::GetArcCount() {

    return lemon::countArcs(*this->g);
}

const uint32_t
 NS_LEM::GetNodeCount() {

    return lemon::countNodes(*this->g);
}

void
 NS_LEM::LoadNet(const uint32_t nmax,  const uint32_t mmax,
                      lemon::FilterArcs<netxpert::data::graph_t,
                                              netxpert::data::graph_t::ArcMap<bool>>* _sg,
                      netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>* _costMap,
                      netxpert::data::graph_t::ArcMap<netxpert::data::capacity_t>* _capMap,
                      netxpert::data::graph_t::NodeMap<supply_t>* _supplyMap)
{
  using namespace lemon;
  using namespace netxpert::data;

  this->g             = _sg;
  this->costMap       = _costMap;
  this->capacityMap   = _capMap;
	this->supplyMap     = _supplyMap;
    //output (must be filtered_graph_t)
	this->flowMap = new netxpert::data::filtered_graph_t::ArcMap<flow_t>(*this->g);
}

const double
 NS_LEM::GetOptimum() const {
	return this->nsimplex->totalCost();
}

netxpert::data::graph_t::ArcMap<netxpert::data::flow_t>*
 NS_LEM::GetMCFFlow() {
    //query flow map after run for returning flow
	this->nsimplex->flowMap(*this->flowMap);

	return this->flowMap;
}

std::vector<netxpert::data::arc_t>
 NS_LEM::GetMCFArcs() {
    std::vector<netxpert::data::arc_t> result;

    return result;
}

netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>*
 NS_LEM::GetMCFCost() {

    return this->costMap;
}

void
 NS_LEM::PrintResult() {
 //   SmartDigraph::NodeMap<double>& distMapVal = *distMap;
	//cout << "***********************************************" << endl;
	//cout << "C++ Output START" << endl;
	//if (!allDests)
	//{
	//	cout << "Dest reached: " << dijk->reached(dest) << endl;
	//}
 //   cout << "Orig reached: " << dijk->reached(orig) << endl;
	//if (!allDests)
	//{
	//	cout << "Path cost to dest: " << distMapVal[dest] << endl;
	//}
	//cout << "C++ Output END" << endl;
	//cout << "***********************************************" << endl;
    //cout << "Path cost to orig: " << distMapVal[orig] << endl;
    //printf("%i\n", distMapVal[start]);
}


const int
 NS_LEM::GetMCFStatus() {
	/* LEMON

	   INFEASIBLE = 0
	   OPTIMAL = 1
	   UNBOUNDED = 2
	  */
	switch (status)
	{
	case netsimplex_t::INFEASIBLE:
		return 2;
		break;
	case netsimplex_t::OPTIMAL:
		return 0;
		break;
	case netsimplex_t::UNBOUNDED:
		return 3;
		break;
	default:
		return -1;
		break;
	}
}

NS_LEM::~NS_LEM() {

    if (this->flowMap)
        delete this->flowMap;
}

