const std = @import("std");

const Job = struct {
    id: u8,
    time: u8,
};

pub fn main() !void {
    std.debug.print("Implementation using constraint programming\n", .{});

    var rnd = std.crypto.random;
    const jobs_count = 5;
    var jobs: [jobs_count]Job = undefined;
    for (&jobs, 0..) |*pt, i| {
        pt.* = Job{
            .id = @intCast(i),
            .time = rnd.intRangeLessThan(u8, 1, 100),
        };
    }

    std.debug.print("List of jobs:\n", .{});
    show_jobs(&jobs);

    const time_slots = std.math.maxInt(u8);
    var solution: [jobs_count]JobSolved = undefined;
    const solver = Solver.init(time_slots, &jobs, &solution);
    std.debug.print("Searching solution... {d} time slots: \n", .{time_slots});
    if (solver.solve()) {
        std.debug.print("Solution found: \n", .{});
        show_jobs_solved(&solution);
    } else {
        var sum: usize = 0;
        for (&jobs) |*pt| {
            sum += pt.time;
        }
        std.debug.print("No solution was found. Total job time is {d}\n", .{sum});
    }
}

const JobSolved = struct {
    job: *const Job,
    start: usize,
};

const Solver = struct {
    total_time: usize,
    jobs: []const Job,
    solution: []JobSolved,

    pub fn init(total_time: usize, jobs: []const Job, solved: []JobSolved) Solver {
        return .{
            .total_time = total_time,
            .jobs = jobs,
            .solution = solved,
        };
    }

    pub fn solve(self: Solver) bool {
        return self.backtrack(0, 0);
    }

    fn backtrack(self: Solver, curr_job: u8, curr_sol: usize) bool {
        if (curr_job == self.jobs.len) {
            return true;
        }

        for (0..self.total_time) |curr_time| {
            const job = &self.jobs[curr_job];
            if (!consistent(self, job, curr_sol, curr_time)) {
                continue;
            }

            self.solution[curr_sol] = JobSolved{
                .job = job,
                .start = curr_time,
            };
            if (self.backtrack(curr_job + 1, curr_sol + 1)) {
                return true;
            }

            self.solution[curr_sol] = undefined;
        }

        return false;
    }

    fn consistent(self: Solver, job: *const Job, curr_sol: usize, curr_time: usize) bool {
        for (self.solution[0..curr_sol]) |value| {
            // Assert new job will no overflow
            if (curr_time + job.time > self.total_time) {
                return false;
            }

            // Assert we are comparing different jobs
            // Assert current job is not overlapping solution
            if (value.job.id != job.id and
                // value.start < curr_time + job.time and
                value.start + value.job.time > curr_time)
            {
                return false;
            }
        }

        return true;
    }
};

fn show_jobs(jobs: []const Job) void {
    for (jobs) |value| {
        std.debug.print("Job {d} {d}\n", .{ value.id, value.time });
    }
}

fn show_jobs_solved(jobs: []const JobSolved) void {
    for (jobs) |value| {
        std.debug.print("Job {d} {d} {d}\n", .{ value.job.id, value.job.time, value.start });
    }
}
