#include "networkbuilder.h"
using namespace netxpert;

NetworkBuilder::NetworkBuilder(Config& cnfg)
{
    //FIXED = 0 Kommastellen
    //FLOATING_SINGLE = 6 Kommastellen
    //FLOATING = 16 Kommastellen
    // Sollte FLOATING sein - sonst gibts evtl geometriefehler (Lücken beim CreateRouteGeometries())
    // Grund ist, dass SpatiaLite eine hohe Präzision hat und diese beim splitten von Linien natürlich auch hoch sein
    // muss.
    // Performance ist zu vernachlässigen, weil ja nur geringe Mengen an Geometrien eingelesen und verarbeitet werden
    // (nur die Kanten, die aufgebrochen werden)
    // --> Zusammengefügt werden die Kanten (Route) ja in der DB bei Spatialite (FGDB?).
	shared_ptr<PrecisionModel> pm (new PrecisionModel( geos::geom::PrecisionModel::FLOATING));

	// Initialize global factory with defined PrecisionModel
	// and a SRID of -1 (undefined).
	GEO_FACTORY = shared_ptr<GeometryFactory> ( new GeometryFactory( pm.get(), -1)); //SRID = -1

    NETXPERT_CNFG = cnfg;
    DBHELPER::Initialize(NETXPERT_CNFG);

}

void LoadData()
{
    LOGGER::LogInfo("Loading data from DB..");

}
