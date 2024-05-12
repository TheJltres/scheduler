#ifndef SCHEDULER_TELESCOPE_CONFIGURATION
#define SCHEDULER_TELESCOPE_CONFIGURATION

#include <climits>
#include <iostream>
#include <ostream>
#include <string>

class TelescopeConfiguration {
  public:
    TelescopeConfiguration(int id, double lat, double lon, std::string name,
              long observation_time) {
        this->Id = id;
        this->Name = name;
        this->Latitude = lat;
        this->Longitude = lon;
    }

    TelescopeConfiguration(int id, double lat, double lon) {
        this->Id = id;
        this->Name = "";
        this->Latitude = lat;
        this->Longitude = lon;
        this->ObservationTime = INT_MAX;
    }

    int GetId() const { return this->Id; }

    std::string GetName() const { return this->Name; }

    double GetLatitude() const { return this->Latitude; }

    double GetLongitude() const { return this->Longitude; }

    long GetObservationTime() const { return this->ObservationTime; }

    void Print() const {
        std::cout << "Id: " << this->Id << std::endl
            << "Name: " << this->Name << std::endl
            << "Latitude: " << this->Latitude << std::endl
            << "Longitude: " << this->Longitude << std::endl
            << "Observation Time: " << this->ObservationTime << std::endl
        << std::endl;
    }

    void GetConfiguration() const {

    }

  private:
    int Id;
    std::string Name;
    double Latitude;
    double Longitude;
    long ObservationTime;
};

#endif
