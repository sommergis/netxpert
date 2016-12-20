#ifndef SPTBGL_H
#define SPTBGL_H

#include <string.h>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/dijkstra_shortest_paths_no_color_map.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/property_map/vector_property_map.hpp>
#include "isptree.h"
#include "logger.h"

namespace netxpert {
    namespace core {

    typedef boost::property<boost::edge_weight_t, double> EdgeWeightProperty;
    typedef boost::property<boost::vertex_index_t, int,
                             boost::property<boost::vertex_color_t, int ,
                             boost::property<boost::vertex_distance_t, double>  > > VertexProperty;

    typedef boost::property<boost::vertex_name_t, std::string> VertexColor;

    struct Vertex
    {
        int index;
        std::string name;
        boost::default_color_type color;
    };

    typedef boost::adjacency_list<boost::vecS,
        boost::vecS, boost::directedS, VertexProperty, EdgeWeightProperty> DirectedGraph;
    typedef boost::graph_traits<DirectedGraph>::edge_iterator edge_iterator;
    typedef boost::graph_traits<DirectedGraph>::vertex_descriptor vertex_descriptor;
    typedef boost::graph_traits<DirectedGraph>::edge_descriptor edge_descriptor;


    template <class Vertex, class Tag>
    struct target_visitor : public boost::default_dijkstra_visitor
    {
        target_visitor(Vertex u) : v(u) { }

        template <class Graph>
        void examine_vertex(Vertex u, Graph& g)
        {

            if ( u == v ) {

                //std::cout << "Target reached" << std::endl;
                netxpert::utils::LOGGER::LogDebug("Target reached: " + std::to_string(u));

                //throw std::runtime_error("Target reached!");
                throw(-1); // execution for dijkstra stops if reached

            }
        }
        private:
            Vertex v;
    };

    template <class Vertex, class Tag>
    target_visitor<Vertex, Tag> target_visit(Vertex u, Tag) {
        return target_visitor<Vertex, Tag>(u);
    }


    /**
    *  \Class Core Solver for the Shortest Path Tree Problem with d-ary Heap structure and
    *   Dijkstra's algorithm of Boost Graph Library.
    *   EXPERIMENTAL!
    */
    class SPT_BGL_Dijkstra : public netxpert::core::ISPTree
    {
        public:
            /** Default constructor */
            SPT_BGL_Dijkstra(unsigned int nmx = 0 , unsigned int mmx = 0 , bool Drctd = true);
            /** Default destructor */
            virtual ~SPT_BGL_Dijkstra();

            /* MCFClass SPTree Interface */
            void ShortestPathTree();
            void LoadNet( unsigned int nmx , unsigned int mmx , unsigned int pn , unsigned int pm ,
                  double *pU , double *pC , double *pDfct ,
                  unsigned int *pSn , unsigned int *pEn );
            unsigned int MCFnmax( void );
            unsigned int MCFmmax( void );
            void SetOrigin( unsigned int NewOrg );
            void SetDest( unsigned int NewDst );
            bool Reached( unsigned int NodeID );
            //unsigned int* ArcPredecessors( void );
            unsigned int* Predecessors( void );
            //void GetArcPredecessors ( unsigned int *outArcPrd );
            void GetPredecessors( unsigned int *outPrd );

            ISPTreePtr create () const        // Virtual constructor (creation)
            {
                return ISPTreePtr(new SPT_BGL_Dijkstra() );
            }
            ISPTreePtr clone () const        // Virtual constructor (copying)
            {
                return ISPTreePtr(new SPT_BGL_Dijkstra(*this));
            }

        protected:
            unsigned int nmax; //max count nodes
            unsigned int mmax; //max count arcs

        private:
            bool isDrctd;
            bool allDests;
            DirectedGraph g;
            std::vector<vertex_descriptor> nodes;
            std::vector<double> costMap;
            std::vector<double> distMap;
            std::vector<vertex_descriptor> predMap;
            std::vector<boost::two_bit_color_type> colorMap;
            vertex_descriptor orig;
            vertex_descriptor dest;
            unsigned int* predecessors;

            //test
            // accessors
            boost::property_map<DirectedGraph, boost::vertex_color_t>::type ColorMap;
            boost::property_map<DirectedGraph, boost::vertex_distance_t>::type DistanceMap;

    };
} //namespace core
} //namespace netxpert

#endif // SPTBGL_H
