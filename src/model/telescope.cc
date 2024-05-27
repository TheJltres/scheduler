#ifndef SCHEDULER_TELESCOPE
#define SCHEDULER_TELESCOPE

#include "./TelescopeLimits.cc"
#include "./angle.cc"
#include "object.cc"
#include <climits>
#include <iostream>
#include <ostream>
#include <string>
extern "C" {
#include "../include/libastro.h"
}

class Telescope {
  public:
    Telescope(int id, double lat, double lon, int altitude, std::string name,
              TelescopeLimits limits) {
        this->Id = id;
        this->Name = name;
        this->Latitude = lat;
        this->Longitude = lon;
        this->Altitude = altitude;
        this->Limits = limits;
    }

    Telescope(int id, double lat, double lon) {
        this->Id = id;
        this->Name = "";
        this->Latitude = lat;
        this->Longitude = lon;
        this->Limits = TelescopeLimits{};
    }

    int GetId() const { return this->Id; }

    std::string GetName() const { return this->Name; }

    double GetLatitude() const { return this->Latitude; }

    double GetLongitude() const { return this->Longitude; }

    int GetAltitude() const { return this->Altitude; }

    int GetObservationTime(Now now) const {
        double julian_twilight_dawn;
        double julian_twilight_dusk;
        int status;
        twilight_cir(
            &now,
            -17.5 * PI / 180,
            &julian_twilight_dawn,
            &julian_twilight_dusk,
            &status
        );

        return trunc((julian_twilight_dusk - julian_twilight_dawn) * 60 * 24);
    }

    void Print() const {
        std::cout << "Id: " << this->Id << std::endl
                  << "Name: " << this->Name << std::endl
                  << "Latitude: " << this->Latitude << std::endl
                  << "Longitude: " << this->Longitude << std::endl
                  << std::endl;
    }

    bool IsObjectVisible(double julian_date, Object object) const {
        double moon_lat, moon_lon, moon_rho, moon_msp, moon_mdp;

        // Moon separation
        moon(julian_date, &moon_lat, &moon_lon, &moon_rho, &moon_msp,
             &moon_mdp);

        auto separation = Angle::separation(moon_lat, moon_lon, object.GetDec(),
                                            object.GetRa())
                              .GetFactor();
        if (separation <= this->Limits.MinLunarDistance &&
            separation > -this->Limits.MinLunarDistance) {
            return false;
        }

        double diff_lat = this->GetLatitude() - object.GetDec();
        double diff_lon = this->GetLongitude() - object.GetRa();

        double lst;
        Now now;
        now.n_mjd = julian_date;
        now.n_lat = this->GetLatitude();
        now.n_lng = this->GetLongitude();
        now.n_temp = 15;
        now.n_dip = now.n_elev = now.n_tz = 0;
        now.n_pressure = 1010;
        now.n_epoch = J2000;

        now_lst(&now, &lst);
        auto lst_ra = lst - object.GetRa();
        if (lst_ra >= this->Limits.MountHA &&
            24.0 - lst_ra >= this->Limits.MountHA) {
            return false;
        }

        if (object.GetDec() >= this->Limits.MinDecSouth ||
            object.GetDec() <= -this->Limits.MinDecNord) {
            return false;
        }

        return true;
    }

  private:
    int Id;
    std::string Name;
    double Latitude;
    double Longitude;
    int Altitude;
    TelescopeLimits Limits;
};

#endif
