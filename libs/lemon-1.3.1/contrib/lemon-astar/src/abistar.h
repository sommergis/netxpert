//TODO: if only one direction is called...
/* -*- mode: C++; indent-tabs-mode: nil; -*-
 *
 * This file is a part of LEMON, a generic C++ optimization library.
 *
 * Copyright (C) 2003-2010
 * Egervary Jeno Kombinatorikus Optimalizalasi Kutatocsoport
 * (Egervary Research Group on Combinatorial Optimization, EGRES).
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

#ifndef LEMON_ABISTAR_H
#define LEMON_ABISTAR_H

///\ingroup shortest_path
///\file
///\brief ABiStar algorithm.

#include <limits>
#include <lemon/list_graph.h>
#include <lemon/bin_heap.h>
#include <lemon/bits/path_dump.h>
#include <lemon/core.h>
#include <lemon/error.h>
#include <lemon/maps.h>
#include <lemon/path.h>
#include <iostream>

namespace lemon {

  /// \brief Default operation traits for the ABiStar algorithm class.
  ///
  /// This operation traits class defines all computational operations and
  /// constants which are used in the ABiStar algorithm.
  template <typename V>
  struct ABiStarDefaultOperationTraits {
    /// \e
    typedef V Value;
    /// \brief Gives back the zero value of the type.
    static Value zero() {
      return static_cast<Value>(0);
    }
    /// \brief Gives back the sum of the given two elements.
    static Value plus(const Value& left, const Value& right) {
      return left + right;
    }
    /// \brief Gives back true only if the first value is less than the second.
    static bool less(const Value& left, const Value& right) {
      return left < right;
    }
  };

  ///Default traits class of ABiStar class.

  ///Default traits class of ABiStar class.
  ///\tparam GR The type of the digraph.
  ///\tparam LEN The type of the length map.
  template<typename GR, typename LEN>
  struct ABiStarDefaultTraits
  {
    ///The type of the digraph the algorithm runs on.
    typedef GR Digraph;

    ///The type of the map that stores the arc lengths.

    ///The type of the map that stores the arc lengths.
    ///It must conform to the \ref concepts::ReadMap "ReadMap" concept.
    typedef LEN LengthMap;
    ///The type of the arc lengths.
    typedef typename LEN::Value Value;

    /// Operation traits for %ABiStar algorithm.

    /// This class defines the operations that are used in the algorithm.
    /// \see ABiStarDefaultOperationTraits
    typedef ABiStarDefaultOperationTraits<Value> OperationTraits;

    /// The cross reference type used by the heap.

    /// The cross reference type used by the heap.
    /// Usually it is \c Digraph::NodeMap<int>.
    typedef typename Digraph::template NodeMap<int> HeapCrossRef;
    ///Instantiates a \c HeapCrossRef.

    ///This function instantiates a \ref HeapCrossRef.
    /// \param g is the digraph, to which we would like to define the
    /// \ref HeapCrossRef.
    static HeapCrossRef *createHeapCrossRef(const Digraph &g)
    {
      return new HeapCrossRef(g);
    }

    ///The heap type used by the %ABiStar algorithm.

    ///The heap type used by the ABiStar algorithm.
    ///
    ///\sa BinHeap
    ///\sa ABiStar
    typedef BinHeap<typename LEN::Value, HeapCrossRef, std::less<Value> > Heap;
    ///Instantiates a \c Heap.

    ///This function instantiates a \ref Heap.
    static Heap *createHeap(HeapCrossRef& r)
    {
      return new Heap(r);
    }

    ///\brief The type of the map that stores the predecessor
    ///arcs of the shortest paths.
    ///
    ///The type of the map that stores the predecessor
    ///arcs of the shortest paths.
    ///It must conform to the \ref concepts::WriteMap "WriteMap" concept.
    typedef typename Digraph::template NodeMap<typename Digraph::Arc> PredMap;
    ///Instantiates a \c PredMap.

    ///This function instantiates a \ref PredMap.
    ///\param g is the digraph, to which we would like to define the
    ///\ref PredMap.
    static PredMap *createPredMap(const Digraph &g)
    {
      return new PredMap(g);
    }

    ///The type of the map that indicates which nodes are processed.

    ///The type of the map that indicates which nodes are processed.
    ///It must conform to the \ref concepts::WriteMap "WriteMap" concept.
    ///By default, it is a NullMap.
    typedef NullMap<typename Digraph::Node,bool> ProcessedMap;
    ///Instantiates a \c ProcessedMap.

    ///This function instantiates a \ref ProcessedMap.
    ///\param g is the digraph, to which
    ///we would like to define the \ref ProcessedMap.
#ifdef DOXYGEN
    static ProcessedMap *createProcessedMap(const Digraph &g)
#else
    static ProcessedMap *createProcessedMap(const Digraph &)
#endif
    {
      return new ProcessedMap();
    }

    ///The type of the map that stores the distances of the nodes.

    ///The type of the map that stores the distances of the nodes.
    ///It must conform to the \ref concepts::WriteMap "WriteMap" concept.
    typedef typename Digraph::template NodeMap<typename LEN::Value> DistMap;
    ///Instantiates a \c DistMap.

    ///This function instantiates a \ref DistMap.
    ///\param g is the digraph, to which we would like to define
    ///the \ref DistMap.
    static DistMap *createDistMap(const Digraph &g)
    {
      return new DistMap(g);
    }
  };

  
  ///Default Estimate Calculator (the heuristic of the algorithm) class of ABiStar class.

  ///Default Estimate Calculator (the heuristic of the algorithm) class of ABiStar class.
  ///\tparam GR The type of the digraph.
  ///\tparam LEN The type of the length map.
  ///\tparam RET The type of the heuristic's return Value.
  template<typename GR, typename LEN,typename RET = typename LEN::Value>
  class DefaultABiStarEstimateCalculator
  {
    typedef GR Digraph;
    typedef typename Digraph::Node Node;
    typedef ABiStarDefaultOperationTraits<RET> OperationTraits;
    
    public:
    /// \brief Gives back the value of the heuristic.
    RET operator()(const Node& u, const Node& v) const
    {
      return OperationTraits::zero();
    }
  };
  
  ///%ABiStar algorithm class.

  /// \ingroup shortest_path
  ///This class provides an efficient implementation of the %ABiStar algorithm.
  ///
  ///The %ABiStar algorithm solves the single-source shortest path problem
  ///when all arc lengths are non-negative. If there are negative lengths,
  ///the BellmanFord algorithm should be used instead.
  ///
  ///The arc lengths are passed to the algorithm using a
  ///\ref concepts::ReadMap "ReadMap",
  ///so it is easy to change it to any kind of length.
  ///The type of the length is determined by the
  ///\ref concepts::ReadMap::Value "Value" of the length map.
  ///It is also possible to change the underlying priority heap.
  ///
  ///There is also a \ref abistar() "function-type interface" for the
  ///%ABiStar algorithm, which is convenient in the simplier cases and
  ///it can be used easier.
  ///
  ///\tparam GR The type of the digraph the algorithm runs on.
  ///The default type is \ref ListDigraph.
  ///\tparam LEN A \ref concepts::ReadMap "readable" arc map that specifies
  ///the lengths of the arcs.
  ///It is read once for each arc, so the map may involve in
  ///relatively time consuming process to compute the arc lengths if
  ///it is necessary. The default map type is \ref
  ///concepts::Digraph::ArcMap "GR::ArcMap<int>".
  ///\tparam EC The type of the heuristic the algorithm uses.
  ///By default, it is \ref DefaultABiStarEstimateCalculator, which
  ///gives 0 for any two nodes, so with this the 
  ///algorithm is identical with the original Dijkstra.
  ///\tparam TR The traits class that defines various types used by the
  ///algorithm. By default, it is \ref ABiStarDefaultTraits
  ///"ABiStarDefaultTraits<GR, LEN>".
  ///In most cases, this parameter should not be set directly,
  ///consider to use the named template parameters instead.
  //update
#ifdef DOXYGEN
  template <typename GR, typename LEN, typename EC, typename TR>
#else
  template <typename GR=ListDigraph,
            typename LEN=typename GR::template ArcMap<int>,
            typename EC=DefaultABiStarEstimateCalculator<GR,LEN>,
            typename TR=ABiStarDefaultTraits<GR,LEN> >
#endif
  class ABiStar {
  public:

    ///The type of the digraph the algorithm runs on.
    typedef typename TR::Digraph Digraph;

    ///The type of the arc lengths.
    typedef typename TR::Value Value;
    ///The type of the map that stores the arc lengths.
    typedef typename TR::LengthMap LengthMap;
    ///\brief The type of the map that stores the predecessor arcs of the
    ///shortest paths.
    typedef typename TR::PredMap PredMap;
    ///The type of the map that stores the distances of the nodes.
    typedef typename TR::DistMap DistMap;
    ///The type of the map that indicates which nodes are processed.
    typedef typename TR::ProcessedMap ProcessedMap;
    ///The type of the paths.
    typedef PredMapPath<Digraph, PredMap> Path;
    ///The cross reference type used for the current heap.
    typedef typename TR::HeapCrossRef HeapCrossRef;
    ///The heap type used by the algorithm.
    typedef typename TR::Heap Heap;
    ///\brief The \ref ABiStarDefaultOperationTraits "operation traits class"
    ///of the algorithm.
    typedef typename TR::OperationTraits OperationTraits;

    ///The \ref ABiStarDefaultTraits "traits class" of the algorithm.
    typedef TR Traits;
    
    ///The type of the heuristic the algorithm uses.
    typedef EC EstimateCalculator;

  private:

    typedef typename Digraph::Node Node;
    typedef typename Digraph::NodeIt NodeIt;
    typedef typename Digraph::Arc Arc;
    typedef typename Digraph::OutArcIt OutArcIt;
    typedef typename Digraph::InArcIt InArcIt;

    //Pointer to the underlying digraph.
    const Digraph *G;
    //Pointer to the length map.
    const LengthMap *_length;
    //Pointer to the heuristic
    const EstimateCalculator *est_calc;//TODO: maybe another for reverse??
    //Pointer to the map of predecessors arcs.
    PredMap *_pred;
    //Pointer to the map of reverse predecessors arcs
    PredMap *_rev_pred;
    //Indicates if _pred is locally allocated (true) or not.
    bool local_pred;
    //Indicates if _rev_pred is locally allocated (true) or not.
    bool local_rev_pred;
    //Pointer to the map of distances.
    DistMap *_dist;
    //Pointer to the map of reverse distances.
    DistMap *_rev_dist;
    //Indicates if _dist is locally allocated (true) or not.
    bool local_dist;
    //Indicates if _rev_dist is locally allocated (true) or not.
    bool local_rev_dist;
    //Pointer to the map of currently known distances.
    DistMap *_current_dist;
    //Pointer to the map of currently known reverse distances.
    DistMap *_current_rev_dist;
    //Indicates if _current_dist is locally allocated (true) or not.
    bool local_current_dist;
    //Indicates if _current_rev_dist is locally allocated (true) or not.
    bool local_current_rev_dist;
    //Pointer to the map of processed status of the nodes.
    ProcessedMap *_processed;
    //Pointer to the map of reversely processed status of the nodes.
    ProcessedMap *_rev_processed;
    //Indicates if _processed is locally allocated (true) or not.
    bool local_processed;
    //Indicates if _rev_processed is locally allocated (true) or not.
    bool local_rev_processed;
    //Pointer to the heap cross references.
    HeapCrossRef *_heap_cross_ref;
    //Pointer to the reverse heap cross references.
    HeapCrossRef *_rev_heap_cross_ref;
    //Indicates if _heap_cross_ref is locally allocated (true) or not.
    bool local_heap_cross_ref;
    //Indicates if _rev_heap_cross_ref is locally allocated (true) or not.
    bool local_rev_heap_cross_ref;
    //Pointer to the heap.
    Heap *_heap;
    //Pointer to the reverse heap
    Heap *_rev_heap;
    //Indicates if _heap is locally allocated (true) or not.
    bool local_heap;
    //Indicates if _rev_heap is locally allocated (true) or not.
    bool local_rev_heap;
    //Indicates whether the forward (false) or the reverse (true) side of the algorithm is to be run next. 
    bool reverse_is_next;
    //Indicates whether any path has been found (true) or not.
    bool bi_path_found;
    //Stores the id of the node where the two side of the (currently known) shortest path connects.
    int con_node_id;
    //Stores the distance of the (currently known) shortest path.
    Value shortest_dist;
    //Indicates whether the absolute whortest path has been found (true) or not.
    bool bi_dist_found;
    //Stores the value of the last processed item of the star heap.
    Value laststar;
    //Stores the value of the last processed item of the reverse star heap.
    Value lastrevstar;
    
    //variables for testing
    long int forwardscannedcounter;
    long int reversescannedcounter;
    long int forwardreachedcounter;
    long int reversereachedcounter;
    long int forwardinsertedcounter;
    long int reverseinsertedcounter;
    long int forwarddecreasedcounter;
    long int reversedecreasedcounter;
    long int forwardimportantcounter;
    long int reverseimportantcounter;
    long int forwardrejectedcounter;
    long int reverserejectedcounter;
    
    //Creates the maps if necessary.
    //TODO: maybe separate creation of reverse maps from the forward ones?
    void create_maps()
    {
      if(!_pred) {
        local_pred = true;
        _pred = Traits::createPredMap(*G);
      }
      if(!_rev_pred) {
        local_rev_pred = true;
        _rev_pred = Traits::createPredMap(*G);
      }
      if(!_dist) {
        local_dist = true;
        _dist = Traits::createDistMap(*G);
      }
      if(!_rev_dist) {
        local_rev_dist = true;
        _rev_dist = Traits::createDistMap(*G);
      }
      if (!_current_dist) {
        local_current_dist = true;
        _current_dist = Traits::createDistMap(*G);
      }
      if (!_current_rev_dist) {
        local_current_rev_dist = true;
        _current_rev_dist = Traits::createDistMap(*G);
      }
      if(!_processed) {
        local_processed = true;
        _processed = Traits::createProcessedMap(*G);
      }
      if(!_rev_processed) {
        local_rev_processed = true;
        _rev_processed = Traits::createProcessedMap(*G);
      }
      if (!_heap_cross_ref) {
        local_heap_cross_ref = true;
        _heap_cross_ref = Traits::createHeapCrossRef(*G);
      }
      if (!_rev_heap_cross_ref) {
        local_rev_heap_cross_ref = true;
        _rev_heap_cross_ref = Traits::createHeapCrossRef(*G);
      }
      if (!_heap) {
        local_heap = true;
        _heap = Traits::createHeap(*_heap_cross_ref);
      }
      if (!_rev_heap) {
        local_rev_heap = true;
        _rev_heap = Traits::createHeap(*_rev_heap_cross_ref);
      }
    }

  public:

    typedef ABiStar Create;

    ///\name Named Template Parameters

    ///@{

    template <class T>
    struct SetPredMapTraits : public Traits {
      typedef T PredMap;
      static PredMap *createPredMap(const Digraph &)
      {
        LEMON_ASSERT(false, "PredMap is not initialized");
        return 0; // ignore warnings
      }
    };
    ///\brief \ref named-templ-param "Named parameter" for setting
    ///\c PredMap type.
    ///
    ///\ref named-templ-param "Named parameter" for setting
    ///\c PredMap type.
    ///It must conform to the \ref concepts::WriteMap "WriteMap" concept.
    template <class T>
    struct SetPredMap
      : public ABiStar< Digraph, LengthMap, SetPredMapTraits<T> > {
      typedef ABiStar< Digraph, LengthMap, SetPredMapTraits<T> > Create;
    };

    template <class T>
    struct SetDistMapTraits : public Traits {
      typedef T DistMap;
      static DistMap *createDistMap(const Digraph &)
      {
        LEMON_ASSERT(false, "DistMap is not initialized");
        return 0; // ignore warnings
      }
    };
    ///\brief \ref named-templ-param "Named parameter" for setting
    ///\c DistMap type.
    ///
    ///\ref named-templ-param "Named parameter" for setting
    ///\c DistMap type.
    ///It must conform to the \ref concepts::WriteMap "WriteMap" concept.
    template <class T>
    struct SetDistMap
      : public ABiStar< Digraph, LengthMap, SetDistMapTraits<T> > {
      typedef ABiStar< Digraph, LengthMap, SetDistMapTraits<T> > Create;
    };

    template <class T>
    struct SetProcessedMapTraits : public Traits {
      typedef T ProcessedMap;
      static ProcessedMap *createProcessedMap(const Digraph &)
      {
        LEMON_ASSERT(false, "ProcessedMap is not initialized");
        return 0; // ignore warnings
      }
    };
    ///\brief \ref named-templ-param "Named parameter" for setting
    ///\c ProcessedMap type.
    ///
    ///\ref named-templ-param "Named parameter" for setting
    ///\c ProcessedMap type.
    ///It must conform to the \ref concepts::WriteMap "WriteMap" concept.
    template <class T>
    struct SetProcessedMap
      : public ABiStar< Digraph, LengthMap, SetProcessedMapTraits<T> > {
      typedef ABiStar< Digraph, LengthMap, SetProcessedMapTraits<T> > Create;
    };

    struct SetStandardProcessedMapTraits : public Traits {
      typedef typename Digraph::template NodeMap<bool> ProcessedMap;
      static ProcessedMap *createProcessedMap(const Digraph &g)
      {
        return new ProcessedMap(g);
      }
    };
    ///\brief \ref named-templ-param "Named parameter" for setting
    ///\c ProcessedMap type to be <tt>Digraph::NodeMap<bool></tt>.
    ///
    ///\ref named-templ-param "Named parameter" for setting
    ///\c ProcessedMap type to be <tt>Digraph::NodeMap<bool></tt>.
    ///If you don't set it explicitly, it will be automatically allocated.
    struct SetStandardProcessedMap
      : public ABiStar< Digraph, LengthMap, SetStandardProcessedMapTraits > {
      typedef ABiStar< Digraph, LengthMap, SetStandardProcessedMapTraits >
      Create;
    };

    template <class H, class CR>
    struct SetHeapTraits : public Traits {
      typedef CR HeapCrossRef;
      typedef H Heap;
      static HeapCrossRef *createHeapCrossRef(const Digraph &) {
        LEMON_ASSERT(false, "HeapCrossRef is not initialized");
        return 0; // ignore warnings
      }
      static Heap *createHeap(HeapCrossRef &)
      {
        LEMON_ASSERT(false, "Heap is not initialized");
        return 0; // ignore warnings
      }
    };
    ///\brief \ref named-templ-param "Named parameter" for setting
    ///heap and cross reference types
    ///
    ///\ref named-templ-param "Named parameter" for setting heap and cross
    ///reference types. If this named parameter is used, then external
    ///heap and cross reference objects must be passed to the algorithm
    ///using the \ref heap() function before calling \ref run(Node) "run()"
    ///or \ref init().
    ///\sa SetStandardHeap
    template <class H, class CR = typename Digraph::template NodeMap<int> >
    struct SetHeap
      : public ABiStar< Digraph, LengthMap, SetHeapTraits<H, CR> > {
      typedef ABiStar< Digraph, LengthMap, SetHeapTraits<H, CR> > Create;
    };

    template <class H, class CR>
    struct SetStandardHeapTraits : public Traits {
      typedef CR HeapCrossRef;
      typedef H Heap;
      static HeapCrossRef *createHeapCrossRef(const Digraph &G) {
        return new HeapCrossRef(G);
      }
      static Heap *createHeap(HeapCrossRef &R)
      {
        return new Heap(R);
      }
    };
    ///\brief \ref named-templ-param "Named parameter" for setting
    ///heap and cross reference types with automatic allocation
    ///
    ///\ref named-templ-param "Named parameter" for setting heap and cross
    ///reference types with automatic allocation.
    ///They should have standard constructor interfaces to be able to
    ///automatically created by the algorithm (i.e. the digraph should be
    ///passed to the constructor of the cross reference and the cross
    ///reference should be passed to the constructor of the heap).
    ///However, external heap and cross reference objects could also be
    ///passed to the algorithm using the \ref heap() function before
    ///calling \ref run(Node) "run()" or \ref init().
    ///\sa SetHeap
    template <class H, class CR = typename Digraph::template NodeMap<int> >
    struct SetStandardHeap
      : public ABiStar< Digraph, LengthMap, SetStandardHeapTraits<H, CR> > {
      typedef ABiStar< Digraph, LengthMap, SetStandardHeapTraits<H, CR> >
      Create;
    };

    template <class T>
    struct SetOperationTraitsTraits : public Traits {
      typedef T OperationTraits;
    };

    /// \brief \ref named-templ-param "Named parameter" for setting
    ///\c OperationTraits type
    ///
    ///\ref named-templ-param "Named parameter" for setting
    ///\c OperationTraits type.
    /// For more information, see \ref ABiStarDefaultOperationTraits.
    template <class T>
    struct SetOperationTraits
      : public ABiStar<Digraph, LengthMap, SetOperationTraitsTraits<T> > {
      typedef ABiStar<Digraph, LengthMap, SetOperationTraitsTraits<T> >
      Create;
    };

    ///@}

  protected:

    ABiStar() {}

  public:

    ///Constructor.

    ///Constructor.
    ///\param g The digraph the algorithm runs on.
    ///\param length The length map used by the algorithm.
    ///\param ec Pointer to the heuristic the algorithm uses.
    ABiStar(const Digraph& g, const LengthMap& length, const EstimateCalculator *ec = NULL) :
      G(&g), _length(&length), est_calc(ec),
      _pred(NULL), _rev_pred(NULL),
      local_pred(false), local_rev_pred(false),
      _dist(NULL), _rev_dist(NULL), 
      local_dist(false), local_rev_dist(false),
      _current_dist(NULL), _current_rev_dist(NULL),
      local_current_dist(false),local_current_rev_dist(false),
      _processed(NULL), _rev_processed(NULL),
      local_processed(false), local_rev_processed(false),
      _heap_cross_ref(NULL), _rev_heap_cross_ref(NULL), 
      local_heap_cross_ref(false), local_rev_heap_cross_ref(false),
      _heap(NULL), _rev_heap(NULL),
      local_heap(false), local_rev_heap(false),
      reverse_is_next(false), bi_path_found(false), 
      bi_dist_found(false)
    { 
      shortest_dist = OperationTraits::zero();
      con_node_id = -1;
      laststar = OperationTraits::zero();
      lastrevstar = OperationTraits::zero();
      
      forwardscannedcounter = 0;
      reversescannedcounter = 0;
      forwardreachedcounter = 0;
      reversereachedcounter = 0;
      forwardinsertedcounter = 0;
      reverseinsertedcounter = 0;
      forwarddecreasedcounter = 0;
      reversedecreasedcounter = 0;
      forwardimportantcounter = 0;
      reverseimportantcounter = 0;
      forwardrejectedcounter = 0;
      reverserejectedcounter = 0;
    }

    ///Destructor.
    ~ABiStar()
    {
      if(local_pred) delete _pred;
      if(local_rev_pred) delete _rev_pred;
      if(local_dist) delete _dist;
      if(local_rev_dist) delete _rev_dist;
      if(local_current_dist) delete _current_dist;
      if(local_current_rev_dist) delete _current_rev_dist;
      if(local_processed) delete _processed;
      if(local_rev_processed) delete _rev_processed;
      if(local_heap_cross_ref) delete _heap_cross_ref;
      if(local_rev_heap_cross_ref) delete _rev_heap_cross_ref;
      if(local_heap) delete _heap;
      if(local_rev_heap) delete _rev_heap;
    }

    ///Sets the length map.

    ///Sets the length map.
    ///\return <tt> (*this) </tt>
    ABiStar &lengthMap(const LengthMap &m)
    {
      _length = &m;
      return *this;
    }

    ///Sets the map that stores the predecessor arcs.

    ///Sets the map that stores the predecessor arcs.
    ///If you don't use this function before calling \ref run(Node) "run()"
    ///or \ref init(), an instance will be allocated automatically.
    ///The destructor deallocates this automatically allocated map,
    ///of course.
    ///\return <tt> (*this) </tt>
    ABiStar &predMap(PredMap &m)
    {
      if(local_pred) {
        delete _pred;
        local_pred=false;
      }
      _pred = &m;
      return *this;
    }

    ///Sets the map that stores the reverse predecessor arcs.

    ///Sets the map that stores the reverse predecessor arcs.
    ///If you don't use this function before calling \ref run(Node,Node) "run()"
    ///or \ref init(), an instance will be allocated automatically.
    ///The destructor deallocates this automatically allocated map,
    ///of course.
    ///\return <tt> (*this) </tt>
    ABiStar &revPredMap(PredMap &m)
    {
      if(local_rev_pred) {
        delete _rev_pred;
        local_rev_pred=false;
      }
      _rev_pred = &m;
      return *this;
    }
    
    ///Sets the map that indicates which nodes are processed.

    ///Sets the map that indicates which nodes are processed.
    ///If you don't use this function before calling \ref run(Node) "run()"
    ///or \ref init(), an instance will be allocated automatically.
    ///The destructor deallocates this automatically allocated map,
    ///of course.
    ///\return <tt> (*this) </tt>
    ABiStar &processedMap(ProcessedMap &m)
    {
      if(local_processed) {
        delete _processed;
        local_processed=false;
      }
      _processed = &m;
      return *this;
    }
    
    ///Sets the map that indicates which nodes are reversely processed.

    ///Sets the map that indicates which nodes are reversely processed.
    ///If you don't use this function before calling \ref run(Node,Node) "run()"
    ///or \ref init(), an instance will be allocated automatically.
    ///The destructor deallocates this automatically allocated map,
    ///of course.
    ///\return <tt> (*this) </tt>
    ABiStar &revProcessedMap(ProcessedMap &m)
    {
      if(local_rev_processed) {
        delete _rev_processed;
        local_rev_processed=false;
      }
      _rev_processed = &m;
      return *this;
    }

    ///Sets the map that stores the distances of the nodes.

    ///Sets the map that stores the distances of the nodes calculated by the
    ///algorithm.
    ///If you don't use this function before calling \ref run(Node) "run()"
    ///or \ref init(), an instance will be allocated automatically.
    ///The destructor deallocates this automatically allocated map,
    ///of course.
    ///\return <tt> (*this) </tt>
    ABiStar &distMap(DistMap &m)
    {
      if(local_dist) {
        delete _dist;
        local_dist=false;
      }
      _dist = &m;
      return *this;
    }
    
    ///Sets the map that stores the reverse distances of the nodes.

    ///Sets the map that stores the reverse distances of the nodes calculated by the
    ///algorithm.
    ///If you don't use this function before calling \ref run(Node,Node) "run()"
    ///or \ref init(), an instance will be allocated automatically.
    ///The destructor deallocates this automatically allocated map,
    ///of course.
    ///\return <tt> (*this) </tt>
    ABiStar &revDistMap(DistMap &m)
    {
      if(local_rev_dist) {
        delete _rev_dist;
        local_rev_dist=false;
      }
      _rev_dist = &m;
      return *this;
    }

    ///Sets the map that stores the currently known distances of the nodes.

    ///Sets the map that stores the currently known distances of the nodes
    ///calculated by the algorithm.
    ///If you don't use this function before calling \ref run(Node) "run()"
    ///or \ref init(), an instance will be allocated automatically.
    ///The destructor deallocates this automatically allocated map,
    ///of course.
    ///\return <tt> (*this) </tt>
    ABiStar &currentDistMap(DistMap &m)
    {
      if(local_dist) {
        delete _current_dist;
        local_current_dist=false;
      }
      _current_dist = &m;
      return *this;
    }
    
    ///Sets the map that stores the currently known reverse distances of the nodes.

    ///Sets the map that stores the currently known reverse distances of the nodes
    ///calculated by the algorithm.
    ///If you don't use this function before calling \ref run(Node) "run()"
    ///or \ref init(), an instance will be allocated automatically.
    ///The destructor deallocates this automatically allocated map,
    ///of course.
    ///\return <tt> (*this) </tt>
    ABiStar &currentRevDistMap(DistMap &m)
    {
      if(local_rev_dist) {
        delete _current_rev_dist;
        local_current_rev_dist=false;
      }
      _current_rev_dist = &m;
      return *this;
    }
    
    ///Sets the heap and the cross reference used by algorithm.

    ///Sets the heap and the cross reference used by algorithm.
    ///If you don't use this function before calling \ref run(Node) "run()"
    ///or \ref init(), heap and cross reference instances will be
    ///allocated automatically.
    ///The destructor deallocates these automatically allocated objects,
    ///of course.
    ///\return <tt> (*this) </tt>
    ABiStar &heap(Heap& hp, HeapCrossRef &cr)
    {
      if(local_heap_cross_ref) {
        delete _heap_cross_ref;
        local_heap_cross_ref=false;
      }
      _heap_cross_ref = &cr;
      if(local_heap) {
        delete _heap;
        local_heap=false;
      }
      _heap = &hp;
      return *this;
    }
    
    ///Sets the reverse heap and the reverse cross reference used by algorithm.

    ///Sets the reverse heap and the reverse cross reference used by algorithm.
    ///If you don't use this function before calling \ref run(Node,Node) "run()"
    ///or \ref init(), reverse heap and reverse cross reference instances will be
    ///allocated automatically.
    ///The destructor deallocates these automatically allocated objects,
    ///of course.
    ///\return <tt> (*this) </tt>
    ABiStar &revHeap(Heap& hp, HeapCrossRef &cr)
    {
      if(local_rev_heap_cross_ref) {
        delete _rev_heap_cross_ref;
        local_rev_heap_cross_ref=false;
      }
      _rev_heap_cross_ref = &cr;
      if(local_rev_heap) {
        delete _rev_heap;
        local_rev_heap=false;
      }
      _rev_heap = &hp;
      return *this;
    }

  private:

    void finalizeNodeData(Node v,Value dst)
    {
      _processed->set(v,true);
      _dist->set(v, dst);
    }
    
    void finalizeRevNodeData(Node v,Value dst)
    {
      _rev_processed->set(v,true);
      _rev_dist->set(v, dst);
    }

  public:

    ///\name Execution Control
    ///The simplest way to execute the %ABiStar algorithm is to use
    ///one of the member functions called \ref run(Node) "run()".\n
    ///If you need better control on the execution, you have to call
    ///\ref init() first, then you can add several source nodes with
    ///\ref addSource(), and several target nodes with \ref addTarget().
    ///Finally the actual path computation can be
    ///performed with one of the \ref start() functions.

    ///@{

    ///\brief Initializes the internal data structures.
    ///
    ///Initializes the internal data structures.
    //TODO: maybe separate reverse initialization from forward?
    void init()
    {
      create_maps();
      _heap->clear();
      _rev_heap->clear();
      for ( NodeIt u(*G) ; u!=INVALID ; ++u ) {
        _pred->set(u,INVALID);
        _rev_pred->set(u,INVALID);
        _processed->set(u,false);
        _rev_processed->set(u,false);
        _heap_cross_ref->set(u,Heap::PRE_HEAP);
        _rev_heap_cross_ref->set(u,Heap::PRE_HEAP);
      }
    }

    ///\brief Sets the pointer to the heuristic.
    
    ///Sets the pointer to the heuristic.
    ///The parameter is a pointer to an instance
    ///of the heuristic class.
    //TODO: maybe another for reverse??
    ABiStar &setEstimateCalculator(const EstimateCalculator &ec)
    {
        est_calc = &ec;
        return *this;
    }
    
    ///Adds a new source node.

    ///Adds a new source node to the priority heap.
    ///The optional second parameter is the initial distance of the node.
    ///
    ///The function checks if the node has already been added to the heap and
    ///it is pushed to the heap only if either it was not in the heap
    ///or the shortest path found till then is shorter than \c dst.
    void addSource(Node s,Value dst=OperationTraits::zero())
    {
      if(_heap->state(s) != Heap::IN_HEAP) {
        _heap->push(s,dst);
      } else if(OperationTraits::less((*_heap)[s], dst)) {
        _heap->set(s,dst);
        _pred->set(s,INVALID);
      }
    }
    
    ///Adds a new source node.

    ///Adds a new source node to the priority heap.
    ///The second parameter is the target node, it is needed for the heuristic.
    ///The optional third parameter is the initial distance of the node.
    ///
    ///The function checks if the node has already been added to the heap and
    ///it is pushed to the heap only if either it was not in the heap 
    ///or the shortest path found till then is shorter than \c dst.
    void addSource(Node s,Node t,Value dst=OperationTraits::zero())
    {
      if(_heap->state(s) != Heap::IN_HEAP) {
        Value starvalue = OperationTraits::plus(dst,(*est_calc)(s,t));
        _heap->push(s,starvalue);
        _current_dist->set(s,dst);
      } else if(OperationTraits::less((*_current_dist)[s], dst)) {
        Value starvalue = OperationTraits::plus(dst,(*est_calc)(s,t));
        _heap->set(s,starvalue);
        _current_dist->set(s,dst);
        _pred->set(s,INVALID);
      }
    }
    
    ///Adds a new target node.

    ///Adds a new target node to the reverse priority heap.
    ///The target is used to run bidirectional astar
    ///The optional second parameter is the initial distance of the node.
    ///
    ///The function checks if the node has already been added to the reverse heap and
    ///it is pushed to the reverse heap only if either it was not in the reverse heap
    ///or the shortest path found till then is shorter than \c dst.
    void addTarget(Node s,Node t,Value dst=OperationTraits::zero())
    {
      if(_heap->state(t) == Heap::IN_HEAP) return;
      if(_rev_heap->state(t) != Heap::IN_HEAP) {
        Value starvalue = OperationTraits::plus(dst,(*est_calc)(s,t));
        _rev_heap->push(t,starvalue);
        _current_rev_dist->set(t,dst);
      } else if(OperationTraits::less((*_current_rev_dist)[t], dst)) {
        Value starvalue = OperationTraits::plus(dst,(*est_calc)(s,t));
        _rev_heap->set(t,starvalue);
        _current_rev_dist->set(t,dst);
        _rev_pred->set(t,INVALID);
      }
    }

    ///Processes the next node in the priority heap

    ///Processes the next node in the priority heap.
    ///
    ///\return The processed node.
    ///
    ///\warning The priority heap must not be empty.
    Node processNextNode()
    {
      Node v=_heap->top();
      Value oldvalue=_heap->prio();
      _heap->pop();
      finalizeNodeData(v,oldvalue);

      for(OutArcIt e(*G,v); e!=INVALID; ++e) {
        Node w=G->target(e);
        switch(_heap->state(w)) {
        case Heap::PRE_HEAP:
          _heap->push(w,OperationTraits::plus(oldvalue, (*_length)[e]));
          _pred->set(w,e);
          break;
        case Heap::IN_HEAP:
          {
            Value newvalue = OperationTraits::plus(oldvalue, (*_length)[e]);
            if ( OperationTraits::less(newvalue, (*_heap)[w]) ) {
              _heap->decrease(w, newvalue);
              _pred->set(w,e);
            }
          }
          break;
        case Heap::POST_HEAP:
          break;
        }
      }
      return v;
    }

  private:
    void chooseDirection()
    {
      reverse_is_next = forwardinsertedcounter + 2*forwardscannedcounter + forwarddecreasedcounter > reverseinsertedcounter + 2*reversescannedcounter + reversedecreasedcounter;
    }
    
  public:
    ///Processes the next node in the priority heaps bidirectionally

    ///Processes the next node in the priority heaps bidirectionally.
    ///
    ///\return The processed node.
    ///
    ///\warning The two priority heaps must not be empty.
    Node processNextNodeBi(Node s,Node t)
    {
      Node v;
      chooseDirection();
      if (!reverse_is_next)
      {
        v=_heap->top();
        Value oldvalue = (*_current_dist)[v];
        Value oldstar = _heap->prio();
        _heap->pop();
        
        finalizeNodeData(v,oldvalue);
        ++forwardscannedcounter;
          
        if (revProcessed(v))
        {
          //bi_path_found = true; //it became redundant with bi_dist_found
          laststar = oldstar;
          ++forwardimportantcounter;
          Value cumulative = OperationTraits::plus(oldvalue, currentRevDist(v));//dist[v] ought to be enough
          if (!bi_dist_found)
          {
            bi_dist_found = true;
            shortest_dist = cumulative;
            con_node_id = G->id(v);
          }
          else if ( OperationTraits::less(cumulative, shortest_dist) )
          {
            shortest_dist = cumulative;
            con_node_id = G->id(v);
          }
          return v;
        }  
          
        if (bi_dist_found)
        {
         if (!OperationTraits::less(oldstar,shortest_dist)) { ++forwardrejectedcounter; return v; }
         if (_rev_heap->state(v) == Heap::IN_HEAP)
         { 
           if (!OperationTraits::less(OperationTraits::plus(OperationTraits::plus(oldvalue,lastrevstar),(*_current_rev_dist)[v]),OperationTraits::plus(shortest_dist,(*_rev_heap)[v]))) { ++forwardrejectedcounter; return v; }
           //if ((oldvalue + lastrevstar + (*_current_rev_dist)[v]) >= shortest_dist + (*_rev_heap)[v]) return v;
         } else if ( !OperationTraits::less(OperationTraits::plus(oldvalue,lastrevstar), OperationTraits::plus(shortest_dist,(*est_calc)(s,v))) ) { ++forwardrejectedcounter; return v; } //only PRE_HEAP is possible
         //else if ( (oldvalue + lastrevstar) >= shortest_dist + (*est_calc)(s,v) ) return v;
        }
        //if (bi_dist_found && (oldvalue + lastrevstar) >= shortest_dist + (*est_calc)(s,v)) return v;
        
        laststar = oldstar;
        
        for(OutArcIt e(*G,v); e!=INVALID; ++e) {
          Node w=G->target(e);
          ++forwardreachedcounter;
          switch(_heap->state(w)) {
          case Heap::PRE_HEAP:
            {
              ++forwardinsertedcounter;
              Value newvalue = OperationTraits::plus(oldvalue, (*_length)[e]);
              _heap->push(w,OperationTraits::plus(newvalue, (*est_calc)(w,t)));
              _current_dist->set(w,newvalue);
              _pred->set(w,e);
              break;
            }
          case Heap::IN_HEAP:
            {
              Value newvalue = OperationTraits::plus(oldvalue, (*_length)[e]);
              if ( OperationTraits::less(newvalue, (*_current_dist)[w]) ) {
                ++forwarddecreasedcounter;
                _heap->decrease(w,OperationTraits::plus(newvalue, (*est_calc)(w,t)));
                _current_dist->set(w, newvalue);
                _pred->set(w,e);
              } else continue;
            }
            break;
          case Heap::POST_HEAP:
            continue;
            break;
          }
        }
      } else {
        v=_rev_heap->top();
        Value oldvalue = (*_current_rev_dist)[v];
        Value oldstar = _rev_heap->prio();
        _rev_heap->pop();
        
        
        finalizeRevNodeData(v,oldvalue);
        ++reversescannedcounter;
          
        if (processed(v))
        {
          //bi_path_found = true; //it became redundant with bi_dist_found
          lastrevstar = oldstar;
          ++reverseimportantcounter;
          Value cumulative = OperationTraits::plus(oldvalue, currentDist(v));//dist[v] ought to be enough
          if (!bi_dist_found)
          {
            bi_dist_found = true;
            shortest_dist = cumulative;
            con_node_id = G->id(v);
          }
          else if ( OperationTraits::less(cumulative, shortest_dist) )
          {
            shortest_dist = cumulative;
            con_node_id = G->id(v);
          }
          return v;
        }
          
        if (bi_dist_found)
        {
         if (!OperationTraits::less(oldstar,shortest_dist)) { ++reverserejectedcounter; return v; }
         if (_heap->state(v) == Heap::IN_HEAP)
         { 
           if (!OperationTraits::less(OperationTraits::plus(OperationTraits::plus(oldvalue,laststar),(*_current_dist)[v]),OperationTraits::plus(shortest_dist,(*_heap)[v]))) { ++reverserejectedcounter; return v; }
           //if (!((oldvalue + laststar + (*_current_dist)[v]) < shortest_dist + (*_heap)[v])) return v;
         } else if (!OperationTraits::less(OperationTraits::plus(oldvalue,laststar), OperationTraits::plus(shortest_dist,(*est_calc)(v,t)))) { ++reverserejectedcounter; return v; } // only PRE_HEAP is possible
         // else if ( !((oldvalue + laststar) < shortest_dist + (*est_calc)(v,t))) return v;
        }
        //if (bi_dist_found && (oldvalue + laststar) >= shortest_dist + (*est_calc)(v,t)) return v;
        
        lastrevstar = oldstar;

        for(InArcIt e(*G,v); e!=INVALID; ++e) {
          Node w=G->source(e);
          ++reversereachedcounter;
          switch(_rev_heap->state(w)) {
          case Heap::PRE_HEAP:
            {
              ++reverseinsertedcounter;
              Value newvalue = OperationTraits::plus(oldvalue, (*_length)[e]);
              _rev_heap->push(w,OperationTraits::plus(newvalue, (*est_calc)(s,w)));
              _current_rev_dist->set(w,newvalue);
              _rev_pred->set(w,e);
              break;
            }
          case Heap::IN_HEAP:
            {
              Value newvalue = OperationTraits::plus(oldvalue, (*_length)[e]);
              if ( OperationTraits::less(newvalue, (*_current_rev_dist)[w]) ) {
                ++reversedecreasedcounter;
                _rev_heap->decrease(w,OperationTraits::plus(newvalue, (*est_calc)(s,w)));
                _current_rev_dist->set(w,newvalue);
                _rev_pred->set(w,e);
              } else continue;
            }
            break;
          case Heap::POST_HEAP:
            continue;
            break;
          }
        }
      }
      return v;
    }
    
    ///The next node to be processed.

    ///Returns the next node to be processed or \c INVALID if the
    ///priority heap is empty.
    Node nextNode() const
    {
      return !_heap->empty()?_heap->top():INVALID;
    }
    
    ///The next node to be processed reversely.

    ///Returns the next node to be processed reversely or \c INVALID if the
    ///reverse priority heap is empty.
    Node nextRevNode() const
    {
      return !_rev_heap->empty()?_rev_heap->top():INVALID;
    }

    ///Returns \c false if there are nodes to be processed.

    ///Returns \c false if there are nodes to be processed
    ///in the priority heap.
    bool emptyQueue() const { return _heap->empty(); }
    
    ///Returns \c false if there are nodes to be processed reversely.

    ///Returns \c false if there are nodes to be processed
    ///in the reverse priority heap.
    bool emptyRevQueue() const { return _rev_heap->empty(); }

    ///Returns the number of the nodes to be processed.

    ///Returns the number of the nodes to be processed
    ///in the priority heap.
    int queueSize() const { return _heap->size(); }
    
    ///Returns the number of the nodes to be processed reversely.

    ///Returns the number of the nodes to be processed
    ///in the reverse priority heap.
    int revQueueSize() const { return _rev_heap->size(); }

    ///Executes the algorithm.

    ///Executes the algorithm.
    ///
    ///This method runs the %ABiStar algorithm from the root node(s)
    ///in order to compute the shortest path to each node.
    ///Due to lack of target node, it runs really the original
    ///%Dijkstra algorithm.
    ///
    ///The algorithm computes
    ///- the shortest path tree (forest),
    ///- the distance of each node from the root(s).
    ///
    ///\pre init() must be called and at least one root node should be
    ///added with addSource() before using this function.
    ///
    ///\note <tt>d.start()</tt> is just a shortcut of the following code.
    ///\code
    ///  while ( !d.emptyQueue() ) {
    ///    d.processNextNode();
    ///  }
    ///\endcode
    void start()
    {
      while ( !emptyQueue() ) processNextNode();
    }

    ///Executes the algorithm until one of the heaps is empty.

    ///Executes the algorithm until one of the heaps is empty.
    ///
    ///This method runs the %ABiStar algorithm from the root node(s)
    ///in order to compute the shortest path to \c t. Technically, 
    ///it runs the %AStar algorithm from both the root node(s)
    ///and the target node \c t.
    ///
    ///The algorithm computes
    ///- the shortest path to \c t,
    ///- the distance of \c t from the root(s).
    ///
    ///The root node parameter is needed for the reverse heuristic.
    ///\pre init() must be called and at least one root node should be
    ///added with addSource() before using this function. If there is any
    ///target node added with addTarget(), the algorithm will run bidirectionally.
    void start(Node s,Node t)
    {
      forwardscannedcounter = 0;
      reversescannedcounter = 0;
      forwardreachedcounter = 0;
      reversereachedcounter = 0;
      forwardinsertedcounter = 0;
      reverseinsertedcounter = 0;
      forwarddecreasedcounter = 0;
      reversedecreasedcounter = 0;
      forwardimportantcounter = 0;
      reverseimportantcounter = 0;
      forwardrejectedcounter = 0;
      reverserejectedcounter = 0;
      bi_path_found = false;
      bi_dist_found = false;
      laststar = OperationTraits::zero();
      lastrevstar = OperationTraits::zero();
      
      shortest_dist = OperationTraits::zero();
      con_node_id = -1;
      
      if (est_calc == NULL || _rev_heap->empty())
      {
        while ( !_heap->empty() && _heap->top()!=t ) processNextNode();
        if ( !_heap->empty() ) {
          finalizeNodeData(_heap->top(),_heap->prio());
          _heap->pop();
        }
      } else {
        while (!_heap->empty() && !_rev_heap->empty()) processNextNodeBi(s,t);
        
        //if (bi_path_found)
        if (bi_dist_found)
        { 
          const Node* nodep = new Node(Digraph::nodeFromId(con_node_id));
          const Node* temp = nodep;
          Value finalvalue = currentDist(*nodep);
          if (!processed(*nodep)) finalizeNodeData(*nodep,finalvalue);
          if (G->id(*nodep) == G->id(t)) finalizeNodeData(t,finalvalue);
          else while (G->id(*nodep) != G->id(t))
          {
            Arc e = (*_rev_pred)[*nodep];
            Node v = G->target(e);
            _pred->set(v,e);
            finalvalue = OperationTraits::plus(finalvalue, (*_length)[e]);
            finalizeNodeData(v,finalvalue);
            nodep = &v;            
          }
          delete temp;
        }
      }
    }

    ///Executes the algorithm until a condition is met.

    ///Executes the algorithm until a condition is met.
    ///
    ///This method runs the %ABiStar algorithm from the root node(s) in
    ///order to compute the shortest path to a node \c v with
    /// <tt>nm[v]</tt> true, if such a node can be found.
    ///
    ///\param nm A \c bool (or convertible) node map. The algorithm
    ///will stop when it reaches a node \c v with <tt>nm[v]</tt> true.
    ///
    ///\return The reached node \c v with <tt>nm[v]</tt> true or
    ///\c INVALID if no such node was found.
    ///
    ///\pre init() must be called and at least one root node should be
    ///added with addSource() before using this function.
    //TODO: do we need this bidirectionally?
    template<class NodeBoolMap>
    Node start(const NodeBoolMap &nm)
    {
      while ( !_heap->empty() && !nm[_heap->top()] ) processNextNode();
      if ( _heap->empty() ) return INVALID;
      finalizeNodeData(_heap->top(),_heap->prio());
      return _heap->top();
    }

    ///Runs the algorithm from the given source node.

    ///This method runs the %ABiStar algorithm from node \c s
    ///in order to compute the shortest path to each node.
    ///Due to the lack of target node, it runs the 
    ///original %Dijkstra algorithm.
    ///
    ///The algorithm computes
    ///- the shortest path tree,
    ///- the distance of each node from the root.
    ///
    ///\note <tt>d.run(s)</tt> is just a shortcut of the following code.
    ///\code
    ///  d.init();
    ///  d.addSource(s);
    ///  d.start();
    ///\endcode
    void run(Node s) {
      init();
      addSource(s);
      start();
    }

    ///Finds the shortest path between \c s and \c t.

    ///This method runs the %ABiStar algorithm from node \c s
    ///in order to compute the shortest path to node \c t
    ///(it stops searching when \c t is processed).
    ///If no heuristic is available, it runs the original
    ///%Dijkstra algorithm.
    ///
    ///\return \c true if \c t is reachable form \c s.
    ///
    ///\note Apart from the return value, <tt>d.run(s,t)</tt> is just a
    ///shortcut of the following code.
    ///\code
    ///  d.init();
    ///  d.addSource(s,t);
    ///  d.addTarget(s,t);
    ///  d.start(t);
    ///\endcode
    bool run(Node s,Node t) {
      init();
      if (est_calc == NULL) addSource(s);
      else { addSource(s,t); addTarget(s,t); }
      start(s,t);
      return (*_heap_cross_ref)[t] == Heap::POST_HEAP;
    }

    ///@}

    ///\name Query Functions
    ///The results of the %ABiStar algorithm can be obtained using these
    ///functions.\n
    ///Either \ref run(Node) "run()" or \ref init() should be called
    ///before using them.

    ///@{

    ///The shortest path to the given node.

    ///Returns the shortest path to the given node from the root(s).
    ///
    ///\warning \c t should be reached from the root(s).
    ///
    ///\pre Either \ref run(Node) "run()" or \ref init()
    ///must be called before using this function.
    Path path(Node t) const { return Path(*G, *_pred, t); }
    
    ///The shortest path from the given node.

    ///Returns the shortest path from the given node to the target(t).
    ///
    ///\warning \c t should be reversely reached from the target(s).
    ///
    ///\pre Either \ref run(Node,Node) "run()" or \ref init()
    ///must be called before using this function.
    //?? does this work?? TODO: check the Path class.. I think this would work only on the reverse graph
    Path revPath(Node t) const { return Path(*G, *_rev_pred, t); }

    ///The distance of the given node from the root(s).

    ///Returns the distance of the given node from the root(s).
    ///
    ///\warning If node \c v is not reached from the root(s), then
    ///the return value of this function is undefined.
    ///
    ///\pre Either \ref run(Node) "run()" or \ref init()
    ///must be called before using this function.
    Value dist(Node v) const { return (*_dist)[v]; }
    
    ///The distance of the target(t) from the given node.

    ///Returns the distance of the target(t) from the given.
    ///
    ///\warning If node \c v is not reversely reached from the target(t), then
    ///the return value of this function is undefined.
    ///
    ///\pre Either \ref run(Node,Node) "run()" or \ref init()
    ///must be called before using this function.
    Value revDist(Node v) const { return (*_rev_dist)[v]; }

    ///\brief Returns the 'previous arc' of the shortest path tree for
    ///the given node.
    ///
    ///This function returns the 'previous arc' of the shortest path
    ///tree for the node \c v, i.e. it returns the last arc of a
    ///shortest path from a root to \c v. It is \c INVALID if \c v
    ///is not reached from the root(s) or if \c v is a root.
    ///
    ///The shortest path tree used here is equal to the shortest path
    ///tree used in \ref predNode() and \ref predMap().
    ///
    ///\pre Either \ref run(Node) "run()" or \ref init()
    ///must be called before using this function.
    Arc predArc(Node v) const { return (*_pred)[v]; }
    
    ///\brief Returns the 'previous arc' of the reverse shortest path tree for
    ///the given node.
    ///
    ///This function returns the 'previous arc' of the reverse shortest path
    ///tree for the node \c v, i.e. it returns the last arc of a
    ///shortest path from a target to \c v in the reverse graph. 
    ///It is \c INVALID if \c v is not reversely reached from the target(s)
    ///or if \c v is a target.
    ///
    ///The shortest path tree used here is equal to the shortest path
    ///tree used in \ref revPredNode() and \ref revPredMap().
    ///
    ///\pre Either \ref run(Node,Node) "run()" or \ref init()
    ///must be called before using this function.
    Arc revPredArc(Node v) const { return (*_rev_pred)[v]; }

    ///\brief Returns the 'previous node' of the shortest path tree for
    ///the given node.
    ///
    ///This function returns the 'previous node' of the shortest path
    ///tree for the node \c v, i.e. it returns the last but one node
    ///of a shortest path from a root to \c v. It is \c INVALID
    ///if \c v is not reached from the root(s) or if \c v is a root.
    ///
    ///The shortest path tree used here is equal to the shortest path
    ///tree used in \ref predArc() and \ref predMap().
    ///
    ///\pre Either \ref run(Node) "run()" or \ref init()
    ///must be called before using this function.
    Node predNode(Node v) const { return (*_pred)[v]==INVALID ? INVALID:
                                  G->source((*_pred)[v]); }

    ///\brief Returns the 'previous node' of the reverse shortest path tree
    ///for the given node.
    ///
    ///This function returns the 'previous node' of the reverse shortest path
    ///tree for the node \c v, i.e. it returns the last but one node
    ///of a shortest path from a target to \c v in the reverse graph.
    //It is \c INVALID if \c v is not reversely reached from the target(t)
    ///or if \c v is a root.
    ///
    ///The shortest path tree used here is equal to the shortest path
    ///tree used in \ref revPredArc() and \ref revPredMap().
    ///
    ///\pre Either \ref run(Node,Node) "run()" or \ref init()
    ///must be called before using this function.
    Node revPredNode(Node v) const { return (*_rev_pred)[v]==INVALID ? INVALID:
                                  G->target((*_rev_pred)[v]); }
    
    ///\brief Returns a const reference to the node map that stores the
    ///distances of the nodes.
    ///
    ///Returns a const reference to the node map that stores the distances
    ///of the nodes calculated by the algorithm.
    ///
    ///\pre Either \ref run(Node) "run()" or \ref init()
    ///must be called before using this function.
    const DistMap &distMap() const { return *_dist;}
    
    ///\brief Returns a const reference to the node map that stores the
    ///reverse distances of the nodes.
    ///
    ///Returns a const reference to the node map that stores the 
    ///reverse distances of the nodes calculated by the algorithm.
    ///
    ///\pre Either \ref run(Node,Node) "run()" or \ref init()
    ///must be called before using this function.
    const DistMap &revDistMap() const { return *_rev_dist;}
    
    ///\brief Returns a const reference to the node map that stores the
    ///currently known distances of the nodes.
    ///
    ///Returns a const reference to the node map that stores the currently
    ///known distances of the nodes calculated by the algorithm.
    ///
    ///\pre Either \ref run(Node) "run()" or \ref init()
    ///must be called before using this function.
    const DistMap &currentDistMap() const { return *_current_dist;}
    
    //update
    const DistMap &currentRevDistMap() const { return *_current_rev_dist;}

    ///\brief Returns a const reference to the node map that stores the
    ///predecessor arcs.
    ///
    ///Returns a const reference to the node map that stores the predecessor
    ///arcs, which form the shortest path tree (forest).
    ///
    ///\pre Either \ref run(Node) "run()" or \ref init()
    ///must be called before using this function.
    const PredMap &predMap() const { return *_pred;}
    
    ///\brief Returns a const reference to the node map that stores the
    ///reverse predecessor arcs.
    ///
    ///Returns a const reference to the node map that stores the 
    ///reverse predecessor arcs, which form the 
    ///reverse shortest path tree (forest).
    ///
    ///\pre Either \ref run(Node,Node) "run()" or \ref init()
    ///must be called before using this function.
    const PredMap &revPredMap() const { return *_rev_pred;}

    ///Checks if the given node is reached from the root(s).

    ///Returns \c true if \c v is reached from the root(s).
    ///
    ///\pre Either \ref run(Node) "run()" or \ref init()
    ///must be called before using this function.
    bool reached(Node v) const { return (*_heap_cross_ref)[v] !=
                                        Heap::PRE_HEAP; }

    ///Checks if the given node is reversely reached from the target(t).

    ///Returns \c true if \c v is reversely reached from the target(t).
    ///
    ///\pre Either \ref run(Node,Node) "run()" or \ref init()
    ///must be called before using this function.
    bool revReached(Node v) const { return (*_rev_heap_cross_ref)[v] !=
                                        Heap::PRE_HEAP; }
                                        
    ///Checks if a node is processed.

    ///Returns \c true if \c v is processed, i.e. the shortest
    ///path to \c v has already found.
    ///
    ///\pre Either \ref run(Node) "run()" or \ref init()
    ///must be called before using this function.
    bool processed(Node v) const { return (*_heap_cross_ref)[v] ==
                                          Heap::POST_HEAP; }

	///Checks if a node is reversely processed.

    ///Returns \c true if \c v is reversely processed, i.e. the shortest
    ///path from \c v to target(t) has already found.
    ///
    ///\pre Either \ref run(Node,Node) "run()" or \ref init()
    ///must be called before using this function.
    bool revProcessed(Node v) const { return (*_rev_heap_cross_ref)[v] ==
                                          Heap::POST_HEAP; }
                                          
    ///The current distance of the given node from the root(s).

    ///Returns the current distance of the given node from the root(s).
    ///It may be decreased in the following processes.
    ///
    ///\pre Either \ref run(Node) "run()" or \ref init()
    ///must be called before using this function and
    ///node \c v must be reached but not necessarily processed.
    Value currentDist(Node v) const {
      return (*_current_dist)[v];
    }

    ///The current reverse distance of the given node from the target(s).

    ///Returns the current reverse distance of the given node from the target(s).
    ///It may be decreased in the following processes.
    ///
    ///\pre Either \ref run(Node,Node) "run()" or \ref init()
    ///must be called before using this function and
    ///node \c v must be reversely reached but not necessarily reversely processed.
    Value currentRevDist(Node v) const {
      return (*_current_rev_dist)[v];
    }
    
    //get functions for testing
    long int getforwardscannedcounter() const { return forwardscannedcounter; }
    long int getreversescannedcounter() const { return reversescannedcounter; }
    long int getforwardreachedcounter() const { return forwardreachedcounter; }
    long int getreversereachedcounter() const { return reversereachedcounter; }
    long int getforwardinsertedcounter() const { return forwardinsertedcounter; }
    long int getreverseinsertedcounter() const { return reverseinsertedcounter; }
    long int getforwarddecreasedcounter() const { return forwarddecreasedcounter; }
    long int getreversedecreasedcounter() const { return reversedecreasedcounter; }
    long int getforwardimportantcounter() const { return forwardimportantcounter; }
    long int getreverseimportantcounter() const { return reverseimportantcounter; }
    long int getforwardrejectedcounter() const { return forwardrejectedcounter; }
    long int getreverserejectedcounter() const { return reverserejectedcounter; }
    
    long int getscannedcounter() const { return forwardscannedcounter + reversescannedcounter; }
    long int getreachedcounter() const { return forwardreachedcounter + reversereachedcounter; }
    long int getinsertedcounter() const { return forwardinsertedcounter + reverseinsertedcounter; }
    long int getdecreasedcounter() const { return forwarddecreasedcounter + reversedecreasedcounter; }
    long int getimportantcounter() const { return forwardimportantcounter + reverseimportantcounter; }
    long int getrejectedcounter() const { return forwardrejectedcounter + reverserejectedcounter; }
    
    int getlength(const Node &v) const
    {
      int length = 0;
      for (Node temp(v); temp != INVALID; temp = predNode(temp)) ++length;
      return length;
    }
    
    ///@}
  };


  ///Default traits class of abistar() function.

  ///Default traits class of abistar() function.
  ///\tparam GR The type of the digraph.
  ///\tparam LEN The type of the length map.
  template<class GR, class LEN>
  struct ABiStarWizardDefaultTraits
  {
    ///The type of the digraph the algorithm runs on.
    typedef GR Digraph;
    ///The type of the map that stores the arc lengths.

    ///The type of the map that stores the arc lengths.
    ///It must conform to the \ref concepts::ReadMap "ReadMap" concept.
    typedef LEN LengthMap;
    ///The type of the arc lengths.
    typedef typename LEN::Value Value;

    /// Operation traits for ABiStar algorithm.

    /// This class defines the operations that are used in the algorithm.
    /// \see ABiStarDefaultOperationTraits
    typedef ABiStarDefaultOperationTraits<Value> OperationTraits;

    /// The cross reference type used by the heap.

    /// The cross reference type used by the heap.
    /// Usually it is \c Digraph::NodeMap<int>.
    typedef typename Digraph::template NodeMap<int> HeapCrossRef;
    ///Instantiates a \ref HeapCrossRef.

    ///This function instantiates a \ref HeapCrossRef.
    /// \param g is the digraph, to which we would like to define the
    /// HeapCrossRef.
    static HeapCrossRef *createHeapCrossRef(const Digraph &g)
    {
      return new HeapCrossRef(g);
    }

    ///The heap type used by the ABiStar algorithm.

    ///The heap type used by the ABiStar algorithm.
    ///
    ///\sa BinHeap
    ///\sa ABiStar
    typedef BinHeap<Value, typename Digraph::template NodeMap<int>,
                    std::less<Value> > Heap;

    ///Instantiates a \ref Heap.

    ///This function instantiates a \ref Heap.
    /// \param r is the HeapCrossRef which is used.
    static Heap *createHeap(HeapCrossRef& r)
    {
      return new Heap(r);
    }

    ///\brief The type of the map that stores the predecessor
    ///arcs of the shortest paths.
    ///
    ///The type of the map that stores the predecessor
    ///arcs of the shortest paths.
    ///It must conform to the \ref concepts::WriteMap "WriteMap" concept.
    typedef typename Digraph::template NodeMap<typename Digraph::Arc> PredMap;
    ///Instantiates a PredMap.

    ///This function instantiates a PredMap.
    ///\param g is the digraph, to which we would like to define the
    ///PredMap.
    static PredMap *createPredMap(const Digraph &g)
    {
      return new PredMap(g);
    }

    ///The type of the map that indicates which nodes are processed.

    ///The type of the map that indicates which nodes are processed.
    ///It must conform to the \ref concepts::WriteMap "WriteMap" concept.
    ///By default, it is a NullMap.
    typedef NullMap<typename Digraph::Node,bool> ProcessedMap;
    ///Instantiates a ProcessedMap.

    ///This function instantiates a ProcessedMap.
    ///\param g is the digraph, to which
    ///we would like to define the ProcessedMap.
#ifdef DOXYGEN
    static ProcessedMap *createProcessedMap(const Digraph &g)
#else
    static ProcessedMap *createProcessedMap(const Digraph &)
#endif
    {
      return new ProcessedMap();
    }

    ///The type of the map that stores the distances of the nodes.

    ///The type of the map that stores the distances of the nodes.
    ///It must conform to the \ref concepts::WriteMap "WriteMap" concept.
    typedef typename Digraph::template NodeMap<typename LEN::Value> DistMap;
    ///Instantiates a DistMap.

    ///This function instantiates a DistMap.
    ///\param g is the digraph, to which we would like to define
    ///the DistMap
    static DistMap *createDistMap(const Digraph &g)
    {
      return new DistMap(g);
    }

    ///The type of the shortest paths.

    ///The type of the shortest paths.
    ///It must conform to the \ref concepts::Path "Path" concept.
    typedef lemon::Path<Digraph> Path;
  };

  /// Default traits class used by ABiStarWizard

  /// Default traits class used by ABiStarWizard.
  /// \tparam GR The type of the digraph.
  /// \tparam LEN The type of the length map.
  template<typename GR, typename LEN>
  class ABiStarWizardBase : public ABiStarWizardDefaultTraits<GR,LEN>
  {
    typedef ABiStarWizardDefaultTraits<GR,LEN> Base;
  protected:
    //The type of the nodes in the digraph.
    typedef typename Base::Digraph::Node Node;

    //Pointer to the digraph the algorithm runs on.
    void *_g;
    //Pointer to the length map.
    void *_length;
    //Pointer to the map of processed nodes.
    void *_processed;
    //Pointer to the map of predecessors arcs.
    void *_pred;
    //Pointer to the map of distances.
    void *_dist;
    //Pointer to the shortest path to the target node.
    void *_path;
    //Pointer to the distance of the target node.
    void *_di;

  public:
    /// Constructor.

    /// This constructor does not require parameters, therefore it initiates
    /// all of the attributes to \c 0.
    ABiStarWizardBase() : _g(0), _length(0), _processed(0), _pred(0),
                           _dist(0), _path(0), _di(0) {}

    /// Constructor.

    /// This constructor requires two parameters,
    /// others are initiated to \c 0.
    /// \param g The digraph the algorithm runs on.
    /// \param l The length map.
    ABiStarWizardBase(const GR &g,const LEN &l) :
      _g(reinterpret_cast<void*>(const_cast<GR*>(&g))),
      _length(reinterpret_cast<void*>(const_cast<LEN*>(&l))),
      _processed(0), _pred(0), _dist(0), _path(0), _di(0) {}

  };

  /// Auxiliary class for the function-type interface of ABiStar algorithm.

  /// This auxiliary class is created to implement the
  /// \ref abistar() "function-type interface" of \ref ABiStar algorithm.
  /// It does not have own \ref run(Node) "run()" method, it uses the
  /// functions and features of the plain \ref ABiStar.
  ///
  /// This class should only be used through the \ref abistar() function,
  /// which makes it easier to use the algorithm.
  ///
  /// \tparam TR The traits class that defines various types used by the
  /// algorithm.
  template<class TR>
  class ABiStarWizard : public TR
  {
    typedef TR Base;

    typedef typename TR::Digraph Digraph;

    typedef typename Digraph::Node Node;
    typedef typename Digraph::NodeIt NodeIt;
    typedef typename Digraph::Arc Arc;
    typedef typename Digraph::OutArcIt OutArcIt;

    typedef typename TR::LengthMap LengthMap;
    typedef typename LengthMap::Value Value;
    typedef typename TR::PredMap PredMap;
    typedef typename TR::DistMap DistMap;
    typedef typename TR::ProcessedMap ProcessedMap;
    typedef typename TR::Path Path;
    typedef typename TR::Heap Heap;

  public:

    /// Constructor.
    ABiStarWizard() : TR() {}

    /// Constructor that requires parameters.

    /// Constructor that requires parameters.
    /// These parameters will be the default values for the traits class.
    /// \param g The digraph the algorithm runs on.
    /// \param l The length map.
    ABiStarWizard(const Digraph &g, const LengthMap &l) :
      TR(g,l) {}

    ///Copy constructor
    ABiStarWizard(const TR &b) : TR(b) {}

    ~ABiStarWizard() {}

    ///Runs ABiStar algorithm from the given source node.

    ///This method runs %ABiStar algorithm from the given source node
    ///in order to compute the shortest path to each node.
    void run(Node s)
    {
      ABiStar<Digraph,LengthMap,TR>
        dijk(*reinterpret_cast<const Digraph*>(Base::_g),
             *reinterpret_cast<const LengthMap*>(Base::_length));
      if (Base::_pred)
        dijk.predMap(*reinterpret_cast<PredMap*>(Base::_pred));
      if (Base::_dist)
        dijk.distMap(*reinterpret_cast<DistMap*>(Base::_dist));
      if (Base::_processed)
        dijk.processedMap(*reinterpret_cast<ProcessedMap*>(Base::_processed));
      dijk.run(s);
    }

    ///Finds the shortest path between \c s and \c t.

    ///This method runs the %ABiStar algorithm from node \c s
    ///in order to compute the shortest path to node \c t
    ///(it stops searching when \c t is processed).
    ///
    ///\return \c true if \c t is reachable form \c s.
    bool run(Node s, Node t)
    {
      ABiStar<Digraph,LengthMap,TR>
        dijk(*reinterpret_cast<const Digraph*>(Base::_g),
             *reinterpret_cast<const LengthMap*>(Base::_length));
      if (Base::_pred)
        dijk.predMap(*reinterpret_cast<PredMap*>(Base::_pred));
      if (Base::_dist)
        dijk.distMap(*reinterpret_cast<DistMap*>(Base::_dist));
      if (Base::_processed)
        dijk.processedMap(*reinterpret_cast<ProcessedMap*>(Base::_processed));
      dijk.run(s,t);
      if (Base::_path)
        *reinterpret_cast<Path*>(Base::_path) = dijk.path(t);
      if (Base::_di)
        *reinterpret_cast<Value*>(Base::_di) = dijk.dist(t);
      return dijk.reached(t);
    }

    template<class T>
    struct SetPredMapBase : public Base {
      typedef T PredMap;
      static PredMap *createPredMap(const Digraph &) { return 0; };
      SetPredMapBase(const TR &b) : TR(b) {}
    };

    ///\brief \ref named-templ-param "Named parameter" for setting
    ///the predecessor map.
    ///
    ///\ref named-templ-param "Named parameter" function for setting
    ///the map that stores the predecessor arcs of the nodes.
    template<class T>
    ABiStarWizard<SetPredMapBase<T> > predMap(const T &t)
    {
      Base::_pred=reinterpret_cast<void*>(const_cast<T*>(&t));
      return ABiStarWizard<SetPredMapBase<T> >(*this);
    }

    template<class T>
    struct SetDistMapBase : public Base {
      typedef T DistMap;
      static DistMap *createDistMap(const Digraph &) { return 0; };
      SetDistMapBase(const TR &b) : TR(b) {}
    };

    ///\brief \ref named-templ-param "Named parameter" for setting
    ///the distance map.
    ///
    ///\ref named-templ-param "Named parameter" function for setting
    ///the map that stores the distances of the nodes calculated
    ///by the algorithm.
    template<class T>
    ABiStarWizard<SetDistMapBase<T> > distMap(const T &t)
    {
      Base::_dist=reinterpret_cast<void*>(const_cast<T*>(&t));
      return ABiStarWizard<SetDistMapBase<T> >(*this);
    }

    template<class T>
    struct SetProcessedMapBase : public Base {
      typedef T ProcessedMap;
      static ProcessedMap *createProcessedMap(const Digraph &) { return 0; };
      SetProcessedMapBase(const TR &b) : TR(b) {}
    };

    ///\brief \ref named-func-param "Named parameter" for setting
    ///the processed map.
    ///
    ///\ref named-templ-param "Named parameter" function for setting
    ///the map that indicates which nodes are processed.
    template<class T>
    ABiStarWizard<SetProcessedMapBase<T> > processedMap(const T &t)
    {
      Base::_processed=reinterpret_cast<void*>(const_cast<T*>(&t));
      return ABiStarWizard<SetProcessedMapBase<T> >(*this);
    }

    template<class T>
    struct SetPathBase : public Base {
      typedef T Path;
      SetPathBase(const TR &b) : TR(b) {}
    };

    ///\brief \ref named-func-param "Named parameter"
    ///for getting the shortest path to the target node.
    ///
    ///\ref named-func-param "Named parameter"
    ///for getting the shortest path to the target node.
    template<class T>
    ABiStarWizard<SetPathBase<T> > path(const T &t)
    {
      Base::_path=reinterpret_cast<void*>(const_cast<T*>(&t));
      return ABiStarWizard<SetPathBase<T> >(*this);
    }

    ///\brief \ref named-func-param "Named parameter"
    ///for getting the distance of the target node.
    ///
    ///\ref named-func-param "Named parameter"
    ///for getting the distance of the target node.
    ABiStarWizard dist(const Value &d)
    {
      Base::_di=reinterpret_cast<void*>(const_cast<Value*>(&d));
      return *this;
    }

  };

  ///Function-type interface for ABiStar algorithm.

  /// \ingroup shortest_path
  ///Function-type interface for ABiStar algorithm.
  ///
  ///This function also has several \ref named-func-param "named parameters",
  ///they are declared as the members of class \ref ABiStarWizard.
  ///The following examples show how to use these parameters.
  ///\code
  ///  // Compute shortest path from node s to each node
  ///  abistar(g,length).predMap(preds).distMap(dists).run(s);
  ///
  ///  // Compute shortest path from s to t
  ///  bool reached = abistar(g,length).path(p).dist(d).run(s,t);
  ///\endcode
  ///\warning Don't forget to put the \ref ABiStarWizard::run(Node) "run()"
  ///to the end of the parameter list.
  ///\sa ABiStarWizard
  ///\sa ABiStar
  template<typename GR, typename LEN>
  ABiStarWizard<ABiStarWizardBase<GR,LEN> >
  abistar(const GR &digraph, const LEN &length)
  {
    return ABiStarWizard<ABiStarWizardBase<GR,LEN> >(digraph,length);
  }

} //END OF NAMESPACE LEMON

#endif
