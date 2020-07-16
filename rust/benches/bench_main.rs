use criterion::criterion_main;

mod benchmarks;

criterion_main![
    benchmarks::fifo_window::experiments
];
