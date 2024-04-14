#include <cstdlib>
#include <iostream>
#include <ostream>
#include <stdlib.h>

#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "absl/strings/str_format.h"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"

class Telescope {
    public:
    Telescope(int id, std::string name, int64_t observation_time){
        this->id = id;
        this->name = name;
        this->observation_time = observation_time;
    }

    int GetId() const {
        return this->id;
    }

    std::string GetName() const {
        return this->name;
    }

    int64_t GetObservationTime() const {
        return this->observation_time;
    }

private:
    int id;
    std::string name;
    int64_t observation_time;
};

class Job {
public:
    Job(int id, int time) {
        this->id = id;
        this->time = time;
    }

    int GetId() const {
        return this->id;
    }

    int GetTime() const {
        return this->time;
    }

    std::string Log() const {
        return absl::StrFormat("Job %d %d", this->GetId(), this->GetTime());
    }

private:
    int id;
    int time;
};

namespace operations_research {
namespace sat {
void BasicExample() {
    std::cout << "Implementation of the Scheduder with OR-Tools" << std::endl;

    std::vector<Telescope> telescopes;
    telescopes.push_back(Telescope(0, "TFRM", 255));

    srand(time(0));
    const int num_jobs = 5;
    std::vector<Job> jobs;
    for (int i = 0; i < num_jobs; i++) {
        int rand_time = rand() % 100;

        Job job = Job(i, rand_time);
        jobs.push_back(job);

        std::cout << job.Log() << std::endl;
    }

    CpModelBuilder model;
    auto constant = model.NewFixedSizeIntervalVar(10, 3);

    std::map<std::tuple<int, int>, IntVar> assigned;
    std::vector<BoolVar> scheduler;
    std::vector<IntervalVar> intervals;
    for (Telescope t : telescopes) {
        IntVar objective = model.NewIntVar({0, t.GetObservationTime()})
            .WithName(absl::StrFormat("makespan_%d", t.GetId()));

        std::vector<IntVar> ends;
        std::vector<BoolVar> restrictions_global;
        for (Job j : jobs) {
            std::string suffix = absl::StrFormat("_%d_%d", j.GetId(), t.GetId());
            IntVar start = model.NewIntVar({0, t.GetObservationTime()})
                .WithName(std::string("job_start") + suffix);
            IntVar end = model.NewIntVar({0, t.GetObservationTime()})
                .WithName(std::string("job_end") + suffix);
            IntervalVar interval = model.NewIntervalVar(
                start,
                j.GetTime(),
                end
            ).WithName(std::string("job_interval") + suffix);
            auto key = std::make_tuple(t.GetId(), j.GetId());
            assigned[key] = start;
            intervals.push_back(interval);

            auto schedule = model.NewBoolVar()
                .WithName(absl::StrFormat("schedule_%d_%d", j.GetId(), t.GetId()));
            scheduler.push_back(schedule);

            ends.push_back(end);
        }

        model.AddMaxEquality(objective, ends);
        model.Minimize(objective);
    }

    model.AddAtMostOne(scheduler);
    model.AddNoOverlap(intervals);

    const CpSolverResponse response = Solve(model.Build());

    if (response.status() == CpSolverStatus::OPTIMAL ||
        response.status() == CpSolverStatus::FEASIBLE) {
        std::cout << "Solution found:" << std::endl;

        std::map<std::tuple<int, int>, int64_t> scheduled;
        for (Telescope t : telescopes) {
            for (Job j : jobs) {
                auto key = std::make_tuple(t.GetId(), j.GetId());
                auto val = SolutionIntegerValue(response, assigned[key]);
                scheduled[key] = val;
            }
        }

        for (const auto value : scheduled) {
            auto job_id = std::get<1>(value.first);
            std::cout << "Telescope: " << std::get<0>(value.first)
                << " Job: " << job_id
                << " Starts at: " << value.second
                << " until " << value.second + jobs[job_id].GetTime()
                << " - " << jobs[job_id].GetTime() << std::endl;
        }


        std::cout << "Optimal Schedule Length: " << response.objective_value() << std::endl;
    }
    else {
        std::cout << "No solution was found" << std::endl;
    }

    // Statistics.
    std::cout << std::endl;
    std::cout << "Statistics" << std::endl;
    std::cout << CpSolverResponseStats(response) << std::endl;
}
}
}

int main() {
    operations_research::sat::BasicExample();
    return EXIT_SUCCESS;
}
