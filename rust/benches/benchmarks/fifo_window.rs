#[path = "../../tests/common.rs"]
mod common;
use common::*;

use alga::general::Operator;
use criterion::*;
use criterion_cpu_time::PosixTime;
use swag::reactive::Reactive;
use swag::recalc::ReCalc;
use swag::soe::SoE;
use swag::two_stacks::TwoStacks;
use swag::FifoWindow;

criterion_group! {
    name = experiments;
    config = Criterion::default()
        .with_measurement(PosixTime::UserTime);
    targets =
      bench_sum,
      bench_max
}

#[inline(always)]
fn synthesize(size: u64) -> impl Iterator<Item = Int> {
    (0..size as i64).map(Int)
}

fn run<BinOp, Window>(group: &mut BenchmarkGroup<PosixTime>, size_exp: u32, name: &str)
where
    Window: FifoWindow<Int, BinOp>,
    BinOp: Operator,
{
    let initial_size = (2 as u64).pow(size_exp);
    let initial_window = &mut Window::new();
    let batch_size = (2 as u64).pow(12);
    let group = group.throughput(Throughput::Elements(batch_size));
    // Prefill the window
    synthesize(initial_size).for_each(|item| initial_window.push(item));
    assert_eq!(initial_window.len(), initial_size as usize);
    // Begin the benchmark
    group.bench_function(BenchmarkId::new(name, size_exp), move |bench| {
        bench.iter_batched_ref(
            // Clone window before starting each experiment (not included in measurement)
            || initial_window.clone(),
            // Code that is executed before every measurement
            |window| {
                synthesize(batch_size).for_each(|item| {
                    Window::push(black_box(window), black_box(Int(item.0 % 101)));
                    Window::pop(black_box(window));
                    black_box(Window::query(black_box(window)));
                })
            },
            // The input/output window can be kept in memory
            BatchSize::SmallInput,
        )
    });
}

pub fn bench_sum(criterion: &mut Criterion<PosixTime>) {
    let group = &mut criterion.benchmark_group("Sum");
    for size_exp in 0..=22 {
        // ReCalc takes too long time for big inputs
        if size_exp < 20 {
            run::<Sum, ReCalc<_, _>>(group, size_exp, "ReCalc");
        }
        run::<Sum, Reactive<_, _>>(group, size_exp, "Reactive");
        run::<Sum, SoE<_, _>>(group, size_exp, "SoE");
        run::<Sum, TwoStacks<_, _>>(group, size_exp, "TwoStacks");
    }
}

pub fn bench_max(criterion: &mut Criterion<PosixTime>) {
    let group = &mut criterion.benchmark_group("Max");
    for size_exp in 0..=22 {
        if size_exp < 20 {
            run::<Max, ReCalc<_, _>>(group, size_exp, "ReCalc");
        }
        run::<Max, Reactive<_, _>>(group, size_exp, "Reactive");
        run::<Max, TwoStacks<_, _>>(group, size_exp, "TwoStacks");
    }
}
