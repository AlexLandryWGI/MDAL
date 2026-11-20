/*
 MDAL - Mesh Data Abstraction Library (MIT License)
 Copyright (C) 2026 Alex Landry
*/
#include "gtest/gtest.h"

//mdal
#include "mdal.h"
#include "mdal_testutils.hpp"

TEST( MeshLandXMLTest, Driver )
{
  MDAL_DriverH driver = MDAL_driverFromName( "LandXML" );
  EXPECT_EQ( strcmp( MDAL_DR_filters( driver ), "*.xml" ), 0 );
  EXPECT_TRUE( MDAL_DR_meshLoadCapability( driver ) );
  EXPECT_FALSE( MDAL_DR_saveMeshCapability( driver ) );
  EXPECT_EQ( MDAL_DR_faceVerticesMaximumCount( driver ), 3 );
}

TEST( MeshLandXMLTest, WrongFiles )
{
  std::string path = test_file( "/landxml/no_surfaces.xml" );
  EXPECT_TRUE( std::string( MDAL_MeshNames( path.c_str() ) ).empty() );
  EXPECT_EQ( MDAL_LoadMesh( path.c_str() ), nullptr );
  EXPECT_EQ( MDAL_LastStatus(), MDAL_Status::Err_UnknownFormat );

  path = test_file( "/landxml/invalid_surfaces.xml" );
  const std::vector<std::string> names = {"invalid point", "unknown point", "not a triangle", "missing"};
  for ( const std::string &name : names )
  {
    const std::string uri = "LandXML:\"" + path + "\":" + name;
    EXPECT_EQ( MDAL_LoadMesh( uri.c_str() ), nullptr );
    EXPECT_EQ( MDAL_LastStatus(), MDAL_Status::Err_InvalidData );
  }
}

TEST( MeshLandXMLTest, Surfaces )
{
  const std::string path = test_file( "/landxml/surfaces.xml" );
  EXPECT_EQ( MDAL_MeshNames( path.c_str() ), "LandXML:\"" + path + "\":first;;LandXML:\"" + path + "\":second surface" );

  MDAL_MeshH m = MDAL_LoadMesh( path.c_str() );
  ASSERT_NE( m, nullptr );
  ASSERT_EQ( MDAL_LastStatus(), MDAL_Status::None );
  EXPECT_EQ( MDAL_M_driverName( m ), std::string( "LandXML" ) );
  EXPECT_EQ( MDAL_M_projection( m ), std::string( "EPSG:3007" ) );
  EXPECT_EQ( MDAL_M_faceVerticesMaximumCount( m ), 3 );

  // points are stored as northing easting elevation
  ASSERT_EQ( MDAL_M_vertexCount( m ), 5 );
  EXPECT_DOUBLE_EQ( getVertexXCoordinatesAt( m, 4 ), 1005 );
  EXPECT_DOUBLE_EQ( getVertexYCoordinatesAt( m, 4 ), 2020 );
  EXPECT_DOUBLE_EQ( getVertexZCoordinatesAt( m, 4 ), 14 );

  // faces refer to point ids, the invisible one is skipped
  ASSERT_EQ( MDAL_M_faceCount( m ), 2 );
  EXPECT_EQ( getFaceVerticesCountAt( m, 1 ), 3 );
  EXPECT_EQ( getFaceVerticesIndexAt( m, 1, 0 ), 0 );
  EXPECT_EQ( getFaceVerticesIndexAt( m, 1, 1 ), 2 );
  EXPECT_EQ( getFaceVerticesIndexAt( m, 1, 2 ), 3 );

  // Bed elevation dataset
  ASSERT_EQ( MDAL_M_datasetGroupCount( m ), 1 );
  MDAL_DatasetGroupH g = MDAL_M_datasetGroup( m, 0 );
  EXPECT_EQ( MDAL_G_name( g ), std::string( "Bed Elevation" ) );
  EXPECT_TRUE( MDAL_G_hasScalarData( g ) );
  EXPECT_EQ( MDAL_G_dataLocation( g ), MDAL_DataLocation::DataOnVertices );
  ASSERT_EQ( MDAL_G_datasetCount( g ), 1 );
  MDAL_DatasetH ds = MDAL_G_dataset( g, 0 );
  EXPECT_TRUE( MDAL_D_isValid( ds ) );
  ASSERT_EQ( MDAL_D_valueCount( ds ), 5 );
  EXPECT_DOUBLE_EQ( getValue( ds, 4 ), 14 );
  MDAL_CloseMesh( m );

  const std::string uri = "LandXML:\"" + path + "\":second surface";
  m = MDAL_LoadMesh( uri.c_str() );
  ASSERT_NE( m, nullptr );
  EXPECT_EQ( MDAL_M_vertexCount( m ), 3 );
  EXPECT_EQ( MDAL_M_faceCount( m ), 1 );
  MDAL_CloseMesh( m );

  // without EPSG code the WKT definition is used
  m = MDAL_LoadMesh( test_file( "/landxml/wkt.xml" ).c_str() );
  ASSERT_NE( m, nullptr );
  EXPECT_EQ( MDAL_M_projection( m ), std::string( "LOCAL_CS[\"site\",UNIT[\"metre\",1]]" ) );
  MDAL_CloseMesh( m );
}

int main( int argc, char **argv )
{
  testing::InitGoogleTest( &argc, argv );
  init_test();
  int ret = RUN_ALL_TESTS();
  finalize_test();
  return ret;
}
