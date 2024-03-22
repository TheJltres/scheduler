const std = @import("std");

const Job = struct {
    id: u8,
    time: u8,
    weight: u8,
};

pub fn main() !void {
    std.debug.print("Implementation of the Shortest Processing Time First rule\n", .{});

    var rnd = std.crypto.random;
    const jobs_count = 10;
    var jobs: [jobs_count]Job = undefined;
    for (&jobs, 0..) |*pt, i| {
        pt.* = Job{
            .id = @intCast(i),
            .time = rnd.int(u8),
            .weight = rnd.intRangeAtMost(u8, 1, std.math.maxInt(u8)),
        };
    }

    std.debug.print("List of jobs:\n", .{});
    show_jobs(&jobs);

    std.sort.heap(Job, &jobs, {}, cmpByWeight);
    std.debug.print("\nWSPT first sorted:\n", .{});
    show_jobs(&jobs);
}

fn cmpByWeight(context: void, a: Job, b: Job) bool {
    return std.sort.asc(u8)(context, a.time / a.weight, b.time / b.weight);
}

fn show_jobs(jobs: []const Job) void {
    for (jobs) |value| {
        std.debug.print("Job {d} {d} {d}\n", .{ value.id, value.time, value.weight });
    }
}
