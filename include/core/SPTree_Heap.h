/*--------------------------------------------------------------------------*/
/*----------------------------- File SPTree_Heap.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of several "classic" Shortest Path Tree algorithms to
 * solve uncapacitated single-source Min Cost Flow problems. The actual
 * algorithm can be chosen at compile time by setting a proper switch.
 * Conforms to the standard MCF interface defined in MCFClass.h.
 *
 * \version 1.80
 *
 * \date 11 - 02 - 2011
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy 1996 - 2011 by Antonio Frangioni.
 */
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef _SPTree_Heap
 #define _SPTree_Heap /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "MCFClass.h"
#include "isptree.h"
/*--------------------------------------------------------------------------*/
/*---------------------------- MACROS --------------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup SPTREE_MACROS Compile-time switches in SPTree.h
    These macros control some important details of the implementation.
    Although using macros for activating features of the implementation is
    not very C++, switching off some unused features may make the code
    more efficient in running time or memory.
    @{ */

/*------------------------------ SPT_ALGRTM --------------------------------*/

//#define SPT_ALGRTM 4
//Johannes Sommer
//const int SPT_ALGRTM = 4;
//const int LABEL_SETTING = 1;
//const int HeapCard = 2;

/**< This macro decides which SPT algorithm has to be used.
   Possible values are:

   - 0  =>  LQueue
   - 1  =>  LDeque
   - 2  =>  (currently unused)
   - 3  =>  Dijkstra
   - 4  =>  Heap

   For algorithms based on priority lists, the macro LABEL_SETTING [see
   below] can be set to 1 to say that the algorithm is of the
   "label-setting" (nodes only exit from Q once) orather than of the
   "label-correcting" (nodes may exit from Q more than once) type. */

//#if( SPT_ALGRTM <= 2 )
// #define LABEL_SETTING 0
// ///< this is a label-correcting SPT algorithm
//#else
// #define LABEL_SETTING 1

 /**< This macro decides if the "label-setting" style is used.

    With a priority lists, the SPT algorithm applied to SPT problems
    with *all nonnegative arc costs* has the "label-setting" property:
    nodes only exit from Q once, hence when a node exits from Q its
    label is permanently set.

    If LABEL_SETTING > 0 the code will assume that this property holds
    and implement some things accordingly; in particular, the algorithm
    is terminated when the last destination is extracted from Q even
    though Q is still nonempty.

    \warning Solving a SPT algorithm with negative arc costs with
             LABEL_SETTING > 0 may produce a suboptimal solution. */

//Johannes Sommer
//4-heaps may perform better than binary heaps in practice, even for delete-min operations.
//http://en.wikipedia.org/wiki/D-ary_heap
// #if( SPT_ALGRTM == 4 )
//  #define HeapCard 2
//
//  /**< Number of sons of each node in the heap.
//     SPT_ALGRTM == 4 means using a C-ary heap to hold the node set Q: each
//     HeapCard is the ariety of the heap, i.e. the max number of sons of a
//     node in the heap. Special treatment is deserved to the case
//     HeapCard == 2. */
// #endif
//#endif

/*------------------------------ SPT_STRTN ---------------------------------*/

#define SPT_STRTN 1

/* Decides if the "start node" information for each arc is explicitly kept
   in a data structure. If SPT_STRTN == 0 the "start node" information is
   computed in O( ln( n ) ) each time it is needed. This is only used in
   methods for reading or changing the data of the problem, and not in the
   main (SPT) algorithm, so it may not be too costly. If SPT_STRTN == 1
   instead the data structure is constructed; if SAME_GRPH_SPT > 0, the
   data structure is "static".

   \note This switch does not appear in the manual because the current
         implementation of Startn() for SPT_STRTN == 0 is flawed. */

/*------------------------------ ORDRD_NMS ---------------------------------*/

#define ORDRD_NMS 1

/**< Decides if arc names in MCFGetX() are ordered.
   If ORDRD_NMS > 0, and MCFGetX() [see below] is asked for a "sparse" flow
   solution (i.e., nms != NULL), then the set of indices returned at the end
   of the method is ordered in increasing sense. If ORDRD_NMS == 0 instead,
   the set of indices may not be ordered.

   ORDRD_NMS > 0 may be useful for some applications, but it is more costly
   (basically, it requires either to compute the "dense" flow solution or to
   sort a vector). Also, "sparse" flow solutions in this class are guaranteed
   to contain no more than n - 1 nonzeroes, hence if ORDRD_NMS == 0 then the
   parameter `F' in MCFGetX( F , nms ) can actually point to a (n - 1)-vector,
   while if ORDRD_NMS > 0 it must point to a m-vector anyway. */

/*----------------------------- SAME_GRPH_SPT ------------------------------*/

#define SAME_GRPH_SPT 0

/**< Decides if all MCFClass instances share the same graph.
   If SAME_GRPH_SPT > 0, then all the instances of the class will work on the
   same "topological" network, while the costs, capacities and supplies can
   change from one instance to another. This allows implementations to share
   some data structures describing the graph, e.g. by declaring them "static",
   saving memory when multiple instances of the solver are active at the same
   time. */

/*----------------------------- DYNMC_MCF_SPT ------------------------------*/

#define DYNMC_MCF_SPT 0

/**< Decides if the graph topology (arcs, nodes) can be changed.
   If DYNMC_MCF_SPT > 0, some of the methods of the public interface of
   class that allow to change the topology of the underlying network are
   actually implemented. Possible values of this macro are:

   - 0 => the topology of the graph cannot be changed;

   - 1 => all the methods that change the topology of the graph are
          implemented. */

/*@} -----------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

#if( OPT_USE_NAMESPACES )
namespace MCFClass_di_unipi_it
{
#endif

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASSES -------------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup SPTREE_CLASSES Classes in SPTree.h
    @{ */

/** The SPTree class derives from the abstract base class MCFClass, thus
    sharing its (standard) interface, and implements Shortest Path Tree
    algorithms for solving "uncapacitated" (Linear) Min Cost Flow
    problems with one source node.

    \warning The SPT algorithm will enter in an infinite loop if a directed
             cycle of negative cost exists in the graph: there is no check
	     about this in the code. */

namespace netxpert {
    namespace core {

/**
*  \Class Core Solver for the Shortest Path Tree Problem with configurable Heap structure (n) and Dijkstra's algorithm.
*/
class SPTree_Heap : public MCFClass, public netxpert::core::ISPTree
{

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/
/*--                                                                      --*/
/*--  The following methods and data are the actual interface of the      --*/
/*--  class: the standard user should use these methods and data only.    --*/
/*--                                                                      --*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------------- PUBLIC METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/

   SPTree_Heap( cIndex nmx = 0 , cIndex mmx = 0 , bool Drctd = true, int HeapAry = 2);

/**< Constructor of the class.

   For the meaning of nmx and mmx see MCFClass::MCFClass().

   The parameter `Drctd' tells if the given graph has really to be
   understood as directed (default), i.e., if the i-th arc is
   Sn[ i ] --> En[ i ], or undirected, i.e., the i-th arc is
   Sn[ i ] <--> En[ i ]. Undirected graphs are internally implemented by
   doubling each arc, but this is completely hidden by the interface. */

/*--------------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/

   void LoadNet( cIndex nmx = 0 , cIndex mmx = 0 , cIndex pn = 0 ,
		 cIndex pm = 0 , cFRow pU = NULL , cCRow pC = NULL ,
		 cFRow pDfct = NULL , cIndex_Set pSn = NULL ,
		 cIndex_Set pEn = NULL );

    // Johannes Sommer
   //-- Methods for ISPTree interface
   void LoadNet(unsigned int nmx, unsigned int mmx, unsigned int pn, unsigned int pm, double* pU,
                            double* pC, double* pDfct, unsigned int* pSn, unsigned int* pEn);
   unsigned int MCFmmax();
   unsigned int MCFnmax();
   ISPTreePtr create () const        // Virtual constructor (creation)
   {
      return ISPTreePtr(new SPTree_Heap ());
   }
   ISPTreePtr clone () const        // Virtual constructor (copying)
   {
      return ISPTreePtr(new SPTree_Heap (*this));
   }
   //void GetPath ( unsigned int Dst, unsigned int *outSn, unsigned int *outEn );
   //--

/**< Inputs a new network, as in MCFClass::LoadNet().

   Arcs with pC[ i ] == Inf<CNumber>() do not "exist". If DYNMC_MCF_SPT > 0,
   these arcs are "closed".

   If DYNMC_MCF_SPT == 0 but SAME_GRPH_SPT > 0, these arcs are dealt with
   explicitly, and can be put back into the formulation by simply changing
   their cost. Note that, however, this is less efficient than eliminating
   them explicitly from the problem.

   If DYNMC_MCF_SPT == 0 and SAME_GRPH_SPT == 0, these arcs are just removed
   from the formulation. However, they have some sort of a "special status"
   (after all, if the user wants to remove them completely he/she can just
   change the data), in that they are still counted into the number of arcs
   of the graph and they will always have 0 flow and Inf<CNumber>() reduced
   cost as "closed" or "deleted" arcs. */

/*--------------------------------------------------------------------------*/
/*-------------------- METHODS FOR SOLVING THE PROBLEM ---------------------*/
/*--------------------------------------------------------------------------*/

   void SolveMCF( void );

/*--------------------------------------------------------------------------*/
/*---------------------- METHODS FOR READING RESULTS -----------------------*/
/*--------------------------------------------------------------------------*/

   void MCFGetX( FRow F , Index_Set nms = NULL  ,
		 cIndex strt = 0 , Index stp = Inf<Index>() );

/*--------------------------------------------------------------------------*/

   void MCFGetRC( CRow CR , cIndex_Set nms = NULL  ,
		  cIndex strt = 0 , Index stp = Inf<Index>() );

   inline CNumber MCFGetRC( cIndex i );

/*--------------------------------------------------------------------------*/

   void MCFGetPi( CRow P , cIndex_Set nms = NULL  ,
		  cIndex strt = 0 , Index stp = Inf<Index>() );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   cCRow MCFGetPi( void );

/**< Same meaning as MCFClass::MCFGetPi().

   \note Some of the potentials may be + Inf<CNumber>(): this means that

   - the node is *not* a destination and it cannot be reached from the Origin
     (however, this does *not* mean that the problem is unfeasible);

   - if LABEL_SETTING == 1, the node is *not* a destination and it has not
     been reached during the algorithm. */

/*--------------------------------------------------------------------------*/

   SPTree_Heap::FONumber MCFGetFO( void );

/**< Same meaning as MCFClass::MCFGetFO().

   \note if not all the specified destinations can be reached from the
         Origin, returns Inf<FONumber>(). */

/*--------------------------------------------------------------------------*/
/*-------------- METHODS FOR READING THE DATA OF THE PROBLEM ---------------*/
/*--------------------------------------------------------------------------*/

   void MCFArcs( Index_Set Startv , Index_Set Endv , cIndex_Set nms = NULL  ,
		 cIndex strt = 0 , Index stp = Inf<Index>() );

   inline Index MCFSNde( cIndex i );

   inline Index MCFENde( cIndex i );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   void MCFCosts( CRow Costv , cIndex_Set nms = NULL  ,
		  cIndex strt = 0 , Index stp = Inf<Index>() );

   inline CNumber MCFCost( cIndex i );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   void MCFUCaps( FRow UCapv , cIndex_Set nms = NULL  ,
		  cIndex strt = 0 , Index stp = Inf<Index>() );

   inline FNumber MCFUCap( cIndex i );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   void MCFDfcts( FRow Dfctv , cIndex_Set nms = NULL  ,
		  cIndex strt = 0 , Index stp = Inf<Index>() );

   inline FNumber MCFDfct( cIndex i );

/*--------------------------------------------------------------------------*/
/*------------- METHODS FOR ADDING / REMOVING / CHANGING DATA --------------*/
/*--------------------------------------------------------------------------*/
/*----- Changing the costs, deficits and upper capacities of the (MCF) -----*/
/*--------------------------------------------------------------------------*/

   void ChgCosts( cCRow NCost , cIndex_Set nms = NULL  ,
		  cIndex strt = 0 , Index stp = Inf<Index>() );

   void ChgCost( Index arc , cCNumber NCost );

/*--------------------------------------------------------------------------*/

   void ChgDfcts( cFRow NDfct , cIndex_Set nms = NULL  ,
		  cIndex strt = 0 , Index stp = Inf<Index>() );

   void ChgDfct( Index nod , cFNumber NDfct );

/*--------------------------------------------------------------------------*/

   void ChgUCaps( cFRow NCap , cIndex_Set nms = NULL  ,
		  cIndex strt = 0 , Index stp = Inf<Index>() );

   void ChgUCap( Index arc , cFNumber NCap );

/*--------------------------------------------------------------------------*/
/*--------------- Modifying the structure of the graph ---------------------*/
/*--------------------------------------------------------------------------*/

  void CloseArc( cIndex name );

  inline bool IsClosedArc( cIndex name );

  void DelNode( cIndex name );

  void OpenArc( cIndex name );

  Index AddNode( cFNumber aDfct );

  void ChangeArc( cIndex name ,
		  cIndex nSS = Inf<Index>() , cIndex nEN = Inf<Index>() );

  void DelArc( cIndex name );

  inline bool IsDeletedArc( cIndex name );

  Index AddArc( cIndex Start , cIndex End , cFNumber aU , cCNumber aC );

/*--------------------------------------------------------------------------*/
/*------------------------ SPECIALIZED INTERFACE ---------------------------*/
/*--------------------------------------------------------------------------*/

   void ShortestPathTree( void );

/**< Solver of the Shortest Path Tree Problem from the current Origin.
   (specified in the constructor or by SetOrigin(), see below)

   If LABEL_SETTING == 0, or if no Destination is speficied (Dst ==
   Inf<Index>() in SetDest() [see below]), the whole Shortest Path Tree (at
   least, the SPT of the component of the graph connected with Origin) is
   computed, otherwise the code stops as soon as the shortest path between
   Origin and Dest is computed.

   Note that methods such as MCFGetX(), MCFGetRC() and MCFGetFO() may need
   some complicate calculations in order to put the solution of the Shortest
   Path in the correct format; since these calculations change some of the
   internal data structures, it is not permitted to call again
   ShortestPathTree() after that any of these methods have been called. */

/*--------------------------------------------------------------------------*/

   inline void SetOrigin( cIndex NewOrg );

/**< Changes the Origin from which Shortest Paths are computed. */

/*--------------------------------------------------------------------------*/

   inline void SetDest( cIndex NewDst );

   //Johannes Sommer
   //inline void SetDest( int i );

/**< Changes the Destination node of Shotest Paths. If LABEL_SETTING == 0, it
   has no influence since label correcting methods cannot stop before the
   whole SPT has been computed. Conversely, label setting algorithms can solve
   Origin-Dest Shortest Path Problems; therefore, it is possible to obtain
   shortest paths between Origin and a subset of the nodes, by calling
   ShortestPathTree() with one of the destinations, and controlling upon
   completion that all the desidered nodes have been visited (see Reached()
   below). If this is not the case, ShortestPathTree() can be invoked again
   with one of the unreached nodes, until they are all visited.

   If no Dest is given, or if Dest is set to Inf<Index>(), the whole Shortest
   Path Tree (at least, the SPT of the component of the graph connected with
   Origin) is computed. */

/*--------------------------------------------------------------------------*/

   void MCFGetX( Index ND , cIndex_Set DB , FRow F , Index_Set nms = NULL ,
		 cIndex strt = 0 , Index stp = Inf<Index>() );

/**< Like SPTree::MCFGetX( FRow , Index_Set , cIndex , Index ), except that
   the primal solution that is returned is relative only to the subset of
   destinations whose names are contained in the first ND entries of the
   vector DB. */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

   SPTree_Heap::FONumber MCFGetFO( Index ND , cIndex_Set DB );

/**< Like SPTree::MCFGetFO( void ), except that the cost that is returned is
   that of the primal solution relative only to the subset of destinations
   whose names are contained in the first ND entries of the vector DB. */

/*--------------------------------------------------------------------------*/

   inline bool Reached( cIndex i );

/**< Return true if a shortest path from Origin to i have already been
   computed; this can be used when LABEL_SETTING == 1 to determine if a
   shortest from Origin to i have been obtained as a by-product of the
   calculation of the shortest path between Origin and some other Dest. */

/*--------------------------------------------------------------------------*/

   inline unsigned int* Predecessors( void );
   //Johannes Sommer
   //inline cIndex_Set Predecessors( unsigned int *size );
   //Johannes Sommer
   inline void GetPredecessors( unsigned int *outPrd );

/**< Return a cIndex* vector p[] such that p[ i ] is the predecessor of node
   i in the shortest path tree. If a node i has no predecessor, i.e.,
   i == Origin, i does not belong to the connected component of the origin or
   the computation have been stopped before reaching i, then p[ i ] == 0.

   \note if the name "0" is used for nodes, (USENAME0 == 1) then node names
         are internally "translated" of +1 to avoid it being used - the
	 the names reported in this vector will follow the same rule.

   For this reason, the first entry of p (*p) is not significative. */

/*--------------------------------------------------------------------------*/

   inline unsigned int* ArcPredecessors( void );
   //Johannes Sommer
   //inline cIndex_Set ArcPredecessors( unsigned int *size );
   inline void GetArcPredecessors ( unsigned int *outArcPrd );

/**< Return a cIndex* vector a[] such that a[ i ] is the index of the arc
   ( p[ i ] , i ), being p[] the vector returned by the above method, and
   with the same structure. If p[ i ] == 0, then a[ i ] is not significative:
   for the Origin (that has p[ Origin ] == 0), however, it is guaranteed that
   a[ Origin ] == Inf<Index>(). */

/*--------------------------------------------------------------------------*/

   inline Index DestN( void );

/**< Return the number of destination nodes in the SPT problem. */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

   inline cIndex_Set Dests( void );
   //Johannes Sommer
   //inline cIndex_Set Dests( unsigned int *size );
   inline void GetDests ( unsigned int *outDstBse );

/**< Return the DestN()-vector containig the names of destination nodes in
   the SPT problem. */

/*--------------------------------------------------------------------------*/
/*------------------------------ DESTRUCTOR --------------------------------*/
/*--------------------------------------------------------------------------*/

   virtual ~SPTree_Heap();

/*--------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/
/*--                                                                      --*/
/*--  The standard user should not care about the following part: users   --*/
/*--  who need to extend the code by deriving a new class may use these   --*/
/*--  methods and data structures. It is *dangerous* to *modify* the      --*/
/*--  data structures, while it safe to read them.                        --*/
/*--                                                                      --*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED TYPES ------------------------------*/
/*--------------------------------------------------------------------------*/

 struct FSElmnt {               // one entry of the Forward Star
                  CNumber Cst;  // cost of the arc
                  Index   Nde;  // end node of the arc
	          };

 typedef FSElmnt *FrwdStr;

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*--------------------- PROTECTED DATA STRUCTURES --------------------------*/
/*--------------------------------------------------------------------------*/

 Index Origin;       // the source
 Index Dest;         // the sink
 FNumber Flow;       // in O-D problems, it is flow to be sent from Origin to
                     // Dest, otherwise is is the supply of Origin, i.e. the
		     // sum of all the deficit of destination nodes

 Index NDsts;        // total number of destinations;
 Index_Set DstBse;   // array of indices of the destinations

 Index_Set NdePrd;   // NdePrd[ i ] = predecessor of i in the shortest path
                     // NdePrd[ Origin ] = 0

 Index_Set ArcPrd;   // ArcPrd[ i ] = index of arc ( NdePrd[ i ] , i )
                     // ArcPrd[ Origin ] = 0

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/
/*--                                                                      --*/
/*-- Nobody should ever look at this part: everything that is under this  --*/
/*-- advice may be changed without notice in any new release of the code. --*/
/*--                                                                      --*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/
/*--------------------------- PRIVATE METHODS ------------------------------*/
/*--------------------------------------------------------------------------*/

   inline void Initialize( void );

/* Initialize the data structures for a "cold start". */

/*--------------------------------------------------------------------------*/

   inline void ScanFS( cIndex mi );

/* Scans the Forward Star of mi and puts in Q those nodes whose distance
   label can be decreased by using an arc emanating from mi. */

/*--------------------------------------------------------------------------*/

   inline Index ExtractQ( void );

/* Extracts an element (depending on the particular algoritm) from the set Q:
   if Q is empty, returns 0. */

/*--------------------------------------------------------------------------*/

//#if( ( SPT_ALGRTM == 0 ) || ( SPT_ALGRTM == 3 ) )
//
//   inline void InsertQ( cIndex j );
//
//#else

   inline void InsertQ( cIndex j , cCNumber label );

//#endif

/* Inserts the node with name j and label label somewhere in Q: the label is
   not needed for LQueue and Djkstra algorithms. */

/*--------------------------------------------------------------------------*/

   inline void CalcArcP( void );

/* Calculates the ArcPrd[] vector. */

/*--------------------------------------------------------------------------*/

#if( ! SPT_STRTN )

   inline Index Startn( cIndex What );

/* Extract the starting node of the arc that is in position What in FS[]
   using a binary search on StartFS[]. */

#endif

/*--------------------------------------------------------------------------*/

 inline void MemAlloc( void );

 inline void MemDeAlloc( void );

/*--------------------------------------------------------------------------*/
/*----------------------- PRIVATE DATA STRUCTURES  -------------------------*/
/*--------------------------------------------------------------------------*/

 int HeapCard;

 CRow Pi;            // node Potentials
 FRow B;             // node deficits vector

 FONumber FO;        // Objective Function value

 bool ReadyArcP;     // if the "arc predecessor" data structure has already
                     // been updated after a "final" ShortestPathTree() call
 FrwdStr FS;         // the Forward Star (itself)

 Index_Set Q;        // the set of scanned nodes: Q[ i ] = Inf<Index>() => i is not Q
 //#if( SPT_ALGRTM <= 3 )
 //                    // here, Q is an array pointer implementation of a list,
 //                    // and *Q is the head of the list (node names are >= 1)
 //#else
  Index_Set H;       // here, Q[ i ] tells the position of node i in the
		     // vector implementing the heap, and H is that vector
 //#endif

 Index cFS;          // cardinality of the FS (m if DirSPT, 2m otherwise)
 Index tail;         // the tail element of the list, or the first free
                     // position in the heap

 // static members- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 static Index_Set Stack;  // temporary for flow calculation and
                          // reoptimization
 static Index InstCntr;   // counter of active instances
 static Index maxnmax;    // max value of nmax among all the instances

 #if( SPT_STRTN )
  #if( SAME_GRPH_SPT )
   static Index_Set Startn;
  #else
   Index_Set Startn;
  #endif
 #endif

 #if( SAME_GRPH_SPT && ( ! DYNMC_MCF_SPT ) )
  static Index_Set StrtFS;

  static Index_Set Dict;
  static Index_Set DictM1;

  static bool DirSPT;
 #else
  Index_Set StrtFS;  // position in FS[] where FS[ i ] begins
  #if( DYNMC_MCF_SPT )
   Index_Set LenFS;  // how many arcs there are in FS[ i ]
  #endif

  Index_Set Dict;    // arc dictionary: for each position in FS[], tells
                     // which arc is that one
  Index_Set DictM1;  // inverse of Dict: for each arc, tells where it stands
                     // in FS[] - if the graph is undirected, the two
		     // consecutive entries 2 * i and 2 * i + 1 tells the
		     // two positions of arc i in FS[]
  bool DirSPT;       // true if the graph is directed
 #endif

/*--------------------------------------------------------------------------*/

 };  // end( class SPTree )

/* @} end( group( SPTREE_CLASSES ) ) */
/*--------------------------------------------------------------------------*/
/*------------------- inline methods implementation ------------------------*/
/*--------------------------------------------------------------------------*/

inline MCFClass::Index SPTree_Heap::MCFSNde( cIndex i )
{
 if( DirSPT )
  #if( SPT_STRTN )
   return( Startn[ i ] );
  #else
   return( Startn( DictM1[ i ] ) - USENAME0 );
  #endif
 else
  return( FS[ DictM1[ 2 * i + 1 ] ].Nde );
 }

/*--------------------------------------------------------------------------*/

inline MCFClass::Index SPTree_Heap::MCFENde( cIndex i )
{
 if( DirSPT )
  return( FS[ DictM1[ i ] ].Nde - USENAME0 );
 else
  return( FS[ DictM1[ 2 * i ] ].Nde - USENAME0 );
 }

/*--------------------------------------------------------------------------*/

inline MCFClass::CNumber SPTree_Heap::MCFCost( cIndex i )
{
 if( DirSPT )
  return( FS[ DictM1[ i ] ].Cst );
 else
  return( FS[ DictM1[ 2 * i ] ].Cst );
 }

/*--------------------------------------------------------------------------*/

inline MCFClass::FNumber SPTree_Heap::MCFUCap( cIndex i )
{
 #if( ! DYNMC_MCF_SPT )
  cIndex pos = DictM1[ i ];
  #if( SAME_GRPH_SPT )
   if( FS[ pos ].Cst == Inf<CNumber>() )
  #else
   if( pos >= StrtFS[ n + 1 ] )
  #endif
    return( 0 );
   else
 #endif
    return( Flow );
 }

/*--------------------------------------------------------------------------*/

inline MCFClass::FNumber SPTree_Heap::MCFDfct( cIndex i )
{
 #if( USENAME0 )
  cIndex myi = i;
 #else
  cIndex myi = i + 1;
  #endif

 if( Origin == myi )
  return( - Flow );
 else {
  for( Index_Set tDB = DstBse + NDsts ; tDB-- > DstBse ; )
   if( *tDB == myi )
    return( B[ tDB - DstBse ] );

  return( 0 );
  }
 }

/*--------------------------------------------------------------------------*/

inline bool SPTree_Heap::IsClosedArc( cIndex name )
{
 #if( DYNMC_MCF_SPT )
  cIndex pos = DictM1[ name ];     // current position of arc name
  #if( SPT_STRTN )
   cIndex nde = Startn[ name ];    // start node of arc name
  #else
   cIndex nde = Startn( pos );     // start node of arc name
  #endif

  return( pos < StrtFS[ nde ] + LenFS[ nde ] );
 #else
  return( false );
 #endif
 }

/*--------------------------------------------------------------------------*/

inline bool SPTree_Heap::IsDeletedArc( cIndex name )
{
 return( SPTree_Heap::IsClosedArc( name ) );
 }

/*--------------------------------------------------------------------------*/

inline void SPTree_Heap::SetOrigin( cIndex NewOrg )
{
 if( Origin != NewOrg + USENAME0 ) {
  Origin = NewOrg + USENAME0;
  status = MCFClass::kUnSolved;
  }
 }

/*--------------------------------------------------------------------------*/

inline void SPTree_Heap::SetDest( cIndex NewDst )
{
  //Johannes Sommer
  //UINT_MAX fails on 64Bit machines - for 32Bit ok
  //--> UINT_MAX + 1 is ok for 64Bit machines
 Index localNewDst = NewDst;
 if (NewDst == Inf<unsigned int>())

#if defined(__linux__) && defined(__LP64__)
    localNewDst = NewDst + 1;
#endif
#if defined(__linux__) && !defined(__LP64__)
    localNewDst = NewDst;
#endif // __linux__

#if defined(_WIN32) && !defined(_M_AMD64)
 localNewDst = NewDst;
#endif
#if defined(_WIN64) || defined(_M_AMD64)
 localNewDst = NewDst + 1;
 std::cout << "adding +1 to UINT_MAX" << std::endl;
#endif

 if( Dest != localNewDst + USENAME0 ) {
  //#if( LABEL_SETTING )
   Dest = localNewDst + USENAME0;
  //#endif
  status = MCFClass::kUnSolved;
  }
 }

//Johannes Sommer
//inline void SPTree_Heap::SetDest( int i )
//{
//	if ( i == -1)
//	{
//		MCFClass::cIndex NewDest = Inf<MCFClass::cIndex>();
//	if( Dest != NewDest + USENAME0 ) {
//	#if( LABEL_SETTING )
//	Dest = NewDest + USENAME0;
//	#endif
//	status = MCFClass::kUnSolved;
//	}
//	}
//}

/*--------------------------------------------------------------------------*/

inline bool SPTree_Heap::Reached( cIndex i )
{
 return( ( Pi[ i ] < Inf<CNumber>() ) && ( Q[ i ] == Inf<Index>() ) );
 }

/*--------------------------------------------------------------------------*/

inline unsigned int* SPTree_Heap::Predecessors( void )
{
 return( NdePrd );
 }

/*--------------------------------------------------------------------------*/

inline MCFClass::Index SPTree_Heap::DestN( void )
{
 return( NDsts );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

inline MCFClass::cIndex_Set SPTree_Heap::Dests( void )
{
 return( DstBse );
 }

/*--------------------------------------------------------------------------*/

//Johannes Sommer
inline unsigned int* SPTree_Heap::ArcPredecessors( void )
{
 CalcArcP();

 return( ArcPrd );
 }

//Johannes Sommer
inline void SPTree_Heap::GetArcPredecessors( unsigned int *outArcPrd )
{
	unsigned int size = MCFClass::MCFnmax()+1;
	MCFClass::cIndex_Set originals = ArcPredecessors();
	memcpy( outArcPrd, originals, size * sizeof( MCFClass::cIndex ) ) ;
    //delete[] originals;
}

//Johannes Sommer
inline void SPTree_Heap::GetPredecessors( unsigned int *outPrd )
{
	unsigned int size = MCFClass::MCFnmax()+1;
	MCFClass::cIndex_Set originals = Predecessors();
	memcpy( outPrd, originals, size * sizeof( MCFClass::cIndex) ) ;
    //delete[] originals;
}
//Johannes Sommer
inline void SPTree_Heap::GetDests( unsigned int *outDstBse )
{
	unsigned int size = MCFClass::MCFnmax()+1;
	MCFClass::cIndex_Set originals = Dests();
	memcpy( outDstBse, originals, size * sizeof( MCFClass::cIndex ) ) ;
}
} //namespace core
} //namespace NetXpert

#if( OPT_USE_NAMESPACES )
 };  // end( namespace MCFClass_di_unipi_it )
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* SPTree_Heap.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File SPTree_Heap.h ------------------------*/
/*--------------------------------------------------------------------------*/
