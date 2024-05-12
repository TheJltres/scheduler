#ifndef SCHEDULER_ANGLE
#define SCHEDULER_ANGLE

#include <cmath>

#include "./object.cc"
#include "../include/libastro.h"

class Angle {
    public:
    Angle(double rad, double fac) {
        this->Radians = rad;
        this->Factor = fac;
    }

    double GetRadians() const {
        return this->Radians;
    }

    double GetFactor() const {
        return this->Factor;
    }

    static Angle separation(Object self, Object other)
    {
        double slat = self.GetDec();
        double slon = self.GetRa();

        double olat = other.GetDec();
        double olon = other.GetRa();

        return separation(slat, slon, olat, olon);
    }

    static Angle separation(double slat, double slon, double olat, double olon)
    {
        /* rounding errors in the trigonometry might return >0 for this case */
        if ((slat == olat) && (slon == olon))
        {
            return Angle(0.0, raddeg(1));
        }

        double spy, cpy, px, qx, sqy, cqy, cosine;

        spy = sin (slat);
        cpy = cos (slat);
        px = slon;
        qx = olon;
        sqy = sin (olat);
        cqy = cos (olat);

        cosine = spy * sqy + cpy * cqy * cos(px-qx);
        if (cosine >= 1.0)  /* rounding sometimes makes this >1 */
        {
            return Angle(0.0, raddeg(1));
        }

        return Angle(acos(cosine), raddeg(1));
    }


private:
    double Radians;
    double Factor;
};

#endif
