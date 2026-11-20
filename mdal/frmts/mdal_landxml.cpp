/*
 MDAL - Mesh Data Abstraction Library (MIT License)
 Copyright (C) 2026 Alex Landry
*/

#include <fstream>
#include <sstream>
#include <unordered_map>

#include "mdal_landxml.hpp"
#include "mdal_utils.hpp"
#include "mdal_logger.hpp"
#include "mdal_xml.hpp"

#define DRIVER_NAME "LandXML"

MDAL::DriverLandXML::DriverLandXML():
  Driver( DRIVER_NAME,
          "LandXML Surface",
          "*.xml",
          Capability::ReadMesh )
{}

MDAL::DriverLandXML *MDAL::DriverLandXML::create()
{
  return new DriverLandXML();
}

bool MDAL::DriverLandXML::canReadMesh( const std::string &uri )
{
  // the root element follows the XML declaration, so the first line is not enough
  std::ifstream in = MDAL::openInputFile( uri );
  char header[1024] = "";
  in.read( header, sizeof( header ) - 1 );
  return MDAL::contains( header, "<LandXML" );
}

// Returns the name elements of all the group children of the parent
static std::vector<xmlNodePtr> elements( const XMLFile &xml, xmlNodePtr parent, const std::string &group, const std::string &name )
{
  std::vector<xmlNodePtr> ret;
  for ( xmlNodePtr g = xml.getCheckChild( parent, group ); g != nullptr; g = xml.getCheckSibling( g, group, false ) )
    for ( xmlNodePtr e = xml.getCheckChild( g, name ); e != nullptr; e = xml.getCheckSibling( e, name, false ) )
      ret.push_back( e );
  return ret;
}

std::string MDAL::DriverLandXML::buildUri( const std::string &meshFile )
{
  std::vector<std::string> meshNames;
  try
  {
    XMLFile xml;
    xml.openFile( meshFile );
    for ( xmlNodePtr surface : elements( xml, xml.getCheckRoot( "LandXML" ), "Surfaces", "Surface" ) )
      meshNames.push_back( xml.attribute( surface, "name" ) );
  }
  catch ( MDAL::Error &err )
  {
    MDAL::Log::error( err, name() );
    return std::string();
  }
  return MDAL::buildAndMergeMeshUris( meshFile, meshNames, name() );
}

std::unique_ptr<MDAL::Mesh> MDAL::DriverLandXML::load( const std::string &meshFile, const std::string &meshName )
{
  MDAL::Log::resetLastStatus();

  try
  {
    XMLFile xml;
    xml.openFile( meshFile );

    xmlNodePtr definition = nullptr;
    for ( xmlNodePtr surface : elements( xml, xml.getCheckRoot( "LandXML" ), "Surfaces", "Surface" ) )
      if ( definition == nullptr && ( meshName.empty() || xml.attribute( surface, "name" ) == meshName ) )
        definition = xml.getCheckChild( surface, "Definition" );
    if ( definition == nullptr )
      throw MDAL::Error( MDAL_Status::Err_InvalidData, "No such mesh with name: " + meshName );

    Vertices vertices;
    std::unordered_map<size_t, size_t> vertexIDtoIndex;
    for ( xmlNodePtr point : elements( xml, definition, "Pnts", "P" ) )
    {
      Vertex vertex;
      std::stringstream coordinates( xml.content( point ) );
      if ( !( coordinates >> vertex.y >> vertex.x >> vertex.z ) ) // northing easting elevation
        throw MDAL::Error( MDAL_Status::Err_InvalidData, "Invalid point: " + xml.content( point ) );
      vertexIDtoIndex[xml.querySizeTAttribute( point, "id" )] = vertices.size();
      vertices.push_back( vertex );
    }

    Faces faces;
    for ( xmlNodePtr triangle : elements( xml, definition, "Faces", "F" ) )
    {
      if ( xml.checkAttribute( triangle, "i", "1" ) ) // invisible
        continue;

      Face face;
      std::stringstream ids( xml.content( triangle ) );
      size_t id;
      while ( ids >> id )
      {
        const auto it = vertexIDtoIndex.find( id );
        if ( it == vertexIDtoIndex.end() )
          throw MDAL::Error( MDAL_Status::Err_InvalidData, "Face refers to unknown point: " + xml.content( triangle ) );
        face.push_back( it->second );
      }
      if ( face.size() != 3 )
        throw MDAL::Error( MDAL_Status::Err_InvalidData, "Face is not a triangle: " + xml.content( triangle ) );
      faces.emplace_back( std::move( face ) );
    }

    std::unique_ptr< MemoryMesh > mesh( new MemoryMesh( DRIVER_NAME, 3, meshFile ) );
    mesh->setFaces( std::move( faces ) );
    mesh->setVertices( std::move( vertices ) );

    xmlNodePtr crs = xml.getCheckChild( xml.root(), "CoordinateSystem", false );
    const int epsg = crs != nullptr && xml.hasAttribute( crs, "epsgCode" ) ? MDAL::toInt( xml.attribute( crs, "epsgCode" ) ) : 0;
    if ( epsg != 0 )
      mesh->setSourceCrsFromEPSG( epsg );
    else if ( crs != nullptr && xml.hasAttribute( crs, "ogcWktCode" ) )
      mesh->setSourceCrsFromWKT( xml.attribute( crs, "ogcWktCode" ) );

    MDAL::addBedElevationDatasetGroup( mesh.get(), mesh->vertices() );
    return std::unique_ptr<Mesh>( mesh.release() );
  }
  catch ( MDAL::Error &err )
  {
    MDAL::Log::error( err, name() );
    return nullptr;
  }
}
