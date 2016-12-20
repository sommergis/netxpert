/*--------------------------------------------------------------------------*/
/*----------------------------- File SPTree_Dijkstra.C ---------------------*/
/*--------------------------------------------------------------------------*/
/*--                                                                      --*/
/*-- Implementation of several "classic" Shortest Path Tree algorithms to --*/
/*-- solve uncapacitated single-source Min Cost Flow problems.            --*/
/*--                                                                      --*/
/*--                            VERSION 1.80                              --*/
/*--                           11 - 02 - 2011                             --*/
/*--                                                                      --*/
/*--                          Implementation by:                          --*/
/*--                                                                      --*/
/*--                          Antonio Frangioni       	                  --*/
/*--                                                                      --*/
/*--                       Operations Research Group                      --*/
/*--                      Dipartimento di Informatica                     --*/
/*--                         Universita' di Pisa                          --*/
/*--                                                                      --*/
/*--              Copyright (C) 1996 - 2011 by Antonio Frangioni.         --*/
/*--                                                                      --*/
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

//Johannes Sommer
// compile error: "assert" not found
#include "assert.h"
#include "SPTree_Dijkstra.h"
using namespace netxpert::core;

/*--------------------------------------------------------------------------*/
/*--------------------------------- MACROS ---------------------------------*/
/*--------------------------------------------------------------------------*/

//#if( ( SPT_ALGRTM == 0 ) || ( SPT_ALGRTM == 3 ) )
 #define InsertQ( j , l ) InsertQ( j )

// // the label is not needed for LQueue and Djkstra algorithms
//#endif

#if( DYNMC_MCF_SPT )
 #define LenFS( i ) LenFS[ i ]
#else
 #define LenFS( i ) StrtFS[ i + 1 ] - StrtFS[ i ]
#endif

#if( SPT_STRTN )
 #define Stn( i , pos ) Startn[ i ]
#else
 #define Stn( i , pos ) Startn( pos )
#endif

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

#if( OPT_USE_NAMESPACES )
using namespace MCFClass_di_unipi_it;
#endif

/*--------------------------------------------------------------------------*/
/*---------------------------- PROCEDURES ----------------------------------*/
/*--                                                                      --*/
/*--  These procedures are not implemented as methods of the class, since --*/
/*--  they don't use directly the data structures of the class: rather,   --*/
/*--  the data they need is explicitely passed as parameters.             --*/
/*--                                                                      --*/
/*--------------------------------------------------------------------------*/

#if( DYNMC_MCF_SPT )

template<class T>
static inline void Swap( T &v1 , T &v2 )
{
 T temp = v1;

 v1 = v2;
 v2 = temp;
 }

#endif

/*--------------------------------------------------------------------------*/
/*----------------------- IMPLEMENTATION OF SPTree -------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- PUBLIC METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/
/*---------------------------- CONSTRUCTOR ---------------------------------*/
/*--------------------------------------------------------------------------*/

SPTree_Dijkstra::SPTree_Dijkstra( cIndex nmx , cIndex mmx , bool Drctd )
	:
        MCFClass( nmx , mmx )
{
 DirSPT = Drctd;

 // allocate memory - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( nmax && mmax )
  {
	  //Johannes Sommer
	  //MemDeAlloc();
	  MemAlloc();
  }
 else
  nmax = mmax = 0;

 // other initializations - - - - - - - - - - - - - - - - - - - - - - - - - -

 InstCntr++;

 }  // end( SPTree )

/*--------------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::LoadNet( cIndex nmx , cIndex mmx , cIndex pn , cIndex pm ,
		      cFRow pU , cCRow pC , cFRow pDfct ,
		      cIndex_Set pSn , cIndex_Set pEn )
{
 // allocating and deallocating memory- - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( ( nmx != nmax ) || ( mmx != mmax ) ) {
  if( nmax && mmax ) {
   MemDeAlloc();
   nmax = mmax = 0;
   }
  if( mmx && nmx ) {
   nmax = nmx;
   mmax = mmx;
   MemAlloc();
   }
  }

 if( ( ! nmax ) || ( ! mmax ) ) {  // just sit down in the corner and wait
  nmax = mmax = 0;
  return;
  }

 // now setting up data - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 n = pn;
 m = pm;
 assert( pDfct );
 assert( pC );

 #if( SPT_STRTN )
  #if( SAME_GRPH_SPT )
   if( ! *Startn )  // Startn[] has not been initialized yet- - - - - - - - -
  #endif
   {
    Index_Set tSn = Startn + m;
    for( pSn += m ; tSn > Startn ; )
     *(--tSn) = *(--pSn);
    }
 #endif

 #if( SAME_GRPH_SPT && ( ! DYNMC_MCF_SPT ) )
  if( StrtFS[ 1 ] == Inf<Index>() )
    // the FS has not been initialized yet- - - - -
 #endif
  {
   // compute the cardinality of FS[ i ] ( i = 1 .. n ) into NdePdr[] - - - -

   Index_Set tNP = NdePrd + n;
   for( ; tNP > NdePrd ; )
    *(tNP--) = 0;

   #if( USENAME0 )
    tNP++;  // if USENAME0 == 1, node names are "translated" of + 1
            // to make name 0 *not* being used
   #endif

   #if( SAME_GRPH_SPT || DYNMC_MCF_SPT )
    // count non-existent arcs (with Inf<CNumber>() cost) in the FSs

    for( cIndex_Set tSn = pSn + m ; tSn > pSn ; )
     tNP[ *(--tSn) ]++;

    if( ! DirSPT )
     for( cIndex_Set tSn = pEn + m ; tSn > pEn ; )
      tNP[ *(--tSn) ]++;
   #else
   {
    // non-existent arcs are completely removed from the formulation

    cCRow tC = pC + m;
    cIndex_Set tSn = pSn + m;
    if( DirSPT ) {
     for( ; tSn-- > pSn ; )
      if( *(--tC) < Inf<CNumber>() )
       tNP[ *tSn ]++;
     }
    else
     for( cIndex_Set tEn = pEn + m ; tSn-- > pSn ; ) {
      tEn--;
      if( *(--tC) < Inf<CNumber>() ) {
       tNP[ *tSn ]++;
       tNP[ *tEn ]++;
       }
      }
    }
   #endif

   #if( USENAME0 )
    tNP--;  // keep the invariant that the lenght of the FS() of the first
            // node is in tNP[ 1 ]
   #endif

   // compute StrtFS[] and copy it into NodePdr[] - - - - - - - - - - - - - -

   Index j = 0;
   Index_Set tSFS = StrtFS;
   for( Index i = n ; i-- ; ) {
    cIndex h = *(++tNP);
    *(++tSFS) = *tNP = j;
    j += h;
    }

   *(++tSFS) = j;

   // construct Dict[] and DictM1[]:  - - - - - - - - - - - - - - - - - - - -
   // in the first pass, only consider the "existing" arcs- - - - - - - - - -

   tNP = NdePrd;
   #if( USENAME0 )
    tNP++;  // once again, shift the vector to adapt to the naming
   #endif

   #if( ( ! SAME_GRPH_SPT ) || DYNMC_MCF_SPT )
    cCRow tC = pC;
   #endif
   if( DirSPT )
    for( Index i = 0 ; i < m ; i++ ) {
     #if( ( ! SAME_GRPH_SPT ) || DYNMC_MCF_SPT )
      if( *(tC++) == Inf<CNumber>() )
       continue;
     #endif

     j = tNP[ pSn[ i ] ]++;
     DictM1[ i ] = j;
     Dict[ j ] = i;
     }
   else
    for( Index i = 0 ; i < m ; i++ ) {
     #if( ( ! SAME_GRPH_SPT ) || DYNMC_MCF_SPT )
      if( *(tC++) == Inf<CNumber>() )
       continue;
     #endif

     j = tNP[ pSn[ i ] ]++;
     Index h = 2 * i;
     DictM1[ h++ ] = j;
     Dict[ j ] = i;

     j = tNP[ pEn[ i ] ]++;
     DictM1[ h ] = j;
     Dict[ j ] = i;
     }

   #if( DYNMC_MCF_SPT )
    // construct LenFS[]- - - - - - - - - - - - - - - - - - - - - - - - - - -

    for( Index i = 0 ; i++ < n ; )
     LenFS[ i ] = NdePrd[ i ] - StrtFS[ i ];

    // now do a second pass considering only *non*existent arcs - - - - - - -
    // hence, all nonexistent arcs in the FS[ i ] are packed after all the
    // existent ones, and LenFS[ i ] only counts the latter

    tC = pC;
    if( DirSPT ) {
     for( Index i = 0 ; i < m ; i++ )
      if( *(tC++) == Inf<CNumber>() ) {
       j = tNP[ pSn[ i ] ]++;
       DictM1[ i ] = j;
       Dict[ j ] = i;
       }
     }
    else
     for( Index i = 0 ; i < m ; i++ )
      if( *(tC++) == Inf<CNumber>() ) {
       j = tNP[ pSn[ i ] ]++;
       Index h = 2 * i;
       DictM1[ h++ ] = j;
       Dict[ j ] = i;

       j = tNP[ pEn[ i ] ]++;
       DictM1[ h ] = j;
       Dict[ j ] = i;
       }
   #elif( ! SAME_GRPH_SPT )
    // now do a second pass considering only *non*existent arcs - - - - - - -
    // all nonexistent arcs are packed after the "end" of FS[], i.e. from
    // StrtFS[ n + 1 ] onwards

    tC = pC;
    j = StrtFS[ n + 1 ];
    if( DirSPT ) {
     for( Index i = 0 ; i < m ; i++ )
      if( *(tC++) == Inf<CNumber>() ) {
       DictM1[ i ] = j;
       Dict[ j++ ] = i;
       }
     }
    else
     for( Index i = 0 ; i < m ; i++ )
      if( *(tC++) == Inf<CNumber>() ) {
       Index h = 2 * i;
       DictM1[ h++ ] = j;
       Dict[ j++ ] = i;

       DictM1[ h ] = j;
       Dict[ j++ ] = i;
       }
   #endif

   }  // end( if( StrtFS[ 1 ] == Inf<Index>() ) )

 // construct FS[] - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 cIndex_Set tDM1 = DictM1;

 if( DirSPT )
  for( Index i = m ; i-- ; ) {
   CNumber Ci = *(pC++);
   #if( DYNMC_MCF_SPT )
    if( Ci == Inf<CNumber>() )
     Ci = 0;
   #endif

   Index j = *(tDM1++);
   FS[ j ].Cst = Ci;
   FS[ j ].Nde = *(pEn++) + USENAME0;
   }
 else
  for( Index i = m ; i-- ; ) {
   CNumber Ci = *(pC++);
   #if( DYNMC_MCF_SPT )
    if( Ci == Inf<CNumber>() )
     Ci = 0;
   #endif

   Index j = *(tDM1++);
   FS[ j ].Cst = Ci;
   FS[ j ].Nde = *(pEn++) + USENAME0;

   FS[ j = *(tDM1++) ].Cst = Ci;
   FS[ j ].Nde = *(pSn++) + USENAME0;
   }

 // construct Origin/Destination-related information- - - - - - - - - - - - -

 ReadyArcP = false;
 assert( pDfct );  // there must be at least two nonzero deficits

 Flow = 0;
 NDsts = 0;
 for( Index i = 0 ; i++ < n ; ) {
  cFNumber di = *(pDfct++);
  if( di > 0 ) {
   Flow += ( B[ NDsts ] = di );
   DstBse[ NDsts++ ] = i;
   }
  else
   if( di < 0 )
    Origin = i;
  }
 //Johannes Sommer
 //delete[] pU;
 //delete[] pC;
 //delete[] pDfct;
 //delete[] pSn;
 //delete[] pEn;

 }  // end( SPTree::LoadNet )

 // Johannes Sommer
 //-- Methods for ISPTree interface
 void SPTree_Dijkstra::LoadNet(unsigned int nmx, unsigned int mmx, unsigned int pn, unsigned int pm, double* pU,
                            double* pC, double* pDfct, unsigned int* pSn, unsigned int* pEn)
 {

    SPTree_Dijkstra::LoadNet( (cIndex)nmx,  (cIndex)mmx, (cIndex)pn,  (cIndex)pm, (cFRow) pU, (cCRow) pC,
                                (cFRow) pDfct, (cIndex_Set) pSn, (cIndex_Set) pEn );
 }
 unsigned int SPTree_Dijkstra::MCFmmax()
 {
    return (unsigned int) MCFClass::MCFmmax();
 }
 unsigned int SPTree_Dijkstra::MCFnmax()
 {
    return (unsigned int) MCFClass::MCFnmax();
 }
 //--

/*--------------------------------------------------------------------------*/
/*-------------------- METHODS FOR SOLVING THE PROBLEM ---------------------*/
/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::SolveMCF( void )
{
 if( MCFt )
  MCFt->Start();

 #if( LABEL_SETTING )
  FO = Inf<FONumber>();
  for( Index h = 0 ;; ) {  // main cycle: until there are unreached dests - -
   Dest = DstBse[ h++ ];   // get the next unreached dest
   ShortestPathTree();     // solve the SPT with *that* Dest

   if( status )           // in case of an error
    break;                // just stop

   while( ( h < NDsts ) && Reached( DstBse[ h ] ) )  // check if there are
    h++;                                             // unreached dests

   if( h < NDsts )   // the scanning has been interrupted as soon as Dest has
    ScanFS( Dest );  // been encountered: if the process must be continued,
   else              // FS( Dest ) must be scanned, since Dest has already
    break;           // been removed from Q
   }
 #else
  ShortestPathTree();  // just solve the SPT- - - - - - - - - - - - - - - - -
 #endif

 if( MCFt )
  MCFt->Stop();

 }  // end( SPTree::SolveMCF )

/*--------------------------------------------------------------------------*/
/*---------------------- METHODS FOR READING RESULTS -----------------------*/
/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::MCFGetX( FRow F , Index_Set nms , cIndex strt , Index stp )
{
 SPTree_Dijkstra::MCFGetX( NDsts , DstBse , F , nms , strt , stp );

 }   // end( SPTree::MCFGetX )

/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::MCFGetRC( CRow CR , cIndex_Set nms , cIndex strt , Index stp )
{
 if( ! DirSPT )
  throw( MCFException( "SPTree::MCFGetRC() not allowed if DirSPT == true" ) );

 if( nms ) {
  while( *nms < strt )
   nms++;

  for( Index h ; ( h = *(nms++) ) < stp ; )
   *(CR++) = SPTree_Dijkstra::MCFGetRC( h );
  }
 else {
  if( stp > m )
   stp = m;

  for( Index i = strt ; i < stp ; )
   *(CR++) = SPTree_Dijkstra::MCFGetRC( i++ );

  /*!!
  does not seem to work

  Index j = 0;
  FrwdStr tFS = FS;
  cIndex_Set tD = Dict;
  for( Index i = 0 ; i < n ; ) {
   cCNumber Pii = Pi[ ++i ];
   #if( DYNMC_MCF_SPT )
    Index h = j + LenFS[ i ];
   #else
    Index h = StrtFS[ i + 1 ];
   #endif

   if( Pii == Inf<CNumber>() )
    for( ; j < h  ; tFS++ , j++ ) {
     cIndex k = *(tD++);
     if( ( k >= strt ) && ( k < stp ) )
      CR[ k - strt ] = Pii;
     }
   else
    for( ; j < h ; tFS++ , j++ ) {
     Index k = *(tD++);
     if( ( k < strt ) || ( k >= stp ) )
      continue;

     k -= strt;

     #if( ! DYNMC_MCF_SPT )
      if( (*tFS).Cst == Inf<CNumber>() ) {
       CR[ k ] = Inf<CNumber>();
       continue;
       }
     #endif

     cCNumber Pij = Pi[ (*tFS).Nde ];
     if( Pij < Inf<CNumber>() )
      CR[ k ] = (*tFS).Cst + Pii - Pij;
     else
      CR[ k ] = Pij;
     }

   #if( DYNMC_MCF_SPT )
    for( h = StrtFS[ i + 1 ] ; j < h ; tFS++ , j++ ) {
     cIndex k = *(tD++);
     if( ( k >= strt ) && ( k < stp ) )
      CR[ k - strt ] = Inf<CNumber>();
     }
  #endif
   }
  !!*/
  }
 }  // end( SPTree::MCFGetRC )

/*--------------------------------------------------------------------------*/

inline MCFClass::CNumber SPTree_Dijkstra::MCFGetRC( cIndex i )
{
 if( ! DirSPT )
  throw( MCFException( "SPTree::MCFGetRC() not allowed if DirSPT == true" ) );

 cIndex pos = DictM1[ i ];
 cIndex nde = FS[ pos ].Nde;

 #if( DYNMC_MCF_SPT )
  if( pos < StrtFS[ nde ] + LenFS[ nde ] )
 #else
  if( FS[ pos ].Cst < Inf<CNumber>() )
 #endif
  {
   cCNumber Pij = Pi[ nde ];
   if( Pij < Inf<CNumber>() ) {
    cIndex j = Stn( i , pos );
    if( Pi[ j ] < Inf<CNumber>() )
     return( FS[ pos ].Cst + Pi[ j ] - Pij );
    }
   }

 return( Inf<CNumber>() );

 }  // end( SPTree::MCFGetRC( i ) )

/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::MCFGetPi( CRow P , cIndex_Set nms , cIndex strt , Index stp )
{
 if( nms ) {
  while( *nms < strt )
   nms++;

  for( Index h ; ( h = *(nms++) ) < stp ; )
   *(P++) = Pi[ ++h ];
  }
 else {
  if( stp > n )
   stp = n;

  CRow tPi = Pi + stp;
  for( P += stp - strt ; tPi > Pi + strt ; )
   *(--P) = *(tPi--);
  }
 }  // end( SPTree::MCFGetPi )

/*--------------------------------------------------------------------------*/

MCFClass::cCRow SPTree_Dijkstra::MCFGetPi( void )
{
 return( Pi + 1 );
 }

/*--------------------------------------------------------------------------*/

MCFClass::FONumber SPTree_Dijkstra::MCFGetFO( void )
{
 //#if( LABEL_SETTING )
  if( FO == Inf<FONumber>() )
   FO = SPTree_Dijkstra::MCFGetFO( NDsts , DstBse );
 //#endif

 return( FO );

 }  // end( SPTree::MCFGetFO )

/*--------------------------------------------------------------------------*/
/*-------------- METHODS FOR READING THE DATA OF THE PROBLEM ---------------*/
/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::MCFArcs( Index_Set Startv , Index_Set Endv ,
		      cIndex_Set nms , cIndex strt , Index stp )
{
 assert( DirSPT );
 assert( ! nms );

 if( stp > m )
  stp = m;

 FrwdStr tFS = FS;
 cIndex_Set tDM1 = DictM1;
 for( Index i = 0 ; i++ < n ; )
  for( Index h = LenFS( i ) ; h-- ; ) {
   Index k = *(tDM1++);
   if( ( k >= strt ) && ( k < stp ) ) {
    k -= strt;

    if( Startv )
     Startv[ k ] = i - USENAME0;

    if( Endv )
     Endv[ k ] = (*(tFS++)).Nde - USENAME0;
    }
   }
 }  // end( SPTree::MCFArcs )

/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::MCFCosts( CRow Costv , cIndex_Set nms , cIndex strt , Index stp )
{
 if( nms ) {
  while( *nms < strt )
   nms++;

  if( DirSPT )
   for( Index h ; ( h = *(nms++) ) < stp ; )
    *(Costv++) = FS[ DictM1[ h ] ].Cst;
  else
   for( Index h ; ( h = *(nms++) ) < stp ; )
    *(Costv++) = FS[ DictM1[ 2 * h ] ].Cst;
  }
 else {
  if( stp > m )
   stp = m;

  Index i = stp - strt;
  Index_Set tDM1 = DictM1 + strt;

  if( DirSPT )
   for( ; i-- ; )
    *(Costv++) = FS[ *(tDM1++) ].Cst;
  else
   for( ; i-- ; tDM1++ )
    *(Costv++) = FS[ *(tDM1++) ].Cst;
  }
 }  // end( SPTree::MCFCosts )

/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::MCFUCaps( FRow UCapv , cIndex_Set nms , cIndex strt , Index stp )
{
 if( nms ) {
  while( *nms < strt )
   nms++;

  for( Index h ; ( h = *(nms++) ) < stp ; )
   *(UCapv++) = SPTree_Dijkstra::MCFUCap( h );
  }
 else {
  if( stp > m )
   stp = m;

  for( Index i = strt ; i < stp ; )
   *(UCapv++) = SPTree_Dijkstra::MCFUCap( i++ );
  }
 }  // end( SPTree::MCFUCaps )

/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::MCFDfcts( FRow Dfctv , cIndex_Set nms , cIndex strt , Index stp )
{
 if( nms ) {
  while( *nms < strt )
   nms++;

  Index h;
  Index j = 0;
  for( ; ( h = *(nms++) ) < stp ; ) {
   #if( ! USENAME0 )
    h++;
   #endif

   if( h == Origin ) {
    *(Dfctv++) = -Flow;
    break;
    }
   else
    if( h == DstBse[ j ] ) {
     *(Dfctv++) = B[ j++ ];
     if( j == NDsts )
      break;
     }
    else
     *(Dfctv++) = 0;
   }


  if( ( j < NDsts ) && ( h < stp ) )
   for( ; ( h = *(nms++) ) < stp ; ) {
    #if( ! USENAME0 )
     h++;
    #endif

    if( h == DstBse[ j ] ) {
     *(Dfctv++) = B[ j++ ];
     if( j == NDsts )
      break;
     }
    else
     *(Dfctv++) = 0;
    }

  if( h < Inf<Index>() )
   for(  ; ( h = *(nms++) ) < Inf<Index>() ; )
    *(Dfctv++) = 0;
  }
 else {
  if( stp > n )
   stp = n;

  #if( ! USENAME0 )
   cIndex mystrt = strt + 1;
   stp++;
  #else
   #define mystrt strt
  #endif

  Index i = mystrt;
  Index j = 0;

  while( DstBse[ j ] < i )
   j++;

  Dfctv -= mystrt;

  while( ( j < NDsts ) && ( i < stp ) )
   if( i == DstBse[ j ] )
    Dfctv[ i++ ] = B[ j++ ];
   else
    Dfctv[ i++ ] = 0;

  for( ; i < stp ; )
   Dfctv[ i++ ] = 0;

  if( ( Origin >= mystrt ) && ( Origin < stp ) )
   Dfctv[ Origin ] = -Flow;
  }
 }  // end( SPTree::MCFDfcts )

/*--------------------------------------------------------------------------*/
/*------------- METHODS FOR ADDING / REMOVING / CHANGING DATA --------------*/
/*--------------------------------------------------------------------------*/
/*----- Changing the costs, deficits and upper capacities of the (MCF) -----*/
/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::ChgCosts( cCRow NCost , cIndex_Set nms , cIndex strt , Index stp )
{
 if( nms ) {
  while( *nms < strt ) {
   nms++;
   NCost++;
   }

  if( DirSPT )
   for( Index h ; ( h = *(nms++) ) < stp ; )
    FS[ DictM1[ h ] ].Cst = *(NCost++);
  else
   for( Index h ; ( h = *(nms++) ) < stp ; ) {
    Index_Set tDM1 = DictM1 + 2 * h;
    cCNumber Ci = *(NCost++);
    FS[ *(tDM1++) ].Cst = Ci;
    FS[ *tDM1 ].Cst = Ci;
    }
  }
 else {
  if( stp > m )
   stp = m;

  cIndex_Set tDM1 = DictM1 + strt;
  if( DirSPT )
   for( Index i = stp - strt ; i-- ; )
    FS[ *(tDM1++) ].Cst = *(NCost++);
  else
   for( Index i = stp - strt ; i-- ; ) {
    cCNumber Ci = *(NCost++);
    FS[ *(tDM1++) ].Cst = Ci;
    FS[ *(tDM1++) ].Cst = Ci;
    }
  }

 status = MCFClass::kUnSolved;

 }  // end( SPTree::ChgCosts( some / all ) )

/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::ChgCost( Index arc , cCNumber NCost )
{
 if( DirSPT )
  FS[ DictM1[ arc ] ].Cst = NCost;
 else {
  arc *= 2;
  FS[ DictM1[ arc++ ] ].Cst = NCost;
  FS[ DictM1[ arc ] ].Cst = NCost;
  }

 status = MCFClass::kUnSolved;

 }  // end( SPTree::ChgCosts( one ) )

/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::ChgDfcts( cFRow NDfct , cIndex_Set nms , cIndex strt , Index stp )
{
 assert( false );  // not ready yet

 }  // end( SPTree::ChgDfcts( some / all ) )

/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::ChgDfct( Index nod , cFNumber NDfct )
{
 assert( false );  // not ready yet

 }  // end( SPTree::ChgDfcts( one ) )

/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::ChgUCaps( cFRow NCap , cIndex_Set nms , cIndex strt , Index stp )
{
 assert( false );
 }

/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::ChgUCap( Index arc , cFNumber NCap )
{
 assert( false );
 }

/*--------------------------------------------------------------------------*/
/*--------------- Modifying the structure of the graph ---------------------*/
/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::CloseArc( cIndex name )
{
 #if( DYNMC_MCF_SPT )
  if( ! DirSPT )
   throw( MCFException( "SPTree::CloseArc() not allowed if DirSPT == false" )
	  );

  cIndex pos = DictM1[ name ];     // current position of arc name
  cIndex nde = Stn( name , pos );  // start node of arc name
  cIndex npos = StrtFS[ nde ] + (--LenFS[ nde ]);  // new position
                                                   // for arc name
  if( npos != pos ) {
   cIndex nname = Dict[ npos ];             // name of the arc that is
                                            // currently in position pos
   Swap( FS[ pos ].Cst , FS[ npos ].Cst );  // now the arc 'name', curently in
   Swap( FS[ pos ].Nde , FS[ npos ].Nde );  // position 'pos' in FS[], is
   Dict[ pos ] = nname;                     // swapped with the arc 'nname'
   Dict[ npos ] = name;                     // currently in position 'npos'
   DictM1[ nname ] = pos;
   DictM1[ name ] = npos;
   }

  status = MCFClass::kUnSolved;
 #else
  throw(
   MCFException( "SPTree::CloseArc() not implemented if DYNMC_MCF_SPT == 0" )
	);
 #endif

 }  // end( SPTree::CloseArc )

/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::DelNode( cIndex name )
{
 throw( MCFException( "SPTree::DelNode() not implemented yet" ) );
 }

/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::OpenArc( cIndex name )
{
 #if( DYNMC_MCF_SPT )
  if( ! DirSPT )
   throw( MCFException( "SPTree::OpenArc() not allowed if DirSPT == false" )
	  );

  cIndex pos = DictM1[ name ];     // current position of arc name
  cIndex nde = Stn( name , pos );  // start node of arc name
  cIndex npos = StrtFS[ nde ] + (LenFS[ nde ]++);  // new position
                                                   // for arc name
  if( npos != pos ) {
   cIndex nname = Dict[ npos ];             // name of the arc that is
                                            // currently in position pos
   Swap( FS[ pos ].Cst , FS[ npos ].Cst );  // now the arc 'name', curently
   Swap( FS[ pos ].Nde , FS[ npos ].Nde );  // in position 'pos' in FS[],
   Dict[ pos ] = nname;                     // is swapped with the arc 'nname'
   Dict[ npos ] = name;                     // currently in position 'npos'
   DictM1[ nname ] = pos;
   DictM1[ name ] = npos;
   }

  status = MCFClass::kUnSolved;
 #else
  throw(
   MCFException( "SPTree::CloseArc() not implemented if DYNMC_MCF_SPT == 0" )
   );
 #endif

 }  // end( SPTree::OpenArc )

/*--------------------------------------------------------------------------*/

MCFClass::Index SPTree_Dijkstra::AddNode( cFNumber aDfct )
{
 throw( MCFException( "SPTree::AddNode() not implemented yet" ) );

 return( Inf<Index>() );
 }

/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::ChangeArc( cIndex name , cIndex nSS , cIndex nEN )
{
 #if( DYNMC_MCF_SPT )
  if( nSS < Inf<Index>() )
   throw( MCFException( "SPTree::ChangeArc() currently requires nSS == INF"
			) );

  if( nEN < Inf<Index>() ) {
   FS[ DictM1[ name ] ].Nde = nEN + USENAME0;
   status = MCFClass::kUnSolved;
   }
 #else
  throw(
   MCFException( "SPTree::ChangeArc() not implemented if DYNMC_MCF_SPT == 0" )
	);
 #endif

 }  // end( SPTree::ChangeArc )

/*--------------------------------------------------------------------------*/

MCFClass::Index SPTree_Dijkstra::AddArc( cIndex Start , cIndex End , cFNumber aU ,
				cCNumber aC )
{
 #if( DYNMC_MCF_SPT )
  Index nde = Start + USENAME0;
  Index pos = StrtFS[ nde ] + LenFS[ nde ];

  if( pos < StrtFS[ nde + 1 ] ) {  // there is already room in FS( Start )
   LenFS[ nde ]++;
   FS[ pos ].Cst = aC;
   FS[ pos ].Nde = End + USENAME0;
   pos = Dict[ pos ];
   }
  else               // have to make space in FS( Start )
   throw( MCFException( "SPTree::AddArc(): feature not implemented yet" ) );

  status = MCFClass::kUnSolved;

  return( pos );
 #else
  throw(
   MCFException( "SPTree::AddArc() not implemented if DYNMC_MCF_SPT == 0" )
   );

  return( Inf<Index>() );
 #endif

 }  // end( SPTree::AddArc )

/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::DelArc( cIndex name )
{
 SPTree_Dijkstra::CloseArc( name );  // limited implementation
 }

/*--------------------------------------------------------------------------*/
/*------------------------ SPECIALIZED INTERFACE ---------------------------*/
/*--------------------------------------------------------------------------*/

void SPTree_Dijkstra::ShortestPathTree( void )
{
 // initialize the data structures- - - - - - - - - - - - - - - - - - - - - -

 if( status )
  Initialize();

 // main cycle: repeat until Q is nonempty (or Dest is reached) - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 Index mi;

 if( MaxIter ) {  // iter check enabled - - - - - - - - - - - - - - - - - - -
  int Iter = 0;

  if( MCFt && MaxTime )  // case I: all checks enabled
   while( ( mi = ExtractQ() ) ) {
    //#if( LABEL_SETTING )
     if( mi == Dest )
      break;
    //#endif

    ScanFS( mi );

    if( ++Iter >  MaxIter ) {  // iterations limit
     status = kStopped;
     break;
     }

    if( MCFt->Read() > MaxTime ) {  // time limit
     status = kStopped;
     break;
     }
    }    // end while( Q not empty )
  else                   // case II: only iter check enabled
   while( ( mi = ExtractQ() ) ) {
    #if( LABEL_SETTING )
     if( mi == Dest )
      break;
    #endif

    ScanFS( mi );

    if( ++Iter >  MaxIter ) {  // iterations limit
     status = kStopped;
     break;
     }
    }    // end while( Q not empty )
   }
 else  // iter check disabled - - - - - - - - - - - - - - - - - - - - - - - -
  if( MCFt && MaxTime )  // case III: only time check enabled
   while( ( mi = ExtractQ() ) ) {
    //#if( LABEL_SETTING )
     if( mi == Dest )
      break;
    //#endif

    ScanFS( mi );

    if( MCFt->Read() > MaxTime ) {  // time limit
     status = kStopped;
     break;
     }
    }    // end while( Q not empty )
  else                   // case IV: no check enabled
   while( ( mi = ExtractQ() ) ) {
    #if( LABEL_SETTING )
     if( mi == Dest )
      break;
    #endif

    ScanFS( mi );
    }    // end while( Q not empty )

 // end main cycle: Q is empty or Dest is reached - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 //#if( LABEL_SETTING )
  if( ! Reached( Dest ) ) {
   status = MCFClass::kUnfeasible;
   FO = Inf<FONumber>();
   }
 //#else
 // for( FO = 0 , mi = NDsts ; mi-- ; ) {
 //  Index h = DstBse[ mi ];
 //  if( Pi[ h ] < Inf<CNumber>() )
 //   FO += B[ mi ] * Pi[ h ];
 //  else {
 //   status = MCFClass::kUnfeasible;
 //   FO = Inf<FONumber>();
 //   break;
 //   }
 //  }
 //#endif

 }  // end( SPTree::ShortestPathTree )

/*--------------------------------------------------------------------------*/

inline void SPTree_Dijkstra::MCFGetX( Index ND , cIndex_Set DB , FRow F ,
			     Index_Set nms , cIndex strt , Index stp )
{
 if( stp > m )
  stp = m;

 #if( ! ORDRD_NMS )
  assert( ( ! strt ) && ( stp == m ) );
  // restricting to a subinterval is not yet supported for unordered names
 #endif

 // if necessary, compute ArcPrd[]- - - - - - - - - - - - - - - - - - - - - -

 CalcArcP();

 // if necessary, initialize the flow to zero - - - - - - - - - - - - - - - -

 #if( ! ORDRD_NMS )
  if( ! nms )
 #endif
   for( FRow tF = F + m ; tF > F ; )
    *(--tF) = 0;

 if( ND == 1 )  // just climb back the (unique) path- - - - - - - - - - - - -
 {              //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Index j = *DB;
  cFNumber bi = Flow;

  #if( ! ORDRD_NMS )
   if( nms ) {  // "sparse" flow
    for( Index h ; ( h = NdePrd[ j ] ) ; j = h ) {
     *(F++) = bi;
     *(nms++) = ArcPrd[ j ];
     }

    *nms = Inf<Index>();
    }
   else  // "dense" flow
  #endif
    for( Index h ; ( h = NdePrd[ j ] ) ; j = h )
     F[ ArcPrd[ j ] ] = bi;
  }
 else  // compute a bottom-up topological sort of the minimal subtree - - - -
 {     // containing all the dests, and having only dests as leaves:- - - - -
       // Stack is here used as an array pointer implementation of the- - - -
       // list containing the sort- - - - - - - - - - - - - - - - - - - - - -
       // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  Index_Set tS = Stack + n;  // Stack[ i ] is the successor of node
  for( ; tS > Stack ; )               // i in the topological sort, and
   *(tS--) = 0;                       // Stack[ i ] == 0 <==> i has not been
                                      // reached yet

  Index hd = Origin;  // Origin is the last node of the list, so
  Stack[ hd ] = hd;   // mark it as "visited" (nonzero) and without
  Index i = ND;       // a successor (Stack[ i ] == i)

  for( ; i-- ; )  // for any dest climb up the tree until an already reached
  {               // node is found, keeping a temporary list of the nodes
   Index hd1 = DB[ i ];
   if( ! Stack[ hd1 ] ) {  // if the node has not already been found
    Index tl = hd1;        // hd1 and tl are the head and tail of
    Index j;               // the temporary list

    while( ! Stack[ j = NdePrd[ tl ] ] ) {
     Stack[ tl ] = j;
     tl = j;
     }

    Stack[ tl ] = hd;             // link it to the main list
    hd = hd1;
    }
   }

  // now, Stack[] contains the bottom-up topological sort, and the flow can
  // be constructed by just visiting the (sub)tree

  Stack[ Origin ] = 0;  // Origin (the last node) has no successor

  #if( ! ORDRD_NMS )
   if( nms ) {  // "sparse" flow- - - - - - - - - - - - - - - - - - - - - - -
    tS = Stack + n;  // use the second half of Stack to compute the position
    i = hd;          // in the "sparse" flow vector of ArcPrd[] of each node
    Index h = 0;
    do {
     nms[ h ] = ArcPrd[ hd ];
     F[ h ] = 0;                           // meanwhile, initialize F[] to 0
     tS[ i ] = h++;
     } while( ( i = Stack[ i ] ) );        // Origin gets the last position,
                                           // as ArcPrd[ Origin ] == INF

    for( i = ND ; i-- ; )                  // initialize F for the dest nodes
     F[ tS[ DB[ i ] ] ] = B[ i ];

    for( h = 0 ; ( i = Stack[ hd ] ) ; ) {  // now climb up the list
     F[ tS[ NdePrd[ hd ] ] ] += F[ h++ ];  // note that F[ tS[ Origin ] ] will
     hd = i;                               // contain Flow, but it is ignored
     }
    }
   else  // "dense" flow- - - - - - - - - - - - - - - - - - - - - - - - - - -
  #endif
   {
    // for each node i, F[ ArcPrd[ i ] ] will contain the total flow request
    // at i; when i is a destination, it is initialized to B[ i ] so that it
    // is ready for the final bottom-up visit

    for( i = ND ; i-- ; )
     F[ ArcPrd[ DB[ i ] ] ] = B[ i ];

    while( ( i = Stack[ hd ] ) ) {   // now climb up the list
     Index tA = ArcPrd[ NdePrd[ hd ] ];

     if( tA < Inf<Index>() )         // avoid to write in ArcPrd[ Origin ]
      F[ tA ] += F[ ArcPrd[ hd ] ];

     hd = i;
     }
    }
  }   // end else( many dests ) - - - - - - - - - - - - - - - - - - - - - - -
      //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // if necessary, turn the "dense" solution into a "sparse" one - - - - - - -

 #if( ORDRD_NMS )
  if( nms ) {
   cFRow tF = F + strt;
   for( Index i = strt ; i < stp ; i++ ) {
    cFNumber ttF = *(tF++);
    if( GTZ( ttF , EpsFlw ) ) {
     *(F++) = ttF;
     *(nms++) = i;
     }
    }

   *nms = Inf<Index>();
   }
 #endif

 }   // end( SPTree::MCFGetX( dest subset ) )

/*--------------------------------------------------------------------------*/

MCFClass::FONumber SPTree_Dijkstra::MCFGetFO( Index ND , cIndex_Set DB )
{
 FONumber tFO = 0;
 for( Index i = ND ; i-- ; )
  tFO += B[ i ] * Pi[ DB[ i ] ];

 return( tFO );

 }  // end( SPTree::MCFGetFO( dest subset ) )

/*--------------------------------------------------------------------------*/

//Johannes Sommer
/*
inline MCFClass::cIndex_Set SPTree::ArcPredecessors( void )
{
 CalcArcP();

 return( ArcPrd );
 }
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ DESTRUCTOR --------------------------------*/
/*--------------------------------------------------------------------------*/

SPTree_Dijkstra::~SPTree_Dijkstra()
{
 if( ! --InstCntr ) {  // deallocating static members - - - - - - - - - - - -
  maxnmax = 0;

  delete[] Stack;
  Stack = NULL;
  }

 MemDeAlloc();  // deallocate all the rest- - - - - - - - - - - - - - - - - -

 }  // end( ~SPTree )

/*--------------------------------------------------------------------------*/
/*--------------------------- PRIVATE METHODS ------------------------------*/
/*--------------------------------------------------------------------------*/

inline void SPTree_Dijkstra::Initialize( void )
{
 status = MCFClass::kOK;
 FO = Inf<SPTree_Dijkstra::FONumber>();

 CRow tPi = Pi + n;
 Index_Set tA = ArcPrd + n;
 for( Index_Set tQ = Q + n ; tQ > Q ; ) {
  *(tPi--) = Inf<CNumber>();
  *(tQ--) = Inf<Index>();
  *(tA--) = cFS;  // ArcPrd[ i ] = Dict[ ArcPrd[ i ] ] in CalcArcP(), and
                  // Dict[ cFS ] == Inf<Index>()
  }

 NdePrd[ Origin ] = 0;    // Origin has no predecessor, ..
 Pi[ Origin ] = 0;        // .. its distance from Origin is 0, ...
 Q[ Origin ] = 0;         // .. and it is in Q

 ReadyArcP = false;

 //#if( SPT_ALGRTM <= 3 )
 // *Q = tail = Origin;
 //#else
 // *H = Origin;
 // tail = 1;
 //#endif

 //Johannes Sommer
 //SPT_ALGRTM = 3, LABEL_SETTING = 1
 *Q = tail = Origin;

 }  // end( Initialize )

/*--------------------------------------------------------------------------*/

inline void SPTree_Dijkstra::ScanFS( cIndex mi )
{
 cCNumber pmi = Pi[ mi ];
 FrwdStr FSj = FS + StrtFS[ mi ];
 for( Index h = LenFS( mi ) ; h-- ; FSj++ ) {
  CNumber dist = (*FSj).Cst;
  #if( SAME_GRPH_SPT && ( ! DYNMC_MCF_SPT ) )
   if( dist == Inf<CNumber>() )
    continue;
  #endif

  dist += pmi;
  cIndex tnde = (*FSj).Nde;
  if( GT( Pi[ tnde ] , dist , EpsCst ) ) {  // can decrease Pi[ tnde ]
   //#if( SPT_ALGRTM <= 3 )
   // if( Q[ tnde ] == Inf<Index>() )  // tnde is not already in Q
   //#endif
   //Johannes Sommer
   //SPT_ALGRTM = 3, LABEL_SETTING = 1
	if( Q[ tnde ] == Inf<Index>() )
    //#if( SPT_ALGRTM == 1 )
    // InsertQ( tnde , Pi[ tnde ] );
    //#else
     InsertQ( tnde , dist );
    //#endif

   NdePrd[ tnde ] = mi;
   ArcPrd[ tnde ] = ( FSj - FS );
   Pi[ tnde ] = dist;

   }  // end if( dist of tnde decreased )
  }  // end for( h - scanning FS[ min ] )
 }  // end( ScanFS )

/*--------------------------------------------------------------------------*/

inline MCFClass::Index SPTree_Dijkstra::ExtractQ( void )
{
 Index mi;

 //#if( SPT_ALGRTM <= 1 )  // - - - - - - - - - - - - - - - - - - - - - - - - -

 // mi = *Q;
 // *Q = Q[ mi ];
 // if( tail == mi )
 //  tail = 0;

 //#elif( SPT_ALGRTM == 2 )  // - - - - - - - - - - - - - - - - - - - - - - - -

 //#elif( SPT_ALGRTM == 3 )  // - - - - - - - - - - - - - - - - - - - - - - - -

  // here, tmp is the position of the predecessor of the node with
  // smallest label, i.e. Q[ mi ] = node with smallest label

  Index tmp = 0;
  CNumber pmi = Pi[ mi = *Q ];

  while( mi ) {
   Index nxt = Q[ mi ];
   CNumber pt = Pi[ nxt ];

   if( pt < pmi ) {
    pmi = pt;
    tmp = mi;
    }

   mi = nxt;
   }

  mi = Q[ tmp ];
  if( mi == tail )
   tail = tmp;

  Q[ tmp ] = Q[ mi ];

 //#else  // ( SPT_ALGRTM == 4 )- - - - - - - - - - - - - - - - - - - - - - - -

 // if( tail ) {
 //  mi = *H;

 //  if( --tail ) {
 //   Index j = *H = H[ tail ];
 //   CNumber Pii = Pi[ j ];
 //   Index i = 0;            // i = current position in the heap
 //   Index h;                // h = LeftSon( i )

 //   while( ( h = HeapCard * i + 1 ) < tail ) {
 //    CNumber Pih = Pi[ H[ h ] ];

 //    #if( HeapCard == 2 )  // binary heap - - - - - - - - - - - - - - - - - -

 //     Index k = h + 1;
 //     if( k < tail ) {
 //      CNumber Pik = Pi[ H[ k ] ];
 //      if( Pik < Pih ) {
 //       Pih = Pik;
 //       h = k;
 //       }
 //      }
 //    #else  // generic c-ary heap - - - - - - - - - - - - - - - - - - - - - -

 //     Index th = min( Index( h + HeapCard ) , tail );
 //     for( Index ls = h ; --th > ls ; ) {
 //      CNumber Pith = Pi[ H[ th ] ];
 //      if( Pith < Pih ) {
 //       Pih = Pith;
 //       h = th;
 //       }
 //      }
 //    #endif  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 //    if( Pih < Pii ) {  // move down
 //     Q[ H[ i ] = H[ h ] ] = i;
 //     i = h;
 //     }
 //    else
 //     break;

 //    }  // end while( not a leaf )

 //   H[ i ] = j;
 //   Q[ j ] = i;

 //   }   // end if( tail )
 //  }
 // else
 //  mi = 0;

 //#endif  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( mi )
  Q[ mi ] = Inf<Index>();

 return( mi );

 }  // end( ExtractQ )

/*--------------------------------------------------------------------------*/

inline void SPTree_Dijkstra::InsertQ( cIndex j , cCNumber label )
{
 //#if( ( SPT_ALGRTM == 0 ) || ( SPT_ALGRTM == 3 ) )  //- - - - - - - - - - - -

  Q[ tail ] = j;
  Q[ tail = j ] = 0;

 //#elif( SPT_ALGRTM == 1 )  // - - - - - - - - - - - - - - - - - - - - - - - -

 // if( label == Inf<CNumber>() ) {
 //  Q[ tail ] = j;
 //  Q[ tail = j ] = 0;
 //  }
 // else {
 //  Q[ j ] = *Q;
 //  *Q = j;

 //  if( ! tail )
 //   tail = j;
 //  }

 //#elif( SPT_ALGRTM == 2 )  // - - - - - - - - - - - - - - - - - - - - - - - -

 //#else  // ( SPT_ALGRTM == 4 )- - - - - - - - - - - - - - - - - - - - - - - -

 // Index i = Q[ j ];

 // if( i == Inf<Index>() )
 //  i = tail++;

 // while( i ) {  // node labels only decrease, hence a node can at most go up
 //  Index h = ( i - 1 ) / HeapCard;
 //  Index hh = H[ h ];

 //  if( Pi[ hh ] <= label )
 //   break;

 //  Q[ H[ i ] = hh ] = i;
 //  i = h;
 //  }

 // H[ i ] = j;
 // Q[ j ] = i;

 //#endif  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 }  // end( InsertQ )

/*--------------------------------------------------------------------------*/

inline void SPTree_Dijkstra::CalcArcP( void )
{
 if( ! ReadyArcP ) {
  for( Index_Set tAP = ArcPrd + n ; tAP > ArcPrd ; tAP-- )
   *tAP = Dict[ *tAP ];

  ReadyArcP = true;
  }
 }  // end( CalcArcP )

/*--------------------------------------------------------------------------*/

#if( ! SPT_STRTN )

inline MCFClass::Index SPTree_Dijkstra::Startn( cIndex What )
{
 // we search for:
 // - What, if it is in StrtFS[], or
 // - the index of the last entry in StrtFS[] that is < What

 assert( false );  // it does not work: if there are several nodes s.t.
                   // StrtFS[ i ] == What (nodes with empty FS[] except
                   // for the last), it should return the last one
                   // rather than just any one

 Index Strt = 1;
 for( Index Stop = n + 1 ; Strt < Stop ; ) {
  // pivot i = ceil( ( Start + Stop ) / 2 )
  Index i = ( Strt + Stop + 1 ) / 2;

  if( StrtFS[ i ] > What )
   Stop = i - 1;
  else {
   Strt = i;

   if( StrtFS[ i ] == What )
    break;
   }
  }

 return( Strt );

 }  // end( Startn )

#endif

/*--------------------------------------------------------------------------*/

inline void SPTree_Dijkstra::MemAlloc( void )
{
 cFS = DirSPT ? mmax : 2 * mmax;

 #if( SPT_STRTN )
  #if( SAME_GRPH_SPT )
   if( ! Startn )  // allocating the start node information - - - - - - - - -
  #endif
   {
    Startn = new Index[ mmax ];
    #if( SAME_GRPH_SPT )
     *Startn = 0;  // 0 is not a feasible value for a *Startn; this tells
                   // that Startn has not been initialized yet
    #endif
    }
 #endif

 #if( SAME_GRPH_SPT && ( ! DYNMC_MCF_SPT ) )
  if( ! StrtFS )  // allocating FS-related data structures- - - - - - - - - -
 #endif
  {
   StrtFS = new Index[ nmax + 1 ];
   #if( SAME_GRPH_SPT && ( ! DYNMC_MCF_SPT ) )
    *StrtFS = Inf<Index>();  // INF is not a feasible value for StrtFS[ 1 ];
                      // this tells that the FS has not been initialized yet
   #endif
   StrtFS--;
   #if( DYNMC_MCF_SPT )
    LenFS = new Index[ nmax ];
    LenFS--;
   #endif

   Dict   = new Index[ cFS + 1 ];
   DictM1 = new Index[ cFS ];

   Dict[ cFS ] = Inf<Index>();  // used in CalcArcP() to set to INF
                                // the arc predecessor of the Origin
   }

 // allocating the remaining (always "local") memory- - - - - - - - - - - - -

 //#if( SPT_ALGRTM > 3 )
 // H = new Index[ nmax - 1 ];
 //#endif

 Q      = new Index[ nmax + 1 ];
 FS     = new FSElmnt[ cFS ];
 Pi     = new CNumber[ nmax + 1 ];
 NdePrd = new Index[ nmax ];
 ArcPrd = new Index[ nmax ];
 DstBse = new Index[ nmax - 1 ];
 B      = new FNumber[ nmax - 1 ];

 *Pi = Inf<CNumber>();
 NdePrd--;
 ArcPrd--;

 // allocating temporaries- - - - - - - - - - - - - - - - - - - - - - - - - -

 if( maxnmax < nmax ) {
  maxnmax = nmax;  // keep the max n. of nodes updated
  delete[] Stack;  // the stack has been allocated for a smaller graph:
  Stack = NULL;    // ensure it will be re-allocated
  }

 if( ! Stack )
  Stack = new Index[ 2 * maxnmax + 1 ];

 }  // end( MemAlloc )

/*--------------------------------------------------------------------------*/

inline void SPTree_Dijkstra::MemDeAlloc( void )
{
 if( mmax && nmax ) {
  delete[] B;
  delete[] DstBse;
  delete[] ++ArcPrd;
  delete[] ++NdePrd;
  delete[] Pi;
  delete[] FS;
  delete[] Q;

  #if( SPT_ALGRTM > 3 )
   delete[] H;
  #endif
  }

 #if( SAME_GRPH_SPT && ( ! DYNMC_MCF_SPT ) )
  if( ! InstCntr )
 #else
  if( mmax && nmax )
 #endif
  {
   delete[] DictM1;
   delete[] Dict;

   #if( DYNMC_MCF_SPT )
    delete[] ++LenFS;
   #endif

   delete[] ++StrtFS;
   StrtFS = NULL;
   }

 #if( SPT_STRTN )
  #if( SAME_GRPH_SPT && DYNMC_MCF_SPT )
   if( ! InstCntr )
  #else
   if( mmax && nmax )
  #endif
   {
    delete[] Startn;
    Startn = NULL;
    }
 #endif

 }  // end( MemDeAlloc )

/*--------------------------------------------------------------------------*/
/*--------------------------- STATIC MEMBERS -------------------------------*/
/*--------------------------------------------------------------------------*/

MCFClass::Index       SPTree_Dijkstra::InstCntr = 0;
MCFClass::Index       SPTree_Dijkstra::maxnmax = 0;
MCFClass::Index_Set   SPTree_Dijkstra::Stack = NULL;

#if( SPT_STRTN && SAME_GRPH_SPT )
 MCFClass::Index_Set  SPTree_Dijkstra::Startn = NULL;
#endif

#if( SAME_GRPH_SPT && ( ! DYNMC_MCF_SPT ) )
 MCFClass::Index_Set  SPTree_Dijkstra::StrtFS = NULL;

 MCFClass::Index_Set  SPTree_Dijkstra::Dict = NULL;
 MCFClass::Index_Set  SPTree_Dijkstra::DictM1 = NULL;
 MCFClass::bool       SPTree_Dijkstra::DirSPT = false;
#endif

/*--------------------------------------------------------------------------*/
/*-------------------------- End File SPTree_Dijkstra.C --------------------*/
/*--------------------------------------------------------------------------*/
