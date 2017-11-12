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

#include "mstlem.h"

using namespace netxpert::core;

MST_LEM::MST_LEM() {
    //ctor
}

MST_LEM::~MST_LEM() {
	//TEST first
    if (this->arcBoolMap)
        delete arcBoolMap;
}

void
MST_LEM::SolveMST() {
	this->arcBoolMap = new netxpert::data::filtered_graph_t::ArcMap<bool>(*this->g);

	this->totalCost = lemon::kruskal(*this->g, *this->costMap, *this->arcBoolMap);
}

void
MST_LEM::LoadNet(const uint32_t nmax,  const uint32_t mmax,
                      lemon::FilterArcs<netxpert::data::graph_t,
                                              netxpert::data::graph_t::ArcMap<bool>>* sg,
                      netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>* cm) {

    //TODO convert sg to g
    using namespace lemon;
    using namespace netxpert::data;

    this->g = sg;
    this->costMap = cm;
	//cout << "ready loading arcs" << endl;
}

const uint32_t
 MST_LEM::GetArcCount() {
    return lemon::countArcs(*this->g);
}

const uint32_t
 MST_LEM::GetNodeCount() {
    return lemon::countNodes(*this->g);
}

std::vector<netxpert::data::arc_t>
MST_LEM::GetMST () {

	//query arcBoolMap for getting the min span tree
	std::vector<netxpert::data::arc_t> result;

	for(netxpert::data::filtered_graph_t::ArcIt it(*this->g); it!=INVALID; ++it) {
		//If Arc is a part of the MST the following is true
		if ((*this->arcBoolMap)[it]) {
			result.push_back(it);
		}
	}
	return result;
}

inline const double
MST_LEM::GetOptimum() const {
	return this->totalCost;
}

