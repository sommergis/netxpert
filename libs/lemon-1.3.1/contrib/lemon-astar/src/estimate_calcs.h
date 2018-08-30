#ifndef ESTIMATE_CALCS
#define ESTIMATE_CALCS

  template<typename GR>
  class EstimateCalculator
  {
    typedef GR Graph;
    typedef typename Graph::Node Node;
    
    public:
    int operator()(const Node& u, const Node& v) const
    {
      return 0;
    }
  };
  
  template<typename GR>
  class EstimateCoordCalculator
  {
    typedef GR Graph;
    typedef typename Graph::Node Node;
    
    const Graph *G;
    int tId;
    int nodes;
    double *longitudes;
    double *latitudes;
    double *dists;
    
    public:
    EstimateCoordCalculator(const Graph &g, int t) : G(&g), tId(t) { readcoords(); calcDists(); }
    ~EstimateCoordCalculator()
    {
      if (longitudes) delete[] longitudes;
      if (latitudes) delete[] latitudes;
      if (dists) delete[] dists;
    }
    
    void readcoords()
    {
      std::ifstream iFile("USA-road-d.NY.co.txt");
      int node;
      double longitude, latitude;
      iFile >> nodes;
      longitudes = new double[nodes];
      latitudes = new double[nodes];
      
      for (int i = 0; i < nodes; ++i)
      {
        iFile >> node >> longitude >> latitude;
        longitudes[node] = longitude;
        latitudes[node] = latitude;
      }
    }
    
    void calcDists()
    {
      dists = new double[nodes];
      double dist;
      double x1,x2,y1,y2;
      x2 = longitudes[tId];
      y2 = latitudes[tId];
      
      for (int i = 0; i < nodes; ++i)
      {
        x1 = longitudes[i];
        y1 = latitudes[i];
        dist = (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1);
        //dists[i] = 0.7 * sqrt(dist);
        dists[i] = sqrt(dist);
      }
    }
    
    double operator()(const Node& u, const Node& v) const
    {
      if (G->id(v)==tId)return dists[G->id(u)];
      return 0.0;
    }
  };
  
  template<typename GR>
  class EstimateRTCoordCalculator
  {
    typedef GR Graph;
    typedef typename Graph::Node Node;
    
    const Graph *G;
    double x,y,x1,x2,y1,y2;
    double *longitudes;
    double *latitudes;
    
    public:
    EstimateRTCoordCalculator(const Graph &g) : G(&g) { x=y=-1; x1=x2=y1=y2=0; readcoords(); }
    ~EstimateRTCoordCalculator()
    {
      if (longitudes) delete[] longitudes;
      if (latitudes) delete[] latitudes;
    }
    
    void readcoords()
    {
      std::ifstream iFile("USA-road-d.NY.co.txt");
      //std::ifstream iFile("fullgraph.co");
      int node,nodes;
      double longitude, latitude;
      iFile >> nodes;
      longitudes = new double[nodes];
      latitudes = new double[nodes];
      
      for (int i = 0; i < nodes; ++i)
      {
        iFile >> node >> longitude >> latitude;
        longitudes[node] = longitude;
        latitudes[node] = latitude;
      }
    }
    
    double operator()(const Node& u, const Node& v) const
    {
      //return 0.7 * sqrt(pow((latitudes[G->id(v)]-latitudes[G->id(u)]),2) + pow((longitudes[G->id(v)]-longitudes[G->id(u)]),2));
      return sqrt(pow((latitudes[G->id(v)]-latitudes[G->id(u)]),2) + pow((longitudes[G->id(v)]-longitudes[G->id(u)]),2));
      //return 0.0;
    }
  };
  
  template<typename GR>
  class EstimatePreciseCalculator
  {
    typedef GR Graph;
    typedef typename Graph::Node Node;
    
    const Graph *G;
    int nodes;
    double *fwdDists;
    double *revDists;
    int sId;
    int tId;
    
    public:
    EstimatePreciseCalculator(const Graph &g) : G(&g) { readDists();}
    ~EstimatePreciseCalculator()
    {
      if (fwdDists) delete[] fwdDists;
      if (revDists) delete[] revDists;
    }
    
    void readDists()
    {
      std::ifstream iFile("precdists.txt");
      int node;
      double fwdDist, revDist;
      iFile >> nodes;
      fwdDists = new double[nodes];
      revDists = new double[nodes];
      
      for (int i = 0; i < nodes; ++i)
      {
        iFile >> node >> fwdDist >> revDist;
        fwdDists[node] = fwdDist;
        revDists[node] = revDist;
        if (!fwdDist) tId = node;
        if (!revDist) sId = node;
      }
    }
    
    double operator()(const Node& u, const Node& v) const
    {
      if (G->id(v)==tId) return fwdDists[G->id(u)];
      if (G->id(u)==sId) return revDists[G->id(v)];
      return 0.0;
    }
  };
#endif // ESTIMATE_CALCS
