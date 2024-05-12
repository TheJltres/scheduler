#include <absl/base/log_severity.h>
#include <absl/log/globals.h>
#include <absl/log/log.h>
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

#include "./model/angle.cc"
#include "./model/object.cc"
#include "./model/telescope.cc"

const int MAX_TIME = 480;

void Schedule(double julian_date, std::vector<Telescope> telescopes,
              std::vector<Object> objects) {
    using namespace operations_research::sat;
    std::cout << "Implementation of the Scheduder with OR-Tools" << std::endl;

    CpModelBuilder model;

    std::map<std::tuple<int, int>, IntVar> assigned;
    std::vector<BoolVar> scheduler;
    std::vector<IntervalVar> intervals;
    for (Telescope telescope : telescopes) {
        IntVar makespan =
            model.NewIntVar({0, telescope.GetObservationTime()})
                .WithName(absl::StrFormat("makespan_%d", telescope.GetId()));

        std::vector<IntVar> ends;
        std::vector<BoolVar> restrictions_global;
        for (Object object : objects) {
            auto object_start = 0;
            auto time = julian_date;
            while (object_start < MAX_TIME &&
                   !telescope.IsObjectVisible(time, object)) {
                object_start++;

                time += ((double)1 / (24 * 60));
            }

            if (object_start == MAX_TIME) {
                break;
            }

            auto object_end = object_start;
            while (object_end < MAX_TIME && telescope.IsObjectVisible(time, object)) {
                object_end++;
                time += ((double)1 / (24 * 60));
            }

            if (object_end - object_start < object.GetObservationTime()) {
                break;
            }

            std::cout << "Object added: " << object_start << " - " << object_end << " | ";
            std::cout << object.GetObservationTime() << " - " << object_end << std::endl;

            std::string suffix =
                absl::StrFormat("_%d_%d", object.GetId(), telescope.GetId());
            IntVar start = model.NewIntVar({0, telescope.GetObservationTime()})
                               .WithName(std::string("object_start") + suffix);
            IntVar end = model.NewIntVar({0, telescope.GetObservationTime()})
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
        for (Telescope t : telescopes) {
            for (Object j : objects) {
                auto key = std::make_tuple(t.GetId(), j.GetId());
                auto val = SolutionIntegerValue(response, assigned[key]);
                scheduled[key] = val;
            }
        }

        for (const auto value : scheduled) {
            auto job_id = std::get<1>(value.first);
            auto object = std::find_if(objects.begin(), objects.end(), [&job_id](const Object& obj) {return obj.GetId() == job_id;});
            std::cout << "Telescope: " << std::get<0>(value.first)
                      << " Job: " << job_id << " Starts at: " << value.second
                      << " until "
                      << value.second + object->GetObservationTime()
                      << " - " << object->GetObservationTime()
                      << std::endl;
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

void custom_test(double time, Telescope &telescope, Object &obj) {
    obj.Print();

    double moon_lat, moon_lon, moon_rho, moon_msp, moon_mdp;
    for (int i = 0; i < 60; i++) {
        std::cout << "Time: " << time + ((double)i / 24) << std::endl;

        // He de saber:
        //   - Posición de la luna
        //   - La posición del objeto
        //   - Mi ángulo visible

        // Primer test:
        // Mirar si el objeto está a + o - de 45º de radio de la luna
        // Pillar la info de la configuración del telescopio.
        moon(time + ((double)i / 24), &moon_lat, &moon_lon, &moon_rho,
             &moon_msp, &moon_mdp);

        auto separation =
            Angle::separation(moon_lat, moon_lon, obj.GetDec(), obj.GetRa());
        std::cout << "Separación: "
                  << separation.GetRadians() * separation.GetFactor()
                  << std::endl;

        // Test 2: este objeto es visible según mi rango de visión
        // Mi punto más alto es la RA y DEC del telescopio
        // Vamos a aplicar la configuración del telescopio para realizar las
        // obsevaciones.
        // 30º con respecto al horizonte
        // 90 - 30 en cada dirección de la declinación
        double diff_lat = telescope.GetLatitude() - obj.GetDec();
        double diff_lon = telescope.GetLongitude() - obj.GetRa();

        double lst;
        Now now;
        now.n_mjd = time + ((double)i / 24);
        now.n_lat = telescope.GetLatitude();
        now.n_lng = telescope.GetLongitude();
        now.n_temp = 15;
        now.n_dip = now.n_elev = now.n_tz = 0;
        now.n_pressure = 1010;
        now.n_epoch = J2000;

        now_lst(&now, &lst);
        auto lst_ra = lst - obj.GetRa();
        if (lst_ra < 4.3 || 24.0 - lst_ra < 4.3) {
            std::cout << "RA Is correct" << std::endl;
        }

        // Test 3: El objeto es visible con su DEC
        // Cuál es mi DEC? La tierra va girando
        // Preguntar
        if (obj.GetDec() < 63.0 && obj.GetDec() > -35.0) {
            std::cout << "DEC Is correct" << std::endl;
        }

        std::cout << "Telescope:" << std::endl;
        std::cout << "  LST: " << lst << std::endl;
        std::cout << "  Diff LST: " << lst_ra << std::endl;
        std::cout << "Diff:" << std::endl;
        std::cout << "  RA: " << obj.GetRa() << std::endl;
        std::cout << "  DEC: " << obj.GetDec() << std::endl;
        std::cout << std::endl;

        std::cin.ignore();
    }
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

    double latitude, longitude, altitude;
    std::stringstream iss(ini["observatory"]["latitude"]);
    if (!(iss >> latitude)) {
        std::cout << "ERR: Telescope config: Observatory: Latitude field was "
                     "not valid"
                  << std::endl;
        return EXIT_FAILURE;
    }
    std::stringstream test(ini["observatory"]["longitude"]);
    if (!(test >> longitude)) {
        std::cout << "ERR: Telescope config: Observatory: Longitude field was "
                     "not valid"
                  << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Telescope" << std::endl;
    auto telescope = Telescope(1, latitude, longitude);
    telescope.Print();

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
    cal_mjd(today->tm_mon, today->tm_mday, today->tm_year + 1900, &mjdp);
    mjdp += today->tm_hour / 24.;
    mjdp += today->tm_min / (24. * 60.);
    mjdp += today->tm_sec / (24. * 60. * 60.);

    std::cout << "Julian Date: " << mjdp << std::endl;

    std::cout << std::endl;
    double lam, bet, rho, msp, mdp;
    moon(mjdp, &lam, &bet, &rho, &msp, &mdp);

    // ??? qué son estos?
    // std::cout << rho << std::endl;
    // std::cout << msp << std::endl;
    // std::cout << mdp << std::endl;

    std::cout << std::endl;
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

        int rand_time = rand() % 100;
        objects.push_back(Object(id, ra, dec, rand_time, rand() % 100));
    }

    // custom_test(mjdp, telescope, objects[0]);

    // return EXIT_SUCCESS;

    std::vector<Telescope> telescopes;
    telescopes.push_back(telescope);

    Schedule(mjdp, telescopes, objects);

    return EXIT_SUCCESS;
}
