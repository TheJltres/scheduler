const std = @import("std");

const Job = struct {
    id: u8,
    time: u8,
};

pub fn main() !void {
    std.debug.print("Implementation of the Shortest Processing Time First rule\n", .{});

    var rnd = std.crypto.random;
    const jobs_count = 10;
    var jobs = init: {
        // Fill array at comptime
        var initial_value: [jobs_count]Job = undefined;
        for (&initial_value, 0..) |*pt, i| {
            pt.* = Job{
                .id = @intCast(i),
                .time = rnd.int(u8),
            };
        }

        break :init initial_value;
    };

    std.debug.print("List of jobs:\n", .{});
    show_jobs(&jobs);

    // TODO: Implement custom method?
    std.sort.block(Job, &jobs, {}, cmpByTime);
    std.debug.print("\nSPT first sorted:\n", .{});
    show_jobs(&jobs);
}

fn cmpByTime(context: void, a: Job, b: Job) bool {
    return std.sort.asc(u8)(context, a.time, b.time);
}

fn show_jobs(jobs: []const Job) void {
    for (jobs) |value| {
        std.debug.print("Job {d} {d}\n", .{ value.id, value.time });
    }
}
