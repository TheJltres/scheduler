#include <absl/base/log_severity.h>
#include <absl/log/globals.h>
#include <absl/log/log.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdlib.h>
#include <time.h>

#include "SchedulerConfig.h"

#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <unistd.h>
#include <vector>

#include "absl/strings/str_format.h"
#include "argh.h"
extern "C" {
#include "include/libastro.h"
}
#include "ini.h"

#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"

#include "./model/object.cc"
#include "./model/telescope.cc"

void Schedule(double julian_date, std::vector<Telescope> telescopes,
              std::vector<Object> objects) {
    using namespace operations_research::sat;
    std::cout << "Implementation of the Scheduder with OR-Tools" << std::endl;

    CpModelBuilder model;

    std::sort(objects.begin(), objects.end());

    std::map<std::tuple<int, int>, IntVar> assigned;
    std::vector<BoolVar> scheduler;
    std::vector<IntervalVar> intervals;
    for (Telescope telescope : telescopes) {
        Now now;
        now.n_mjd = julian_date;
        now.n_lat = telescope.GetLatitude() * PI / 180;
        now.n_lng = telescope.GetLongitude() * PI / 180;
        now.n_elev = telescope.GetAltitude();
        now.n_temp = 15;
        now.n_dip = now.n_tz = 0;
        now.n_pressure = 1010;
        now.n_epoch = J2000;

        double julian_twilight_dawn;
        double julian_twilight_dusk;
        int status;
        twilight_cir(&now, -17.5 * PI / 180, &julian_twilight_dawn,
                     &julian_twilight_dusk, &status);

        int total_observation_time =
            trunc((julian_twilight_dusk - julian_twilight_dawn) * 60 * 24);

        std::cout << julian_twilight_dawn << std::endl;
        std::cout << julian_twilight_dusk << std::endl;
        std::cout << total_observation_time << std::endl;

        IntVar makespan =
            model.NewIntVar({0, total_observation_time})
                .WithName(absl::StrFormat("makespan_%d", telescope.GetId()));
        std::vector<IntVar> ends;
        std::vector<BoolVar> restrictions_global;
        for (Object object : objects) {
            int visible_start = 0;
            auto time = julian_twilight_dusk;
            while (visible_start < total_observation_time &&
                   !telescope.IsObjectVisible(time, object)) {
                visible_start++;

                time += ((double)1 / (24 * 60));
            }

            if (visible_start == total_observation_time) {
                continue;
            }

            auto visible_end = visible_start;
            while (visible_end < total_observation_time &&
                   telescope.IsObjectVisible(time, object)) {
                visible_end++;

                time += ((double)1 / (24 * 60));
            }

            if (visible_end - visible_start < object.GetObservationTime()) {
                continue;
            }

            std::cout << "Object added: " << telescope.GetId() << " - "
                      << object.GetId() << " - " << visible_start << " - "
                      << visible_end << " - " << object.GetObservationTime()
                      << std::endl;

            std::string suffix =
                absl::StrFormat("_%d_%d", object.GetId(), telescope.GetId());
            IntVar start =
                model.NewIntVar({visible_start, visible_end})
                    .WithName(std::string("twilight_start") + suffix);
            IntVar end = model.NewIntVar({visible_start, visible_end})
                             .WithName(std::string("object_end") + suffix);
            IntervalVar interval =
                model.NewIntervalVar(start, object.GetObservationTime(), end)
                    .WithName(std::string("object_interval") + suffix);
            auto key = std::make_tuple(telescope.GetId(), object.GetId());
            assigned[key] = start;
            intervals.push_back(interval);

            auto schedule = model.NewBoolVar().WithName(absl::StrFormat(
                "schedule_%d_%d", telescope.GetId(), object.GetId()));
            scheduler.push_back(schedule);

            ends.push_back(end);
        }

        model.AddMaxEquality(makespan, ends);
        model.Minimize(makespan);
    }

    model.AddAtMostOne(scheduler);
    model.AddNoOverlap(intervals);

    const CpSolverResponse response = Solve(model.Build());

    if (response.status() == CpSolverStatus::OPTIMAL ||
        response.status() == CpSolverStatus::FEASIBLE) {
        std::cout << "Solution found:" << std::endl;

        std::map<std::tuple<int, int>, int64_t> scheduled;
        for (auto item : assigned) {
            auto val = SolutionIntegerValue(response, item.second);
            scheduled[item.first] = val;
        }

        for (const auto value : scheduled) {
            auto job_id = std::get<1>(value.first);
            auto object = std::find_if(
                objects.begin(), objects.end(),
                [&job_id](const Object &obj) { return obj.GetId() == job_id; });
            std::cout << "Telescope: " << std::get<0>(value.first)
                      << " Job: " << job_id << " Starts at: " << value.second
                      << " until "
                      << value.second + object->GetObservationTime() << " - "
                      << object->GetObservationTime() << std::endl;
        }

        std::cout << "Optimal Schedule Length: " << response.objective_value()
                  << std::endl;
    } else {
        std::cout << "No solution was found" << std::endl;
    }

    // Statistics
    std::cout << std::endl;
    std::cout << "Statistics" << std::endl;
    std::cout << CpSolverResponseStats(response) << std::endl;
}

std::vector<std::string> split(std::string value, int delimeter) {
    size_t start = 0;
    size_t end;
    std::string item;
    std::vector<std::string> retVal;
    while ((end = value.find(delimeter, start)) != std::string::npos) {
        item = value.substr(start, end - start);
        retVal.push_back(item);

        start = end + 1;
    }

    retVal.push_back(value.substr(start));

    return retVal;
}

void print_help() {
    std::cout << "Usage scheduler:" << std::endl;
    std::cout << "  scheduler -t <file> -s <file> [options]" << std::endl;

    int setw_size = 38;
    std::cout << "" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout
        << "  -t, --telescope <file>          Telescope configuration file"
        << std::endl;
    std::cout
        << "  -i, --import-objects <file>     File with objects to schedule"
        << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  --date <date>     Date of the observation (dd/MM/yyyy)"
              << std::endl;
    std::cout << "  --verbose         Print all logs" << std::endl;
    std::cout << "  -h, --help            Print help message" << std::endl;
    std::cout << "  -v, --version         Print version information"
              << std::endl;

    std::cout << "" << std::endl;
    std::cout << "For more information, you can take a look at \"man "
                 "./docs/scheduler\""
              << std::endl;
}

int main(int argc, char *argv[]) {
    argh::parser cmdl(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);

    bool show_help = cmdl[{"-h", "--help"}];
    if (cmdl[{"-v", "--version"}] || show_help) {
        std::cout << "Scheduder version: " << Scheduler_VERSION << std::endl;
        if (show_help) {
            print_help();
        }

        return EXIT_SUCCESS;
    }

    if (!cmdl[{"verbose"}]) {
        absl::SetMinLogLevel(absl::LogSeverityAtLeast::kWarning);
    }

    std::filesystem::path telescope_config;
    auto telescope_command = cmdl({"-t", "--telescope"});
    if (!telescope_command) {
        print_help();

        std::cout << std::endl;
        std::cout << "ERR: No telescope config file was provided" << std::endl;

        return EXIT_FAILURE;
    } else {
        telescope_config = telescope_command.str();
        if (!std::filesystem::exists(telescope_config)) {
            std::filesystem::path dir;
            if (!std::getenv("SCHEDULER_CONFIG")) {
                std::filesystem::path config =
                    ".config/scheduler" / telescope_config;
                dir = std::getenv("HOME") / config;
            } else {
                dir = std::getenv("SCHEDULER_CONFIG") / telescope_config;
            }

            if (!std::filesystem::exists(dir)) {
                std::cout << "File '" << dir << "' does not exists"
                          << std::endl;
                return EXIT_FAILURE;
            }

            telescope_config = dir;
        }
    }

    std::string objects_file;
    if (!cmdl({"-i", "--import-objects"})) {
        print_help();

        std::cout << std::endl;
        std::cout << "ERR: No objects file was provided" << std::endl;
        return EXIT_FAILURE;
    } else {
        objects_file = cmdl({"-i", "--import-objects"}).str();
        if (!std::filesystem::exists(objects_file)) {
            std::cout << "File '" << objects_file << "' does not exists"
                      << std::endl;
            return EXIT_FAILURE;
        }
    }

    mINI::INIFile file(telescope_config);
    mINI::INIStructure ini;
    file.read(ini);

    if (!ini.has("observatory")) {
        std::cout << "ERR: Telesope config: Observatory section was not found"
                  << std::endl;
        return EXIT_FAILURE;
    }

    // read a value
    if (!ini["observatory"].has("latitude")) {
        std::cout
            << "ERR: Telesope config: Observatory: latitude field was not found"
            << std::endl;
        return EXIT_FAILURE;
    }

    double latitude, longitude;
    int altitude;
    std::stringstream ini_latitude(ini["observatory"]["latitude"]);
    if (!(ini_latitude >> latitude)) {
        std::cout << "ERR: Telescope config: Observatory: Latitude field was "
                     "not valid"
                  << std::endl;
        return EXIT_FAILURE;
    }

    std::stringstream ini_longitude(ini["observatory"]["longitude"]);
    if (!(ini_longitude >> longitude)) {
        std::cout << "ERR: Telescope config: Observatory: Longitude field was "
                     "not valid"
                  << std::endl;
        return EXIT_FAILURE;
    }

    std::stringstream ini_altitude(ini["observatory"]["altitude"]);
    if (!(ini_altitude >> altitude)) {
        std::cout << "ERR: Telescope config: Observatory: Altitude field was "
                     "not valid"
                  << std::endl;
        return EXIT_FAILURE;
    }

    double min_height, min_lunar_dist, min_dec_N, min_dec_S, mount_HA;
    std::stringstream ini_limit_min_height(ini["limits"]["min_height"]);
    if (!(ini_limit_min_height >> min_height)) {
        std::cout << "ERR: Telescope config: Observatory: Min height field was "
                     "not valid"
                  << std::endl;
        return EXIT_FAILURE;
    }

    std::stringstream init_limit_min_lunar_dist(
        ini["limits"]["min_lunar_dist"]);
    if (!(init_limit_min_lunar_dist >> min_lunar_dist)) {
        std::cout
            << "ERR: Telescope config: Observatory: Min lunar dist field was "
               "not valid"
            << std::endl;
        return EXIT_FAILURE;
    }

    std::stringstream ini_limit_dec_N(ini["limits"]["min_dec_N"]);
    if (!(ini_limit_dec_N >> min_dec_N)) {
        std::cout << "ERR: Telescope config: Observatory: Min dec N field was "
                     "not valid"
                  << std::endl;
        return EXIT_FAILURE;
    }

    std::stringstream ini_limit_min_dec_S(ini["limits"]["min_dec_S"]);
    if (!(ini_limit_min_dec_S >> min_dec_S)) {
        std::cout << "ERR: Telescope config: Observatory: Min dec S field was "
                     "not valid"
                  << std::endl;
        return EXIT_FAILURE;
    }

    std::stringstream ini_limit_mount_ha(ini["limits"]["mount_HA"]);
    if (!(ini_limit_mount_ha >> mount_HA)) {
        std::cout << "ERR: Telescope config: Observatory: Min height field was "
                     "not valid"
                  << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Telescope" << std::endl;
    auto telescope = Telescope(1, latitude, longitude, altitude, "Test",
                               {
                                   min_height,
                                   min_lunar_dist,
                                   min_dec_N,
                                   min_dec_S,
                                   mount_HA,
                               });

    time_t custom_date{};
    if (cmdl({"-d", "--date"})) {
        auto tmp_date = cmdl({"-d", "--date"}).str();
        struct std::tm tm {};
        strptime(tmp_date.c_str(), "%d/%m/%Y %H:%M", &tm);
        custom_date = mktime(&tm);
    } else {
        custom_date = std::time(0);
    }

    auto today = std::localtime(&custom_date);
    today->tm_mon = today->tm_mon + 1;
    std::cout << "Date to schedule: " << today->tm_mday << "/" << today->tm_mon
              << "/" << today->tm_year + 1900 << " " << today->tm_hour << ":"
              << today->tm_min << std::endl;

    double mjdp;
    double m;
    cal_mjd(today->tm_mon, today->tm_mday, today->tm_year + 1900, &mjdp);
    mjdp += today->tm_hour / 24.;
    mjdp += today->tm_min / (24. * 60.);
    mjdp += today->tm_sec / (24. * 60. * 60.);

    std::cout << "Julian Date: " << mjdp << std::endl;

    std::vector<Object> objects;
    std::ifstream ObjectsFile(objects_file);
    std::string buf;
    srand(time(0));
    const int num_jobs = 5;
    while (std::getline(ObjectsFile, buf)) {
        auto items = split(buf, ' ');
        auto id = std::stoi(items.at(0));
        auto ra_items = split(items.at(1), ':');
        auto ra = std::stof(ra_items.at(0)) + std::stof(ra_items.at(1)) / 60 +
                  std::stof(ra_items.at(2)) / 3600;
        auto dec_items = split(items.at(2), ':');
        auto dec = std::stof(dec_items.at(0)) +
                   std::stof(dec_items.at(1)) / 60 +
                   std::stof(dec_items.at(2)) / 3600;

        int priority = rand() % 100;
        objects.push_back(Object(id, ra, dec, priority, rand() % 60));
    }

    std::vector<Telescope> telescopes;
    telescopes.push_back(telescope);

    Schedule(mjdp, telescopes, objects);

    return EXIT_SUCCESS;
}
