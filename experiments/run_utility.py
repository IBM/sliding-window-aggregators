import csv, time, sys, math, collections, os
from subprocess import call, Popen, PIPE

KB = 1024
MB = 1024*KB
MILLION = int(1e6)

def exec_no_fail(seq):
    p = Popen(seq, stdout=PIPE, stderr=PIPE, text=True)
    stdout, stderr = p.communicate()
    if p.returncode != 0:
        print('err: ' + stderr)
        print('ret: ' + str(p.returncode))
        print(str(seq) + ' failed.')
    return stdout

def run_latency(aggregators, functions, name_base):
    for agg in aggregators:
        for f, params in functions.items():
            base_iterations = params[0]
            window_sizes = params[1]

            for w in window_sizes:
                for d in [0, w/4]:
                    print(agg + '_' + f, w, d, ':')
                    sys.stdout.flush()
                    iterations = base_iterations + w
                    exec_no_fail(['../bin/' + name_base + '_benchmark_driver', agg, f, str(w), str(d), str(iterations), 'latency'])

def run_fifo_latency(aggregators, functions):
    for agg in aggregators:
        for f, params in functions.items():
            print(agg + '_' + f)
            base_iterations = params[0]
            window_size = params[1]
            iterations = base_iterations + window_size
            exec_no_fail(['../bin/benchmark_driver', agg, f, str(window_size), str(iterations), 'latency'])

def get_runtime(stdout):
    for line in stdout.splitlines():
        if 'core runtime: ' in line:
            return float(line.split()[2])

    raise ValueError('Could not find "core runtime" in: ' + stdout)

def exec_runtime(command):
    stdout = exec_no_fail(command)
    return get_runtime(stdout)

def run_ooo(aggregators, functions, degrees, name_base, sample_size=5):
    for agg in aggregators:
        results_file = open('results/' + name_base + '_' + agg + '.csv', 'w')
        results = csv.writer(results_file)

        for f, params in functions.items():
            print(agg + '_' + f)
            base_iterations = params[0]
            window_sizes = params[1]

            for w in window_sizes:
                for d in degrees:
                    if d > w:
                        continue
                    row = [f, w, d]
                    print(w, d, ':',)

                    iterations = base_iterations + w
                    for i in range(sample_size):
                        runtime = exec_runtime(['../bin/' + name_base + '_benchmark_driver', agg, f, str(w), str(d), str(iterations)])
                        print(runtime,)
                        sys.stdout.flush()
                        row.append(runtime)

                    print()
                    results.writerow(row)
                    results_file.flush()

        results_file.close()


def run_bulk(aggregators, functions, degrees, bulks, name_base, sample_size=5):
    for agg in aggregators:
        with open('results/' + name_base + '_' + agg + '.csv', 'w') as results_file:
            results = csv.writer(results_file)

            for f, params in functions.items():
                print(agg + '_' + f)
                base_iterations = params[0]
                window_sizes = params[1]

                for w in window_sizes:
                    for d in degrees:
                        if d > w:
                            continue

                        iterations = base_iterations + w
                        for b in bulks:
                            row = [f, w, d, b]
                            print(w, d, b, ':',)

                            for i in range(sample_size):
                                runtime = exec_runtime(['../bin/' + name_base + '_benchmark_driver', agg, f, str(w), str(d), str(b), str(iterations)])
                                print(runtime,)
                                sys.stdout.flush()
                                row.append(runtime)

                            print()
                            results.writerow(row)
                            results_file.flush()


def run_fifo(aggregators, functions, name_base, sample_size=5):
    for agg in aggregators:
        results_file = open('results/' + name_base + '_' + agg + '.csv', 'w')
        results = csv.writer(results_file)

        for f, params in functions.items():
            print(agg + '_' + f)
            base_iterations = params[0]
            window_sizes = params[1]

            for w in window_sizes:
                row = [f, w]
                print(w, ':',)

                iterations = base_iterations + w
                for i in range(sample_size):
                    runtime = exec_runtime(['../bin/benchmark_driver', agg, f, str(w), str(iterations)])
                    print(runtime,)
                    sys.stdout.flush()
                    row.append(runtime)

                print()
                results.writerow(row)
                results_file.flush()

        results_file.close()

def run_dynamic(aggregators, functions, name_base, sample_size=5):
    for agg in aggregators:
        results_file = open('results/' + name_base + '_' + agg + '.csv', 'w')
        results = csv.writer(results_file)

        for f, params in functions.items():
            print(agg + '_' + f)
            base_iterations = params[0]
            window_sizes = params[1]

            for w in window_sizes:
                row = [f, w]
                print(w, ':',)

                iterations = base_iterations + w
                for i in range(sample_size):
                    runtime = exec_runtime(['../bin/dynamic_benchmark_driver', agg, f, str(w), str(iterations)])
                    print(runtime,)
                    sys.stdout.flush()
                    row.append(runtime)

                print()
                results.writerow(row)
                results_file.flush()

        results_file.close()


def run_shared(aggregators, functions, name_base, sample_size=5):
    for agg, shared_kind in aggregators.items():
        for sk in shared_kind:
            results_file = open('results/' + name_base + '_' + agg + '_' + sk + '.csv', 'w')
            results = csv.writer(results_file)

            for f, params in functions.items():
                print(agg + '_' + f + '_' + sk)
                base_iterations = params[0]
                small_window_sizes = params[1]
                big_window_size = params[2]

                for sw in small_window_sizes:
                    row = [f, big_window_size, sw]
                    print(sw, ':',)

                    iterations = base_iterations + big_window_size
                    for i in range(sample_size):
                        runtime = exec_runtime(['../bin/' + name_base + '_benchmark_driver', agg, f, str(big_window_size), str(sw), str(iterations), sk])
                        print(runtime,)
                        sys.stdout.flush()
                        row.append(runtime)

                    print()
                    results.writerow(row)
                    results_file.flush()

            results_file.close()

def run_shared_half(aggregators, functions, window_sizes, name_base, sample_size=5):
    for agg, shared_kind in aggregators.items():
        for sk in shared_kind:
            results_file = open('results/' + name_base + '_' + agg + '_' + sk + '.csv', 'w')
            results = csv.writer(results_file)

            for f, params in functions.items():
                print(agg + '_' + f + '_' + sk)
                base_iterations = params

                for w in window_sizes:
                    big_window_size = w 
                    small_window_size = w / 2

                    row = [f, big_window_size, small_window_size]
                    print(big_window_size, small_window_size, ':',)

                    iterations = base_iterations + big_window_size
                    for i in range(sample_size):
                        runtime = exec_runtime(['../bin/' + name_base + '_benchmark_driver', agg, f, str(big_window_size), str(small_window_size), str(iterations), sk])
                        print(runtime,)
                        sys.stdout.flush()
                        row.append(runtime)

                    print()
                    results.writerow(row)
                    results_file.flush()

            results_file.close()

def run_data(aggregators, functions, durations, data_sets, name_base, latency='', sample_size=5):
    for data_set, data_file in data_sets.items():
        exp_filename = 'experiments_to_run.txt'
        with open(exp_filename, 'w') as exp_file:
            for agg in aggregators:
                for f in functions:
                    for d in durations:
                        exp_file.write(' '.join([agg, f, str(d), latency]) + '\n')

        stdout = exec_no_fail(['../bin/' + name_base + '_benchmark', exp_filename, str(sample_size), data_set, data_file])
        with open('results/' + data_set + '_' + name_base + '.log', 'w') as log:
            log.write(stdout)

