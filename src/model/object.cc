#ifndef SCHEDULER_OBJECT
#define SCHEDULER_OBJECT

#include <iostream>
class Object {
  public:
    Object(int id, double ra, double dec, unsigned int priority,
           unsigned int observationTime) {
        this->Id = id;
        this->Ra = ra;
        this->Dec = dec;
        this->Priority = priority;
        this->ObservationTime = observationTime;
    }

    int GetId() const { return this->Id; }

    double GetRa() const { return this->Ra; }

    double GetDec() const { return this->Dec; }

    unsigned int GetPriority() const { return this->Priority; }

    unsigned int GetObservationTime() const { return this->ObservationTime; }

    void Print() const {
        std::cout << "Id: " << this->Id << std::endl
                  << "RA: " << this->GetRa() << std::endl
                  << "DEC: " << this->GetDec() << std::endl
                  << "Priority: " << this->Priority << std::endl
                  << "Observation Time: " << this->ObservationTime << std::endl
                  << std::endl;
    }

    bool operator<(const Object &object) const {
        return this->Priority < object.Priority &&
               this->GetObservationTime() < object.GetObservationTime();
    }

  private:
    int Id;
    double Ra;
    double Dec;
    unsigned int Priority;
    unsigned int ObservationTime;
};

#endif
