const std = @import("std");

const Job = struct {
    id: u8,
    time: u8,
    start: usize,
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
            .start = 0,
        };
    }

    std.debug.print("List of jobs:\n", .{});
    show_jobs(&jobs);

    const time_slots = std.math.maxInt(u8);
    const solver = Solver.init(time_slots, &jobs);
    std.debug.print("Searching solution... {d} time slots: \n", .{time_slots});
    if (solver.solve()) {
        std.debug.print("Solution found: \n", .{});
        show_jobs(&jobs);
    } else {
        var sum: usize = 0;
        for (&jobs) |*pt| {
            sum += pt.time;
        }
        std.debug.print("No solution was found. Total job time is {d}\n", .{sum});
    }
}

const Solver = struct {
    total_time: usize,
    jobs: []Job,

    pub fn init(total_time: usize, jobs: []Job) Solver {
        return .{
            .total_time = total_time,
            .jobs = jobs,
        };
    }

    pub fn solve(self: Solver) bool {
        return self.backtrack(0);
    }

    fn backtrack(self: Solver, curr: usize) bool {
        if (curr == self.jobs.len) {
            return self.consistent();
        }

        for (curr..self.jobs.len) |i| {
            std.mem.swap(Job, &self.jobs[curr], &self.jobs[i]);
            if (self.backtrack(curr + 1)) {
                return true;
            }

            std.mem.swap(Job, &self.jobs[curr], &self.jobs[i]);
        }

        return false;
    }

    fn consistent(self: Solver) bool {
        var time: usize = 0;
        for (0..self.jobs.len - 1) |i| {
            const item = self.jobs[i];
            // Font allow to schedule first item until time 50 or last
            if (i != self.jobs.len - 1 and item.id == 0 and time < 50) {
                return false;
            }

            const next = self.jobs[i + 1];
            // Assert job time is sorted
            if (item.time > next.time) {
                return false;
            }

            // Don't allow schedule at time between 5 and 9
            if ((time < 5 and time + item.time > 5) or (time >= 5 and time < 10)) {
                time = 10;
            }

            self.jobs[i].start = time;
            time += item.time;
        }

        if (time > self.total_time) {
            return false;
        }

        self.jobs[self.jobs.len - 1].start = time;

        return true;
    }
};

fn show_jobs(jobs: []const Job) void {
    for (jobs) |value| {
        std.debug.print("Job {d} {d} {d}\n", .{ value.id, value.time, value.start });
    }
}
