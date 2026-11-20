/*
 MDAL - Mesh Data Abstraction Library (MIT License)
 Copyright (C) 2026 Alex Landry
*/

#ifndef MDAL_LANDXML_HPP
#define MDAL_LANDXML_HPP

#include "mdal_driver.hpp"
#include "mdal_data_model.hpp"
#include "mdal_memory_data_model.hpp"

namespace MDAL
{
  /**
   * TIN surfaces of LandXML files, see http://www.landxml.org/schema/LandXML-1.2/LandXML-1.2.xsd
   *
   * Each Surface element is a separate mesh. Points are stored as northing, easting
   * and elevation, faces refer to them by id and those flagged as invisible are skipped.
   */
  class DriverLandXML: public Driver
  {
    public:
      DriverLandXML();
      DriverLandXML *create() override;

      int faceVerticesMaximumCount() const override {return 3;}

      bool canReadMesh( const std::string &uri ) override;
      std::string buildUri( const std::string &meshFile ) override;
      std::unique_ptr< Mesh > load( const std::string &meshFile, const std::string &meshName = "" ) override;
  };

} // namespace MDAL
#endif //MDAL_LANDXML_HPP
