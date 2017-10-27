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

#include "sptbgl.h"

using namespace boost;
using namespace netxpert::core;

SPT_BGL_Dijkstra::SPT_BGL_Dijkstra(uint32_t nmx, uint32_t mmx , bool Drctd)
{
    //nmax = 0;
	//mmax = 0;
	isDrctd = Drctd;
}

void SPT_BGL_Dijkstra::ShortestPathTree()
{
    netxpert::utils::LOGGER::LogInfo("dijkstra: spt start");

    /*graph_traits < DirectedGraph >::vertex_iterator vi, vend;
    for (boost::tie(vi, vend) = vertices(g); vi != vend; ++vi) {
        //std::cout << "distance(" << name[*vi] << ") = " << d[*vi] << ", ";
        vertex_descriptor v = *vi;
        std::cout << get(ColorMap, v) << std::endl;
    }*/

    boost::dijkstra_shortest_paths(this->g, this->orig,
                                    color_map( boost::make_iterator_property_map(this->colorMap.begin(),
                                                       boost::get(boost::vertex_index, this->g) ) )
                                    .distance_map( boost::make_iterator_property_map( this->distMap.begin(),
                                                     boost::get(boost::vertex_index, this->g)))
                                    .predecessor_map( boost::make_iterator_property_map( this->predMap.begin(),
                                                     boost::get(boost::vertex_index, this->g)))
                                    );

    /*graph_traits < DirectedGraph >::vertex_iterator vi2, vend2;
    for (boost::tie(vi2, vend2) = vertices(g); vi2 != vend2; ++vi2) {
        //std::cout << "distance(" << name[*vi] << ") = " << d[*vi] << ", ";
        vertex_descriptor v = *vi2;
        std::cout << get(ColorMap, v) << std::endl;
    }
    */

    /*graph_traits < DirectedGraph >::vertex_iterator vi3, vend3;
    for (boost::tie(vi3, vend3) = vertices(g); vi3 != vend3; ++vi3) {
        std::cout << "distance " << this->distMap[*vi3] << std::endl;
        //vertex_descriptor v = *vi3;
        //std::cout << get(DistanceMap, v) << std::endl;
        std::cout << "color " << this->colorMap[*vi3] << std::endl;
    }*/

    //boost::two_bit_color_map< std::vector<boost::default_color_type> > color(num_vertices(g), vertex_index_map( boost::get(boost::vertex_index, this->g));


    //Backup:
    /*
    boost::dijkstra_shortest_paths(this->g, this->orig,

        //vertex_index_map( boost::make_iterator_property_map( this->nodes.begin(),
        //                  boost::get(boost::vertex_index, this->g) )
        predecessor_map( boost::make_iterator_property_map( this->predMap.begin(),
                          boost::get(boost::vertex_index, this->g) ) )//.
        //vertex_index_map( boost::get(boost::vertex_index, this->g) )//.
        //vertex_color_map( color )
        //vertex_color_map( boost::make_iterator_property_map(this->colorMap.begin(),
        //            boost::get(boost::vertex_index, this->g) ))
        ); //this->colorMap.data() ));*/

    netxpert::utils::LOGGER::LogInfo("dijkstra: spt stop");

    //boost::property_map< netxpert::core::DirectedGraph, boost::vertex_color_t >::type c =
    //        boost::get(boost::vertex_color, g);

    //auto cMap = boost::get(boost::vertex_color_map(verte), this->g);

    //for (auto n : this->nodes)
    //{
    //    auto v_color = get(color_map(colorMap), n);
        //std::cout << v_color << std::endl;
        //std::cout << n << ": " << this->colorMap[n] << std::endl;
    //}


    //for (auto n : w)
    //    std::cout << *n << std::endl;


    /*netxpert::utils::LOGGER::LogInfo("dijkstra_no_color_map: spt start");
    boost::dijkstra_shortest_paths_no_color_map(this->g, this->orig,
        predecessor_map(&(this->predMap[0])));
    netxpert::utils::LOGGER::LogInfo("dijkstra_no_color_map: spt stop");

    netxpert::utils::LOGGER::LogInfo("dijkstra_visitor: spt start");
    try
    {
            boost::dijkstra_shortest_paths(this->g, this->orig,
             predecessor_map(&(this->predMap[0]))
            //.color_map(&(this->colorMap[0]))
            //.distance_map(&(this->distMap[0]))
            .visitor(target_visit(this->dest, on_examine_vertex())));
    }
    catch (int& ex){}

    netxpert::utils::LOGGER::LogInfo("dijkstra_visitor: spt stop");

    netxpert::utils::LOGGER::LogInfo("dijkstra_no_color_map_visitor: spt start");
    try
    {
            boost::dijkstra_shortest_paths_no_color_map(this->g, this->orig,
             predecessor_map(&(this->predMap[0]))
            //.color_map(&(this->colorMap))
            //.distance_map(&(this->distMap[0]))
            .visitor(target_visit(this->dest, on_examine_vertex())));
    }
    catch (int& ex){}
    netxpert::utils::LOGGER::LogInfo("dijkstra_no_color_map_visitor: spt stop");

    */


    /*if (this->orig)
        boost::dijkstra_shortest_paths(this->g, this->orig,
        predecessor_map(&(this->predMap[0])));
        //.color_map(&(this->colorMap))
        //.distance_map(&(this->distMap[0])));
    if (this->orig && this->dest)
    {
        //std::cout << "running s-t mode.." << std::endl;
        //try
        //{
            boost::dijkstra_shortest_paths(this->g, this->orig,
             predecessor_map(&(this->predMap[0])));
            //.color_map(&(this->colorMap))
            //.distance_map(&(this->distMap[0]))
            //.visitor(target_visit(this->dest, on_examine_vertex())));
        //}
        //catch (int& ex)
        //{

        //}
    }*/
}

void SPT_BGL_Dijkstra::LoadNet( uint32_t nmx , uint32_t mmx , uint32_t pn , uint32_t pm ,
		      double *pU , double *pC , double *pDfct ,
		      uint32_t *pSn , uint32_t *pEn )
{
    nmax = nmx;
	mmax = mmx;

	//this->ColorMap = get(boost::vertex_color, g);
	//this->DistanceMap = get(boost::vertex_distance, g);

    this->distMap.resize(nmax);
    //this->costMap.resize(mmax);
    this->predMap.resize(nmax);
    this->colorMap.resize(nmax);

    int origNode;
    int destNode;
    double cost;

    nodes.resize(nmax);

    for (uint32_t i = 0; i < nmax; ++i){
        nodes[i] = boost::add_vertex(g);
        //vertex_descriptor v = nodes[i];

        //set grey color for all vertices
        //this->colorMap[v] = boost::two_bit_color_type::two_bit_gray;
        //this->colorMap.insert( std::make_pair(nodes[i],boost::default_color_type::white_color));
    }

    if (isDrctd)
    {
		for (uint32_t i = 0; i < mmx; ++i) {
			origNode = pSn[i];
			destNode = pEn[i];
			cost = pC[i];
			//std::pair<edge_descriptor,bool> edge
            boost::add_edge(nodes[origNode-1],nodes[destNode-1], cost, this->g);
			//costMap[edge] = cost;
			//std::cout << "Arc: #" << i << " " << origNode << " " << destNode << " " << cost << std::endl;
		}
	}
    else // both directions
	{
		for (uint32_t i = 0; i < mmx; ++i) {
			origNode = pSn[i];
			destNode = pEn[i];
			cost = pC[i];
            boost::add_edge(nodes[origNode-1],nodes[destNode-1], cost, this->g);
			//costMap[edge] = cost;
			boost::add_edge(nodes[destNode-1],nodes[origNode-1], cost, this->g);
			//costMap[edge] = cost;
			//std::cout << "Arc: #" << i << " " << origNode << " " << destNode << " " << cost <<  std::endl;
		}
	}
}
void SPT_BGL_Dijkstra::SetOrigin( uint32_t NewOrg )
{
    //1. uint to BGL Vertex
	this->orig = nodes[NewOrg-1];
}

void SPT_BGL_Dijkstra::SetDest( uint32_t NewDst )
{
    //1. uint to BGL Vertex
	if (NewDst != UINT_MAX)
	{
		this->dest = nodes[NewDst-1];
	}
}
//TODO
bool SPT_BGL_Dijkstra::Reached( uint32_t NodeID )
{
    //1. uint to BGL Vertex
    //std::cout << NodeID << std::endl;
	vertex_descriptor node = nodes[NodeID-1];
    //http://www.boost.org/doc/libs/1_60_0/libs/graph/doc/dijkstra_shortest_paths.html
    //At the end of the algorithm, vertices reachable from the source vertex will have been colored black.
    //All other vertices will still be white.
    //std::cout << node << std::endl;
    //std::cout << this->colorMap[node] << std::endl;

    //auto color_map = vertex_color_map( get(vertex_, g));
    //auto cm = color_map( make_iterator_property_map( this->colorMap.begin(), get(boost::vertex_color_t(), g)));
    auto e = get(boost::edge_weight_t(), g);
    //auto blue = get(boost::vertex_color_t(), g);
    //boost::property_map<DirectedGraph, boost::vertex_color_t>::type ColorMap = get(vertex_color, g);
    //boost::property_map<DirectedGraph, boost::edge_weight_t>::type EdgeWeightMap = get(boost::edge_weight_t(), g);

    graph_traits < DirectedGraph >::vertex_iterator vi, vend;
    for (boost::tie(vi, vend) = vertices(g); vi != vend; ++vi) {
        //std::cout << "distance(" << name[*vi] << ") = " << d[*vi] << ", ";
        vertex_descriptor v = *vi;
        //std::cout << get(ColorMap, v) << std::endl;

        //vertexColor vc = *vi;
        //std::cout << "index: " << g[v].index << std::endl;
        //std::cout << "color: " << g[v].color << std::endl;

        //std::cout << "name: " << std::to_string( vc ) << std::endl;
        //std::cout << "dist: " << get(vertex_distance, v) << std::endl;
        //std::cout << "centrality: " << get(vertex_centrality, v) << std::endl;
        //std::cout << "name: " << get(&, v) << std::endl;

//        std::cout << get(vertex_color, v) << std::endl;
        //std::cout << v[0] << std::endl;
        //auto c = boost::get( color_map , v);
        //std::cout << get() << std::endl;

        //std::cout << "color: " << g[v] << std::endl;
    }


    for (auto n : this->nodes)
    {

        //std::cout << n << ": " << this->colorMap[n] << std::endl; //this->colorMap[n] << std::endl;

        //auto color = boost::get(&boost::Graph::NodePropertiesType::color, this->g);
        //std::cout << color << std::endl;
    }
    //return (this->colorMap[node] == boost::default_color_type::black_color);
    return false;
}

/*
uint32_t* SPT_BGL_Dijkstra::ArcPredecessors( void )
{
	throw std::runtime_error("ArcPredecessors() not implemented!");
}*/

uint32_t* SPT_BGL_Dijkstra::Predecessors( void )
{
	predecessors = new uint32_t [MCFnmax()+1];
	predecessors[0] = 0; //first entry of pred has no predecessor
    for (int i = 1; i < MCFnmax()+1; i++)
    {
        predecessors[i] = this->predMap[nodes[i-1]] + 1;
    }
	return predecessors;
}

/*void SPT_BGL_Dijkstra::GetArcPredecessors( uint32_t *outArcPrd )
{
	throw std::runtime_error("GetArcPredecessors() not implemented!");
}*/

void SPT_BGL_Dijkstra::GetPredecessors( uint32_t *outPrd )
{
	uint32_t size = MCFnmax()+1;
	auto predecessors = Predecessors();
	memcpy(outPrd, predecessors, size * sizeof (uint32_t ) );
	delete[] predecessors;
}

uint32_t SPT_BGL_Dijkstra::MCFnmax()
{
	return ( this->nmax );
}

uint32_t SPT_BGL_Dijkstra::MCFmmax()
{
	return ( this->mmax );
}

SPT_BGL_Dijkstra::~SPT_BGL_Dijkstra()
{
    //dtor
}
