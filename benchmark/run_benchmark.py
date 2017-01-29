#!/usr/bin/env python3

from timeit import timeit
from subprocess import run
from itertools import product
from collections import OrderedDict

thread_nums = (8,)  # most common thread numbers, I imagine
job_nums = range(0, 100000, 10000)
task_nums = (10, 100, 1000, 10000, 100000, 1000000)

# this becomes a list of tuples (thread_num, job_num, task_num)
master_list = product(thread_nums, job_nums, task_nums)

plot_dict = OrderedDict()
for group in master_list:
    print("Running with {} threads, {} jobs and {} tasks.".format(*group))
    time = timeit("run(['./benchmark', '{}', '{}', '{}'])".format(*group), globals=globals(), number=10)
    plot_list = plot_dict.get(group[2], [[], []])
    plot_list[0].append(group[1])
    plot_list[1].append(time)
    plot_dict[group[2]] = plot_list

try:
    import matplotlib.pyplot as plt
    fignum = 1
    for key, value in plot_dict.items():
        plt.figure(fignum)
        plt.title("Task Number: {}".format(key))
        plt.plot(*value, 'ro')
        plt.xlabel("job_num")
        plt.ylabel("time")
        fignum = fignum + 1
        plt.savefig("benchmark{}.png".format(key))
except ImportError:
    pass

with open('benchmark.csv', 'a') as fname:
    for key, value in plot_dict.items():
        for i in range(len(value[0])):
            fname.write('{}, {}, {}\n'.format(key, value[0][i], value[1][i]))

# this becomes a list of tuples (thread_num, task_num)
master_list = product(thread_nums, task_nums)

plot_list = [[], []]
for group in master_list:
    print("Running old with {} threads and {} tasks.".format(*group))
    time = timeit("run(['./benchmark_old', '{}', '{}'])".format(*group), globals=globals(), number=10)
    plot_list[0].append(group[1])
    plot_list[1].append(time)

try:
    import matplotlib.pyplot as plt
    plt.title("Old Benchmark")
    plt.plot(*plot_list, 'ro')
    plt.xlabel("task_num")
    plt.ylabel("time")
    plt.savefig("benchmark_old.png")
except ImportError:
    pass

with open('benchmark_old.csv', 'a') as fname:
    for i in range(len(plot_list[0])):
        fname.write('{}, {}\n'.format(plot_list[0][i], plot_list[1][i]))
